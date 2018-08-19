#include "lua_module_net.h"
#include <lua/lua.hpp>
#include "../gateway/packet.h"
#include "../gateway/userclient.h"
#include "../utils/utils_string.h"
#include "../cluster/message.h"
#include "../logger.h"

static const char* MT_NET_PKTIN = "mt_net_pktin";
static const char* MT_NET_PKTOUT = "mt_net_pktout";

namespace base
{
    namespace lua
    {
        using namespace std;
        using namespace base::gateway;

        enum class DataType
        {
            NONE    =   0,
            INTEGER =   1,
            FLOAT   =   2,
            DOUBLE  =   3,
            STR     =   4,
            BOOL    =   5,
            LIST    =   6,
        };

        struct FormatParamerInfo {

            enum KeyType {
                INT,
                STR,
            };

            struct StrKey {
                const char* str;
                int len;
            };

            union Key {
                int intKey;
                StrKey strKey;
            };

            KeyType keyType;
            DataType valType;
            Key key;

            FormatParamerInfo* next;
            FormatParamerInfo* parent;
            FormatParamerInfo* child;

            void Reset() {
                next = nullptr;
                parent = nullptr;
                child = nullptr;
            }

            void Display(int depth = 0) const;
        };

        class FormatStringParser
        {
        public:
            enum class State
            {
                INIT1,
                INIT2,
                KEY,
                VAL,
                ERROR,
            };

            bool Parse(const char* format);

            const FormatParamerInfo* GetHead() const {
                return m_used > 0 ? m_data : nullptr;
            }

            const std::string& GetError() const {
                return m_error;
            }

            void Dump();

        private:
            void Dump(const FormatParamerInfo* p, int depth);

            void Reset() {
                m_used = 0;
                m_error.clear();
            }

            FormatParamerInfo* AllocParamer(const FormatParamerInfo::KeyType& keyType1, const FormatParamerInfo::Key& key1, DataType valType1) {
                FormatParamerInfo& param = m_data[m_used];
                param.Reset();
                param.keyType = keyType1;
                param.key = key1;
                param.valType = valType1;
                ++m_used;
                return &param;
            }

            FormatParamerInfo m_data[512];
            int m_used = 0;
            std::string m_error;
        };

        static FormatStringParser parser;

        static inline PacketIn* _check_pktin(lua_State* L, int idx)
        {
            return (PacketIn*)luaL_checkudata(L, idx, MT_NET_PKTIN);
        }

        static inline PacketOut* _check_pktout(lua_State* L, int idx)
        {
            return (PacketOut*)luaL_checkudata(L, idx, MT_NET_PKTOUT);
        }

        static int pushPacketValue(lua_State* L, PacketIn* pktin, DataType type)
        {
            try {
                switch (type) {
                    case DataType::BOOL:
                        lua_pushboolean(L, pktin->ReadBoolean());
                        break;
                    case DataType::INTEGER:
                        lua_pushinteger(L, pktin->ReadVarInteger<int64_t>());
                        break;
                    case DataType::FLOAT:
                        lua_pushnumber(L, pktin->ReadFloat());
                        break;
                    case DataType::DOUBLE:
                        lua_pushnumber(L, pktin->ReadDouble());
                        break;
                    case DataType::STR: {
                        std::string v;
                        pktin->ReadString(v);
                        lua_pushlstring(L, v.c_str(), v.length());
                    }
                    break;
                    case DataType::NONE:
                        lua_pushnil(L);
                        break;
                    case DataType::LIST:
                        lua_pushnil(L);
                        break;
                }
                return 1;
            } catch (exception& ex) {
                return luaL_error(L, ex.what());
            }
        }

        static int _pktin_code(lua_State* L)
        {
            PacketIn* pktin = _check_pktin(L, 1);
            lua_pushinteger(L, pktin->code());
            return 1;
        }

        static int _pktin_session(lua_State* L)
        {
            PacketIn* pktin = _check_pktin(L, 1);
            lua_pushinteger(L, pktin->sessionid());
            return 1;
        }

        static int _pktin_read_integer(lua_State* L)
        {
            PacketIn* pktin = _check_pktin(L, 1);
            lua_pushinteger(L, pktin->ReadVarInteger<int64_t>());
            return 1;
        }

        static int _pktin_read_string(lua_State* L)
        {
            PacketIn* pktin = _check_pktin(L, 1);
            pushPacketValue(L, pktin, DataType::STR);
            return 1;
        }

