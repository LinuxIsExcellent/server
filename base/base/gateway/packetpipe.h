#ifndef BASE_GATEWAY_PACKETPIPE_H
#define BASE_GATEWAY_PACKETPIPE_H
#include "../observer.h"
#include "../cluster/mailboxid.h"
#include <unordered_set>

namespace base
{
    namespace cluster
    {
        class Mailbox;
    }

    namespace gateway
    {

        class PacketOut;

        class PacketIn;

        class UserSession;
        class PacketPipe
        {
            DISABLE_COPY(PacketPipe);
        public:
            PacketPipe(UserSession& userSession, const base::cluster::MailboxID& forwardTarget, base::cluster::Mailbox* mailbox);
            ~PacketPipe();

            void RegisterCode(uint16_t code) {
                m_codeSet.insert(code);
            }
            void ProcessPacketIn(base::gateway::PacketIn& pktin);

        private:
            void BackwardHandler(base::gateway::PacketOut& pktout);

            std::unordered_set<uint16_t> m_codeSet;
            UserSession& m_userSession;
            base::cluster::MailboxID m_forwardTarget;
            base::Observer* m_mailboxObserver;
            base::cluster::Mailbox* m_mailbox;
        };
    }
}

#endif // PACKETPIPE_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
