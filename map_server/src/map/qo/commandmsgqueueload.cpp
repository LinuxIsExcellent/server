#include "commandmsgqueueload.h"
#include "../agent.h"
#include <base/dbo/connection.h>

namespace ms
{
    namespace map
    {
        namespace qo
        {
            using namespace std;
            using namespace base::dbo;

            void CommandMsgQueueLoad::OnCommandExecute()
            {
                PreparedStatement* pstmt = PreparedStatement::Create(2, "select * from s_msg_queue where uid = ?");
                pstmt->SetInt64(m_msgQueue.m_agent->uid());

                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        // error
                    } else {
                        while (rs.Next()) {
                            int i = 0;
                            info::DbMsgQueueInfo info;
                            info.uid = rs.GetInt64(++i);
                            info.mid = rs.GetInt32(++i);
                            info.type = static_cast<model::MessageQueueType>(rs.GetInt16(++i));
                            info.data = rs.GetString(++i);
                            info.create_time = rs.GetInt64(++i);

                            if (m_msgQueue.m_msg_id < info.mid) {
                                m_msgQueue.m_msg_id = info.mid;
                            }
                            m_msgQueue.LoadMsgFromDb(info);
                        }
                        m_msgQueue.SendMsg();
                    }
                    Stop();
                }, m_autoObserver);
            }
        }
    }
}
