#include "battle.h"
#include "../agent.h"
#include <model/tpl/templateloader.h>
#include <model/tpl/configure.h>
#include <base/logger.h>
#include "../unit/unit.h"
#include  "../tpl/templateloader.h"
#include "../tpl/npcarmytpl.h"

namespace ms
{
    namespace map
    {
        namespace battle
        {
            using namespace model::tpl;

            Battle::Battle(model::BattleType type, Troop* troop, Unit* unit)
                : m_type(type), m_unit(unit), m_atkTroop(troop)
            {
                if (unit->troop()) {
                    m_defTroop = unit->troop();
                }
            }

            Battle::~Battle() {}

            void Battle::Init(engine::InitialDataInput& attackInput, engine::InitialDataInput& defenseInput)
            {
                SetAttackInput(attackInput);
                SetDefenseInput(defenseInput);
                inputArmyList(attackInput,  m_atkArmyList, nullptr);
                inputArmyList(defenseInput,  m_defArmyList, m_defTrapSet);
                //战斗开始时统计一下双方总兵力
                if (m_atkArmyList)
                {
                    m_reportInfo.attackerBeginArmyCount = m_atkArmyList->ArmyCount(ArmyState::NORMAL);
                }
                if (m_defArmyList)
                {
                    m_reportInfo.defenderBeginArmyCount = m_defArmyList->ArmyCount(ArmyState::NORMAL);
                }
                m_reportInfo.level = m_unit->tpl().level;
            }

            void Battle::OutputSingleResult(const engine::SingleCombatResult& singleResult)
            {
                LOG_DEBUG("attackArmyList Begin");
                // m_atkArmyList->Debug();
                // 转换伤兵
                if (m_atkArmyList) {
                    // 产生伤兵
                    if (m_atkTroop->uid() != 0 && singleResult.attackLoseArmyPercent > 0) {
                        m_atkArmyList->ToWound(singleResult.attackLoseArmyPercent);
                    }
                    
                    // 扣除体力
                    for (auto it = m_atkArmyList->armies().begin(); it != m_atkArmyList->armies().end(); ++it) {
                        HeroInfo& info = it->second.heroInfo();
                        if (singleResult.attackHeroId != 0 && singleResult.attackHeroId == info.tplId())
                        {
                            info.SetPhysical(singleResult.attackHeroHp);
                        }
                    }
                }
                LOG_DEBUG("attackArmyList End");
                // m_atkArmyList->Debug();


                LOG_DEBUG("defenseArmyList Begin");
                // m_defArmyList->Debug();
                if (m_defArmyList) {
                    if (m_defTroop && singleResult.defenseLostArmyPercent > 0) {
                        // 玩家被攻击，产生伤兵
                        m_defArmyList->ToWound(singleResult.defenseLostArmyPercent);
                    }

                    // 扣除体力
                    for (auto it = m_defArmyList->armies().begin(); it != m_defArmyList->armies().end(); ++it) {
                        HeroInfo& info = it->second.heroInfo();
                        if (singleResult.defenseHeroId != 0 && singleResult.defenseHeroId == info.tplId())
                        {
                            info.SetPhysical(singleResult.defenseHeroHp);
                        }
                    }
                }
                LOG_DEBUG("defenseArmyList End");
                // m_defArmyList->Debug();
            }

            void Battle::OutputResult(const engine::MixedCombatResult& mixedResult)
            {
                OutPutAttackArmyList(mixedResult);
                OutPutDefenseArmyList(mixedResult);
            }

            void Battle::SetAttackInput(engine::InitialDataInput& attackInput)
            {
                attackInput.uid = m_atkTroop->uid();
                attackInput.name = m_atkTroop->agent().nickname();
                attackInput.level = m_atkTroop->agent().level();
                attackInput.headIcon = m_atkTroop->agent().headId();

                //属性加成
                auto& property = m_atkTroop->agent().property();
                attackInput.prop.trapHorseDamagePct = property.trapHorseDamagePct;
                attackInput.prop.trapHorseDamageExt = property.trapHorseDamageExt;
                attackInput.prop.trapStoneDamagePct = property.trapStoneDamagePct;
                attackInput.prop.trapStoneDamageExt = property.trapStoneDamageExt;
                attackInput.prop.trapWoodDamagePct = property.trapWoodDamagePct;
                attackInput.prop.trapWoodDamageExt = property.trapWoodDamageExt;
                attackInput.prop.trapOilDamagePct = property.trapOilDamagePct;
                attackInput.prop.trapOilDamageExt = property.trapOilDamageExt;
                attackInput.prop.riderRestrainInfantryPct = property.riderRestrainInfantryPct;
                attackInput.prop.infantryRestrainArcherPct = property.infantryRestrainArcherPct;
                attackInput.prop.archerRestrainMachinePct = property.archerRestrainMachinePct;
                attackInput.prop.machineRestrainRiderPct = property.machineRestrainRiderPct;

                m_atkArmyList = m_atkTroop->armyList();
            }

