#include "modulegateway.h"
#include <base/logger.h>
#include <base/gateway/userclient.h>
#include <base/gateway/usersession.h>
#include <base/memory/memorypoolmgr.h>
#include "fsclientsession.h"
#include "aio.h"
#include "lua/luaer.h"
#include <base/utils/utils_string.h>
#include <base/lua/luavm.h>
#include <base/framework.h>
#include <model/protocol.h>
#include "fsclientsession.h"
#include <base/event/dispatcher.h>
#include <base/objectmaintainer.h>

namespace ts
{
    using namespace std;
    using namespace base::gateway;
    using namespace base::lua;

    /// TCAioHandler
    class TCAioHandler : public Aio::AioHandler
    {
    public:
        TCAioHandler(UserClient* client) : client_(client) {}
        virtual ~TCAioHandler() {
            client_ = nullptr;
        }
        virtual void AioHandle(const std::string& str) override {
            if (!client_) return;
            if (str[0] == '@') {
                if (FSClientSession* session = static_cast<FSClientSession*>(client_->session())) {
                    lua_State* L = session->luaer()->L();
                    lua_getglobal(L, "commandHandle");
                    lua_pushstring(L, str.c_str());
                    session->SetMetatable(L);
                    int err = lua_pcall(L, 2, 0, 0);
                    if (err) {
                        LOG_WARN("[%s] exec fail: %s\n", "commandHandle", lua_tostring(L, -1));
                    }
                }
                return;
            } else if (str == "#packs") {
                maintainer_.Add(
                g_dispatcher->quicktimer().SetIntervalWithLinker([&]() {
                    PacketOut pktout(214, 32, client_->mempool());
                    pktout << 3 << 1001001 << 1 << true;
                    client_->Send(pktout);
                }, 10));
            } else if (str == "#testCall") {
                maintainer_.Add(
                g_dispatcher->quicktimer().SetIntervalWithLinker([&]() {
                    PacketOut pktout(9999, 32, client_->mempool());
                    client_->Send(pktout);
                }, 10));
            } else if (str == "#march") {
                maintainer_.Add(
                g_dispatcher->quicktimer().SetIntervalWithLinker([&]() {
                    PacketOut pktout(1016, 32, client_->mempool());
                    pktout << 1 << 599 << 599 << 1 << 1003010 << 1 << 0 << 0 << 0 << false;
                    client_->Send(pktout);
                }, 10));
            } else {
                vector<string> datas = base::utils::string_split(str, ' ', false);
                if (datas.empty()) return;
                int32_t code = 0;
                try {
                    code = atoi(datas[0].c_str());
                } catch (std::exception& e) {
                    cout << "####input error, u must input code first####" << endl;
                    return;
                }
                PacketOut pktout(code, 32, client_->mempool());
                cout << "code:" << datas[0] << ",";
                for (size_t i = 1; i < datas.size(); ++i) {
                    if (datas[i] == "0") {
                        pktout.WriteVarInteger(0);
                        cout << "i:" << 0 << ",";
                        continue;
                    }
                    string& data = datas[i];
                    try {
                        int64_t r = atol(data.c_str());
                        pktout.WriteVarInteger(r);
                        cout << "i:" << r << ",";
                    } catch (std::exception& e) {
                        try {
                            float r = atof(data.c_str());
                            pktout.WriteFloat(r);
                            cout << "f:" << r << ",";
                        } catch (std::exception& e) {
                            pktout.WriteString(data);
                            cout << "s:" << data << ",";
                        }
                    }
                }
                cout << endl;
                client_->Send(pktout);
            }
        }

    private:
        UserClient* client_ = nullptr;
        base::ObjectMaintainer maintainer_;
    };

    /// FSUserClientEventHandler
    struct FSUserClientEventHandler : public base::gateway::UserClient::EventHandler {
        virtual ~FSUserClientEventHandler() {
            SAFE_DELETE(aiohandler_);
        }

        virtual void OnUserClientConnect(UserClient* sender) override {
            cout << "OnUserClientConnect" << endl;
            /*PacketOut pktout(23333, 32, sender->mempool());
            pktout << true;
            pktout.WriteInt8(233);
            pktout.WriteUInt8(233);
            pktout.WriteInt16(23333);
            pktout.WriteUInt16(23333);
            pktout.WriteInt32(23333333);
            pktout.WriteUInt32(23333333);
            pktout.WriteInt64(23333333333);
            pktout.WriteUInt64(33333333333);
            pktout << (float) 23333.33;
            pktout << (double) 2333.33;
            pktout << "2333333333333333333333333333哈哈哈哈哈哈";
            pktout << 2333;

            sender->Send(pktout);*/

            //aio setup
            aiohandler_ = new TCAioHandler(sender);
            Aio::instance().Setup(aiohandler_);
        }
        virtual void OnUserClientConnectFail(UserClient* sender, int eno, const char* reason) override {
            cout << "OnUserClientConnectFail" << endl;
        }
        virtual void OnUserClientClose(UserClient* sender) override {
            cout << "OnUserClientClose" << endl;
        }

        TCAioHandler* aiohandler_ = nullptr;
    };

    /// ModuleGateway
    ModuleGateway* ModuleGateway::Create()
    {
        ModuleGateway* obj = new ModuleGateway();
        obj->AutoRelease();
        return obj;
    }

    ModuleGateway::ModuleGateway() : ModuleBase("ts.gateway")
    {
    }

    ModuleGateway::~ModuleGateway()
    {
        SAFE_DELETE(m_handle);
        SAFE_DELETE(m_session);
        SAFE_DELETE(m_client);
    }

    void ModuleGateway::OnModuleSetup()
    {
        base::memory::MemoryPool* mempool = base::memory::g_memory_pool_mgr->Aquire("ts.gateway.mempool", 64, 256);
        if (mempool == nullptr) {
            SetModuleState(base::MODULE_STATE_DELETE);
            return;
        }
        m_handle = new FSUserClientEventHandler();
        m_client = new base::gateway::UserClient(*m_handle, *mempool);
        //m_client->Connect("192.168.1.55", 5100);
        m_client->Connect("127.0.0.1", 8100);
        m_session = new FSClientSession(m_client);
        SetModuleState(base::MODULE_STATE_RUNNING);
    }

    void ModuleGateway::OnModuleCleanup()
    {
        if (m_client && m_client->connect()) {
            m_client->Close();
        } else {
            SetModuleState(base::MODULE_STATE_DELETE);
        }
    }

}

