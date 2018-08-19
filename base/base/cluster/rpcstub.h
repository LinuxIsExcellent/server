#ifndef BASE_CLUSTER_RPCSTUB_H
#define BASE_CLUSTER_RPCSTUB_H

#include "mailbox.h"
#include "../object.h"
#include "../observer.h"
#include <functional>
#include <unordered_map>

namespace base
{
    namespace cluster
    {
        typedef std::function<void(MessageIn&)> rpc_callback_t;
        typedef std::function<void()> rpc_error_callback_t;

        class RpcCallObject;
        class RpcCallLinker : public Object
        {
            virtual const char* GetObjectName() {
                return "base::cluster::RpcCallLinker";
            }
        };

        class RpcStub : public Mailbox::EventHandler
        {
        public:
            RpcStub(const char* service_name, bool auto_reconnect = false);
            ~RpcStub();

            bool is_service_avaiable() const {
                return is_service_avaiable_;
            }

            memory::MemoryPool& mempool() {
                return mailbox_->mempool();
            }

            void BeginSetup(std::function<void(bool)> cb);

            // TODO 重构 rpc_callback_t 使其具备从参数中返回错误信息的功能（比如超时)
            void Call(MessageOut& msgout, const rpc_callback_t& cb, const AutoObserver& atob);
            void Call(MessageOut& msgout, const rpc_callback_t& cb, const rpc_error_callback_t& error_cb, const AutoObserver& atob);
            void Call(MessageOut& msgout, const rpc_callback_t& cb, const rpc_error_callback_t& error_cb, Observer* observer);
            void Cast(MessageOut& msgout);

            void UnifyWithServiceMailbox(MailboxID& mbid) {
                mbid.Unify(mbid_service_);
            }

        private:
            // 启动时
            virtual bool OnSetup() {
                return true;
            }
            // 与服务建立连接
            virtual void OnConnectToService() {}
            // 与服务断开连接
            virtual void OnDisconnectFromService() {}
            // 接收到订阅消息
            virtual void OnSubscribeArrive(MessageIn& msgin) {}

        private:
            virtual void OnMessageReceive(MessageIn& msgin) final;
            void OnServiceUp(const MailboxID& mbid) ;
            void OnServiceDown();
            uint16_t GenSessionID() {
                uint16_t ret = session_cur_++;
                if (ret < 10u) {
                    session_cur_ = 10u;
                    ret = session_cur_++;
                }
                return ret;
            }
            void SendRegisterStub();
            void SendUnRegisterStub();

            Mailbox* mailbox_;
            uint16_t session_cur_;
            MailboxID mbid_service_;
            std::string service_name_;
            base::AutoObserver auto_observer_;
            bool auto_reconnect_;

            bool is_service_avaiable_ = false;
            std::unordered_map<uint16_t, RpcCallObject*> calls_;
            std::function<void(bool)> cb_setup_;
        };
    }
}

#define DEFINE_RPC_STUB(type) \
        public:\
            static type* instance() { return sInstance; }\
            static type* Create(std::function<void(bool)> setupCallback);\
            static void Destroy();\
        private:\
            type();\
            virtual ~type();\
            static type* sInstance;\
 
#define IMPLEMENT_RPC_STUB(type) \
        type* type::sInstance = nullptr;\
        type* type::Create(std::function<void(bool)> setupCallback)\
        {\
            if (sInstance == nullptr) {\
                sInstance = new type();\
                sInstance->BeginSetup(setupCallback);\
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
 
#endif // RPCSTUB_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
