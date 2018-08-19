#ifndef BASE_REST_BINARYSERVER_H
#define BASE_REST_BINARYSERVER_H

#include "server.h"
#include "binaryclient.h"

namespace base
{
    namespace rest
    {
        class BinaryRestDispatcher
        {
        public:
            virtual void Process(BinaryClient* client, BinaryMessageIn& msgin) = 0;
        };

        class BinaryServer : public Server
        {
        public:
            BinaryServer(BinaryRestDispatcher& dispatcher);
            virtual ~BinaryServer();

            virtual const char* GetObjectName() {
                return "base::rest::BinaryServer";
            }

            bool StartWithConfigureFile();
            virtual void Stop();

        private:
            virtual void OnUpdate() {
                UpdateClientList();
            }
            virtual void OnListenerAccept(net::Listener* sender, int clientfd);
            void UpdateClientList();

            BinaryRestDispatcher& dispatcher_;
            std::list<BinaryClient*> clients_;
        };
    }
}

#endif // BINARYSERVER_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
