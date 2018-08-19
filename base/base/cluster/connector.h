#ifndef BASE_CLUSTER_CONNECTOR_H
#define BASE_CLUSTER_CONNECTOR_H

#include "../net/client.h"
#include "nodemonitor.h"

namespace base
{
    namespace cluster
    {
        enum NodeInternalCode {
            INTERNAL_HANDSHAKE = 1,
            INTERNAL_HANDSHAKE_RESPONSE,
            INTERNAL_FETCH_NODE_ID,
            INTERNAL_FETCH_NODE_ID_RESPONSE,
            INTERNAL_LINK_NORMAL,
            INTERNAL_LINK_NORMAL_RESPONSE,
            INTERNAL_LINK_STANDALONE,
            INTERNAL_LINK_STANDALONE_RESPONSE,
            INTERNAL_REGISTER_NAMED_MAILBOX,                // 注册命名邮箱
            INTERNAL_UNREGISTER_NAMED_MAILBOX,              // 取消注册命名邮箱
            INTERNAL_SYNC_NORMAL_NODE,
        };

        class Connector : public net::Client
        {
        public:
            DISABLE_COPY(Connector)
            Connector(NodeMonitor& node_monitor, memory::MemoryPool& mempool, bool passive, int32_t retry_count);
            virtual ~Connector();

            virtual const char* GetObjectName() {
                return "base::cluster::Connector";
            }

            const NodeInfo& info() const {
                return info_;
            }
            bool authed() const {
                return info_.node_id != 0u;
            }
            int32_t retry_count() const {
                return retry_count_;
            }

            void SetNodeID(uint32_t id) {
                info_.node_id = id;
            }

            void SendMessage(MessageOut& msgout);

        private:
            virtual void OnConnect();
            virtual void OnConnectFail(int eno, const char* reason);
            virtual void OnReceive(size_t count);
            virtual void OnClose();

        private:
            void SendHandShake();
            void SendHandShakeResponse(int cmd);
            void SendFetchNodeID();
            void SendFetchNodeIDResponse(uint16_t generated_node_id);
            void SendLinkNormalNode();
            void SendLinkNormalNodeResponse();
            void SendLinkStandaloneNode();
            void SendLinkStandaloneNodeResponse();

        private:
            void HandleInternalMessage(MessageIn& msgin);
            NodeInfo info_;
            NodeMonitor& node_monitor_;
            bool passive_;
            int32_t retry_count_;
        };
    }
}

#endif // CONNECTOR_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
