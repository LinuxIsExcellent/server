#ifndef BASE_GATEWAY_USER_SESSION_H
#define BASE_GATEWAY_USER_SESSION_H

#include "../object.h"
#include <vector>

namespace base
{
    namespace cluster
    {

        class Mailbox;
        class MailboxID;
    }

    namespace gateway
    {
        class UserClient;
        class PacketIn;
        class PacketOut;
        class PacketPipe;

        // 用户会话
        /// gateway 持有 userclient
        /// usersession 持有 userclient
        class UserSession : public Object
        {
        public:
            UserSession(UserClient* client);
            virtual ~UserSession();

            virtual const char* GetObjectName() {
                return "base::gateway::UserSession";
            }

            UserClient* client() {
                return client_;
            }

            void RegisterPipe(const std::vector<uint16_t>& codeSet, base::cluster::Mailbox* mailbox, const base::cluster::MailboxID& forwardTarget);

            void ClearAllPipe();

            void Send(base::gateway::PacketOut& pktout);

            virtual void OnUserClientReceivePacket(PacketIn& pktin) = 0;
            virtual void OnUserClientClose() = 0;
            virtual void OnUserSessionSend(PacketOut& pktout) {}

        private:
            void ProcessUserClientReceivePacket(PacketIn& pktin);

            UserClient* client_;
            std::vector<PacketPipe*> m_pipeList;
            friend class UserClient;
        };
    }
}

#endif
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
