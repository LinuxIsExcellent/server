#include "connector.h"
#include "../logger.h"

namespace base
{
    namespace cluster
    {
        using namespace std;

        /// Connector
        Connector::Connector(NodeMonitor& node_monitor, memory::MemoryPool& mempool, bool passive, int32_t retry_count)
            : Client(mempool), node_monitor_(node_monitor), passive_(passive), retry_count_(retry_count)
        {
        }

        Connector::~Connector()
        {
        }

        void Connector::SendMessage(MessageOut& msgout)
        {
            msgout.WriteHead();
            message_data_t data = msgout.FetchData();
            PushSend(data);
        }

        void Connector::HandleInternalMessage(MessageIn& msgin)
        {
            const NodeInfo& local_info = node_monitor_.local_node_info();
            NodeInternalCode code = static_cast<NodeInternalCode>(msgin.code());
            switch (code) {
                case INTERNAL_HANDSHAKE: {
                    NodeType from_node_type = static_cast<NodeType>(msgin.ReadUInt8());
                    switch (from_node_type) {
                        case NodeType::MASTER: {
                            switch (local_info.type) {
                                case NodeType::MASTER: {
                                    LOG_ERROR("can not link two master node together");
                                    Close();
                                }
                                return;
                                case NodeType::NORMAL:
                                    SendFetchNodeID();
                                    break;
                                case NodeType::STANDALONE:
                                    SendHandShakeResponse((int)INTERNAL_LINK_STANDALONE);
                                    break;
                                case NodeType::UNKNOWN:
                                    Close();
                                    break;
                            }
                        }
                        break;
                        case NodeType::STANDALONE:
                            SendLinkStandaloneNode();
                            break;
                        case NodeType::NORMAL: {
                            switch (local_info.type) {
                                case NodeType::MASTER:
                                    SendHandShakeResponse((int)INTERNAL_FETCH_NODE_ID);
                                    break;
                                case NodeType::NORMAL:
                                    SendLinkNormalNode();
                                    break;
                                case NodeType::STANDALONE:
                                    SendHandShakeResponse((int)INTERNAL_LINK_STANDALONE);
                                    break;
                                case NodeType::UNKNOWN:
                                    Close();
                                    break;
                            }
                        }
                        break;
                        default:
                            Close();
                            return;
                    }
                }
                break;
                case INTERNAL_HANDSHAKE_RESPONSE: {
                    NodeInternalCode cmd = static_cast<NodeInternalCode>(msgin.ReadInt32());
                    switch (cmd) {
                        case INTERNAL_FETCH_NODE_ID:
                            SendFetchNodeID();
                            break;
                        case INTERNAL_LINK_NORMAL:
                            SendLinkNormalNode();
                            break;
                        case INTERNAL_LINK_STANDALONE:
                            SendLinkStandaloneNode();
                            break;
                        default:
                            break;
                    }
                }
                break;
                case INTERNAL_FETCH_NODE_ID: {
                    if (local_info.type != NodeType::MASTER || info_.node_id != 0u) {
                        Close();
                        return;
                    }
                    info_.node_name = msgin.ReadString();
                    info_.type = static_cast<NodeType>(msgin.ReadUInt8());
                    info_.listen_ip = msgin.ReadString();
                    info_.listen_port = msgin.ReadInt32();
                    // 给请求连接的节点分配一个节点ID
                    node_monitor_.RegisterNodeConnector(this, [this]() {
                        SendFetchNodeIDResponse(info_.node_id);
                    });
                }
                break;
                case INTERNAL_FETCH_NODE_ID_RESPONSE: {
                    if (local_info.type != NodeType::NORMAL) {
                        Close();
                        return;
                    }
                    info_.node_id = msgin.ReadUInt16();
                    info_.node_name = msgin.ReadString();
                    info_.type = static_cast<NodeType>(msgin.ReadUInt8());
                    info_.listen_ip = msgin.ReadString();
                    info_.listen_port = msgin.ReadInt32();
                    node_monitor_.local_node_info_.node_id = msgin.ReadUInt16();
                    node_monitor_.SetNodeConnector(this);
                }
                break;
                case INTERNAL_LINK_NORMAL: {
                    if (local_info.type != NodeType::NORMAL || local_info.node_id == 0u) {
                        LOG_ERROR("duplicate link normal request");
                        Close();
                        return;
                    }
                    info_.node_id = msgin.ReadUInt16();
                    info_.node_name = msgin.ReadString();
                    info_.type = static_cast<NodeType>(msgin.ReadUInt8());
                    info_.listen_ip = msgin.ReadString();
                    info_.listen_port = msgin.ReadInt32();
                    if (node_monitor_.SetNodeConnector(this)) {
                        SendLinkNormalNodeResponse();
                    } else {
                        LOG_ERROR("duplicate link normal request");
                        Close();
                        return;
                    }
                }
                break;
                case INTERNAL_LINK_NORMAL_RESPONSE: {
                    if (local_info.type != NodeType::NORMAL || local_info.node_id == 0u) {
                        LOG_ERROR("bad link normal response");
                        Close();
                    }
                    info_.node_id = msgin.ReadUInt16();
                    info_.node_name = msgin.ReadString();
                    info_.type = static_cast<NodeType>(msgin.ReadUInt8());
                    info_.listen_ip = msgin.ReadString();
                    info_.listen_port = msgin.ReadInt32();
                    if (node_monitor_.SetNodeConnector(this)) {
                        ; // ok
                    } else {
                        LOG_ERROR("duplicate link normal request");
                        Close();
                        return;
                    }
                }
                break;
                case INTERNAL_LINK_STANDALONE: {
                    if (local_info.type != NodeType::STANDALONE) {
                        Close();
                        return;
                    }
                    info_.node_name = msgin.ReadString();
                    info_.type = static_cast<NodeType>(msgin.ReadUInt8());
                    info_.listen_ip = msgin.ReadString();
                    info_.listen_port = msgin.ReadInt32();
                    node_monitor_.RegisterNodeConnector(this, [this]() {
                        SendLinkStandaloneNodeResponse();
                    });
                }
                break;
                case INTERNAL_LINK_STANDALONE_RESPONSE: {
                    info_.node_name = msgin.ReadString();
                    info_.type = static_cast<NodeType>(msgin.ReadUInt8());
                    info_.listen_ip = msgin.ReadString();
                    info_.listen_port = msgin.ReadInt32();
                    if (info_.type != NodeType::STANDALONE) {
                        Close();
                        return;
                    }
                    node_monitor_.RegisterNodeConnector(this, []() {});
                }
                break;
                default:
                    LOG_WARN("unexpected message handled by connector!\n");
                    break;
            }
        }

