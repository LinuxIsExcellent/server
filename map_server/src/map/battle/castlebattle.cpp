#include "castlebattle.h"
#include "../agent.h"
#include "../unit/castle.h"
#include <model/tpl/templateloader.h>
#include <model/tpl/configure.h>
#include <base/logger.h>
#include <cmath>
#include <iostream>
namespace ms
{
    namespace map
    {
        namespace battle
        {
            using namespace model;
            using namespace model::tpl;
            using namespace msgqueue;
            using namespace std;
            CastleBattle::CastleBattle(Troop* troop, Castle* castle)
                : Battle(model::BattleType::SIEGE_CASTLE, troop, castle), m_castle(castle)//, m_arrArmyList(castle->armyListArray())
            {

            }

            CastleBattle::~CastleBattle()
            {
            }

            void CastleBattle::SetDefenseInput( engine::InitialDataInput& defenseInput)
            {
               
                // 1.先设置友军守军
                if (m_defTroop) {
                    defenseInput.uid = m_defTroop->uid();
                    defenseInput.name = m_defTroop->agent().nickname();
                    defenseInput.level = m_defTroop->agent().level();
                    defenseInput.headIcon = m_defTroop->agent().headId();
                    m_defArmyList = m_defTroop->armyList();
                    if (m_defArmyList) {
                        auto& battleConf = g_tploader->configure().battle;
                        defenseInput.team.trapAtkPercentage = battleConf.traptake;
                        defenseInput.team.turretAtkPower = m_castle->agent().property().turretAttackForce;
    
                        // 防御加成(城防伤害减免判定值)
                        if (m_castle->cityDefense() >= m_castle->cityDefenseMax() *battleConf.wallprotect) {
                            m_defArmyList->SetAddDefensePct(m_castle->agent().property().cityTroopsDefensePct);
                        }
                    }
                    m_isCastleAgent = false;
                    m_isDefener = false;
                    return;
                }     


                // 2.再攻击玩家设置的守城守军
                int defender = m_castle->getDefTeam() - 1;
                if ( defender >= 0 && defender < 5 && !m_castle->armyListArray()[defender].IsOut() /* && !m_castle->armyListArray()[defender].IsAllDie()*/)
                {  
                    m_defArmyList = &m_castle->armyListArray()[defender];
                    //守军自动补满 减少城内兵
                    auto& armySet = m_castle->agent().armySet();
                    for (auto army : m_defArmyList->armies()) {
                        auto& armyInfo = army.second.armyInfo();
                        auto& heroInfo = army.second.heroInfo();
                        int heroId = heroInfo.tplId();
                        int level = heroInfo.level();
                        int leaderShip = 1000 + 300 * level;                  
                        int oldNormalCount = armyInfo.count(ArmyState::NORMAL);
                        if (leaderShip > oldNormalCount) {
                            int changeCount = leaderShip - oldNormalCount;
                            auto cArmyInfo = armySet.GetArmy(armyInfo.type());
                            if (cArmyInfo) {    
                                int cCount = cArmyInfo->count(ArmyState::NORMAL); 
                                if( cCount > changeCount) {
                                    cArmyInfo->Add(ArmyState::MARCHING, changeCount);
                                    cArmyInfo->Remove(ArmyState::NORMAL, changeCount);
                                    m_defArmyList->SetArmy(heroId,ArmyState::NORMAL, leaderShip);
                                } else {
                                    cArmyInfo->Add(ArmyState::MARCHING, cCount);
                                    cArmyInfo->Remove(ArmyState::NORMAL, cCount);
                                    m_defArmyList->SetArmy(heroId,ArmyState::NORMAL, oldNormalCount + cCount);
                                }
                            }
                        }
                    }
                    m_castle->agent().msgQueue().AppendMsgCityDefenerFill(m_defArmyList);  
 
                    m_defTrapSet = &m_castle->agent().trapSet();
                    
                    defenseInput.uid = m_castle->uid();
                    defenseInput.name = m_castle->nickname();
                    defenseInput.level = m_castle->agent().level();
                    defenseInput.headIcon = m_castle->agent().headId();
    
                    if (m_defArmyList) {
                        auto& battleConf = g_tploader->configure().battle;
                        defenseInput.team.trapAtkPercentage = battleConf.traptake;
                        defenseInput.team.turretAtkPower = m_castle->agent().property().turretAttackForce;
    
                        // 防御加成(城防伤害减免判定值)
                        if (m_castle->cityDefense() >= m_castle->cityDefenseMax() *battleConf.wallprotect) {
                            m_defArmyList->SetAddDefensePct(m_castle->agent().property().cityTroopsDefensePct);
                        }
                    }
                    m_isCastleAgent = true;
                    m_isDefener = true;
                    return;
                }

               

                // 3.最后攻击点将台空闲守军 
                // while (m_index < 5) {
                //     auto armyList = m_castle->armyListArray()[m_index];
                //     // std::cout << "castle ArmyList debug Begin" << std::endl;
                //     // armyList.Debug();
                //     // std::cout << "castle ArmyList debug End" << std::endl;
                //     if (armyList.team() == 0) {
                //         m_index++;
                //         continue;
                //     }
                //     if (armyList.IsOut()) {
                //         m_index++;
                //         continue;
                //     }
                //     // 检测带兵数是否为0
                //     if (armyList.IsAllDie())
                //     {
                //         m_index++;
                //         continue;
                //     }
                //     break;
                // }

                // if (m_index < 5 && m_index > -1) {
                //     m_defArmyList = &m_castle->armyListArray()[m_index];
                //      //m_defArmyList = m_castle->DefArmyList();
                //     m_defTrapSet = &m_castle->agent().trapSet();
                    
                //     defenseInput.uid = m_castle->uid();
                //     defenseInput.name = m_castle->nickname();
                //     defenseInput.level = m_castle->agent().level();
                //     defenseInput.headIcon = m_castle->agent().headId();
    
                //     if (m_defArmyList) {
                //         auto& battleConf = g_tploader->configure().battle;
                //         defenseInput.team.trapAtkPercentage = battleConf.traptake;
                //         defenseInput.team.turretAtkPower = m_castle->agent().property().turretAttackForce;
    
                //         // 防御加成(城防伤害减免判定值)
                //         if (m_castle->cityDefense() >= m_castle->cityDefenseMax() *battleConf.wallprotect) {
                //             m_defArmyList->SetAddDefensePct(m_castle->agent().property().cityTroopsDefensePct);
                //         }
                //     }
                //     m_isCastleAgent = true;
                //     m_isDefener = false;
                // }
            }

