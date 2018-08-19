#include "battlemgr.h"
#include "../unit/monster.h"
#include "../unit/castle.h"
#include "../unit/resnode.h"
#include "../unit/camp.h"
#include "../unit/famouscity.h"
#include "../unit/monster.h"
#include "interface.h"
#include "battle/singlecombat.h"
#include "battle/mixedcombat.h"
#include "model/tpl/templateloader.h"
#include <model/tpl/configure.h>
#include "model/tpl/hero.h"
#include <model/rpc/map.h>
#include <model/protocol.h>
#include "../agent.h"
#include "../agentProxy.h"
#include "../mapProxy.h"
#include "../mapMgr.h"
#include "battle.h"
#include "castlebattle.h"
#include "citybattle.h"
#include "resourcebattle.h"
#include "campfixedbattle.h"
#include "camptempbattle.h"
#include "monsterbattle.h"
#include "catapultbattle.h"
#include "worldbossbattle.h"
#include "capitalbattle.h"
#include "base/utils/crypto.h"
#include "../tpl/mapcitytpl.h"
#include <base/logger.h>
#include <base/data.h>


namespace ms
{
    namespace map
    {
        namespace battle
        {
            using namespace model;
            using namespace model::tpl;
            using namespace base;

            bool BattleMgr::GenerateBattle (Troop* troop, Unit* unit)
            {
                if (troop == nullptr || unit == nullptr) {
                    return false;
                }

                LOG_DEBUG("Generate Battle Type : %d ",  (int)unit->type());

                Troop* defTroop = nullptr;
                Troop* atkTroop = nullptr;
                
                // troops 
                if (unit->troop()) {
                    defTroop = unit->troop();
                }

                if (troop) {
                    atkTroop = troop;
                }

                if (auto castle = unit->ToCastle()) {
                    m_currentBattle = new CastleBattle(troop, castle);
                }else if (auto city = unit->ToCapital()) {
                    m_currentBattle = new CapitalBattle(troop, city);
                } else if (auto city = unit->ToFamousCity()) {
                    m_currentBattle = new CityBattle(troop, city);
                } else if (auto res = unit->ToResNode()) {
                    m_currentBattle = new ResourceBattle(troop, res);
                } else if (auto monster = unit->ToMonster()) {
                    m_currentBattle = new MonsterBattle(troop, monster);
                } else if (auto campFixed = unit->ToCampFixed()) {
                    m_currentBattle = new CampFixedBattle(troop, campFixed);
                } else if (auto campTemp = unit->ToCampTemp()) {
                    m_currentBattle = new CampTempBattle(troop, campTemp);
                } else if (auto catapult = unit->ToCatapult()) {
                    m_currentBattle = new CatapultBattle(troop, catapult);
                } else if (auto worldboss = unit->ToWorldBoss()) {
                    m_currentBattle = new WorldBossBattle(troop, worldboss);
                }
                else {
                    m_currentBattle = new DefaultBattle(troop, unit);
                }

                bool isTroopWin = true;
                bool next_flag = true;
                int attack_win_count = 0;
                //在unit中判断这个军队是不是最后一个，是的就表示战斗结束了.
                while(isTroopWin && next_flag)
                {
                    /*if (attack_win_count > 10)
                        break;*/
                    attack_win_count += 1;
                    //攻击方部队
                    engine::InitialDataInput attackInput;
                    ArmyList* atkArmyList = nullptr;
                    //防守方部队
                    engine::InitialDataInput defenseInput;
                    ArmyList* defArmyList = nullptr;

                    std::cout << "PalaceBattle Begin===============" << std::endl;
                    if (m_currentBattle) {
                        m_currentBattle->Init(attackInput,  defenseInput);
                        m_currentBattle->OnStart();
                        atkArmyList = m_currentBattle->atkArmyList();
                        defArmyList = m_currentBattle->defArmyList();
                    }
                    std::cout << "atkArmyList===============" << std::endl;
                    // atkArmyList->Debug();
                   
                    if (defArmyList != nullptr) {
                        std::cout << "defArmyList===============" << std::endl;
                        // defArmyList->Debug();
                    }

                    if (defArmyList ==  nullptr) {
                        isTroopWin = true;
                        if (unit->ToCastle() != nullptr) { //针对玩家城池,没有编组和驻守部队的特殊处理
                            m_currentBattle->OnEnd(isTroopWin  ? AttackType::ATTACK : AttackType::DEFENSE, 0, true);
                        }
                        break; //如果没有防御部队，就表明击穿了防御部队
                    }

                    //std::string battleInitialData = "";
                    int reportId = 0;
                    if (atkArmyList && defArmyList) {
                        do {
                            if (atkArmyList->IsAllDie()) {
                                isTroopWin = false;
                                break;
                            }
                            
                            int randSeed = framework.random().GenRandomNum(1, 80000000);
                            engine::Combat combat(randSeed, true, true, 1);   //不进行单挑
                            bool single = combat.Init(attackInput, defenseInput);
                            if (single) {
                                combat.CreateSingleCombat();
                                combat.StartSingleCombat();
                                if (m_currentBattle) {
                                    m_currentBattle->OutputSingleResult(combat.singleCombat()->result());
                                }
                            }

                            combat.CreateMixedCombat();
                            combat.StartMixedCombat();

                            if (combat.mixedCombat()) {
                                auto& mixedResult = combat.mixedCombat()->result();
                                isTroopWin = mixedResult.isAttackWin;

                                if (m_currentBattle) {
                                    m_currentBattle->OutputResult(mixedResult);
                                }

                                // 需要再加上单挑的数据
                                
                                auto& report = combat.mixedCombat()->report();
                                engine::WarReport::InitTeamData initTeamData1;
                                initTeamData1.uid = attackInput.uid;
                                initTeamData1.headId = attackInput.headIcon;
                                initTeamData1.nickName = attackInput.name;
                                initTeamData1.soloHeroId = -1;

                                engine::WarReport::InitTeamData initTeamData2;
                                initTeamData2.uid = defenseInput.uid;
                                initTeamData2.headId = defenseInput.headIcon;
                                initTeamData2.nickName = defenseInput.name;
                                initTeamData2.soloHeroId = -1;
                                report.initTeamDatas.push_back(initTeamData1);
                                report.initTeamDatas.push_back(initTeamData2);
                                report.result.win = isTroopWin ? engine::TeamType::ATTACKER : engine::TeamType::DEFENDER;

                                //WarReport 这个数据结构需要填充  to do ...
                                
                                reportId = g_mapMgr->GetNextReportId();
                                std::string reportData = combat.SerializeReportData(report, reportId);
                                g_mapMgr->proxy()->AddReport(troop->agent().uid(), reportId, reportData);

                                if (m_currentBattle) {
                                    //battleInitialData = CreateBattleInitialData(randSeed, attackInput, defenseInput, 
                                        // m_currentBattle->battleType(), unit->pos(), reportId);
                                }
                            }
                        } while (false);
                    }

                    if (m_currentBattle) {
                        // if (!defTroop && defArmyList) {
                        //     g_mapMgr->OnKillNpc(defArmyList->ArmyCount(ArmyState::DIE));
                        // }
                        bool last_flag = false;
                        if (defArmyList) 
                        {
                            if (defArmyList->IsAllDie()) {
                                last_flag = true; 
                            }
                            last_flag = last_flag && m_currentBattle->IsLastTroop();
                            last_flag = last_flag && atkArmyList->IsAllDie();
                        }
                        m_currentBattle->OnEnd(isTroopWin  ? AttackType::ATTACK : AttackType::DEFENSE, reportId, last_flag);
                        std::cout << "battleMgr, 222 last_flag = " << last_flag << std::endl;
                        //SAFE_DELETE(m_currentBattle);
                    }
                
                    next_flag = false;
                    if (defArmyList) 
                    {
                        //切换部队
                        if (defArmyList->IsAllDie()) {
                            next_flag = m_currentBattle->SwitchTroop();
                        }
                        // } else if (isTroopWin) {
                        //     LOG_DEBUG("Defence amry is not all die , retry attack");
                        //     next_flag = true;
                        //     defArmyList->Debug();
                        // }
                    }
                    // ...............
                }

                LOG_DEBUG("Battle End Result:%d   attack_count:%d", (bool)isTroopWin, attack_win_count);
                if (m_currentBattle) {
                    //  if (!defTroop && defArmyList) {
                    //      g_mapMgr->OnKillNpc(defArmyList->ArmyCount(ArmyState::DIE));
                    //  }
                     m_currentBattle->OnBattleEnd(isTroopWin  ? AttackType::ATTACK : AttackType::DEFENSE);
                     // 战斗结束之后的处理
                     m_currentBattle->OnBattleRecoverArmy();
                     SAFE_DELETE(m_currentBattle);
                }

                 //战斗结果
                if (isTroopWin) {
                    if (defTroop) {
                        defTroop->OnDefenceFailed(unit);
                    }

                    if (atkTroop) {
                        atkTroop->OnAttackWin(unit);
                    }
                } else {
                    if (defTroop) {
                        defTroop->OnDefenceWin(unit);
                    }

                    if (atkTroop) {
                        atkTroop->OnAttackFailed(unit);
                    }
                }

                return true;
            }

