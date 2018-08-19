#ifndef MAP_MAP_H
#define MAP_MAP_H
#include <base/cluster/mailbox.h>
#include <base/command/runner.h>
#include <base/observer.h>
#include <base/bootable.h>
#include <lua/llimits.h>
#include "base/event.h"
#include <base/lua/luamessage.h>
#include <model/metadata.h>
#include "area.h"
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
        class MapMgr;
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

        struct MapGlobalData {
            int castleCnt = 0;                              // 城池数量
            int killNpcCnt = 0;                             // KillNPC数量
            int occupyResCnt = 0;                      // 占领资源地数量
            base::DataTable occupyRes;
            int occupyCapitalCnt = 0;
            int occupyChowCnt = 0;
            int occupyPrefectureCnt = 0;
            int occupyCountyCnt = 0;
            int64_t totalPower = 0;                             // 总战力
            int gatherCnt = 0;                              // 采集总量
            base::DataTable heroCnt;
            base::DataTable monsterDist;               //野怪分布数据
            
            bool isDirty = false;
        };

        // 地图
        class Map : public base::Bootable
        {
        public:
            static constexpr int MAP_WIDTH = 1200;
            static constexpr int MAP_HEIGHT = 1200;
            static constexpr int BLOCK_SIZE = 13;

            static constexpr int AREA_WIDTH = 5;
            static constexpr int AREA_HEIGHT = 5;
            static constexpr int AREA_MAX_X = (int)ceil((float)MAP_WIDTH / AREA_WIDTH) - 1;
            static constexpr int AREA_MAX_Y = (int)ceil((float)MAP_HEIGHT / AREA_HEIGHT) - 1;

            static constexpr int MAX_RANGE = 3;
            static constexpr int PALACE_MIN = 595;
            static constexpr int PALACE_MAX = 606;

            //出征事件
            base::Event<void(Troop*, MapTroopEvent)> evt_troop;
            // Unit事件
            base::Event<void(Unit*, MapUnitEvent)> evt_unit;

            Map(int id);
            virtual ~Map();

            std::string name()
            {
                return m_name;
            }

            base::memory::MemoryPool& mempool();

            Capital& capital() {
                return *m_capital;
            }

            const Agent* king() const {
                return m_king;
            }

            const std::vector<Catapult*> catapults() const {
                return m_catapults;
            }

            const std::unordered_map<int64_t, Agent*>& agents() const {
                return m_agents;
            }

            const std::vector<Unit*>& cities() {
                return m_cities;
            }

            Alliance& alliance() const {
                return *m_alliance;
            }

            int localServiceId() const {
                return m_localServiceId;
            }

            int64_t openServerTime() const {
                return m_openServerTime;
            }

            int64_t palaceWarTime() const {
                return m_palaceWarTime;
            }

            const std::string& serverName() const {
                return m_serverName;
            }

            bool canChangeServerName() const {
                return m_canChangeServerName;
            }

            int resourceNodeRateId() const {
                return m_resourceNodeRateId;
            }

            int lastChangeResIdTime() const {
                return m_lastChangeResIdTime;
            }

            int castleLevelAvg() const {
                return m_castleLevelAvg;
            }
            
            MapGlobalData& globalData()
            {
                return m_globalData;
            }

            bool isLocalUid(int64_t uid) const;

            std::tuple<int, int, int, int> OccupyCityCnt();

            MapProxy* proxy()
            {
                return m_mapProxy;
            }

        public:
            
            int GetUnitTplid(const Point& pos);
            Unit* FindUnit(const Point& pos) {
                return CheckPos(pos) ? m_units[pos.x][pos.y] : nullptr;
            }

            std::vector<Unit*> FetchUnitsBy2Radius(const Point& pos, int rOut, int rIn = 0);

            Agent* FindAgentByUID(int64_t uid) {
                auto it = m_agents.find(uid);
                return it == m_agents.end() ? nullptr : it->second;
            }

            Agent* FindAgentWaitInit(int64_t uid) {
                auto it = m_agentsWaitInit.find(uid);
                return it == m_agentsWaitInit.end() ? nullptr : it->second;
            }
            Agent* FindVisitorByUID(int64_t uid) {
                auto it = m_agentsVisitor.find(uid);
                return it == m_agentsVisitor.end() ? nullptr : it->second;
            }

            Troop* FindTroop(int troopId) {
                auto it = m_troops.find(troopId);
                return it == m_troops.end() ? nullptr : it->second;
            }

            void FindMatchUnit(std::vector<Unit*>& units, const std::function<bool(Unit*)>& matcher);

            int GetHighestMonsterLv();
//             Castle* FindCastleByUid(int64_t uid);
            Castle* FindCastleByPos(const Point& pos);
            Unit* FindNearestUnit(const Point& pos, const std::function<bool(Unit*)>& matcher);
            Unit* FindNearestUnit(const Point& pos, const int allianceId,  const std::function<bool(Unit*, int)>& matcher,
                bool skipSelf);
            std::vector<Unit*> GetUnitsInRect(const Point& start, const Point& end, model::MapUnitType type);
            std::vector<Unit*> GetUnitsInRect(const Point& start, const Point& end);
            std::vector<Unit*> GetUnitsInRect(const Point& start, const Point& end, const int allianceId,
                const std::function<bool(Unit*, int)>& matcher);

            short GetBlockByPoint(const Point &pos);
            // 获取随机位置(不一定可用)
            Point GetRandomPoint();
            // 通过出生点 获取一个附近的随机点 且不在原来的灯塔内(默认九宫格)
            // 若找不到合适的随机点 则返回nowPos
            Point GetRandomPoint(const Point& bornPos, const Point& nowPos, int range = 1);
            bool RandomMoveInWholeMap(Unit* unit);
            
            void VisitorJoin(Agent* agent);
            void VisitorLeave(int64_t uid);

            void OnAgentInitFinished(int64_t uid);
            void RebuildCastle(Castle* castle);
            CampTemp* CreateCampTemp(int x, int y, Troop* troop);
            CampFixed* CreateCampFixed(int x, int y, Troop* troop);

            bool Teleport(Unit* unit, int x, int y);

            void SetMaxReportId(int id)
            {
                m_max_report = id;
            }

            int GetNextReportId()
            {
                return ++m_max_report;
            }

        public:
            /*** evt from fs ***/
            void OnCastleLevelUp(Castle* castle, int level);

            // winTeam 1 red, 2 blue
            // battleType 1 SIEGE, 2 DECLARE
            void OnBattleFinish(const msgqueue::MsgPvpPlayerInfo& attacker, const msgqueue::MsgPvpPlayerInfo& defender, model::AttackType winner, model::MapTroopType troopType);
            //广播附近viewer怪物攻城
            void OnMonsterSiege(Castle* castle, int level);
            bool OnTransport(Agent* agent, Agent* toAgent, model::MapTroopType type, const Point& from, const Point& to, base::DataTable& resourceParams);
            //王城争霸开始
            void OnPalaceWarStart();

            const std::list<FamousCity*>* AllianceCity(int64_t allianceId);
            int AllianceCityCnt(int64_t allianceId);
            
            void OnCreateCastle(Castle* castle);
            void OnKillNpc(int count);
            void OnOccupyRes(ResNode* res);
            void OnOccupyCity(FamousCity* city);
            bool OnTotalPowerChange(int64_t totalPower);
            void OnGather(int count);
            void OnAddHero(int heroId);

        public:
            //广播x成为国王
            void BroadBeKing();
            void SaveMapGlobalData();
            void UnitsForeach(const std::function<void(Unit*)>& f);

            void ViewerLeave(Agent* agent);

        private:
        
            //资源带判断
            bool InResArea(model::ResAreaType areaType, int x, int y);
            //王城范围判断
            bool InCapitalArea(const Point& p);
            //野怪密度
            bool MonsterDensity(const Point& p);

            void ReadMapData();

            bool CanPlace(const tpl::MapUnitTpl* tpl, int x, int y); // 是否可放置
            // 是否可放置(移走区域内的树)
            bool CanPlaceExcludeTree(const tpl::MapUnitTpl* tpl, int x, int y,  bool moveTree);
            void MarkPlace(const tpl::MapUnitTpl* tpl, int x, int y); // 标志放置
            void UnmarkPlace(const tpl::MapUnitTpl* tpl, int x, int y); // 不标志放置
            bool RemoveUnit(Unit* unit);
            bool MoveUnit(Unit* unit, int toX, int toY);
            void OnTreeMove(Unit* unit);
            void SetTreePos(Unit* unit);
            bool UpdateUnit(Unit* unit);
            bool UpdateResource(Unit* unit);
            bool UpdateMonster(Unit* unit);

            Troop* CreateTroop(Agent& agent, model::MapTroopType type, const Point& from, const Point& to, int teamId );

            int GenerateID() {
                return ++m_max_id;
            }
            int GenerateTroopID() {
                return ++m_max_tid;
            }
            int GenerateDynamicUnitID() {
                return ++m_max_duid;
            }
            int GenerateFightID() {
                return ++m_max_fight_id;
            }
            int GenerateTransportRecordID() {
                return ++m_max_transport_record_id;
            }

            /***AOI***/
            inline bool is_in_rect(const Point& pos, const Point& start, const Point& end) {
                return is_in_rect(pos.x, pos.y, start, end);
            }
            inline bool is_in_rect(const FloatPoint& pos, const Point& start, const Point& end) {
                return (pos.x >= start.x && pos.x <= end.x && pos.y >= start.y && pos.y <= end.y);
            }
            inline bool is_in_rect(int x, int y, const Point& start, const Point& end) {
                return (x >= start.x && x <= end.x && y >= start.y && y <= end.y);
            }
            inline bool is_in_palace(const Point& pos) {
                return is_in_palace(pos.x, pos.y);
            }
            inline bool is_in_palace(int x, int y) {
                return is_in_rect(x, y, {PALACE_MIN, PALACE_MIN}, {PALACE_MAX, PALACE_MAX});
            }

            bool CheckPos(int x, int y);
            bool CheckPos(const Point& pos) {
                return CheckPos(pos.x, pos.y);
            }
            //Trans the absolut pos to tower pos.
            Point TransPos(int x, int y);
            Point TransPos(const Point& pos) {
                return TransPos(pos.x, pos.y);
            }

            //Get the postion limit of given range. (x为tower x, y为tower y, range是以灯塔(即area)为最小单位)
            PosLimit GetAreaPosLimit(const Point& pos, int range = 1);
            // Get the postion as the center pos 
            PosLimit GetCenterPosLimit(const Point& pos, const tpl::MapUnitTpl* tpl, int range);
            std::vector<Unit*> GetUnitsByPos(int x, int y, int range = 1);

            /***TROOP 行军相关***/
            inline float get_distance(const Point& a, const Point& b) {
                return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
            }
            inline float get_distance(const FloatPoint& a, const Point& b) {
                return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
            }

        private:
            void OnUnitAdd(Unit* unit);
            void OnUnitRemove(Unit* unit);
            void OnUnitUpdate(Unit* unit);
            void OnTroopAdd(Troop* tp);
            void OnTroopUpdate(Troop* tp, bool resetCross = false);
            void OnTroopRemove(Troop* tp);
            void OnViewerAdd(Agent* agent, const Point& pos, int range = 1);
            void OnViewerRemove(Agent* agent, const Point& pos, int range = 1);
            bool OnViewerMove(Agent* agent, const Point& oldPos, const Point& newPos, int oldRange = 1, int newRange = 1);
            void OnSearchMap(Agent* agent, const Point& CastlePos, int type, int level, int range = 1);

        private:
            bool m_isSetupFinish = false;
            base::command::Runner m_runner;
            int64_t m_lastUpdateTick = 0;
            base::cluster::MailboxID m_csMbid;

            std::string m_name;
            int m_localServiceId = 0;
            std::vector<int> m_merged_ids;
            int64_t m_openServerTime = 0;
            int64_t m_palaceWarTime = 0; // 王城争霸开始时间戳配置
            
            bool m_isDbLoad = false;
            Area m_areas[AREA_MAX_X + 1][AREA_MAX_Y + 1];
            int m_max_id = 0;  //unit id
            int m_max_tid = 0;  //troop id
            int m_max_duid = 200000000;  //dynamic unit id, 适用与不需要保存到数据库的单元
            int m_max_fight_id = 0;
            int m_max_transport_record_id = 0;
            Unit* m_units[MAP_WIDTH][MAP_HEIGHT] = {};
            bool m_walkable[MAP_WIDTH][MAP_HEIGHT] = {};
            //阻挡区域
            //bool m_invalidPoint[MAP_WIDTH][MAP_HEIGHT] = {{false}};
            Point m_trees[MAP_WIDTH][MAP_HEIGHT];           //= {{Point(-1, -1)}};
            std::list<Unit*> m_unitsToDelete;

            std::vector<std::set<Point>> m_blockPoints;
            std::map<std::tuple<short, short>, short> m_pointBlock;

            std::string m_serverName;
            bool m_canChangeServerName = false;
            int m_resourceNodeRateId = 0;       //资源点产出率ID
            int64_t m_lastChangeResIdTime = 0;
            Agent* m_king = nullptr;            //国王

            Capital* m_capital = nullptr;
            std::vector<Catapult*> m_catapults;

            time_t m_lastUpdateSecond = 0;
            size_t m_updateSeconds = 0;
            size_t m_refreshIndex = 0;
            std::vector<Unit*> m_refreshUnits;          //只定时刷新系统普通怪物和资源(不会DELETE)
            std::vector<Unit*> m_cities;          //名城，定时刷新城中NPC
            std::list<Unit*> m_campFixed;        //大地图所有行营，定时刷新成
            
            int m_castleLevelAvg = 8;

            int m_count = 0; //计算资源单位 

            int m_max_report = 0; //当前战报的最大值

            int m_mapSearchTimeStamp = 0;  //地图搜索时间戳
            int m_mapSearchTime = 0;   //地图搜索次数
            int m_mapSearchDis = 0;      //上一次搜索的距离，下一次搜索就比上一次的距离长

            PosLimit m_capital_poslimit;

            std::unordered_map<int64_t, Agent*> m_agents;
            std::unordered_map<int64_t, Agent*> m_agentsWaitInit;
            std::unordered_map<int64_t, Agent*> m_agentsVisitor;
            std::unordered_map<int, Troop*> m_troops;
            std::unordered_map<int, Troop*> m_troopsRemoved;
            
            Alliance* m_alliance = nullptr;
//             MonsterSiege* m_monsterSiege = nullptr;
            std::unordered_map<int, BattleInfo*> m_battleInfoList;
//             CrossTeleportMgr* m_crossTeleportMgr = nullptr;
            ActivityMgr* m_activityMgr = nullptr;
            // <allianceId, cityCnt>
            std::map<int64_t, std::list<FamousCity*>> m_allianceCity;

            MapGlobalData m_globalData;
            MapProxy* m_mapProxy;

            friend class MapMailboxEventHandler;
            friend class Agent;
            friend class AgentProxy;
            friend class Unit;
            friend class ResNode;
            friend class Troop;
            friend class MapProxy;
            friend class MapMgr;
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

    }
}

#endif // MAP_H

