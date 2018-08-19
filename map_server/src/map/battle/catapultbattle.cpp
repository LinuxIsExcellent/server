#include "catapultbattle.h"
#include "../agent.h"
#include "../unit/catapult.h"
#include "../unit/castle.h"
#include "../alliance.h"
#include "model/tpl/templateloader.h"
#include "model/tpl/configure.h"

namespace ms
{
    namespace map
    {
        namespace battle
        {
            using namespace std;
            using namespace model::tpl;
            using namespace msgqueue;

            CatapultBattle::CatapultBattle(Troop* troop, Catapult* catapult)
                : Battle(model::BattleType::CATAPULT, troop, catapult), m_catapult(catapult)
            {
                m_worldBossInfo = new MsgWorldBossInfo();
                m_worldBossInfo->tplId = m_catapult->tpl().id;
                m_worldBossInfo->beginArmyCount = m_catapult->CurrentArmyTotal();
                m_worldBossInfo->maxArmyCount = m_catapult->ArmyListsTotal();
            }

            CatapultBattle::~CatapultBattle()
            {
                SAFE_DELETE(m_worldBossInfo);
            }

            void CatapultBattle::SetDefenseInput(engine::InitialDataInput& defenseInput)
            {
                Battle::SetDefenseInput(defenseInput);
               /* if (!m_defTroop) {
                    m_defCache = m_catapult->armyList();
                    m_defArmyList = &m_defCache;
                }*/
                //m_trapCache = m_catapult->trapSet();
                m_defArmyList = m_catapult->DefArmyList();
                auto& battleConf = g_tploader->configure().battle;
                defenseInput.team.trapAtkPercentage = battleConf.traptake;
                //defenseInput.team.turretAtkPower = m_catapult->turretAtkPower();
            }

            void CatapultBattle::OnStart()
            {
                //attacker
                Agent& attAgent = m_atkTroop->agent();
                m_attackerInfo.SetData(attAgent);

                // defender
                if (m_defTroop) {
                    Agent& defAgent = m_defTroop->agent();
                    m_defenderInfo.SetData(defAgent);
                } else {
                    m_defenderInfo.SetUnitData(m_catapult);
                }
            }

            void CatapultBattle::OnEnd(model::AttackType winner, int reportId, bool last_flag)
            {
                m_attackerInfo.detail.SetData(*m_atkTroop);

                if (m_defArmyList) {
                    m_defenderInfo.detail.armyList = *m_defArmyList;
                }

                if (m_atkTroop != nullptr) {
                    g_mapMgr->AllianceCatapultBeAttacked(m_catapult->occupierId(), m_catapult, m_atkTroop);
                }
                m_posName = m_catapult->tpl().name;
                m_isLang = 1;
                //战斗结束时统计一下双方总兵力
                OutPutEndAmryCount();

                if (winner ==  model::AttackType::ATTACK) {
                    if (m_defArmyList) {
                        if (m_atkTroop) {
                            if (!m_catapult->isOccupy()) {
                                auto dropTpl = g_tploader->FindDrop(m_catapult->GetDropId());
                                if (dropTpl) {
                                    m_dorpItems = dropTpl->DoDrop();
                                }
                            }
                            //attacker
                            Agent& attAgent = m_atkTroop->agent();
                            attAgent.AddBattleCount(winner,  model::AttackType::ATTACK);
                            attAgent.msgQueue().AppendMsgAttackCatapult(AttackType::ATTACK,  AttackType::ATTACK,  m_attackerInfo, m_defenderInfo, m_dorpItems, reportId, m_catapult->id(),  m_atkTroop->id(), m_reportInfo);
                        }
                        if (m_defTroop) {
                            Agent& defAgent = m_defTroop->agent();
                            defAgent.AddBattleCount(winner,  model::AttackType::DEFENSE);
                            defAgent.msgQueue().AppendMsgAttackCatapult(AttackType::ATTACK,  AttackType::DEFENSE,  m_attackerInfo, m_defenderInfo, m_dorpItems, reportId, m_catapult->id(),  m_atkTroop->id(), m_reportInfo);
                        }

                        if (!m_defTroop) {
                            // 打羸了NPC，清空NPC部队
                            m_catapult->ClearNpc();
                        }
                    }
                } else {
                    if (m_defArmyList) {
                        if (m_atkTroop) {
                            //attacker
                            Agent& attAgent = m_atkTroop->agent();
                            attAgent.AddBattleCount(winner,  model::AttackType::ATTACK);
                            attAgent.msgQueue().AppendMsgAttackCatapult(AttackType::DEFENSE,  AttackType::ATTACK,  m_attackerInfo, m_defenderInfo, m_dorpItems, reportId,  m_catapult->id(),  m_atkTroop->id(), m_reportInfo);
                        }
                        if (m_defTroop) {
                            Agent& defAgent = m_defTroop->agent();
                            defAgent.AddBattleCount(winner,  model::AttackType::DEFENSE);
                            defAgent.msgQueue().AppendMsgAttackCatapult(AttackType::DEFENSE,  AttackType::DEFENSE,  m_attackerInfo, m_defenderInfo, m_dorpItems, reportId,  m_catapult->id(),  m_atkTroop->id(), m_reportInfo);

                            m_catapult->UpdateDefTroop();
                        }
                    }
                    
                }

                ResetArmyList();
            }

