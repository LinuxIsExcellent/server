#include "binaryserver.h"
#include "../net/client.h"
#include "../event/dispatcher.h"
#include "configure.h"

namespace base
{
    namespace rest
    {
        using namespace std;

        ///
        /// BinaryServer
        ///
        BinaryServer::BinaryServer(BinaryRestDispatcher& dispatcher)
            : dispatcher_(dispatcher)
        {
        }

        BinaryServer::~BinaryServer()
        {
        }

        bool BinaryServer::StartWithConfigureFile()
        {
            BinaryConfigure conf;
            if (!conf.ReadFromConfigureFile()) {
                return false;
            }
            return Start(conf.ip.c_str(), conf.port, conf.auth_key);
        }

        void BinaryServer::Stop()
        {
            for (list<BinaryClient*>::iterator it = clients_.begin(); it != clients_.end(); ++it) {
                (*it)->Close();
                (*it)->Release();
            }
            clients_.clear();

            base::rest::Server::Stop();
        }

        void BinaryServer::OnListenerAccept(net::Listener* sender, int clientfd)
        {
            BinaryClient* c = new BinaryClient(mempool(), dispatcher_, auth_key());
            c->Connect(clientfd);
            // 加入列表中进行维护
            clients_.push_back(c);
        }

        void BinaryServer::UpdateClientList()
        {
            int64_t now = base::event::Dispatcher::instance().GetTimestampCache();

            for (list<BinaryClient*>::iterator it = clients_.begin(); it != clients_.end();) {
                if (!(*it)->connect() || now - (*it)->last_active_ts() > 60) {
                    (*it)->Close();
                    (*it)->Release();
                    it = clients_.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
