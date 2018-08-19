#include "rpcservice.h"
#include "message.h"
#include "nodemonitor.h"
#include <iostream>
#include <algorithm>
#include <functional>

#include "../logger.h"

namespace base
{
    namespace cluster
    {
        using namespace std;

        RpcService::RpcService(const char* service_name, bool sys)
            : mailbox_(nullptr), service_name_(service_name), sys_(sys)
        {
        }

        RpcService::~RpcService()
        {
            SAFE_DELETE(mailbox_);
        }

        bool RpcService::Setup()
        {
            NodeMonitor::instance().evt_node_down.Attach(std::bind(&RpcService::OnNodeDown, this, std::placeholders::_1), auto_observer_);
            mailbox_ = Mailbox::Create(*this, service_name_.c_str(), sys_);
            return mailbox_ != nullptr && OnSetup();
        }

        void RpcService::Reply(const MailboxID& to, uint16_t session, MessageOut& msgout)
        {
            msgout.SetSession(session);
            mailbox_->Cast(to, msgout);
        }

        void RpcService::Publish(const MailboxID& to, MessageOut& msgout)
        {
            msgout.SetSession(0u);
            mailbox_->Cast(to, msgout);
        }

        void RpcService::PublishToAll(MessageOut& msgout)
        {
            msgout.SetSession(0);
            for (const MailboxID & id : stubs_) {
                mailbox_->Cast(id, msgout);
            }
        }

        void RpcService::PublishToAllExcept(MessageOut& msgout, const MailboxID& except)
        {
            msgout.SetSession(0);
            for (const MailboxID & id : stubs_) {
                if (id != except) {
                    mailbox_->Cast(id, msgout);
                }
            }
        }

        void RpcService::Cast(const MailboxID& to, MessageOut& msgout)
        {
            msgout.SetSession(0);
            mailbox_->Cast(to, msgout);
        }

        const MailboxID* RpcService::FindStubByNodeID(uint16_t nodeid) const
        {
            for (const MailboxID & mbid : stubs_) {
                if (mbid.nodeid() == nodeid) {
                    return &mbid;
                }
            }
            return nullptr;
        }

        void RpcService::OnMessageReceive(MessageIn& msgin)
        {
            if (msgin.session() > 0u && msgin.session() < 10u) {
                const MailboxID& id = msgin.from();
                // 保留字段，内部通信
                if (msgin.session() == 1u) {
                    bool found = false;
                    for (auto it = stubs_.begin(); it != stubs_.end(); ++it) {
                        if (*it == id) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        stubs_.push_back(id);
                        OnStubConnect(id);
                    }
                } else if (msgin.session() == 2u) {
                    for (auto it = stubs_.begin(); it != stubs_.end();) {
                        if (*it == id) {
                            OnStubDisconnect(id);
                            it = stubs_.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
                return;
            }

            if (msgin.session() != 0) {
                OnCall(msgin.from(), msgin.session(), msgin);
            } else {
                OnCast(msgin.from(), msgin);
            }
        }

        void RpcService::OnNodeDown(const NodeInfo& node)
        {
            for (auto it = stubs_.begin(); it != stubs_.end();) {
                if ((*it).nodeid() == node.node_id) {
                    OnStubDisconnect(*it);
                    it = stubs_.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
