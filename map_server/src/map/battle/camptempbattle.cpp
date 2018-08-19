#include "camptempbattle.h"
#include "../agent.h"
#include "../unit/camp.h"

namespace ms
{
    namespace map
    {
        namespace battle
        {
            using namespace model;
            using namespace model::tpl;
            using namespace msgqueue;

            CampTempBattle::CampTempBattle(Troop* troop, CampTemp* campTemp)
                : Battle(model::BattleType::CAMP_TEMP, troop, campTemp), m_campTemp(campTemp)
            {
            }

            CampTempBattle::~CampTempBattle()
            {
            }

            void CampTempBattle::OnStart()
            {
                //attacker
                Agent& attAgent = m_atkTroop->agent();
                m_attackerInfo.SetData(attAgent);

                // defender
                if (m_defTroop) {
                    Agent& defAgent = m_defTroop->agent();
                    m_defenderInfo.SetData(defAgent);
                    m_defenderInfo.castlePos = m_campTemp->pos();
                } else {
                }
            }

            void CampTempBattle::OnEnd(model::AttackType winner, int reportId, bool last_flag)
            {
                {
                    m_attackerInfo.detail.SetData(*m_atkTroop);
                }

                {
                    if (m_defTroop) {
                        m_defenderInfo.detail.SetData(*m_defTroop);
                    }
                }
                m_posName = "Worldmap_SPACE";
                m_isLang = 1;
                //战斗结束时统计一下双方总兵力
                OutPutEndAmryCount();
                if (winner ==  model::AttackType::ATTACK) {
                    //attacker
                    Agent& attAgent = m_atkTroop->agent();
                    attAgent.AddBattleCount(winner,  model::AttackType::ATTACK);
                    attAgent.msgQueue().AppendMsgAttackCampTemp(AttackType::ATTACK,  AttackType::ATTACK,  m_attackerInfo, m_defenderInfo, reportId,  m_campTemp->pos(),  m_campTemp->tpl().id,  m_campTemp->id(),  m_atkTroop->id(), m_reportInfo);
                    if (m_defTroop) {
                        Agent& defAgent = m_defTroop->agent();
                        defAgent.AddBattleCount(winner,  model::AttackType::DEFENSE);
                        defAgent.msgQueue().AppendMsgAttackCampTemp(AttackType::ATTACK,  AttackType::DEFENSE,  m_attackerInfo, m_defenderInfo, reportId,  m_campTemp->pos(),  m_campTemp->tpl().id, m_campTemp->id(),  m_atkTroop->id(), m_reportInfo);
                    }
                } else {
                    //attacker
                    Agent& attAgent = m_atkTroop->agent();
                    attAgent.AddBattleCount(winner,  model::AttackType::ATTACK);
                    attAgent.msgQueue().AppendMsgAttackCampTemp(AttackType::DEFENSE,  AttackType::ATTACK,  m_attackerInfo, m_defenderInfo, reportId,  m_campTemp->pos(),  m_campTemp->tpl().id, m_campTemp->id(),  m_atkTroop->id(), m_reportInfo);
                    if (m_defTroop) {
                        Agent& defAgent = m_defTroop->agent();
                        defAgent.AddBattleCount(winner,  model::AttackType::DEFENSE);
                        defAgent.msgQueue().AppendMsgAttackCampTemp(AttackType::DEFENSE,  AttackType::DEFENSE,  m_attackerInfo, m_defenderInfo, reportId,  m_campTemp->pos(),  m_campTemp->tpl().id, m_campTemp->id(),  m_atkTroop->id(), m_reportInfo);
                    }
                }

                ResetArmyList();
            }

        }
    }
}
