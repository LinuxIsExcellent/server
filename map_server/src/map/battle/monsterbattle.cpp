#include "monsterbattle.h"
#include "../agent.h"
#include "../unit/monster.h"
#include "model/tpl/templateloader.h"
#include "model/tpl/configure.h"
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

            MonsterBattle::MonsterBattle(Troop* troop, Monster* monster)
                : Battle(model::BattleType::MONSTER, troop, monster), m_monster(monster)
            {
            }

            MonsterBattle::~MonsterBattle()
            {
            }

            void MonsterBattle::OnStart()
            {
                //attacker
                Agent& attAgent = m_atkTroop->agent();
                m_attackerInfo.SetData(attAgent);

                // defender
                m_defenderInfo.SetUnitData(m_monster);
            }

            void MonsterBattle::OnEnd(model::AttackType winner, int reportId, bool last_flag)
            {
                {
                     m_attackerInfo.detail.SetData(*m_atkTroop);
                }

                {
                    if (m_defArmyList) {
                        m_defenderInfo.detail.armyList = *m_defArmyList;
                    }
                }
                m_posName = m_monster->tpl().name;
                m_isLang = 1;
                //战斗结束时统计一下双方总兵力
                OutPutEndAmryCount();

                if (winner ==  model::AttackType::ATTACK) {
                    // 物品掉落
                    if (!m_defTroop) {
//                         auto dropTpl = g_tploader->FindDrop(2890010);
                        auto dropTpl = g_tploader->FindDrop(m_monster->tpl().dropId);
                        if (dropTpl) {
                            m_dorpItems = dropTpl->DoDrop();
                        }

                        LOG_DEBUG("Monster -------------- kill");
                        m_monster->RemoveSelf();
                        //g_map->OnKillMonster(m_monster);
                    }

                    Agent& attAgent = m_atkTroop->agent();
                    attAgent.AddBattleCount(winner,  model::AttackType::ATTACK);
                    //attAgent.msgQueue().AppendMsgAttackMonster(AttackType::ATTACK,  AttackType::ATTACK,  m_attackerInfo, m_defenderInfo, battleInitialData, m_monster->pos(), m_monster->tpl().id, m_dorpItems, m_monster->id(), m_atkTroop->id());
                    attAgent.msgQueue().AppendMsgAttackMonster(AttackType::ATTACK, AttackType::ATTACK, m_attackerInfo, 
                        m_defenderInfo, m_monster->pos(), m_monster->tpl().id, 100, 20, m_dorpItems, *m_atkArmyList, true, 
                        m_monster->id(), m_atkTroop->id(), reportId, m_reportInfo);
                    //如果战胜了高一个等级的怪物，则击败过的怪物最高等级加1
                    if( m_monster->level() == ( attAgent.GetMonsterDefeatedLevel() + 1 )){
                         attAgent.SetMonsterDefeatedLevel(m_monster->level());
                    }
                   
                } else {
                    Agent& attAgent = m_atkTroop->agent();
                    attAgent.AddBattleCount(winner,  model::AttackType::ATTACK);
                    //attAgent.msgQueue().AppendMsgAttackMonster(AttackType::DEFENSE,  AttackType::ATTACK,  m_attackerInfo, m_defenderInfo, battleInitialData, m_monster->pos(), m_monster->tpl().id, m_dorpItems, m_monster->id(), m_atkTroop->id());
                    attAgent.msgQueue().AppendMsgAttackMonster(AttackType::DEFENSE, AttackType::ATTACK, m_attackerInfo, 
                        m_defenderInfo, m_monster->pos(), m_monster->tpl().id, 100, 20, m_dorpItems, *m_atkArmyList, true, 
                        m_monster->id(), m_atkTroop->id(), reportId, m_reportInfo);
                }

                ResetArmyList();
            }
        }
    }
}
