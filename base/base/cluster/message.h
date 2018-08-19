#ifndef BASE_CLUSTER_MESSAGE_H
#define BASE_CLUSTER_MESSAGE_H

#include "../gateway/packet.h"
#include "mailboxid.h"

namespace base
{
    namespace cluster
    {
        class Mailbox;

        // Message头部格式如下 head_size=21byte
        // 4 -- size            uint32
        // 2 -- code            uint16
        // 6 -- mbid_from       MailboxID
        // 6 -- mbid_to         MailboxID
        // 2 -- session         uint16
        // 1 -- type            uint8

        typedef gateway::packet_data_t message_data_t;

        enum class MessageType : uint8_t
        {
            RPC_CALL                = 1,
            RPC_CAST,
            SUBSCRIBE,
            FORWARD_PACKET_IN,
            BACKWARD_PACKET_OUT,

            EXTENSION_BEGIN         = 100,
        };

        // 收到的消息
        class MessageIn : public gateway::PacketInBase
        {
        public:
            DISABLE_COPY(MessageIn)
            MessageIn(message_data_t& data)
                : gateway::PacketInBase(data) {}
            virtual ~MessageIn() {}

            static const uint16_t HEAD_SIZE = 21;

            uint16_t code() const {
                return code_;
            }

            const MailboxID& from() const {
                return mbid_from_;
            }

            const MailboxID& to() const {
                return mbid_to_;
            }

            uint16_t session() const {
                return session_;
            }

            uint8_t type() const {
                return m_type;
            }

            void ReadHead() {
                SkipTo(4);
                code_ = ReadUInt16();
                *this >> mbid_from_
                      >> mbid_to_;
                session_ = ReadUInt16();
                m_type = ReadUInt8();
                if (rewrite_node_id != 0u) {
                    mbid_from_ = MailboxID(rewrite_node_id, mbid_from_.pid());
                }
            }

            void ResetFromNodeID(uint16_t node_id);

        private:
            uint16_t code_;
            uint16_t session_;
            MailboxID mbid_from_;
            MailboxID mbid_to_;
            uint8_t m_type;
            uint16_t rewrite_node_id = 0u;
        };

        // 发出的消息
        class MessageOut : public base::gateway::PacketOutBase
        {
        public:
            DISABLE_COPY(MessageOut)

            MessageOut(uint16_t code, uint32_t approx_size, base::memory::MemoryPool& mp)
                : gateway::PacketOutBase(approx_size < HEAD_SIZE ? HEAD_SIZE : approx_size, mp),
                  code_(code), session_(0) {
                SkipTo(HEAD_SIZE);
            }
            virtual ~MessageOut() {}

            static const uint16_t HEAD_SIZE = 21;

            const MailboxID& from() const {
                return mbid_from_;
            }

            const MailboxID& to() const {
                return mbid_to_;
            }

            uint16_t code() const {
                return code_;
            }

            uint16_t session() const {
                return session_;
            }

            void SetFrom(const MailboxID& from) {
                mbid_from_ = from;
            }
            void SetTo(const MailboxID& to) {
                mbid_to_ = to;
            }
            void SetType(uint8_t type) {
                m_type = type;
            }
            void SetSession(uint16_t session) {
                session_ = session;
            }

            void WriteHead() {
                SkipTo(0);
                WriteUInt32(size());
                WriteUInt16(code_);
                *this << mbid_from_
                      << mbid_to_;
                WriteUInt16(session_);
                WriteUInt8(m_type);
            }

            uint16_t code_;
            MailboxID mbid_from_;
            MailboxID mbid_to_;
            uint16_t session_;
            uint8_t m_type = 0u;
        };
    }
}

#endif // BASE_CLUSTER_MESSAGE_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
