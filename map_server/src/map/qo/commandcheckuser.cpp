#include "commandcheckuser.h"
#include <base/dbo/preparedstatement.h>
#include <base/logger.h>

namespace ms
{
    namespace map
    {
        namespace qo
        {
            using namespace std;
            using namespace base::dbo;

            void CommandCheckUsernameIsExist::OnCommandExecute()
            {
                PreparedStatement* pstmt = PreparedStatement::Create(0, "SELECT uid FROM s_user WHERE username = ?");
                pstmt->SetString(m_username);

                pstmt->Execute([this](ResultSet& rs) {
                    if (rs.HasError()) {
                        LOG_ERROR("CommandCheckUsernameIsExist Error %s\n", rs.error_message());
                    } else if (rs.rows_count() > 0) {
                        while (rs.Next()) {
                            int64_t uid = rs.GetInt64(1);
                            Stop();
                            m_cb(true, uid);
                            return;
                        }
                    }
                    Stop();
                    m_cb(false, 0);
                }, m_autoObserver);
            }

            void CommandCheckNicknameIsExist::OnCommandExecute()
            {
                PreparedStatement* pstmt = PreparedStatement::Create(0, "SELECT nickname FROM s_user WHERE nickname = ?");
                pstmt->SetString(m_nickname);

                pstmt->Execute([this](ResultSet& rs) {
                    bool isExist = false;
                    if (rs.HasError()) {
                        LOG_ERROR("CommandCheckNicknameIsExist Error %s\n", rs.error_message());
                        isExist = true;
                    } else if (rs.rows_count() > 0) {
                        isExist = true;
                    }
                    Stop();
                    m_cb(isExist);
                }, m_autoObserver);
            }
        }
    }
}
