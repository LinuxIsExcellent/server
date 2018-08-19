#include "dbtemplate.h"

namespace base
{
    namespace dbo
    {
        class CommandExecuteStatement : public command::Command
        {
        public:
            CommandExecuteStatement(StatementBase* stmt, std::function<void(ResultSet&)> resultHandler)
                : m_stmt(stmt), m_resultHandler(resultHandler) {
                m_stmt->Retain();
            }

            virtual ~CommandExecuteStatement() {
                m_stmt->Release();
            }

            virtual void OnCommandExecute() {
                m_stmt->Execute([this](ResultSet & rs) {
                    m_resultHandler(rs);
                    Stop();
                }, m_autoObserver);
            }

        private:
            StatementBase* m_stmt;
            std::function<void(ResultSet&)> m_resultHandler;
        };

        void DBTemplate::Execute(command::Runner& runner, int poolId, const char* sql, std::function< void(ResultSet&) > resultHandler)
        {
            Statement* stmt = Statement::Create(poolId, sql);
            CommandExecuteStatement* cmd = new CommandExecuteStatement(stmt, resultHandler);
            runner.PushCommand(cmd);
        }

        void DBTemplate::ExecutePrepare(command::Runner& runner, int poolId, const char* sql, std::function< void(PreparedStatement&) > prepareHandler, std::function< void(ResultSet&)> resultHandler)
        {
            PreparedStatement* pstmt = PreparedStatement::Create(poolId, sql);
            prepareHandler(*pstmt);
            CommandExecuteStatement* cmd = new CommandExecuteStatement(pstmt, resultHandler);
            runner.PushCommand(cmd);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
