#include "usersession.h"
#include "userclient.h"
#include "packetpipe.h"

namespace base
{
    namespace gateway
    {
        UserSession::UserSession(UserClient* client)
            : client_(client)
        {
            client_->SetSession(this);
            client_->Retain();
        }

        UserSession::~UserSession()
        {
            client_->ResetSession();
            ClearAllPipe();
            SAFE_RELEASE(client_);
        }

        void UserSession::ClearAllPipe()
        {
            for (PacketPipe * pipe  : m_pipeList) {
                delete pipe;
            }
            m_pipeList.clear();
        }

        void UserSession::Send(PacketOut& pktout)
        {
            client_->Send(pktout);
        }

        void UserSession::RegisterPipe(const std::vector< uint16_t >& codeSet, cluster::Mailbox* mailbox, const cluster::MailboxID& forwardTarget)
        {
            PacketPipe* pipe = new PacketPipe(*this, forwardTarget, mailbox);
            for (uint16_t code : codeSet) {
                pipe->RegisterCode(code);
            }
            m_pipeList.push_back(pipe);
        }

        void UserSession::ProcessUserClientReceivePacket(PacketIn& pktin)
        {
            OnUserClientReceivePacket(pktin);
            for (PacketPipe * pipe : m_pipeList) {
                pktin.ReadHead();
                pipe->ProcessPacketIn(pktin);
            }
        }
    }
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
