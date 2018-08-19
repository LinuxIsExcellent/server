#include "lua_module_dbo.h"
#include <lua/lua.hpp>
#include "../dbo/dbpool.h"
#include "../dbo/connection.h"
#include "../logger.h"
#include "luaex.h"

namespace base
{
    namespace lua
    {
        using namespace std;
        using namespace base::dbo;

        static const char* MT_DBO_DB = "mt_dbo_db";
        static char DBO_LIST_ADDR = 'c';

        enum class DBParamType
        {
            INT8 = 1,
            INT16,
            INT32,
            INT64,
            FLOAT,
            DOUBLE,
            STRING,
        };

        class LuaDB
        {
        public:
            LuaDB(int dbNo, bool fixed) : m_dbNo(dbNo), m_fixed(fixed) {
            }

            ~LuaDB() {
            }

            int InsertBatch(lua_State* L, int pendingNo) {
                {
                    int top = lua_gettop(L);
                    const char* tableName = luaL_checkstring(L, 2);

                    int count = luaL_len(L, 3);
                    if (count == 0) {
                        return luaL_error(L, "data is empty");
                    }
                    int tt = lua_rawgeti(L, 3, 1);
                    if (tt != LUA_TTABLE) {
                        return luaL_error(L, "expect array of table in data");
                    }
                    vector<const char*> keys;
                    lua_pushnil(L);
                    while (lua_next(L, -2)) {
                        if (lua_type(L, -2) == LUA_TSTRING) {
                            const char* key = lua_tostring(L, -2);
                            keys.push_back(key);
                        }
                        lua_pop(L, 1);
                    }
                    if (keys.empty()) {
                        return luaL_error(L, "found empty table in data");
                    }

                    // updateFields
                    vector<const char*> updates;
                    if (top >= 4) {
                        if (luaL_len(L, 4) == 0) {
                            return luaL_error(L, "update fields is empty");
                        }
                        lua_pushvalue(L, 4);
                        lua_pushnil(L);
                        while (lua_next(L, -2)) {
                            if (lua_type(L, -1) == LUA_TSTRING) {
                                const char* fields = lua_tostring(L, -1);
                                updates.push_back(fields);
                            } else {
                                return luaL_error(L, "expect array of string in update fields");
                            }
                            lua_pop(L, 1);
                        }
                    }

                    string sql = "insert into ";
                    sql.append(tableName);

                    sql.append("(");
                    for (size_t i = 0u; i < keys.size(); ++i) {
                        if (i != 0u) {
                            sql.append(",");
                        }
                        sql.append(keys[i]);
                    }
                    sql.append(")");

                    sql.append(" values ");

                    string fieldStr = "(";
                    for (size_t i = 0; i < keys.size(); ++i) {
                        if (i != 0u) {
                            fieldStr.append(",");
                        }
                        fieldStr.append("?");
                    }
                    fieldStr.append(")");

                    for (int i = 0; i < count; ++i) {
                        if (i != 0) {
                            sql.append(",");
                        }
                        sql.append(fieldStr);
                    }

                    if (!updates.empty()) {
                        fieldStr.clear();
                        fieldStr.append(" on duplicate key update ");
                        for (size_t i = 0; i < updates.size(); ++i) {
                            if (i != 0u) {
                                fieldStr.append(",");
                            }
                            fieldStr.append(updates[i]).append("=values(").append(updates[i]).append(")");
                        }
                        sql.append(fieldStr);
                    }
                    
                    //cout << "insertBatch:" << sql << endl;

                    PreparedStatement* pstmt = PreparedStatement::Create(m_dbNo, sql);

                    lua_pushnil(L);
                    while (lua_next(L, 3) != 0) {
                        for (const char * key : keys) {
                            lua_getfield(L, -1, key);

                            switch (lua_type(L, -1)) {
                                case LUA_TNIL:
                                    pstmt->SetNull();
                                    break;
                                case LUA_TBOOLEAN:
                                    pstmt->SetBoolean(lua_toboolean(L, -1));
                                    break;
                                case LUA_TNUMBER: {
                                    lua_Integer x = lua_tointeger(L, -1);
                                    lua_Number n = lua_tonumber(L, -1);
                                    if ((lua_Number)x == n) {
                                        pstmt->SetInt64(x);
                                    } else {
                                        pstmt->SetDouble(n);
                                    }
                                }
                                break;
                                case LUA_TSTRING: {
                                    size_t len;
                                    const char* param = lua_tolstring(L, -1, &len);
                                    pstmt->SetString(param, len);
                                }
                                break;
                                default:
                                    LOG_ERROR("this type is not supported save into database");
                                    break;
                            }
                            lua_pop(L, 1);
                        }
                        lua_pop(L, 1);
                    }

                    pstmt->Execute([this, L, pendingNo](ResultSet & rs) {
                        int nargs = this->LuaPushResultSet(rs, L);
                        luaEx_resume(L, NULL, nargs);
                        luaEx_getPrivateTableOnRegistry(L, &DBO_LIST_ADDR); // 移除引用
                        lua_pushnil(L);
                        lua_rawseti(L, -2, pendingNo);
                    }, m_autoObserver);
                }

                return lua_yield(L, 0);
            }

