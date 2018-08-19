#include "modulelua.h"
#include "../framework.h"
#include "../utils/file.h"
#include "../utils/utils_string.h"
#include "../logger.h"
#include "../action/executor.h"
#include "luavmpool.h"
#include "luacallbackmgr.h"
#include <lua.hpp>
#include "../configurehelper.h"
#include "../memory/memorypoolmgr.h"
#include "../event/dispatcher.h"
#include <iostream>
#include <functional>

base::lua::ModuleLua* g_module_lua = nullptr;

namespace base
{
    namespace lua
    {
        using namespace std;

        const int64_t ModuleLua::CHECK_INTERVAL_TICK = 10 * 1000;          // 10s

        ModuleLua::ModuleLua()
            : ModuleBase("lua")
        {
            assert(g_module_lua == nullptr);
            g_module_lua = this;
            AddDependentModule("dbo");
            AddDependentModule("cluster");
            AddDependentModule("http_client");
            AddDependentModule("trie");
        }

        ModuleLua::~ModuleLua()
        {
            LuaCallbackMgr::Delete();
            for (auto it = m_pools.begin(); it != m_pools.end(); ++it) {
                if (*it) {
                    delete *it;
                }
            }
            m_pools.clear();
            g_module_lua = nullptr;
        }

        ModuleLua* ModuleLua::Create()
        {
            ModuleLua* obj = new ModuleLua;
            obj->AutoRelease();
            return obj;
        }

        LuaVmPool* ModuleLua::GetVmPool(int poolId)
        {
            for (LuaVmPool * p : m_pools) {
                if (p->id() == poolId) {
                    return p;
                }
            }
            return nullptr;
        }

        LuaVm* ModuleLua::AquireVm(int poolId)
        {
            LuaVmPool* pool = GetVmPool(poolId);
            if (pool == nullptr) {
                return nullptr;
            }
            return pool->AquireVm();
        }

        void ModuleLua::OnModuleSetup()
        {
            ConfigureHelper helper;
            tinyxml2::XMLElement* node = helper.fetchConfigNodeWithServerRule();
            tinyxml2::XMLElement* xmlVmPool = helper.FirstChildElementWithPath(node, "lua/vmPool");
            if (xmlVmPool == nullptr) {
                LOG_ERROR("not found lua.vmPool node");
                SetModuleState(MODULE_STATE_DELETE);
                return;
            }
            LuaCallbackMgr::Create();
            while (xmlVmPool) {
                size_t id = xmlVmPool->UnsignedAttribute("id");
                int min = xmlVmPool->IntAttribute("min");
                int max = xmlVmPool->IntAttribute("max");
                const char* bootstrap = xmlVmPool->Attribute("bootstrap");

                if (bootstrap == nullptr) {
                    LOG_ERROR("common.conf: require lua/vmPool.bootstrap attribute");
                    break;
                }

                if (min < 1) {
                    min = 1;
                }

                if (max < min) {
                    max = min;
                }

                if (id >= m_pools.size()) {
                    m_pools.resize(id + 1u, nullptr);
                }
                if (m_pools[id] == nullptr) {
                    LuaVmPool* pool = new LuaVmPool(id, min, max);
                    string path = framework.resource_dir();
                    base::utils::string_path_append(path, bootstrap);
                    pool->GenerateAllVm(path);
                    m_pools[id] = pool;
                } else {
                    LOG_ERROR("common.conf: duplicate id in lua/vmPool configure file");
                    break;
                }

                xmlVmPool = xmlVmPool->NextSiblingElement("vmPool");
            }

            linker_ = g_dispatcher->quicktimer().SetIntervalWithLinker(std::bind(&ModuleLua::IntervalCheck, this), CHECK_INTERVAL_TICK);
            linker_->Retain();

            SetModuleState(MODULE_STATE_RUNNING);
        }

        void ModuleLua::OnModuleCleanup()
        {
            SAFE_RELEASE(linker_);

            LuaCallbackMgr::Delete();
            for (auto it = m_pools.begin(); it != m_pools.end(); ++it) {
                if (*it) {
                    delete *it;
                }
            }
            m_pools.clear();
            SetModuleState(MODULE_STATE_DELETE);
        }

        void ModuleLua::IntervalCheck()
        {
            for (auto it = m_pools.begin(); it != m_pools.end(); ++it) {
                if (*it) {
                    (*it)->IntervalCheck();
                }
            }
        }

    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
