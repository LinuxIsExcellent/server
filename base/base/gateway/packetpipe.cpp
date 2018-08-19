#include "packetpipe.h"
#include "packet.h"
#include "usersession.h"
#include "../cluster/mailbox.h"
#include <functional>

namespace base
{
    namespace gateway
    {
        PacketPipe::PacketPipe(UserSession& userSession,
                               const cluster::MailboxID& forwardTarget,
                               cluster::Mailbox* mailbox)
            : m_userSession(userSession), m_forwardTarget(forwardTarget), m_mailbox(mailbox)
        {
            m_mailboxObserver = m_mailbox->GetObserver();
            m_mailboxObserver->Retain();
            m_mailbox->SetBackwardHandler(std::bind(&PacketPipe::BackwardHandler, this, std::placeholders::_1));
        }

        PacketPipe::~PacketPipe()
        {
            SAFE_RELEASE(m_mailboxObserver);
        }

        void PacketPipe::ProcessPacketIn(PacketIn& pktin)
        {
            uint16_t code = pktin.code();

            if (m_codeSet.find(code) != m_codeSet.end()) {
                if (m_mailboxObserver->IsExist()) {
                    m_mailbox->Forward(m_forwardTarget, pktin);
                }
            }
        }

        void PacketPipe::BackwardHandler(PacketOut& pktout)
        {
            m_userSession.Send(pktout);
        }
    }
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
