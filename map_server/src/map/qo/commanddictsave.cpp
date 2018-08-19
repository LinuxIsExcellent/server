#include "commanddictsave.h"
#include <base/logger.h>
#include <base/dbo/preparedstatement.h>

namespace ms
{
    namespace map
    {
        namespace qo
        {
            using namespace std;
            using namespace base::dbo;

            void CommandDictSave::OnCommandExecute()
            {
                PreparedStatement* pstmt = PreparedStatement::Create(0, "insert into s_dict(uid, k, v) values(0, ?, ?) on duplicate key update v=values(v)");
                pstmt->SetString(m_k);
                pstmt->SetString(m_v);
//                 cout << "k = " << m_k << " v = " << m_v << endl;

                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        LOG_ERROR("CommandDictSave Error %s\n", rs.error_message());
                    }
                    Stop();
                }, m_autoObserver);
            }

        }
    }
}
