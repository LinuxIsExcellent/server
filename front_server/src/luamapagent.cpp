#include "luamapagent.h"
#include "worldmgr.h"
#include <base/cluster/message.h>
#include <base/memory/memorypoolmgr.h>
#include <../model/model/rpc/map.h>
#include <base/logger.h>
#include <base/data.h>
#include <base/cluster/nodemonitor.h>
#include <base/lua/lua_module_net.h>
#include <base/lua/luavm.h>
#include <base/lua/luacallbackmgr.h>
#include <base/lua/luaex.h>
#include <base/gateway/userclient.h>
#include <base/utils/utils_string.h>
#include <model/protocol.h>
#include "playersession.h"

namespace fs
{
    using namespace std;
    using namespace base;
    using namespace base::cluster;
    using namespace base::lua;

    static const char* ADDR_MAP = "addr_map";

    int lua_get_map_registry(lua_State* L)
    {
        return lua_rawgetp(L, LUA_REGISTRYINDEX, ADDR_MAP);
    }

    static int read_cb_val(lua_State* L, MessageIn& msgin)
    {
        base::ValueType type = static_cast<base::ValueType>(msgin.ReadVarInteger<int>());
        switch (type) {
            case base::ValueType::BOOLEAN: {
                bool v = msgin.ReadBoolean();
                lua_pushboolean(L, v);
            }
            break;
            case base::ValueType::INTEGER: {
                int64_t v = msgin.ReadVarInteger<int64_t>();
                lua_pushinteger(L, v);
            }
            break;
            case base::ValueType::DOUBLE: {
                double v = msgin.ReadDouble();
                lua_pushnumber(L, v);
            }
            break;
            case base::ValueType::STRING: {
                string v = msgin.ReadString();
                lua_pushstring(L, v.c_str());
            }
            break;
            case base::ValueType::TABLE: {
                int size = msgin.ReadVarInteger<int>();
                lua_newtable(L);
                for (int i = 0; i < size; ++i) {
                    read_cb_val(L, msgin);
                    read_cb_val(L, msgin);
                    lua_rawset(L, -3);
                }
            }
            break;
            default:
                //TODO lua error or throw and resume?
                return luaL_error(L, "read_cb_val unsupport param type =%d", type);
        }
        return 0;
    }

    static int lua_write_msgout(lua_State* L, int idx, base::cluster::MessageOut& msgout)
    {
        int type = lua_type(L, idx);
        switch (type) {
            case LUA_TBOOLEAN:
                msgout << (int)base::ValueType::BOOLEAN;
                msgout << lua_toboolean(L, idx);
                //cout << idx << " " << lua_toboolean(L, idx) << endl;
                break;
            case LUA_TNUMBER: {
                if (lua_isinteger(L, idx)) {
                    lua_Integer x = lua_tointeger(L, idx);
                    msgout << (int)base::ValueType::INTEGER;
                    msgout << (int64_t)x;
                    //cout << idx << " " << x << endl;
                } else {
                    lua_Number n = lua_tonumber(L, idx);
                    msgout << (int)base::ValueType::DOUBLE;
                    msgout << n;
                    //cout << idx << " " << n << endl;
                }
            }
            break;
            case LUA_TSTRING: {
                size_t len;
                const char* param = lua_tolstring(L, idx, &len);
                msgout << (int)base::ValueType::STRING;
                msgout << param;
                //cout << idx << " " << param << endl;
            }
            break;
            case LUA_TTABLE: {
                msgout << (int)base::ValueType::TABLE;
                int size = 0;
                lua_pushnil(L);
                while (lua_next(L, idx) != 0) {
                    ++size;
                    lua_pop(L, 1);
                }
                msgout << size;
                lua_pushnil(L);
                while (lua_next(L, idx) != 0) {
                    int top = lua_gettop(L);
                    lua_write_msgout(L, top - 1, msgout);
                    lua_write_msgout(L, top, msgout);
                    lua_pop(L, 1);
                }
            }
            break;
            default:
                return luaL_error(L, "lua_write_msgout unsupport param type %d=>%s", idx, lua_typename(L, lua_type(L, idx)));
        }
        return 0;
    }


