#ifndef BASE_LUA_LUA_MODULE_TIMER_H
#define BASE_LUA_LUA_MODULE_TIMER_H

typedef struct lua_State lua_State;

namespace base
{
    namespace lua
    {
        void luaopen_timer(lua_State* L);
    }
}

#endif // LUA_MODULE_TIMER_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
