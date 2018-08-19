#include "lua_module_vm.h"
#include <lua/lua.hpp>
#include <iostream>

namespace base
{
    namespace lua
    {
        using namespace std;

        static char ADDR_CACHED_SCRIPTS = 'c';

        static int _lua_vm_cacheScript(lua_State* L)
        {
            luaL_checktype(L, 1, LUA_TSTRING);
            const char* filename = luaL_checkstring(L, 2);
            int r = luaL_loadfile(L, filename);
            if (r != LUA_OK) {
                const char* errorMessage = lua_tostring(L, -1);
                return luaL_error(L, errorMessage);
            }
            //cout << "cache script:" << lua_tostring(L, 1) << "," << filename << endl;
            lua_rawgetp(L, LUA_REGISTRYINDEX, &ADDR_CACHED_SCRIPTS);
            lua_pushvalue(L, 1);
            lua_pushvalue(L, -3);
            lua_rawset(L, -3);
            return 0;
        }

        static int _lua_vm_executeCachedScript(lua_State* L)
        {
            int argc = lua_gettop(L);
            const char* name = luaL_checkstring(L, 1);
            lua_rawgetp(L, LUA_REGISTRYINDEX, &ADDR_CACHED_SCRIPTS);
            lua_pushvalue(L, 1);
            lua_rawget(L, -2);
            if (lua_isfunction(L, -1)) {
                int pos1 = lua_gettop(L);
                int nargs = 0;
                for (int i = 2; i <= argc; ++i) {
                    lua_pushvalue(L, i);
                    ++nargs;
                }
                int r = lua_pcall(L, nargs, LUA_MULTRET, 0);
                if (r != LUA_OK) {
                    const char* errorMessage = lua_tostring(L, -1);
                    return luaL_error(L, errorMessage);
                }
                int pos2 = lua_gettop(L);
                if (pos2 >= pos1) {
                    return pos2 - pos1 + 1;
                }
                return 0;
            } else {
                return luaL_error(L, "not found script: %s in cache", name);
            }
        }

        static int _lua_vm_loader(lua_State* L)
        {
            int idx = lua_upvalueindex(1);
            lua_pushvalue(L, idx);
            if (lua_isfunction(L, -1)) {
                int r = lua_pcall(L, 0, 1, 0);
                if (r != LUA_OK) {
                    const char* errorMessage = lua_tostring(L, -1);
                    return luaL_error(L, errorMessage);
                }
                return 1;
            } else {
                return luaL_error(L, "upvalue is not function");
            }
        }

        static int _lua_vm_searcher(lua_State* L)
        {
            const char* name = luaL_checkstring(L, 1);
            lua_rawgetp(L, LUA_REGISTRYINDEX, &ADDR_CACHED_SCRIPTS);
            lua_pushfstring(L, "%s.lua", name);
            lua_rawget(L, -2);
            if (!lua_isfunction(L, -1)) {
                lua_pushfstring(L, "\n\tno found in cache code, name=[%s.lua]", name);
            } else {
                lua_pushcclosure(L, _lua_vm_loader, 1);
            }
            return 1;
        }

        static int _lua_open_vm(lua_State* L)
        {
            // 用于存储缓存的代码
            lua_newtable(L);
            lua_pushstring(L, "list_cached_code");
            lua_setfield(L, -2, "collect_name");
            lua_rawsetp(L, LUA_REGISTRYINDEX, &ADDR_CACHED_SCRIPTS);

            luaL_Reg libFuns[] = {
                {"cacheScript", _lua_vm_cacheScript},
                {"executeCachedScript", _lua_vm_executeCachedScript},
                {"searcher", _lua_vm_searcher},
                {nullptr, nullptr}
            };

            luaL_newlib(L, libFuns);
            return 1;
        }

        int lua_vm_execute_cached_script(lua_State* L, const char* name, int argsBeginIdx)
        {
            int argc = lua_gettop(L);
            lua_rawgetp(L, LUA_REGISTRYINDEX, &ADDR_CACHED_SCRIPTS);
            lua_pushstring(L, name);
            lua_rawget(L, -2);
            if (lua_isfunction(L, -1)) {
                int pos1 = lua_gettop(L);
                int nargs = 0;
                for (int i = argsBeginIdx; i <= argc; ++i) {
                    lua_pushvalue(L, i);
                    ++nargs;
                }
                int r = lua_pcall(L, nargs, LUA_MULTRET, 0);
                if (r != LUA_OK) {
                    const char* errorMessage = lua_tostring(L, -1);
                    return luaL_error(L, errorMessage);
                }
                int pos2 = lua_gettop(L);
                if (pos2 >= pos1) {
                    return pos2 - pos1 + 1;
                }
                return 0;
            } else {
                return luaL_error(L, "not found script: %s in cache", name);
            }
        }

        void luaopen_vm(lua_State* L)
        {
            luaL_requiref(L, "vm", &_lua_open_vm, 0);
            lua_pop(L, 1);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
