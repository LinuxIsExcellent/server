#ifndef BASE_CLUSTER_NODEMONITOR_H
#define BASE_CLUSTER_NODEMONITOR_H

#include "../global.h"
#include "../net/listener.h"
#include "../net/client.h"
#include "message.h"
#include "mailbox.h"
#include "configure.h"
#include "../event.h"
#include "../command/runner.h"
#include <string>
#include <map>
#include <list>
#include <functional>
#include <unordered_map>
#include "../observer.h"

namespace base
{
    namespace cluster
    {
        using namespace std;

        class Connector;

        // check if process name match
        // style 1: calculator          => [calculator] | []
        // style 2: calculator@*        => [calculator@a, calculator@b, calculator] | []
        // style 3: calculator@a        => [calculator@a] | []
        // style 4: calculator.*@a      => [calculator.1@a, calculator.2@b] | []
        bool is_process_name_match(const std::string& matcher, const std::string& process_name);

        typedef base::Event<void(const std::string& name, const MailboxID& pid)> NamedMailboxUpEvent;
        typedef base::Event<void(const std::string& name, const MailboxID& pid)> NamedMailboxDownEvent;

        typedef base::Event<void(uint16_t nodeid, const std::string& name)> NodeUpEvent;
        typedef base::Event<void(uint16_t nodeid, const std::string& name)> NodeDownEvent;

        typedef std::function<void(const MailboxID&)> async_fetch_named_mailbox_callback_t;
        typedef std::unordered_map<std::string, MailboxID> named_mailbox_map_t;

        class AsyncFetchLinker : public Object
        {
            virtual const char* GetObjectName() {
                return "base::cluster::AsyncFetchLinker";
            }
        };

        class NodeMonitor : public net::Listener::EventHandler
        {
        public:
            static void CreateInstance();
            static void DestroyInstance();
            static NodeMonitor& instance();

            virtual ~NodeMonitor();

            Event<void(const char*, const MailboxID&)> evt_named_mailbox_up;
            Event<void(const char*)> evt_named_mailbox_down;
            Event<void(const NodeInfo&)> evt_node_up;
            Event<void(const NodeInfo&)> evt_node_down;

            memory::MemoryPool& mempool() {
                return *mempool_;
            }
            // 本地节点信息
            const NodeInfo& local_node_info() const {
                return local_node_info_;
            }

            // 判断mbid是否为本地邮箱
            bool inline IsLocalMailbox(const MailboxID& mbid) const {
                return local_node_info().node_id == mbid.nodeid();
            }

            void DumpInfo();

            // 开始启动
            void BeginSetup(const std::function<void(bool)>& cb);
            // 开始停止
            void BeginCleanup(std::function<void()> cb);

            void PingNode(const std::string& ip, int32_t port, bool enableAutoReconnect = false) {
                PingNode(ip, port, enableAutoReconnect ? 0 : -1);
            }

            // 注册邮箱, 返回是否成功
            bool Register(Mailbox* mb, bool sys);
            // 取消注册邮箱
            void UnRegister(Mailbox* mb);
            // 发送消息
            void SendMessage(MessageOut& msgout);

            // 获取命名邮箱
            // matcher是匹配模式, 有如下四种方式
            // style 1: calculator          => [calculator] | []
            // style 2: calculator@*        => [calculator@a, calculator@b, calculator] | []
            // style 3: calculator@a        => [calculator@a] | []
            // style 4: calculator.*@a      => [calculator.1@a, calculator.2@b] | []
            void FetchNamedMailbox(const std::string& matcher, async_fetch_named_mailbox_callback_t cb);
            AsyncFetchLinker* FetchNamedMailboxWithLinker(const std::string& matcher, async_fetch_named_mailbox_callback_t cb);

        private:
            NodeMonitor();
            virtual void OnListenerAccept(net::Listener* sender, int clientfd);
            virtual void OnConnectorReceiveMessage(Connector* conn, MessageIn& msgin);

