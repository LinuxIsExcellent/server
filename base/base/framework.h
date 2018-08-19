#ifndef BASE_FRAMEWORK_H
#define BASE_FRAMEWORK_H

#include "global.h"
#include "utils/random.h"
#include "event.h"
#include <functional>
#include <unordered_map>
#include <vector>

namespace base
{
    namespace action
    {
        class Executor;
    }

    namespace memory
    {
        class MemoryPoolMgr;
    }

    class ModuleBase;

    typedef std::unordered_map<std::string, ModuleBase*> module_map_t;

    class ServerRule
    {
    public:
        ServerRule() : name_(""), id_(0), full_name_("") {}
        ServerRule(const std::string& name, uint16_t id = 0u);

        uint16_t id() const {
            return id_;
        }
        const std::string& name() const {
            return name_;
        }
        const std::string& full_name() const {
            return full_name_;
        }

        void Parse(const char* data);

    private:
        std::string name_;
        uint16_t id_;
        std::string full_name_;
    };

    typedef Event<void()> event_framework_before_shutdown_t;

    class BeforeShutdownCmd : public Object
    {
    public:
        void Execute() {
            if (!m_executed) {
                m_executed = true;
                OnExecute();
            }
        }
        virtual void OnExecute() = 0;
        virtual bool IsFinish() = 0;

        virtual const char* GetObjectName() {
            return "base::BeforeShutdownCmd";
        }

    private:
        bool m_executed = false;
    };

    class FrameworkImpl;
    class Framework
    {
    public:
        Framework();
        ~Framework();

        event_framework_before_shutdown_t evt_before_shutdown;

        // 随机数生成器
        utils::Random& random() {
            return random_;
        }
        // 资源目录
        const std::string& resource_dir() const {
            return resource_dir_;
        }
        // 私有目录
        const std::string& priv_dir() const {
            return priv_dir_;
        }
        // 服务器角色
        const ServerRule& server_rule() const {
            return server_rule_;
        }

        // 获取当前时间(高效)
        int64_t GetTickCache() const;
        // 注册模块
        void RegisterModule(ModuleBase* mod);
        // 启动
        bool AutoSetup(const ServerRule& server_rule, const std::string& dev_resource_dir);
        bool Setup(const std::string& resource_dir, const ServerRule& server_rule);
        // 运行
        int Run();
        // 停止
        void Stop();
        // 输出所有模块
        void DumpAllModules();
        // 根据模块名获取模块
        ModuleBase* GetModuleByName(const std::string& mod_name) const {
            module_map_t::const_iterator it = modules_.find(mod_name);
            return it == modules_.end() ? nullptr : it->second;
        }
        std::vector<const ModuleBase*> GetAllModules() const {
            std::vector<const ModuleBase*> ret;
            for (module_map_t::const_iterator it = modules_.begin(); it != modules_.end(); ++it) {
                ret.push_back(it->second);
            }
            return ret;
        }
        void EnsureCoreModule(const char* core_mod_name);

        void BeforeShutdown(BeforeShutdownCmd* cmd);

    private:
        void Cleanup();
        module_map_t modules_;        // 所有模块
        action::Executor* exe_;
        memory::MemoryPoolMgr* mempool_mgr_;
        utils::Random random_;
        std::string resource_dir_;
        std::string priv_dir_;
        ServerRule server_rule_;
        std::vector<BeforeShutdownCmd*> m_beforeShutdownCmds;
        FrameworkImpl* m_impl = nullptr;

        friend class ActionBootstrap;
        friend class ActionShutdown;
        friend class FrameworkImpl;
    };
}

extern base::Framework framework;

#endif // FRAMEWORK_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
