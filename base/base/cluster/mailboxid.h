#ifndef BASE_CLUSTER_MAILBOXID_H
#define BASE_CLUSTER_MAILBOXID_H

#include "../global.h"
#include <iosfwd>

namespace base
{
    namespace gateway
    {
        class PacketInBase;
        class PacketOutBase;
    }

    namespace cluster
    {
        // 分布式邮箱ID
        class MailboxID
        {
        public:
            MailboxID() : nodeid_(0), pid_(0) {}
            MailboxID(uint16_t nodeid, uint32_t pid) : nodeid_(nodeid), pid_(pid) {}

            // 节点ID
            inline uint16_t nodeid() const {
                return nodeid_;
            }

            // 本地"消息进程"ID
            inline uint32_t pid() const {
                return pid_;
            }

            operator bool() const {
                return nodeid_ != 0u && pid_ != 0u;
            }

            bool operator == (const MailboxID& rhs) const {
                return nodeid() == rhs.nodeid() && pid() == rhs.pid();
            }

            bool operator != (const MailboxID& rhs) const {
                return !(*this == rhs);
            }

            void Clear() {
                nodeid_ = 0u;
                pid_ = 0u;
            }

            void Set(uint16_t nodeid, uint32_t pid) {
                nodeid_ = nodeid;
                pid_ = pid;
            }

            void Unify(const MailboxID& targetMailboxId) {
                nodeid_ = targetMailboxId.nodeid();
            }

            static const MailboxID& NullID() {
                static MailboxID nullid(0u, 0u);
                return nullid;
            }

        private:
            uint16_t nodeid_;
            uint32_t pid_;
            friend gateway::PacketInBase& operator >> (gateway::PacketInBase& pktin, MailboxID& mbid);
        };

        std::ostream& operator << (std::ostream& out, const MailboxID& mbid);
        gateway::PacketOutBase& operator << (gateway::PacketOutBase& pktout, const MailboxID& mbid);
        gateway::PacketInBase& operator >> (gateway::PacketInBase& pktin, MailboxID& mbid);

        // 节点ID生成器
        class NodeIDGen
        {
        public:
            // 主节点固定值
            static const uint16_t MASTER_NODE_ID;
            // 独立节点固定值
            static const uint16_t STANDALONE_NODE_ID;
            // 普通与独立节点分界值
            static const uint16_t NORMAL_STANDALONE_DIVIDING;
        };

        // 本地邮箱PID生成器
        class MailboxPIDGen
        {
        public:
            MailboxPIDGen() : sys_cur_(1u), normal_cur_(10000u) {}

            // 生成进程ID
            // sys 系统进程, 1 ~ 10000, 不可循环使用
            int32_t Gen(bool sys);

        private:
            static const uint32_t MAX_NORMAL_ID;
            uint32_t sys_cur_;
            uint32_t normal_cur_;
        };
    }
}

#endif // MAILBOXID_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
