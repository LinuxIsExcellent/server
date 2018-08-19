#include "moduledbo.h"
#include "../memory/memorypool.h"
#include "../logger.h"
#include "dbpool.h"
#include "../event/dispatcher.h"
#include <functional>

namespace base
{
    namespace dbo
    {
        using namespace std;

        ModuleDBO::ModuleDBO()
            : ModuleBase("dbo"), dbpool_(nullptr), linker_(nullptr)
        {
            AddDependentModule("threadpool");
        }

        ModuleDBO::~ModuleDBO()
        {
            SAFE_DELETE(dbpool_);
            SAFE_RELEASE(linker_);
        }

        ModuleDBO* ModuleDBO::Create()
        {
            ModuleDBO* ptr = new ModuleDBO();
            ptr->AutoRelease();
            return ptr;
        }

        void ModuleDBO::OnModuleSetup()
        {
            SetModuleState(MODULE_STATE_IN_SETUP);
            dbpool_ = new DBPool();
            if (!dbpool_->BeginSetup()) {
                LOG_ERROR("<module>.dbo setup fail..\n");
                SetModuleState(MODULE_STATE_DELETE);
            } else {
                // 轮询连接状态, 要求每个池至少有一个可用连接
                SAFE_RELEASE(linker_);
                linker_ = g_dispatcher->quicktimer().SetIntervalWithLinker(std::bind(&ModuleDBO::CheckIfSetupFinish,
                          this), 100);
                linker_->Retain();
            }
        }

        void ModuleDBO::CheckIfSetupFinish()
        {
            vector<const StatisticInfo*> stats = dbpool_->UpdateAndFetchStatistics();
            for (vector<const StatisticInfo*>::iterator it = stats.begin(); it != stats.end(); ++it) {
                if ((*it)->ActiveCount() == 0) {
                    return;
                }
            }
            SAFE_RELEASE(linker_);
            SetModuleState(MODULE_STATE_RUNNING);
        }

        void ModuleDBO::OnModuleCleanup()
        {
            SetModuleState(MODULE_STATE_IN_CLEANUP);
            dbpool_->BeginCleanup();
            // 轮询所有连接都已经关闭
            SAFE_RELEASE(linker_);
            linker_ = g_dispatcher->quicktimer().SetIntervalWithLinker(std::bind(&ModuleDBO::CheckIfCleanupFinish,
                      this), 100);
            linker_->Retain();
        }

        void ModuleDBO::CheckIfCleanupFinish()
        {
            vector<const StatisticInfo*> stats = dbpool_->UpdateAndFetchStatistics();
            for (vector<const StatisticInfo*>::iterator it = stats.begin(); it != stats.end(); ++it) {
                if (!(*it)->IsEmpty()) {
                    return;
                }
            }
            SAFE_RELEASE(linker_);
            SetModuleState(MODULE_STATE_DELETE);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
