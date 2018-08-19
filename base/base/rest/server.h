#ifndef BASE_REST_SERVER_H
#define BASE_REST_SERVER_H

#include "../net/listener.h"
#include "../action/actioninterval.h"
#include <string>

namespace base
{
    namespace memory
    {
        class MemoryPool;
    }

    namespace rest
    {
        class ServerUpdateAction;

        class Server : public net::Listener::EventHandler
        {
        public:
            Server();
            virtual ~Server();

            virtual const char* GetObjectName() {
                return "base::rest::Server";
            }

            base::memory::MemoryPool& mempool() {
                return *mempool_;
            }
            const std::string& auth_key() const {
                return auth_key_;
            }

            virtual bool Start(const char* ip, int port, const std::string& auth_key);
            virtual void Stop();

        private:
            virtual void OnUpdate() = 0;
            net::Listener* listener_;
            base::memory::MemoryPool* mempool_;
            base::action::Executor* executor_;
            std::string auth_key_;

            friend class ServerUpdateAction;
        };
    }
}

#endif // SERVER_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
