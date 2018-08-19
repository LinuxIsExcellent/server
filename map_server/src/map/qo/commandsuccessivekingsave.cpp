#include "commandsuccessivekingsave.h"
#include <base/dbo/preparedstatement.h>
#include <base/logger.h>

namespace ms
{
    namespace map
    {
        namespace qo
        {
            using namespace base::dbo;

            void CommandSuccessiveKingSave::OnCommandExecute()
            {
                PreparedStatement* pstmt = PreparedStatement::Create(2, "INSERT INTO s_palace_war_record (type, data, timestamp) VALUES(?, ?, ?)");
                pstmt->SetInt32((int)model::PalaceWarRecordType::BE_KING);
                pstmt->SetString(m_sk.Serialize());
                pstmt->SetInt64(m_sk.timestamp);

                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        LOG_ERROR("CommandSuccessiveKingSave Error %s\n", rs.error_message());
                    }
                    Stop();
                }, m_autoObserver);
            }
        }
    }
}
