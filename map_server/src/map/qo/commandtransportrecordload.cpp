#include "commandtransportrecordload.h"
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

            void CommandTransportRecordLoad::OnCommandExecute()
            {
                PreparedStatement* pstmt = PreparedStatement::Create(0, "SELECT max(id) as id from s_transport_record");
                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        LOG_ERROR("CommandTransportRecordLoad Error");
                        cb_(false);
                    } else {
                        while(rs.Next()) {
                            rs.Dump();
                            if (!rs.IsNull(1))
                            {
                                m_mapMgr.m_max_transport_record_id = rs.GetInt64(1);
                                LOG_DEBUG("CommandTransportRecordLoad maxRecordId is %d", m_mapMgr.m_max_transport_record_id);
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
