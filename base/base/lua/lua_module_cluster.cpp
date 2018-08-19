#include "lua_module_cluster.h"
#include <lua/lua.hpp>
#include "../cluster/nodemonitor.h"
#include "../cluster/rpcservice.h"
#include "../cluster/rpcstub.h"
#include "../cluster/mailbox.h"
#include "luacallbackmgr.h"
#include "luaex.h"
#include "luamessage.h"
#include "../logger.h"

namespace base
{
    namespace lua
    {
        using namespace std;
        using namespace base::cluster;

        static const char ADDR_SERVICE_LIST = 'c';
        static const char ADDR_STUB_LIST = 'c';
        static const char ADDR_MAILBOX_LIST = 'c';
        static const char ADDR_EVENT_LISTENER_LIST = 'c';
        static const char* MT_SERVICE_LIST = "mt_cluster_service_list";
        static const char* MT_STUB_LIST = "mt_cluster_stub_list";
        static const char* MT_MAILBOX_LIST = "mt_cluster_mailbox_list";
        static const char* MT_EVENT_LIST = "mt_cluster_event_list";
        static const char* MT_CLUSTER_EVENT_LISTENER = "mt_cluster_event_listener";
        static const char* MT_CLUSTER_SERVICE = "mt_cluster_service";
        static const char* MT_CLUSTER_STUB = "mt_cluster_stub";
        static const char* MT_CLUSTER_MAILBOX = "mt_cluster_mailbox";


        static inline void write_lua_boolean(base::cluster::MessageOut& msgout, bool v)
        {
            msgout.WriteInt8((int8_t)LuaMessageValueType::BOOLEAN);
            msgout.WriteBoolean(v);
        }

        static inline void write_lua_string(base::cluster::MessageOut& msgout, const char* v)
        {
            msgout.WriteInt8((int8_t)LuaMessageValueType::STRING);
            msgout.WriteString(v);
        }

        // 写入lua数据至message中
        static void write_lua_value(base::cluster::MessageOut& msgout, lua_State* L, int idx)
        {
            lua_pushvalue(L, idx);
            int type = lua_type(L, -1);
            switch (type) {
                case LUA_TNIL: {
                    msgout.WriteInt8((int8_t)LuaMessageValueType::NIL);
                }
                break;
                case LUA_TNUMBER: {
                    if (lua_isinteger(L, -1)) {
                        msgout.WriteInt8((int8_t)LuaMessageValueType::EX_INTEGER);
                        msgout.WriteVarInteger<int64_t>(lua_tointeger(L, -1));
                    } else {
                        msgout.WriteInt8((int8_t)LuaMessageValueType::NUMBER);
                        msgout.WriteDouble(lua_tonumber(L, -1));
                    }
                }
                break;
                case LUA_TBOOLEAN: {
                    msgout.WriteInt8((int8_t)LuaMessageValueType::BOOLEAN);
                    msgout.WriteBoolean(lua_toboolean(L, -1));
                }
                break;
                case LUA_TSTRING: {
                    msgout.WriteInt8((int8_t)LuaMessageValueType::STRING);
                    size_t len;
                    const char* v = lua_tolstring(L, -1, &len);
                    msgout.WriteBits(v, len);
                }
                break;
                case LUA_TTABLE: {
                    msgout.WriteInt8((int8_t)LuaMessageValueType::TABLE);
                    int len = 0;
                    lua_pushnil(L);
                    while (lua_next(L, -2)) {
                        ++len;
                        lua_pop(L, 1);
                    }
                    msgout.WriteVarInteger(len);
                    lua_pushnil(L);
                    while (lua_next(L, -2)) {
                        write_lua_value(msgout, L, -2);
                        write_lua_value(msgout, L, -1);
                        lua_pop(L, 1);
                    }
                }
                break;
                case LUA_TLIGHTUSERDATA: {
                    msgout.WriteInt8((int8_t)LuaMessageValueType::LIGHTUSERDAT);
                    void* ptr = lua_touserdata(L, -1);
                    msgout.WriteInt64(reinterpret_cast<intptr_t>(ptr));
                }
                break;
                default:
                    luaL_error(L, "unspported type in rpc method");
                    break;
            }
            lua_pop(L, 1);
        }

        static inline void write_lua_message(MessageOut& msgout, lua_State* L, int beginIdx)
        {
            int top = lua_gettop(L);
            assert(beginIdx <= (top + 1) && beginIdx > 0);
            msgout.WriteVarInteger(top - beginIdx + 1);
            for (int i = beginIdx; i <= top; ++i) {
                write_lua_value(msgout, L, i);
            }
        }

