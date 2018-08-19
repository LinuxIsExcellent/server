#include "nodemonitor.h"
#include "../logger.h"
#include "../framework.h"
#include "../memory/memorypool.h"
#include "connector.h"
#include "../event/dispatcher.h"
#include <list>
#include <functional>
#include <algorithm>

namespace base
{
    namespace cluster
    {
        using namespace std;

        bool is_process_name_match(const std::string& matcher, const std::string& process_name)
        {
            size_t m = 0, p = 0;
            bool star = false;
            while (m < matcher.size() && p < process_name.size()) {
                if (star) {
                    if (process_name[p] == '@') {
                        star = false;
                    } else {
                        ++p;
                    }
                } else {
                    if (matcher[m] == '*') {
                        star = true;
                        ++m;
                    } else {
                        if (matcher[m] == process_name[p]) {
                            ++m;
                            ++p;
                        } else {
                            return false;
                        }
                    }
                }
            }

            if (m < matcher.size()) {
                std::size_t rest = matcher.size() - m;
                if (rest == 2) {
                    if (matcher[m] == '@' && matcher[m + 1] == '*') {
                        return true;
                    }
                }
                return false;
            } else {
                if (p < process_name.size()) {
                    return matcher[matcher.length() - 1] == '*';
                } else {
                    return true;
                }
            }
        }

        /// NodeMonitor
        static NodeMonitor* s_node_monitor = nullptr;
        void NodeMonitor::CreateInstance()
        {
            if (s_node_monitor == nullptr) {
                s_node_monitor = new NodeMonitor();
            }
        }

        void NodeMonitor::DestroyInstance()
        {
            SAFE_DELETE(s_node_monitor);
        }

        NodeMonitor& NodeMonitor::instance()
        {
            return *s_node_monitor;
        }

        NodeMonitor::NodeMonitor()
            : mempool_(nullptr), listener_(nullptr), state_(NODE_MONITOR_STATE_IN_SETUP)
        {
            authed_connectors_.resize(2u, nullptr);
            authed_connectors_.reserve(5u);
            standalone_connectors_.reserve(5);
        }

        NodeMonitor::~NodeMonitor()
        {
            SAFE_RELEASE(listener_);
            SAFE_DELETE(mempool_);
            for (list<AsyncFetchNamedMailbox*>::iterator it = async_fetch_.begin(); it != async_fetch_.end(); ++it) {
                delete *it;
            }
            async_fetch_.clear();
        }

        void NodeMonitor::OnListenerAccept(net::Listener* sender, int clientfd)
        {
            Connector* conn = new Connector(*this, *mempool_, true, -1);
            conn->Connect(clientfd);
            anonymous_connectors_.push_back(conn);
        }

        void NodeMonitor::OnConnectorReceiveMessage(Connector* conn, MessageIn& msgin)
        {
            if (!msgin.to()) {
                NodeInternalCode code = static_cast<NodeInternalCode>(msgin.code());
                switch (code) {
                    case INTERNAL_REGISTER_NAMED_MAILBOX: {
                        string name;
                        MailboxID mbid;
                        msgin >> name >> mbid;
                        name.append("@");
                        name.append(conn->info().node_name);
                        if (local_node_info().type == NodeType::STANDALONE || conn->info().type == NodeType::STANDALONE) {
                            // rewrite node id
                            mbid = MailboxID(conn->info().node_id, mbid.pid());
                        }
                        OnNamedMailboxUp(name, mbid);
                    }
                    break;
                    case INTERNAL_UNREGISTER_NAMED_MAILBOX: {
                        string name;
                        msgin >> name;
                        name.append("@");
                        name.append(conn->info().node_name);
                        if (named_mailboxes_.erase(name) == 1) {
                            OnNamedMailboxDown(name);
                        }
                    }
                    break;
                    case INTERNAL_SYNC_NORMAL_NODE: {
                        string ip = msgin.ReadString();
                        int port = msgin.ReadInt32();
                        PingNode(ip, port);
                    }
                    break;
                    default:
                        LOG_ERROR("can not handle message without dest mailboxid!\n");
                        break;
                }
            } else {
                Mailbox* mb = FindMailboxByPID(msgin.to().pid());
                if (mb) {
                    try {
                        mb->HandleMessageReceive(msgin);
                    } catch (exception& ex) {
                        LOG_ERROR("catch exception when handle mailbox message, code=%u, reason=%s\n", msgin.code(), ex.what());
                    }
                } else {
                    cout << "can not find target mailbox to handle message, mailboxId=" << msgin.to() << endl;
                }
            }
        }

