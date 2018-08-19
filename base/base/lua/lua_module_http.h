#ifndef BASE_LUA_LUA_MODULE_HTTP_H
#define BASE_LUA_LUA_MODULE_HTTP_H

typedef struct lua_State lua_State;

namespace base
{
    namespace lua
    {
        void luaopen_http(lua_State* L);
    }
}


#endif // LUA_MODULE_HTTP_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