            void Battle::SetDefenseInput(engine::InitialDataInput& defenseInput)
            {
                if (m_defTroop) {
                    defenseInput.uid = m_defTroop->uid();
                    defenseInput.name = m_defTroop->agent().nickname();
                    defenseInput.level = m_defTroop->agent().level();
                    defenseInput.headIcon = m_defTroop->agent().headId();
                    m_defArmyList = m_defTroop->armyList();

                    //属性加成
                    auto& property = m_defTroop->agent().property();
                    defenseInput.prop.trapHorseDamagePct = property.trapHorseDamagePct;
                    defenseInput.prop.trapHorseDamageExt = property.trapHorseDamageExt;
                    defenseInput.prop.trapStoneDamagePct = property.trapStoneDamagePct;
                    defenseInput.prop.trapStoneDamageExt = property.trapStoneDamageExt;
                    defenseInput.prop.trapWoodDamagePct = property.trapWoodDamagePct;
                    defenseInput.prop.trapWoodDamageExt = property.trapWoodDamageExt;
                    defenseInput.prop.trapOilDamagePct = property.trapOilDamagePct;
                    defenseInput.prop.trapOilDamageExt = property.trapOilDamageExt;
                    defenseInput.prop.riderRestrainInfantryPct = property.riderRestrainInfantryPct;
                    defenseInput.prop.infantryRestrainArcherPct = property.infantryRestrainArcherPct;
                    defenseInput.prop.archerRestrainMachinePct = property.archerRestrainMachinePct;
                    defenseInput.prop.machineRestrainRiderPct = property.machineRestrainRiderPct;
                } else {
                    defenseInput.uid = 0;
                    auto npcArmyTpl = tpl::m_tploader->FindNpcArmyTpl(m_unit->tpl().armyGroup);
                    if (npcArmyTpl) {
                        defenseInput.name = npcArmyTpl->name;
                        defenseInput.level = npcArmyTpl->level;
                        defenseInput.headIcon = npcArmyTpl->headId;
                    }
                    //m_unit->DefArmyList()->Debug();
                    m_defArmyList = m_unit->DefArmyList();
                }
            }

            void Battle::OutPutAttackArmyList(const engine::MixedCombatResult& mixedResult)
            {
                if (m_atkArmyList) {
                    auto tpl = g_tploader->FindBattleInjuredSoldiers(m_type);
                    if (tpl)
                    {
                        for (auto & hurt : mixedResult.attackHurt) {
                            if (hurt.dieCount > 0) {
                                // 1.受伤
                                m_atkArmyList->Wound(hurt.heroId, std::ceil(hurt.dieCount * tpl->injuredSoldiers));
                                // 2.死亡
                                m_atkArmyList->Die(hurt.heroId, std::ceil(hurt.dieCount * tpl->deadSoldiers));
                                // 3.待恢复
                                m_atkArmyList->Recover(hurt.heroId, std::floor(hurt.dieCount * (1 - tpl->injuredSoldiers - tpl->deadSoldiers)));
                            }
                            if (hurt.killCount > 0) {
                                m_atkArmyList->SetArmy(hurt.heroId, model::ArmyState::KILL, hurt.killCount);
                            }
                        }
                    }
                    
                    // 1.统计攻击方在这次战斗之中所消耗的兵力总和,攻击方是在一场战斗结束之后才计算恢复兵力
                    m_attackArmyHurts.push_back(mixedResult.attackHurt);

                    // 失败方进行兵力补偿
                    //if (!mixedResult.isAttackWin && mixedResult.attackTotalDie > 5000) {
                    if (!mixedResult.isAttackWin) {
                        auto& atkAgent = m_atkTroop->agent();
                        Compensate(model::AttackType::ATTACK, mixedResult, atkAgent);
                    }
                }
            }