        static int _pktin_read_boolean(lua_State* L)
        {
            PacketIn* pktin = _check_pktin(L, 1);
            pushPacketValue(L, pktin, DataType::BOOL);
            return 1;
        }

        static int _pktin_read_float(lua_State* L)
        {
            PacketIn* pktin = _check_pktin(L, 1);
            pushPacketValue(L, pktin, DataType::FLOAT);
            return 1;
        }

        static int _pktin_read_double(lua_State* L)
        {
            PacketIn* pktin = _check_pktin(L, 1);
            pushPacketValue(L, pktin, DataType::DOUBLE);
            return 1;
        }

        static int _pktin_read(lua_State* L)
        {
            PacketIn* pktin = _check_pktin(L, 1);
            size_t len;

            int count = 0;
            bool compact = true;
            const char* format = luaL_checklstring(L, 2, &len);
            if (len > 2 && format[0] == '@' && format[1] == '@') {
                if (!parser.Parse(format)) {
                    return luaL_error(L, parser.GetError().c_str());
                }
                compact = false;
            }

            if (compact) {
                const char* p = format;
                while (*p) {
                    switch (*p) {
                        case 'i':
                            pushPacketValue(L, pktin, DataType::INTEGER);
                            break;
                        case 'd':
                            pushPacketValue(L, pktin, DataType::DOUBLE);
                            break;
                        case 'f':
                            pushPacketValue(L, pktin, DataType::FLOAT);
                            break;
                        case 's':
                            pushPacketValue(L, pktin, DataType::STR);
                            break;
                        case 'b':
                            pushPacketValue(L, pktin, DataType::BOOL);
                            break;
                        case '[':
                            // TODO
                            break;
                        case ']':
                            // TODO
                            break;
                    }
                    ++count;
                    ++p;
                }
            } else {
                const FormatParamerInfo* param = parser.GetHead();
                while (param) {
                    // TODO
                    param = param->next;
                }
            }
            return count;
        }

        static int _pktin_size(lua_State* L)
        {
            PacketIn* pktin = _check_pktin(L, 1);
            lua_pushinteger(L, pktin->size());
            return 1;
        }

        static int _pktout_code(lua_State* L)
        {
            PacketOut* pktout = _check_pktout(L, 1);
            lua_pushinteger(L, pktout->code());
            return 1;
        }

        static void writeLuaValueToPktout(lua_State* L, int idx, base::gateway::PacketOut& pktout);
        static int _pktout_write(lua_State* L)
        {
            PacketOut* pktout = _check_pktout(L, 1);
            int argc = lua_gettop(L);
            for (int idx = 2; idx <= argc; ++idx) {
                writeLuaValueToPktout(L, idx, *pktout);
            }
            return 0;
        }

        static int _pktout_write_format(lua_State* L)
        {
            return 0;
        }

        static int _pktout_gc(lua_State* L)
        {
            PacketOut* pktout = (PacketOut*)luaL_checkudata(L, 1, MT_NET_PKTOUT);
            pktout->~PacketOut();
            return 0;
        }

        static int _pktout_write_integer(lua_State* L)
        {
            PacketOut* pktout = _check_pktout(L, 1);
            pktout->WriteVarInteger(luaL_checkinteger(L, 2));
            return 0;
        }

        static int _pktout_write_string(lua_State* L)
        {
            PacketOut* pktout = _check_pktout(L, 1);
            pktout->WriteString(luaL_checkstring(L, 2));
            return 0;
        }

        static int _pktout_write_boolean(lua_State* L)
        {
            PacketOut* pktout = _check_pktout(L, 1);
            pktout->WriteBoolean(lua_toboolean(L, 2));
            return 0;
        }

        static int _pktout_write_float(lua_State* L)
        {
            PacketOut* pktout = _check_pktout(L, 1);
            pktout->WriteFloat(luaL_checknumber(L, 2));
            return 0;
        }

        static int _pktout_write_double(lua_State* L)
        {
            PacketOut* pktout = _check_pktout(L, 1);
            pktout->WriteDouble(luaL_checknumber(L, 2));
            return 0;
        }

        static int _lua_connect(lua_State* L)
        {
            // TODO
            return 0;
        }

