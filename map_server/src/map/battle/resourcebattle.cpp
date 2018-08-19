#include "resourcebattle.h"
#include "../agent.h"
#include "../unit/resnode.h"
#include "model/tpl/templateloader.h"
#include "model/tpl/configure.h"

namespace ms
{
    namespace map
    {
        namespace battle
        {
            using namespace model;
            using namespace model::tpl;
            using namespace msgqueue;

            ResourceBattle::ResourceBattle(Troop* troop, ResNode* res)
                : Battle(model::BattleType::GATHER, troop, res), m_resNode(res)
            {
            }

            ResourceBattle::~ResourceBattle()
            {
            }

            void ResourceBattle::OnStart()
            {
                //attacker
                Agent& attAgent = m_atkTroop->agent();
                m_attackerInfo.SetData(attAgent);

                // defender
                if (m_defTroop) {
                    Agent& defAgent = m_defTroop->agent();
                    m_defenderInfo.SetData(defAgent);
                } else {
                    m_defenderInfo.SetUnitData(m_resNode);
                }
            }

            void ResourceBattle::OnEnd(model::AttackType winner, int reportId, bool last_flag)
            {
                {
                     m_attackerInfo.detail.SetData(*m_atkTroop);
                }

                {
                    if (m_defArmyList) {
                        m_defenderInfo.detail.armyList = *m_defArmyList;
                    }
                }
                m_posName = m_resNode->tpl().name;
                m_isLang = 1;
                //战斗结束时统计一下双方总兵力
                OutPutEndAmryCount();
                if (winner ==  model::AttackType::ATTACK) {
                    // 物品掉落
                    if (!m_defTroop) {
//                         auto dropTpl = g_tploader->FindDrop(2890010);
                        auto dropTpl = g_tploader->FindDrop(m_resNode->tpl().dropId);
                        if (dropTpl) {
                            m_dorpItems = dropTpl->DoDrop();
                        }

                        g_mapMgr->OnOccupyRes(m_resNode);
                    }

                    Agent& attAgent = m_atkTroop->agent();
                    if (m_defTroop) {
                        attAgent.AddBattleCount(winner,  model::AttackType::ATTACK);
                        attAgent.msgQueue().AppendMsgAttackResource(AttackType::ATTACK,  AttackType::ATTACK,  m_attackerInfo, m_defenderInfo, reportId, m_resNode->pos(), m_resNode->tpl().id, m_dorpItems, m_resNode->id(), m_atkTroop->id(), m_reportInfo);
                        Agent& defAgent = m_defTroop->agent();
                        defAgent.AddBattleCount(winner,  model::AttackType::DEFENSE);
                        defAgent.msgQueue().AppendMsgAttackResource(AttackType::ATTACK,  AttackType::DEFENSE,  m_attackerInfo, m_defenderInfo, reportId, m_resNode->pos(), m_resNode->tpl().id, m_dorpItems, m_resNode->id(), m_atkTroop->id(), m_reportInfo);
                    }
                } else {
                    Agent& attAgent = m_atkTroop->agent();
                    attAgent.AddBattleCount(winner,  model::AttackType::ATTACK);
                    attAgent.msgQueue().AppendMsgAttackResource(AttackType::DEFENSE,  AttackType::ATTACK,  m_attackerInfo, m_defenderInfo, reportId, m_resNode->pos(), m_resNode->tpl().id, m_dorpItems, m_resNode->id(), m_atkTroop->id(), m_reportInfo);
                    if (m_defTroop) {
                        Agent& defAgent = m_defTroop->agent();
                        defAgent.AddBattleCount(winner,  model::AttackType::DEFENSE);
                        defAgent.msgQueue().AppendMsgAttackResource(AttackType::DEFENSE,  AttackType::DEFENSE,  m_attackerInfo, m_defenderInfo, reportId, m_resNode->pos(), m_resNode->tpl().id, m_dorpItems, m_resNode->id(), m_atkTroop->id(), m_reportInfo);
                    }
                }

                ResetArmyList();
            }

        }
    }
}
