#include "lua_olibs.h"
#include <base/3rd/lua/lua.hpp>
#include <iostream>

namespace lua
{
    using namespace std;
    
    int _lua_test(lua_State* L)
    {
        cout << "lua_test" << endl;
        return 0;
    }
    
    int _lua_count_table(lua_State* L)
    {
        size_t count = 0;
        lua_pushnil(L);
        while(lua_next(L, 1) != 0) {
            ++count;
            lua_pop(L, 1);
        }
        lua_pushinteger(L, count);
        return 1;
    }
    
    int _lua_open_olibs(lua_State* L)
    {
        luaL_Reg list[] = {
            {"test", _lua_test},
            {"countTable", _lua_count_table},
            {nullptr, nullptr}
        };
        luaL_newlib(L, list);
        return 1;
    }
    
    void luaopen_olibs(lua_State* L)
    {
        luaL_requiref(L, "olibs", &_lua_open_olibs, 0);
        lua_pop(L, 1);
    }
}