            int Execute(lua_State* L, int pendingNo) {
                {
                    const char* sql = luaL_checkstring(L, 2);
                    Statement* stmt = Statement::Create(m_dbNo, sql);
                    stmt->Execute([this, L, pendingNo](ResultSet & rs) {
                        int nargs = this->LuaPushResultSet(rs, L);
                        luaEx_resume(L, NULL, nargs);
                        luaEx_getPrivateTableOnRegistry(L, &DBO_LIST_ADDR); // 移除引用
                        lua_pushnil(L);
                        lua_rawseti(L, -2, pendingNo);
                    }, m_autoObserver);
                }
                return lua_yield(L, 0);
            }

            int ExecutePrepare(lua_State* L, int pendingNo) {
                {
                    const char* sql = luaL_checkstring(L, 2);
                    PreparedStatement* pstmt = PreparedStatement::Create(m_dbNo, sql);
                    int top = lua_gettop(L);
                    for (int i = 3; i <= top; ++i) {
                        switch (lua_type(L, i)) {
                            case LUA_TNIL:
                                pstmt->SetNull();
                                break;
                            case LUA_TBOOLEAN:
                                pstmt->SetBoolean(lua_toboolean(L, i));
                                break;
                            case LUA_TNUMBER: {
                                lua_Integer x = lua_tointeger(L, i);
                                lua_Number n = lua_tonumber(L, i);
                                if ((lua_Number)x == n) {
                                    pstmt->SetInt64(x);
                                } else {
                                    pstmt->SetDouble(n);
                                }
                            }
                            break;
                            case LUA_TSTRING: {
                                size_t len;
                                const char* param = lua_tolstring(L, i, &len);
                                pstmt->SetString(param, len);
                            }
                            break;
                            case LUA_TTABLE: {
                                lua_rawgeti(L, i, 1);
                                lua_rawgeti(L, i, 2);
                                DBParamType paramType = (DBParamType)luaL_checkinteger(L, -2);
                                switch (paramType) {
                                    case DBParamType::INT8:
                                        pstmt->SetInt8(lua_tointeger(L, -1));
                                        break;
                                    case DBParamType::INT16:
                                        pstmt->SetInt16(lua_tointeger(L, -1));
                                        break;
                                    case DBParamType::INT32:
                                        pstmt->SetInt32(lua_tointeger(L, -1));
                                        break;
                                    case DBParamType::INT64:
                                        pstmt->SetInt64(lua_tointeger(L, -1));
                                        break;
                                    case DBParamType::FLOAT:
                                        pstmt->SetFloat(lua_tonumber(L, -1));
                                        break;
                                    case DBParamType::DOUBLE:
                                        pstmt->SetDouble(lua_tonumber(L, -1));
                                        break;
                                    case DBParamType::STRING: {
                                        size_t len;
                                        const char* param = lua_tolstring(L, i, &len);
                                        pstmt->SetString(param, len);
                                    }
                                    break;
                                }
                            }
                            break;
                            default:
                                return luaL_error(L, "unsupport param type %d=>%s", i, lua_typename(L, lua_type(L, i)));
                        }
                    }
                    pstmt->Execute([this, L, pendingNo](ResultSet & rs) {
                        int nargs = this->LuaPushResultSet(rs, L);
                        luaEx_resume(L, NULL, nargs);
                        luaEx_getPrivateTableOnRegistry(L, &DBO_LIST_ADDR); // 移除引用
                        lua_pushnil(L);
                        lua_rawseti(L, -2, pendingNo);
                    }, m_autoObserver);
                }
                return lua_yield(L, 0);
            }

            int Close(lua_State* L) {
                m_autoObserver.SetNotExist();
                return 0;
            }

