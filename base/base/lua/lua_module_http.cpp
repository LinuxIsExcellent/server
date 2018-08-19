#include "lua_module_http.h"
#include "luacallbackmgr.h"
#include "luaex.h"
#include "lua_module_utils.h"
#include "../http/httpserver.h"
#include "../http/httpclient.h"
#include "../http/urlbuilder.h"
#include "../http/response.h"
#include <lua/lua.hpp>
#include "../logger.h"

namespace base
{
    namespace lua
    {
        using namespace std;
        using namespace base::http;

        static char HTTP_SERVER_LIST_ADDR = 'c';

        /// client api

        enum class ResponseParseType
        {
            STRING,
            JSON,
        };

        class HttpRequestEventHandler : public base::http::HttpClientEventHandler
        {
        public:
            HttpRequestEventHandler(lua_State* _L, ResponseParseType parseType, int callbackKey, Observer* observer)
                : HttpClientEventHandler(observer), m_parseType(parseType), m_callbackKey(callbackKey), L(_L) {}

            virtual void OnHttpClose() override {
            }

            virtual void OnHttpResponse(HttpStatusCode code, const string& body) override {
                lua_State* mainL = 0;
                if (g_lua_callback_mgr->Fetch(m_callbackKey, &mainL, true)) {
                    lua_State* L = lua_tothread(mainL, -1);
                    lua_pushinteger(L, (int)code);
                    if (m_parseType == ResponseParseType::STRING) {
                        lua_pushlstring(L, body.c_str(), body.length());
                    } else {
                        lua_utils_fromJson(L, body.c_str());
                    }
                    luaEx_resume(L, mainL, 2);
                    lua_pop(mainL, 1);
                }
            }

        private:
            ResponseParseType m_parseType;
            int m_callbackKey;
            lua_State* L;
        };

        static int _lua_http_doRequest(lua_State* L, ResponseParseType parseType, HttpMethod method)
        {
            HttpClient::Error error;
            const char* url = luaL_checkstring(L, 1);

            {
                //lua_yield会调用long_jump, 所以用{}来控制下面两个变量的生命周期
                UrlBuilder urlBuilder(url);
                string json;

                if (lua_istable(L, 2)) {
                    if (method == HttpMethod::GET) {
                        lua_pushnil(L);
                        while (lua_next(L, 2) != 0) {
                            const char* key = lua_tostring(L, -2);
                            const char* val = lua_tostring(L, -1);
                            urlBuilder.appendQueryParam(key, val);
                            lua_pop(L, 1);
                        }
                    } else {
                        json = lua_utils_toJson(L, 2);
                    }
                }

                if (lua_pushthread(L) == 1) {
                    return luaL_error(L, "can not do this operator in main thread");
                }

                int callbackKey = g_lua_callback_mgr->Store(L);

                Observer* observer = g_lua_callback_mgr->GetObserver(L);
                HttpRequestEventHandler* handler = new HttpRequestEventHandler(L, parseType, callbackKey, observer);
                handler->AutoRelease();
                if (method == HttpMethod::POST) {
                    error = HttpClient::instance()->PostJsonAsync(urlBuilder.getResult(), json, handler);
                } else {
                    error = HttpClient::instance()->GetAsync(urlBuilder.getResult(), handler);
                }
            }
            if (error == HttpClient::Error::OK) {
                // 挂起当前协程
                return lua_yield(L, 0);
            } else {
                lua_pushboolean(L, false);
                lua_pushstring(L, "bad uri");
                return 2;
            }
        }

        static int _lua_http_getForString(lua_State* L)
        {
            return _lua_http_doRequest(L, ResponseParseType::STRING, HttpMethod::GET);
        }

        static int _lua_http_getForObject(lua_State* L)
        {
            return _lua_http_doRequest(L, ResponseParseType::JSON, HttpMethod::GET);
        }

        static int _lua_http_postForString(lua_State* L)
        {
            return _lua_http_doRequest(L, ResponseParseType::STRING, http::HttpMethod::POST);
        }

        static int _lua_http_postForObject(lua_State* L)
        {
            return _lua_http_doRequest(L, ResponseParseType::JSON, http::HttpMethod::POST);
        }

        /// server api

        static const char* MT_HTTP_SERVER = "mt_http_server";
        static const char* MT_HTTP_RESPONSE = "mt_http_response";

        class LuaHttpServer : public HttpServer
        {
        public:
            LuaHttpServer(lua_State* L) : m_L(L) {
                m_L = luaEx_get_main_thread(L); // 注意，可能在协程中被创建，因此需要存储mainThread
            }
            virtual ~LuaHttpServer() {
            }

