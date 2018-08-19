#include "rpcstub.h"
#include "message.h"
#include "nodemonitor.h"
#include <functional>

namespace base
{
    namespace cluster
    {
        using namespace std;
        using namespace base;

        class RpcCallObject
        {
        public:
            rpc_callback_t fun;
            rpc_error_callback_t error_fun;
            Observer* observer = nullptr;
            RpcCallObject(const rpc_callback_t& cb, const AutoObserver& atob) : fun(cb) {
                observer = atob.GetObserver();
                observer->Retain();
            }
            RpcCallObject(const rpc_callback_t& cb, const rpc_error_callback_t& error_cb, const AutoObserver& atob) : fun(cb), error_fun(error_cb) {
                observer = atob.GetObserver();
                observer->Retain();
            }
            RpcCallObject(const rpc_callback_t& cb, const rpc_error_callback_t& error_cb, Observer* ob) : fun(cb), error_fun(error_cb) {
                observer = ob;
                observer->Retain();
            }
            ~RpcCallObject() {
                SAFE_RELEASE(observer);
            }
        };

        RpcStub::RpcStub(const char* service_name, bool auto_reconnect) : mailbox_(nullptr),
            session_cur_(10u), service_name_(service_name), auto_reconnect_(auto_reconnect)
        {
        }

        RpcStub::~RpcStub()
        {
            if (mbid_service_) {
                SendUnRegisterStub();
            }
            SAFE_DELETE(mailbox_);
            for (auto it = calls_.begin(); it != calls_.end(); ++it) {
                delete it->second;
            }
            calls_.clear();
        }

        void RpcStub::BeginSetup(std::function<void(bool)> cb)
        {
            mailbox_ = Mailbox::Create(*this);
            if (mailbox_ == nullptr) {
                cb(false);
                return;
            }
            cb_setup_ = cb;
            if (!OnSetup()) {
                cb(false);
                return;
            }
            NodeMonitor::instance().FetchNamedMailbox(service_name_, std::bind(&RpcStub::OnServiceUp, this, std::placeholders::_1));
            NodeMonitor::instance().evt_named_mailbox_down.Attach([this](const char * name) {
                if (service_name_ == name) {
                    this->OnServiceDown();
                }
            }, auto_observer_);
        }

        void RpcStub::OnServiceUp(const MailboxID& mbid)
        {
            is_service_avaiable_ = true;
            mbid_service_ = mbid;
            SendRegisterStub();
            if (cb_setup_) {
                cb_setup_(true);
                cb_setup_ = NULL;
            }
            OnConnectToService();
        }

        void RpcStub::OnServiceDown()
        {
            is_service_avaiable_ = false;
            if (auto_reconnect_) {
                OnDisconnectFromService();
                NodeMonitor::instance().FetchNamedMailbox(service_name_, std::bind(&RpcStub::OnServiceUp, this, std::placeholders::_1));
            } else {
                OnDisconnectFromService();
            }
        }

        void RpcStub::Call(MessageOut& msgout, const rpc_callback_t& cb, const AutoObserver& atob)
        {
            msgout.SetSession(GenSessionID());
            RpcCallObject* call = new RpcCallObject(cb, atob);
            calls_.insert(make_pair(msgout.session(), call));
            mailbox_->Cast(mbid_service_, msgout);
            // TODO 超时检测
        }

        void RpcStub::Call(MessageOut& msgout, const rpc_callback_t& cb, const rpc_error_callback_t& error_cb, const AutoObserver& atob)
        {
            Call(msgout, cb, error_cb, atob.GetObserver());
        }

        void RpcStub::Call(MessageOut& msgout, const rpc_callback_t& cb, const rpc_error_callback_t& error_cb, Observer* observer)
        {
            if (!is_service_avaiable_) {
                if (observer->IsExist()) {
                    error_cb();
                }
                return;
            }
            msgout.SetSession(GenSessionID());
            RpcCallObject* call = new RpcCallObject(cb, error_cb, observer);
            calls_.insert(make_pair(msgout.session(), call));
            mailbox_->Cast(mbid_service_, msgout);
        }

        void RpcStub::Cast(MessageOut& msgout)
        {
            msgout.SetSession(0u);
            mailbox_->Cast(mbid_service_, msgout);
        }

        void RpcStub::OnMessageReceive(MessageIn& msgin)
        {
            if (msgin.session() == 0u) {
                OnSubscribeArrive(msgin);
            } else {
                auto it = calls_.find(msgin.session());
                if (it != calls_.end()) {
                    RpcCallObject* co = it->second;
                    if (co->observer->IsExist()) {
                        co->fun(msgin);
                    }
                    delete co;
                    calls_.erase(it);
                }
            }
        }

        void RpcStub::SendRegisterStub()
        {
            MessageOut msgout(0u, 40, mempool());
            msgout.SetSession(1);
            mailbox_->Cast(mbid_service_, msgout);
        }

        void RpcStub::SendUnRegisterStub()
        {
            MessageOut msgout(0u, 40, mempool());
            msgout.SetSession(2);
            mailbox_->Cast(mbid_service_, msgout);
        }
    }
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
