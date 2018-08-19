#include "commandunitsave.h"
#include <base/dbo/connection.h>
#include <base/logger.h>
#include "../unit/castle.h"
#include "../unit/resnode.h"
#include "../unit/unit.h"

namespace ms
{
    namespace map
    {
        namespace qo
        {
            using namespace std;
            using namespace base::dbo;

            void CommandUnitSave::OnCommandExecute()
            {
                if (!m_unit) {
                    Stop();
                    return;
                }

                PreparedStatement* pstmt = PreparedStatement::Create(2, "insert into s_unit (id, tplid, x, y, uid, data) values (?, ?, ?, ?, ?, ?) on duplicate key update tplid=values(tplid), x=values(x), y=values(y), uid=values(uid), data=values(data)");
                pstmt->SetInt32(m_unit->id());
                pstmt->SetInt32(m_unit->tpl().id);
                pstmt->SetInt32(m_unit->x());
                pstmt->SetInt32(m_unit->y());
                if(Castle* castle = m_unit->ToCastle()) {
                    pstmt->SetInt64(castle->uid());
                } else {
                    pstmt->SetInt64(0);
                }
                pstmt->SetString(m_unit->Serialize());

                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        LOG_ERROR("CommandUnitSave Error %s\n", rs.error_message());
                    } else {
                        m_unit->SetClean();
                    }
                    Stop();
                }, m_autoObserver);
            }
            
            void CommandUnitDelete::OnCommandExecute()
            {
                PreparedStatement* pstmt = PreparedStatement::Create(2, "DELETE FROM s_unit WHERE id = ?");
                pstmt->SetInt32(m_id);
                
                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        LOG_ERROR("CommandAgentDelete Error %s\n", rs.error_message());
                    }
                    Stop();
                }, m_autoObserver);
            }
            
        }
    }
}