        void NodeMonitor::BeginSetup(const function< void(bool) >& cb)
        {
            SetupOption option;
            if (!option.ReadFromDefaultConfigFile()) {
                cb(false);
                return;
            }

            local_node_info_.listen_ip = option.listen_ip;
            local_node_info_.listen_port = option.listen_port;
            local_node_info_.node_name = option.node_name;
            local_node_info_.type = option.type;

            listener_ = new net::Listener(*this);
            if (!listener_->Bind(option.listen_ip.c_str(), option.listen_port)) {
                cb(false);
                return;
            }

            mempool_ = new memory::MemoryPool(256, 256);

            if (local_node_info_.type == NodeType::NORMAL) {
                // connect master node to fetch node id
                if (!option.master_ip.empty() && option.master_port != 0) {
                    PingNode(option.master_ip, option.master_port);
                } else {
                    LOG_ERROR("cluster: not found master node configure");
                    cb(false);
                    return;
                }
            } else if (local_node_info_.type == NodeType::MASTER) {
                local_node_info_.node_id = NodeIDGen::MASTER_NODE_ID;
            } else {
                local_node_info_.node_id = NodeIDGen::STANDALONE_NODE_ID;
            }
            cb_setup_ = cb;
            UpdateState();
        }

        void NodeMonitor::PingNode(const string& ip, int32_t port, int32_t retry_count)
        {
            auto filter = [&ip, port](Connector * c) {
                if (c == nullptr) {
                    return false;
                }
                return c->ipaddr() == ip && c->port() == port;
            };
            if (any_of(anonymous_connectors_.begin(), anonymous_connectors_.end(), filter)) {
                return;
            }
            if (any_of(authed_connectors_.begin(), authed_connectors_.end(), filter)) {
                return;
            }
            if (any_of(standalone_connectors_.begin(), standalone_connectors_.end(), filter)) {
                return;
            }
            Connector* conn = new Connector(*this, *mempool_, false, retry_count);
            anonymous_connectors_.push_back(conn);
            conn->Connect(ip.c_str(), port);
        }

        void NodeMonitor::BeginCleanup(std::function<void()> cb)
        {
            reconnect_timer_auto_observer_.SetNotExist();
            state_ = NODE_MONITOR_STATE_IN_CLEANUP;
            cb_cleanup_ = cb;
            CloseAll();
            UpdateState();
        }

        void NodeMonitor::CloseAll()
        {
            LOG_DEBUG("[node] close all");
            if (listener_) {
                listener_->Close();
            }
            for (list<Connector*>::iterator it = anonymous_connectors_.begin();
                    it != anonymous_connectors_.end();) {
                (*it)->Close();
                if (!(*it)->connect()) {
                    (*it)->Release();
                    it = anonymous_connectors_.erase(it);
                } else {
                    ++it;
                }
            }
            for (size_t i = 0u; i < standalone_connectors_.size(); ++i) {
                Connector* c = standalone_connectors_[i];
                if (c) {
                    c->Close();
                    if (!c->connect()) {
                        c->Release();
                        standalone_connectors_[i] = nullptr;
                    }
                }
            }
            for (size_t i = 0u; i < authed_connectors_.size(); ++i) {
                Connector* c = authed_connectors_[i];
                if (c) {
                    c->Close();
                    if (!c->connect()) {
                        c->Release();
                        authed_connectors_[i] = nullptr;
                    }
                }
            }
        }

        MailboxID NodeMonitor::GenerateMailboxID(bool sys)
        {
            MailboxID mbid(local_node_info().node_id, pid_gen_.Gen(sys));
            return mbid;
        }

        bool NodeMonitor::Register(Mailbox* mb, bool sys)
        {
            if (!mb->name().empty()) {
                if (named_mailboxes_.find(mb->name()) != named_mailboxes_.end()) {
                    LOG_WARN2("duplicate named mailbox %s\n", mb->name().c_str());
                    return false;
                }
            }
            mb->mbid_ = GenerateMailboxID(sys);
            mailboxes_.insert(make_pair(mb->mbid_.pid(), mb));
            if (!mb->name().empty()) {
                OnNamedMailboxUp(mb->name(), mb->mbid());
                BroadcastRegisterNamedMailbox(mb->name(), mb->mbid());
            }
            return true;
        }

