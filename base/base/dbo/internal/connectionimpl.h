#ifndef BASE_DBO_INTERNAL_REALCONNECTION_H
#define BASE_DBO_INTERNAL_REALCONNECTION_H

#include "../../net/client.h"
#include "../connection.h"
#include "configure.h"
#include <string>
#include <map>

namespace base
{
    namespace dbo
    {
        class Statement;
        class PreparedStatement;
        struct PreparedStatementMetadata;

        namespace internal
        {
            class PacketIn;
            class PacketOut;

            enum ConnectionPhase {
                CONN_PHASE_INIT_HANDSHAKE,
                CONN_PHASE_HANDSHAKE_RESPONSE,
                CONN_PHASE_OK,
            };

            // 代表一个真实的连接
            class ConnectionImpl : public net::Client
            {
            public:
                ConnectionImpl(Connection& p, memory::MemoryPool& mempool);
                virtual ~ConnectionImpl();

                void Connect();
                void Send(PacketOut& pktout);

                PreparedStatementMetadata* FetchCachedPreparedStatement(const std::string& sql) {
                    std::map<std::string, PreparedStatementMetadata*>::iterator it = prepared_stmt_pool_.find(sql);
                    return it == prepared_stmt_pool_.end() ? 0 : it->second;
                }

                void SavePreparedStatementCache(const std::string& sql, PreparedStatementMetadata* pstmt) {
                    prepared_stmt_pool_[sql] = pstmt;
                }

            private:
                virtual void OnConnect();
                virtual void OnConnectFail(int eno, const char* reason);
                virtual void OnClose();
                virtual void OnReceive(std::size_t count);

                void OnReceivePacket(PacketIn& pktin);

                void HandleAuthInitHandshake(PacketIn& pktin);
                void HandleAuthResponse(PacketIn& pktin);

                Connection& p_;
                ConnectionPhase conn_phase_;

            private:
                // prepared statment 池
                std::map<std::string, PreparedStatementMetadata*> prepared_stmt_pool_;
            };
        }
    }
}

#endif // REALCONNECTION_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
