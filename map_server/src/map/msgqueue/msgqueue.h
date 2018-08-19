#ifndef MAP_MSGQUEUE_MSGQUEUE_H
#define MAP_MSGQUEUE_MSGQUEUE_H

#include <base/command/runner.h>
#include <model/metadata.h>
#include <list>
#include <functional>
#include <string>
#include "msgrecord.h"
#include "../info/dbinfo.h"
#include "../info/agentinfo.h"

namespace ms
{
    namespace map
    {
        class Agent;
        class AllianceSimple;

        namespace qo
        {
            class CommandMsgQueueSave;
            class CommandMsgQueueDelete;
            class CommandMsgQueueUpdate;
            class CommandMsgQueueLoad;
            class DataService;
        }

        namespace msgqueue
        {
            class MsgQueue
            {
            public:
                MsgQueue();
                ~MsgQueue();

                bool Empty();

                void Init(Agent* agent);

                void DbLoad();
                void DbSave();
                void ConfirmMsg(int msgId);

                // 出征
                void AppendMsgMarch(model::MapTroopType troopType, const Point& toPos, const ArmyList* armyList, const int troopId);
                // 出征返回
                void AppendMsgMarchBack(bool isArriveCastle,model::MapTroopType troopType, const Point& toPos, int food, int wood, int iron, int stone, const ArmyList* armyList);
                // 出征到达失效
                void AppendMsgTroopReachInvalid(model::MapTroopType troopType);
                // 攻击怪物失效
                void AppendMsgAttackMonsterInvalid();
                // 创建行营失败
                void AppendMsgCreateCampFixedFailed();
                // 资源采集
                void AppendMsgGatherResource(const Point& toPos, int resTplId, const std::vector<model::tpl::DropItem>& dropItems,  int gatherRemain);
                // 攻击怪物
                void AppendMsgAttackMonster(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, 
                    const MsgPvpPlayerInfo& defender, const Point& unitPos, int tplId, int monsterHpBegin, int monsterHpEnd, 
                    const std::vector<model::tpl::DropItem>& dropItems, const ArmyList& armyList, bool isCaptive, int unitId, 
                    int troopId, int reportId, const MsgReportInfo& reportInfo);
                // 攻击名城
                void AppendMsgAttackCity(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, 
                    const MsgPvpPlayerInfo& defender, int reportId, const Point& unitPos, int tplId, 
                    const std::vector<model::tpl::DropItem>& dropItems, int unitId, int troopId, const MsgReportInfo& reportInfo);
                // 攻击玩家城池
                void AppendMsgAttackCastle(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, 
                    const MsgPvpPlayerInfo& defender, int reportId, const Point& unitPos, int tplId, int unitId, int troopId, int armyHurt, 
                    int food, int wood, int iron, int stone, int foodRemove, int woodRemove, int ironRemove, int stoneRemove, 
                    const std::vector<info::CollectInfo>& collectInfos, int64_t burnEndTs, const MsgReportInfo& reportInfo);
                // 攻击玩家城池结果 击败/失败
                void AppendMsgBeatCastle(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int unitId, int troopId, int food, int wood, int iron, int stone,
                  int foodRemove, int woodRemove, int ironRemove, int stoneRemove, const std::vector<info::CollectInfo>& collectInfos, int64_t burnEndTs);
                // 攻击资源点战
                void AppendMsgAttackResource(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, 
                    const MsgPvpPlayerInfo& defender, int reportId, const Point& unitPos, int tplId, 
                    const std::vector<model::tpl::DropItem>& dropItems, int unitId, int troopId, const MsgReportInfo& reportInfo);
                // 攻击行营
                void AppendMsgAttackCampFixed(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, 
                    const MsgPvpPlayerInfo& defender, int reportId, const Point& unitPos, int tplId, int unitId, int troopId, const MsgReportInfo& reportInfo);
                // 占领行营
                void AppendMsgOccupyCampFixed(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, 
                    const MsgPvpPlayerInfo& defender, int reportId, const Point& unitPos, int tplId, int unitId, int troopId, const MsgReportInfo& reportInfo);
                // 攻击驻扎
                void AppendMsgAttackCampTemp(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, 
                    const MsgPvpPlayerInfo& defender, int reportId, const Point& unitPos, int tplId, int unitId, int troopId, const MsgReportInfo& reportInfo);
                // 攻击王城战
                void AppendMsgAttackPalace(model::AttackType winner, model::AttackType myAttackType, 
                    const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int unitId, int troopId, 
                    const MsgPvpDragonInfo& dragon);
                // 侦查
                void AppendMsgScout(model::AttackType winner, model::AttackType myAttackType, const std::vector<model::WATCHTOWER_SCOUT_TYPE>& scoutTypes, const MsgPlayerInfo& attacker, const MsgScoutDefenderInfo& defender);
                // buff移除
                void AppendMsgBuffRemove(model::BuffType type);
                // 资源援助
                void AppendMsgResourceHelp(const MsgPlayerInfo& player, model::AttackType myAttackType, int food, int wood, int iron, int stone);
                // 援兵
                void AppendMsgReinforcements(const MsgPlayerInfo& player, model::AttackType myAttackType, const ArmyList& armyList);
                // 攻击中立城池
                void AppendMsgAttackNeutralCastle(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int unitId, int troopId);
                // 攻击炮塔
                void AppendMsgAttackCatapult(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, const std::vector<model::tpl::DropItem>& dropItems, int reportId, int unitId, int troopId, const MsgReportInfo& reportInfo);
                // 攻击地精营地
                void AppendMsgAttackGoblinCamp(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int unitId, int troopId);
                // 中立城池行为邮件
                void AppendMsgNeutralCastleNoticeMail(model::MailSubType mailSubType, const AllianceSimple* alliance, const AllianceSimple* occupier = nullptr);
                // 怪物攻城
                void AppendMsgMonsterSiege(int level, int food, int wood, int iron, int stone, int foodRemove, int woodRemove, int ironRemove, int stoneRemove, const std::vector<info::CollectInfo>& collectInfos);
                // 怪物攻城资源抢回
                void AppendMsgMonsterSiegeResourceGetBack(int level, int food, int wood, int iron, int stone);
                // 被魔法塔攻击
                void AppendMsgBeAttackedByCatapult(const ArmyList& dieList, bool isCaptive);
                // 城堡重建
                void AppendMsgCastleRebuild(Point castlePos);
                // 杀龙排行
                void AppendMsgKillDragonRank(int rank);
                // 杀龙最后一击
                void AppendMsgKillDragonLastAttack();
                // 探索神秘城市
                void AppendMsgExploreMysteriousCity(const std::vector<model::tpl::DropItem>& drops);
                // 城防值更新
                void AppendMsgCityDefenseUpdate(int cityDefense);
                // 巡逻
                void AppendMsgCityPatrol(const MsgCityInfo& cityInfo, const std::vector<MsgCityPatrolEvent> &events);
                // 攻击世界BOSS
                void AppendMsgAttackWorldBoss(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int reportId, int unitId, int troopId, const MsgReportInfo& reportInfo);
                // 攻击世界BOSS结算
                void AppendMsgAttackWorldBossEnd(const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, const MsgWorldBossInfo& WorldBoss, int unitId, int troopId);
                // 攻击名城NPC结算
                void AppendMsgAttachCityEnd(const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, const MsgWorldBossInfo& WorldBoss, const std::vector<model::tpl::DropItem>& dropItems,  const ArmyList& armyList, int unitId, int troopId);
                // 攻击箭塔NPC结算
                void AppendMsgAttachCatapultEnd(const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, const MsgWorldBossInfo& WorldBoss, int unitId, int troopId);
                // 运输资源
                void AppendMsgTransport(model::TransportType myTransportType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int food, int wood, int iron, int stone, bool isSuccess);
                // 战斗伤病补偿
                void AppendMsgCompensate(int food, int wood, int stone, int iron);
                // 城墙驻军补充兵力
                void AppendMsgCityDefenerFill(const ArmyList* defList);

            private:
                int GenerateID() {
                    return ++m_msg_id;
                }

                void SendMsg();

                void AppendMsg(MsgRecord* msg);

                void LoadMsgFromDb(info::DbMsgQueueInfo& info);

            private:
                map::Agent* m_agent = nullptr;
                base::command::Runner m_runner;
                base::AutoObserver m_autoObserver;
                std::list<MsgRecord*> m_waiting_records;
                std::list<MsgRecord*> m_sent_records;
                int m_msg_id = 0;
                bool m_stop = false;

                friend class map::Agent;
                friend class map::qo::CommandMsgQueueSave;
                friend class map::qo::CommandMsgQueueDelete;
                friend class map::qo::CommandMsgQueueUpdate;
                friend class map::qo::CommandMsgQueueLoad;
                friend class map::qo::DataService;
            };
        }
    }
}

#endif // MSGQUEUE_H
