#include "luavm.h"
#include "../framework.h"
#include "../utils/file.h"
#include <lua/lua.hpp>
#include "lua_module_net.h"
#include "luaex.h"
#include "lua_module_dbo.h"
#include "lua_module_http.h"
#include "lua_module_timer.h"
#include "lua_module_utils.h"
#include "lua_module_trie.h"
#include "lua_module_vm.h"
#include "lua_module_cluster.h"
#include "lua_module_framework.h"
#include "../logger.h"
#include "../event/dispatcher.h"

namespace base
{
    namespace lua
    {
        using namespace std;

        int LuaVm::_lua_bootstrap_callback(lua_State* L)
        {
            bool ok = lua_toboolean(L, 1);
            cout << "bootstrap finish, result=" << boolalpha << ok << endl;
            LuaVm* vm = (LuaVm*)lua_touserdata(L, lua_upvalueindex(1));
            vm->m_state = ok ? LuaVmState::OK : LuaVmState::ERROR;
            return 0;
        }

        LuaVm::LuaVm()
        {
            m_L = luaL_newstate();
            m_state = LuaVmState::INIT;
            m_createTime = g_dispatcher->GetTimestampCache();
        }

        LuaVm::~LuaVm()
        {
            lua_close(m_L);
            m_L = nullptr;
        }

        void LuaVm::Bootstrap(const string& bootstrapScript)
        {
            m_bootstrapScript = bootstrapScript;

            // lua基础库
            luaL_openlibs(m_L);
            // 框架基础类库
            luaopen_utils(m_L);
            luaopen_trie(m_L);
            luaopen_vm(m_L);
            luaopen_timer(m_L);
            luaopen_net(m_L);
            luaopen_dbo(m_L);
            luaopen_http(m_L);
            luaopen_framework(m_L);
            luaopen_cluster(m_L);

            // 执行初始化脚本
            if (base::utils::file_can_read(bootstrapScript.c_str())) {
                int err = luaL_loadfile(m_L, bootstrapScript.c_str());
                if (err == LUA_OK) {
                    lua_pushlightuserdata(m_L, this);
                    lua_pushcclosure(m_L, &_lua_bootstrap_callback, 1);
                    err = luaEx_pcall(m_L, 1, 0, 0);
                } else {
                    LOG_ERROR("[%s] load fail: %s\n", bootstrapScript.c_str(), lua_tostring(m_L, -1));
                }
            } else {
                LOG_WARN("[%s] can not read\n", bootstrapScript.c_str());
            }
        }

        void LuaVm::ExecuteCachedScript(const string& script, int argBeginIdx)
        {
            lua_vm_execute_cached_script(m_L, script.c_str(), argBeginIdx);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
