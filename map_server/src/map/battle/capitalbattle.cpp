#include "capitalbattle.h"
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

            CapitalBattle::CapitalBattle(Troop* troop, Capital* city)
                : Battle(model::BattleType::PALACE_WAR, troop, city), m_capital(city)
            {
                m_unit = static_cast<Unit*>(m_capital);

                m_worldBossInfo = new MsgWorldBossInfo();
                m_worldBossInfo->tplId = m_capital->tpl().id;
                m_worldBossInfo->beginArmyCount = m_capital->CurrentArmyTotal();
                m_worldBossInfo->maxArmyCount = m_capital->ArmyListsTotal();

            }

            CapitalBattle::~CapitalBattle()
            {
                SAFE_DELETE(m_worldBossInfo);
            }

            void CapitalBattle::SetDefenseInput(engine::InitialDataInput& defenseInput)
            {
                Battle::SetDefenseInput(defenseInput);
                // if (!m_defTroop) {
                //     m_defCache = m_capital->armyList();
                //     m_defArmyList = &m_defCache;
                // }
                m_trapCache = m_capital->trapSet();
                m_defTrapSet = &m_trapCache;
                auto& battleConf = g_tploader->configure().battle;
                defenseInput.team.trapAtkPercentage = battleConf.traptake;
                defenseInput.team.turretAtkPower = m_capital->turretAtkPower();
            }

            void CapitalBattle::OnStart()
            {
                //attacker
                Agent& attAgent = m_atkTroop->agent();
                m_attackerInfo.SetData(attAgent);

                // defender
                if (m_defTroop) {
                    Agent& defAgent = m_defTroop->agent();
                    m_defenderInfo.SetData(defAgent);
                } else {
                    m_defenderInfo.SetUnitData(m_capital);
                }
            }

            void CapitalBattle::OnEnd(model::AttackType winner, int reportId, bool last_flag)
            {
                m_attackerInfo.detail.SetData(*m_atkTroop);

                if (m_defArmyList) {
                    //m_defenderInfo.detail.trapSet = m_trapCache;
                    m_defenderInfo.detail.armyList = *m_defArmyList;
                    // LOG_DEBUG("CityBatlle ------------- End !!!");
                    // m_defArmyList->Debug();
                }

                if (m_atkTroop != nullptr) {
                    g_mapMgr->AllianceCityBeAttacked(m_capital->allianceId(), m_capital, m_atkTroop);
                }
                m_posName = m_capital->tpl().name;
                m_isLang = 1;
                //战斗结束时统计一下双方总兵力
                OutPutEndAmryCount();
                
                if (winner ==  model::AttackType::ATTACK) {
                    if (m_defArmyList) {
                        if (m_atkTroop) {
                            //attacker
                            Agent& attAgent = m_atkTroop->agent();
                            attAgent.AddBattleCount(winner,  model::AttackType::ATTACK);
                            if (!m_capital->isOccupy()) {
                                auto dropTpl = g_tploader->FindDrop(m_capital->GetDropId());
                                if (dropTpl) {
                                    m_dorpItems = dropTpl->DoDrop();
                                }
                            }
                            // todo by zds  王城站和名城战要区分开
                            attAgent.msgQueue().AppendMsgAttackCity(AttackType::ATTACK,  AttackType::ATTACK,  m_attackerInfo, m_defenderInfo, reportId, m_capital->pos(),  m_capital->tpl().id, m_dorpItems,  m_capital->id(),  m_atkTroop->id(), m_reportInfo);
                        }
                        if (m_defTroop) {
                            Agent& defAgent = m_defTroop->agent();
                            defAgent.AddBattleCount(winner,  model::AttackType::DEFENSE);
                            defAgent.msgQueue().AppendMsgAttackCity(AttackType::ATTACK,  AttackType::DEFENSE,  m_attackerInfo, m_defenderInfo, reportId, m_capital->pos(),  m_capital->tpl().id, m_dorpItems,  m_capital->id(),  m_atkTroop->id(), m_reportInfo);
                        }

                        if (!m_defTroop) {
                            // 打羸了NPC，清空NPC部队
                            m_capital->ClearNpc();
                        }
                    }
                } else {
                    if (m_defArmyList) {
                        if (m_atkTroop) {
                            //attacker
                            Agent& attAgent = m_atkTroop->agent();
                            attAgent.AddBattleCount(winner,  model::AttackType::ATTACK);
                            attAgent.msgQueue().AppendMsgAttackCity(AttackType::DEFENSE,  AttackType::ATTACK,  m_attackerInfo, m_defenderInfo, reportId, m_capital->pos(),  m_capital->tpl().id, m_dorpItems,  m_capital->id(),  m_atkTroop->id(), m_reportInfo);
                        }
                        if (m_defTroop) {
                            Agent& defAgent = m_defTroop->agent();
                            defAgent.AddBattleCount(winner,  model::AttackType::DEFENSE);
                            defAgent.msgQueue().AppendMsgAttackCity(AttackType::DEFENSE,  AttackType::DEFENSE,  m_attackerInfo, m_defenderInfo, reportId, m_capital->pos(),  m_capital->tpl().id, m_dorpItems,  m_capital->id(),  m_atkTroop->id(), m_reportInfo);

                            m_capital->UpdateDefTroop();
                            m_capital->UpdateCityDefRecover();
                        }
                        if (!m_defTroop) {
                            // NPC赢了，计算npc受到的伤害
                            m_capital->HurtNpc();
                        }
                    }
                    
                }

                ResetArmyList();
            }

            void CapitalBattle::OnBattleEnd(model::AttackType winner)
            {
                if (winner ==  model::AttackType::ATTACK) {
                    // 所有NPC军队打完最后一级才有掉落奖励
                    // 物品掉落
                    /*if (!m_capital->isOccupy()) {
                        auto dropTpl = g_tploader->FindDrop(m_capital->cityTpl().dropId);
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
                        m_capital->AddCityDefense(-hurt);
                    }

                    if (m_capital->cityDefense() <=  0) {
                        if (!m_capital->isOccupy()) {
                            g_mapMgr->OnOccupyCity(m_capital);
                        }
                        if (m_capital->occupierId() == 0)
                        {
                            const Agent& atker = m_atkTroop->agent();
                            base::DataTable dt;
                            dt.Set(1, atker.headId());
                            dt.Set(2, atker.allianceName());
                            dt.Set(3, atker.nickname());
                            std::string param = dt.Serialize();
                            m_capital->AddPalaceWarRecord(PalaceWarRecordType::KILL_PALACE_NPC, param);
                        }                        
                        m_capital->Occupy(m_atkTroop);
                        m_atkTroop->SetIsOccupyCity(true);

                        auto &cityWallConfig = g_tploader->configure().cityWallConfig;
                        m_capital->AddCityDefense(m_capital->cityDefenseMax() * cityWallConfig.recovery);
                        m_capital->StartRecoverCityDefense();
                    }
                }
                else {
                    if (m_capital->occupierId() == 0)
                    {
                        //记录攻打王城(NPC状态)失败
                        const Agent& atker = m_atkTroop->agent();
                        base::DataTable dt;
                        dt.Set(1, atker.headId());
                        dt.Set(2, atker.allianceName());
                        dt.Set(3, atker.nickname());
                        std::string param = dt.Serialize();
                        m_capital->AddPalaceWarRecord(PalaceWarRecordType::ATTACK_NPC_FAIL, param);
                    }
                    else
                    {
                        //记录攻打王城(玩家占领状态)失败
                        const Agent& atker = m_atkTroop->agent();
                        const Agent& defer = *(m_capital->occupier());
                        base::DataTable dt;
                        dt.Set(1, atker.headId());
                        dt.Set(2, atker.allianceName());
                        dt.Set(3, atker.nickname());
                        dt.Set(4, defer.headId());
                        dt.Set(5, defer.allianceName());
                        dt.Set(6, defer.nickname());
                        std::string param = dt.Serialize();
                        m_capital->AddPalaceWarRecord(PalaceWarRecordType::ATTACK_PLAYER_FAIL, param);
                    }       
                }
                //如果刚开始的NPC数量大于0，则走NPC掉落逻辑
                if (m_worldBossInfo->beginArmyCount > 0)
                {
                    Agent& attAgent = m_atkTroop->agent();
                    // 在战斗结束的时候计算一下当前剩余的兵量
                    m_worldBossInfo->endArmyCount = m_capital->CurrentArmyTotal();
                    std::vector<model::tpl::DropItem> dropItems;
                    attAgent.msgQueue().AppendMsgAttachCityEnd(m_attackerInfo, m_defenderInfo, *m_worldBossInfo, dropItems, *m_atkArmyList, m_capital->id(), m_atkTroop->id());
                    if (winner ==  model::AttackType::ATTACK && m_capital->CurrentArmyTotal() == 0)
                    {
                        // 通知联盟
                        if (const AllianceSimple* alliance = g_mapMgr->alliance().FindAllianceSimple(attAgent.allianceId())) {
                            alliance->OnKillCityNpc(attAgent.nickname(), *m_capital);
                        }
                    }
                }
            }

        }
    }
}
