#ifndef BASE_LUA_LUAVM_H
#define BASE_LUA_LUAVM_H

#include <functional>
#include "../object.h"

typedef struct lua_State lua_State;

namespace base
{
    namespace lua
    {
        class LuaVmPool;

        enum class LuaVmState
        {
            INIT,
            OK,
            ERROR,
        };

        class LuaVm : public Object
        {
        public:
            LuaVm();
            ~LuaVm();

            virtual const char* GetObjectName() {
                return "base::lua::LuaVm";
            }

            lua_State* l() {
                return m_L;
            }

            LuaVmState state() const {
                return m_state;
            }

            int64_t createTime() const {
                return m_createTime;
            }

            void Bootstrap(const std::string& bootstrapScript);

            void ExecuteCachedScript(const std::string& script, int argBeginIdx);

        private:
            void setExpired() {
                m_createTime = 0;
            }
            static int _lua_bootstrap_callback(lua_State* L);

            std::string m_bootstrapScript;
            lua_State* m_L;
            LuaVmState m_state;
            int64_t m_createTime;

            friend class LuaVmPool;
        };
    }
}

#endif // LUAVM_H

// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