        private:
            int LuaPushResultSet(ResultSet& rs, lua_State* L) {
                if (rs.HasError()) {
                    lua_newtable(L);
                    lua_pushstring(L, "ok");
                    lua_pushboolean(L, false);
                    lua_rawset(L, -3);
                    lua_pushstring(L, "errorMessage");
                    lua_pushstring(L, rs.error_message());
                    lua_rawset(L, -3);
                    return 1;
                } else {
                    lua_newtable(L);
                    lua_pushstring(L, "ok");
                    lua_pushboolean(L, true);
                    lua_rawset(L, -3);
                    lua_pushstring(L, "lastInsertedID");
                    lua_pushinteger(L, rs.last_insert_id());
                    lua_rawset(L, -3);
                    lua_pushstring(L, "affectedRows");
                    lua_pushinteger(L, rs.affected_rows());
                    lua_rawset(L, -3);
                    lua_pushstring(L, "rowsCount");
                    lua_pushinteger(L, rs.rows_count());
                    lua_rawset(L, -3);
                    lua_pushstring(L, "warningCount");
                    lua_pushinteger(L, rs.warning_count());
                    lua_rawset(L, -3);

                    int idx = 0;
                    while (rs.Next()) {
                        lua_newtable(L);
                        int col = 0;
                        for (vector<base::dbo::ColumnDefinition>::const_iterator it = rs.metadata().columns.begin(); it != rs.metadata().columns.end(); ++it) {
                            const base::dbo::ColumnDefinition& meta = *it;
                            lua_pushstring(L, meta.name.c_str());
                            ++col;
                            if (rs.IsNull(col)) {
                                lua_pushnil(L);
                            } else {
                                switch (meta.type) {
                                    case base::dbo::internal::MYSQL_TYPE_TINY:
                                        lua_pushboolean(L, rs.GetInt8(col));
                                        break;
                                    case base::dbo::internal::MYSQL_TYPE_SHORT:
                                    case base::dbo::internal::MYSQL_TYPE_YEAR:
                                        lua_pushinteger(L, rs.GetInt16(col));
                                        break;
                                    case base::dbo::internal::MYSQL_TYPE_LONG:
                                    case base::dbo::internal::MYSQL_TYPE_INT24:
                                        lua_pushinteger(L, rs.GetInt32(col));
                                        break;
                                    case base::dbo::internal::MYSQL_TYPE_LONGLONG:
                                        lua_pushinteger(L, rs.GetInt64(col));
                                        break;
                                    case base::dbo::internal::MYSQL_TYPE_FLOAT:
                                        lua_pushnumber(L, rs.GetFloat(col));
                                        break;
                                    case base::dbo::internal::MYSQL_TYPE_DOUBLE:
                                        lua_pushnumber(L, rs.GetDouble(col));
                                        break;
                                    case base::dbo::internal::MYSQL_TYPE_VARCHAR:
                                    case base::dbo::internal::MYSQL_TYPE_BIT:
                                    case base::dbo::internal::MYSQL_TYPE_TINY_BLOB:
                                    case base::dbo::internal::MYSQL_TYPE_MEDIUM_BLOB:
                                    case base::dbo::internal::MYSQL_TYPE_LONG_BLOB:
                                    case base::dbo::internal::MYSQL_TYPE_BLOB:
                                    case base::dbo::internal::MYSQL_TYPE_VAR_STRING:
                                    case base::dbo::internal::MYSQL_TYPE_STRING: {
                                        string field = rs.GetString(col);
                                        lua_pushstring(L, field.c_str());
                                    }
                                    break;
                                    default:
                                        lua_pushnil(L);
                                        LOG_ERROR("not support field type: %s\n", meta.type);
                                        break;
                                }
                            }
                            lua_rawset(L, -3);
                        }
                        lua_rawseti(L, -2, ++idx);
                    }
                    return 1;
                }
            }

        private:
            base::AutoObserver m_autoObserver;
            int m_dbNo;
            bool m_fixed;
        };

        static int s_pendingCur = 0;

        static int _lua_db_executePrepare(lua_State* L)
        {
            if (lua_pushthread(L) == 1) {
                return luaL_error(L, "should call in coroutine");
            }
            lua_pop(L, 1);

            LuaDB* db = (LuaDB*)luaL_checkudata(L, 1, MT_DBO_DB);

            int pendingNo = ++s_pendingCur;

            luaEx_getPrivateTableOnRegistry(L, &DBO_LIST_ADDR);
            lua_newtable(L);
            lua_pushvalue(L, 1);    // 将当前协程与DB对象都存起来，避免回收
            lua_rawseti(L, -2, 1);
            lua_pushthread(L);
            lua_rawseti(L, -2, 2);
            lua_rawseti(L, -2, pendingNo);
            lua_pop(L, 1);
            return db->ExecutePrepare(L, pendingNo);
        }

