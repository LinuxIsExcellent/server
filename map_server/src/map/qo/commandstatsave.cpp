#include "commandstatsave.h"
#include <base/logger.h>
#include <base/dbo/preparedstatement.h>

namespace ms
{
    namespace map
    {
        namespace qo
        {
            using namespace base::dbo;

            void CommandStatSave::OnCommandExecute()
            {
                PreparedStatement* pstmt = PreparedStatement::Create(0, "insert into s_stat(k, v) values(?, ?) on duplicate key update v=values(v)");
                pstmt->SetString(m_k);
                pstmt->SetString(m_v);

                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        LOG_ERROR("CommandStatSave Error %s\n", rs.error_message());
                    }
                    Stop();
                }, m_autoObserver);
            }

        }
    }
}
