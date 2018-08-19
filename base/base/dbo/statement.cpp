#include "statement.h"
#include "../logger.h"
#include "resultset.h"
#include "connection.h"
#include "internal/packet.h"
#include "internal/common.h"
#include "internal/connectionimpl.h"
#include "internal/rowreadertext.h"
#include "dbpool.h"
#include "../event/dispatcher.h"
#include "resultset.h"

namespace base
{
    namespace dbo
    {
        using namespace std;
        using namespace base::dbo::internal;

        /// StatementBase
        StatementBase::StatementBase(int pool_id, const char* sql)
            : observer_(nullptr), sql_(sql), pool_id_(pool_id), realconn_(nullptr),
              sequenct_id_(0)
        {
            create_tick_ = g_dispatcher->GetTickCache();
        }

        StatementBase::~StatementBase()
        {
            SAFE_RELEASE(observer_);
        }

        void StatementBase::CallRealExecute()
        {
            OnCallRealExecute();
        }

        void StatementBase::HandleRawResponse(PacketIn& pktin)
        {
            if (sequenct_id_ == pktin.sequence_id()) {
                ++sequenct_id_;
                HandleResponse(pktin);
            } else {
                LOG_ERROR("mysql packet sequence_id mismatch! except:%u, current:%u\n", sequenct_id_, pktin.sequence_id());
            }
        }

        void StatementBase::Execute(const execute_callback_t& cb, Observer* observer)
        {
            assert(observer_ == nullptr);
            cb_ = cb;
            observer_ = observer;
            SAFE_RETAIN(observer_);
            // 加入等待执行队列
            g_dbpool->QueueStatement(pool_id(), this);
        }

        void StatementBase::ExecuteWithConnection(const ConnectionLockScope& scope, const execute_callback_t& cb, Observer* observer)
        {
            assert(observer_ == nullptr);
            cb_ = cb;
            observer_ = observer;
            SAFE_RETAIN(observer_);
            // 立即执行
            scope->ExecuteStatement(this);
        }

        void StatementBase::HandleError(int error, const char* description)
        {
            rs_.error_code_ = error;
            rs_.error_message_ = description;
            LOG_ERROR("stmt[%s] error: %d, %s", sql_.c_str(), error, description);
            if (observer_ == nullptr || observer_->IsExist()) {
                if (cb_) {
                    cb_(rs_);
                    cb_ = NULL;
                } else {
                    LOG_WARN("bad stmt handle error function call..");
                }
            }
        }

        /// Statement
        Statement* Statement::Create(int pool_id, const char* sql)
        {
            Statement* obj = new Statement(pool_id, sql);
            obj->AutoRelease();
            return obj;
        }

        Statement::~Statement()
        {
        }

        void Statement::OnCallRealExecute()
        {
            phase_ = STATEMENT_WAIT_RESPONSE;
            PacketOut pktout(realconn()->mempool(), 20, FetchSequenceID());
            pktout.WriteFixedInteger<1>(internal::COM_QUERY);
            pktout.WriteFixedString(sql_);
            realconn()->Send(pktout);
        }

        void Statement::HandleResponse(PacketIn& pktin)
        {
            if (phase_ != STATEMENT_WAIT_RESPONSE) {
                LOG_ERROR("unexpected response\n");
                return;
            }
            if (rs_.Parse(pktin, PROTOCOL_TEXT)) {
                phase_ = STATEMENT_FINISH;
                if (rs_.HasError() && IsLogErrorEnable()) {
                    LOG_ERROR("dbo error: code= %u, %s, sql=%s\n", rs_.error_code(), rs_.error_message(), sql_.c_str());
                }
                if (cb_ && (observer_ == nullptr || observer_->IsExist())) {
                    cb_(rs_);
                    cb_ = NULL;
                }
            }
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
