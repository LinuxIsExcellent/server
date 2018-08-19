#ifndef MAP_AGENT_H
#define MAP_AGENT_H
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
        class AgentProxy;

        // 用户代理
        class Agent : public model::Dirty
        {
        public:
            Agent(int64_t uid);
            ~Agent();

            std::string Serialize() const;
            bool Deserialize(std::string data);
            void FinishDeserialize();
            
            int64_t uid() const {
                return m_uid;
            }

            int sid() const {
                return m_uid > 0 ? (m_uid >> 32) : 0;
            }

            bool isLocalPlayer() const;
            bool isKing() const;

            bool isPlayer() const {
                return m_uid > 0;
            }

            bool isOnline() const {
                return m_mbid;
            }

            const std::string& nickname() const {
                return m_nickname;
            }

            int64_t headId() const {
                return m_headId;
            }

            model::LangType langType() const {
                return m_langType;
            }

            int level() const {
                return m_level;
            }
            
            int64_t allianceId() const;
            const AllianceSimple* alliance()
            {
                return m_alliance;
            }

            const std::string& allianceNickname() const;

            const std::string& allianceName() const;

            int allianceLevel() const;

            int allianceBannerId() const;

            const Point& viewPoint() const {
                return m_viewPoint;
            }

            bool isViewing() const {
                return m_viewPoint.x >= 0 && m_viewPoint.y >= 0;
            }

            Castle* castle() const {
                return m_castle;
            }

            const Point& castlePos() const {
                return m_castlePos;
            }

            void SetCastlePos(const Point& pos) {
                m_castlePos = pos;
            }

            int castleLevel() const;

            ArmyList* defArmyList();

            int getDefTeam() const;

            ArmyList* TeamArmyList(int teamId) {
                if (teamId >= 1 && teamId <=  (int)m_arrArmyList.size()) {
                    return &m_arrArmyList[teamId - 1];
                }
                return nullptr;
            }

            std::array<ArmyList, 5>& armyListArray(){
                return m_arrArmyList;
            }

            TrapSet& trapSet() {
                return m_trapSets;
            }

            ArmySet& armySet() {
                return m_armySet;
            }

            const std::unordered_map<int, Troop*>& troops() const {
                return m_troops;
            }

            int TroopCount();

            int TroopCountByType(model::MapTroopType type);

            int watchtowerLevel() {
                return m_watchtowerLevel;
            }

            int turretLevel() {
                return m_turretLevel;
            }

            int food() const {
                return m_food;
            }

            int wood() const {
                return m_wood;
            }

            int iron() const {
                return m_iron;
            }

            int stone() const {
                return m_stone;
            }

            int gold() const {
                return m_gold;
            }

            int totalPower() const {
                return m_totalPower;
            }

            int MaxMarchingQueue() {
                return m_property.maxMarchingQueue;
            }

            int allianceReinforcementNum() {
                return m_property.allianceReinforcementNum;
            }

            const std::unordered_map<int, info::BuffInfo>& cityBuffs() const {
                return m_cityBuffs;
            }

            const std::unordered_map<int, info::BuffInfo>& skillBuffs() const {
                return m_skillBuffs;
            }

            const info::Property& property() const {
                return m_property;
            }

            const info::CityPatrol& cityPatrol() const {
                return m_cityPatrol;
            }

            const std::unordered_map<int, int>& skills() const {
                return m_skills;
            }

            const std::unordered_map<int, int>& technologies() const {
                return m_technologies;
            }

            const base::cluster::MailboxID& mbid() const {
                return m_mbid;
            }

            msgqueue::MsgQueue& msgQueue() {
                return m_msgQueue;
            }

            const std::vector<info::CollectInfo>& collectInfos() const {
                return m_collectInfos;
            }

            bool useSkillHelp() const {
                return m_useSkillHelp;
            }

            int64_t registerTimestamp() const {
                return m_registerTimestamp;
            }

            int titleId() const {
                return m_titleId;
            }

            bool equipView() const {
                return m_equipView;
            }

            int vipLevel() const {
                return m_vip.level;
            }

            bool isVip() const;
            
            int allianceCdTimestamp() const {
                return m_allianceCdTimestamp;
            }

            void SetUseSkillHelp(bool v) {
                m_useSkillHelp = v;
            }

            void Connect(const base::cluster::MailboxID& mbid) {
                m_mbid = mbid;
                AttachTroopEvent();
                AttachUnitEvent();  
                if (isLocalPlayer()) {
                    m_msgQueue.Init(this);
                    m_msgQueue.DbLoad();
                }
            }

            void Disconnect() {
                m_mbid.Clear();
                DetachTroopEvent();
                DetachUnitEvent();
                if (isLocalPlayer()) {
                    m_msgQueue.DbSave();
                }
                m_tidCache.clear();
                m_unitIdCache.clear();
            }

            void SetViewPoint(int x, int y) {
                m_viewPoint.x = x;
                m_viewPoint.y = y;
            }

            void SetHeadId(int64_t headId) {
                m_headId = headId;
            }

            void SetNickname(const std::string& nickname) {
                m_nickname = nickname;
            }

            void RemoveTroop(int id) {
                m_troops.erase(id);
            }

            void UpdateTitleId(int titleId);
            void AddResource(const std::vector<model::tpl::DropItem>& drops);

            // 是否活跃用户
            bool IsActiveUser() const;
            //获取城池资源点的总量
            int GetCollectTotal(model::ResourceType type);
            //城池资源被抢夺
            int ResourceBePlundered(model::ResourceType type, int innerResource,  int outerResource );
            //获取采集产量
            void GetCollectOutput(int& food, int& wood, int& iron, int& stone);
            //检查BUFF是否有效
            bool CheckBuffIsValid(model::BuffType type, bool isCityBuff);
            // 是否开启了伪装术
            bool IsFalseArmy() const;
            // 获取有效buff(包括cityBuff = 1和skillBuff = 2, all = 3)
            std::unordered_map<int, info::BuffInfo> GetValidBuffs(int buffType, bool isWatchTower = false) const;

            int CampFixedCount() {
                return m_campFixed.size();
            }

            AgentProxy* proxy() {
                return m_agentProxy;
            }

        public:
            void OnHour(int hour);

            // 部队加成
            void GetArmyAddition(std::vector<std::tuple<int, int, int>>& adds) const;

        public:
            // 城池被攻击后 同步一些数据
            void AfterCastleBeAttacked();
//             // 行军士兵死亡
//             void OnTroopArmyDie(Troop* troop);
            // 行军回城
            void OnTroopArriveCastle(Troop* troop);
            // 行军回行营
            void OnTroopArriveCampFixed(Troop* troop);
//             // 怪物战结束
//             void OnMonsterBattleEnd(model::AttackType winner);
//             // 资源战结束
//             void OnResourceBattleEnd(model::AttackType winner, model::AttackType myAttackType);
//             // 攻城战结束
//             void OnCastleBattleEnd(model::AttackType winner, model::AttackType myAttackType);
//             // 营地战结束
//             void OnCampBattleEnd(model::AttackType winner, model::AttackType myAttackType);
            // 侦查
            void OnScout(model::AttackType winner, model::AttackType myAttackType, const std::vector<model::WATCHTOWER_SCOUT_TYPE>& scoutTypes, const msgqueue::MsgPlayerInfo& attacker, const msgqueue::MsgScoutDefenderInfo& defender);
            // 主动解除战争保护状态
            void OnRemovePeaceShield();
            // 资源援助
            void OnResourceHelp(Troop& troop, const Agent& recipient);
            // 被资源帮助
            void OnResouceHelped(Troop& troop);
            // 援兵
            void OnReinforcements(Troop& troop, const Agent& recipient);
            // 联盟信息更新
            void OnAllianceInfoUpdate(const AllianceSimple* allianceInfo, bool needBroadTroops = false);
            // 联盟信息重置
            void OnAllianceInfoReset(const AllianceSimple* oldAlliance);
            // 失去名城
            void OnAllianceLoseCity(int id);

            // 联盟拥有了萌城
            void OnOwnNeutralCastle();
            // 联盟失去了萌城
            void OnLossNeutralCastle(const AllianceSimple* occupier);
            // 联盟摧毁了别人的萌城
            void OnDestoryNeutralCastle();
            // 更新战斗记录
            void OnBattleHistoryUpdate(BattleInfo* info);
            // 城堡重建
            void OnCastleRebuild();

            //增加战斗统计
            void AddBattleCount(model::AttackType myAttackType, model::AttackType winner);

             // 被魔法塔攻击
            void OnBeAttackedByCatapult(const ArmyList& dieList, bool isCaptive);

            void OnPlunderCastle(int castleId);
            int GetPlunderCount(int castleId);
            int plunderTotalCount() {
                return m_plunderTotalCount;
            }

            void AddCampFixed(CampFixed *campFixed);
            void RemoveCampFixed(CampFixed *campFixed);
            void AddCampTemp(CampTemp *campTemp);
            void RemoveCampTemp(CampTemp *campTemp);
            void AddRelatedUnit(Unit *unit);
            void RemoveRelatedUnit(Unit *unit);

            void OnMarch(Troop* troop,  bool isRemarch = false);
            void ImmediateReturnAllTroop();

            inline void AddInvited(int64_t uid) {
                m_invited.emplace(uid);
            }
            
            inline void ClearInvited() {
                m_invited.clear();
            }

            void AddCityPatrolCount(int count) {
                m_cityPatrol.patrolCount +=  count;
            }

            void AddCityPatrolCD(int id, int64_t endTime) {
                auto it = m_cityPatrol.patrolCD.find(id);
                if (it !=  m_cityPatrol.patrolCD.end()) {
                    it->second = endTime;
                } else {
                    m_cityPatrol.patrolCD.emplace(id, endTime);
                }
            }
            
	    inline void setAllianceCdTimestamp(int64_t allianceCdTimestamp) {
                m_allianceCdTimestamp = allianceCdTimestamp;
            }

            //alliance buff
            void AllianceBuffOpen(const info::AllianceBuffInfo& buffInfo);
            void AllianceBuffClosed(model::AllianceBuffType buffType);
            void DisableAllianceBuff(model::AllianceBuffType buffType);
            const info::AllianceBuffInfo* GetValidAllianceBuff(model::AllianceBuffType buffType);


            const int GetMapSearchTimeStamp()
            {
                return m_mapSearchTimeStamp;
            }

            void SetMapSearchTimeStamp(int timeStamp)
            {
                m_mapSearchTimeStamp = timeStamp;
            }

            const int GetMapSearchTime()
            {
                return m_mapSearchTime;
            }

            void SetMapSearchTime(int time)
            {
                m_mapSearchTime = time;
            }

            const int GetMapSearchDis()
            {
                return m_mapSearchDis;
            }

            void SetMapSearchDis(int dis)
            {
                m_mapSearchDis = dis;
            }

            const int GetMapSearchType()
            {
                return m_mapSearchType;
            }

            void SetMapSearchType(int type)
            {
                m_mapSearchType = type;
            }

            const int GetMapSearchLevel()
            {
                return m_mapSearchLevel;
            }

            void SetMapSearchLevel(int level)
            {
                m_mapSearchLevel = level;
            }

            void ReSetMapSearch();

            void SetMonsterDefeatedLevel(int level);
          

            const int GetMonsterDefeatedLevel()
            {
                return m_monsterDefeatedLevel;
            }

        private:
            Troop* FindTroop(int troopId) {
                auto it = m_troops.find(troopId);
                return it != m_troops.end() ? it->second : nullptr;
            }

            bool IsMarchingHero(int heroId);

            bool HadInvited(int64_t uid) {
                return m_invited.find(uid) != m_invited.end();
            }
            
            // pos是否自己的所在领地
            bool IsMyPos(const Point& pos);
            // 发送所有与自己相关的行军给自己
            void SendRelatedTroops();
            void SendRelatedUnits();
            
            bool NeedSend(Troop* troop);
            void AttachTroopEvent();
            void DetachTroopEvent();

            bool NeedSend(Unit* unit);
            void AttachUnitEvent();
            void DetachUnitEvent();

            void CheckResetTimer();

        private:
            int64_t m_uid = 0;
            base::cluster::MailboxID m_mbid;
            msgqueue::MsgQueue m_msgQueue;
            base::command::Runner m_runner;
            Point m_viewPoint;
            Castle* m_castle = nullptr;
            Point m_castlePos;
            std::unordered_map<int, Troop*> m_troops; // troopid, troop
            
            std::list<Unit*> m_campFixed;                    // 所有行营
            std::list<Unit*> m_campTemp;                    // 所有驻扎
            std::list<Unit*> m_relatedUnit;                // 相关的unit, 如名城，采集

            int64_t m_headId = 0;
            std::string m_nickname;
            int m_level = 0;
            int m_exp = 0;
            model::LangType m_langType;

            int m_mapSearchTimeStamp = 0;  //地图搜索时间戳
            int m_mapSearchTime = 0;   //地图搜索次数
            int m_mapSearchDis = 0;      //上一次搜索的距离，下一次搜索就比上一次的距离长

            int m_mapSearchType = 0;       // 上一次搜索的类型
            int m_mapSearchLevel = 0;         // 上一次搜索的等级
            
            std::map<int, std::shared_ptr<HeroInfo>> m_heroes;
            std::array<ArmyList, 5> m_arrArmyList;
            TrapSet m_trapSets;                            // 陷阱
            ArmySet m_armySet;                // 城内的兵
            ArmyList m_defArmyList;                     // 守城部队
            
            int m_castleDefenderId = 1;              // 守城编组Id
            int m_monsterDefeatedLevel = 0;          // 击败过的野怪的最高等级

            //resource
            int m_gold = 0;
            int m_food = 0;
            int m_wood = 0;
            int m_iron = 0;
            int m_stone = 0;
            //lordInfo
            int m_attackWins = 0;
            int m_attackLosses = 0;
            int m_defenseWins = 0;
            int m_defenseLosses = 0;
            int m_scoutCount = 0;
            int m_kills = 0;
            int m_losses = 0;
            int m_heals = 0;
            //power
            int m_lordPower = 0;
            int m_troopPower = 0;
            int m_buildingPower = 0;
            int m_sciencePower = 0;
            int m_trapPower = 0;
            int m_heroPower = 0;
            int m_totalPower = 0;

            int m_watchtowerLevel = 0;
            int m_turretLevel = 0;

            int64_t m_lastLoginTimestamp = 0;
            int64_t m_registerTimestamp = 0;
            int64_t m_allianceCdTimestamp = 0;
            bool m_useSkillHelp = false;  //是否拥有救援技能效果
            int m_titleId = 0;          //称号(官职)
            // player sets
            bool m_equipView = false;   // 装备他人可见
            

            //collectInfo
            std::vector<info::CollectInfo> m_collectInfos;
            //medal
            std::vector<int> m_medals;
            //buffs
            std::unordered_map<int, info::BuffInfo> m_cityBuffs;
            std::unordered_map<int, info::BuffInfo> m_skillBuffs;
            //technologies 领主科技 tplid, level
            std::unordered_map<int, int> m_technologies;
            //skills 领主天赋 tplid, level
            std::unordered_map<int, int> m_skills;
            // vip info
            info::VipInfo m_vip;
            //aliance buff
            std::map<model::AllianceBuffType, int64_t> m_allianceInvalidBuffs;

            // 掠夺玩家城池次数
            std::map<int,  int> m_plunderCastleCount;
            int m_plunderTotalCount = 0;
            int64_t m_plunderResetTime = 0;

            //property
            info::Property m_property;

            const AllianceSimple* m_alliance = nullptr;
            const std::string m_nullstr = "";
            std::set<int64_t> m_invited; //已经邀请过进联盟的人

            info::CityPatrol m_cityPatrol;
            int64_t m_cityPatrolResetTime = 0;


            AgentProxy* m_agentProxy;

            std::set<int> m_tidCache;         
            std::set<int> m_unitIdCache;
            base::AutoObserver m_troopObserver;
            base::AutoObserver m_unitObserver;
            friend class Map;
            friend class MapProxy;
            friend class AgentProxy;
        };
    }
}

#endif // AGENT_H