        void NodeMonitor::UnRegister(Mailbox* mb)
        {
            if (!mb->name().empty()) {
                named_mailboxes_.erase(mb->name());
                OnNamedMailboxDown(mb->name());
                BroadcastUnRegisterNamedMailbox(mb->name());
            }
            mailboxes_.erase(mb->mbid_.pid());
            mb->mbid_.Clear();
        }

        void NodeMonitor::BroadcastRegisterNamedMailbox(const string& name, const MailboxID& mbid)
        {
            for (Connector * c : authed_connectors_) {
                if (c) {
                    SendRegisterNamedMailbox(c, name, mbid);
                }
            }
            for (Connector * c : standalone_connectors_) {
                if (c) {
                    SendRegisterNamedMailbox(c, name, mbid);
                }
            }
        }

        void NodeMonitor::SendRegisterNamedMailbox(Connector* conn, const string& name, const MailboxID& mbid)
        {
            MessageOut msgout((uint16_t)INTERNAL_REGISTER_NAMED_MAILBOX, 30, *mempool_);
            msgout << name << mbid;
            conn->SendMessage(msgout);
        }

        void NodeMonitor::BroadcastUnRegisterNamedMailbox(const string& name)
        {
            for (Connector * c : authed_connectors_) {
                if (c) {
                    MessageOut msgout((uint16_t)INTERNAL_UNREGISTER_NAMED_MAILBOX, 30, *mempool_);
                    msgout << name;
                    c->SendMessage(msgout);
                }
            }
            for (Connector * c : standalone_connectors_) {
                if (c) {
                    MessageOut msgout((uint16_t)INTERNAL_UNREGISTER_NAMED_MAILBOX, 30, *mempool_);
                    msgout << name;
                    c->SendMessage(msgout);
                }
            }
        }

        void NodeMonitor::SendSyncNoramlNode(Connector* c, const NodeInfo& info)
        {
            MessageOut msgout((uint16_t)INTERNAL_SYNC_NORMAL_NODE, 30, *mempool_);
            msgout.WriteString(info.listen_ip);
            msgout.WriteInt32(info.listen_port);
            c->SendMessage(msgout);
        }

        void NodeMonitor::SendInternalMessageAtNextCall(MailboxID to, message_data_t data)
        {
            Mailbox* mb = FindMailboxByPID(to.pid());
            if (mb) {
                MessageIn msgin(data);
                msgin.ReadHead();
                mb->HandleMessageReceive(msgin);
            }
        }

        void NodeMonitor::SendMessage(MessageOut& msgout)
        {
            const MailboxID& to = msgout.to();

            if (IsLocalMailbox(to)) {
                msgout.WriteHead();
                message_data_t data = msgout.FetchData();
                runner_.PushCommand(base::command::CreateCommandCallback([this, data, to]() {
                    SendInternalMessageAtNextCall(to, data);
                }));
            } else {
                Connector* conn = FindConnectorByNodeID(to.nodeid());
                if (conn) {
                    conn->SendMessage(msgout);
                }
            }
        }

        void NodeMonitor::OnNamedMailboxUp(const std::string& name, const MailboxID& mbid)
        {
            cout << "[node.named_mailbox]:" << name << "," << mbid << ", up.." << endl;
            named_mailboxes_.insert(make_pair(name, mbid));
            CheckAsyncFetchList(name, mbid);
            evt_named_mailbox_up.Trigger(name.c_str(), mbid);
        }

        void NodeMonitor::OnNamedMailboxDown(const std::string& name)
        {
            cout << "[node.named_mailbox]:" << name << ", down..." << endl;
            evt_named_mailbox_down.Trigger(name.c_str());
        }

        void NodeMonitor::OnNodeUp(Connector* conn)
        {
            const NodeInfo& info = conn->info();
            cout << "[node]:{" << (uint32_t)info.node_id << "," << info.node_name << "," << info.GetTypeString() << "}  up.." << endl;
            // 将所有本地命名邮箱同步给新来的节点
            for (named_mailbox_map_t::iterator it = named_mailboxes_.begin(); it != named_mailboxes_.end(); ++it) {
                if (IsLocalMailbox(it->second)) {
                    SendRegisterNamedMailbox(conn, it->first, it->second);
                }
            }
            // 如果是主节点, 将现在的其它节点告知新来的节点
            if (local_node_info_.type == NodeType::MASTER && conn->info().type == NodeType::NORMAL) {
                for (Connector * c : authed_connectors_) {
                    if (c && c->info().type == NodeType::NORMAL && c != conn) {
                        SendSyncNoramlNode(conn, c->info());
                    }
                }
            }

            evt_node_up.Trigger(info);
        }