        static int _lua_db_insertBatch(lua_State* L)
        {
            int top = lua_gettop(L);
            if (lua_pushthread(L) == 1) {
                return luaL_error(L, "should call in coroutine");
            }
            lua_pop(L, 1);

            LuaDB* db = (LuaDB*)luaL_checkudata(L, 1, MT_DBO_DB);

            luaL_checktype(L, 2, LUA_TSTRING);
            luaL_checktype(L, 3, LUA_TTABLE);
            if (top >= 4) {
                luaL_checktype(L, 4, LUA_TTABLE);
            }

            int pendingNo = ++s_pendingCur;

            luaEx_getPrivateTableOnRegistry(L, &DBO_LIST_ADDR);
            lua_newtable(L);
            lua_pushvalue(L, 1);    // 将当前协程与DB对象都存起来，避免回收
            lua_rawseti(L, -2, 1);
            lua_pushthread(L);
            lua_rawseti(L, -2, 2);
            lua_rawseti(L, -2, pendingNo);
            lua_pop(L, 1);
            return db->InsertBatch(L, pendingNo);
        }

        static int _lua_db_execute(lua_State* L)
        {
            if (lua_pushthread(L) == 1) {
                return luaL_error(L, "should call in coroutine");
            }
            lua_pop(L, 1);

            LuaDB* db = (LuaDB*)luaL_checkudata(L, 1, MT_DBO_DB);

            int pendingNo = ++s_pendingCur;

            luaEx_getPrivateTableOnRegistry(L, &DBO_LIST_ADDR);
            lua_newtable(L);
            lua_pushvalue(L, 1);    // 将当前协程与DB对象都存起来，避免回收
            lua_rawseti(L, -2, 1);
            lua_pushthread(L);
            lua_rawseti(L, -2, 2);
            lua_rawseti(L, -2, pendingNo);
            lua_pop(L, 1);
            return db->Execute(L, pendingNo);
        }

        static int _lua_db_close(lua_State* L)
        {
            LuaDB* db = (LuaDB*)luaL_checkudata(L, 1, MT_DBO_DB);
            return db->Close(L);
        }

        static int _lua_db___gc(lua_State* L)
        {
            LuaDB* db = (LuaDB*)luaL_checkudata(L, 1, MT_DBO_DB);
            db->~LuaDB();
            return 0;
        }

        static int _lua_dbo_open(lua_State* L)
        {
            int dbNo = luaL_checkinteger(L, 1);
            bool fixed = false;
            if (lua_isboolean(L, 2)) {
                fixed = lua_toboolean(L, 2);
            }
            LuaDB* db = (LuaDB*)lua_newuserdata(L, sizeof(LuaDB));
            luaL_setmetatable(L, MT_DBO_DB);
            new(db)LuaDB(dbNo, fixed);
            return 1;
        }

        static int _lua_dbo_disconnect_all(lua_State* L)
        {
            int dbNo = luaL_checkinteger(L, 1);
            dbo::g_dbpool->DisconnectAll(dbNo);
            return 0;
        }

        static int _lua_open_dbo(lua_State* L)
        {
            luaEx_createPrivateTableOnRegistry(L, &DBO_LIST_ADDR, "dbo", "");
            // class DB
            luaL_Reg dbFuncs[] = {
                {"execute", _lua_db_execute},
                {"executePrepare", _lua_db_executePrepare},
                {"insertBatch", _lua_db_insertBatch},
                {"close", _lua_db_close},
                {"__gc", _lua_db___gc},
                {nullptr, nullptr},
            };

            int ok = luaL_newmetatable(L, MT_DBO_DB);
            assert(ok == 1);
            luaL_setfuncs(L, dbFuncs, 0);
            lua_pushstring(L, "__index");
            lua_pushvalue(L, -2);
            lua_rawset(L, -3);
            lua_pop(L, 1);

            luaL_Reg dboFuns[] = {
                {"open", _lua_dbo_open},
                {"disconnectAll", _lua_dbo_disconnect_all},
                {nullptr, nullptr}
            };

            luaL_newlib(L, dboFuns);
            return 1;
        }

        void luaopen_dbo(lua_State* L)
        {
            luaL_requiref(L, "dbo", &_lua_open_dbo, 0);
            lua_pop(L, 1);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
