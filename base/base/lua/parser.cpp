#include "parser.h"
#include "lua.hpp"
#include "../logger.h"

namespace base
{
    namespace lua
    {
        using namespace std;

        // TODO we need to create a lua State to handle all parse job, avoid create lua state every time

        vector<int> Parser::ParseAsIntArray(const std::string& code, bool* hasError /* = nullptr */)
        {
            vector<int> ret;

            string program = "return ";
            program.append(code);

            lua_State* L = luaL_newstate();

            int error = luaL_dostring(L, program.c_str());
            if (hasError != nullptr) {
                if (error) {
                    *hasError = true;
                    LOG_ERROR("parse lua code with error: %s\n", lua_tostring(L, -1));
                } else {
                    *hasError = false;
                }
            }

            if (!error) {
                if (lua_istable(L, -1)) {
                    int size = lua_rawlen(L, -1);
                    for (int i = 1; i <= size; ++i) {
                        lua_rawgeti(L, -1, i);
                        int v = lua_tointeger(L, -1);
                        lua_pop(L, 1);
                        ret.push_back(v);
                    }
                }
            }

            lua_close(L);

            return ret;
        }

        vector< string > Parser::ParseAsStringArray(const string& code, bool* hasError /* = nullptr */)
        {
            vector<string> ret;

            string program = "return ";
            program.append(code);

            lua_State* L = luaL_newstate();

            int error = luaL_dostring(L, program.c_str());
            if (hasError != nullptr) {
                if (error) {
                    *hasError = true;
                    LOG_ERROR("parse lua code with error: %s\n", lua_tostring(L, -1));
                } else {
                    *hasError = false;
                }
            }

            if (!error) {
                if (lua_istable(L, -1)) {
                    int size = lua_rawlen(L, -1);
                    for (int i = 1; i <= size; ++i) {
                        lua_rawgeti(L, -1, i);
                        const char* str = lua_tostring(L, -1);
                        if (str != nullptr) {
                            ret.push_back(str);
                        }
                        lua_pop(L, 1);
                    }
                }
            }

            lua_close(L);

            return ret;
        }

        static DataValue read_lua_value(lua_State* L, int idx)
        {
            DataValue ret;
            lua_pushvalue(L, idx);
            int type = lua_type(L, -1);
            switch (type) {
                case LUA_TNIL:
                    ret = nullptr;
                    break;
                case LUA_TBOOLEAN: {
                    ret = lua_toboolean(L, -1) == 0 ? false : true;
                }
                break;
                case LUA_TSTRING:
                    ret = lua_tostring(L, -1);
                    break;
                case LUA_TNUMBER:
                    if (lua_isinteger(L, -1)) {
                        ret = (int64_t)lua_tointeger(L, -1);
                    } else {
                        ret = lua_tonumber(L, -1);
                    }
                    break;
                case LUA_TTABLE: {
                    DataTable table;
                    lua_pushnil(L);
                    while (lua_next(L, -2)) {
                        DataValue key = read_lua_value(L, -2);
                        DataValue val = read_lua_value(L, -1);
                        if (key.IsInteger()) {
                            table.Set(key.ToInteger(), std::move(val));
                        } else if (key.IsString()) {
                            table.Set(key.ToString(), std::move(val));
                        }
                        lua_pop(L, 1);
                    }
                    ret = std::move(table);
                }
                break;
            }
            lua_pop(L, 1);
            return ret;
        }

        DataTable Parser::ParseAsDataTable(const string& code, bool* hasError /* = nullptr */)
        {
            DataTable ret;

            string program = "return ";
            program.append(code);

            lua_State* L = luaL_newstate();

            int error = luaL_dostring(L, program.c_str());
            if (hasError != nullptr) {
                if (error) {
                    *hasError = true;
                    LOG_ERROR("parse lua code with error: %s\n", lua_tostring(L, -1));
                } else {
                    *hasError = false;
                }
            }

            if (!error) {
                if (lua_istable(L, -1)) {
                    lua_pushnil(L);
                    while (lua_next(L, -2)) {
                        DataValue key = read_lua_value(L, -2);
                        DataValue val = read_lua_value(L, -1);
                        if (key.IsInteger()) {
                            ret.Set(key.ToInteger(), std::move(val));
                        } else if (key.IsString()) {
                            ret.Set(key.ToString(), std::move(val));
                        }
                        lua_pop(L, 1);
                    }
                }
            }

            lua_close(L);

            return ret;
        }


        DataValue Parser::ParseAsDataValue(const string& code, bool* hasError /* = nullptr */)
        {
            DataValue ret;

            string program = "return ";
            program.append(code);

            lua_State* L = luaL_newstate();

            int error = luaL_dostring(L, program.c_str());
            if (hasError != nullptr) {
                if (error) {
                    *hasError = true;
                    LOG_ERROR("parse lua code with error: %s\n", lua_tostring(L, -1));
                } else {
                    *hasError = false;
                }
            }

            if (!error) {
                ret = read_lua_value(L, -1);
            }

            lua_close(L);

            return ret;
        }
    }

}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
