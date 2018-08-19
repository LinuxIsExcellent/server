#include "commandstatsave.h"
#include <base/dbo/preparedstatement.h>
#include <base/logger.h>

namespace fs
{
    namespace qo
    {
        using namespace std;
        using namespace base::dbo;
        
        void CommandStatSave::OnCommandExecute()
        {
            PreparedStatement* pstmt = PreparedStatement::Create(0, "insert into s_stat (k, v) values (?,?) on duplicate key update v=values(v)");
            pstmt->SetString(m_k);
            pstmt->SetString(m_v);
            
            pstmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    LOG_ERROR("CommandStatSave Error");
                }
                Stop();
            }, m_autoObserver);
        }

    }
}
