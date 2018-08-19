#ifndef BASE_GATEWAY_USERCLIENT_H
#define BASE_GATEWAY_USERCLIENT_H

#include "../net/client.h"
#include "packet.h"

namespace base
{
    namespace gateway
    {
        class UserSession;
        class WebSocketHeadParser;
        class WebSocketFrameParser;

        class UserClient : public net::Client
        {
        public:
            struct EventHandler {
                virtual ~EventHandler() {}
                virtual void OnUserClientConnect(UserClient* sender) {}
                virtual void OnUserClientConnectFail(UserClient* sender, int eno, const char* reason) {}
                virtual void OnUserClientClose(UserClient* sender) = 0;
            };

            UserClient(EventHandler& event_handler, base::memory::MemoryPool& mempool)
                : net::Client(mempool), event_handler_(event_handler), session_(nullptr), connect_ts_(0), trust_(false) {}
            virtual ~UserClient() ;

            virtual const char* GetObjectName() {
                return "base::gateway::UserClient";
            }

            UserSession* session() {
                return session_;
            }

            // 设置会话
            void SetSession(UserSession* sess) {
                session_ = sess;
            }

            // 清除会话
            void ResetSession() {
                session_ = nullptr;
            }

            // 发送协议包
            void Send(PacketOut& pktout);

            bool is_expired(int64_t now) const {
                //return !trust_ && (now - connect_ts_ > 30);
                return !trust_ && (now - connect_ts_ > 300);
            }

            bool is_trust() const {
                return trust_;
            }
            void SetTrust() {
                trust_ = true;
            }

        private:
            virtual void OnClose() {
                event_handler_.OnUserClientClose(this);
            }
            virtual void OnConnect() ;
            virtual void OnConnectFail(int eno, const char* reason) {
                event_handler_.OnUserClientConnectFail(this, eno, reason);
            }
            virtual void OnReceive(std::size_t count) ;

            bool ProcessPacketIn(base::gateway::PacketIn& pktin);

            EventHandler& event_handler_;
            UserSession* session_;
            PacketIn::PacketHead pkthead_;
            int64_t connect_ts_;
            bool trust_;
            WebSocketHeadParser* websocket_head_parser_ = nullptr;
            WebSocketFrameParser* websocket_frame_parser_ = nullptr;
            friend class WebSocketHeadParser;
            friend class WebSocketFrameParser;
        };
    }
}
#endif
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
