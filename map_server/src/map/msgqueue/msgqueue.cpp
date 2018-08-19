#include "msgqueue.h"
#include <model/rpc/map.h>
#include <base/logger.h>
#include <base/dbo/connection.h>
#include <base/3rd/rapidjson/rapidjson.h>
#include "../agent.h"
#include "../map.h"
#include "../alliance.h"
#include "../qo/commandmsgqueueload.h"
#include "../qo/commandmsgqueuesave.h"
#include "../qo/dataservice.h"

namespace ms
{
    namespace map
    {
        namespace msgqueue
        {
            using namespace std;
            using namespace base::dbo;

            MsgQueue::MsgQueue()
            {
            }

            MsgQueue::~MsgQueue()
            {
                m_agent = nullptr;

                auto it = m_sent_records.begin();
                while (it != m_sent_records.end()) {
                    SAFE_DELETE(*it);
                    ++it;
                }
                it = m_waiting_records.begin();
                while (it != m_waiting_records.end()) {
                    SAFE_DELETE(*it);
                    ++it;
                }
            }

            bool MsgQueue::Empty()
            {
                return m_sent_records.empty() && m_waiting_records.empty();
            }

            void MsgQueue::Init(Agent* agent)
            {
                if (m_agent == nullptr && agent != nullptr) {
                    m_agent = agent;
                    //g_dispatcher->quicktimer().SetInterval(std::bind(&MsgQueue::SendMsg, this), 1000, m_autoObserver);
                }
            }

            void MsgQueue::ConfirmMsg(int msgId)
            {
                auto it = m_sent_records.begin();
                while (it != m_sent_records.end()) {
                    MsgRecord* msg = *it;
                    if (msg && msg->id() == msgId) {
                        it = m_sent_records.erase(it);
                        if (msg->IsFromDb()) {
                            m_runner.PushCommand(new map::qo::CommandMsgQueueDelete(msg));
                        } else {
                            SAFE_DELETE(msg);
                        }
                        break;
                    } else {
                        ++it;
                    }
                }
            }

            void MsgQueue::DbLoad()
            {
                m_stop = false;
                m_runner.PushCommand(new map::qo::CommandMsgQueueLoad(*this));
            }

            void MsgQueue::DbSave()
            {
                m_stop = true;
                qo::g_dataService->AppendMsgQueueData(*this);
            }


