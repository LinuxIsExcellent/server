#ifndef BASE_LUA_LUAVMPOOL_H
#define BASE_LUA_LUAVMPOOL_H

#include <vector>
#include <list>
#include "luavm.h"

namespace base
{
    namespace lua
    {
        class LuaVmPool
        {
        public:
            // 服务过期时间(秒)
            static const int64_t SERVICE_EXPIRED_SECONDS;

            LuaVmPool(int id, int minSize, int maxSize);
            ~LuaVmPool();

            int id() const {
                return m_id;
            }

            LuaVm* AquireVm();

            void GenerateAllVm(const std::string& boostrapScript);

            void IntervalCheck();

        private:
            int m_id;
            int m_minSize;
            int m_maxSize;
            std::string m_boostrapScript;
            std::list<LuaVm*> m_expiredVmArray;
            std::vector<LuaVm*> m_vmArray;
        };
    }
}


#endif // LUAVMPOOL_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