            static int _lua_setHandlers(lua_State* L) {
                LuaHttpServer* server = (LuaHttpServer*)luaL_checkudata(L, 1, MT_HTTP_SERVER);
                luaL_checktype(L, 2, LUA_TTABLE);
                lua_pushvalue(L, 2);
                lua_setuservalue(L, 1);
                server->RegisterDefaultHandler();
                return 0;
            }

            static int _lua_setResourceDir(lua_State* L) {
                LuaHttpServer* server = (LuaHttpServer*)luaL_checkudata(L, 1, MT_HTTP_SERVER);
                const char* resourceDir = luaL_checkstring(L, 2);
                server->SetResourcesDir(resourceDir);
                return 0;
            }

            static int _lua_stop(lua_State* L) {
                LuaHttpServer* server = (LuaHttpServer*)luaL_checkudata(L, 1, MT_HTTP_SERVER);
                server->BeginCleanup([]() {});
                return 0;
            }

            static int _lua___gc(lua_State* L) {
                LuaHttpServer* server = (LuaHttpServer*)luaL_checkudata(L, 1, MT_HTTP_SERVER);
                server->BeginCleanup([]() {});
                server->~LuaHttpServer();
                return 0;
            }

        private:
            void RegisterDefaultHandler() {
                SetDefaultHandler(std::bind(&LuaHttpServer::HttpRequestHandler, this, std::placeholders::_1), m_autoObserver.GetObserver());
            }

            bool HttpRequestHandler(const Request& request) {
                lua_State* L  = m_L;
                StackKeeper sk(L);
                lua_rawgetp(L, LUA_REGISTRYINDEX, this);
                int udServerIdx = lua_gettop(L);
                lua_getuservalue(L, -1);
                if (!lua_istable(L, -1)) {
                    return false;
                }
                lua_pushlstring(L, request.path.c_str(), request.path.length());
                int rt = lua_gettable(L, -2);
                if (rt == LUA_TFUNCTION) {
                    // 创建请求数据
                    lua_newtable(L);
                    lua_pushstring(L, "method");
                    lua_pushstring(L, HTTP_METHOD_STRING(request.method));
                    lua_rawset(L, -3);
                    lua_pushstring(L, "path");
                    lua_pushlstring(L, request.path.c_str(), request.path.length());
                    lua_rawset(L, -3);
                    lua_pushstring(L, "queries");
                    lua_newtable(L);
                    for (const pair<string, string>& q : request.queries) {
                        lua_pushlstring(L, q.first.c_str(), q.first.length());
                        lua_pushlstring(L, q.second.c_str(), q.second.length());
                        lua_rawset(L, -3);
                    }
                    lua_rawset(L, -3);
                    lua_pushstring(L, "headers");
                    lua_newtable(L);
                    for (const pair<string, string>& h : request.headers) {
                        lua_pushlstring(L, h.first.c_str(), h.first.length());
                        lua_pushlstring(L, h.second.c_str(), h.second.length());
                        lua_rawset(L, -3);
                    }
                    lua_rawset(L, -3);
                    lua_pushstring(L, "body");
                    lua_pushlstring(L, request.body.c_str(), request.body.length());
                    lua_rawset(L, -3);

                    // 创建响应对象
                    lua_newtable(L);
                    luaL_setmetatable(L, MT_HTTP_RESPONSE);
                    lua_pushstring(L, "__server");
                    lua_pushvalue(L, udServerIdx);
                    lua_rawset(L, -3);
                    lua_pushstring(L, "__session");
                    lua_pushinteger(L, request.session);
                    lua_rawset(L, -3);

                    luaEx_pcall(L, 2, 1, 0);
                    return true;
                } else {
                    return false;
                }
            }

            lua_State* m_L;
            base::AutoObserver m_autoObserver;
        };

        static int _lua_http_createServer(lua_State* L)
        {
            const char* ip = luaL_checkstring(L, 1);
            int port = luaL_checkinteger(L, 2);

            LuaHttpServer* server = (LuaHttpServer*)lua_newuserdata(L, sizeof(LuaHttpServer));
            new(server)LuaHttpServer(L);
            luaL_setmetatable(L, MT_HTTP_SERVER);
            server->Start(ip, port);

            // 存储在列表上
            lua_pushvalue(L, -1);
            lua_rawsetp(L, LUA_REGISTRYINDEX, server); // TODO fix me , move to REGISTER SUB VALUE

            return 1;
        }

