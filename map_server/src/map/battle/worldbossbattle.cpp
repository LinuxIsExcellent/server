#include "worldbossbattle.h"
#include "../agent.h"
#include "../unit/worldboss.h"
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

            WorldBossBattle::WorldBossBattle(Troop* troop, WorldBoss* worldboss)
                : Battle(model::BattleType::WORLDBOSS, troop, worldboss), m_worldboss(worldboss)
            {
                //记录世界BOSS战斗开始数据信息
                m_worldBossInfo = new MsgWorldBossInfo();
                m_worldBossInfo->tplId = m_worldboss->tpl().id;
                m_worldBossInfo->beginArmyCount = m_worldboss->CurrentArmyTotal();
                m_worldBossInfo->maxArmyCount = m_worldboss->ArmyListsTotal();
            }

            WorldBossBattle::~WorldBossBattle()
            {
            	SAFE_DELETE(m_worldBossInfo);
            }

            void WorldBossBattle::SetDefenseInput(engine::InitialDataInput& defenseInput)
            {
                Battle::SetDefenseInput(defenseInput);
               /* if (!m_defTroop) {
                    m_defCache = m_catapult->armyList();
                    m_defArmyList = &m_defCache;
                }*/
                //m_trapCache = m_catapult->trapSet();
                m_defArmyList = m_worldboss->DefArmyList();
                auto& battleConf = g_tploader->configure().battle;
                defenseInput.team.trapAtkPercentage = battleConf.traptake;
                //defenseInput.team.turretAtkPower = m_catapult->turretAtkPower();
            }

            void WorldBossBattle::OnStart()
            {
                //attacker
                Agent& attAgent = m_atkTroop->agent();
                m_attackerInfo.SetData(attAgent);

                // defender
                if (m_defTroop) {
                    Agent& defAgent = m_defTroop->agent();
                    m_defenderInfo.SetData(defAgent);
                } else {
                    m_defenderInfo.SetUnitData(m_worldboss);
                }
            }

            void WorldBossBattle::OnEnd(model::AttackType winner, int reportId, bool last_flag)
            {
                m_attackerInfo.detail.SetData(*m_atkTroop);

                if (m_defArmyList) {
                    m_defenderInfo.detail.armyList = *m_defArmyList;
                }

                if (m_atkTroop != nullptr) {
                    // g_mapMgr->AllianceCatapultBeAttacked(0, m_worldboss, m_atkTroop);
                }
                m_posName = m_worldboss->tpl().name;
                m_isLang = 1;
                //战斗结束时统计一下双方总兵力
                OutPutEndAmryCount();

                if (winner ==  model::AttackType::ATTACK) {
                    if (m_defArmyList) {
                        if (m_atkTroop) {
                            //attacker
                            Agent& attAgent = m_atkTroop->agent();
                            attAgent.AddBattleCount(winner,  model::AttackType::ATTACK);
                            attAgent.msgQueue().AppendMsgAttackWorldBoss(AttackType::ATTACK,  AttackType::ATTACK,  m_attackerInfo, m_defenderInfo, reportId, m_worldboss->id(),  m_atkTroop->id(), m_reportInfo);
                        }
                    }
                } else {
                    if (m_defArmyList) {
                        if (m_atkTroop) {
                            //attacker
                            Agent& attAgent = m_atkTroop->agent();
                            attAgent.AddBattleCount(winner,  model::AttackType::ATTACK);
                            attAgent.msgQueue().AppendMsgAttackWorldBoss(AttackType::DEFENSE,  AttackType::ATTACK,  m_attackerInfo, m_defenderInfo,  reportId, m_worldboss->id(),  m_atkTroop->id(), m_reportInfo);
                        }
                    }
                    
                }
                ResetArmyList();
            }

            void WorldBossBattle::OnBattleEnd(model::AttackType winner)
            {
            	//这里做战斗结算
          		//如果当前
          		Agent& attAgent = m_atkTroop->agent();
                // 在战斗结束的时候计算一下当前剩余的兵量
                m_worldBossInfo->endArmyCount = m_worldboss->CurrentArmyTotal();
            	attAgent.msgQueue().AppendMsgAttackWorldBossEnd(m_attackerInfo, m_defenderInfo, *m_worldBossInfo, m_worldboss->id(), m_atkTroop->id());
            	if (winner ==  model::AttackType::ATTACK && m_worldboss->CurrentArmyTotal() == 0)
            	{
            		// 通知联盟
                    if (const AllianceSimple* alliance = g_mapMgr->alliance().FindAllianceSimple(attAgent.allianceId())) {
                        alliance->OnKillWorldBoss(attAgent.nickname(), *m_worldboss);
                    }
                    m_worldboss->RemoveSelf();
                    // 这里加一个判断，当前的世界BOSS是否需要刷新
                    // to do
                    /*m_worldboss->RefreshSelf();*/
            	}
            }

        }
    }
}