        // 从message中读取lua数据
        static void read_lua_value(base::cluster::MessageIn& msgin, lua_State* L)
        {
            LuaMessageValueType type = (LuaMessageValueType)msgin.ReadInt8();
            switch (type) {
                case LuaMessageValueType::NIL:
                    lua_pushnil(L);
                    break;
                case LuaMessageValueType::EX_INTEGER: {
                    int64_t v = msgin.ReadVarInteger<int64_t>();
                    lua_pushinteger(L, v);
                }
                break;
                case LuaMessageValueType::NUMBER: {
                    double v = msgin.ReadDouble();
                    lua_pushnumber(L, v);
                }
                break;
                case LuaMessageValueType::BOOLEAN: {
                    bool v = msgin.ReadBoolean();
                    lua_pushboolean(L, v);
                }
                break;
                case LuaMessageValueType::STRING: {
                    string v = msgin.ReadString();
                    lua_pushstring(L, v.c_str());
                }
                break;
                case LuaMessageValueType::TABLE: {
                    lua_newtable(L);
                    int len = msgin.ReadVarInteger<int>();
                    for (int i = 0; i < len; ++i) {
                        read_lua_value(msgin, L);
                        read_lua_value(msgin, L);
                        lua_rawset(L, -3);
                    }
                }
                break;
                case LuaMessageValueType::LIGHTUSERDAT: {
                    intptr_t v = msgin.ReadInt64();
                    lua_pushlightuserdata(L, (void*)v);
                }
                break;
                default:
                    break;
            }
        }

        static inline int read_lua_message(MessageIn& msgin, lua_State* L)
        {
            int argc = msgin.ReadVarInteger<int>();
            for (int i = 0; i < argc; ++i) {
                read_lua_value(msgin, L);
            }
            return argc;
        }

        MailboxID luaEx_check_mailboxId(lua_State* L, int idx)
        {
            luaL_checktype(L, idx, LUA_TTABLE);
            lua_rawgeti(L, idx, 1);
            int nodeId = luaL_checkinteger(L, -1);
            lua_pop(L, 1);
            lua_rawgeti(L, idx, 2);
            int pid = luaL_checkinteger(L, -1);
            lua_pop(L, 1);
            return MailboxID(nodeId, pid);
        }

        void luaEx_push_mailboxId(lua_State* L, const MailboxID& mbid)
        {
            lua_createtable(L, 2, 0);
            lua_pushinteger(L, mbid.nodeid());
            lua_rawseti(L, -2, 1);
            lua_pushinteger(L, mbid.pid());
            lua_rawseti(L, -2, 2);
        }

        /// LuaService
        class LuaService : public RpcService
        {
        public:
            static int _lua_publish(lua_State* L) {
                LuaService* service = (LuaService*)luaL_checkudata(L, 1, MT_CLUSTER_SERVICE);
                MailboxID to = luaEx_check_mailboxId(L, 2);
                MessageOut msgout(0, 60, service->mempool());
                write_lua_message(msgout, L, 3);
                service->Publish(to, msgout);
                return 0;
            }

            static int _lua_publishToAll(lua_State* L) {
                LuaService* service = (LuaService*)luaL_checkudata(L, 1, MT_CLUSTER_SERVICE);
                MessageOut msgout(0, 60, service->mempool());
                write_lua_message(msgout, L, 2);
                service->PublishToAll(msgout);
                return 0;
            }

            static int _lua_publishToAllExcept(lua_State* L) {
                LuaService* service = (LuaService*)luaL_checkudata(L, 1, MT_CLUSTER_SERVICE);
                MailboxID except = luaEx_check_mailboxId(L, 2);

                MessageOut msgout(0, 60, service->mempool());
                write_lua_message(msgout, L, 3);
                service->PublishToAllExcept(msgout, except);
                return 0;
            }

            static int _lua_cast(lua_State* L) {
                LuaService* service = (LuaService*)luaL_checkudata(L, 1, MT_CLUSTER_SERVICE);
                MailboxID to = luaEx_check_mailboxId(L, 2);

                MessageOut msgout(0, 60, service->mempool());
                write_lua_message(msgout, L, 3);
                service->Cast(to, msgout);
                return 0;
            }

            static int _lua_from(lua_State* L) {
                LuaService* service = (LuaService*)luaL_checkudata(L, 1, MT_CLUSTER_SERVICE);
                luaEx_push_mailboxId(L, service->m_currentFrom);
                return 1;
            }

            static int _lua_session(lua_State* L) {
                LuaService* service = (LuaService*)luaL_checkudata(L, 1, MT_CLUSTER_SERVICE);
                lua_createtable(L, 3, 0);
                lua_pushinteger(L, service->m_currentFrom.nodeid());
                lua_rawseti(L, -2, 1);
                lua_pushinteger(L, service->m_currentFrom.pid());
                lua_rawseti(L, -2, 2);
                lua_pushinteger(L, service->m_currentSession);
                lua_rawseti(L, -2, 3);
                return 1;
            }

            static int _lua_disableAutoReply(lua_State* L) {
                LuaService* service = (LuaService*)luaL_checkudata(L, 1, MT_CLUSTER_SERVICE);
                service->m_disableAutoReply = true;
                return 0;
            }