            void MsgQueue::LoadMsgFromDb(info::DbMsgQueueInfo& info)
            {
                if (m_agent && m_agent->uid() == info.uid) {
                    //std::cout << "msgQueue uid,type = " << info.uid << " " << (int)info.type << " " << info.data.c_str() << std::endl;

                    MsgRecord* msg = nullptr;
                    switch (info.type) {
                        case model::MessageQueueType::MARCH: {
                            msg = new MsgMarch(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::MARCH_BACK: {
                            msg = new MsgMarchBack(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::GATHER_RESOURCE: {
                            msg = new MsgGatherResource(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::ATTACK_MONSTER: {
                            msg = new MsgAttackMonster(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::ATTACK_RESOURCE: {
                            msg = new MsgAttackResource(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::ATTACK_CITY: {
                            msg = new MsgAttackCity(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::ATTACK_CASTLE: {
                            msg = new MsgAttackCastle(info.mid, info.uid);
                        }
                        break;
//                         case model::MessageQueueType::ATTACK_CAMP: {
//                             msg = new MsgAttackCamp(info.mid, info.uid);
//                         }
//                         break;
                        case model::MessageQueueType::SCOUT_RESULT: {
                            msg = new MsgScout(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::RUINS_EXPLOR: {
                            // TODO
                        }
                        break;
                        case model::MessageQueueType::BUFF_REMOVE: {
                            msg = new MsgBuffRemove(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::RESOURCE_HELP: {
                            msg = new MsgResourceHelp(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::ATTACK_PALACE: {
                            msg = new MsgAttackPalace(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::ATTACK_NEUTRAL_CASTLE: {
                            msg = new MsgAttackNeutralCastle(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::NEUTRAL_CASTLE_NOTICE_MAIL: {
                            msg = new MsgNeutralCastleNoticeMail(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::MONSTER_SIEGE: {
                            msg = new MsgMonsterSiege(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::MONSTER_SIEGE_RESOURCE_GET_BACK: {
                            msg = new MsgMonsterSiegeResourceGetBack(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::ATTACK_CATAPULT: {
                            msg = new MsgAttackCatapult(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::BE_ATTACK_BY_CATAPULT: {
                            msg = new MsgBeAttackedByCatapult(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::ATTACK_MONSTER_INVALID: {
                            msg = new MsgAttackMonsterInvalid(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::CASTLE_REBUILD: {
                            msg = new MsgCastleRebuild(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::KILL_DRAGON_RANK: {
                            msg = new MsgKillDragonRank(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::KILL_DRAGON_LAST_ATTACK: {
                            msg = new MsgKillDragonLastAttack(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::REINFORCEMENTS: {
                            msg = new MsgReinforcements(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::EXPLORE_MYSTERIOUS_CITY: {
                            msg = new MsgExploreMysteriousCity(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::ATTACK_GOBLIN_CAMP: {
                            msg = new MsgAttackGoblinCamp(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::CITYDEFENSE_UPDATE: {
                            msg = new MsgCityDefenseUpdate(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::CREATE_CAMP_FIXED_FAILED: {
                            msg = new MsgCreateCampFixedFailed(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::ATTACK_CAMP_FIXED: {
                            msg = new MsgAttackCampFixed(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::OCCUPY_CAMP_FIXED: {
                            msg = new MsgOccupyCampFixed(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::ATTACK_CAMP_TEMP: {
                            msg = new MsgAttackCampTemp(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::CITY_PATROL: {
                            msg = new MsgCityPatrol(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::BEAT_CASTLE: {
                            msg = new MsgBeatCastle(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::ATTACK_WORLDBOSS: {
                            msg = new MsgAttackWorldBoss(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::ATTACK_WORLDBOSS_END: {
                            msg = new MsgAttackWorldBossEnd(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::ATTACK_CITY_END: {
                            msg = new MsgAttackCityEnd(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::ATTACK_CATAPULT_END: {
                            msg = new MsgAttackCatapultEnd(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::TRANSPORT: {
                            msg = new MsgTransport(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::COMPENSATE: {
                            msg = new MsgCompensate(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::CASTLE_DEFENER_FILL: {
                            msg = new MsgCityDefenerFill(info.mid, info.uid);
                        }
                        break;
                        case model::MessageQueueType::TROOP_REACH_INVALID: {
                            // 行军无效 todo
                        }
                        break;
                        default:
                            LOG_ERROR("MsgQueue::LoadMsgFromDb MessageQueueType not exist, uid=%ld, type=%d, data=%s", info.uid, (int)info.type, info.data.c_str());
                            break;
                    }

                    if (msg != nullptr) {
                        bool exist = false;
                        for (auto it = m_waiting_records.begin(); it != m_waiting_records.end(); ++it) {
                            if ((*it)->id() == info.mid) {
                                exist = true;
                            }
                        }
                        if (!exist) {
                            for (auto it = m_sent_records.begin(); it != m_sent_records.end(); ++it) {
                                if ((*it)->id() == info.mid) {
                                    exist = true;
                                }
                            }
                        }

                        if (!exist && msg->Deserialize(info.data)) {
                            msg->SetFromDb();
                            m_waiting_records.push_back(msg);
                        } else {
                            // drop error data
                            if (!exist) {
                                LOG_ERROR("MsgQueue::LoadMsgFromDb Deserialize fail, uid=%ld, type=%d, data=%s", info.uid, (int)info.type, info.data.c_str());
                            }
                            m_runner.PushCommand(new map::qo::CommandMsgQueueDelete(msg));
                        }
                    }
                }
            }

            void MsgQueue::SendMsg()
            {
                if (m_stop || !m_agent || !m_agent->mbid()) {
                    return;
                }

                // handle re-send
                {
                    auto it = m_sent_records.begin();
                    while (it != m_sent_records.end()) {
                        MsgRecord* msg = *it;
                        if (msg->ShouldResend()) {
                            msg->Send(m_agent);
                        }
                        ++it;
                    }
                }

                {
                    auto it = m_waiting_records.begin();
                    while (it != m_waiting_records.end()) {
                        MsgRecord* msg = *it;
                        msg->Send(m_agent);
                        m_sent_records.push_back(msg);
                        it = m_waiting_records.erase(it);
                    }
                }
            }

            void MsgQueue::AppendMsg(MsgRecord* msg)
            {
                if (m_agent && m_agent->mbid()) {
                    m_waiting_records.push_back(msg);
                    SendMsg();
                } else {
                    qo::g_dataService->AppendData(qo::g_dataService->CreateDbInfo(msg));
                    SAFE_DELETE(msg);
                }
            }


            void MsgQueue::AppendMsgMarch(model::MapTroopType troopType, const Point& toPos, const ArmyList* armyList, const int troopId)
            {
                if (m_agent) {
                    MsgMarch* msg = new MsgMarch(GenerateID(), m_agent->uid());
                    msg->SetData(troopType, toPos, armyList, troopId);
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgMarchBack(bool isArriveCastle,model::MapTroopType troopType, const Point& toPos, int food, int wood, int iron, int stone, const ArmyList* armyList)
            {
                if (m_agent) {
                    MsgMarchBack* msg = new MsgMarchBack(GenerateID(), m_agent->uid());
                    msg->SetData(isArriveCastle, troopType, toPos, food, wood, iron, stone, armyList);
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgTroopReachInvalid(model::MapTroopType troopType)
            {
                if (m_agent) {
                    MsgTroopReachInvalid* msg = new MsgTroopReachInvalid(GenerateID(), m_agent->uid());
                    msg->SetData(troopType);
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgAttackMonsterInvalid()
            {
                if (m_agent) {
                    MsgAttackMonsterInvalid* msg = new MsgAttackMonsterInvalid(GenerateID(), m_agent->uid());
                    msg->SetData();
                    AppendMsg(msg);
                }
            }

            // 创建行营失败
            void MsgQueue::AppendMsgCreateCampFixedFailed()
            {
                if (m_agent) {
                    MsgCreateCampFixedFailed* msg = new MsgCreateCampFixedFailed(GenerateID(), m_agent->uid());
                    msg->SetData();
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgGatherResource(const Point& toPos, int resTplId, const std::vector< model::tpl::DropItem >& dropItems,  int gatherRemain)
            {
                if (m_agent) {
                    MsgGatherResource* msg = new MsgGatherResource(GenerateID(), m_agent->uid());
                    msg->SetData(toPos, resTplId, dropItems, gatherRemain, g_dispatcher->GetTimestampCache());
                    AppendMsg(msg);
                }
            }

            //攻击野怪
            void MsgQueue::AppendMsgAttackMonster(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, 
                    const Point& unitPos, int tplId, int monsterHpBegin, int monsterHpEnd, const std::vector<model::tpl::DropItem>& dropItems, 
                    const ArmyList& armyList, bool isCaptive, int unitId, int troopId, int reportId, const MsgReportInfo& reportInfo)
            {
                if (m_agent) {
                    MsgAttackMonster* msg = new MsgAttackMonster(GenerateID(), m_agent->uid());
                    msg->SetData(winner, myAttackType, attacker, defender, unitPos, tplId, monsterHpBegin, monsterHpEnd, dropItems, armyList, 
                        isCaptive, g_dispatcher->GetTimestampCache(), unitId, troopId, reportId, reportInfo);
                    AppendMsg(msg);
                }
            }

            // 攻击名城
            void MsgQueue::AppendMsgAttackCity(model::AttackType winner, model::AttackType myAttackType, 
                const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int reportId, 
                const Point& unitPos, int tplId, const std::vector<model::tpl::DropItem>& dropItems, int unitId, int troopId, const MsgReportInfo& reportInfo)
            {
                if (m_agent) {
                    MsgAttackCity* msg = new MsgAttackCity(GenerateID(), m_agent->uid());
                    msg->SetData(winner,myAttackType, attacker, defender, reportId, unitPos, tplId, dropItems, 
                        g_dispatcher->GetTimestampCache(), unitId, troopId, reportInfo);
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgAttackCastle(model::AttackType winner, model::AttackType myAttackType, 
                const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int reportId, const Point& unitPos, 
                int tplId, int unitId, int troopId, int armyHurt, int food, int wood, int iron, int stone,
              int foodRemove, int woodRemove, int ironRemove, int stoneRemove, const std::vector<info::CollectInfo>& collectInfos, int64_t burnEndTs, const MsgReportInfo& reportInfo)
            {
                if (m_agent) {
                    MsgAttackCastle* msg = new MsgAttackCastle(GenerateID(), m_agent->uid());
                    msg->SetData(winner, myAttackType, attacker,  defender, reportId, unitPos, tplId, armyHurt, food, wood, iron, stone, g_dispatcher->GetTimestampCache(),
                      foodRemove,  woodRemove,  ironRemove,  stoneRemove,  collectInfos,  burnEndTs, unitId,  troopId, reportInfo);
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgBeatCastle(model::AttackType winner, model::AttackType myAttackType,  const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int unitId, int troopId, int food, int wood, 
                int iron, int stone, int foodRemove, int woodRemove, int ironRemove, int stoneRemove, 
                const std::vector<info::CollectInfo>& collectInfos, int64_t burnEndTs)
            {
                if (m_agent) {
                    MsgBeatCastle* msg = new MsgBeatCastle(GenerateID(), m_agent->uid());
                    msg->SetData(winner, myAttackType, attacker,  defender, food, wood, iron, stone, g_dispatcher->GetTimestampCache(),
                      foodRemove,  woodRemove,  ironRemove,  stoneRemove,  collectInfos,  burnEndTs, unitId,  troopId);
                    AppendMsg(msg); 
                }
            }

            void MsgQueue::AppendMsgAttackResource(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, 
                const MsgPvpPlayerInfo& defender, int reportId, const Point& unitPos, int tplId, 
                const std::vector<model::tpl::DropItem>& dropItems, int unitId, int troopId, const MsgReportInfo& reportInfo)
            {
                if (m_agent) {
                    MsgAttackResource* msg = new MsgAttackResource(GenerateID(), m_agent->uid());
                    msg->SetData(winner, myAttackType, attacker, defender, reportId, unitPos, tplId, dropItems, 
                        g_dispatcher->GetTimestampCache(), unitId, troopId, reportInfo);
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgAttackCampFixed(model::AttackType winner, model::AttackType myAttackType, 
                const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int reportId, 
                const Point& unitPos, int tplId, int unitId, int troopId, const MsgReportInfo& reportInfo)
            {
                if (m_agent) {
                    MsgAttackCampFixed* msg = new MsgAttackCampFixed(GenerateID(), m_agent->uid());
                    msg->SetData(winner, myAttackType, attacker, defender, reportId, unitPos, tplId, g_dispatcher->GetTimestampCache(), unitId, troopId, reportInfo);
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgOccupyCampFixed(model::AttackType winner, model::AttackType myAttackType, 
                const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int reportId, 
                const Point& unitPos, int tplId, int unitId, int troopId, const MsgReportInfo& reportInfo)
            {
                if (m_agent) {
                    MsgOccupyCampFixed* msg = new MsgOccupyCampFixed(GenerateID(), m_agent->uid());
                    msg->SetData(winner, myAttackType, attacker, defender, reportId, unitPos, tplId, g_dispatcher->GetTimestampCache(), unitId, troopId, reportInfo);
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgAttackCampTemp(model::AttackType winner, model::AttackType myAttackType, 
                const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int reportId, const Point& unitPos, 
                int tplId, int unitId, int troopId, const MsgReportInfo& reportInfo)
            {
                if (m_agent) {
                    MsgAttackCampTemp* msg = new MsgAttackCampTemp(GenerateID(), m_agent->uid());
                    msg->SetData(winner, myAttackType, attacker, defender, reportId, unitPos, tplId, g_dispatcher->GetTimestampCache(), unitId, troopId, reportInfo);
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgAttackPalace(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int unitId, int troopId, const MsgPvpDragonInfo& dragon)
            {
                if (m_agent) {
                    MsgAttackPalace* msg = new MsgAttackPalace(GenerateID(), m_agent->uid());
                    msg->SetData(winner, myAttackType, attacker, defender, g_dispatcher->GetTimestampCache(), unitId, troopId, dragon);
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgAttackNeutralCastle(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int unitId, int troopId)
            {
                if (m_agent) {
                    MsgAttackNeutralCastle* msg = new MsgAttackNeutralCastle(GenerateID(), m_agent->uid());
                    msg->SetData(winner, myAttackType, attacker, defender, g_dispatcher->GetTimestampCache(), unitId, troopId);
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgAttackCatapult(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, const std::vector<model::tpl::DropItem>& dropItems, int reportId, int unitId, int troopId, const MsgReportInfo& reportInfo)
            {
                if (m_agent) {
                    MsgAttackCatapult* msg = new MsgAttackCatapult(GenerateID(), m_agent->uid());
                    msg->SetData(winner, myAttackType, attacker, defender, dropItems, reportId, g_dispatcher->GetTimestampCache(), unitId, troopId, reportInfo);
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgAttackGoblinCamp(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int unitId, int troopId)
            {
                if (m_agent) {
                    cout << "MsgQueue::AppendMsgAttackGoblinCamp" << endl;
                    MsgAttackGoblinCamp* msg = new MsgAttackGoblinCamp(GenerateID(), m_agent->uid());
                    msg->SetData(winner, myAttackType, attacker, defender, g_dispatcher->GetTimestampCache(), unitId, troopId);
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgNeutralCastleNoticeMail(model::MailSubType mailSubType, const AllianceSimple* alliance, const AllianceSimple* occupier)
            {
                if (m_agent) {
//                     MsgNeutralCastleNoticeMail* msg = new MsgNeutralCastleNoticeMail(GenerateID(), m_agent->uid());
//                     MsgAllianceInfo aInfo;
//                     if (alliance) {
//                         aInfo.allianceName = alliance->info.name;
//                         aInfo.allianceNickname = alliance->info.nickname;
//                         aInfo.bannerId = alliance->info.bannerId;
//                     }
//                     string enemyAllianceName;
//                     if (mailSubType == model::MailSubType::LOSS_NEUTRAL_CASTLE && occupier) {
//                         enemyAllianceName = occupier->info.name;
//                     }
//                     msg->SetData(mailSubType, aInfo, g_dispatcher->GetTimestampCache(), enemyAllianceName);
//                     AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgScout(model::AttackType winner, model::AttackType myAttackType, const std::vector< model::WATCHTOWER_SCOUT_TYPE >& scoutTypes, const MsgPlayerInfo& attacker, const MsgScoutDefenderInfo& defender)
            {
                if (m_agent) {
                    MsgScout* msg = new MsgScout(GenerateID(), m_agent->uid());
                    msg->SetData(winner, myAttackType, scoutTypes, attacker, defender, g_dispatcher->GetTimestampCache());
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgBuffRemove(model::BuffType type)
            {
                if (m_agent) {
                    //std::cout << "AppendMsgBuffRemove" << std::endl;
                    MsgBuffRemove* msg = new MsgBuffRemove(GenerateID(), m_agent->uid());
                    msg->SetData(type);
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgResourceHelp(const MsgPlayerInfo& player, model::AttackType myAttackType, int food, int wood, int iron, int stone)
            {
                if (m_agent) {
                    //std::cout << "AppendMsgResourceHelp" << std::endl;
                    MsgResourceHelp* msg = new MsgResourceHelp(GenerateID(), m_agent->uid());
                    msg->SetData(player, food, wood, iron, stone, myAttackType, g_dispatcher->GetTimestampCache());
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgReinforcements(const MsgPlayerInfo& player, model::AttackType myAttackType, const ArmyList& armyList)
            {
                if (m_agent) {
                    //std::cout << "AppendMsgReinforcements" << std::endl;
                    MsgReinforcements* msg = new MsgReinforcements(GenerateID(), m_agent->uid());
                    msg->SetData(player, myAttackType, armyList, g_dispatcher->GetTimestampCache());
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgMonsterSiege(int level, int food, int wood, int iron, int stone, int foodRemove, int woodRemove, int ironRemove, int stoneRemove, const std::vector< info::CollectInfo >& collectInfos)
            {
                if (m_agent) {
                    //std::cout << "AppendMsgMonsterSiege" << std::endl;
                    MsgMonsterSiege* msg = new MsgMonsterSiege(GenerateID(), m_agent->uid());
                    msg->SetData(level, food, wood, iron, stone, foodRemove, woodRemove, ironRemove, stoneRemove, collectInfos, g_dispatcher->GetTimestampCache());
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgMonsterSiegeResourceGetBack(int level, int food, int wood, int iron, int stone)
            {
                if (m_agent) {
                    //std::cout << "AppendMsgMonsterSiegeResourceGetBack" << std::endl;
                    MsgMonsterSiegeResourceGetBack* msg = new MsgMonsterSiegeResourceGetBack(GenerateID(), m_agent->uid());
                    msg->SetData(level, food, wood, iron, stone, g_dispatcher->GetTimestampCache());
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgBeAttackedByCatapult(const ArmyList& dieList, bool isCaptive)
            {
                if (m_agent) {
                    //std::cout << "AppendMsgBeAttackedByCatapult" << std::endl;
                    MsgBeAttackedByCatapult* msg = new MsgBeAttackedByCatapult(GenerateID(), m_agent->uid());
                    msg->SetData(dieList, isCaptive);
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgCastleRebuild(Point castlePos)
            {
                if (m_agent) {
                    //std::cout << "AppendMsgCastleRebuild" << std::endl;
                    MsgCastleRebuild* msg = new MsgCastleRebuild(GenerateID(), m_agent->uid());
                    msg->SetData(castlePos, g_dispatcher->GetTimestampCache());
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgKillDragonRank(int rank)
            {
                if (m_agent) {
                    //std::cout << "AppendMsgKillDragonRank" << std::endl;
                    MsgKillDragonRank* msg = new MsgKillDragonRank(GenerateID(), m_agent->uid());
                    msg->SetData(rank, g_dispatcher->GetTimestampCache());
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgKillDragonLastAttack()
            {
                if (m_agent) {
                    //std::cout << "AppendMsgKillDragonLastAttack" << std::endl;
                    MsgKillDragonLastAttack* msg = new MsgKillDragonLastAttack(GenerateID(), m_agent->uid());
                    msg->SetData(g_dispatcher->GetTimestampCache());
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgExploreMysteriousCity(const vector< model::tpl::DropItem >& drops)
            {
                if (m_agent) {
                    // std::cout << "AppendMsgExploreMysteriousCity" << std::endl;
                    MsgExploreMysteriousCity* msg = new MsgExploreMysteriousCity(GenerateID(), m_agent->uid());
                    msg->SetData(drops);
                    AppendMsg(msg);
                }
            }

            // 城防值更新
            void MsgQueue::AppendMsgCityDefenseUpdate(int cityDefense)
            {
                    MsgCityDefenseUpdate* msg = new MsgCityDefenseUpdate(GenerateID(), m_agent->uid());
                    msg->SetData(cityDefense);
                    AppendMsg(msg);
            }

            void MsgQueue::AppendMsgCityPatrol(const MsgCityInfo& cityInfo, const std::vector<MsgCityPatrolEvent> &events)
            {
                    MsgCityPatrol* msg = new MsgCityPatrol(GenerateID(), m_agent->uid());
                    msg->SetData(cityInfo, events, g_dispatcher->GetTimestampCache());
                    AppendMsg(msg);
            }

            void MsgQueue::AppendMsgAttackWorldBoss(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int reportId, int unitId, int troopId, const MsgReportInfo& reportInfo)
            {
                if (m_agent) {
                    MsgAttackWorldBoss* msg = new MsgAttackWorldBoss(GenerateID(), m_agent->uid());
                    msg->SetData(winner, myAttackType, attacker, defender, reportId, g_dispatcher->GetTimestampCache(), unitId, troopId, reportInfo);
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgAttackWorldBossEnd(const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, const MsgWorldBossInfo& WorldBoss, int unitId, int troopId)
            {
                if (m_agent) {
                    MsgAttackWorldBossEnd* msg = new MsgAttackWorldBossEnd(GenerateID(), m_agent->uid());
                    msg->SetData(attacker, defender, WorldBoss, g_dispatcher->GetTimestampCache(), unitId, troopId);
                    AppendMsg(msg);
                }
            }


            void MsgQueue::AppendMsgAttachCityEnd(const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, const MsgWorldBossInfo& WorldBoss, const std::vector<model::tpl::DropItem>& dropItems,  const ArmyList& armyList, int unitId, int troopId)
            {
                if (m_agent) {
                    MsgAttackCityEnd* msg = new MsgAttackCityEnd(GenerateID(), m_agent->uid());
                    msg->SetData(attacker, defender, WorldBoss, dropItems, armyList, g_dispatcher->GetTimestampCache(), unitId, troopId);
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgAttachCatapultEnd(const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, const MsgWorldBossInfo& WorldBoss, int unitId, int troopId)
            {
                if (m_agent) {
                    MsgAttackCatapultEnd* msg = new MsgAttackCatapultEnd(GenerateID(), m_agent->uid());
                    msg->SetData(attacker, defender, WorldBoss, g_dispatcher->GetTimestampCache(), unitId, troopId);
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgTransport(model::TransportType myTransportType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int food, int wood, int iron, int stone, bool isSuccess)
            {
                if (m_agent) {
                    MsgTransport* msg = new MsgTransport(GenerateID(), m_agent->uid());
                    msg->SetData(myTransportType, attacker, defender, food, wood, iron, stone, isSuccess, g_dispatcher->GetTimestampCache());
                    AppendMsg(msg);
                }
            }

            void MsgQueue::AppendMsgCompensate(int food, int wood, int stone, int iron)
            {
                if (m_agent) {
                    MsgCompensate* msg = new MsgCompensate(GenerateID(), m_agent->uid());
                    msg->SetData(food, wood, stone, iron, g_dispatcher->GetTimestampCache());
                    AppendMsg(msg);
                }
            }
            void MsgQueue::AppendMsgCityDefenerFill(const ArmyList* defList)
            {
                if (m_agent) {
                    MsgCityDefenerFill* msg = new MsgCityDefenerFill(GenerateID(), m_agent->uid());
                    msg->SetData(defList, g_dispatcher->GetTimestampCache());
                    AppendMsg(msg);
                }
            }

        }
    }
}