            void Battle::OutPutDefenseArmyList(const engine::MixedCombatResult& mixedResult)
            {
                if (m_defArmyList) {
                    auto tpl = g_tploader->FindBattleInjuredSoldiers(m_type);
                    if (tpl)
                    {
                        for (auto & hurt : mixedResult.defenseHurt) {
    
                            if (hurt.dieCount > 0) {
                                // 1.受伤
                                m_defArmyList->Wound(hurt.heroId, std::ceil(hurt.dieCount * tpl->injuredSoldiers));
                                // 2.死亡
                                m_defArmyList->Die(hurt.heroId, std::ceil(hurt.dieCount * tpl->deadSoldiers));
                                // 3.待恢复
                                m_defArmyList->Recover(hurt.heroId, std::floor(hurt.dieCount * (1 - tpl->injuredSoldiers - tpl->deadSoldiers)));
                            }
                            if (hurt.killCount > 0) {
                                m_defArmyList->SetArmy(hurt.heroId, model::ArmyState::KILL, hurt.killCount);
                            }
                        }
                    }
                    if (m_defTroop) {
                        // 失败方进行兵力补偿
                        if (mixedResult.isAttackWin) {
                            auto& defAgent = m_defTroop->agent();
                            Compensate(model::AttackType::DEFENSE, mixedResult, defAgent);
                        }
                    }
                }
            }

            void  Battle::inputArmyList(engine::InitialDataInput& input, ArmyList* armyList, TrapSet* trapSet)
            {
                if (armyList) {
                    auto& armies = armyList->armies();
                    for (auto & army : armies) {
                        ArmyGroup& armyGroup = army.second;
                        int armyCount = armyGroup.armyInfo().count(model::ArmyState::NORMAL);
                        if (armyCount <= 0) {
                            continue;
                        }

                        if (!armyGroup.armyInfo().IsTrap() && model::ArmyType_IsValid(armyGroup.armyInfo().type())) {
                            engine::I_Group iGroup;
                            iGroup.hero.id = armyGroup.heroInfo().tplId();
                            iGroup.hero.level = armyGroup.heroInfo().level();
                            iGroup.hero.star = armyGroup.heroInfo().star();
                            iGroup.hero.physical = armyGroup.heroInfo().physical();
                            iGroup.hero.heroPower = armyGroup.heroInfo().heroPower();
                            iGroup.hero.heroDefense = armyGroup.heroInfo().heroDefense();
                            iGroup.hero.heroWisdom = armyGroup.heroInfo().heroWisdom();
                            iGroup.hero.heroLucky = armyGroup.heroInfo().heroLucky();
                            iGroup.hero.heroSkill = armyGroup.heroInfo().heroSkill();
                            iGroup.hero.heroAgile = armyGroup.heroInfo().heroAgile();
                            iGroup.hero.heroLife = armyGroup.heroInfo().heroLife();
                            iGroup.hero.heroPhysicalPower = armyGroup.heroInfo().heroPhysicalPower();
                            iGroup.hero.heroPhysicalDefense = armyGroup.heroInfo().heroPhysicalDefense();
                            iGroup.hero.heroSkillPower = armyGroup.heroInfo().heroSkillPower();
                            iGroup.hero.heroSkillDefense = armyGroup.heroInfo().heroSkillDefense();
                            iGroup.hero.heroHit = armyGroup.heroInfo().heroHit();
                            iGroup.hero.heroAvoid = armyGroup.heroInfo().heroAvoid();
                            iGroup.hero.heroCritHit = armyGroup.heroInfo().heroCritHit();
                            iGroup.hero.heroCritAvoid = armyGroup.heroInfo().heroCritAvoid();
                            iGroup.hero.heroSpeed = armyGroup.heroInfo().heroSpeed();
                            iGroup.hero.heroCityLife = armyGroup.heroInfo().heroCityLife();
                            iGroup.hero.heroSolohp = armyGroup.heroInfo().heroSolohp();
                            iGroup.hero.heroTroops = armyGroup.heroInfo().heroTroops();

                            for (auto & spell : armyGroup.heroInfo().skill()) {
                                engine::I_Spell iSpell;
                                iSpell.id = spell.id;
                                iSpell.level = spell.level;
                                iGroup.hero.spells.push_back(iSpell);
                            }

                            iGroup.army.armyType = armyGroup.armyInfo().type();
                            iGroup.army.armyLevel = armyGroup.armyInfo().level();
                            iGroup.army.armyCount = armyCount;
                            iGroup.army.hp = armyGroup.armyInfo().hp();
                            iGroup.army.attack = armyGroup.armyInfo().attack();
                            iGroup.army.defense = armyGroup.armyInfo().defense() * (1.0f + armyList->addDefensePct());
                            iGroup.army.speed = armyGroup.armyInfo().speed();

                            iGroup.position = armyGroup.position();

                            input.team.groups.push_back(iGroup);
                        }
                    }

                }
            }

