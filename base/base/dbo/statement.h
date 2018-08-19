#ifndef BASE_DBO_STATEMENT_H
#define BASE_DBO_STATEMENT_H

#include "../global.h"
#include "resultset.h"
#include "internal/rowreader.h"
#include <functional>
#include "../observer.h"

namespace base
{
    namespace dbo
    {
        class Connection;
        class ConnectionLockScope;

        namespace internal
        {
            class PacketIn;
            class ConnectionImpl;
        }

        typedef std::function<void(ResultSet&)> execute_callback_t;

        class StatementBase : public Object
        {
        public:
            StatementBase(int pool_id, const char* sql);
            virtual ~StatementBase();

            virtual const char* GetObjectName() {
                return "base::dbo::StatementBase";
            }

            int pool_id() const {
                return pool_id_;
            }
            int64_t create_tick() const {
                return create_tick_;
            }
            const std::string& sql() const {
                return sql_;
            }
            bool IsValidate() const {
                return cb_ && (observer_ == nullptr || observer_->IsExist());
            }
            bool IsLogErrorEnable() const {
                return m_logErrorEnable;
            }
            void SetLogErrorEnable(bool enable) {
                m_logErrorEnable = enable;
            }

            // 是否已执行完毕
            virtual bool IsFinish() const = 0;

            // 执行 observer可为空，为空表示不检查回调函数的生命周期
            void Execute(const execute_callback_t& cb, Observer* observer);
            void Execute(const execute_callback_t& cb, const AutoObserver& auto_observer) {
                Execute(cb, auto_observer.GetObserver());
            }
            void ExecuteWithConnection(const ConnectionLockScope& scope, const execute_callback_t& cb, Observer* observer);
            void ExecuteWithConnection(const ConnectionLockScope& scope, const execute_callback_t& cb, const AutoObserver& auto_observer) {
                ExecuteWithConnection(scope, cb, auto_observer.GetObserver());
            }

            void HandleRawResponse(internal::PacketIn& pktin);
            void HandleError(int error, const char* description);

            virtual void HandleResponse(internal::PacketIn& pktin) = 0;
            // 连接已准备就绪，正式执行语句
            void CallRealExecute() ;


        protected:
            internal::ConnectionImpl* realconn() {
                return realconn_;
            }
            void ResetSequenceID() {
                sequenct_id_ = 0;
            }
            uint8_t FetchSequenceID() {
                return sequenct_id_++;
            }

            std::function<void(ResultSet&)> cb_;
            Observer* observer_;
            std::string sql_;
            ResultSet rs_;

        private:
            virtual void OnCallRealExecute() = 0;
            void SetRealConn(internal::ConnectionImpl* realconn) {
                realconn_ = realconn;
            }
            int pool_id_;
            internal::ConnectionImpl* realconn_;
            uint8_t sequenct_id_;
            int64_t create_tick_;
            bool m_logErrorEnable = true;
            friend class Connection;
        };

        class Statement : public StatementBase
        {
        public:
            static Statement* Create(int pool_id, const char* sql);
            static Statement* Create(int pool_id, const std::string& sql) {
                return Create(pool_id, sql.c_str());
            }

            virtual ~Statement();

            virtual bool IsWait() const {
                return phase_ == STATEMENT_WAIT;
            }
            virtual bool IsFinish() const {
                return phase_ == STATEMENT_FINISH;
            }

        private:
            Statement(int pool_id, const char* sql)
                : StatementBase(pool_id, sql), phase_(STATEMENT_NEW) {}
            virtual void OnCallRealExecute();
            virtual void HandleResponse(internal::PacketIn& pktin);

            enum StatementPhase {
                STATEMENT_NEW,                  // 新建的语句
                STATEMENT_WAIT,         // 等待执行的语句
                STATEMENT_WAIT_RESPONSE,        // 等待执行结果
                STATEMENT_FINISH,               // 已完成的语句
            };
            StatementPhase phase_;
        };
    }
}

#endif // STATEMENT_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
