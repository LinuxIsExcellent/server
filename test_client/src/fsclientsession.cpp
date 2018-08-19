#include "fsclientsession.h"
#include <base/gateway/userclient.h>
#include <base/gateway/packet.h>
#include <base/utils/utils_string.h>
#include "lua/luaer.h"
#include <model/protocol.h>
#include <base/lua/lua_module_net.h>
#include <base/logger.h>
#include <base/lua/lua_module_net.h>

static const char* META_FS_SESSION = "meta_fs_session";

namespace ts
{
    using namespace lua;
    using namespace std;
    using namespace base::gateway;
    using namespace model;

    static int _test(lua_State* L)
    {
        cout << "_test" << endl;
        return 0;
    }

    static int _lua_new_pktout(lua_State* L)
    {
        //self, size
        if (FSClientSession* s = (FSClientSession*)luaL_checkudata(L, 1, META_FS_SESSION)) {
            uint16_t code = luaL_checkinteger(L, 2);
            size_t size = luaL_checkinteger(L, 3);
            base::lua::lua_net_new_pktout(s->client(), L, code, size);
            return 1;
        }
        return 0;
    }

    static int _lua_send_pktout(lua_State* L)
    {
        //pktout
        if (FSClientSession* s = (FSClientSession*)luaL_checkudata(L, 1, META_FS_SESSION)) {
            PacketOut* pktout = (PacketOut*)lua_touserdata(L, 2);
            s->Send(*pktout);
        }
        return 0;
    }

    FSClientSession::FSClientSession(base::gateway::UserClient* client) : UserSession(client)
    {
        luaer_ = new Luaer();
        luaer_->Setup();

        luaL_Reg func[] = {
            {"test", _test},
            {"newPktout", _lua_new_pktout},
            {"send", _lua_send_pktout},
            {nullptr, nullptr}
        };

        lua_State* L = luaer_->L();
        lua_pushlightuserdata(L, this);
        int ok = luaL_newmetatable(L, META_FS_SESSION);
        assert(ok == 1);
        luaL_setfuncs(L, func, 0);
        lua_pushstring(L, "__index");
        lua_pushvalue(L, -2);
        lua_rawset(L, -3);

        lua_setmetatable(L, -2);
        lua_pop(L, 1);
    }

    FSClientSession::~FSClientSession()
    {
        SAFE_DELETE(luaer_);
    }

    void FSClientSession::OnUserClientReceivePacket(PacketIn& pktin)
    {
        SC code = (SC)pktin.code();
        if (code != SC::PING) {
            cout << "c recv code = " << pktin.code() << endl;
        }
        switch (code) {
            case SC::PING: {
                int64_t v1 = 0;
                pktin.ReadVarInteger(&v1);
                PacketOut pktout((uint16_t)CS::PING_REPLY, 16, client()->mempool());
                pktout.WriteVarInteger(v1);
                client()->Send(pktout);
            }
            break;
            case SC::PING_RESULT: {
                int64_t v1 = 0;
                pktin.ReadVarInteger(&v1);
                if (v1 > 100) cout << "delay = " << v1 << endl;
            }
            break;
            case SC::TEMPLATE_UPDATE: {
                int64_t v1 = 0;
                pktin.ReadVarInteger(&v1);
                cout << "progress = " << v1;
                pktin.ReadVarInteger(&v1);
                cout << " total = " << v1 << endl;
                if (v1 > 0) {
                    string name = pktin.ReadString();
                    cout << "name = " << name;
                    pktin.ReadVarInteger(&v1);
                    cout << " partProgress = " << v1;
                    int64_t v2 = 0;
                    pktin.ReadVarInteger(&v2);
                    cout << " partTotal = " << v2;
                    string content = pktin.ReadString();
                    //str = base::utils::zlib_uncompress(str);
                    //cout << " content = " << str;
                    if (name == "map_unit.json") {
                        static string test;
                        test += content;
                        if (v1 == v2) {
                            string un = base::utils::zlib_uncompress(test);
                            cout << " un = " << un;
                        }
                    }
                    cout << endl;
                }
            }
            break;
            /*case 23333: {
                bool a = 0;
                pktin >> a;
                int8_t b = pktin.ReadInt8();
                uint8_t c = pktin.ReadUInt8();
                int16_t d = pktin.ReadInt16();
                uint16_t e = pktin.ReadUInt16();
                int32_t f = pktin.ReadInt32();
                uint32_t g = pktin.ReadUInt32();
                int64_t h = pktin.ReadInt64();
                uint64_t i = pktin.ReadUInt64();
                float j = 0.0;
                pktin >> j;
                double k = 0.0;
                pktin >> k;
                string str;
                pktin >> str;

                uint64_t l = 0;
                pktin >> l;

                cout << a << endl;
                cout << (int32_t)b << endl;
                cout << (int32_t)c << endl;
                cout << d << endl;
                cout << e << endl;
                cout << f << endl;
                cout << g << endl;
                cout << h << endl;
                cout << i << endl;
                cout << j << endl;
                cout << k << endl;
                cout << str << endl;
                cout << l << endl;
            }
            break;*/
            default: {
                lua_State* L = luaer_->L();
                lua_getglobal(L, "recvPkt");
                base::lua::lua_net_push_packet_in(L, pktin);
                int err = lua_pcall(L, 1, 0, 0);
                if (err) {
                    LOG_WARN("[%s] exec fail: %s\n", "recvPkt", lua_tostring(L, -1));
                }
                return;
            }
        }
    }

    void FSClientSession::OnUserClientClose()
    {
        //不会被调用
        cout << "FSClientSession::OnUserClientClose" << endl;
    }

    void FSClientSession::SetMetatable(lua_State* L)
    {
        lua_pushlightuserdata(L, this);
        luaL_newmetatable(L, META_FS_SESSION);
        lua_setmetatable(L, -2);
    }

}
