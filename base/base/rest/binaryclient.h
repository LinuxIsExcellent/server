#ifndef BASE_REST_BINARYCLIENT_H
#define BASE_REST_BINARYCLIENT_H

#include "../net/client.h"
#include "../gateway/packet_base.h"

namespace base
{
    namespace rest
    {
        class BinaryRestDispatcher;

        // BinaryMessage头部格式如下 head_size=6byte
        // 4 -- size            uint32
        // 2 -- code            uint16
        class BinaryMessageIn : public gateway::PacketInBase
        {
        public:
            DISABLE_COPY(BinaryMessageIn);
            BinaryMessageIn(gateway::packet_data_t& data)
                : gateway::PacketInBase(data) {}
            virtual ~BinaryMessageIn() {}

            static const uint32_t HEAD_SIZE = 6u;

            uint16_t code() {
                return code_;
            }

            void ReadHead() {
                SkipTo(4);
                code_ = ReadUInt16();
            }

        private:
            uint16_t code_;
        };

        class BinaryMessageOut : public gateway::PacketOutBase
        {
        public:
            DISABLE_COPY(BinaryMessageOut);
            BinaryMessageOut(uint16_t code, uint32_t approx_size,
                             base::memory::MemoryPool& mp)
                : gateway::PacketOutBase(approx_size < HEAD_SIZE ? HEAD_SIZE : approx_size, mp),
                  code_(code) {
                SkipTo(HEAD_SIZE);
            }
            virtual ~BinaryMessageOut() {}

            static const uint32_t HEAD_SIZE = 6u;

            uint16_t code() const {
                return code_;
            }

            void WriteHead() {
                SkipTo(0);
                WriteUInt32(size());
                WriteUInt16(code_);
            }

        private:
            uint16_t code_;
        };

        class BinaryClient : public net::Client
        {
        public:
            BinaryClient(memory::MemoryPool& mempool, BinaryRestDispatcher& dispatcher, const std::string& auth_key);
            virtual ~BinaryClient();

            int64_t last_active_ts() const {
                return last_active_ts_;
            }

            void UpdateLastActiveTs();

            void Send(BinaryMessageOut& msgout);
            void SendError(int32_t rc, const char* msg, ...);

        private:
            virtual void OnConnect();
            virtual void OnClose();
            virtual void OnConnectFail(int eno, const char* reason) {}
            virtual void OnReceive(std::size_t count);

            BinaryRestDispatcher& dispatcher_;
            // 上次活跃时间
            int64_t last_active_ts_;
            enum class AuthPhase
            {
                INIT,
                CHALLENGE,
                OK,
            };
            AuthPhase auth_pahse_ = AuthPhase::INIT;
            const std::string& auth_key_;
            std::string auth_challenge_;
        };
    }
}
#define BMSGOUT(code, approx_len) base::rest::BinaryMessageOut msgout((uint16_t)code, approx_len, client->mempool())

#endif // BINARYCLIENT_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
