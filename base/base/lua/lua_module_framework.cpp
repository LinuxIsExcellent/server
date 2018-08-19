#include "lua_module_framework.h"
#include "luacallbackmgr.h"
#include "luaex.h"
#include <lua/lua.hpp>
#include "../framework.h"

namespace base
{
    namespace lua
    {
        using namespace std;

        class LuaBeforeShutdown : public BeforeShutdownCmd
        {
        public:
            LuaBeforeShutdown(int key) : m_key(key) {}
            virtual ~LuaBeforeShutdown() {}

            virtual void OnExecute() override {
                lua_State* mainL = nullptr;
                if (g_lua_callback_mgr->Fetch(m_key, &mainL, true)) {
                    lua_pushlightuserdata(mainL, this);
                    lua_pushcclosure(mainL, _lua_callback, 1);
                    int r = luaEx_pcall(mainL, 1, 0, 0);
                    if (r != LUA_OK) {
                        m_finish = true;
                    }
                } else {
                    m_finish = true;
                }
            }

            virtual bool IsFinish() override {
                return m_finish;
            }

        private:
            static int _lua_callback(lua_State* L) {
                LuaBeforeShutdown* self = (LuaBeforeShutdown*)lua_touserdata(L, lua_upvalueindex(1));
                self->m_finish = true;
                return 0;
            }

            bool m_finish = false;
            int m_key;
        };


        static int _lua_beforeShutdown(lua_State* L)
        {
            luaL_checktype(L, 1, LUA_TFUNCTION);
            int key = g_lua_callback_mgr->Store(L);
            LuaBeforeShutdown* cmd = new LuaBeforeShutdown(key);
            framework.BeforeShutdown(cmd);
            cmd->Release();
            return 0;
        }

        static int _lua_shutdown(lua_State* L)
        {
            framework.Stop();
            return 0;
        }

        static int _lua_open_framework(lua_State* L)
        {
            luaL_Reg libFuns[] = {
                {"beforeShutdown", _lua_beforeShutdown},
                {"shutdown", _lua_shutdown},
                {nullptr, nullptr}
            };

            luaL_newlib(L, libFuns);
            return 1;
        }

        void luaopen_framework(lua_State* L)
        {
            luaL_requiref(L, "framework", &_lua_open_framework, 0);
            lua_pop(L, 1);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