        private:
            void CloseAll();
            void PingNode(const std::string& ip, int32_t port, int32_t retry_count);
            void SendInternalMessageAtNextCall(MailboxID to, message_data_t);

        private:
            void OnNodeUp(Connector* conn);
            void OnNodeDown(const NodeInfo& info);

            void BroadcastRegisterNamedMailbox(const std::string& name, const MailboxID& mbid);
            void SendRegisterNamedMailbox(Connector* conn, const std::string& name, const MailboxID& mbid);
            void BroadcastUnRegisterNamedMailbox(const std::string& name);
            void SendSyncNoramlNode(Connector* c, const NodeInfo& info);

            void OnNamedMailboxUp(const std::string& name, const MailboxID& mbid);
            void OnNamedMailboxDown(const std::string& name);

        private:
            // 获取pid获取mailbox
            Mailbox* FindMailboxByPID(uint32_t pid) {
                std::map<uint32_t, Mailbox*>::iterator it = mailboxes_.find(pid);
                return it == mailboxes_.end() ? nullptr : it->second;
            }

            Connector* FindConnectorByNodeID(uint16_t node_id) {
                if (node_id >= NodeIDGen::NORMAL_STANDALONE_DIVIDING) {
                    node_id -= NodeIDGen::NORMAL_STANDALONE_DIVIDING;
                    if (node_id < standalone_connectors_.size()) {
                        return standalone_connectors_[node_id];
                    }
                } else {
                    if (node_id < authed_connectors_.size()) {
                        return authed_connectors_[node_id];
                    }
                }
                return nullptr;
            };

            Connector* FindMasterNodeConnector();

            void UpdateState();

            void RegisterNodeConnector(Connector* c, const std::function<void()>& cb_got_node_id);
            bool SetNodeConnector(Connector* c);
            void RemoveNodeConnector(Connector* c);

            void SetNodeID(uint32_t id) {
                local_node_info_.node_id = id;
            }

        private:
            std::function<void(bool)> cb_setup_;
            std::function<void()> cb_cleanup_;
            memory::MemoryPool* mempool_;
            net::Listener* listener_;                           // 侦听器
            std::list<Connector*> anonymous_connectors_;        // 匿名连接
            std::vector<Connector*> authed_connectors_;
            std::vector<Connector*> standalone_connectors_;
            std::map<uint32_t, Mailbox*> mailboxes_;             // 所有已注册的邮箱
            named_mailbox_map_t named_mailboxes_;               // 命名邮箱
            enum NodeMonitorState {
                NODE_MONITOR_STATE_IN_SETUP,
                NODE_MONITOR_STATE_OK,
                NODE_MONITOR_STATE_IN_CLEANUP,
                NODE_MONITOR_STATE_STOP,
            };
            NodeMonitorState state_;
            MailboxID GenerateMailboxID(bool sys);
            MailboxPIDGen pid_gen_;
            NodeInfo local_node_info_;
            base::command::Runner runner_;

        private:
            struct AsyncFetchNamedMailbox {
                DISABLE_COPY(AsyncFetchNamedMailbox);
                std::string matcher;
                async_fetch_named_mailbox_callback_t callback;
                AsyncFetchLinker* linker;
                AsyncFetchNamedMailbox(const std::string& m, const async_fetch_named_mailbox_callback_t& c, AsyncFetchLinker* l = nullptr)
                    : matcher(m), callback(c), linker(l) {}
                ~AsyncFetchNamedMailbox() {
                    SAFE_RELEASE(linker);
                }
            };
            std::list<AsyncFetchNamedMailbox*> async_fetch_;
            // 检查是否有适合的
            void CheckAsyncFetchList(const std::string& name, const MailboxID& mbid);
            friend class Connector;

        private:
            base::AutoObserver reconnect_timer_auto_observer_;
        };
    }
}

#endif // BASE_CLUSTER_NODEMONITOR_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