            static int _lua_reply(lua_State* L) {
                LuaService* service = (LuaService*)luaL_checkudata(L, 1, MT_CLUSTER_SERVICE);
                luaL_checktype(L, 2, LUA_TTABLE);
                if (lua_rawlen(L, 2) != 3) {
                    return luaL_error(L, "bad argument#2, expected table with length=3");
                }
                int top = lua_gettop(L);

                lua_rawgeti(L, 2, 1);
                int nodeId = lua_tointeger(L, -1);
                lua_pop(L, 1);

                lua_rawgeti(L, 2, 2);
                int pid = lua_tointeger(L, -1);
                lua_pop(L, 1);

                lua_rawgeti(L, 2, 3);
                int session = lua_tointeger(L, -1);
                lua_pop(L, 1);

                MailboxID to(nodeId, pid);
                base::cluster::MessageOut msgout(0, 64, service->mempool());
                msgout.WriteVarInteger(top - 2 + 1);
                write_lua_boolean(msgout, true);
                for (int i = 3; i <= top; ++i) {
                    write_lua_value(msgout, L, i);
                }
                service->Reply(to, session, msgout);
                return 0;
            }

            static int _lua_replyError(lua_State* L) {
                LuaService* service = (LuaService*)luaL_checkudata(L, 1, MT_CLUSTER_SERVICE);
                luaL_checktype(L, 2, LUA_TTABLE);
                if (lua_rawlen(L, 2) != 3) {
                    return luaL_error(L, "bad argument#2, expected table with length=3");
                }
                luaL_checktype(L, 3, LUA_TSTRING);

                lua_rawgeti(L, 2, 1);
                int nodeId = lua_tointeger(L, -1);
                lua_pop(L, 1);

                lua_rawgeti(L, 2, 2);
                int pid = lua_tointeger(L, -1);
                lua_pop(L, 1);

                lua_rawgeti(L, 2, 3);
                int session = lua_tointeger(L, -1);
                lua_pop(L, 1);

                MailboxID to(nodeId, pid);
                base::cluster::MessageOut msgout(0, 64, service->mempool());
                msgout.WriteVarInteger(2);
                write_lua_boolean(msgout, false);
                write_lua_value(msgout, L, 3);
                service->Reply(to, session, msgout);
                return 0;
            }

            static int _lua___gc(lua_State* L) {
                LuaService* service = (LuaService*)luaL_checkudata(L, 1, MT_CLUSTER_SERVICE);
                service->~LuaService();
                return 0;
            }

        public:
            LuaService(const char* service_name, lua_State* L) : RpcService(service_name, true) {
                // LuaService will not alive without lua_State, so we just save the pointer
                mainL = luaEx_get_main_thread(L);
            }

            virtual void OnCall(const MailboxID& from, uint16_t session, MessageIn& msgin) override {
                m_disableAutoReply = false;
                m_currentFrom = from;
                m_currentSession = session;
                base::cluster::MessageOut msgout(msgin.code(), 100, mempool());
                lua_State* L = mainL;
                StackKeeper sk(L);
                lua_rawgetp(L, LUA_REGISTRYINDEX, &ADDR_SERVICE_LIST);              // {serviceList}
                lua_rawgetp(L, -1, this);                                           // {serviceList}, service
                lua_getuservalue(L, -1);                                            // {serviceList}, service, serviceImpl
                int type = lua_type(L, -1);
                if (type == LUA_TFUNCTION) {
                    int top1 = lua_gettop(L);
                    int argc = msgin.ReadVarInteger<int>();
                    for (int i = 0; i < argc; ++i) {
                        read_lua_value(msgin, L);
                    }
                    if (luaEx_pcall(L, argc, LUA_MULTRET, 0) == LUA_OK) {
                        int top2 = lua_gettop(L);
                        int returnCount = top2 - top1 + 1;
                        assert(returnCount >= 0);
                        msgout.WriteVarInteger(returnCount + 1);
                        write_lua_boolean(msgout, true);
                        for (int i = top1; i <= top2; ++i) {
                            write_lua_value(msgout, L, i);
                        }
                    } else {
                        lua_pushboolean(L, false);
                        lua_pushstring(L, "service error");
                        msgout.WriteVarInteger(2);
                        int top2 = lua_gettop(L);
                        for (int i = top2 - 1; i <= top2; ++i) {
                            write_lua_value(msgout, L, i);
                        }
                    }
                } else if (type == LUA_TTABLE) {
                    string method = msgin.ReadString();
                    lua_pushstring(L, method.c_str());                              // {serviceList}, service, serviceImpl, key
                    lua_rawget(L, -2);                                              // {serviceList}, service, serviceImpl, val
                    if (lua_isfunction(L, -1)) {
                        int top1 = lua_gettop(L);
                        int argc = msgin.ReadVarInteger<int>();
                        for (int i = 0; i < argc; ++i) {
                            read_lua_value(msgin, L);
                        }
                        if (luaEx_pcall(L, argc, LUA_MULTRET, 0) == LUA_OK) {
                            int top2 = lua_gettop(L);
                            int returnCount = top2 - top1 + 1;
                            assert(returnCount >= 0);
                            msgout.WriteVarInteger(returnCount + 1);
                            write_lua_boolean(msgout, true);
                            for (int i = top1; i <= top2; ++i) {
                                write_lua_value(msgout, L, i);
                            }
                        } else {
                            lua_pushboolean(L, false);
                            lua_pushstring(L, "service error");
                            msgout.WriteVarInteger(2);
                            int top2 = lua_gettop(L);
                            for (int i = top2 - 1; i <= top2; ++i) {
                                write_lua_value(msgout, L, i);
                            }
                        }
                    } else {
                        lua_pushboolean(L, false);
                        lua_pushstring(L, "not exist method");
                        msgout.WriteVarInteger(2);
                        int top2 = lua_gettop(L);
                        for (int i = top2 - 1; i <= top2; ++i) {
                            write_lua_value(msgout, L, i);
                        }
                    }
                } else {
                    lua_pushboolean(L, false);
                    lua_pushstring(L, "not support type");
                    msgout.WriteVarInteger(2);
                    int top2 = lua_gettop(L);
                    for (int i = top2 - 1; i <= top2; ++i) {
                        write_lua_value(msgout, L, i);
                    }
                }
                if (!m_disableAutoReply) {
                    Reply(from, session, msgout);
                }
            }