        static int _open_net(lua_State* L)
        {
            // class PacketIn
            luaL_Reg pktinFuns[] = {
                {"code", _pktin_code},
                {"session", _pktin_session},
                {"readInteger", _pktin_read_integer},
                {"readString", _pktin_read_string},
                {"readBoolean", _pktin_read_boolean},
                {"readBool", _pktin_read_boolean},
                {"readFloat", _pktin_read_float},
                {"readDouble", _pktin_read_double},
                {"read", _pktin_read},
                {"size", _pktin_size},
                {nullptr, nullptr},
            };


            int ok = luaL_newmetatable(L, MT_NET_PKTIN);
            assert(ok == 1);
            luaL_setfuncs(L, pktinFuns, 0);
            lua_pushstring(L, "__index");
            lua_pushvalue(L, -2);
            lua_rawset(L, -3);
            lua_pop(L, 1);

            // class PacketOut
            luaL_Reg pktoutFuns[] = {
                {"code", _pktout_code},
                {"write", _pktout_write},
                {"writeFormat", _pktout_write_format},
                {"writeInteger", _pktout_write_integer},
                {"writeString", _pktout_write_string},
                {"writeBoolean", _pktout_write_boolean},
                {"writeBool", _pktout_write_boolean},
                {"writeFloat", _pktout_write_float},
                {"writeDouble", _pktout_write_double},
                {"__gc", _pktout_gc},
                {nullptr, nullptr},
            };

            ok = luaL_newmetatable(L, MT_NET_PKTOUT);
            assert(ok == 1);
            luaL_setfuncs(L, pktoutFuns, 0);
            lua_pushstring(L, "__index");
            lua_pushvalue(L, -2);
            lua_rawset(L, -3);
            lua_pop(L, 1);

            luaL_Reg clientFuns[] = {
                {"connect", _lua_connect},
                //{"request", _lua_request},
                {nullptr, nullptr}
            };

            luaL_newlib(L, clientFuns);

            return 1;
        }

        void luaopen_net(lua_State* L)
        {
            luaL_requiref(L, "net", &_open_net, 0);
            lua_pop(L, 1);
        }

        void lua_net_push_packet_in(lua_State* L, PacketIn& pktin)
        {
            lua_pushlightuserdata(L, &pktin);
            luaL_newmetatable(L, MT_NET_PKTIN);
            lua_setmetatable(L, -2);
        }

        PacketIn* lua_check_packet_in(lua_State* L, int idx)
        {
            return (PacketIn*)luaL_checkudata(L, idx, MT_NET_PKTIN);
        }

        void lua_net_push_message_in(lua_State* L, base::cluster::MessageIn& msgin)
        {
            lua_pushlightuserdata(L, &msgin);
            luaL_newmetatable(L, MT_NET_PKTIN);
            lua_setmetatable(L, -2);
        }

        void FormatParamerInfo::Display(int depth) const
        {
            for (int i = 0; i < depth; ++i) {
                cout << "    ";
            }
            if (keyType == INT) {
                cout << key.intKey;
            } else {
                string str(key.strKey.str, key.strKey.len);
                cout << str;
            }
            cout << " = ";
            switch (valType) {
                case DataType::NONE:
                    cout << "NONE";
                    break;
                case DataType::BOOL:
                    cout << "BOOL";
                    break;
                case DataType::INTEGER:
                    cout << "INTEGER";
                    break;
                case DataType::FLOAT:
                    cout << "FLOAT";
                    break;
                case DataType::LIST:
                    cout << "LIST";
                    break;
                case DataType::STR:
                    cout << "STR";
                    break;
                case DataType::DOUBLE:
                    cout << "DOUBLE";
                    break;
            }
            cout << endl;
        }

