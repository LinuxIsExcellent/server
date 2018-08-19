#include "commanddictload.h"
#include "../map.h"
#include "../mapProxy.h"
#include "../alliance.h"
#include <base/dbo/preparedstatement.h>
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
            using namespace base::dbo;
            using namespace base::lua;

            void CommandDictLoad::OnCommandExecute()
            {
                PreparedStatement* pstmt = PreparedStatement::Create(0, "SELECT k, v FROM s_dict WHERE uid = 0");
                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        LOG_ERROR("CommandDictLoad Error");
                        m_cb(false);
                    } else {
                        while(rs.Next()) {
                            int i = 0;
                            string k = rs.GetString(++i);
                            string v = rs.GetString(++i);
                            if (k == "ms_global") {
                                DataTable dt = Parser::ParseAsDataTable(v.c_str());
                                g_mapMgr->proxy()->SetMapGlobalData(dt);
                            }
                        }
                        m_cb(true);
                    }
                    Stop();
                }, m_autoObserver);
            }
        }
    }
}