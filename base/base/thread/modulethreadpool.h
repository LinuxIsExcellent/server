#ifndef BASE_MODULE_MODULETHREADPOOL_H
#define BASE_MODULE_MODULETHREADPOOL_H

#include "../modulebase.h"

namespace base
{
    namespace thread
    {
        class ThreadPool;

        class ModuleThreadPool : public ModuleBase
        {
        public:
            static ModuleThreadPool* Create();

            ModuleThreadPool();
            virtual ~ModuleThreadPool();

            virtual const char* GetObjectName() {
                return "base::thread::ModuleThreadPool";
            }

        private:
            virtual void OnModuleSetup();
            virtual void OnModuleCleanup();

            ThreadPool* thread_pool_ = nullptr;
        };
    }
}

#endif // MODULETHREADPOOL_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
