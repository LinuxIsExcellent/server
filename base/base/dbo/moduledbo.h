#ifndef BASE_DBO_MODULEDBO_H
#define BASE_DBO_MODULEDBO_H

#include "../modulebase.h"
#include "../timer.h"

namespace base
{
    namespace dbo
    {
        class DBPool;
        class ModuleDBO : public ModuleBase
        {
        public:
            static ModuleDBO* Create();
            virtual ~ModuleDBO();

            virtual const char* GetObjectName() {
                return "base::dbo::ModuleDBO";
            }

        private:
            ModuleDBO();

            virtual void OnModuleSetup();
            virtual void OnModuleCleanup();

            void CheckIfSetupFinish();
            void CheckIfCleanupFinish();

            DBPool* dbpool_;
            TimeoutLinker* linker_;
        };
    }
}

#endif // MODULEDBO_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
