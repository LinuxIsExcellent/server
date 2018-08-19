#ifndef BASE_LUA_LUA_MODULE_DBO_H
#define BASE_LUA_LUA_MODULE_DBO_H

typedef struct lua_State lua_State;

namespace base
{
    namespace lua
    {
        void luaopen_dbo(lua_State* L);
    }
}

#endif
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