            void CastleBattle::OutPutDefenseArmyList(const engine::MixedCombatResult& mixedResult)
            {
                if (m_defArmyList) {
                    m_defArmyList->SetAddDefensePct(0.0f);

                    for (auto & hurt : mixedResult.defenseHurt) {
                        if (hurt.dieCount > 0) {
                            m_defArmyList->Die(hurt.heroId, hurt.dieCount);
                        }

                        if (hurt.killCount > 0) {
                            m_defArmyList->SetArmy(hurt.heroId, ArmyState::KILL, hurt.killCount);
                        }
                    }

                    auto& battleConf = g_tploader->configure().battle;
                    // 玩家被攻击，产生伤兵
                    m_defArmyList->DieToWound(battleConf.injuryratio);

                    // 减少城内兵
                    auto& armySet = m_castle->agent().armySet();
                    for (auto army : m_defArmyList->armies()) {
                        ArmyGroup& armyGroup = army.second;
                        auto cArmyInfo = armySet.GetArmy(armyGroup.armyInfo().type());
                        if (cArmyInfo) {
                            int die = armyGroup.armyInfo().count(ArmyState::DIE);
                            int wound = armyGroup.armyInfo().count(ArmyState::WOUNDED);
                            cArmyInfo->Add(ArmyState::DIE, die);
                            cArmyInfo->Add(ArmyState::WOUNDED, wound);
                            cArmyInfo->Remove(ArmyState::NORMAL, die + wound);
                        }
                    }
                }

                // 失败方进行兵力补偿  //Todo: 这种防守部队的agent查找 有可能有漏洞
                //if (mixedResult.isAttackWin && mixedResult.defenseTotalDie > 5000) {
                if (mixedResult.isAttackWin) {
                    if (m_defTroop) {
                        auto& agent = m_defTroop->agent();
                        Compensate(model::AttackType::DEFENSE, mixedResult, agent);
                    } else {
                        auto& agent = m_castle->agent();
                        Compensate(model::AttackType::DEFENSE, mixedResult, agent);
                    }
                }
            }

