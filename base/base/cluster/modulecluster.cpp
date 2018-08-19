#include "modulecluster.h"
#include "nodemonitor.h"
#include <functional>

namespace base
{
    namespace cluster
    {
        ModuleCluster::ModuleCluster()
            : ModuleBase("cluster")
        {
            AddDependentModule("dbo");
        }

        ModuleCluster::~ModuleCluster()
        {
            NodeMonitor::DestroyInstance();
        }

        ModuleCluster* ModuleCluster::Create()
        {
            ModuleCluster* obj = new ModuleCluster();
            obj->AutoRelease();
            return obj;
        }

        void ModuleCluster::OnModuleSetup()
        {
            SetModuleState(MODULE_STATE_IN_SETUP);
            NodeMonitor::CreateInstance();
            NodeMonitor::instance().BeginSetup(std::bind(&ModuleCluster::NodeMonitorSetupCallback, this, std::placeholders::_1));
        }

        void ModuleCluster::OnModuleCleanup()
        {
            SetModuleState(MODULE_STATE_IN_CLEANUP);
            NodeMonitor::instance().BeginCleanup(std::bind(&ModuleCluster::NodeMonitorCleanupCallback, this));
        }

        void ModuleCluster::NodeMonitorSetupCallback(bool result)
        {
            if (result) {
                SetModuleState(MODULE_STATE_RUNNING);
            } else {
                SetModuleState(MODULE_STATE_DELETE);
            }
        }

        void ModuleCluster::NodeMonitorCleanupCallback()
        {
            SetModuleState(MODULE_STATE_DELETE);
        }
    }
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
