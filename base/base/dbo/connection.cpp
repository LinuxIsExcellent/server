#include "connection.h"
#include "internal/connectionimpl.h"
#include "dbpool.h"
#include "../exception.h"
#include "../logger.h"
#include "../event/dispatcher.h"

namespace base
{
    namespace dbo
    {
        using namespace std;

        Connection::Connection(int pool_id, const internal::ServerInfo& server_conf,
                               Connection::EventHandler& evt_handler)
            : pool_id_(pool_id), server_conf_(server_conf), impl_(nullptr),
              cur_stmt_(nullptr), next_stmt_(nullptr), evt_handler_(evt_handler),
              status_(CONN_CLOSED), lock_count_(0)
        {
        }

        Connection::~Connection()
        {
            SAFE_RELEASE(impl_);
            SAFE_RELEASE(cur_stmt_);
            SAFE_RELEASE(next_stmt_);
        }

        void Connection::Connect()
        {
            assert(status_ == CONN_CLOSED);
            status_ = CONN_INIT;
            if (impl_ == nullptr) {
                impl_ = new internal::ConnectionImpl(*this, g_dbpool->mempool());
            }
            impl_->Connect();
        }

        void Connection::Disconnect()
        {
            // 强制置为忙碌状态，不会被误用
            if (impl_ != nullptr) {
                if (status_ != CONN_CLOSING) {
                    status_ = CONN_CLOSING;
                    impl_->Close();
                }
            }
        }

        void Connection::ExecuteStatement(StatementBase* stmt)
        {
            if (cur_stmt_ != nullptr) {
                assert(next_stmt_ == nullptr);
                // 加入下一个执行语句
                next_stmt_ = stmt;
                next_stmt_->Retain();
                return;
            }
            assert(cur_stmt_ == nullptr);
            cur_stmt_ = stmt;
            cur_stmt_->Retain();

            TryExecute();
        }

        void Connection::TryExecute()
        {
            if (cur_stmt_ == nullptr) {
                if (next_stmt_ != nullptr) {
                    // 执行下一个等待中的语句
                    cur_stmt_ = next_stmt_;
                    next_stmt_ = nullptr;
                }
            }
            if (cur_stmt_ == nullptr) {
                return;
            }
            assert(status_ == CONN_FREE);
            ++stat_.execute_count;
            stat_.execute_begin_tick = g_dispatcher->GetTickCache();
            stat_.last_active_tick = g_dispatcher->GetTickCache();
            status_ = CONN_BUSY;
            cur_stmt_->SetRealConn(impl_);
            cur_stmt_->CallRealExecute();
        }

        void Connection::OnConnect()
        {
        }

        void Connection::OnConnectFail(int error, const char* reason)
        {
            LOG_ERROR("connect to mysql server fail: %d, %s", error, reason);
            status_ = CONN_CLOSED;
            SAFE_RELEASE(impl_);
        }

        void Connection::OnAuthSuccess()
        {
            LOG_DEBUG("mysql connection [%d,%s], connect success!\n", pool_id(), server_conf().dbname.c_str());
            stat_.execute_count = 0;
            OnBecomeFree();
        }

        void Connection::OnAuthFail(int error, const char* reason)
        {
            LOG_ERROR("connect to mysql server fail: %d, %s", error, reason);
            Disconnect();
        }

        void Connection::OnReceivePacket(internal::PacketIn& pktin)
        {
            cur_stmt_->HandleRawResponse(pktin);
            if (cur_stmt_->IsFinish()) {
                cur_stmt_->Release();
                cur_stmt_ = nullptr;
                OnBecomeFree();
            }
        }

        void Connection::OnClose()
        {
            status_ = CONN_CLOSED;
            SAFE_RELEASE(impl_);
            LOG_DEBUG("mysql connection [%d,%s] closed, total execute count=%u!\n", pool_id(), server_conf().dbname.c_str(), stat_.execute_count);
        }

        void Connection::OnBecomeFree()
        {
            status_ = CONN_FREE;
            stat_.last_active_tick = g_dispatcher->GetTickCache();
            stat_.execute_begin_tick = 0;
            TryExecute();
            if (status_ == CONN_FREE && !locked()) {
                evt_handler_.OnConnectBecomeFree(this);
            }
        }
    }
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
