#include "commandfighthistoryload.h"
#include <base/dbo/connection.h>
#include "../alliance.h"

namespace ms
{
    namespace map
    {
        namespace qo
        {
            using namespace std;
            using namespace base::dbo;

            void CommandFightHistoryLoad::OnCommandExecute()
            {
                PreparedStatement* pstmt = PreparedStatement::Create(2, "select max(id) from s_battle_history");
                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        cb_(false);
                        Stop();
                    } else {
                        while (rs.Next()) {
                            if (!rs.IsNull(1)) {
                                m_mapMgr.m_max_fight_id = rs.GetInt32(1);
                            }
                        }
                        Load();
                    }
                }, m_autoObserver);
            }

            void CommandFightHistoryLoad::Load()
            {
                PreparedStatement* pstmt = PreparedStatement::Create(2, "select id, redUid, redNickname, redAllianceId, redAllianceNickname, \
                                           blueUid, blueNickname, blueAllianceId, blueAllianceNickname, isRedDelete, isBlueDelete, winTeam, battleType, battleTime from s_battle_history \
                                           where isRedDelete = false or isBlueDelete = false");

                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        cb_(false);
                    } else {
                        while (rs.Next()) {
                            int i = 0;
                            BattleInfo* info = new BattleInfo;
                            info->id = rs.GetInt32(++i);
                            info->red.uid = rs.GetInt64(++i);
                            info->red.nickname = rs.GetString(++i);
                            info->red.allianceId = rs.GetInt64(++i);
                            info->red.allianceNickname = rs.GetString(++i);
                            info->blue.uid = rs.GetInt64(++i);
                            info->blue.nickname = rs.GetString(++i);
                            info->blue.allianceId = rs.GetInt64(++i);
                            info->blue.allianceNickname = rs.GetString(++i);
                            info->isRedDelete = rs.GetBoolean(++i);
                            info->isBlueDelete = rs.GetBoolean(++i);
                            info->winTeam = rs.GetInt32(++i);
                            info->battleType = rs.GetInt32(++i);
                            info->battleTime = rs.GetInt64(++i);

                            info->isDirty = false;

                            m_mapMgr.m_battleInfoList.emplace(info->id, info);
                            m_mapMgr.m_alliance->OnBattleUpdate(info, false);
                        }
                        cb_(true);
                    }
                    Stop();
                }, m_autoObserver);
            }

        }
    }
}
