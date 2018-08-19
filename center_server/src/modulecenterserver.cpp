#include "modulecenterserver.h"
#include <base/cluster/configure.h>
#include <base/cluster/nodemonitor.h>
#include <base/logger.h>
#include <base/event/dispatcher.h>
#include <malloc.h>

namespace cs
{
    using namespace std;

    ModuleCenterServer* ModuleCenterServer::Create()
    {
        ModuleCenterServer* obj = new ModuleCenterServer();
        obj->AutoRelease();
        return obj;
    }

    ModuleCenterServer::ModuleCenterServer() : ModuleBase("cs.server")
    {
        AddDependentModule("cluster");
        AddDependentModule("lua");
        AddDependentModule("http_client");
    }

    ModuleCenterServer::~ModuleCenterServer()
    {

    }

    void ModuleCenterServer::OnModuleSetup()
    {
        base::cluster::NodeInfo msInfo;
        if (!msInfo.ReadFromConfigFile("ms")) {
            LOG_ERROR("NOT FOUND MS NODE CONFIGURE");
            SetModuleState(base::MODULE_STATE_DELETE);
            return;
        }
        
        SetModuleState(base::MODULE_STATE_RUNNING); 
        base::cluster::NodeMonitor::instance().PingNode(msInfo.listen_ip, msInfo.listen_port, true);
        
        g_dispatcher->quicktimer().SetInterval(std::bind(&ModuleCenterServer::OnTimerUpdate, this), 1800 * 1000, m_autoObserver);
    }

    void ModuleCenterServer::OnModuleCleanup()
    {
        m_autoObserver.SetNotExist();
        SetModuleState(base::MODULE_STATE_DELETE);
    }
    
    void ModuleCenterServer::OnTimerUpdate()
    {
        malloc_trim(4096);
    }

}