            void Battle::ResetArmyList()
            {
                if (m_atkArmyList) {
                    m_atkArmyList->ClearAllExceptNormal();
                }

                if (m_defArmyList) {
                    // 防守方每轮战斗结束之后就把待恢复的兵恢复成正常兵
                    m_defArmyList->RecoverToNormal();
                    m_defArmyList->ClearAllExceptNormal();
                }

                if (m_defTrapSet) {
                    m_defTrapSet->ClearAllExceptNormal();
                }
            }

            // 输出结束兵力
            void Battle::OutPutEndAmryCount()
            {
                if (m_atkArmyList)
                {
                    m_reportInfo.attackerEndArmyCount = m_atkArmyList->ArmyCount(ArmyState::NORMAL);
                }
                if (m_defArmyList)
                {
                    m_reportInfo.defenderEndArmyCount = m_defArmyList->ArmyCount(ArmyState::NORMAL);
                }
                m_reportInfo.posName = m_posName;
            }

            // 一场战斗结束之后，把需要恢复的兵变成正常兵
            void Battle::OnBattleRecoverArmy()
            {
                if (m_atkArmyList)
                {
                    auto tpl = g_tploader->FindBattleInjuredSoldiers(m_type);
                    for (auto armyHurt : m_attackArmyHurts)
                    {
                        for (auto & hurt : armyHurt)
                        {
                            m_atkArmyList->Add(hurt.heroId, std::floor(hurt.dieCount * (1 - tpl->injuredSoldiers - tpl->deadSoldiers)));
                        }
                    }
                }
            }

            bool Battle::SwitchTroop()
            {
                bool ret =  m_unit->SwitchTroop();
                if (m_unit->troop()) {
                    m_defTroop = m_unit->troop();
                }
                return ret;
            }

            bool Battle::IsLastTroop() 
            {
                return m_unit->IsLastTroop();
            }

            void Battle::Compensate(model::AttackType attackType, const engine::MixedCombatResult& mixedResult, Agent& agent) 
            {            
                int first = 0;
                int second = 0;
                int third = 0;
                int fourth = 0;
                int fifth = 0;

                if (attackType == model::AttackType::ATTACK) {  
                    if (m_atkArmyList) {
                        for (auto army : m_atkArmyList->armies()) {
                            ArmyGroup& armyGroup = army.second;
                            int die = armyGroup.armyInfo().count(ArmyState::DIE);
                            int level = armyGroup.armyInfo().level();                        
                            
                            switch(level) {
                                case 1:
                                    first += die;
                                    break;
                                case 2:
                                    second += die;
                                    break;
                                case 3:
                                    third += die;
                                    break;
                                case 4:
                                    fourth += die;
                                    break;
                                case 5:
                                    fifth += die;
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                } else if (attackType == model::AttackType::DEFENSE) {
                    if (m_defArmyList) {
                        for (auto army : m_defArmyList->armies()) {
                            ArmyGroup& armyGroup = army.second;
                            int die = armyGroup.armyInfo().count(ArmyState::DIE);
                            int level = armyGroup.armyInfo().level();                        
                            
                            switch(level) {
                                case 1:
                                    first += die;
                                    break;
                                case 2:
                                    second += die;
                                    break;
                                case 3:
                                    third += die;
                                    break;
                                case 4:
                                    fourth += die;
                                    break;
                                case 5:
                                    fifth += die;
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                }

                //兵种等级系数统计
                int sum = + (first * g_tploader->configure().compensate.first) + (second * g_tploader->configure().compensate.second) 
                    + (third * g_tploader->configure().compensate.third) + (fourth * g_tploader->configure().compensate.fourth)
                    + (fifth * g_tploader->configure().compensate.fifth);

                int food = g_tploader->configure().compensate.resourceBase * g_tploader->configure().compensate.food + sum;
                int wood = g_tploader->configure().compensate.resourceBase * g_tploader->configure().compensate.wood + sum;
                int stone = g_tploader->configure().compensate.resourceBase * g_tploader->configure().compensate.stone + sum;
                int iron = g_tploader->configure().compensate.resourceBase * g_tploader->configure().compensate.iron + sum;
                
                agent.msgQueue().AppendMsgCompensate(food, wood, stone, iron);
            }

        }
    }
}