            void CastleBattle::OnStart()
            {
                //attacker
                Agent& attAgent = m_atkTroop->agent();
                m_attackerInfo.SetData(attAgent);

                // defender
                Agent& defAgent = m_isCastleAgent ? m_castle->agent() : m_defTroop->agent();
                m_defenderInfo.SetData(defAgent);
            }

            void CastleBattle::OnEnd(model::AttackType winner, int reportId, bool last_flag)
            {
                // LOG_DEBUG("CastleBattle::OnEnd");
                Agent& attAgent = m_atkTroop->agent();
                Agent& defAgent = m_isCastleAgent ? m_castle->agent() : m_defTroop->agent();

                {
                    m_attackerInfo.detail.SetData(*m_atkTroop);
                }

                {
                    if (m_defArmyList) {
                        m_defenderInfo.detail.armyList = *m_defArmyList;
                    }

                    if (m_defTrapSet) {
                        m_defenderInfo.detail.trapSet = *m_defTrapSet;
                    }
                }
                m_posName = defAgent.nickname();
                m_isLang = 0;

                //战斗结束时统计一下双方总兵力
                OutPutEndAmryCount();

                if (winner ==  model::AttackType::ATTACK) {
                    // 若被攻击方当前驻守队伍=0，或驻守队伍兵力=0，
                    // 而城中有未上阵的士兵，则需要计算进攻方的攻击，对士兵造成伤害
                    int armyHurt = 0;
                    if (!m_defArmyList || m_defArmyList->size() ==  0) {
                        if (m_atkArmyList) {
                            for (auto &army :  m_atkArmyList->armies()) {
                                ArmyInfo& armyInfo = army.second.armyInfo();
                                int count = armyInfo.count(ArmyState::NORMAL);
                                armyHurt += count * armyInfo.attack();
                            }
                        }
                    }

                    // 掠夺
                    int foodReal = 0, woodReal = 0, ironReal = 0, stoneReal = 0;
                    // 防守方失去的资源数
                    int foodRemove = 0, woodRemove = 0, ironRemove = 0, stoneRemove = 0;
                    if (last_flag)
                    {
                        ResourcePlunder(foodReal, woodReal, ironReal, stoneReal, foodRemove, woodRemove, ironRemove, stoneRemove);
                    }
                    attAgent.msgQueue().AppendMsgAttackCastle(AttackType::ATTACK,  AttackType::ATTACK,  m_attackerInfo, 
                        m_defenderInfo, reportId,  m_castle->pos(), m_castle->tpl().id, m_castle->id(), m_atkTroop->id(), 
                        0,  foodReal,  woodReal,  ironReal,  stoneReal, foodRemove, woodRemove, 
                        ironRemove,  stoneRemove,  std::vector<info::CollectInfo>(), 0, m_reportInfo);
                    defAgent.msgQueue().AppendMsgAttackCastle(AttackType::ATTACK,  AttackType::DEFENSE, m_attackerInfo, 
                        m_defenderInfo, reportId,  m_castle->pos(), m_castle->tpl().id, m_castle->id(), m_atkTroop->id(),  
                        0,  foodReal,  woodReal,  ironReal,  stoneReal, foodRemove, woodRemove, 
                        ironRemove,  stoneRemove,  std::vector<info::CollectInfo>(), 0, m_reportInfo);
                } else {
                    attAgent.msgQueue().AppendMsgAttackCastle(AttackType::DEFENSE,  AttackType::ATTACK, m_attackerInfo, m_defenderInfo, reportId,  m_castle->pos(), 
                        m_castle->tpl().id, m_castle->id(), m_atkTroop->id(),  0,  0,  0,  0,  0, 0, 0, 0,  0,  std::vector<info::CollectInfo>(), 0, m_reportInfo);
                    defAgent.msgQueue().AppendMsgAttackCastle(AttackType::DEFENSE,  AttackType::DEFENSE,  m_attackerInfo, m_defenderInfo, reportId,  m_castle->pos(), 
                        m_castle->tpl().id, m_castle->id(), m_atkTroop->id(),  0,  0,  0,  0,  0, 0, 0, 0,  0,  std::vector<info::CollectInfo>(), 0, m_reportInfo);
                }
                attAgent.AddBattleCount(winner,  model::AttackType::ATTACK);
                defAgent.AddBattleCount(winner,  model::AttackType::DEFENSE);

                ResetArmyList();
            }

