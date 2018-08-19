#ifndef BASE_LUA_LUA_MODULE_UTILS_H
#define BASE_LUA_LUA_MODULE_UTILS_H

#include <string>

typedef struct lua_State lua_State;

namespace base
{
    namespace lua
    {
        std::string lua_utils_toJson(lua_State* L, int idx);
        void lua_utils_fromJson(lua_State* L, const char* json);

        void luaopen_utils(lua_State* L);
    }
}

#endif // LUA_MODULE_UTILS_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