            void BattleMgr::PatrolCity(Troop* troop, FamousCity* city)
            {
                if (troop == nullptr || city ==  nullptr) {
                    return;
                }

                int evtGroupId = 0;
                auto cityTpl = city->cityTpl();
                if (cityTpl) {
                    evtGroupId = cityTpl->evtGroupId;
                }

                if (evtGroupId <= 0) {
                    return;
                }

                auto armyList = troop->armyList();
                if (armyList == nullptr) {
                    return;
                }
                
                std::vector<msgqueue::MsgCityPatrolEvent> events;

                auto procEvt = [&](const tpl::CityPatrolEventTpl* evtTpl) {
                    if (evtTpl) {
                        msgqueue::MsgCityPatrolEvent event;
                        event.id = evtTpl->id;
                        for (auto& remove :  evtTpl->removeList) {
                            model::tpl::DropItem removeItem(remove.tpl,  remove.count);
                            event.removes.emplace_back(removeItem);
                        }

                        switch (evtTpl->type) {
                            case model::PatrolEvent::EMPTY: {
                                if (armyList) {
                                    int armyCount = armyList->GetArmyTotal();
                                    double rate = 1.0 * armyCount/evtTpl->armyCount;
                                    for (auto& drop :  evtTpl->dropList) {
                                        model::tpl::DropItem dropItem(drop.tpl,  drop.count * rate);
                                        event.drops.emplace_back(dropItem);
                                    }
                                }
                            }
                            break;
                            case model::PatrolEvent::FIGHT: {
                                for (auto& drop :  evtTpl->dropList) {
                                    model::tpl::DropItem dropItem(drop.tpl,  drop.count);
                                    event.drops.emplace_back(dropItem);
                                }
                                if (armyList) {
                                    int hurtHp = evtTpl->hurtHp;
                                    for (int pos = 1; pos <= 9; ++pos) {
                                        auto armyGroup = armyList->GetArmyGroupByPos(pos);
                                        if (armyGroup !=  nullptr) {
                                            hurtHp -= armyGroup->armyInfo().HurtHp(hurtHp);
                                        }
                                        if (hurtHp <= 0) {
                                            break;
                                        }
                                    }
                                    event.armyList = *armyList;
                                    armyList->ClearAllExceptNormal();
                                }
                            }
                            break;
                            case model::PatrolEvent::BUFF: {
                                for (auto& drop :  evtTpl->dropList) {
                                    model::tpl::DropItem dropItem(drop.tpl,  drop.count);
                                    event.drops.emplace_back(dropItem);
                                }
                            }
                            break;
                            case model::PatrolEvent::REWARD: {
                                auto dropTpl = g_tploader->FindDrop(evtTpl->dropId);
                                if (dropTpl) {
                                    event.drops = dropTpl->DoDrop();
                                }
                            }
                            break;
                            default:
                                break;
                        }

                        events.push_back(event);
                    }
                };

                auto evtGroupTpl = tpl::m_tploader->FindCityPatrolEvtGroup(evtGroupId);
                if (evtGroupTpl) {
                    for (int i = 0; i < 9; ++i) {
                        auto evtTpl = evtGroupTpl->GetRandomEvent(true);
                        procEvt(evtTpl);

                        if (armyList->GetArmyTotal() <=  0) {
                            break;
                        }
                    }

                    if (armyList->GetArmyTotal() >  0) {
                        auto evtTpl = evtGroupTpl->GetRandomEvent(false);
                        procEvt(evtTpl);
                    }
                }

                msgqueue::MsgCityInfo cityInfo;
                cityInfo.SetData(city);
                troop->agent().msgQueue().AppendMsgCityPatrol(cityInfo, events);

                int patrolCD = g_tploader->configure().troopConfig.patrolCD;
                troop->agent().AddCityPatrolCount(1);
                troop->agent().AddCityPatrolCD(city->cityTpl()->cityId, g_dispatcher->GetTimestampCache() + patrolCD);

                troop->agent().proxy()->SendNoticeMessage(ErrorCode::TROOP_PATROLCITY_SUCCESS, 1);
            }

        }
    }
}