    class RpcCallObject
    {
    public:
        rpc_callback_t fun;
        rpc_error_callback_t error_fun;
        Observer* observer = nullptr;
        RpcCallObject(const rpc_callback_t& cb, const AutoObserver& atob) : fun(cb) {
            observer = atob.GetObserver();
            observer->Retain();
        }
        RpcCallObject(const rpc_callback_t& cb, const rpc_error_callback_t& error_cb, const AutoObserver& atob) : fun(cb), error_fun(error_cb) {
            observer = atob.GetObserver();
            observer->Retain();
        }
        RpcCallObject(const rpc_callback_t& cb, const rpc_error_callback_t& error_cb, Observer* ob) : fun(cb), error_fun(error_cb) {
            observer = ob;
            observer->Retain();
        }
        ~RpcCallObject() {
            SAFE_RELEASE(observer);
        }
    };

    LuaMapAgent::LuaMapAgent(PlayerSession* ps) : m_ps(ps)
    {
        m_mailbox = Mailbox::Create(*this);
        m_ps->m_luaMapAgent = this;
    }

    LuaMapAgent::~LuaMapAgent()
    {
        QuitMap(m_service_mbid);
        QuitMap(m_other_service_mbid);

        SAFE_DELETE(m_mailbox);
        for (auto it = calls_.begin(); it != calls_.end(); ++it) {
            SAFE_DELETE(it->second);
        }
        calls_.clear();

        if (m_ps) {
            m_ps->m_luaMapAgent = nullptr;
            m_ps = nullptr;
        }
        cout << "##LuaMapAgent::~dtor" << endl;
    }

