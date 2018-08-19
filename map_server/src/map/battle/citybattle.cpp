#include "citybattle.h"
#include "../agent.h"
#include "../unit/famouscity.h"
#include "../unit/palace.h"
#include "model/tpl/templateloader.h"
#include "model/tpl/configure.h"
#include  "../tpl/templateloader.h"
#include "../tpl/npcarmytpl.h"
#include "../alliance.h"
#include <base/logger.h>

namespace ms
{
    namespace map
    {
        namespace battle
        {
            using namespace model;
            using namespace model::tpl;
            using namespace msgqueue;

            CityBattle::CityBattle(Troop* troop, FamousCity* city)
                : Battle(model::BattleType::SIEGE_CITY, troop, city), m_city(city)
            {
                m_unit = static_cast<Unit*>(m_city);

                m_worldBossInfo = new MsgWorldBossInfo();
                m_worldBossInfo->tplId = m_city->tpl().id;
                m_worldBossInfo->beginArmyCount = m_city->CurrentArmyTotal();
                m_worldBossInfo->maxArmyCount = m_city->ArmyListsTotal();

            }

            CityBattle::~CityBattle()
            {
                SAFE_DELETE(m_worldBossInfo);
            }

            void CityBattle::SetDefenseInput(engine::InitialDataInput& defenseInput)
            {
                Battle::SetDefenseInput(defenseInput);
                // if (!m_defTroop) {
                //     m_defCache = m_city->armyList();
                //     m_defArmyList = &m_defCache;
                // }
                m_trapCache = m_city->trapSet();
                m_defTrapSet = &m_trapCache;
                auto& battleConf = g_tploader->configure().battle;
                defenseInput.team.trapAtkPercentage = battleConf.traptake;
                defenseInput.team.turretAtkPower = m_city->turretAtkPower();
            }

            void CityBattle::OnStart()
            {
                //attacker
                Agent& attAgent = m_atkTroop->agent();
                m_attackerInfo.SetData(attAgent);

                // defender
                if (m_defTroop) {
                    Agent& defAgent = m_defTroop->agent();
                    m_defenderInfo.SetData(defAgent);
                } else {
                    m_defenderInfo.SetUnitData(m_city);
                }
            }

            void CityBattle::OnEnd(model::AttackType winner, int reportId, bool last_flag)
            {
                m_attackerInfo.detail.SetData(*m_atkTroop);

                if (m_defArmyList) {
                    //m_defenderInfo.detail.trapSet = m_trapCache;
                    m_defenderInfo.detail.armyList = *m_defArmyList;
                    // LOG_DEBUG("CityBatlle ------------- End !!!");
                    // m_defArmyList->Debug();
                }

                if (m_atkTroop != nullptr) {
                    g_mapMgr->AllianceCityBeAttacked(m_city->allianceId(), m_city, m_atkTroop);
                }
                m_posName = m_city->tpl().name;
                m_isLang = 1;
                //战斗结束时统计一下双方总兵力
                OutPutEndAmryCount();
                
                if (winner ==  model::AttackType::ATTACK) {
                    if (m_defArmyList) {
                        if (m_atkTroop) {
                            //attacker
                            Agent& attAgent = m_atkTroop->agent();
                            attAgent.AddBattleCount(winner,  model::AttackType::ATTACK);
                            if (!m_city->isOccupy()) {
                                auto dropTpl = g_tploader->FindDrop(m_city->GetDropId());
                                if (dropTpl) {
                                    m_dorpItems = dropTpl->DoDrop();
                                }
                            }
                            attAgent.msgQueue().AppendMsgAttackCity(AttackType::ATTACK,  AttackType::ATTACK,  m_attackerInfo, m_defenderInfo, reportId, m_city->pos(),  m_city->tpl().id, m_dorpItems,  m_city->id(),  m_atkTroop->id(), m_reportInfo);
                        }
                        if (m_defTroop) {
                            Agent& defAgent = m_defTroop->agent();
                            defAgent.AddBattleCount(winner,  model::AttackType::DEFENSE);
                            defAgent.msgQueue().AppendMsgAttackCity(AttackType::ATTACK,  AttackType::DEFENSE,  m_attackerInfo, m_defenderInfo, reportId, m_city->pos(),  m_city->tpl().id, m_dorpItems,  m_city->id(),  m_atkTroop->id(), m_reportInfo);
                        }

                        if (!m_defTroop) {
                            // 打羸了NPC，清空NPC部队
                            m_city->ClearNpc();
                        }
                    }
                } else {
                    if (m_defArmyList) {
                        if (m_atkTroop) {
                            //attacker
                            Agent& attAgent = m_atkTroop->agent();
                            attAgent.AddBattleCount(winner,  model::AttackType::ATTACK);
                            attAgent.msgQueue().AppendMsgAttackCity(AttackType::DEFENSE,  AttackType::ATTACK,  m_attackerInfo, m_defenderInfo, reportId, m_city->pos(),  m_city->tpl().id, m_dorpItems,  m_city->id(),  m_atkTroop->id(), m_reportInfo);
                        }
                        if (m_defTroop) {
                            Agent& defAgent = m_defTroop->agent();
                            defAgent.AddBattleCount(winner,  model::AttackType::DEFENSE);
                            defAgent.msgQueue().AppendMsgAttackCity(AttackType::DEFENSE,  AttackType::DEFENSE,  m_attackerInfo, m_defenderInfo, reportId, m_city->pos(),  m_city->tpl().id, m_dorpItems,  m_city->id(),  m_atkTroop->id(), m_reportInfo);

                            m_city->UpdateDefTroop();
                            m_city->UpdateCityDefRecover();
                        }
                        if (!m_defTroop) {
                            // NPC赢了，计算npc受到的伤害
                            m_city->HurtNpc();
                        }
                    }
                    
                }

                ResetArmyList();
            }