        static void fillHeaders(lua_State* L, int respIdx, Response& response)
        {
            lua_getfield(L, respIdx, "headers");
            if (lua_istable(L, -1)) {
                lua_pushnil(L);
                while (lua_next(L, -2)) {
                    if (lua_type(L, -2) == LUA_TSTRING) {
                        const char* name = lua_tostring(L, -2);
                        const char* value = lua_tostring(L, -1);
                        response.AddHeader(name, value);
                    }
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1);
        }

        static int _lua_http_response_send(lua_State* L)
        {
            luaL_checktype(L, 1, LUA_TTABLE);
            lua_pushstring(L, "__server");
            lua_rawget(L, 1);
            LuaHttpServer* server = (LuaHttpServer*)luaL_checkudata(L, -1, MT_HTTP_SERVER);
            lua_pushstring(L, "__session");
            lua_rawget(L, 1);
            int session = luaL_checkinteger(L, -1);
            const char* content = luaL_checkstring(L, 2);

            Response response;
            fillHeaders(L, 1, response);
            response.SetContent(content);
            server->SendResponse(session, response);
            return 0;
        }

        static int _lua_http_response_sendHtml(lua_State* L)
        {
            luaL_checktype(L, 1, LUA_TTABLE);
            lua_pushstring(L, "__server");
            lua_rawget(L, 1);
            LuaHttpServer* server = (LuaHttpServer*)luaL_checkudata(L, -1, MT_HTTP_SERVER);
            lua_pushstring(L, "__session");
            lua_rawget(L, 1);
            int session = luaL_checkinteger(L, -1);
            const char* html = luaL_checkstring(L, 2);

            HtmlResponse response;
            fillHeaders(L, 1, response);
            response.AppendHtml(html);
            server->SendResponse(session, response);
            return 0;
        }

        static int _lua_http_response_sendJson(lua_State* L)
        {
            luaL_checktype(L, 1, LUA_TTABLE);
            lua_pushstring(L, "__server");
            lua_rawget(L, 1);
            LuaHttpServer* server = (LuaHttpServer*)luaL_checkudata(L, -1, MT_HTTP_SERVER);
            lua_pushstring(L, "__session");
            lua_rawget(L, 1);
            int session = luaL_checkinteger(L, -1);
            string json = lua_utils_toJson(L, 2);

            Response response;
            fillHeaders(L, 1, response);
            response.AddHeader("Content-Type", "application/json;charset=utf-8");
            response.SetContent(move(json));
            server->SendResponse(session, response);
            return 0;
        }

        /// export

        static int _lua_open_http(lua_State* L)
        {
            // 用于保存所有httpServer对象
            lua_newtable(L);
            lua_pushstring(L, "list_httpServer");
            lua_setfield(L, -2, "collect_name");
            lua_pushvalue(L, -1);
            lua_pushstring(L, "__mode");
            lua_pushstring(L, "v");
            lua_rawset(L, -3);
            lua_setmetatable(L, -2);
            lua_rawsetp(L, LUA_REGISTRYINDEX, &HTTP_SERVER_LIST_ADDR);

            // class:httpServer
            luaL_Reg httpServerFuns[] = {
                {"setHandlers", LuaHttpServer::_lua_setHandlers},
                {"setResourceDir", LuaHttpServer::_lua_setResourceDir},
                {"stop", LuaHttpServer::_lua_stop},
                {"__gc", LuaHttpServer::_lua___gc},
                {nullptr, nullptr},
            };

            int ok = luaL_newmetatable(L, MT_HTTP_SERVER);
            assert(ok == 1);
            luaL_setfuncs(L, httpServerFuns, 0);
            lua_pushstring(L, "__index");
            lua_pushvalue(L, -2);
            lua_rawset(L, -3);
            lua_pop(L, 1);

            // class:httpResponse
            luaL_Reg httpResponseFuns[] = {
                {"send", _lua_http_response_send},
                {"sendJson", _lua_http_response_sendJson},
                {"sendHtml", _lua_http_response_sendHtml},
                {nullptr, nullptr},
            };

            ok = luaL_newmetatable(L, MT_HTTP_RESPONSE);
            assert(ok == 1);
            luaL_setfuncs(L, httpResponseFuns, 0);
            lua_pushstring(L, "__index");
            lua_pushvalue(L, -2);
            lua_rawset(L, -3);
            lua_pop(L, 1);

            // http module
            luaL_Reg httpFuns[] = {
                {"getForString", _lua_http_getForString},
                {"getForObject", _lua_http_getForObject},
                {"postForString", _lua_http_postForString},
                {"postForObject", _lua_http_postForObject},
                {"createServer", _lua_http_createServer},
                {nullptr, nullptr}
            };

            luaL_newlib(L, httpFuns);
            return 1;
        }

        void luaopen_http(lua_State* L)
        {
            luaL_requiref(L, "http", &_lua_open_http, 0);
            lua_pop(L, 1);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
