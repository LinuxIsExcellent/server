#include "commandunitload.h"
#include <base/dbo/connection.h>
#include "../unit/unit.h"
#include "../tpl/mapunittpl.h"

namespace ms
{
    namespace map
    {
        namespace qo
        {
            using namespace std;
            using namespace base::dbo;

            void CommandUnitLoad::OnCommandExecute()
            {
                PreparedStatement* pstmt = PreparedStatement::Create(2, "select id, tplid, x, y, data from s_unit");
                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        cb_(false);
                    } else {
                        while (rs.Next()) {
                            int i = 0;
                            int id = rs.GetInt32(++i);
                            int tplid = rs.GetInt32(++i);
                            int x = rs.GetInt32(++i);
                            int y = rs.GetInt32(++i);
                            string data = rs.GetString(++i);
                            if (const MapUnitTpl* tpl_unit = tpl::m_tploader->FindMapUnit(tplid)) {
                                m_mapMgr.AddUnitFromDb(id, x, y, data, tpl_unit);
                            }
                            if (m_mapMgr.m_max_id < id) {
                                m_mapMgr.m_max_id = id;
                            }
                            m_mapMgr.m_isDbLoad = true;
                        }
                        cb_(true);
                    }
                    Stop();
                }, m_autoObserver);
            }
        }
    }
}
