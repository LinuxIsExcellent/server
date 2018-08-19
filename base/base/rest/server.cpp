#include "server.h"
#include "../memory/memorypool.h"
#include "../action/executor.h"
#include "../utils/crypto.h"
#include <iostream>

namespace base
{
    namespace rest
    {
        using namespace std;
        ///
        /// ServerUpdateAction
        ///
        class ServerUpdateAction : public base::action::ActionInterval
        {
        public:
            ServerUpdateAction(Server& s, int32_t interval)
                : ActionInterval(interval), server_(s) {}

        private:
            virtual bool IsDone() {
                return false;
            }

            virtual void OnInterval(int64_t tick, int32_t span) {
                server_.OnUpdate();
            }
            Server& server_;
        };

        ///
        /// Server
        ///
        Server::Server()
            : listener_(nullptr), mempool_(nullptr), executor_(nullptr)
        {
            mempool_ = new memory::MemoryPool(64, 256);
        }

        Server::~Server()
        {
            SAFE_DELETE(mempool_);
            SAFE_RELEASE(listener_);
            SAFE_RELEASE(executor_);
        }

        bool Server::Start(const char* ip, int port, const string& auth_key)
        {
            auth_key_ = base::utils::sha1hex(auth_key);
            executor_ = new base::action::Executor;
            listener_ = new net::Listener(*this);
            if (!listener_->Bind(ip, port)) {
                cout << "rest::Server listen at[" << ip << ":" << port << "]" << endl;
                return false;
            }

            ServerUpdateAction* act = new ServerUpdateAction(*this, 60 * 1000);
            executor_->RunAction(act);
            act->Release();
            return true;
        }

        void Server::Stop()
        {
            if (listener_) {
                listener_->Close();
                listener_->Release();
                listener_ = nullptr;
            }
            executor_->StopAllAction();
            SAFE_RELEASE(executor_);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
