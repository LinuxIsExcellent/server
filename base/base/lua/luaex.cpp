#include "luaex.h"
#include "../logger.h"
#include <lua.hpp>
#include <iostream>

namespace base
{
    namespace lua
    {
        using namespace std;

        lua_State* luaEx_get_main_thread(lua_State* L)
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);
            lua_State* mainL = lua_tothread(L, -1);
            lua_pop(L, 1);
            return mainL;
        }

        int luaEx_dump_stack(lua_State* L)
        {
            int i;
            int r = -1;
            int top = lua_gettop(L);
            cout << "================dumpStack==============" << endl;
            for (i = top; i > 0; i--) {
                int type = lua_type(L, i);
                switch (type) {
                    case LUA_TNUMBER:
                        printf("  %d(%d) = number %g\n", i, r, lua_tonumber(L, i));
                        break;
                    case LUA_TSTRING:
                        printf("  %d(%d) = string '%s'\n", i, r, lua_tostring(L, i));
                        break;
                    case LUA_TBOOLEAN:
                        printf("  %d(%d) = boolean %s\n", i, r, lua_toboolean(L, i) ? "true" : "false");
                        break;
                    case LUA_TTABLE:
                    case LUA_TLIGHTUSERDATA:
                    case LUA_TUSERDATA: {
                        if (lua_getmetatable(L, i) == 1) {
                            lua_pushstring(L, "__name");
                            lua_rawget(L, -2);
                            const char* meta = lua_tostring(L, -1);
                            printf("  %d(%d) = %s (%s)\n", i, r, lua_typename(L, type), meta);
                            lua_pop(L, 2);
                        } else {
                            printf("  %d(%d) = %s\n", i, r, lua_typename(L, type));
                        }
                    }
                    break;
                    case LUA_TTHREAD: {
                        int isMain = lua_pushthread(L);
                        lua_pop(L, 1);
                        printf("  %d(%d) = %s main=%s\n", i, r, lua_typename(L, type), isMain ? "true" : "false");
                    }
                    break;
                    default:
                        printf("  %d(%d) = %s\n", i, r, lua_typename(L, type));
                        break;
                }
                r--;
            }
            cout << "=======================================" << endl;
            return 0;
        }

        int luaEx_resume(lua_State* L, lua_State* from, int nargs)
        {
            int r = lua_resume(L, from, nargs);

            if (r != LUA_OK && r != LUA_YIELD) {
                const char* errorMessage = lua_tostring(L, -1);
                LOG_ERROR("luaEx resume.error: %s\n", errorMessage);
                lua_pop(L, 1);
            }
            return r;
        }

        int luaEx_pcall(lua_State* L, int nargs, int nresults, int msgh)
        {
            int r = lua_pcall(L, nargs, nresults, msgh);
            if (r != LUA_OK) {
                const char* errorMessage = lua_tostring(L, -1);
                LOG_ERROR("luaEx pcall.error: %s\n", errorMessage);
                lua_pop(L, 1);
            }
            return r;
        }

        void luaEx_createPrivateTableOnRegistry(lua_State* L, void* addr, const char* name, const char* weakFlag)
        {
            // 在registry上创建用于存放私有数据的table
            lua_newtable(L);
            if (name && strlen(name) > 0) {
                lua_pushstring(L, name);
                lua_setfield(L, -2, "collect_name");
            }
            lua_pushvalue(L, -1);
            if (weakFlag && strlen(weakFlag) > 0) {
                lua_pushstring(L, "__mode");
                lua_pushstring(L, weakFlag);
                lua_rawset(L, -3);
            }
            lua_setmetatable(L, -2);
            lua_rawsetp(L, LUA_REGISTRYINDEX, addr);
        }

        int luaEx_getPrivateTableOnRegistry(lua_State* L, void* addr)
        {
            return lua_rawgetp(L, LUA_REGISTRYINDEX, addr);
        }

        StackKeeper::StackKeeper(lua_State* L)
        {
            m_L = L;
            m_oldIndex = lua_gettop(m_L);
        }

        StackKeeper::~StackKeeper()
        {
            lua_settop(m_L, m_oldIndex);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
