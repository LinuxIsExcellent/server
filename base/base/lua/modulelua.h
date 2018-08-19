#ifndef BASE_LUA_MODULELUA_H
#define BASE_LUA_MODULELUA_H

#include "../modulebase.h"
#include "../observer.h"
#include "../timer.h"

namespace base
{
    namespace lua
    {

        class LuaVm;
        class LuaVmPool;

        class ModuleLua : public ModuleBase
        {
        public:
            virtual ~ModuleLua();
            static ModuleLua* Create();

            LuaVmPool* GetVmPool(int poolId);
            LuaVm* AquireVm(int poolId);

        private:
            ModuleLua();
            virtual void OnModuleSetup();
            virtual void OnModuleCleanup();

            void IntervalCheck();

        private:
            // 定时检查时间间隔(毫秒）
            static const int64_t CHECK_INTERVAL_TICK;
            TimeoutLinker* linker_;

            std::vector<LuaVmPool*> m_pools;
        };
    }
}

extern base::lua::ModuleLua* g_module_lua;

#endif // MODULELUA_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
