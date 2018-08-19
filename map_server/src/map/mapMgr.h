#ifndef MAP_MAP_MGR_H
#define MAP_MAP_MGR_H
#include <base/cluster/mailbox.h>
#include <base/command/runner.h>
#include <base/observer.h>
#include <base/bootable.h>
#include <lua/llimits.h>
#include "base/event.h"
#include <base/lua/luamessage.h>
#include <model/metadata.h>
#include "area.h"
#include "map.h"
#include <unordered_map>
#include <vector>
#include <queue>
#include <math.h>
#include "point.h"
#include <functional>
#include <map>
#include <tuple>
#include "troopevent.h"
#include "unitevent.h"
#include "interface.h"

namespace model
{
    namespace tpl
    {
        struct DropTpl;
    }
}

namespace base
{
    namespace lua
    {
        class LuaMessageWriter;
    }

    namespace cluster
    {
        struct NodeInfo;
    }
    class DataValue;
}

namespace engine
{
    struct WarReport;
}


namespace ms
{
    namespace map
    {
        class ActivityMgr;
        namespace info
        {
            class Kingdom;
        }

        namespace msgqueue
        {
            struct MsgPvpPlayerInfo;
        }

        namespace tpl
        {
            struct MapUnitTpl;
        }

        namespace qo
        {
            class CommandAgentLoad;
            class CommandAgentSave;
            class CommandAgentUpdate;
            class CommandUnitLoad;
            class CommandUnitSave;
            class CommandUnitUpdate;
            class CommandTroopLoad;
            class CommandTroopSave;
            class CommandTroopUpdate;
            class CommandFightHistoryLoad;
            class CommandFightHistorySave;
            class CommandFightHistoryUpdate;
            class CommandServerInfoLoad;
            class CommandPalaceWarLoad;
            class CommandTransportRecordLoad;
            class DataService;
        }
        using namespace base;
        class ArmyList;
        class Unit;
        class ResNode;
        class Castle;
        class Agent;
        class AgentProxy;
        class MapProxy;
        class Troop;
        class ArmyInfo;
        class CampTemp;
        class CampFixed;
        class FamousCity;
        class MapMailboxEventHandler;
        class Alliance;
        struct TeamInfo;
        struct BattleInfo;
        class Capital;
        class Catapult;

        // 地图
        class MapMgr : public Map
        {
        public:  
            ~MapMgr();

            static MapMgr* Create(int id);

            Castle* CreateNewCastle(Agent& agent, int level);
            Unit* CreateUnit(const tpl::MapUnitTpl* tpl, int x, int y, int id = 0);
            void CreateResource(model::MapUnitType type, int level, model::ResAreaType areaType, int unitCount);
            //创建资源
            void RefreshMonster();

            void AllianceOwnCity(int64_t allianceId, FamousCity* city);
            void AllianceLoseCity(int64_t allianceId, FamousCity* city, int atkAllianceId);
            void AllianceLoseOccupyCity(int64_t allianceId, FamousCity* city, int atkAllianceId);
            void AllianceOwnOccupyCity(int64_t allianceId, FamousCity* city);

            void AllianceCityBeAttacked(int64_t allianceId, FamousCity* city, Troop* atkTroop);
            void AllianceCastleBeAttacked(int64_t allianceId, Castle* castle, Troop* atkTroop);
            void AllianceCatapultBeAttacked(int64_t allianceId, Catapult* catapult, Troop* atkTroop);

            bool CheckSiegeCityLimit(int64_t allianceId, model::MapUnitType attackUnitType, Agent& agent);

            void ChangeKing(int64_t uid);
            bool ChangeServerName(const std::string& name);
            bool ChangeResNodeRate(int id);


            //名城转移失败联盟玩家城池
            void MoveLoseCastle(const Point& pos, const int width, const int height, const Unit* origin_unit, 
                const int allianceId);

        public:
            model::MarchErrorNo CanMarch(model::MapTroopType type, const Point& from, const Point& to, Agent& agent);
            model::MarchErrorNo March(Agent& agent, model::MapTroopType type, const Point& from, const Point& to, int teamId);

        public:
            bool IsViewingUnit(const Point& unitPos,const Point& viewPos, int range = 1);
            bool IsViewingTroop(const Point& to,const Point& from, const Point& viewPos, int range = 1);
            
            void Enter(int64_t uid, const base::cluster::MailboxID& mbid);
            void Quit(int64_t uid);
            void ViewerJoin(Agent* agent, int x, int y);
            
            void ViewerMove(Agent* agent, int x, int y);
            void SearchMap(Agent* agent, int x, int y, int type, int level);

        private:
            virtual void OnBeginSetup();
            virtual void OnBeginCleanup();

            void Setup();
            void OnNodeUp(const base::cluster::NodeInfo& node);
            void OnNodeDown(const base::cluster::NodeInfo& node);

            void FinishDbLoad();
            void InitMapUnits();

            void AddUnitFromDb(int id, int x, int y, const std::string& data, const tpl::MapUnitTpl* tpl);
            void AddTroopFromDb(int id, const std::string& data);

            void OnIntervalUpdate();
            void OnHour(int hour);
            bool CheckTotalPower();
            void UpdateCastleAvgLevel();
            // 200毫秒 update所有行军
            void OnUpdate();

        private:
            MapMgr(int id);

            base::AutoObserver m_autoObserver;

            friend class MapMailboxEventHandler;
            friend class Agent;
            friend class AgentProxy;
            friend class Unit;
            friend class ResNode;
            friend class Troop;
            friend class MapProxy;
            friend class qo::CommandAgentLoad;
            friend class qo::CommandAgentSave;
            friend class qo::CommandAgentUpdate;
            friend class qo::CommandUnitLoad;
            friend class qo::CommandUnitSave;
            friend class qo::CommandUnitUpdate;
            friend class qo::CommandTroopLoad;
            friend class qo::CommandTroopSave;
            friend class qo::CommandTroopUpdate;
            friend class qo::CommandFightHistoryLoad;
            friend class qo::CommandFightHistorySave;
            friend class qo::CommandFightHistoryUpdate;
            friend class qo::CommandServerInfoLoad;
            friend class qo::CommandPalaceWarLoad;
            friend class qo::DataService;
            friend class qo::CommandTransportRecordLoad;
        };

        extern MapMgr* g_mapMgr;
    }
}

#endif // MAP_MAP_MGR_H