            void CatapultBattle::OnBattleEnd(model::AttackType winner)
            {
                if (winner ==  model::AttackType::ATTACK) {
                    // 物品掉落
                    if (!m_catapult->isOccupy()) {
                        auto dropTpl = g_tploader->FindDrop(m_catapult->tpl().dropId);
                        if (dropTpl) {
                            m_dorpItems = dropTpl->DoDrop();
                        }
                    }
                    if (m_catapult->occupierId() == 0)
                    {
                        const Agent& atker = m_atkTroop->agent();
                        base::DataTable dt;
                        dt.Set(1, atker.headId());
                        dt.Set(2, atker.allianceName());
                        dt.Set(3, atker.nickname());
                        string param = dt.Serialize();
                        m_catapult->AddPalaceWarRecord(PalaceWarRecordType::KILL_CATAPULT_NPC, param);
                    }
                    //NPC军队为0的时候才能占领
                    if (m_catapult->CurrentArmyTotal() == 0) {
                        m_catapult->Occupy(m_atkTroop);
                        m_atkTroop->SetIsOccupyCity(true);
                    }
                }
                else{
                    if (m_catapult->occupierId() == 0)
                    {
                        //记录攻打箭塔(NPC状态)失败
                        const Agent& atker = m_atkTroop->agent();
                        base::DataTable dt;
                        dt.Set(1, atker.headId());
                        dt.Set(2, atker.allianceName());
                        dt.Set(3, atker.nickname());
                        string param = dt.Serialize();
                        m_catapult->AddPalaceWarRecord(PalaceWarRecordType::ATTACK_CATAPULT_NPC_FAIL, param);
                    }
                    else
                    {
                        //记录攻打箭塔(玩家占领状态)失败
                        const Agent& atker = m_atkTroop->agent();
                        const Agent& defer = *(m_catapult->occupier());
                        base::DataTable dt;
                        dt.Set(1, atker.headId());
                        dt.Set(2, atker.allianceName());
                        dt.Set(3, atker.nickname());
                        dt.Set(4, defer.headId());
                        dt.Set(5, defer.allianceName());
                        dt.Set(6, defer.nickname());
                        string param = dt.Serialize();
                        m_catapult->AddPalaceWarRecord(PalaceWarRecordType::ATTACK_CATAPULT_PLAYER_FAIL, param);
                    }
                }

                //如果刚开始的NPC数量大于0，则走NPC掉落逻辑
                if (m_worldBossInfo->beginArmyCount > 0)
                {
                    m_worldBossInfo->endArmyCount = m_catapult->CurrentArmyTotal(); 
                    Agent& attAgent = m_atkTroop->agent();
                    // 在战斗结束的时候计算一下当前剩余的兵量
                    m_worldBossInfo->endArmyCount = m_catapult->CurrentArmyTotal();
                    attAgent.msgQueue().AppendMsgAttachCatapultEnd(m_attackerInfo, m_defenderInfo, *m_worldBossInfo, m_catapult->id(), m_atkTroop->id());
                    if (winner ==  model::AttackType::ATTACK && m_catapult->CurrentArmyTotal() == 0)
                    {
                        // 通知联盟
                        if (const AllianceSimple* alliance = g_mapMgr->alliance().FindAllianceSimple(attAgent.allianceId())) {
                            alliance->OnKillCatapultNpc(attAgent.nickname(), *m_catapult);
                        }
                    }
                } 
            }

        }
    }
}
