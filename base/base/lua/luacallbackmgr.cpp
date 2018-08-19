#include "luacallbackmgr.h"
#include "../observer.h"
#include "luaex.h"
#include <lua.hpp>

namespace base
{
    namespace lua
    {
        using namespace std;

        static const char CALLBACK_OBSERER_ADDR = 'c';
        static const char* MT_OBSERVER = "mt_observer";

        struct CallbackObject {
            CallbackObject(lua_State* _mainL, int _key, Observer* _observer)
                : mainL(_mainL), key(_key), observer(_observer) {
                observer->Retain();
            }
            ~CallbackObject() {
                observer->Release();
            }
            lua_State* mainL;
            int key;
            Observer* observer;
        };

        static int _lua_autoObserver___gc(lua_State* L)
        {
            AutoObserver* autoObserver = (AutoObserver*)luaL_checkudata(L, 1, MT_OBSERVER);
            autoObserver->~AutoObserver();
            return 0;
        }

        ///
        LuaCallbackMgr* g_lua_callback_mgr = nullptr;

        LuaCallbackMgr* LuaCallbackMgr::Create()
        {
            if (g_lua_callback_mgr == nullptr) {
                g_lua_callback_mgr = new LuaCallbackMgr();
            }
            return g_lua_callback_mgr;
        }

        void LuaCallbackMgr::Delete()
        {
            SAFE_DELETE(g_lua_callback_mgr);
        }

        LuaCallbackMgr::LuaCallbackMgr()
        {
        }

        LuaCallbackMgr::~LuaCallbackMgr()
        {
            for (auto it = m_callbackObjs.begin(); it != m_callbackObjs.end(); ++it) {
                delete it->second;
            }
            m_callbackObjs.clear();
        }

        Observer* LuaCallbackMgr::GetObserver(lua_State* L)
        {
            lua_rawgetp(L, LUA_REGISTRYINDEX, &CALLBACK_OBSERER_ADDR);
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1);
                AutoObserver* autoObserver = (AutoObserver*)lua_newuserdata(L, sizeof(AutoObserver));
                new(autoObserver)AutoObserver();
                if (luaL_newmetatable(L, MT_OBSERVER) != 0) {
                    lua_pushstring(L, "__gc");
                    lua_pushcfunction(L, _lua_autoObserver___gc);
                    lua_rawset(L, -3);
                }
                lua_setmetatable(L, -2);
                lua_pushvalue(L, -1);
                lua_newtable(L);
                lua_setuservalue(L, -2);
                lua_rawsetp(L, LUA_REGISTRYINDEX, &CALLBACK_OBSERER_ADDR);
            }

            AutoObserver* autoObserver = (AutoObserver*)luaL_checkudata(L, -1, MT_OBSERVER);
            return autoObserver->GetObserver();
        }

        int LuaCallbackMgr::Store(lua_State* L)
        {
            int idx = lua_gettop(L);
            lua_rawgetp(L, LUA_REGISTRYINDEX, &CALLBACK_OBSERER_ADDR);
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1);
                AutoObserver* autoObserver = (AutoObserver*)lua_newuserdata(L, sizeof(AutoObserver));
                new(autoObserver)AutoObserver();
                if (luaL_newmetatable(L, MT_OBSERVER) != 0) {
                    lua_pushstring(L, "__gc");
                    lua_pushcfunction(L, _lua_autoObserver___gc);
                    lua_rawset(L, -3);
                }
                lua_setmetatable(L, -2);
                lua_pushvalue(L, -1);
                lua_newtable(L);
                lua_setuservalue(L, -2);
                lua_rawsetp(L, LUA_REGISTRYINDEX, &CALLBACK_OBSERER_ADDR);
            }

            AutoObserver* autoObserver = (AutoObserver*)luaL_checkudata(L, -1, MT_OBSERVER);
            Observer* observer = autoObserver->GetObserver();
            int key = GenerateKey();
            CallbackObject* obj = new CallbackObject(luaEx_get_main_thread(L), key, observer);
            m_callbackObjs.emplace(key, obj);

            lua_getuservalue(L, -1);
            lua_pushvalue(L, idx);
            lua_rawsetp(L, -2, obj);
            lua_pop(L, 2);

            return key;
        }


        bool LuaCallbackMgr::Fetch(int key, lua_State** mainL, bool cleanup)
        {
            auto it = m_callbackObjs.find(key);
            if (it == m_callbackObjs.end()) {
                return false;
            }

            CallbackObject* obj = it->second;
            if (!obj->observer->IsExist()) {
                delete obj;
                m_callbackObjs.erase(it);
                return false;
            }

            lua_State* L = obj->mainL;
            *mainL = L;

            lua_rawgetp(L, LUA_REGISTRYINDEX, &CALLBACK_OBSERER_ADDR);  // userdata
            lua_getuservalue(L, -1);                                    // userdata, {uservalue}
            lua_rawgetp(L, -1, obj);                                    // userdata, {uservalue}, obj
            lua_replace(L, -3);                                         // obj, {uservalue}

            if (cleanup) {
                delete obj;
                m_callbackObjs.erase(it);
                lua_pushnil(L);                                         // obj, {uservalue}, nil
                lua_rawsetp(L, -2, obj);                                // obj, {uservalue}
            }

            lua_pop(L, 1);
            return true;
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
