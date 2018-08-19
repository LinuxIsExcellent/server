#ifndef BASE_CLUSTER_MAILBOX_H
#define BASE_CLUSTER_MAILBOX_H

#include "mailboxid.h"
#include "../observer.h"
#include <string>
#include <functional>

namespace base
{
    namespace gateway
    {

        class PacketIn;
        class PacketOut;
    }

    namespace memory
    {
        class MemoryPool;
    }

    namespace cluster
    {
        class MessageOut;
        class MessageIn;
        class NodeMonitor;

        class Mailbox
        {
        public:
            struct EventHandler {
                virtual ~EventHandler() {}
                virtual void OnMessageReceive(MessageIn& msgin) = 0;
            };
            ~Mailbox();

            // 名称
            const std::string& name() const {
                return name_;
            }

            memory::MemoryPool& mempool() {
                return mempool_;
            }

            // 邮箱ID
            const MailboxID& mbid() const {
                return mbid_;
            }

            base::Observer* GetObserver() const {
                return m_autoObserver.GetObserver();
            }

            // 创建一个邮箱
            static Mailbox* Create(EventHandler& handler, const char* name = "", bool sys = false);

            // 给其它邮箱发送消息
            void Cast(const MailboxID& to, MessageOut& msgout);

            void SetForwardHandler(std::function<void(base::gateway::PacketIn&)> handler) {
                m_forwardHandler = handler;
            }

            void ClearForwardHandler() {
                m_forwardHandler = nullptr;
            }

            void Forward(const MailboxID& to, base::gateway::PacketIn& pktin);

            void SetBackwardHandler(std::function<void(base::gateway::PacketOut&)> handler) {
                m_backwardHandler = handler;
            }
            void ClearBackwardHandler() {
                m_backwardHandler = nullptr;
            }

            void Backward(const MailboxID& to, base::gateway::PacketOut& pktout);

        private:
            Mailbox(EventHandler& handler, const char* name);

            void HandleMessageReceive(MessageIn& msgin);

            memory::MemoryPool& mempool_;
            EventHandler& handler_;
            std::function<void(base::gateway::PacketIn&)> m_forwardHandler;
            std::function<void(base::gateway::PacketOut&)> m_backwardHandler;
            MailboxID mbid_;
            std::string name_;
            base::AutoObserver m_autoObserver;
            friend class NodeMonitor;
            DISABLE_COPY(Mailbox);
        };

#define MSGOUT(code, approx_len) base::cluster::MessageOut msgout((uint16_t)code, approx_len, mempool())
    }
}

#endif // MAILBOX_H

// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
