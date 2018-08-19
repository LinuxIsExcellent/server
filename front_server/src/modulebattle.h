#ifndef MAP_MODULEBATTLE_H
#define MAP_MODULEBATTLE_H
#include <base/modulebase.h>

namespace fs
{
        class ModuleBattle : public base::ModuleBase
        {
        public:
            static ModuleBattle* Create();
            virtual ~ModuleBattle();

        private:
            ModuleBattle();
            virtual void OnModuleSetup() override;
            virtual void OnModuleCleanup() override;
        };
}

#endif // ModuleBattle
