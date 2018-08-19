#ifndef BASE_LUA_LUACALLBACKMGR_H
#define BASE_LUA_LUACALLBACKMGR_H

#include "../global.h"
#include "../observer.h"
#include <unordered_map>

typedef struct lua_State lua_State;

namespace base
{
    namespace lua
    {
        struct CallbackObject;
        class LuaCallbackMgr
        {
        public:
            static LuaCallbackMgr* Create();
            static void Delete();

        public:
            // store the value at the top of the stack as callback object, then pop it [-1, +0, -]
            int Store(lua_State* L);

            // fetch callback object with key, if found, return true,
            // the callback object with push on to the stack, otherwise return false [-0, +(0|1), e]
            bool Fetch(int key, lua_State** mainL, bool cleanup);

            // fetch observer of L
            Observer* GetObserver(lua_State* L);

        private:
            LuaCallbackMgr();
            ~LuaCallbackMgr();

            int GenerateKey() {
                int key = m_idx++;
                if (key == 0) {
                    key = m_idx++;
                }
                return key;
            }
            int m_idx = 0;
            std::unordered_map<int, CallbackObject*> m_callbackObjs;
        };

        extern LuaCallbackMgr* g_lua_callback_mgr;
    }
}


#endif // LUACALLBACKMGR_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
