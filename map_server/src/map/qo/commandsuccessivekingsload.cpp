#include "commandsuccessivekingsload.h"
#include "../info/palaceWar.h"
#include "../mapMgr.h"
#include "../unit/palace.h"
#include <base/dbo/preparedstatement.h>
#include <model/metadata.h>
#include <base/logger.h>
#include <base/lua/parser.h>

namespace ms
{
    namespace map
    {
        namespace qo
        {
            using namespace std;
            using namespace base;
            using namespace info;
            using namespace model;
            using namespace base::dbo;
            using namespace base::lua;

            void CommandSuccessiveKingsLoad::OnCommandExecute()
            {
                PreparedStatement* pstmt = PreparedStatement::Create(2, "SELECT data, timestamp FROM s_palace_war_record WHERE type = ? ORDER BY timestamp DESC LIMIT 100");
                pstmt->SetInt32((int)model::PalaceWarRecordType::BE_KING);

                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        LOG_ERROR("CommandSuccessiveKingsLoad Error");
                    } else {
                        while (rs.Next()) {
                            int i = 0;
                            SuccessiveKing sk;
                            std::string data = rs.GetString(++i);
                            sk.timestamp = rs.GetInt64(++i);
                            DataTable dt = Parser::ParseAsDataTable(data);
                            sk.headId = dt.Get(1)->ToInteger();
                            sk.allianceNickname = dt.Get(2)->ToString();
                            sk.nickname = dt.Get(3)->ToString();
                            sk.n = dt.Get(4)->ToInteger();
                            g_mapMgr->capital().m_successiveKings.push_back(sk);
                        }
                    }
                    Stop();
                }, m_autoObserver);
            }
        }
    }
}