            virtual void OnCast(const MailboxID& from, MessageIn& msgin) override {
                lua_State* L = mainL;
                StackKeeper sk(L);
                lua_rawgetp(L, LUA_REGISTRYINDEX, &ADDR_SERVICE_LIST);              // {serviceList}
                lua_rawgetp(L, -1, this);                                           // {serviceList}, service
                lua_getuservalue(L, -1);                                            // {serviceList}, service, serviceImpl
                int type = lua_type(L, -1);
                if (type == LUA_TFUNCTION) {
                    int argc = msgin.ReadVarInteger<int>();
                    for (int i = 0; i < argc; ++i) {
                        read_lua_value(msgin, L);
                    }
                    luaEx_pcall(L, argc, 0, 0);
                } else if (type == LUA_TTABLE) {
                    string method = msgin.ReadString();
                    //cout<<"method:"<<method<<endl;
                    lua_pushstring(L, method.c_str());                              // {serviceList}, service, serviceImpl, key
                    lua_rawget(L, -2);                                              // {serviceList}, service, serviceImpl, val
                    if (lua_isfunction(L, -1)) {
                        int argc = msgin.ReadVarInteger<int>();
                        //cout<<"argc:"<<argc<<endl;
                        for (int i = 0; i < argc; ++i) {
                            read_lua_value(msgin, L);
                        }
                        //cout<<"luaEx_pcall"<<endl;
                        luaEx_pcall(L, argc, 0, 0);
                    }
                }
            }

            virtual void OnStubConnect(const MailboxID& mbid) override {
                lua_State* L = mainL;
                StackKeeper sk(L);
                lua_rawgetp(L, LUA_REGISTRYINDEX, &ADDR_SERVICE_LIST);              // {serviceList}
                lua_rawgetp(L, -1, this);                                           // {serviceList}, service
                lua_getuservalue(L, -1);                                            // {serviceList}, service, serviceImpl
                if (lua_istable(L, -1)) {
                    lua_pushstring(L, "_onStubConnect");                        // {serviceList}, service, serviceImpl, "_onStubConnect"
                    lua_rawget(L, -2);                                          // {serviceList}, service, serviceImpl, "_onStubConnect", fun
                    if (lua_isfunction(L, -1)) {
                        luaEx_push_mailboxId(L, mbid);                          // {serviceList}, service, serviceImpl, "_onStubConnect", fun, mbid
                        luaEx_pcall(L, 1, 0, 0);
                    }
                }
            }
        private:
            lua_State* mainL;
            bool m_disableAutoReply = false;
            MailboxID m_currentFrom;
            uint16_t m_currentSession = 0u;
        };

        static int _lua_cluster_createService(lua_State* L)
        {
            const char* name = luaL_checkstring(L, 1);
            luaL_checktype(L, 2, LUA_TTABLE);
            LuaService* service = (LuaService*)lua_newuserdata(L, sizeof(LuaService));
            lua_pushvalue(L, 2);
            lua_setuservalue(L, -2);
            new(service)LuaService(name, L);
            luaL_setmetatable(L, MT_CLUSTER_SERVICE);
            if (!service->Setup()) {
                lua_pushnil(L);
                return 1;
            }
            lua_rawgetp(L, LUA_REGISTRYINDEX, &ADDR_SERVICE_LIST);
            lua_pushvalue(L, -2);
            lua_rawsetp(L, -2, service);
            lua_pop(L, 1);
            return 1;
        }

        /// LuaStub
        class LuaStub : public RpcStub
        {
        public:
            static int _lua_call(lua_State* L) {
                const char* method = luaL_checkstring(L, lua_upvalueindex(1));
                //LOG_DEBUG("------------ method -------- %s ", method);
                LuaStub* stub = (LuaStub*)luaL_checkudata(L, 1, MT_CLUSTER_STUB);

                {
                    MessageOut msgout(0, 60, stub->mempool());
                    msgout << method;
                    write_lua_message(msgout, L, 2);

                    lua_pushthread(L);
                    int callbackKey = g_lua_callback_mgr->Store(L);
                    stub->Call(msgout, [callbackKey](MessageIn & msgin) {
                        lua_State* mainL = 0;
                        if (g_lua_callback_mgr->Fetch(callbackKey, &mainL, true)) {
                            lua_State* L = lua_tothread(mainL, -1);
                            int argc = read_lua_message(msgin, L);
                            luaEx_resume(L, mainL, argc);
                            lua_pop(mainL, 1);
                        }
                    }, [callbackKey]() {
                        lua_State* mainL = 0;
                        if (g_lua_callback_mgr->Fetch(callbackKey, &mainL, true)) {
                            lua_State* L = lua_tothread(mainL, -1);
                            lua_pushboolean(L, false);
                            lua_pushstring(L, "rpc error:timeout");
                            luaEx_resume(L, mainL, 2);
                            lua_pop(mainL, 1);
                        }
                    }, stub->m_autoObserver);
                    // dtor for MessageOut
                }

                return lua_yield(L, 0);
            }