        void NodeMonitor::OnNodeDown(const NodeInfo& info)
        {
            cout << "[node]:{" << (uint32_t)info.node_id << "," << info.node_name << "," << info.GetTypeString() << "}  down.." << endl;
            // 移除该节点对应的命名邮箱
            for (named_mailbox_map_t::iterator it = named_mailboxes_.begin(); it != named_mailboxes_.end();) {
                if (it->second.nodeid() == info.node_id) {
                    OnNamedMailboxDown(it->first);
                    it = named_mailboxes_.erase(it);
                } else {
                    ++it;
                }
            }
            evt_node_down.Trigger(info);

        }

        void NodeMonitor::FetchNamedMailbox(const string& matcher, async_fetch_named_mailbox_callback_t cb)
        {
            // check all
            for (named_mailbox_map_t::iterator it = named_mailboxes_.begin(); it != named_mailboxes_.end(); ++it) {
                if (is_process_name_match(matcher, it->first)) {
                    cb(it->second);
                    return;
                }
            }
            async_fetch_.push_back(new AsyncFetchNamedMailbox(matcher, cb));
        }

        AsyncFetchLinker* NodeMonitor::FetchNamedMailboxWithLinker(const string& matcher, async_fetch_named_mailbox_callback_t cb)
        {
            for (named_mailbox_map_t::iterator it = named_mailboxes_.begin(); it != named_mailboxes_.end(); ++it) {
                if (is_process_name_match(matcher, it->first)) {
                    cb(it->second);
                    return nullptr;
                }
            }
            AsyncFetchLinker* linker = new AsyncFetchLinker;
            async_fetch_.push_back(new AsyncFetchNamedMailbox(matcher, cb, linker));
            return linker;
        }

        void NodeMonitor::CheckAsyncFetchList(const string& name, const MailboxID& mbid)
        {
            for (list<AsyncFetchNamedMailbox*>::iterator it = async_fetch_.begin(); it != async_fetch_.end();) {
                if ((*it)->linker != nullptr && (*it)->linker->reference_count() != 2) {
                    delete *it;
                    it = async_fetch_.erase(it);
                    continue;
                }
                if (is_process_name_match((*it)->matcher, name)) {
                    (*it)->callback(mbid);
                    delete *it;
                    it = async_fetch_.erase(it);
                } else {
                    ++it;
                }
            }
        }

        void NodeMonitor::RegisterNodeConnector(Connector* c, const std::function<void()>& cb)
        {
            if (c->info().type == NodeType::STANDALONE || local_node_info_.type == NodeType::STANDALONE) {
                for (size_t i = 0u; i < standalone_connectors_.size(); ++i) {
                    if (standalone_connectors_[i] == nullptr) {
                        standalone_connectors_[i] = c;
                        c->SetNodeID(i + NodeIDGen::NORMAL_STANDALONE_DIVIDING);
                        break;
                    }
                }
                if (c->info().node_id == 0u) {
                    standalone_connectors_.push_back(c);
                    c->SetNodeID(standalone_connectors_.size() - 1u + NodeIDGen::NORMAL_STANDALONE_DIVIDING);
                }
            } else {
                for (size_t i = 2u; i < authed_connectors_.size(); ++i) {
                    if (authed_connectors_[i] == nullptr) {
                        authed_connectors_[i] = c;
                        c->SetNodeID(i);
                        break;
                    }
                }
                if (c->info().node_id == 0u) {
                    authed_connectors_.push_back(c);
                    c->SetNodeID(authed_connectors_.size() - 1u);
                }
            }
            cb();
            anonymous_connectors_.remove(c);
            OnNodeUp(c);
            UpdateState();
        }

        bool NodeMonitor::SetNodeConnector(Connector* c)
        {
            if (c->info().node_id == 0u || c->info().type == NodeType::STANDALONE) {
                LOG_WARN2("bad node info, type=%u, name=%s", (uint32_t)c->info().type, c->info().node_name.c_str());
                return false;
            }
            if (authed_connectors_.size() <= c->info().node_id) {
                authed_connectors_.resize(c->info().node_id + 1u, nullptr);
            }

            if (authed_connectors_[c->info().node_id] != nullptr) {
                LOG_ERROR("duplicate node id when try to connect, name=%s, from_ip=%s", c->info().node_name.c_str(), c->ipaddr().c_str());
                return false;
            }
            authed_connectors_[c->info().node_id] = c;
            anonymous_connectors_.remove(c);
            OnNodeUp(c);
            UpdateState();
            return true;
        }

