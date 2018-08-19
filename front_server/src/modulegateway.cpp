#include "modulegateway.h"
#include "playersession.h"
#include "worldmgr.h"
#include <base/logger.h>
#include <base/gateway/gateway.h>
#include <base/memory/memorypoolmgr.h>
#include <base/configurehelper.h>
#include <base/cluster/configure.h>
#include <base/cluster/nodemonitor.h>

namespace fs
{
    /// FSGatewayEventHandler
    class FSGatewayEventHandler : public base::gateway::Gateway::EventHandler
    {
    public:
        virtual bool HandleAcceptUserClient(base::gateway::ClientPtr client) {
            auto ps = gWorldMgr->AddPlayer(client);
            return ps != nullptr;
        }

        virtual void OnUserClientClose(base::gateway::ClientPtr client) {
        }
    };

    /// ModuleGateway

    ModuleGateway* ModuleGateway::Create()
    {
        ModuleGateway* obj = new ModuleGateway();
        obj->AutoRelease();
        return obj;
    }

    ModuleGateway::ModuleGateway() : ModuleBase("fs.gateway")
    {
        AddDependentModule("dbo");
        AddDependentModule("cluster");
        AddDependentModule("lua");
        AddDependentModule("http_client");
        m_evtHandler = new FSGatewayEventHandler();
    }

    ModuleGateway::~ModuleGateway()
    {
        delete m_evtHandler;
        SAFE_DELETE(m_gateway);
        WorldMgr::Destroy();
    }

    void ModuleGateway::OnModuleSetup()
    {
        base::cluster::NodeInfo nodeInfo;
        if (!nodeInfo.ReadFromConfigFile("ms")) {
            LOG_ERROR("not found ms node configure");
            SetModuleState(base::MODULE_STATE_DELETE);
            return;
        }
        base::cluster::NodeMonitor::instance().PingNode(nodeInfo.listen_ip, nodeInfo.listen_port, true);
        
        base::ConfigureHelper helper;
        tinyxml2::XMLElement* msNode = helper.fetchConfigNodeWithServerRule("ms", 0);
        tinyxml2::XMLElement* mapServiceNode = helper.FirstChildElementWithPath(msNode, "mapService");
        if (mapServiceNode == nullptr) {
            std::cout << "mapServiceNode of ms node is not found" << std::endl;
            SetModuleState(base::MODULE_STATE_DELETE);
            return;
        }
        int id = mapServiceNode->IntAttribute("id");
        std::cout << "map id = " << id << std::endl;

        WorldMgr::Create(id);
        base::memory::MemoryPool* mempool = base::memory::g_memory_pool_mgr->Aquire("fs.gateway.mempool", 64, 256);
        if (mempool == nullptr) {
            SetModuleState(base::MODULE_STATE_DELETE);
            return;
        }
        m_gateway = new base::gateway::Gateway(*m_evtHandler, *mempool);
        if (!m_gateway->SetupByConfigFile()) {
            SetModuleState(base::MODULE_STATE_DELETE);
            return;
        }
        SetModuleState(base::MODULE_STATE_RUNNING);
    }

    void ModuleGateway::OnModuleCleanup()
    {
        SetModuleState(base::MODULE_STATE_IN_CLEANUP);
        m_gateway->StopListener();
        gWorldMgr->DisconnectAll([this]() {
            SetModuleState(base::MODULE_STATE_DELETE);
        });
    }

}