        bool FormatStringParser::Parse(const char* format)
        {
            Reset();

            int i = 0;
            const char* p = format;
            State state = State::INIT1;

            FormatParamerInfo* prev = nullptr;
            FormatParamerInfo* parent = nullptr;

            int tokenBegin = -1;
            int tokenCount = 0;

            const char* strKey = nullptr;
            int strKeyLength = 0;

            FormatParamerInfo::KeyType keyType = FormatParamerInfo::INT;
            FormatParamerInfo::Key key;

            bool ok = true;
            bool run = true;
            while (run) {
                char c = *p;
                switch (state) {
                    case State::INIT1: {
                        if (c == '@') {
                            state = State::INIT2;
                        } else {
                            state = State::ERROR;
                            m_error = "should start with @@";
                        }
                    }
                    break;
                    case State::INIT2: {
                        if (c == '@') {
                            tokenBegin = i + 1;
                            tokenCount = 0;
                            state = State::KEY;
                        } else {
                            state = State::ERROR;
                            m_error = "should start with @@";
                        }
                    }
                    break;
                    case State::KEY: {
                        if (c == '=') {
                            strKey = format + tokenBegin;
                            strKeyLength = tokenCount;

                            bool isAllDigit = true;
                            for (int s0 = 0; s0 < strKeyLength; ++s0) {
                                if (!isdigit(strKey[s0])) {
                                    isAllDigit = false;
                                    break;
                                }
                            }
                            if (isAllDigit) {
                                keyType = FormatParamerInfo::INT;
                                key.intKey = strtol(strKey, 0, 0);
                            } else {
                                keyType = FormatParamerInfo::STR;
                                key.strKey.str = strKey;
                                key.strKey.len = strKeyLength;
                            }

                            state = State::VAL;
                            tokenBegin = i + 1;
                            tokenCount = 0;
                        } else if (std::isalnum(c)) {
                            ++tokenCount;
                        } else {
                            state = State::ERROR;
                            base::utils::string_append_format(m_error, "unrecognized key=%c", c);
                        }
                    }
                    break;
                    case State::VAL: {
                        if (c == ',' || c == '\0') {
                            state = State::KEY;
                            tokenBegin = i + 1;
                            tokenCount = 0;
                        } else if (c == ']') {
                            if (prev != nullptr) {
                                prev = prev->parent;
                                parent = prev->parent;
                            } else {
                                state = State::ERROR;
                                base::utils::string_append_format(m_error, "bad list close tag ]");
                            }
                        } else {
                            FormatParamerInfo* param = nullptr;
                            switch (c) {
                                case '[': {
                                    param = AllocParamer(keyType, key, DataType::LIST);
                                    state = State::KEY;
                                    tokenBegin = i + 1;
                                    tokenCount = 0;
                                }
                                break;
                                case 'b':
                                    param = AllocParamer(keyType, key, DataType::BOOL);
                                    break;
                                case 'i':
                                    param = AllocParamer(keyType, key, DataType::INTEGER);
                                    break;
                                case 'd':
                                    param = AllocParamer(keyType, key, DataType::DOUBLE);
                                    break;
                                case 'f':
                                    param = AllocParamer(keyType, key, DataType::FLOAT);
                                    break;
                                case 's':
                                    param = AllocParamer(keyType, key, DataType::STR);
                                    break;
                                default: {
                                    state = State::ERROR;
                                    base::utils::string_append_format(m_error, "unrecognized val=%c", c);
                                }
                                break;
                            }

                            if (param != nullptr) {
                                if (prev != nullptr) {
                                    prev->next = param;
                                } else {
                                    if (parent != nullptr) {
                                        parent->child = param;
                                    }
                                }

                                if (parent != nullptr) {
                                    param->parent = parent;
                                }

                                prev = param;

                                if (param->valType == DataType::LIST) {
                                    parent = param;
                                    prev = nullptr;
                                }
                            }
                        }
                    }
                    break;
                    case State::ERROR: {
                        run = false;
                        ok = false;
                    }
                    break;
                }

                if (c) {
                    ++i;
                    ++p;
                } else {
                    run = false;
                }
            }

            return ok;
        }

        void FormatStringParser::Dump(const FormatParamerInfo* p, int depth)
        {
            while (p) {
                p->Display(depth);

                if (p->child != nullptr) {
                    Dump(p->child, depth + 1);
                }
                p = p->next;
            }
        }

        void FormatStringParser::Dump()
        {
            if (!m_error.empty()) {
                cout << "error:" << m_error << endl;
            } else {
                Dump(GetHead(), 0);
            }
        }

        static void writeLuaValueToPktout(lua_State* L, int idx, base::gateway::PacketOut& pktout)
        {
            int argType = lua_type(L, idx);
            switch (argType) {
                case LUA_TTABLE: {
                    int i = 1;
                    while (true) {
                        lua_rawgeti(L, idx, i);
                        if (lua_isnil(L, -1)) {
                            lua_pop(L, 1);
                            break;
                        } else {
                            writeLuaValueToPktout(L, lua_gettop(L), pktout);
                            lua_pop(L, 1);
                        }
                        ++i;
                    }
                }
                break;
                case LUA_TNUMBER:
                    if (lua_tointeger(L, idx) == lua_tonumber(L, idx)) {
                        pktout.WriteVarInteger(lua_tointeger(L, idx));
                    } else {
                        pktout.WriteFloat(lua_tonumber(L, idx));
                    }
                    break;
                case LUA_TBOOLEAN:
                    pktout.WriteBoolean(lua_toboolean(L, idx));
                    break;
                case LUA_TSTRING: {
                    size_t len;
                    const char* str = lua_tolstring(L, idx, &len);
                    pktout.WriteString(str, len);
                }
                break;
                case LUA_TNIL:
                    pktout.WriteBoolean(false);
                    break;
                default:
                    throw std::runtime_error("unsupported type");
            }
        }

