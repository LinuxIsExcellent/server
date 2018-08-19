#ifndef MAP_AGENT_PROXY_H
#define MAP_AGENT_PROXY_H
#include <base/command/runner.h>
#include <base/cluster/mailboxid.h>
#include "point.h"
#include <vector>
#include <unordered_map>
#include <set>
#include <model/metadata.h>
#include <model/dirtyflag.h>
#include "msgqueue/msgqueue.h"
#include "info/agentinfo.h"
#include "info/property.h"
#include "info/palaceWar.h"
#include "info/cityPatrolInfo.h"
#include "troopevent.h"
#include "unitevent.h"
#include "agent.h"

namespace base
{
    namespace cluster
    {
        class MessageIn;
        class MessageOut;
    }
}

namespace ms
{
    namespace map
    {
        class Area;
        class Map;
        class Troop;
        class ArmyList;
        class Unit;
        class Troop;
        class Castle;
        struct AllianceSimple;
        struct BattleInfo;
        class CampTemp;
        class TrapSet;

        class AgentProxy
        {
        public:
            AgentProxy(Agent& agent)
                : m_agent(agent) {}
            ~AgentProxy() {}



            void HandleMessage(base::cluster::MessageIn& msgin);
            void SendMessage(base::cluster::MessageOut& msgout);
            template<typename ...Args>
            void SendMessage(uint16_t code, uint32_t approx_size, uint16_t session, Args ...args);


            inline void SendMapUnitUpdate(Unit* unit) {
                std::vector<Unit*> units;
                units.push_back(unit);
                SendMapUnitUpdate(units, std::vector<int>(),  false);
            }
            void SendMapUnitUpdate(const std::vector<Unit*>& units, const std::vector<int>& removeList, bool isOnViewMove);

            inline void SendMapUnitRemove(int unid) {
                std::vector<int> unids;
                unids.push_back(unid);
                SendMapUnitRemove(unids);
            }
            void SendMapUnitRemove(const std::vector<int>& unids);

            inline void SendMapTroopUpdate(Troop* troop) {
                if (m_agent.m_mbid) {
                    std::vector<Troop*> troops;
                    troops.push_back(troop);
                    SendMapTroopUpdate(troops, false);
                }
            }
            void SendMapTroopUpdate(std::vector<Troop*>& troops, bool isOnViewMove);

            inline void SendMapTroopRemove(int tid) {
                if (m_agent.m_mbid) {
                    std::vector<int> tids;
                    tids.push_back(tid);
                    SendMapTroopRemove(tids);
                }
            }
            void SendMapTroopRemove(const std::vector<int>& tids);

             void SendMapMarchResponse(model::MarchErrorNo errNo);
            // type 1:tips 2:跑马灯
            void SendNoticeMessage(model::ErrorCode id, int type, const std::string& notice = "");
            void SendMapPersonalInfoUpdate();
            // 发送同盟增援部队信息
            void SendAllianceReinforcements();
            // ------------
            void SendMapQueryPlayerInfoResponse(const Agent* agent, const std::vector<info::RankInfo>& vecRank);

            void SendMapAllianceInviteListResponse(const std::vector<Agent*>& list);

            void SendMapMonsterSiegeInfoUpdate();
            void SendAllianceCitiesInfoUpdate();
            //联盟单元的坐标信息
            void SendKingmapUnitPosition();
            //最近的单元坐标
            void SendMapGetNearestUnitResponse(const Unit* unit);
            // 最近资源坐标
            void SendMapSearchResponse(int x, int y);
            //服务器信息
            void SendGetServerInfosResponse();
            //历届国王
            void SendPalaceWarSuccessiveKingsResponse();
            //王城争霸记录
            void SendPalaceWarRecordsResponse(int x, int y, const std::list<info::PalaceWarRecord>& records);
            //获取王国设置信息
            void SendKingGetKingdomSetInfosResponse();
            //获取行军额外信息(瞭望塔军情)
            void SendGetTroopExtraInfoResponse(std::list<Troop*>& troops);
            // 发送撤回结果
            void SendRecallAllianceTroop(bool result);
            // 发送可出征野怪等级更新
            void SendCanMarchMonsterLevelUpdate();

        private:
            //AgentMessageHander
            void OnLuaCall(base::cluster::MessageIn& msgin);
            void OnLuaCast(base::cluster::MessageIn& msgin);
            void OnLuaForward(base::cluster::MessageIn& msgin);

        private:
            Agent& m_agent;
        };
    }
}

#endif    // MAP_AGENT_PROXY_H