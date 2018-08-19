#include "modulethreadpool.h"
#include "threadpool.h"

namespace base
{
    namespace thread
    {
        ModuleThreadPool* ModuleThreadPool::Create()
        {
            ModuleThreadPool* obj = new ModuleThreadPool;
            obj->AutoRelease();
            return obj;
        }

        ModuleThreadPool::ModuleThreadPool(): ModuleBase("threadpool")
        {
        }

        ModuleThreadPool::~ModuleThreadPool()
        {
            SAFE_DELETE(thread_pool_);
        }

        void ModuleThreadPool::OnModuleSetup()
        {
            thread_pool_ = new ThreadPool(4, 8);
            thread_pool_->start();
            SetModuleState(MODULE_STATE_RUNNING);
        }

        void ModuleThreadPool::OnModuleCleanup()
        {
            thread_pool_->stop();
            SAFE_DELETE(thread_pool_);
            SetModuleState(MODULE_STATE_DELETE);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