        static void writeLuaValueToPktout(lua_State* L, int idx, const FormatParamerInfo* param, base::gateway::PacketOut& pktout)
        {
            switch (param->valType) {
                case DataType::STR: {
                    if (lua_isstring(L, idx)) {
                        size_t len;
                        const char* str = lua_tolstring(L, idx, &len);
                        pktout.WriteString(str, len);
                    } else {
                        throw runtime_error("expect string");
                    }
                }
                break;
                case DataType::INTEGER: {
                    if (lua_isinteger(L, idx)) {
                        pktout.WriteVarInteger<int64_t>(lua_tointeger(L, idx));
                    } else {
                        throw runtime_error("expect integer");
                    }
                }
                break;
                case DataType::FLOAT: {
                    if (lua_isnumber(L, idx)) {
                        pktout.WriteFloat(lua_tonumber(L, idx));
                    } else {
                        throw runtime_error("expect float");
                    }
                }
                break;
                case DataType::DOUBLE: {
                    if (lua_isnumber(L, idx)) {
                        pktout.WriteDouble(lua_tonumber(L, idx));
                    } else {
                        throw runtime_error("expect float");
                    }
                }
                break;
                case DataType::BOOL: {
                    pktout.WriteBoolean(lua_toboolean(L, idx));
                }
                break;
                case DataType::LIST: {
                    if (lua_istable(L, idx)) {
                        int len = lua_rawlen(L, idx);
                        pktout.WriteVarInteger(len);
                        for (int i = 1; i <= len; ++i) {
                            lua_rawgeti(L, idx, i);
                            if (lua_istable(L, -1)) {
                                const FormatParamerInfo* cp = param->child;
                                while (cp) {
                                    if (cp->keyType == FormatParamerInfo::INT) {
                                        lua_pushinteger(L, cp->key.intKey);
                                    } else {
                                        lua_pushlstring(L, cp->key.strKey.str, cp->key.strKey.len);
                                    }
                                    lua_gettable(L, -2);
                                    writeLuaValueToPktout(L, lua_gettop(L), cp, pktout);
                                    lua_pop(L, 1);
                                    cp = cp->next;
                                }
                                lua_pop(L, 1);
                            } else {
                                throw runtime_error("expect table");
                            }
                        }
                    } else {
                        throw runtime_error("expect table");
                    }
                }
                break;
                default:
                    break;
            }
        }

        int lua_net_send_values(base::gateway::UserClient* client, lua_State* L, int beginIdx, int session)
        {
            uint16_t code = luaL_checkinteger(L, beginIdx);
            int argc = lua_gettop(L);

            const char* format = nullptr;
            if (lua_type(L, beginIdx + 1) == LUA_TSTRING) {
                size_t strLen = 0lu;
                format = lua_tolstring(L, beginIdx + 1, &strLen);
                if (strLen > 2 && format[0] == '@' && format[1] == '@') {
                    if (!parser.Parse(format)) {
                        return luaL_error(L, "parse format found error: %s", parser.GetError().c_str());
                    }
                    // parser.Dump();
                } else {
                    format = nullptr;
                }
            }

            try {
                base::gateway::PacketOut pktout(code, 60, client->mempool());
                pktout.SetSessionID(session);

                if (format == nullptr) {
                    for (int idx = beginIdx + 1; idx <= argc; ++idx) {
                        writeLuaValueToPktout(L, idx, pktout);
                    }
                } else {
                    const FormatParamerInfo* param = parser.GetHead();
                    for (int idx = beginIdx + 2; idx <= argc && param != nullptr; ++idx) {
                        writeLuaValueToPktout(L, idx, param, pktout);
                        param = param->next;
                    }
                }
                client->Send(pktout);
                return 0;

            } catch (exception& ex) {
                string err = ex.what();
                err.append(" lua_net_send_values code = " + to_string(code));
                LOG_ERROR("%s", err.c_str());
                lua_pushboolean(L, false);
                return 1;
            }
        }

        class LuaPacketOut
        {
        public:

        };

        int lua_net_new_pktout(UserClient* client, lua_State* L, int code, int size, int session)
        {
            PacketOut* pktout = (PacketOut*)lua_newuserdata(L, sizeof(PacketOut));
            new(pktout)PacketOut((uint16_t)code, size, client->mempool());
            luaL_newmetatable(L, MT_NET_PKTOUT);
            lua_setmetatable(L, -2);
            return 1;
        }
    }
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