            static int _lua_cast(lua_State* L) {
                LuaStub* stub = (LuaStub*)luaL_checkudata(L, 1, MT_CLUSTER_STUB);
                const char* method = luaL_checkstring(L, lua_upvalueindex(1));
                MessageOut msgout(0, 60, stub->mempool());
                msgout << method;
                write_lua_message(msgout, L, 2);
                stub->Cast(msgout);
                return 0;
            }

            static int _lua_setSubscriber(lua_State* L) {
                LuaStub* stub = (LuaStub*)luaL_checkudata(L, 1, MT_CLUSTER_STUB);
                int t2 = lua_type(L, 2);
                if (t2 != LUA_TTABLE && t2 != LUA_TFUNCTION) {
                    return luaL_argerror(L, 2, "table or function expected");
                }
                stub->m_mainL = luaEx_get_main_thread(L);
                lua_pushvalue(L, 2);
                lua_setuservalue(L, 1);
                return 0;
            }

            static int _lua_unifyMailboxId(lua_State* L) {
                LuaStub* stub = (LuaStub*)luaL_checkudata(L, 1, MT_CLUSTER_STUB);
                MailboxID mbid = luaEx_check_mailboxId(L, 2);
                stub->UnifyWithServiceMailbox(mbid);
                luaEx_push_mailboxId(L, mbid);
                return 1;
            }

            static int _lua___index(lua_State* L) {
                const char* method = luaL_checkstring(L, 2);
                if (strcmp(method, "setSubscriber") == 0) {
                    lua_pushcfunction(L, LuaStub::_lua_setSubscriber);
                } else if (strcmp(method, "unifyMailboxId") == 0) {
                    lua_pushcfunction(L, LuaStub::_lua_unifyMailboxId);
                } else {
                    if (strncmp(method, "cast_", 5) == 0) {
                        method = method + 5;
                        lua_pushstring(L, method);
                        lua_pushcclosure(L, LuaStub::_lua_cast, 1);
                    } else if (strncmp(method, "call_", 5) == 0) {
                        method = method + 5;
                        lua_pushstring(L, method);
                        lua_pushcclosure(L, LuaStub::_lua_call, 1);
                    } else {
                        lua_pushcclosure(L, LuaStub::_lua_call, 1);
                    }
                }
                return 1;
            }

            static int _lua___gc(lua_State* L) {
                LuaStub* stub = (LuaStub*)luaL_checkudata(L, 1, MT_CLUSTER_STUB);
                stub->~LuaStub();
                return 0;
            }

        public:
            LuaStub(const char* serviceName, bool autoReconnect) : RpcStub(serviceName, autoReconnect), m_mainL(NULL) {}
            ~LuaStub() {}

        private:
            virtual void OnSubscribeArrive(MessageIn& msgin) override {
                if (m_mainL == NULL) {
                    return;
                }
                lua_State* L = m_mainL;
                StackKeeper sk(L);

                lua_rawgetp(L, LUA_REGISTRYINDEX, &ADDR_STUB_LIST);                     // {stub list}
                lua_rawgetp(L, -1, this);                                               // {stub list}, LuaStub
                lua_getuservalue(L, -1);                                                // {stub list}, LuaStub, subscriber
                int type = lua_type(L, -1);
                if (type == LUA_TFUNCTION) {
                    int argc = read_lua_message(msgin, L);
                    luaEx_pcall(L, argc, 0, 0);
                } else if (type == LUA_TTABLE) {
                    int argc = read_lua_message(msgin, L);                              // {stub list}, LuaStub, subscriber, {key,...}
                    if (argc == 0) {
                        return;
                    }
                    lua_pushvalue(L, -argc);                                            // {stub list}, LuaStub, subscriber, {key,...}, key
                    lua_rawget(L, -argc - 2);                                           // {stub list}, LuaStub, subscriber, {key,...}, handler
                    lua_replace(L, -argc - 1);                                          // {stub list}, LuaStub, subscriber, {handler,...}
                    if (lua_isfunction(L, -argc)) {
                        luaEx_pcall(L, argc - 1, 0, 0);
                    }
                }
            }
            AutoObserver m_autoObserver;
            lua_State* m_mainL = NULL;
        };

