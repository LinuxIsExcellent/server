#ifndef BASE_CLUSTER_RPCSERVICE_H
#define BASE_CLUSTER_RPCSERVICE_H

#include "mailbox.h"
#include "configure.h"
#include "../observer.h"
#include <vector>

namespace base
{
    namespace cluster
    {
        class Mailbox;

        class RpcService : Mailbox::EventHandler
        {
        public:
            RpcService(const char* service_name, bool sys = false);
            virtual ~RpcService();

            const std::string& service_name() const {
                return service_name_;
            }

            bool Setup();

        protected:
            memory::MemoryPool& mempool() {
                return mailbox_->mempool();
            }
            Mailbox& self() {
                return *mailbox_;
            }

            void Reply(const MailboxID& to, uint16_t session, MessageOut& msgout);
            void Publish(const MailboxID& to, MessageOut& msgout);
            void PublishToAll(MessageOut& msgout);
            void PublishToAllExcept(MessageOut& msgout, const MailboxID& except);
            void PublishToAllExcept(MessageOut& msgout, const MailboxID* except) {
                if (except == nullptr) {
                    PublishToAll(msgout);
                } else {
                    PublishToAllExcept(msgout, *except);
                }
            }
            void Cast(const MailboxID& to, MessageOut& msgout);

            const MailboxID* FindStubByNodeID(uint16_t nodeid) const;

        private:
            virtual bool OnSetup() {
                return true;
            }
            virtual void OnCall(const MailboxID& from, uint16_t session, MessageIn& msgin) = 0;
            virtual void OnCast(const MailboxID& from, MessageIn& msgin) = 0;
            virtual void OnMessageReceive(MessageIn& msgin);

            virtual void OnStubConnect(const MailboxID& mbid) {}
            virtual void OnStubDisconnect(const MailboxID& mbid) {}

            void OnNodeDown(const NodeInfo& node);

            Mailbox* mailbox_;
            std::string service_name_;
            bool sys_;
            base::AutoObserver auto_observer_;

            std::vector<MailboxID> stubs_;
        };
    }
}

#define DEFINE_RPC_SERVICE(type) \
        public:\
            static type* instance() {\
                return sInstance;\
            }\
            static type* Create();\
            static void Destroy();\
        private:\
            static type* sInstance;\
            type();\
            virtual ~type();\
 
#define IMPLEMENT_RPC_SERVICE(type) \
        type* type::Create() \
        {\
            if (sInstance == nullptr) {\
                sInstance = new type;\
                if (!sInstance->Setup()) {\
                    delete sInstance;\
                    sInstance = nullptr;\
                }\
            }\
            return sInstance;\
        }\
        void type::Destroy()\
        {\
            if (sInstance != nullptr) {\
                delete sInstance;\
                sInstance = nullptr;\
            }\
        }\
        type* type::sInstance = nullptr;\
 
#endif // RPCSERVICE_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
