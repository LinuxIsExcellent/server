#ifndef BASE_DBO_CONNECTION_H
#define BASE_DBO_CONNECTION_H

#include "../object.h"
#include "internal/configure.h"
#include "statement.h"
#include "preparedstatement.h"
#include <string>

namespace base
{

    template<class T >
    class Event;
    namespace dbo
    {
        class DBPool;
        class ConnectionLockScope;
        namespace internal
        {
            class ConnectionImpl;
            class ConnectionPool;
        }

        class Connection : public Object
        {
        public:
            struct Stat {
                Stat() : execute_count(0), execute_begin_tick(0), last_active_tick(0) {}
                uint32_t execute_count;
                int64_t execute_begin_tick;
                int64_t last_active_tick;
            };
            struct EventHandler {
                virtual ~EventHandler() {}
                virtual void OnConnectBecomeFree(Connection* conn) = 0;
            };

            enum ConnectionStatus {
                CONN_CLOSED,
                CONN_INIT,
                CONN_FREE,
                CONN_BUSY,
                CONN_CLOSING,
            };
            Connection(int pool_id, const internal::ServerInfo& server_conf, EventHandler& evt_hander);

            virtual ~Connection();

            virtual const char* GetObjectName() {
                return "base::dbo::Connection";
            }

            ConnectionStatus status() const {
                return status_;
            }
            bool locked() const {
                return lock_count_ > 0;
            }
            int pool_id() const {
                return pool_id_;
            }
            const internal::ServerInfo& server_conf() const {
                return server_conf_;
            }
            const Stat& stat() const {
                return stat_;
            }
            const StatementBase* cur_stmt() const {
                return cur_stmt_;
            }

            // 连接
            void Connect();

            // 断开
            void Disconnect();

            // 执行一条语句
            void ExecuteStatement(StatementBase* stmt);

        private:
            void RetainLock() {
                ++lock_count_;
            }
            void ReleaseLock() {
                --lock_count_;
                if (lock_count_ == 0 && status_ == CONN_FREE) {
                    evt_handler_.OnConnectBecomeFree(this);
                }
            }
            void TryExecute();

            void OnConnect();
            void OnConnectFail(int error, const char* reason);
            void OnAuthSuccess();
            void OnAuthFail(int error, const char* reason);
            virtual void OnReceivePacket(internal::PacketIn& pktin);
            virtual void OnClose();

            void OnBecomeFree();

        private:
            int pool_id_;
            const internal::ServerInfo& server_conf_;
            internal::ConnectionImpl* impl_;
            // 当前执行的语句
            StatementBase* cur_stmt_;
            // 下一个即将执行的语句
            StatementBase* next_stmt_;
            // 事件
            EventHandler& evt_handler_;
            // 状态
            ConnectionStatus status_;
            // 独占式锁定计数
            int lock_count_;
            // 统计
            Stat stat_;
            friend class StatementBase;
            friend class internal::ConnectionImpl;
            friend class ConnectionLockScope;
        };

        class ConnectionLockScope
        {
        public:
            ConnectionLockScope() : ptr_(nullptr) {}
            ConnectionLockScope(Connection* ptr) : ptr_(ptr) {
                if (ptr_ != nullptr) {
                    ptr_->RetainLock();
                }
            }
            ~ConnectionLockScope() {
                if (ptr_ != nullptr) {
                    ptr_->ReleaseLock();
                }
            }
            ConnectionLockScope& operator = (ConnectionLockScope& rhs) {
                if (ptr_ != nullptr) {
                    ptr_->ReleaseLock();
                }
                ptr_ = rhs.ptr_;
                if (ptr_ != nullptr) {
                    ptr_->RetainLock();
                }
                return *this;
            }
            ConnectionLockScope(const ConnectionLockScope& rhs) {
                ptr_ = rhs.ptr_;
                if (ptr_ != nullptr) {
                    ptr_->RetainLock();
                }
            }

            operator bool () const {
                return ptr_ != nullptr;
            }

            Connection* operator -> () const {
                return ptr_;
            }

            void ForceRelease() {
                if (ptr_ != nullptr) {
                    ptr_->ReleaseLock();
                    ptr_ = nullptr;
                }
            }

        private:
            Connection* ptr_;
        };
    }
}

#endif // CONNECTION_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