            void CastleBattle::OnBattleEnd(model::AttackType winner)
            {
                Agent& attAgent = m_atkTroop->agent();
                Agent& defAgent = m_castle->agent();
                m_attackerInfo.detail.SetData(*m_atkTroop);

                if (m_defArmyList) {
                    m_defenderInfo.detail.armyList = *m_defArmyList;
                }

                if (m_defTrapSet) {
                    m_defenderInfo.detail.trapSet = *m_defTrapSet;
                }
                
                if (defAgent.allianceId() > 0) {
                    g_mapMgr->AllianceCastleBeAttacked(defAgent.allianceId(), m_castle, m_atkTroop);
                }
                
                if (winner ==  model::AttackType::ATTACK) {
                    //攻击城墙
                    int wallHurt = 0;
                    if (m_atkArmyList) {
                        for (auto &army :  m_atkArmyList->armies()) {
                            ArmyInfo& armyInfo = army.second.armyInfo();
                            int count = armyInfo.count(ArmyState::NORMAL);
                            wallHurt += count * armyInfo.wallAttack();
                        }
                    }

                    // 消耗城防值
                    if (wallHurt > 0) {
                        m_castle->AddCityDefense(-wallHurt);
                    }
                    // 掠夺
                    int foodReal = 0, woodReal = 0, ironReal = 0, stoneReal = 0;
                    // 防守方失去的资源数
                    int foodRemove = 0, woodRemove = 0, ironRemove = 0, stoneRemove = 0;
                    // ResourcePlunder(foodReal, woodReal, ironReal, stoneReal, foodRemove, woodRemove, ironRemove, stoneRemove);
                    // ==================
                    attAgent.msgQueue().AppendMsgBeatCastle(AttackType::ATTACK,  AttackType::ATTACK,  m_attackerInfo, m_defenderInfo, m_castle->id(), m_atkTroop->id(), foodReal,  woodReal,  ironReal,  stoneReal, foodRemove, woodRemove, ironRemove,  stoneRemove,
                        std::vector<info::CollectInfo>(), 0);

                    defAgent.msgQueue().AppendMsgBeatCastle(AttackType::ATTACK,  AttackType::DEFENSE,  m_attackerInfo, m_defenderInfo, m_castle->id(), m_atkTroop->id(), foodReal,  woodReal,  ironReal,  stoneReal, foodRemove, woodRemove, ironRemove,  stoneRemove,
                        std::vector<info::CollectInfo>(), 0);    

                    if (m_castle->cityDefense() <=  0) {
                        g_mapMgr->RebuildCastle(m_castle);  //Todo: 城防值小于0时，应该发送系统邮件
                    } else {
                        m_castle->Burn();
                    }
                } else {

                    defAgent.msgQueue().AppendMsgBeatCastle(AttackType::ATTACK,  AttackType::DEFENSE,  m_attackerInfo, m_defenderInfo, m_castle->id(), m_atkTroop->id(), 0,  0,  0,  0, 0, 0, 0,  0,
                        std::vector<info::CollectInfo>(), 0); 
                    
                }
                // attAgent.AddBattleCount(winner,  model::AttackType::ATTACK);
                // defAgent.AddBattleCount(winner,  model::AttackType::DEFENSE);
            }

            bool CastleBattle::SwitchTroop()
            {
                 // 1.同盟增援部队

                if (m_castle->SwitchTroop()/* && !m_isCastleAgent*/)
                {
                    m_defTroop = m_castle->troop();
                    return true;
                }
                else
                {
                    m_defTroop = nullptr;
                }
                // 2.玩家默认守军
                int i = 0;
                int defender = m_castle->getDefTeam() - 1; 
               
                if (defender >= 0 && defender < 5 && !m_castle->armyListArray()[defender].IsOut() && !m_castle->armyListArray()[defender].IsAllDie())
                {
                    return true;
                }
                if (m_isDefener)
                {   
                    if (m_defTroop)
                    {
                        return true;
                    }   
                }
               
                // 3.玩家空闲守军（部队编组） 
                while (i < 5 /*&& m_isCastleAgent*/)
                {
                    auto armyList = m_castle->armyListArray()[i];
                    // 没有出征，军队的兵还没有死完
                    if (!armyList.IsOut() && !armyList.IsAllDie()) {
                       return true;
                    }
                    i++;
                }
                return false;
            }

            bool CastleBattle::IsLastTroop() 
            {
                return m_unit->IsLastTroop();
            }

