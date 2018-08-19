#include "mailbox.h"
#include "nodemonitor.h"

namespace base
{
    namespace cluster
    {
        using namespace std;

        Mailbox::Mailbox(Mailbox::EventHandler& handler, const char* name)
            : mempool_(NodeMonitor::instance().mempool()), handler_(handler), name_(name) {}

        Mailbox::~Mailbox()
        {
            NodeMonitor::instance().UnRegister(this);
        }

        Mailbox* Mailbox::Create(Mailbox::EventHandler& handler, const char* name, bool sys)
        {
            Mailbox* mb = new Mailbox(handler, name);
            if (!NodeMonitor::instance().Register(mb, sys)) {
                delete mb;
                mb = nullptr;
            }
            return mb;
        }

        void Mailbox::Cast(const MailboxID& to, MessageOut& msgout)
        {
            msgout.SetFrom(mbid_);
            msgout.SetTo(to);
            NodeMonitor::instance().SendMessage(msgout);
        }

        void Mailbox::Forward(const MailboxID& to, gateway::PacketIn& pktin)
        {
            pktin.SkipTo(0);
            base::gateway::packet_data_t data = pktin.ReadRaw(pktin.size());

            MessageOut msgout(0, pktin.size(), mempool());
            msgout.SetType((uint8_t)MessageType::FORWARD_PACKET_IN);
            msgout.WriteRaw(data);
            Cast(to, msgout);
        }

        void Mailbox::Backward(const MailboxID& to, gateway::PacketOut& pktout)
        {
            pktout.WriteHead();
            base::gateway::packet_data_t data = pktout.FetchData();

            MessageOut msgout(0, pktout.size(), mempool());
            msgout.SetType((uint8_t)MessageType::BACKWARD_PACKET_OUT);
            msgout.WriteRaw(data);
            Cast(to, msgout);
        }

        void Mailbox::HandleMessageReceive(MessageIn& msgin)
        {
            MessageType type = (MessageType)msgin.type();
            if (type == MessageType::FORWARD_PACKET_IN) {
                if (m_forwardHandler) {
                    base::gateway::packet_data_t data = msgin.ReadRaw(msgin.size() - MessageIn::HEAD_SIZE);
                    base::gateway::PacketIn pktin(data);
                    pktin.ReadHead();
                    m_forwardHandler(pktin);
                }
            } else if (type == MessageType::BACKWARD_PACKET_OUT) {
                if (m_backwardHandler) {
                    base::gateway::packet_data_t data = msgin.ReadRaw(msgin.size() - MessageIn::HEAD_SIZE);
                    base::gateway::PacketOut pktout(data, mempool());
                    m_backwardHandler(pktout);
                }
            } else {
                handler_.OnMessageReceive(msgin);
            }
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