        void NodeMonitor::RemoveNodeConnector(Connector* c)
        {
            if (c->info().node_id != 0) {
                uint16_t idx = c->info().node_id;
                if (c->info().type == NodeType::STANDALONE || local_node_info().type == NodeType::STANDALONE) {
                    idx -= NodeIDGen::NORMAL_STANDALONE_DIVIDING;
                    if (idx < standalone_connectors_.size() && standalone_connectors_[idx] == c) {
                        OnNodeDown(c->info());
                        standalone_connectors_[idx] = nullptr;
                    }
                } else {
                    if (idx < authed_connectors_.size() && authed_connectors_[idx] == c) {
                        OnNodeDown(c->info());
                        authed_connectors_[idx] = nullptr;
                    }
                }
            }
            anonymous_connectors_.remove(c);
            c->Release();
            UpdateState();

            if (state_ == NODE_MONITOR_STATE_OK && c->retry_count() >= 0) {
                // 自动重连
                string ip = c->info().listen_ip;
                int32_t port = c->info().listen_port;
                int32_t retry_count = c->retry_count() + 1;
                int64_t timeout = (10 + retry_count * 3);      //seconds
                if (timeout > 5 * 60) {
                    timeout = 5 * 60;
                }
                cout << "try to auto reconnect " << ip << ":" << port << " after " << timeout << " seconds" << endl;
                g_dispatcher->quicktimer().SetTimeout([this, ip, port, retry_count]() {
                    this->PingNode(ip, port, retry_count);
                }, timeout * 1000, reconnect_timer_auto_observer_);
            }
        }

        Connector* NodeMonitor::FindMasterNodeConnector()
        {
            for (Connector * c : authed_connectors_) {
                if (c && c->info().type == NodeType::MASTER) {
                    return c;
                }
            }
            return nullptr;
        }

        void NodeMonitor::UpdateState()
        {
            if (state_ == NODE_MONITOR_STATE_IN_SETUP) {
                if (local_node_info_.type != NodeType::NORMAL) {
                    state_ = NODE_MONITOR_STATE_OK;
                } else {
                    if (FindMasterNodeConnector()) {
                        state_ = NODE_MONITOR_STATE_OK;
                    }
                }
                if (state_ == NODE_MONITOR_STATE_OK) {
                    runner_.PushCommand(base::command::CreateCommandCallback([this]() {
                        cb_setup_(true);
                    }));
                }
            } else if (state_ == NODE_MONITOR_STATE_IN_CLEANUP) {
                bool ok = anonymous_connectors_.empty();
                if (ok) {
                    for (Connector * c : authed_connectors_) {
                        if (c != nullptr) {
                            ok = false;
                            break;
                        }
                    }
                }
                if (ok) {
                    for (Connector * c : standalone_connectors_) {
                        if (c != nullptr) {
                            ok = false;
                            break;
                        }
                    }
                }
                if (ok) {
                    state_ = NODE_MONITOR_STATE_STOP;
                    runner_.PushCommand(base::command::CreateCommandCallback([this]() {
                        cb_cleanup_();
                    }));
                }
            }
        }

        void NodeMonitor::DumpInfo()
        {
            cout << endl;
            cout << "-------------------------------- node info dump begin -----------------------------" << endl;

            cout << "  *local:" << local_node_info() << endl;

            for (auto it = anonymous_connectors_.begin(); it != anonymous_connectors_.end(); ++it) {
                cout << "anonymous" << (*it)->info() << endl;
            }

            for (size_t i = 0u; i < authed_connectors_.size(); ++i) {
                if (authed_connectors_[i] != nullptr) {
                    cout << "authed[" << i << "]" << authed_connectors_[i]->info() << endl;
                }
            }

            for (size_t i = 0u; i < standalone_connectors_.size(); ++i) {
                if (standalone_connectors_[i] != nullptr) {
                    cout << "standalone[" << i << "]" << standalone_connectors_[i]->info() << endl;
                }
            }

            cout << "-------------------------------- node info dump end-----------------------------" << endl;
            cout << endl;
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