        static int _lua_cluster_connectService(lua_State* L)
        {
            const char* serviceName = luaL_checkstring(L, 1);
            bool autoReconnect = lua_toboolean(L, 2);
            // TODO OPTIMIZE first we shoud read from cache, if exist, we just increment the reference
            // should notice: setSubscriber function shoud add all subscribe to a list
            LuaStub* stub = (LuaStub*)lua_newuserdata(L, sizeof(LuaStub));              // LuaStub,
            new(stub)LuaStub(serviceName, autoReconnect);
            luaL_setmetatable(L, MT_CLUSTER_STUB);
            lua_rawgetp(L, LUA_REGISTRYINDEX, &ADDR_STUB_LIST);                         // LuaStub, {stub list}
            lua_pushvalue(L, -2);                                                       // LuaStub, {stub list}, LuaStub
            lua_rawsetp(L, -2, stub);                                                   // LuaStub, {stub, list}
            lua_pop(L, 1);                                                              // LuaStub
            lua_newtable(L);                                                            // LuaStub, {}
            if (lua_pushthread(L) == 1) {                                               // LuaStub, {}, thread
                return luaL_error(L, "can not do this operator in main thread");
            }
            lua_rawseti(L, -2, 1);                                                      // LuaStub, {thread}
            lua_pushvalue(L, -2);                                                       // LuaStub, {thread}, LuaStub
            lua_rawseti(L, -2, 2);                                                      // LuaStub, {thread, LuaStub}

            {
                int callbackKey = g_lua_callback_mgr->Store(L);                             // LuaStub
                stub->BeginSetup([callbackKey](bool ok) {
                    lua_State* mainL = 0;
                    if (g_lua_callback_mgr->Fetch(callbackKey, &mainL, true)) {             // {thread, LuaStub}
                        lua_rawgeti(mainL, -1, 1);                                          // {thread, LuaStub}, thread
                        if (ok) {
                            lua_rawgeti(mainL, -2, 2);                                          // {thread, LuaStub}, thread, LuaStub
                        } else {
                            lua_pushnil(mainL);
                        }
                        lua_State* L = lua_tothread(mainL, -2);
                        lua_xmove(mainL, L, 1);
                        if (lua_status(L) == LUA_YIELD) {
                            luaEx_resume(L, mainL, 1);
                        }
                        lua_pop(mainL, 2);
                    }
                });
            }
            if (!stub->is_service_avaiable()) {
                return lua_yield(L, 0);
            } else {
                return 1;
            }
        }

        /// LuaMailbox
        class LuaMailbox : public Mailbox::EventHandler
        {
        public:
            static int _lua_getMailboxId(lua_State* L) {
                LuaMailbox* mailbox = (LuaMailbox*)luaL_checkudata(L, 1, MT_CLUSTER_MAILBOX);
                if (mailbox->m_mailBox == nullptr) {
                    return 0;
                }
                const MailboxID& mbid = mailbox->m_mailBox->mbid();
                luaEx_push_mailboxId(L, mbid);
                return 1;
            }

            static int _lua_getMailboxPtr(lua_State* L) {
                LuaMailbox* mailbox = (LuaMailbox*)luaL_checkudata(L, 1, MT_CLUSTER_MAILBOX);
                lua_pushlightuserdata(L, mailbox->m_mailBox);
                return 1;
            }

            static int _lua_cast(lua_State* L) {
                LuaMailbox* mailbox = (LuaMailbox*)luaL_checkudata(L, 1, MT_CLUSTER_MAILBOX);
                luaL_checktype(L, 2, LUA_TTABLE);
                if (mailbox->m_mailBox == nullptr) {
                    return luaL_error(L, "mailbox is deleted");
                }

                MailboxID to = luaEx_check_mailboxId(L, 2);

                MessageOut msgout(0, 60, mailbox->m_mailBox->mempool());
                write_lua_message(msgout, L, 3);
                mailbox->m_mailBox->Cast(to, msgout);
                return 0;
            }

            static int _lua_call(lua_State* L) {
                return luaL_error(L, "not implement");
            }

            static int _lua_delete(lua_State* L) {
                LuaMailbox* mailbox = (LuaMailbox*)luaL_checkudata(L, 1, MT_CLUSTER_MAILBOX);
                SAFE_DELETE(mailbox->m_mailBox);
                return 0;
            }

            static int _lua___gc(lua_State* L) {
                LuaMailbox* mailbox = (LuaMailbox*)luaL_checkudata(L, 1, MT_CLUSTER_MAILBOX);
                mailbox->~LuaMailbox();
                return 0;
            }

        public:
            LuaMailbox(lua_State* L) {
                m_mailBox = Mailbox::Create(*this);
                m_mainL = luaEx_get_main_thread(L);
            }
            ~LuaMailbox() {
                SAFE_DELETE(m_mailBox);
            }

        private:
            virtual void OnMessageReceive(MessageIn& msgin) override {
                lua_State* L = m_mainL;
                StackKeeper sk(L);
                lua_rawgetp(L, LUA_REGISTRYINDEX, &ADDR_MAILBOX_LIST);                  // {mailboxList}
                lua_rawgetp(L, -1, this);                                               // {mailboxList}, mailbox
                lua_getuservalue(L, -1);                                                // {mailboxList}, mailbox, handler

                int type = lua_type(L, -1);
                if (msgin.code() > 0) {
                    string msg = msgin.Dump(true);
                    LOG_ERROR("code=%d,session=%d,size=%d,type=%d,from.nodeid=%d,from.pid=%d,to.nodeid=%d,to.pid=%d\n",
                              msgin.code(), msgin.session(), msgin.size(), type, msgin.from().nodeid(), msgin.from().pid(), msgin.to().nodeid(), msgin.to().pid());
                    return;
                }
                if (type == LUA_TFUNCTION) {
                    int argc = read_lua_message(msgin, L);
                    luaEx_pcall(L, argc, 0, 0);
                } else if (type == LUA_TTABLE) {
                    string method = msgin.ReadString();
                    lua_pushstring(L, method.c_str());
                    lua_rawget(L, -2);
                    if (lua_isfunction(L, -1)) {
                        int argc = read_lua_message(msgin, L);
                        luaEx_pcall(L, argc, 0, 0);
                    }
                }
            }

