#include "commandserverinfosave.h"
#include "../mapMgr.h"
#include "../agent.h"
#include <base/dbo/preparedstatement.h>
#include <base/logger.h>
#include <model/metadata.h>

namespace ms
{
    namespace map
    {
        namespace qo
        {
            using namespace std;
            using namespace model;
            using namespace base::dbo;

            void CommandServerInfoSave::OnCommandExecute()
            {
                PreparedStatement* pstmt = PreparedStatement::Create(4, "UPDATE s_area SET lang = ?, name = ?, king = ? WHERE id = ?");
                LangType lang = model::LangType::ALL;
                string kingNickname;
                if (const Agent* king = g_mapMgr->king()) {
                    lang = king->langType();
                    kingNickname = king->nickname();
                }
                pstmt->SetInt32((int)lang);
                pstmt->SetString(g_mapMgr->serverName());
                pstmt->SetString(kingNickname);
                pstmt->SetInt32(g_mapMgr->localServiceId());
                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        LOG_ERROR("CommandServerInfoSave Error %s\n", rs.error_message());
                    }
                    Stop();
                }, m_autoObserver);
            }
        }
    }
}
