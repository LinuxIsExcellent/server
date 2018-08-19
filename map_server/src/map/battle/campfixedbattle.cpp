#include "campfixedbattle.h"
#include "../agent.h"
#include "../unit/camp.h"
#include "model/tpl/templateloader.h"
#include "model/tpl/configure.h"
#include  "../tpl/templateloader.h"

namespace ms
{
    namespace map
    {
        namespace battle
        {
            using namespace model;
            using namespace model::tpl;
            using namespace msgqueue;

            CampFixedBattle::CampFixedBattle(Troop* troop, CampFixed* campFixed)
                : Battle(model::BattleType::CAMP_FIXED, troop, campFixed), m_campFixed(campFixed)
            {
            }

            CampFixedBattle::~CampFixedBattle()
            {
            }

            void CampFixedBattle::OnStart()
            {
                //attacker
                Agent& attAgent = m_atkTroop->agent();
                m_attackerInfo.SetData(attAgent);

                // defender
                if (m_defTroop) {
                    Agent& defAgent = m_defTroop->agent();
                    m_defenderInfo.SetData(defAgent);
                    m_defenderInfo.castlePos = m_campFixed->pos();
                } else {
                }
            }

            void CampFixedBattle::OnEnd(model::AttackType winner, int reportId, bool last_flag)
            {
                {
                    m_attackerInfo.detail.SetData(*m_atkTroop);
                }

                {
                    if (m_defTroop) {
                        m_defenderInfo.detail.SetData(*m_defTroop);
                    }
                }
                m_posName = m_campFixed->tpl().name;
                m_isLang = 1;
                //战斗结束时统计一下双方总兵力
                OutPutEndAmryCount();
                if (m_atkTroop) {
                    MapTroopType troopType = m_atkTroop->type();
                    if (winner ==  model::AttackType::ATTACK) {
                        //attacker
                        if (m_atkTroop) {
                            Agent& attAgent = m_atkTroop->agent();
                            attAgent.AddBattleCount(winner,  model::AttackType::ATTACK);
                            if (troopType ==  MapTroopType::CAMP_FIXED_OCCUPY) {
                                // AppendMsgOccupyCampFixed
                                attAgent.msgQueue().AppendMsgAttackCampFixed(AttackType::ATTACK,  AttackType::ATTACK,  m_attackerInfo, m_defenderInfo, reportId,  m_campFixed->pos(), m_campFixed->tpl().id, m_campFixed->id(),  m_atkTroop->id(), m_reportInfo);
                            } else if (troopType ==  MapTroopType::CAMP_FIXED_ATTACK) {
                                attAgent.msgQueue().AppendMsgAttackCampFixed(AttackType::ATTACK,  AttackType::ATTACK,  m_attackerInfo, m_defenderInfo, reportId,  m_campFixed->pos(), m_campFixed->tpl().id,  m_campFixed->id(),  m_atkTroop->id(), m_reportInfo);
                            }
                        }
                        if (m_defTroop) {
                            Agent& defAgent = m_defTroop->agent();
                            defAgent.AddBattleCount(winner,  model::AttackType::DEFENSE);
                            if (troopType ==  MapTroopType::CAMP_FIXED_OCCUPY) {
                                defAgent.msgQueue().AppendMsgAttackCampFixed(AttackType::ATTACK,  AttackType::DEFENSE,  m_attackerInfo, m_defenderInfo, reportId,  m_campFixed->pos(), m_campFixed->tpl().id, m_campFixed->id(),  m_atkTroop->id(), m_reportInfo);
                            } else if (troopType ==  MapTroopType::CAMP_FIXED_ATTACK) {
                                defAgent.msgQueue().AppendMsgAttackCampFixed(AttackType::ATTACK,  AttackType::DEFENSE,  m_attackerInfo, m_defenderInfo, reportId,  m_campFixed->pos(), m_campFixed->tpl().id, m_campFixed->id(),  m_atkTroop->id(), m_reportInfo);
                            }
                        }
                    } else {
                        //attacker
                        if (m_atkTroop) {
                            Agent& attAgent = m_atkTroop->agent();
                            attAgent.AddBattleCount(winner,  model::AttackType::ATTACK);
                            if (troopType ==  MapTroopType::CAMP_FIXED_OCCUPY) {
                                attAgent.msgQueue().AppendMsgAttackCampFixed(AttackType::DEFENSE,  AttackType::ATTACK,  m_attackerInfo, m_defenderInfo, reportId,  m_campFixed->pos(), m_campFixed->tpl().id, m_campFixed->id(),  m_atkTroop->id(), m_reportInfo);
                            } else if (troopType ==  MapTroopType::CAMP_FIXED_ATTACK) {
                                attAgent.msgQueue().AppendMsgAttackCampFixed(AttackType::DEFENSE,  AttackType::ATTACK,  m_attackerInfo, m_defenderInfo, reportId,  m_campFixed->pos(), m_campFixed->tpl().id, m_campFixed->id(),  m_atkTroop->id(), m_reportInfo);
                            }
                        }
                        if (m_defTroop) {
                            Agent& defAgent = m_defTroop->agent();
                            defAgent.AddBattleCount(winner,  model::AttackType::DEFENSE);
                            if (troopType ==  MapTroopType::CAMP_FIXED_OCCUPY) {
                                defAgent.msgQueue().AppendMsgAttackCampFixed(AttackType::DEFENSE,  AttackType::DEFENSE,  m_attackerInfo, m_defenderInfo, reportId,  m_campFixed->pos(), m_campFixed->tpl().id, m_campFixed->id(),  m_atkTroop->id(), m_reportInfo);
                            } else if (troopType ==  MapTroopType::CAMP_FIXED_ATTACK) {
                                defAgent.msgQueue().AppendMsgAttackCampFixed(AttackType::DEFENSE,  AttackType::DEFENSE,  m_attackerInfo, m_defenderInfo, reportId,  m_campFixed->pos(), m_campFixed->tpl().id, m_campFixed->id(),  m_atkTroop->id(), m_reportInfo);
                            }
                        }
                    }
                }

                ResetArmyList();
            }