            Mailbox* m_mailBox;
            lua_State* m_mainL;
        };

        static int _lua_cluster_createMailbox(lua_State* L)
        {
            int t = lua_type(L, 1);
            if (t != LUA_TTABLE && t != LUA_TFUNCTION) {
                return luaL_argerror(L, 1, "table or function expected");
            }
            LuaMailbox* mailbox = (LuaMailbox*)lua_newuserdata(L, sizeof(LuaMailbox));  // mailbox
            new(mailbox)LuaMailbox(L);
            luaL_setmetatable(L, MT_CLUSTER_MAILBOX);
            lua_pushvalue(L, 1);                                                        // mailbox, handler,
            lua_setuservalue(L, -2);                                                    // mailbox
            lua_rawgetp(L, LUA_REGISTRYINDEX, &ADDR_MAILBOX_LIST);                      // mailbox, {mailboxList}
            lua_pushvalue(L, -2);                                                       // mailbox, {mailboxList}, mailbox
            lua_rawsetp(L, -2, mailbox);                                                // mailbox, {mailboxList}
            lua_pop(L, 1);                                                              // mailbox
            return 1;
        }

        static int _lua_cluster_addNodeEventListener(lua_State* L)
        {
            luaL_checktype(L, 1, LUA_TTABLE);
            lua_rawgetp(L, LUA_REGISTRYINDEX, &ADDR_EVENT_LISTENER_LIST);               // {events}
            lua_pushvalue(L, 1);
            int len = lua_rawlen(L, -1);
            lua_rawseti(L, -2, len + 1);
            return 0;
        }

        class LuaNodeEventListener
        {
        public:
            LuaNodeEventListener(lua_State* L) {
                m_mainL = luaEx_get_main_thread(L);
                NodeMonitor::instance().evt_node_up.Attach(std::bind(&LuaNodeEventListener::OnNodeUp, this, std::placeholders::_1), m_autoObserver);
                NodeMonitor::instance().evt_node_down.Attach(std::bind(&LuaNodeEventListener::OnNodeDown, this, std::placeholders::_1), m_autoObserver);
            }
            ~LuaNodeEventListener() {
            }

            static int _lua___gc(lua_State* L) {
                LuaNodeEventListener* listener = (LuaNodeEventListener*)luaL_checkudata(L, 1, MT_CLUSTER_EVENT_LISTENER);
                listener->~LuaNodeEventListener();
                return 0;
            }

        private:
            void OnNodeUp(const NodeInfo& nodeInfo) {
                lua_State* L = m_mainL;
                StackKeeper sk(L);
                lua_rawgetp(L, LUA_REGISTRYINDEX, &ADDR_EVENT_LISTENER_LIST);       // {event list}
                lua_pushnil(L);                                                     // {event list}, nil
                while (lua_next(L, -2) != 0) {                                      // {event list}, key, val
                    if (lua_istable(L, -1)) {
                        int top1 = lua_gettop(L);
                        lua_pushstring(L, "onNodeUp");                              // {event list}, key, val, "onNodeUp"
                        lua_rawget(L, -2);                                          // {event list}, key, val, "onNodeUp", fun
                        if (lua_isfunction(L, -1)) {
                            lua_pushinteger(L, nodeInfo.node_id);
                            lua_pushstring(L, nodeInfo.node_name.c_str());
                            lua_pushinteger(L, (int)nodeInfo.type);
                            luaEx_pcall(L, 3, 0, 0);
                        }
                        lua_settop(L, top1);
                    }
                    lua_pop(L, 1);
                }
            }

            void OnNodeDown(const NodeInfo& nodeInfo) {
                lua_State* L = m_mainL;
                StackKeeper sk(L);
                lua_rawgetp(L, LUA_REGISTRYINDEX, &ADDR_EVENT_LISTENER_LIST);       // {event list}
                lua_pushnil(L);                                                     // {event list}, nil
                while (lua_next(L, -2) != 0) {                                      // {event list}, key, val
                    if (lua_istable(L, -1)) {
                        int top1 = lua_gettop(L);
                        lua_pushstring(L, "onNodeDown");                            // {event list}, key, val, "onNodeDown"
                        lua_rawget(L, -2);                                          // {event list}, key, val, "onNodeDown", fun
                        if (lua_isfunction(L, -1)) {
                            lua_pushinteger(L, nodeInfo.node_id);
                            lua_pushstring(L, nodeInfo.node_name.c_str());
                            lua_pushinteger(L, (int)nodeInfo.type);
                            luaEx_pcall(L, 3, 0, 0);
                        }
                        lua_settop(L, top1);
                    }
                    lua_pop(L, 1);
                }
            }

