#include "moduletestclient.h"
#include <base/gateway/userclient.h>
#include <base/gateway/usersession.h>
#include <base/memory/memorypoolmgr.h>
#include <base/utils/utils_string.h>
#include <base/utils/crypto.h>
#include <model/protocol.h>
#include <iomanip>
#include <unistd.h>

#include "clientSession.h"

using namespace std;
using namespace base::gateway;


class ModuleTestClientImpl : public base::gateway::UserClient::EventHandler
{
public:
    ModuleTestClientImpl(ModuleTestClient& p) : m_parent(p) {}

    virtual ~ModuleTestClientImpl() {
        SAFE_RELEASE(m_client);
        SAFE_RELEASE(m_session);
    }

    void BeginSetup() {
        base::memory::MemoryPool* mempool =  base::memory::g_memory_pool_mgr->Aquire("testclient.mempool", 60, 100);
        m_client = new UserClient(*this, *mempool);
        m_client->Connect(m_parent.m_ip.c_str(), m_parent.m_port);
    }

    void BeginCleanup() {
        if (m_client != nullptr && m_client->connect()) {
            m_isCleanup = true;
            m_client->Close();
        } else {
            m_parent.SetModuleState(base::MODULE_STATE_DELETE);
        }
    }

    virtual void OnUserClientConnect(UserClient* sender) override {
        cout << "OnUserClientConnect.." << endl;
        m_session = new MySession(sender);
        m_session->SendTemplateCheck();
        //m_session->SendLogin();

    }

    virtual void OnUserClientConnectFail(UserClient* sender, int eno, const char* reason) override {

        cout << "OnUserClientConnectFail.. reason=" << reason << endl;
    }

    virtual void OnUserClientClose(UserClient* sender) override {
        cout << "OnUserClientClose.." << endl;

        if (m_isCleanup) {
            m_parent.SetModuleState(base::MODULE_STATE_DELETE);
        }
        SAFE_RELEASE(m_client);
    }

private:
    base::gateway::UserClient* m_client = nullptr;
    MySession* m_session = nullptr;
    ModuleTestClient& m_parent;
    bool m_isCleanup = false;
};

/// ModuleTestClient
ModuleTestClient* ModuleTestClient::Create(std::string ip, int port)
{
    ModuleTestClient* obj = new ModuleTestClient(ip, port);
    obj->AutoRelease();
    return obj;
}

ModuleTestClient::ModuleTestClient(std::string ip, int port): ModuleBase("client.test_client"), m_ip(ip),m_port(port), m_impl(new ModuleTestClientImpl(*this))
{
}

ModuleTestClient::~ModuleTestClient()
{
    delete m_impl;
}

void ModuleTestClient::OnModuleSetup()
{
    m_impl->BeginSetup();
    SetModuleState(base::MODULE_STATE_RUNNING);
}

void ModuleTestClient::OnModuleCleanup()
{
    m_impl->BeginCleanup();
}
