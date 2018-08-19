#include "commandagentsave.h"
#include <base/dbo/connection.h>
#include <base/logger.h>
#include "../agent.h"
#include "../msgqueue/msgqueue.h"
#include <list>

namespace ms
{
    namespace map
    {
        namespace qo
        {
            using namespace std;
            using namespace base::dbo;

            void CommandAgentSave::OnCommandExecute()
            {
                if (!m_agent) {
                    Stop();
                    return;
                }

                PreparedStatement* pstmt = PreparedStatement::Create(2, "insert into s_agent (uid, data) values (?,?) on duplicate key update data=values(data)");
                pstmt->SetInt64(m_agent->uid());
                pstmt->SetString(m_agent->Serialize());

                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        LOG_ERROR("CommandAgentSave Error %s\n", rs.error_message());
                    } else {
                        m_agent->SetClean();
                    }
                    Stop();
                }, m_autoObserver);
            }

            void CommandAgentDelete::OnCommandExecute()
            {
                PreparedStatement* pstmt = PreparedStatement::Create(2, "DELETE FROM s_agent WHERE uid = ?");
                pstmt->SetInt64(m_uid);

                pstmt->Execute([this](ResultSet& rs) {
                    if (rs.HasError()) {
                        LOG_ERROR("CommandAgentDelete Error %s\n", rs.error_message());
                    }
                    Stop();
                }, m_autoObserver);

            }

        }
    }
}
