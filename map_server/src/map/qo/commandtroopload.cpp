#include "commandtroopload.h"
#include <base/dbo/connection.h>

namespace ms
{
    namespace map
    {
        namespace qo
        {
            using namespace std;
            using namespace base::dbo;

            void CommandTroopLoad::OnCommandExecute()
            {
                PreparedStatement* pstmt = PreparedStatement::Create(2, "select id, data from s_troop where is_delete = 0");
                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        cb_(false);
                    } else {
                        while (rs.Next()) {
                            int i = 0;
                            int id = rs.GetInt32(++i);
                            string data = rs.GetString(++i);
                            m_mapMgr.AddTroopFromDb(id, data);
                            if (m_mapMgr.m_max_tid < id) {
                                m_mapMgr.m_max_tid = id;
                            }
                        }
                        cb_(true);
                    }
                    Stop();
                }, m_autoObserver);
            }
        }
    }
}
