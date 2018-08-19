#include "framework.h"
#include "modulebase.h"
#include "action/actioninterval.h"
#include "action/executor.h"
#include "event/dispatcher.h"
#include "autoreleasepool.h"
#include "logger.h"
#include "cli/moduleconsole.h"
#include "dbo/moduledbo.h"
#include "cluster/modulecluster.h"
#include "lua/modulelua.h"
#include "lang.h"
#include "utils/utils_string.h"
#include "cron/modulecronjob.h"
#include "thread/modulethreadpool.h"
#include "http/modulehttpclient.h"
#include "memory/memorypoolmgr.h"
#include "trie/moduletrietree.h"
#include <unistd.h>
#include <csignal>
#include <cassert>
#include <iostream>
#include <clocale>
#include <libgen.h>

base::Framework framework;

namespace base
{
    using namespace std;

    extern uint32_t g_object_count;

    static void when_app_exit()
    {
        cout << "== check memory leak ==" << endl;
        cout << "g_object_count: " << g_object_count << endl;
    }

    static void signal_handler(int sig)
    {
        if (sig == SIGINT) {
            cout << endl;
            framework.Stop();
        }
    }

    ServerRule::ServerRule(const string& name, uint16_t id)
        : name_(name), id_(id)
    {
        full_name_ = name;
        if (id > 0u) {
            base::utils::string_append_format(full_name_, "-%u", id);
        }
    }

    void ServerRule::Parse(const char* data)
    {
        name_.clear();
        id_ = 0u;
        const char* p = data;
        while (*p) {
            if (*p != '-') {
                name_.push_back(*p);
            } else {
                ++p;
                break;
            }
            ++p;
        }
        if (*p) {
            id_ = atoi(p);
        }
        full_name_ = name_;
        if (id_ > 0u) {
            base::utils::string_append_format(full_name_, "-%u", id_);
        }
    }

    /// ActionBootstrap
    // 按依赖关系启动模块
    class ActionBootstrap : public action::ActionInterval
    {
    public:
        ActionBootstrap()
            : action::ActionInterval(100), cur_module_(nullptr), count_(0), bootstrap_order_(0) {}
        virtual ~ActionBootstrap() {}

    private:
        virtual void OnInterval(int64_t tick, int32_t span) {
            if (cur_module_ == nullptr) {
                for (module_map_t::iterator it = framework.modules_.begin(); it != framework.modules_.end(); ++it) {
                    if (it->second->module_state() == MODULE_STATE_NEW) {
                        bool all_depentds_ready = true;
                        const vector<string>& depends = it->second->GetDependentsModules();
                        for (vector<string>::const_iterator it2 = depends.begin(); it2 != depends.end(); ++it2) {
                            ModuleBase* mod = framework.GetModuleByName(*it2);
                            if (mod == nullptr || mod->module_state() != MODULE_STATE_RUNNING) {
                                all_depentds_ready = false;
                                break;
                            }
                        }
                        if (all_depentds_ready) {
                            LOG_DEBUG("[framework]: setup module <%s>\n", it->first.c_str());
                            cur_module_ = it->second;
                            it->second->bootstrap_order_ = bootstrap_order_;
                            ++bootstrap_order_;
                            it->second->OnModuleSetup();
                            break;
                        }
                    }
                }
                ++count_;
            }
        }

        virtual bool IsDone() {
            if (cur_module_ && cur_module_->module_state() == MODULE_STATE_RUNNING) {
                cur_module_ = nullptr;
            }
            bool is_done = true;
            bool has_error = false;
            for (module_map_t::iterator it = framework.modules_.begin(); it != framework.modules_.end(); ++it) {
                if (it->second->module_state() == MODULE_STATE_NEW || it->second->module_state() == MODULE_STATE_IN_SETUP) {
                    is_done = false;
                    break;
                }
                if (it->second->module_state() == MODULE_STATE_DELETE) {
                    is_done = true;
                    has_error = true;
                    LOG_ERROR("[framework]: module %s setup fail!\n", it->second->module_name().c_str());
                    break;
                }
            }
            if (count_ > 2000) {
                LOG_ERROR("bootstrap module cost too much time!\n");
                is_done = true;
            }
            if (is_done) {
                if (!has_error) {
                    LOG_DEBUG("[framework]: all modules setup completed, server is working now!");
                }
            }
            return is_done;
        }