            void CastleBattle::ResourcePlunder(int& foodReal, int& woodReal, int& stoneReal, int& ironReal, int& foodRemove, int& woodRemove, int& stoneRemove, int& ironRemove)
            {
                Agent& attAgent = m_atkTroop->agent();
                Agent& defAgent = m_castle->agent();
                int plunderCount = attAgent.GetPlunderCount(m_castle->id());
                int plunderTotalCount = attAgent.plunderTotalCount();
                // 掠夺
                // 内外掠夺的资源数
                int foodOuter = 0, woodOuter = 0, ironOuter = 0, stoneOuter = 0;
                // 城内掠夺的资源数
                int foodInner = 0, woodInner = 0, ironInner = 0, stoneInner = 0;
                // 根据城内和城外的资源得出来能掠夺的资源总数
                int foodCan = 0, woodCan = 0, ironCan = 0, stoneCan = 0;
                const ResourceLoadConf& resourceLoadConf = g_tploader->configure().resourceLoad;
                const PlunderConfig& plunderConfig =g_tploader->configure().plunderConfig;
                if (plunderCount <  plunderConfig.perplunderlimit && plunderTotalCount <  plunderConfig.plunderlimit) {
                    float decay = 0.0f;//掠夺同一玩家衰减系数
                    for (auto it = plunderConfig.timeDecays.rbegin(); it !=  plunderConfig.timeDecays.rend(); ++it) {
                        auto& timeDecay = *it;
                        if ((plunderCount + 1) == std::get<0>(timeDecay)) {
                            decay = std::get<1>(timeDecay);
                        }
                    }
                    int attackCastleLevel = attAgent.castle()->level();
                    int defenseCastleLevel = m_castle->level();
                    // 只有进攻方等级比防守方的等级高
                    int levelDistance = attackCastleLevel - defenseCastleLevel > 0 ? attackCastleLevel - defenseCastleLevel : 0; //等级差
                    std::cout << "levelDistance = " << levelDistance << std::endl;
                    float reduction = 0.0f;         // 等级差系数
                    for (auto it = plunderConfig.levelLimits.rbegin(); it !=  plunderConfig.levelLimits.rend(); ++it) {
                        auto& levelLimit = *it;
                        /*std::cout << "std::get<0>(levelLimit) = " << std::get<0>(levelLimit) << std::endl;
                        std::cout << "std::get<1>(levelLimit) = " << std::get<1>(levelLimit) << std::endl;*/
                        if (levelDistance >= std::get<0>(levelLimit)) {
                            reduction = std::get<1>(levelLimit);
                            break;
                        }
                    }
                    std::cout << "decay = " << decay << "reduction = " << reduction << std::endl;
                    // 城内掠夺
                    float innerPlunderParams = 0;
                    int secureResource = 0;   // 安全资源，TODO by zds
                    {
                        float temp = (float)(defAgent.food() - secureResource - defAgent.property().foodProtectedLimit);
                        temp = temp > 0 ? temp : 0.0f;
                        innerPlunderParams = temp / defAgent.food();
                        if(innerPlunderParams > plunderConfig.foodinnerMax)
                        {
                            innerPlunderParams = plunderConfig.foodinnerMax;
                        }
                        foodInner = (int)defAgent.food()*innerPlunderParams*decay;
                        // 城外掠夺
                        foodOuter = defAgent.GetCollectTotal(model::ResourceType::FOOD) * plunderConfig.foodoutresourcelimit;
                        // 可以掠夺的食物总量
                        foodCan = (foodInner + foodOuter)*reduction;
                    }
                    std::cout << "foodInner = " << foodInner << "foodOuter = " << foodOuter << "foodCan = " << foodCan << std::endl;
                    {
                        float temp = (float)(defAgent.wood() - secureResource - defAgent.property().woodProtectedLimit);
                        temp = temp > 0 ? temp : 0.0f;
                        innerPlunderParams = temp / defAgent.wood();
                        if(innerPlunderParams > plunderConfig.woodinnerMax )
                        {
                            innerPlunderParams = plunderConfig.woodinnerMax;
                        }
                        woodInner = (int)defAgent.wood()*innerPlunderParams*decay;
                        // 城外掠夺
                        woodOuter = defAgent.GetCollectTotal(model::ResourceType::WOOD) * plunderConfig.woodoutresourcelimit;
                        // 可以掠夺的木材总量
                        woodCan = (woodInner + woodOuter)*reduction;
                    }
                    std::cout << "woodInner = " << woodInner << "woodOuter = " << woodOuter << "woodCan = " << woodCan << std::endl;
                    {
                        float temp = (float)(defAgent.stone() - secureResource - defAgent.property().stoneProtectedLimit);
                        temp = temp > 0 ? temp : 0.0f;
                        innerPlunderParams = temp / defAgent.stone();
                        if(innerPlunderParams > plunderConfig.stoneinnerMax )
                        {
                            innerPlunderParams = plunderConfig.stoneinnerMax;
                        }
                        stoneInner = (int)defAgent.stone()*innerPlunderParams*decay;
                        // 城外掠夺
                        stoneOuter = defAgent.GetCollectTotal(model::ResourceType::STONE) * plunderConfig.stoneoutresourcelimit;
                        // 可以掠夺的石头总量
                        stoneCan = (stoneInner + stoneOuter)*reduction;
                    }
                    std::cout << "stoneInner = " << stoneInner << "stoneOuter = " << stoneOuter << "stoneCan = " << stoneCan << std::endl;
                    {
                        float temp = (float)(defAgent.iron() - secureResource - defAgent.property().ironProtectedLimit);
                        temp = temp > 0 ? temp : 0.0f;
                        innerPlunderParams = temp / defAgent.iron();
                        if(innerPlunderParams > plunderConfig.ironinnerMax )
                        {
                            innerPlunderParams = plunderConfig.ironinnerMax;
                        }
                        ironInner = (int)defAgent.iron()*innerPlunderParams*decay;
                        // 城外掠夺
                        ironOuter = defAgent.GetCollectTotal(model::ResourceType::IRON) * plunderConfig.ironoutresourcelimit;
                        // 可以掠夺的铁总量
                        ironCan = (ironInner + ironOuter)*reduction;
                    }
                    std::cout << "ironInner = " << ironInner << "ironOuter = " << ironOuter << "ironCan = " << ironCan << std::endl;
                    //cout << "foodCan, woodCan, ironCan, mithrilCan, totalWeight : " << foodCan << " " << woodCan << " " << ironCan << " " << mithrilCan << " " << totalWeight << endl;
                    //2 计算负重
                    int loadSum = m_atkTroop->GetLoadTotal();
                    std::cout << "loadSum = " << loadSum << std::endl;
                    int foodLoad = std::floor(foodCan * resourceLoadConf.food);
                    int woodLoad = std::floor(woodCan * resourceLoadConf.wood);
                    int stoneLoad = std::floor(stoneCan * resourceLoadConf.stone);
                    int ironLoad = std::floor(ironCan * resourceLoadConf.iron); 
                    int canResourceLoadSum = foodLoad + woodLoad + stoneLoad + ironLoad;
                    // 全拿走
                    if (loadSum >= canResourceLoadSum)
                    {
                        foodReal = foodCan;
                        woodReal = woodCan;
                        stoneReal = stoneCan;
                        ironReal = ironCan;
                    }
                    else
                    {
                        int perLoad = std::floor(loadSum / 4);
                        //如果负重平均值小于或者等于所有资源的负重，
                        if (perLoad <= foodLoad && perLoad <= woodLoad && perLoad <= stoneLoad && perLoad <= ironLoad)
                        {
                            foodReal = std::floor(perLoad / resourceLoadConf.food);
                            woodReal = std::floor(perLoad / resourceLoadConf.wood);
                            stoneReal = std::floor(perLoad / resourceLoadConf.stone);
                            ironReal = std::floor(perLoad / resourceLoadConf.iron);
                            int lastLeftLoad = loadSum - foodReal * resourceLoadConf.food - woodReal * resourceLoadConf.wood - stoneReal * resourceLoadConf.stone -ironReal * resourceLoadConf.iron;
                            // 还要计算剩余的那部分 TODO
                            foodReal = foodReal + std::floor(lastLeftLoad / resourceLoadConf.food);
                        }
                        // 如果出现有些资源的负重大于平均值，有些资源的负重小于平均值,至少有一个负重超出
                        else
                        {
                            int lessThanPerLoad = 4;
                            bool foodIsDone = false;
                            bool woodIsDone = false;
                            bool stoneIsDone = false;
                            bool ironIsDone = false;
                            while (1)
                            {
                                int leftLoad = 0;
                                if (foodLoad < perLoad)
                                {
                                    leftLoad = perLoad - foodLoad;
                                    foodReal = foodCan;
                                    lessThanPerLoad--;
                                    foodIsDone = true;
                                }
                                if (woodLoad < perLoad)
                                {
                                    leftLoad = perLoad - woodLoad;
                                    woodReal = woodCan;
                                    lessThanPerLoad--;
                                    woodIsDone = true;
                                }
                                if (stoneLoad < perLoad)
                                {
                                    leftLoad = perLoad - stoneLoad;
                                    stoneReal = stoneCan;
                                    lessThanPerLoad--;
                                    stoneIsDone = true;
                                }
                                if (ironLoad < perLoad)
                                {
                                    leftLoad = perLoad - ironLoad;
                                    ironReal = ironCan;
                                    lessThanPerLoad--;
                                    ironIsDone = true;
                                }
                                if (lessThanPerLoad <= 0)
                                {
                                    break;
                                }
                                perLoad = std::floor(leftLoad + lessThanPerLoad * perLoad / lessThanPerLoad);
                                // 如果剩下的比上一次平均资源负重要大的资源都大于下一次平均资源，则退出循环
                                bool isBreak = true;
                                if (foodReal == 0 && foodLoad < perLoad)
                                {
                                    isBreak = false;
                                }
                                if (woodReal == 0 && woodLoad < perLoad)
                                {
                                    isBreak = false;
                                }
                                if (stoneReal == 0 && stoneLoad < perLoad)
                                {
                                    isBreak = false;
                                }
                                if (ironReal == 0 && ironLoad < perLoad)
                                {
                                    isBreak = false;
                                }
                                if (isBreak)
                                {
                                    break;
                                }
                            }
                            int leftCount = 0;
                            if (!foodIsDone)
                            {
                                leftCount++;
                                foodReal = std::floor(perLoad / resourceLoadConf.food);
                            }
                            if (!woodIsDone)
                            {
                                leftCount++;
                                woodReal = std::floor(perLoad / resourceLoadConf.wood);
                            }
                            if (!stoneIsDone)
                            {
                                leftCount++;
                                stoneReal = std::floor(perLoad / resourceLoadConf.stone);
                            }
                            if (!ironIsDone)
                            {
                                leftCount++;
                                ironReal = std::floor(perLoad / resourceLoadConf.iron);
                            }
                            // 最后填充剩下的那部分资源
                            int lastLeftLoad = loadSum - foodReal * resourceLoadConf.food - woodReal * resourceLoadConf.wood - stoneReal * resourceLoadConf.stone - ironReal * resourceLoadConf.iron;
                            foodReal = foodReal + std::floor(lastLeftLoad / resourceLoadConf.food);
                        }
                    }
                    foodRemove = foodReal;
                    woodRemove = woodReal;
                    ironRemove = stoneReal;
                    stoneRemove = ironReal;
                    foodReal = std::floor(foodReal * (1 - plunderConfig.recovery));
                    woodReal = std::floor(woodReal * (1 - plunderConfig.recovery));
                    ironReal = std::floor(ironReal * (1 - plunderConfig.recovery));
                    stoneReal = std::floor(stoneReal * (1 - plunderConfig.recovery));
                    defAgent.ResourceBePlundered(model::ResourceType::FOOD, foodRemove,  foodOuter);
                    defAgent.ResourceBePlundered(model::ResourceType::WOOD, woodRemove,  woodOuter);
                    defAgent.ResourceBePlundered(model::ResourceType::IRON, ironRemove,  ironOuter);
                    defAgent.ResourceBePlundered(model::ResourceType::STONE, stoneRemove,  stoneOuter);
                    //cout << "foodRemove = " << foodRemove << " woodRemove = " << woodRemove << " ironRemove = " << ironRemove << " stoneRemove = " << stoneRemove << endl;
                    m_atkTroop->AddResource(model::ResourceType::FOOD, foodReal);
                    m_atkTroop->AddResource(model::ResourceType::WOOD, woodReal);
                    m_atkTroop->AddResource(model::ResourceType::IRON, ironReal);
                    m_atkTroop->AddResource(model::ResourceType::STONE, stoneReal);
                    attAgent.OnPlunderCastle(m_castle->id()); //在战报中计算的赢家掠夺的资源显示
                }
            }
        }
    }
}