        void Connector::SendHandShake()
        {
            MessageOut msgout((uint16_t)INTERNAL_HANDSHAKE, 40, mempool());
            msgout.WriteUInt8(static_cast<uint8_t>(node_monitor_.local_node_info().type));
            SendMessage(msgout);
        }

        void Connector::SendHandShakeResponse(int cmd)
        {
            MessageOut msgout((uint16_t)INTERNAL_HANDSHAKE_RESPONSE, 40, mempool());
            msgout.WriteInt32(cmd);
            SendMessage(msgout);
        }

        void Connector::SendFetchNodeID()
        {
            MessageOut msgout((uint16_t)INTERNAL_FETCH_NODE_ID, 40, mempool());
            const NodeInfo& local_info = NodeMonitor::instance().local_node_info();
            msgout.WriteString(local_info.node_name);
            msgout.WriteUInt8(static_cast<uint8_t>(local_info.type));
            msgout.WriteString(local_info.listen_ip);
            msgout.WriteInt32(local_info.listen_port);
            // TODO add auth string
            SendMessage(msgout);
        }

        void Connector::SendFetchNodeIDResponse(uint16_t generated_node_id)
        {
            const NodeInfo& local_info = node_monitor_.local_node_info();
            MessageOut msgout((uint16_t)INTERNAL_FETCH_NODE_ID_RESPONSE, 40, mempool());
            msgout.WriteUInt16(local_info.node_id);
            msgout.WriteString(local_info.node_name);
            msgout.WriteUInt8(static_cast<uint8_t>(local_info.type));
            msgout.WriteString(local_info.listen_ip);
            msgout.WriteInt32(local_info.listen_port);
            msgout.WriteUInt16(generated_node_id);
            SendMessage(msgout);
        }