        ModuleBase* cur_module_;
        int count_;
        uint32_t bootstrap_order_;
    };

    /// ActionShutdown
    class ActionShutdown : public action::ActionInterval
    {
    public:
        ActionShutdown()
            : action::ActionInterval(100) {
        }
        virtual ~ActionShutdown() {
            event::Dispatcher::instance().DebugDump();
        }

    private:
        ModuleBase* GetLatestBootstrapModule() {
            ModuleBase* latest_mod = nullptr;
            for (module_map_t::iterator it = framework.modules_.begin(); it != framework.modules_.end(); ++it) {
                if (latest_mod == nullptr) {
                    latest_mod = it->second;
                } else if (latest_mod->bootstrap_order_ < it->second->bootstrap_order_) {
                    latest_mod = it->second;
                }
            }
            return latest_mod;
        }

        virtual void OnInterval(int64_t tick, int32_t span) {
            // 反向关闭
            ModuleBase* latest_mod = GetLatestBootstrapModule();
            if (latest_mod && latest_mod->module_state() == MODULE_STATE_RUNNING) {
                LOG_DEBUG("[framework]: cleanup module <%s>\n", latest_mod->module_name().c_str());
                latest_mod->OnModuleCleanup();
            }
        }

        virtual bool IsDone() {
            for (module_map_t::iterator it = framework.modules_.begin(); it != framework.modules_.end();) {
                if (it->second->module_state() == MODULE_STATE_DELETE || it->second->module_state() == MODULE_STATE_NEW) {
                    it->second->Release();
                    it = framework.modules_.erase(it);
                } else {
                    ++it;
                }
            }
            return framework.modules_.empty();
        }
    };

    class FrameworkImpl
    {
    public:
        AutoObserver autoObserver;
    };

    static bool is_framework_exist = false;

    Framework::Framework()
        : exe_(nullptr), mempool_mgr_(nullptr)
    {
        assert(!is_framework_exist);
        is_framework_exist = true;

        ObjectTracker::Create();

        signal(SIGINT, signal_handler);
        signal(SIGPIPE, SIG_IGN);
    }

    Framework::~Framework()
    {
        is_framework_exist = false;
        SAFE_RELEASE(exe_);
        SAFE_DELETE(mempool_mgr_);
        SAFE_DELETE(m_impl);
        ObjectTracker::Destroy();
    }

    int64_t Framework::GetTickCache() const
    {
        return event::Dispatcher::instance().GetTickCache();
    }

    bool Framework::AutoSetup(const ServerRule& dev_rule, const string& dev_resource_dir)
    {
        // 使用默认规则
        // dev mode
        string res_dir = dev_resource_dir;
        ServerRule rule = dev_rule;
        {
#ifdef __APPLE__
            char* path = getcwd(NULL, 256);
#else
            char* path = get_current_dir_name();
#endif
            char* name = basename(path);
            // production mode
            if (strcmp(name, "src") != 0 && strcmp(name, "Debug") != 0 && strcmp(name, "bin") != 0) {
                res_dir = dirname(path) + string("/resource");
                rule.Parse(name);
            } 
            free(path);
        }
        return Setup(res_dir, rule);
    }

    bool Framework::Setup(const string& resource_dir, const ServerRule& server_rule)
    {
        LOG_WARN("****************************************\n");
        LOG_WARN("     application start running ...\n");
        LOG_WARN("****************************************\n");
        LOG_WARN("res_dir:  %s\n", resource_dir.c_str());
        LOG_WARN("server_rule: %s\n", server_rule.full_name().c_str());
        LOG_WARN("\n");

        if (m_impl == nullptr) {
            m_impl = new FrameworkImpl;
        }

        // init time cache
        GetTickCache();

        server_rule_ = server_rule;
        resource_dir_ = resource_dir;
        priv_dir_ = resource_dir_;
        base::utils::string_append_format(priv_dir_, "/%s", server_rule_.full_name().c_str());

        mempool_mgr_ = new memory::MemoryPoolMgr();

        // 设置环境
        setlocale(LC_ALL, "en_US.utf8");
        // 设定时区
        tzset();
        PoolManager::CreateInstance();
        atexit(when_app_exit);
        exe_ = new action::Executor;
        random_.Setup();

        // 加载多语言
        Lang::instance().Setup(resource_dir_);

        return true;
    }