    void LuaMapAgent::EnterMap(const base::cluster::MailboxID& mbid)
    {
        // cout << "LuaMapAgent::EnterMap" << endl;
        if (mbid) {
            base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::ENTER, 32, mempool());
            msgout << m_uid;
            msgout.SetSession(0u);
            m_mailbox->Cast(mbid, msgout);
        }
    }

    void LuaMapAgent::QuitMap(const base::cluster::MailboxID& mbid)
    {
        // cout << "LuaMapAgent::QuitMap" << endl;
        if (mbid) {
            base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::QUIT, 32, mempool());
            msgout << m_uid;
            msgout.SetSession(0u);
            m_mailbox->Cast(mbid, msgout);
        }
    }

    bool LuaMapAgent::ConnectMapService(const string& service_name)
    {
        cout << "LuaMapAgent::ConnectMapService" << endl;
        if (m_mailbox == nullptr) {
            cout << "LuaMapAgent::ConnectMapService ERROR1 service_name=" << service_name << endl;
            return false;
        }

        m_service_name = service_name;

        NodeMonitor::instance().FetchNamedMailbox(service_name, [this](const MailboxID & mbid) {
            this->OnServiceUp(mbid);
        });

        NodeMonitor::instance().evt_named_mailbox_down.Attach([this](const char * name) {
            if (this->m_service_name == name) {
                this->OnServiceDown();
            }
        }, m_observer);

        if (!m_service_mbid) {
            cout << "LuaMapAgent::ConnectMapService ERROR2 service_name=" << service_name << endl;
            return false;
        }

        EnterMap(m_service_mbid);

        return true;
    }

    void LuaMapAgent::OnServiceUp(const MailboxID& mbid)
    {
        m_service_mbid = mbid;
    }

    void LuaMapAgent::OnServiceDown()
    {
        m_service_mbid.Clear();
    }

    void LuaMapAgent::Cast(MessageOut& msgout)
    {
        msgout.SetSession(0u);
        if (m_mapServiceId == m_currentJoinMap) {
            m_mailbox->Cast(m_service_mbid, msgout);
        } else {
            //g_mapMgr->CastMessage(m_currentJoinMap, m_mailbox, msgout);
        }
    }

    void LuaMapAgent::Call(MessageOut& msgout, const rpc_callback_t& cb, const base::AutoObserver& atob)
    {
        msgout.SetSession(GenSessionID());
        RpcCallObject* call = new RpcCallObject(cb, atob);
        calls_.insert(make_pair(msgout.session(), call));
        m_mailbox->Cast(m_service_mbid, msgout);
        // TODO 超时检测
    }

    void LuaMapAgent::Call(MessageOut& msgout, const rpc_callback_t& cb, const rpc_error_callback_t& error_cb, const AutoObserver& atob)
    {
        Call(msgout, cb, error_cb, atob.GetObserver());
    }

    void LuaMapAgent::Call(MessageOut& msgout, const rpc_callback_t& cb, const rpc_error_callback_t& error_cb, Observer* observer)
    {
        if (!m_service_mbid) {
            if (observer->IsExist()) {
                error_cb();
            }
            return;
        }
        msgout.SetSession(GenSessionID());
        RpcCallObject* call = new RpcCallObject(cb, error_cb, observer);
        calls_.insert(make_pair(msgout.session(), call));
        m_mailbox->Cast(m_service_mbid, msgout);
    }


    void LuaMapAgent::OnMessageReceive(base::cluster::MessageIn& msgin)
    {
        if (msgin.session() == 0u) {
            OnSubscribeArrive(msgin);
        } else {
            //cout << "OnMessageReceive session = " << msgin.session() << endl;
            auto it = calls_.find(msgin.session());
            if (it != calls_.end()) {
                RpcCallObject* co = it->second;
                if (co->observer->IsExist()) {
                    co->fun(msgin);
                }
                delete co;
                calls_.erase(it);
            }
        }
    }

    void LuaMapAgent::OnSubscribeArrive(MessageIn& msgin)
    {
        if (!m_ps) {
            LOG_ERROR("LuaMapAgent::OnSubscribeArrive m_ps == null");
            return;
        }
        if (m_ps->m_closed) {
            LOG_ERROR("LuaMapAgent::OnSubscribeArrive m_ps->m_closed");
            return;
        }

        model::rpc::MapCode code = static_cast<model::rpc::MapCode>(msgin.code());
        if (code == model::rpc::MapCode::FORWARD) {
            int code = msgin.ReadVarInteger<int>();
            std::vector<char> datas;
            uint16_t size = msgin.size() - msgin.pos();
            msgin.ReadRaw(datas, size);

            base::gateway::PacketOut pktout(code, size, mempool());
            pktout.WriteRaw(datas.data(), size);
            if (m_ps->client()) {
                m_ps->client()->Send(pktout); // m_ps.client() may be dirty pointer fixed by m_closed
            }
        } else if (code == model::rpc::MapCode::EVENT) {
            if (!m_ps->m_vm) {
                return;
            }
            int argc = msgin.ReadVarInteger<int>();
            msgin.ReadVarInteger<int>();   //读取第二个参数的类型 固定为STRING
            string func = msgin.ReadString();
            //cout << "func,argc = " << func << " " << argc << endl;
            --argc;
            lua_State* L = m_ps->m_vm->l();
            if (lua_rawgetp(L, LUA_REGISTRYINDEX, ADDR_MAP) != LUA_TTABLE) {
                LOG_ERROR("LuaMapAgent::OnMessageReceive lua_rawgetp(m_L, LUA_REGISTRYINDEX, ADDR_MAP) != LUA_TTABLE");
            }
            if (lua_rawgetp(L, -1, m_ps) != LUA_TUSERDATA) {
                LOG_ERROR("LuaMapAgent::OnMessageReceive lua_rawgetp(m_L, -1, &(m_ps)) != LUA_TUSERDATA");
            }
            lua_getuservalue(L, -1);
            if (lua_istable(L, -1)) {
                lua_pushstring(L, func.c_str());
                lua_rawget(L, -2);
                if (lua_isfunction(L, -1)) {
                    for (int i = 0; i < argc; ++i) {
                        read_cb_val(L, msgin);
                    }
                    int err = lua_pcall(L, argc, 0, 0);
                    if (err) {
                        LOG_WARN("[%s] exec fail: %s\n", func.c_str(), lua_tostring(L, -1));
                    }
                }
            }
            lua_settop(L, 0);
        }
    }



    int LuaMapAgent::_luaCall(lua_State* L)
    {
        if (LuaMapAgent* self = (LuaMapAgent*)luaL_checkudata(L, 1, MT_LUA_MAP_AGENT)) {
            int n = 2;  //2个固定参数 self, func
            const char* func = luaL_checkstring(L, 2);
            if (func) {
                //lua_yield会调用long_jump,这样msgout就不会被析构,加上{}来控制他的生命周期
                {
                    base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::LUA_CALL, 32, self->mempool());
                    //write uid
                    msgout << self->m_uid;
                    //write func
                    msgout << func;
                    int argc = lua_gettop(L);
                    //write argc
                    msgout << argc - n;
                    //write dataTable
                    for (int i = n + 1; i <= argc; ++i) {
                        msgout << (int)base::ValueType::INTEGER;
                        msgout << i - n;
                        lua_write_msgout(L, i, msgout);
                    }
                    lua_pushthread(L);
                    int callbackKey = g_lua_callback_mgr->Store(L);
                    self->Call(msgout,
                    [callbackKey](base::cluster::MessageIn & msgin) {
                        //cout << "call back" << endl;
                        lua_State* mainL = 0;
                        if (g_lua_callback_mgr->Fetch(callbackKey, &mainL, true)) {
                            lua_State* L = lua_tothread(mainL, -1);
                            int argc = msgin.ReadVarInteger<int>();
                            for (int i = 0; i < argc; ++i) {
                                read_cb_val(L, msgin);
                            }
                            //cout << "argc = " << argc << " r = " << r << endl;
                            luaEx_resume(L, mainL, argc);
                        }
                    },
                    [callbackKey]() {
                        lua_State* mainL = 0;
                        if (g_lua_callback_mgr->Fetch(callbackKey, &mainL, true)) {
                            lua_State* L = lua_tothread(mainL, -1);
                            lua_pushboolean(L, false);
                            lua_pushstring(L, "rpc error:timeout");
                            luaEx_resume(L, mainL, 2);
                        }
                    },
                    self->m_observer);
                }
                return lua_yield(L, 0);
            } else {
                LOG_ERROR("LuaMapAgent::_luaCall(lua_State* L) func null");
            }
        }
        return 0;
    }

    int LuaMapAgent::_luaCast(lua_State* L)
    {
        if (LuaMapAgent* self = (LuaMapAgent*)luaL_checkudata(L, 1, MT_LUA_MAP_AGENT)) {
            int n = 2;  //2个固定参数 self, func
            base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::LUA_CAST, 32, self->mempool());
            //write uid
            msgout << self->m_uid;
            //write func
            const char* func = luaL_checkstring(L, 2);
            if (func) {
                msgout << func;
                int argc = lua_gettop(L);
                //write argc
                msgout << argc - n;
                //write dataTable
                for (int i = n + 1; i <= argc; ++i) {
                    msgout << (int)base::ValueType::INTEGER;
                    msgout << i - n;
                    lua_write_msgout(L, i, msgout);
                }
                // check send local
                if (strcmp(func, "") == 0) {
                    if (self->m_mapServiceId == self->m_currentJoinMap) {
                        self->Cast(msgout);
                    } else {
                        const std::string name = gWorldMgr->MsIdToName(self->m_currentJoinMap);
                        const base::cluster::MailboxID* mbid = gWorldMgr->FindMsMbid(name);
                        if (mbid) {
                            // 之前浏览的游戏区要先退出地图
                            if (self->m_other_service_mbid != *mbid) {
                                self->QuitMap(self->m_other_service_mbid);
                                self->m_other_service_mbid = *mbid;
                            }
                            self->m_mailbox->Cast(*mbid, msgout);
                            //printf("LuaMapAgent::_luaCast ... name %s, mbid %d:%d, msgsize %d\n", name.c_str(), mbid->nodeid(), mbid->pid(), msgout.size());
                        }
                    }
                } else {
                    self->Cast(msgout);
                }
            }
        }
        return 0;
    }

    int LuaMapAgent::_forward(lua_State* L)
    {
        if (LuaMapAgent* self = (LuaMapAgent*)luaL_checkudata(L, 1, MT_LUA_MAP_AGENT)) {
            if (base::gateway::PacketIn* pktin = base::lua::lua_check_packet_in(L, 2)) {
                std::vector<char> datas;
                pktin->SkipTo(pktin->HEAD_SIZE);

                // 如果当前在别的地图中,转发出去,如果当前不在别的地图,则判断是否MAP_JOIN命令
                model::CS code = static_cast<model::CS>(pktin->code());
                //printf("LuaMapAgent::_forward ... code %d\n", int(code));

                if (code == model::CS::MAP_JOIN) {
                    self->m_currentJoinMap = pktin->ReadVarInteger<int>();
                    self->m_currentJoinMap = gWorldMgr->GetTrueMsId(self->m_currentJoinMap);
                    if (self->m_currentJoinMap == self->m_mapServiceId) {
                        if (self->m_other_service_mbid) {
                            self->QuitMap(self->m_other_service_mbid);
                            self->m_other_service_mbid.Clear();
                        }
                    }
                    //printf("LuaMapAgent::_forward ... MAP_JOIN %d\n", self->m_currentJoinMap);
                }

                pktin->SkipTo(pktin->HEAD_SIZE);
                uint16_t size = pktin->size() - pktin->HEAD_SIZE;
                pktin->ReadRaw(datas, size);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, pktin->size() + 4, self->mempool());
                msgout << self->m_uid;
                msgout << pktin->code();
                msgout.WriteRaw(datas.data(), size);
                if (self->m_mapServiceId == self->m_currentJoinMap) {
                    self->Cast(msgout);
                } else {
                    const std::string name = gWorldMgr->MsIdToName(self->m_currentJoinMap);
                    const base::cluster::MailboxID* mbid = gWorldMgr->FindMsMbid(name);
                    if (mbid) {
                        // 之前浏览的游戏区要先退出地图
                        if (self->m_other_service_mbid != *mbid) {
                            self->QuitMap(self->m_other_service_mbid);
                            self->m_other_service_mbid = *mbid;
                        }
                        self->m_mailbox->Cast(*mbid, msgout);
                        //printf("LuaMapAgent::_forward ... name %s, mbid %d:%d, msgsize %d\n", name.c_str(), mbid->nodeid(), mbid->pid(), msgout.size());
                    }
                }

                if (code == model::CS::MAP_LEAVE) {
                    self->m_currentJoinMap = self->m_mapServiceId;
                }
            }
        }
        return 0;
    }

    int LuaMapAgent::_setMapEvtHandler(lua_State* L)
    {
        if (LuaMapAgent* self = (LuaMapAgent*)luaL_checkudata(L, 1, MT_LUA_MAP_AGENT)) {
            luaL_checktype(L, 2, LUA_TTABLE);
            lua_setuservalue(L, 1);

            if (lua_rawgetp(L, LUA_REGISTRYINDEX, ADDR_MAP) != LUA_TTABLE) {
                lua_pop(L, 1);
                lua_newtable(L);
                lua_pushvalue(L, -1);
                lua_rawsetp(L, LUA_REGISTRYINDEX, ADDR_MAP);
            }
            lua_pushvalue(L, -2);
            lua_rawsetp(L, -2, self->m_ps);
        }
        return 0;
    }

    int LuaMapAgent::_crossTeleport(lua_State* L)
    {
        //self, k, x, y

        return 0;
    }

    int LuaMapAgent::_checkMsExist(lua_State* L)
    {
        //self, k
        bool result = false;
        if (luaL_checkudata(L, 1, MT_LUA_MAP_AGENT)) {
            int k = luaL_checkinteger(L, 2);
            if (k > 0 && gWorldMgr->IsMsUp(k)) {
                result = true;
            }
        }
        lua_pushboolean(L, result);
        return 1;
    }

    int LuaMapAgent::_quitAllMap(lua_State* L)
    {
        if (LuaMapAgent* self = (LuaMapAgent*)luaL_checkudata(L, 1, MT_LUA_MAP_AGENT)) {
            self->QuitMap(self->m_service_mbid);
            self->QuitMap(self->m_other_service_mbid);
        }
        return 0;
    }
}