            AutoObserver m_autoObserver;
            lua_State* m_mainL;
        };

        static int _lua_open_cluster(lua_State* L)
        {
            // class Service
            luaL_Reg serviceFuncs[] = {
                {"publish", LuaService::_lua_publish},
                {"publishToAll", LuaService::_lua_publishToAll},
                {"publishToAllExcept", LuaService::_lua_publishToAllExcept},
                {"cast", LuaService::_lua_cast},
                {"from", LuaService::_lua_from},
                {"disableAutoReply", LuaService::_lua_disableAutoReply},
                {"reply", LuaService::_lua_reply},
                {"replyError", LuaService::_lua_replyError},
                {"session", LuaService::_lua_session},
                {"__gc", LuaService::_lua___gc},
                {nullptr, nullptr},
            };

            int ok = luaL_newmetatable(L, MT_CLUSTER_SERVICE);
            assert(ok == 1);
            luaL_setfuncs(L, serviceFuncs, 0);
            lua_pushstring(L, "__index");
            lua_pushvalue(L, -2);
            lua_rawset(L, -3);
            lua_pop(L, 1);

            // class Stub
            luaL_Reg stubFuncs[] = {
                {"__gc", LuaStub::_lua___gc},
                {"__index", LuaStub::_lua___index},
                {nullptr, nullptr},
            };

            ok = luaL_newmetatable(L, MT_CLUSTER_STUB);
            assert(ok == 1);
            luaL_setfuncs(L, stubFuncs, 0);
            lua_pop(L, 1);

            // class LuaNodeEventListener
            luaL_Reg nodeEventListenerFuncs[] = {
                {"__gc", LuaNodeEventListener::_lua___gc},
                {nullptr, nullptr},
            };

            ok = luaL_newmetatable(L, MT_CLUSTER_EVENT_LISTENER);
            assert(ok == 1);
            luaL_setfuncs(L, nodeEventListenerFuncs, 0);
            lua_pop(L, 1);

            // node event listener
            LuaNodeEventListener* listener = (LuaNodeEventListener*)lua_newuserdata(L, sizeof(LuaNodeEventListener));
            new(listener)LuaNodeEventListener(L);
            luaL_setmetatable(L, MT_CLUSTER_EVENT_LISTENER);
            lua_rawsetp(L, LUA_REGISTRYINDEX, listener);

            // class Mailbox
            luaL_Reg mailboxFuncs[] = {
                {"getMailboxID", LuaMailbox::_lua_getMailboxId},
                {"getMailboxId", LuaMailbox::_lua_getMailboxId},
                {"getMailboxPtr", LuaMailbox::_lua_getMailboxPtr},
                {"cast", LuaMailbox::_lua_cast},
                {"call", LuaMailbox::_lua_call},
                {"delete", LuaMailbox::_lua_delete},
                {"__gc", LuaMailbox::_lua___gc},
                {nullptr, nullptr},
            };
            ok = luaL_newmetatable(L, MT_CLUSTER_MAILBOX);
            assert(ok == 1);
            luaL_setfuncs(L, mailboxFuncs, 0);
            lua_pushstring(L, "__index");
            lua_pushvalue(L, -2);
            lua_rawset(L, -3);
            lua_pop(L, 1);

            // table to store all service
            lua_newtable(L);
            luaL_newmetatable(L, MT_SERVICE_LIST);
            lua_pushstring(L, "v");
            lua_setfield(L, -2, "__mode");
            lua_setmetatable(L, -2);
            lua_rawsetp(L, LUA_REGISTRYINDEX, &ADDR_SERVICE_LIST);

            // table to store all stub
            lua_newtable(L);
            luaL_newmetatable(L, MT_STUB_LIST);
            lua_pushstring(L, "v");
            lua_setfield(L, -2, "__mode");
            lua_setmetatable(L, -2);
            lua_rawsetp(L, LUA_REGISTRYINDEX, &ADDR_STUB_LIST);

            // table to store all mailbox
            lua_newtable(L);
            luaL_newmetatable(L, MT_MAILBOX_LIST);
            lua_pushstring(L, "v");
            lua_setfield(L, -2, "__mode");
            lua_setmetatable(L, -2);
            lua_rawsetp(L, LUA_REGISTRYINDEX, &ADDR_MAILBOX_LIST);

            // table to store all event
            lua_newtable(L);
            luaL_newmetatable(L, MT_EVENT_LIST);
            lua_pushstring(L, "v");
            lua_setfield(L, -2, "__mode");
            lua_setmetatable(L, -2);
            lua_rawsetp(L, LUA_REGISTRYINDEX, &ADDR_EVENT_LISTENER_LIST);

            luaL_Reg libFuns[] = {
                {"createService", _lua_cluster_createService},
                {"connectService", _lua_cluster_connectService},
                {"createMailbox", _lua_cluster_createMailbox},
                {"addNodeEventListener", _lua_cluster_addNodeEventListener},
                {nullptr, nullptr}
            };

            luaL_newlib(L, libFuns);
            return 1;
        }

        void luaopen_cluster(lua_State* L)
        {
            luaL_requiref(L, "cluster", &_lua_open_cluster, 0);
            lua_pop(L, 1);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