            void CityBattle::OnBattleEnd(model::AttackType winner)
            {
                if (winner ==  model::AttackType::ATTACK) {
                    // 所有NPC军队打完最后一级才有掉落奖励
                    // 物品掉落
                    /*if (!m_city->isOccupy()) {
                        auto dropTpl = g_tploader->FindDrop(m_city->cityTpl().dropId);
                        if (dropTpl) {
                            m_dorpItems = dropTpl->DoDrop();
                        }
                    }*/

                    // 攻击城墙
                    int hurt = 0;
                    if (m_atkArmyList) {
                        for (auto &army :  m_atkArmyList->armies()) {
                            ArmyInfo& armyInfo = army.second.armyInfo();
                            int count = armyInfo.count(ArmyState::NORMAL);
                            hurt += count * armyInfo.wallAttack();
                        }
                    }

                    // 消耗城防值
                    if (hurt > 0) {
                        m_city->AddCityDefense(-hurt);
                    }

                    if (m_city->cityDefense() <=  0) {
                        if (!m_city->isOccupy()) {
                            g_mapMgr->OnOccupyCity(m_city);
                        }
                        m_city->Occupy(m_atkTroop);
                        m_atkTroop->SetIsOccupyCity(true);

                        auto &cityWallConfig = g_tploader->configure().cityWallConfig;
                        m_city->AddCityDefense(m_city->cityDefenseMax() * cityWallConfig.recovery);
                        m_city->StartRecoverCityDefense();
                    }
                }
                //如果刚开始的NPC数量大于0，则走NPC掉落逻辑
                if (m_worldBossInfo->beginArmyCount > 0)
                {
                    Agent& attAgent = m_atkTroop->agent();
                    // 在战斗结束的时候计算一下当前剩余的兵量
                    m_worldBossInfo->endArmyCount = m_city->CurrentArmyTotal();
                    std::vector<model::tpl::DropItem> dropItems;
                    attAgent.msgQueue().AppendMsgAttachCityEnd(m_attackerInfo, m_defenderInfo, *m_worldBossInfo, dropItems, *m_atkArmyList, m_city->id(), m_atkTroop->id());
                    if (winner ==  model::AttackType::ATTACK && m_city->CurrentArmyTotal() == 0)
                    {
                        // 通知联盟
                        if (const AllianceSimple* alliance = g_mapMgr->alliance().FindAllianceSimple(attAgent.allianceId())) {
                            alliance->OnKillCityNpc(attAgent.nickname(), *m_city);
                        }
                    }
                }
            }

        }
    }
}
