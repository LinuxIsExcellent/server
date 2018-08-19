#include "commandmsgqueuesave.h"
#include <base/logger.h>
#include <base/dbo/connection.h>

namespace ms
{
    namespace map
    {
        namespace qo
        {
            using namespace std;
            using namespace base::dbo;

            void CommandMsgQueueDelete::OnCommandExecute()
            {
                if (!m_msg) {
                    Stop();
                    return;
                }

                PreparedStatement* pstmt = PreparedStatement::Create(2, "delete from s_msg_queue where uid = ? and mid = ?");
                pstmt->SetInt64(m_msg->uid());
                pstmt->SetInt32(m_msg->id());

                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        LOG_ERROR("CommandMsgQueueDelete Error %s\n", rs.error_message());
                    }
                    SAFE_DELETE(m_msg);
                    Stop();
                }, m_autoObserver);
            }

        }
    }
}
