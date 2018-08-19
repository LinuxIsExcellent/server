#include "commandmapservicelistload.h"
#include "../worldmgr.h"
#include <base/dbo/connection.h>
#include <base/utils/utils_string.h>
#include <base/logger.h>

namespace fs
{
    namespace qo
    {
        using namespace std;
        using namespace base::dbo;

        void CommandMapServiceListLoad::OnCommandExecute()
        {
            /*PreparedStatement* pstmt = PreparedStatement::Create(4, "select id, lan_ip, ms_port from s_area where id != ?");
            pstmt->SetUInt32(gWorldMgr->m_msServiceId);
            pstmt->Execute([this](ResultSet & rs) {
                std::vector<MapServiceInfo> infos;
                if (rs.HasError()) {
                    LOG_ERROR("CommandMapServiceListLoad load s_area fail");
                    cb_(false, infos);
                } else {
                    while (rs.Next()) {
                        int i = 0;
                        MapServiceInfo ms;
                        ms.id = rs.GetInt32(++i);
                        ms.name = gWorldMgr->MsIdToName(ms.id);
                        ms.ip = rs.GetString(++i);
                        ms.port = rs.GetInt32(++i);
                        infos.push_back(ms);
                        if (gWorldMgr->m_msList.find(ms.name) == gWorldMgr->m_msList.end()) {
                            gWorldMgr->m_msList.emplace(ms.name, ms);
                            //printf("CommandMapServiceListLoad ... id %d, name %s, ms_ip %s, ms_port %d\n", ms.id, ms.name.c_str(), ms.ip.c_str(), ms.port);
                        }
                    }
                    cb_(true, infos);
                }
                Stop();
            }, m_autoObserver);*/
        }
    }
}