            void CampFixedBattle::OnBattleEnd(model::AttackType winner)
            {
                if (winner ==  model::AttackType::ATTACK) {

                    // 攻击耐久值
                    int hurt = 0;
                    if (m_atkArmyList) {
                        for (auto &army :  m_atkArmyList->armies()) {
                            ArmyInfo& armyInfo = army.second.armyInfo();
                            int count = armyInfo.count(ArmyState::NORMAL);
                            hurt += count * armyInfo.wallAttack();
                        }
                    }

                    // 消耗耐久值
                    if (hurt > 0) {
                        m_campFixed->AddCampFixedDurable(-hurt);
                    }

                    /*if (m_campFixed->campFixedDurable() <= 0)
                    {
                        
                        // 击败行营就算是直接拆掉
                         // m_campFixed->Occupy(m_atkTroop);

                        if (m_atkTroop) {
                            Agent& attAgent = m_atkTroop->agent();
                            attAgent.AddBattleCount(winner,  model::AttackType::ATTACK);
                            if (m_atkTroop->type() ==  MapTroopType::CAMP_FIXED_OCCUPY) {
                                attAgent.msgQueue().AppendMsgOccupyCampFixed(AttackType::ATTACK,  AttackType::ATTACK,  m_attackerInfo, m_defenderInfo, 0,
                                    m_campFixed->pos(), m_campFixed->tpl().id, m_campFixed->id(),  m_atkTroop->id());
                            }
                        }
                        if (m_defTroop) {
                            Agent& defAgent = m_defTroop->agent();
                            defAgent.AddBattleCount(winner,  model::AttackType::DEFENSE);
                            if (m_atkTroop->type() ==  MapTroopType::CAMP_FIXED_OCCUPY) {
                                defAgent.msgQueue().AppendMsgOccupyCampFixed(AttackType::ATTACK,  AttackType::DEFENSE,  m_attackerInfo, m_defenderInfo, 0,
                                     m_campFixed->pos(), m_campFixed->tpl().id, m_campFixed->id(),  m_atkTroop->id());
                            }
                        }   
                    } */
                } else {
                    //attacker
                    /*if (m_atkTroop) {
                        Agent& attAgent = m_atkTroop->agent();
                        attAgent.AddBattleCount(winner,  model::AttackType::ATTACK);
                        if (m_atkTroop->type() ==  MapTroopType::CAMP_FIXED_OCCUPY) {
                            attAgent.msgQueue().AppendMsgOccupyCampFixed(AttackType::DEFENSE,  AttackType::ATTACK,  m_attackerInfo, m_defenderInfo, 0,
                                m_campFixed->pos(), m_campFixed->tpl().id, m_campFixed->id(),  m_atkTroop->id());
                        }
                    }
                    if (m_defTroop) {
                        Agent& defAgent = m_defTroop->agent();
                        defAgent.AddBattleCount(winner,  model::AttackType::DEFENSE);
                        if (m_atkTroop->type() ==  MapTroopType::CAMP_FIXED_OCCUPY) {
                            defAgent.msgQueue().AppendMsgOccupyCampFixed(AttackType::DEFENSE,  AttackType::DEFENSE,  m_attackerInfo, m_defenderInfo, 0,
                                m_campFixed->pos(), m_campFixed->tpl().id, m_campFixed->id(),  m_atkTroop->id());
                        }
                    }*/
                }
            }
        }
    }
}
