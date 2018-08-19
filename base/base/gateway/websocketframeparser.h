#ifndef BASE_GATEWAY_WEBSOCKETFRAMEPARSER_H
#define BASE_GATEWAY_WEBSOCKETFRAMEPARSER_H

#include "userclient.h"

namespace base
{
    namespace gateway
    {
        enum class WebSocketFrameType : int
        {
            Continuation = 0,
            Text = 1,
            Binary = 2,
            ConnectionClose = 8,
            Ping = 9,
            Pong = 10
        };

        class WebSocketFrameParser
        {
        public:
            WebSocketFrameParser(UserClient& client) : m_client(client) {}
            ~WebSocketFrameParser() {}

            void OnReceive(std::size_t count);

        private:
            std::size_t PopReceive(std::vector<memory::RefMemoryChunk>& dst, std::size_t count);
            std::size_t CopyReceive(char* dst, std::size_t count);

            PacketIn::PacketHead pkthead_;
            UserClient& m_client;

            std::deque<memory::RefMemoryChunk> recvq_;
            std::size_t recvq_count_ = 0u;
        };
    }
}

#endif // WEBSOCKETFRAMEPARSER_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
