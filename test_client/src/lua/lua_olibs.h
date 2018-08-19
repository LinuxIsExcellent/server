#ifndef LUA_OLIBS_H
#define LUA_OLIBS_H

typedef struct lua_State lua_State;

namespace lua
{
    void luaopen_olibs( lua_State* L);
}

#endif // LUA_OLIBS_H