        void Connector::SendLinkNormalNode()
        {
            const NodeInfo& local_info = NodeMonitor::instance().local_node_info();
            MessageOut msgout((uint16_t)INTERNAL_LINK_NORMAL, 40, mempool());
            msgout.WriteUInt16(local_info.node_id);
            msgout.WriteString(local_info.node_name);
            msgout.WriteUInt8(static_cast<uint8_t>(local_info.type));
            msgout.WriteString(local_info.listen_ip);
            msgout.WriteInt32(local_info.listen_port);
            SendMessage(msgout);
        }

        void Connector::SendLinkNormalNodeResponse()
        {
            const NodeInfo& local_info = node_monitor_.local_node_info();
            assert(local_info.node_id != 0u);
            MessageOut msgout((uint16_t)INTERNAL_LINK_NORMAL_RESPONSE, 40, mempool());
            msgout.WriteUInt16(local_info.node_id);
            msgout.WriteString(local_info.node_name);
            msgout.WriteUInt8(static_cast<uint8_t>(local_info.type));
            msgout.WriteString(local_info.listen_ip);
            msgout.WriteInt32(local_info.listen_port);
            SendMessage(msgout);
        }

        void Connector::SendLinkStandaloneNode()
        {
            const NodeInfo& local_info = node_monitor_.local_node_info();
            assert(local_info.node_id != 0u);
            MessageOut msgout((uint16_t)INTERNAL_LINK_STANDALONE, 40, mempool());
            msgout.WriteString(local_info.node_name);
            msgout.WriteUInt8(static_cast<uint8_t>(local_info.type));
            msgout.WriteString(local_info.listen_ip);
            msgout.WriteInt32(local_info.listen_port);
            SendMessage(msgout);
        }

        void Connector::SendLinkStandaloneNodeResponse()
        {
            const NodeInfo& local_info = node_monitor_.local_node_info();
            assert(local_info.node_id != 0u);
            MessageOut msgout((uint16_t)INTERNAL_LINK_STANDALONE_RESPONSE, 40, mempool());
            msgout.WriteString(local_info.node_name);
            msgout.WriteUInt8(static_cast<uint8_t>(local_info.type));
            msgout.WriteString(local_info.listen_ip);
            msgout.WriteInt32(local_info.listen_port);
            SendMessage(msgout);
        }

        void Connector::OnReceive(std::size_t count)
        {
            while (count >= MessageIn::HEAD_SIZE) {
                uint32_t pktlen = 0;
                CopyReceive((char*)&pktlen, 4);

                if (pktlen < MessageIn::HEAD_SIZE || pktlen > 1024 * 1024 * 5) {
                    LOG_WARN("the message size it too small or big, size= %u\n", pktlen);
                }

                if (pktlen >= MessageIn::HEAD_SIZE && count >= pktlen) {
                    message_data_t data;
                    PopReceive(data, pktlen);
                    try {
                        MessageIn msgin(data);
                        msgin.ReadHead();
                        if (info_.node_id != 0u) {
                            if (info_.type == NodeType::STANDALONE || node_monitor_.local_node_info().type == NodeType::STANDALONE) {
                                msgin.ResetFromNodeID(info_.node_id);
                            }
                        }

                        if (!msgin.to() && !authed()) {
                            HandleInternalMessage(msgin);
                        } else {
                            node_monitor_.OnConnectorReceiveMessage(this, msgin);
                        }
                    } catch (exception& ex) {
                        LOG_ERROR("occurred exception when handle node message: %s\n", ex.what());
                        Close();
                    }
                    count -= pktlen;
                } else {
                    break;
                }
            }
        }

        void Connector::OnConnect()
        {
            if (retry_count_ > 0) {
                retry_count_ = 0;   // reset count
            }
            if (!passive_) {
                SendHandShake();
            }
        }

        void Connector::OnConnectFail(int eno, const char* reason)
        {
            //LOG_WARN("[framework.cluster] connector to %s:%d fail, with error:%s", ipaddr().c_str(), port(), reason);
            info_.listen_ip = ipaddr();
            info_.listen_port = port();
            node_monitor_.RemoveNodeConnector(this);
        }

        void Connector::OnClose()
        {
            node_monitor_.RemoveNodeConnector(this);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
