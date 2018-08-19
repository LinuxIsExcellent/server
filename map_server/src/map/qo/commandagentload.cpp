#include "commandagentload.h"
#include <base/dbo/connection.h>
#include <base/logger.h>
#include "../msgqueue/msgqueue.h"
#include "../agent.h"

namespace ms
{
    namespace map
    {
        namespace qo
        {
            using namespace std;
            using namespace base::dbo;

            void CommandAgentLoad::OnCommandExecute()
            {
                PreparedStatement* pstmt = PreparedStatement::Create(2, "select uid, data from s_agent");
                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        cb_(false);
                        Stop();
                    } else {
                        while (rs.Next()) {
                            int i = 0;
                            int64_t uid = rs.GetInt64(++i);
                            string data = rs.GetString(++i);

                            Agent* agent = new Agent(uid);
                            if (agent->Deserialize(data)) {
                                agent->msgQueue().Init(agent);

                                m_mapMgr.m_agents.emplace(uid, agent);
                            }
                        }
                        cb_(true);
                        Stop();
                    }
                }, m_autoObserver);
            }

        }
    }
}