    int Framework::Run()
    {
        // main loop
        event::Dispatcher::instance().Dispatch();

        Cleanup();
        return 0;
    }

    void Framework::Cleanup()
    {
        SAFE_RELEASE(exe_);
        SAFE_DELETE(mempool_mgr_);
        evt_before_shutdown.Clear();
        SAFE_DELETE(m_impl);
        PoolManager::DeleteInstance();
    }

    void Framework::BeforeShutdown(BeforeShutdownCmd* cmd)
    {
        m_beforeShutdownCmds.push_back(cmd);
        cmd->Retain();
    }

    void Framework::Stop()
    {
        g_dispatcher->quicktimer().SetInterval([this]() {
            if (m_beforeShutdownCmds.empty()) {
                if (exe_) {
                    evt_before_shutdown.Trigger();
                    ActionShutdown* shutdown = new ActionShutdown;
                    shutdown->AutoRelease();
                    exe_->RunAction(shutdown);
                    SAFE_RELEASE(exe_);
                    event::Dispatcher::instance().NormalExit();
                }
            } else {
                BeforeShutdownCmd* cmd = m_beforeShutdownCmds.back();
                if (cmd->IsFinish()) {
                    cmd->Release();
                    m_beforeShutdownCmds.pop_back();
                } else {
                    cmd->Execute();
                }
            }
        }, 500, m_impl->autoObserver);
    }

    void Framework::DumpAllModules()
    {
        cout << "module list:" << endl;
        for (module_map_t::iterator it = modules_.begin(); it != modules_.end(); ++it) {
            cout << it->second->module_name() << ", state:" << it->second->module_state() << endl;
        }
    }

    void Framework::RegisterModule(ModuleBase* mod)
    {
        mod->Retain();
        // check is exist module
        ModuleBase* exist = GetModuleByName(mod->module_name());
        if (exist) {
            LOG_ERROR("duplicate module: %s\n", mod->module_name().c_str());
            mod->Release();
            return;
        }
        modules_.insert(make_pair(mod->module_name(), mod));

        // 触发模块启动
        if (!exe_->GetActionByTag(1)) {
            ActionBootstrap* bootstrap = new ActionBootstrap;
            bootstrap->SetTag(1);
            bootstrap->AutoRelease();
            exe_->RunAction(bootstrap);
        }
    }

    void Framework::EnsureCoreModule(const char* core_mod_name)
    {
        if (GetModuleByName(core_mod_name) != nullptr) {
            return;
        }

        if (strcmp(core_mod_name, "cluster") == 0) {
            RegisterModule(cluster::ModuleCluster::Create());
        } else if (strcmp(core_mod_name, "console") == 0) {
            RegisterModule(cli::ModuleConsole::Create());
        } else if (strcmp(core_mod_name, "cronjob") == 0) {
            RegisterModule(cron::ModuleCronJob::Create());
        } else if (strcmp(core_mod_name, "dbo") == 0) {
            RegisterModule(dbo::ModuleDBO::Create());
        } else if (strcmp(core_mod_name, "threadpool") == 0) {
            RegisterModule(thread::ModuleThreadPool::Create());
        } else if (strcmp(core_mod_name, "http_client") == 0) {
            RegisterModule(http::ModuleHttpClient::Create());
        } else if (strcmp(core_mod_name, "lua") == 0) {
            RegisterModule(lua::ModuleLua::Create());
        } else if (strcmp(core_mod_name, "trie") == 0) {
            RegisterModule(trie::ModuleTrieTree::Create());
        } else {
            if (strchr(core_mod_name, '.') == NULL) {
                LOG_ERROR("not found core module = %s", core_mod_name);
                assert(false);
            }
        }
    }
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
