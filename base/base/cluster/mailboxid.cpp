#include "mailboxid.h"
#include "message.h"
#include <ostream>
#include <cassert>
#include <limits>

namespace base
{
    namespace cluster
    {
        using namespace std;

        std::ostream& operator<<(std::ostream& out, const MailboxID& mbid)
        {
            out << "{node:" << mbid.nodeid() << ", pid:" << mbid.pid() << "}";
            return out;
        }

        gateway::PacketOutBase& operator<<(gateway::PacketOutBase& pktout, const MailboxID& mbid)
        {
            pktout.WriteUInt16(mbid.nodeid());
            pktout.WriteUInt32(mbid.pid());
            return pktout;
        }

        gateway::PacketInBase& operator>>(gateway::PacketInBase& pktin, MailboxID& mbid)
        {
            mbid.nodeid_ = pktin.ReadUInt16();
            mbid.pid_ = pktin.ReadUInt32();
            return pktin;
        }

        const uint16_t NodeIDGen::MASTER_NODE_ID = 1u;

        const uint16_t NodeIDGen::STANDALONE_NODE_ID = 1u;

        const uint16_t NodeIDGen::NORMAL_STANDALONE_DIVIDING = 10000u;

        const uint32_t MailboxPIDGen::MAX_NORMAL_ID = numeric_limits<uint32_t>::max() - 10u;

        int32_t MailboxPIDGen::Gen(bool sys)
        {
            if (sys) {
                assert(sys_cur_ < 10000u);
                return sys_cur_++;
            } else {
                normal_cur_++;
                if (normal_cur_ >= MAX_NORMAL_ID) {
                    normal_cur_ = 1000u;
                }
                return normal_cur_++;
            }
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
