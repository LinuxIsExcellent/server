#include "mixedcombat.h"
#include "../tpl/t_buff.h"
#include "buff.h"
#include <iostream>
#include <assert.h>
#include <algorithm>
#include <base/logger.h>



#include <model/tpl/templateloader.h>
#include <model/tpl/attackrange.h>


namespace engine
{
    namespace battle
    {
        using namespace tpl;
        using namespace model;
        using namespace model::tpl;

        SmallRoundNode* RoundNode::ToSmallRoundNode()
        {
            return m_type == MixedRoundType::ROUND_SMALL ?  static_cast<SmallRoundNode*>(this) : nullptr;
        }

        TurretRoundNode* RoundNode::ToTurretRoundNode()
        {
            return m_type == MixedRoundType::ROUND_TURRET ?  static_cast<TurretRoundNode*>(this) : nullptr;
        }

        ComplexRoundNode* RoundNode::ToComplexRoundNode() {
            return m_type == MixedRoundType::ROUND_COMPLEX ?  static_cast<ComplexRoundNode*>(this) : nullptr;
        }


        bool MixedCombat::Init(I_Team& attackTeam, I_Team& defenseTeam, const WarReport::SoloData* attackSoloReport, const WarReport::SoloData* defenseSoloReport)
        {

            if (attackTeam.groups.empty() || defenseTeam.groups.empty()) {
                return false;
            }

            // std::cout << "mixedCombat init ------------ " << std::endl;

            m_attackTeam.Init(attackTeam, TeamType::ATTACKER);
            m_defenseTeam.Init(defenseTeam, TeamType::DEFENDER);

            auto dieCallBack = [this](Hero* hero) {
                this->OnHeroDie(hero);
            };

            auto setDieCallBack = [&](Team& team) {
                for (auto& hero : team.GetHeroList()) {
                    hero.SetDieCb(dieCallBack);
                }
            };

            setDieCallBack(m_attackTeam);
            setDieCallBack(m_defenseTeam);

            //设置攻击对位
            std::list<Hero>& attackHeros = m_attackTeam.GetHeroList();
            for (auto& fromHero : attackHeros) {
                if (fromHero.IsDie()) {
                    continue;
                }
                Hero* toNewHero = FindHeroAttackTo(&fromHero);
                if (toNewHero) {
                    fromHero.SetAttackTo(toNewHero->Position());
                    toNewHero->AddBeAttackFrom(fromHero.Position());
                } else {
                    fromHero.SetAttackTo(0);
                }
            }

            std::list<Hero>& defenseHero = m_defenseTeam.GetHeroList();
            for (auto& fromHero : defenseHero) {
                if (fromHero.IsDie()) {
                    continue;
                }
                Hero* toNewHero = FindHeroAttackTo(&fromHero);
                if (toNewHero) {
                    fromHero.SetAttackTo(toNewHero->Position());
                    toNewHero->AddBeAttackFrom(fromHero.Position());
                } else {
                    fromHero.SetAttackTo(0);
                }
            }

            m_result.attackOldPower = m_attackTeam.TotalPower();
            m_result.defenseOldPower = m_defenseTeam.TotalPower();

            // std::cout << "========================================== Before Passive Skill ============== " << std::endl;
            for (auto& hero : m_attackTeam.GetHeroList()) {
                // std::cout << "Attack hero --- " << hero.tplId()  << "Position : " << hero.Position() << std::endl;
                hero.Debug();
            }

            for (auto& hero : m_defenseTeam.GetHeroList()) {
                // std::cout << "Defense --- hero --- " << hero.tplId() << " Position: " << hero.Position() << std::endl;
                hero.Debug();
            }

            // 加被动Buff
            auto castPassiveBuff = [&](Team& team) {
                for (auto& hero : team.GetHeroList()) {
                    for (auto spell : hero.iGroup().hero.spells) {
                        // std::cout << "Skill--- id: " << spell.id << " level: " << spell.level << std::endl;
                        auto skillTpl = model::tpl::g_tploader->FindSkill(spell.id);
                        if (!skillTpl || skillTpl->skillType != 1 ) {
                            continue;
                        }

                        CastPassiveSkill(hero, skillTpl, spell.level, 0);
                    }
                }
            };

            castPassiveBuff(m_attackTeam);
            castPassiveBuff(m_defenseTeam);

            // //初始hero 二级属性
            // auto initSecondProp = [&](Team& team) {
            //     for (auto &hero : team.GetHeroList()) {
            //         hero.InitSecondProp();
            //     }
            // };

            // initSecondProp(m_attackTeam);
            // initSecondProp(m_defenseTeam);

            // std::cout << "========================================== After Passive Buff ============== " << std::endl;

            for (auto& hero : m_attackTeam.GetHeroList()) {
                 // std::cout << "Attack hero --- " << hero.tplId()  << "Position : " << hero.Position() << std::endl;
                hero.Debug();
            }

            for (auto& hero : m_defenseTeam.GetHeroList()) {
                // std::cout << "Defense --- hero --- " << hero.tplId() << " Position: " << hero.Position() << std::endl;
                hero.Debug();
            }

            //判断是否有队伍全部死光
            if (m_attackTeam.IsAllDie() || m_defenseTeam.IsAllDie()) {
                m_isEnd = true;
                m_state = MixedCombatState::OVER;
                return true;
            }

            // 战报-初始数据
            WarReport::HeroArmyDatas heroDatas;
            heroDatas.armyDataType = WarReport::ArmyDataType::SoloEnd;
            for (auto& hero : m_attackTeam.GetHeroList()) {
                WarReport::HeroArmyData data;
                data.attackType = TeamType::ATTACKER;
                data.armyId = GetAbsolutePos(&hero);
                data.heroId = hero.tplId();
                data.soldierId = hero.armyTpl()->id;
                data.hp = hero.armyHp();
                data.initPosition = hero.Position();

                data.power = hero.power();
                data.defense = hero.defense();
                data.wisdom = hero.wisdom();
                data.skill = hero.skill();
                data.agile = hero.agile();
                data.lucky = hero.lucky();

                heroDatas.heroArmyData.push_back(data);
            }

            for (auto& hero : m_defenseTeam.GetHeroList()) {
                WarReport::HeroArmyData  data;
                data.attackType = TeamType::DEFENDER;
                data.armyId = GetAbsolutePos(&hero);
                data.heroId = hero.tplId();
                data.soldierId = hero.armyTpl()->id;
                data.hp = hero.armyHp();
                data.initPosition = hero.Position();

                data.power = hero.power();
                data.defense = hero.defense();
                data.wisdom = hero.wisdom();
                data.skill = hero.skill();
                data.agile = hero.agile();
                data.lucky = hero.lucky();

                heroDatas.heroArmyData.push_back(data);
            }
            m_report.heroArmyDatas.push_back(heroDatas);

            // 填充单挑数据
            if (attackSoloReport && defenseSoloReport) {
                m_report.soloDatas.push_back(*attackSoloReport);
                m_report.soloDatas.push_back(*defenseSoloReport);
            }

            m_beginArmyCount = m_attackTeam.RemainTotalHp();
            BigRoundBegin();
            return true;
        }

        int MixedCombat::GetAbsolutePos(Hero* hero) {
            if (!hero)
            {
                return 0;
            }
            int position = 0;
            if (hero->teamType() == TeamType::ATTACKER) {
                position =  hero->Position();
            } else {
                position = 9 + hero->Position();
            }
            return position;
        }

        Hero* MixedCombat::GetAbsolutePosHero(int position) {
            if (position < 0 || position > 18) { return nullptr; }

            Hero* hero = nullptr;
            if (position <= 9) {
                hero = m_attackTeam.GetHeroByPos(position);
            } else {
                hero = m_defenseTeam.GetHeroByPos(position - 9);
            }

            return hero;
        }

        Hero* MixedCombat::GetAnyHero() {
            Hero* hero = nullptr;

            int slot[18] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};
            int last = 18;
            while (last > 0) {
                int rand = m_randEngine.RandBetween(1, last);
                int temp = slot[rand - 1];
                slot[rand - 1] = slot[last -1];
                slot[last - 1] = temp;
                last--;

                hero = GetAbsolutePosHero(rand);
                if (hero) {
                    break;
                }
            }

            return hero;
        }

        void MixedCombat::CastPassiveSkill(Hero& hero, const model::tpl::SkillTpl* skillTpl, int level, int hurt)
        {
            if (skillTpl->passiveType1 != 0) {
                T_Buff buff;
                buff.id = skillTpl->id;
                buff.type = static_cast<BuffType>(skillTpl->passiveType1);
                buff.target = static_cast<BuffTarget>(skillTpl->skillRange);
                buff.base_value1 = skillTpl->passiveParameter1;
                buff.level_value1 = skillTpl->passiveCoefficient1;
                buff.trigger_state = TriggerBuffState::SMALL_ROUND_START;
                CastBuff(hero, hero, buff, level, true, 0);
            }

            if (skillTpl->passiveType2 != 0) {
                T_Buff buff;
                buff.id = skillTpl->id;
                buff.type = static_cast<BuffType>(skillTpl->passiveType2);
                buff.target = static_cast<BuffTarget>(skillTpl->skillRange);
                buff.base_value1 = skillTpl->passiveParameter2;
                buff.level_value1 = skillTpl->passiveCoefficient2;
                buff.trigger_state = TriggerBuffState::SMALL_ROUND_START;
                CastBuff(hero, hero, buff, level, true, 0);
            }
        }

        bool MixedCombat::Start()
        {
            while (MixedCombatState::OVER != Next());
            return true;
        }

        MixedCombatState MixedCombat::Next()
        {
            switch (m_state) {
                case MixedCombatState::SMALL_READY:
                    SmallRoundCombat();
                    RoundEnd();
                    break;
                case MixedCombatState::COMPLEX_READY:
                    ComplexRoundCombat();
                    RoundEnd();
                    break;
                case MixedCombatState::WAITING:
                    if (!Ready()) {
                        BigRoundEnd();
                        if (!IsEnd()) {
                            Next();
                        }
                    }
                    break;
                // 箭塔
                case MixedCombatState::TURRET_READY:
                    TurretRoundCombat();
                    RoundEnd();
                    break;
                default:
                    break;
            }

            if (IsEnd()) {
                End();
                m_state = MixedCombatState::OVER;
            }

            return m_state;
        }

        int MixedCombat::GetDistance(int fromPos, int toPos)
        {
            int fromRow = fromPos / 3 + (fromPos % 3 > 0 ? 1 : 0);
            int toRow = toPos / 3 + (toPos % 3 > 0 ? 1 : 0);

            int distance = fromRow + toRow - 1;
            return distance;
        }

        void MixedCombat::CheckDie(engine::TeamType teamType)
        {
            std::vector<Hero*> dieGroups;
            //OutputHpAferAttack(teamType, dieGroups);

            // for (auto dieGroup : dieGroups) {
            //     if (dieGroup && dieGroup->IsDie()) {
            //         OnGroupDie(dieGroup);
            //     }
            // }
        }

        bool MixedCombat::Ready()
        {
            CheckInvalidRound();

            if (m_roundNodes.empty()) {
                return false;
            }

            auto roundNode = m_roundNodes.front();

            if (roundNode->position() > 0) {
                auto fromHero = FromTeam(roundNode->teamType()).GetHeroByPos(roundNode->position());
                if (fromHero) {
                    m_curAttacker = fromHero;
                }
            }

            bool ready = false;
            if (roundNode->ToSmallRoundNode()) {
                if (SmallRoundReady()) {
                    m_state = MixedCombatState::SMALL_READY;
                    ready = true;
                }
            } else if (roundNode->ToComplexRoundNode()) {
                if (ComplexRoundReady()) {
                    m_state = MixedCombatState::COMPLEX_READY;
                    ready = true;
                }
            }
            else if (roundNode->ToTurretRoundNode()) {
                if (TurretRoundReady()) {
                    m_state = MixedCombatState::TURRET_READY;
                    ready = true;
                }
            }

            return ready;
        }

        bool MixedCombat::SmallRoundReady()
        {
            if (m_roundNodes.empty()) {
                return false;
            }

            auto roundNode = m_roundNodes.front()->ToSmallRoundNode();
            if (roundNode ==  nullptr) {
                return false;
            }

            ++m_smallRound;

            AttackNode& attackNode =  roundNode->m_attackNode;

            Hero* nodeHero = FromTeam(attackNode.teamType).GetHeroByPos(attackNode.position);
            if (nodeHero != nullptr) {
                return true;
            }
            return false;
        }

        bool MixedCombat::ComplexRoundReady()
        {
            if (m_roundNodes.empty()) {
                return false;
            }

            auto roundNode = m_roundNodes.front()->ToComplexRoundNode();
            if (roundNode ==  nullptr) {
                return false;
            }

            ++m_complexRound;

            AttackNode& attackNode =  roundNode->m_attackNode;

            Hero* nodeHero = FromTeam(attackNode.teamType).GetHeroByPos(attackNode.position);
            if (nodeHero != nullptr) {
                return true;
            }
            return false;
        }

        void MixedCombat::OnGroupDie(Hero* group)
        {
            if (nullptr != group) {
                for (auto it = m_roundNodes.begin(); it != m_roundNodes.end();) {
                    auto roundNode = *it;
                    if (roundNode->IsOwner(group->teamType(),  group->Position())) {
                        delete roundNode;
                        it = m_roundNodes.erase(it);
                    } else {
                        ++it;
                    }
                }

                //更新对位
                Hero* toGroup = ToTeam(group->teamType()).GetHeroByPos(group->attackTo());;
                if (toGroup) {
                    toGroup->RemoveBeAttackFrom(group->Position());
                }

                const std::list<int>& beAttackFrom = group->beAttackFrom();
                for (auto fromPos : beAttackFrom) {
                    Hero* fromGroup  = ToTeam(group->teamType()).GetHeroByPos(fromPos);
                    if (fromGroup) {
                        Hero* toNewGroup = FindHeroAttackTo(fromGroup);
                        if (toNewGroup) {
                            fromGroup->SetAttackTo(toNewGroup->Position());
                            toNewGroup->AddBeAttackFrom(fromGroup->Position());
                        } else {
                            fromGroup->SetAttackTo(0);
                        }
                    }
                }

                if (FromTeam(group->teamType()).IsAllDie()) {
                    m_isEnd = true;
                }
//                 group->OnDie();
            }
        }

        int MixedCombat::AssistSum(Team& team, Hero* hero) {
            int sum = 0;
            if (hero->heroTpl()->supportPlus1 != 0) {
                for (auto& h : team.GetHeroList()) {
                    if (h.tplId() == hero->heroTpl()->supportPlus1) {
                        sum += hero->heroTpl()->supportValues1;
                    }
                }
            }

            if (hero->heroTpl()->supportPlus2 != 0) {
                for (auto& h : team.GetHeroList()) {
                    if (h.tplId() == hero->heroTpl()->supportPlus2) {
                        sum += hero->heroTpl()->supportValues2;
                    }
                }
            }

            if (hero->heroTpl()->supportPlus3 != 0) {
                for (auto& h : team.GetHeroList()) {
                    if (h.tplId() == hero->heroTpl()->supportPlus3) {
                        sum += hero->heroTpl()->supportValues3;
                    }
                }
            }

            if (hero->heroTpl()->supportPlus4 != 0) {
                for (auto& h : team.GetHeroList()) {
                    if (h.tplId() == hero->heroTpl()->supportPlus4) {
                        sum += hero->heroTpl()->supportValues4;
                    }
                }
            }

            if (hero->heroTpl()->supportPlus5 != 0) {
                for (auto& h : team.GetHeroList()) {
                    if (h.tplId() == hero->heroTpl()->supportPlus5) {
                        sum += hero->heroTpl()->supportValues5;
                    }
                }
            }

            //// std::cout << "Assit->sum : " << sum << " heroId: " << hero->tplId() << std::endl;
            return sum;
        }

        void MixedCombat::RecordReport(Hero* hero, Hero* toHero, WarReport::AttackType attackType,
            int hurt, int changeRage) {
            WarReport::UnitRound unitRound;
            unitRound.armyId = GetAbsolutePos(hero);
            unitRound.targetId = GetAbsolutePos(toHero);
            unitRound.changeHp = hurt;
            unitRound.changeBatVal = 0;
            unitRound.changeRageVal = changeRage;
            unitRound.castType = attackType;
            unitRound.skill.skillId = 0;

            for (auto& v : m_report.allBigRounds) {
                if (v.bigRoundId == m_bigRound) {
                    v.allUnitRounds.push_back(unitRound);
                }
            }
        }

        bool MixedCombat::NormalAttack(Hero* hero, Hero* toHero) {
            LOG_DEBUG(" NormalAttack --- ");
            auto recordReport = [&] (int lastest_hurt, bool crit_flag) {
                WarReport::UnitRound unitRound;
                unitRound.armyId = GetAbsolutePos(hero);
                unitRound.targetId = GetAbsolutePos(toHero);
                unitRound.changeHp = lastest_hurt;
                unitRound.changeBatVal = 0;
                unitRound.changeRageVal = hero->HasStatus(TargetStatus::WEAK) ? 0 : 20;
                unitRound.castType = WarReport::AttackType::NORMAL;
                unitRound.skill.skillId = 0;
                unitRound.crit = crit_flag ? 1 : 0;

                for (auto& v : m_report.allBigRounds) {
                    if (v.bigRoundId == m_bigRound) {
                        v.allUnitRounds.push_back(unitRound);
                    }
                }
            };

            if (hero->HasStatus(TargetStatus::TUMBLE)) {
                LOG_DEBUG("--- NormalAttack Hero Tumble !!! --- ");
                toHero = GetAnyHero();
            }

            if (!toHero || toHero->IsDie()) {
                LOG_DEBUG("---------- NormalAttack Hero Die--- ");
                recordReport(0, false);
                return false;
            }

            int rand = m_randEngine.RandBetween(1, 100);
            float hit_rate = 0;
            float param = hero->hit() + AssistSum(m_attackTeam, hero) - toHero->avoid() - AssistSum(m_defenseTeam, toHero)
                + (float)(toHero->HasStatus(TargetStatus::PARALYSIS) ? 50 : 0);
            // std::cout << " hit : " << hero->hit() << " assit " << AssistSum(m_attackTeam, hero) << " to->hit: " << toHero->avoid()
            //     << " to->assit: " << AssistSum(m_defenseTeam, toHero) << " IsStuas " << toHero->HasStatus(TargetStatus::PARALYSIS)
            //     << " param: " << param << std::endl ;
            hit_rate = 90 * pow(1.005, param);

            //普通命中判断
            rand = m_randEngine.RandBetween(1, 100);
            LOG_DEBUG("Hit rate: %.3f  ---  rand:%d ", hit_rate, rand);
            if ((int)hit_rate < 100  && (int)hit_rate < rand) {
                //小回合结束
                LOG_DEBUG("Not Hit --- ");
                recordReport(0, false);
                return false;
            }

            //暴击命中判断
            float crit_hit = hero->crit_hit() + AssistSum(m_attackTeam, hero) - toHero->crit_avoid() - AssistSum(m_defenseTeam, toHero)
                + (toHero->HasStatus(TargetStatus::PARALYSIS) ? 50 : 0);
            float crit_rate = crit_hit > 0 ? crit_hit : 0;

            bool crit_flag = false;
            rand = m_randEngine.RandBetween(1, 100);
            LOG_DEBUG("crit rate: %.3f , rand:%d ", crit_rate, rand);
            if ((int)crit_rate > 100 || (int)crit_rate < rand) {
               //暴击命中产生增益效果
               crit_flag = true;
            }

            //产生伤害
            //bool isAttackValid = ArmyAttack(hero, nirvarna_flag, crit_flag);

            if (!toHero) {
                // std::cout << " No Target " << std::endl;
                LOG_DEBUG("---------- No Target ------------- ");
                recordReport(0, false);
                return false;
            }

            if (toHero) {
                if (toHero->IsDie()) {
                    LOG_DEBUG("---------- Target Hero Die------------- ");
                    // std::cout << "Target Hero Die !!! " << std::endl;
                    recordReport(0, false);
                    return false;
                }

                int lastest_hurt = 0;
                float physical_hurt = 0;
                float magic_hurt = 0;
                int float_value = m_randEngine.RandBetween(1, 11) - 6; // -5 ~ 5
                DamageType type = static_cast<DamageType>(hero->damage());
                if (hero->max_army_count() <= 0)
                {
                    LOG_DEBUG("---------- fight is zero,Target Hero is %d\n------------- ", hero->tplId());
                }
                float pad_value =  hero->army_count() / hero->max_army_count();
                pad_value = pad_value < 0.5 ? 0.5 : pad_value;

                // std::cout << "float_value : " << float_value  << " pad_value: " << pad_value
                //    <<  "armyAdapt: " << hero->GetArmyAdapt()
                //    << " army restraint: " << ArmyRestraint(hero->armyType(), toHero->armyType())
                //    << " wound status: "<< hero->HasStatus(TargetStatus::WOUND)
                //    << " target wound status: "<< toHero->HasStatus(TargetStatus::WOUND) << std::endl;

                if (type == DamageType::PHYSICAL_HURT || type == DamageType::MIX_HURT) {
                    // std::cout << " physical_attack : " << hero->physical_attack() << " toHero->defense: " << toHero->physical_defense()
                    //   << std::endl;
                    physical_hurt = (pow(hero->physical_attack(), 2) / ((hero->physical_attack() + toHero->physical_defense()))
                        * pow(1.01, float_value - (hero->HasStatus(TargetStatus::WOUND) ? 50 : 0)
                        +  (toHero->HasStatus(TargetStatus::WOUND) ? 50 : 0)) + AssistSum(m_attackTeam, hero) - AssistSum(m_defenseTeam, toHero))
                        * hero->GetArmyAdapt() * (1 + ArmyRestraint(hero->armyType(), toHero->armyType()))
                        * (1 + (crit_flag ? 1 : 0)) * (1 + 0/100 - 0/100) * pad_value;
                    physical_hurt = physical_hurt > 0.0 ? physical_hurt : 0.0;

                } else if (type == DamageType::MAGIC_HURT || type == DamageType::MIX_HURT) {
                    magic_hurt = (pow(hero->wisdom_attack(), 2) / ((hero->wisdom_attack() + toHero->wisdom_defense()))
                        * pow(1.01, float_value - (hero->HasStatus(TargetStatus::WOUND) ? 50 : 0)
                            +  (toHero->HasStatus(TargetStatus::WOUND) ? 50 : 0)) + AssistSum(m_attackTeam, hero) - AssistSum(m_defenseTeam, toHero))
                        * hero->GetArmyAdapt() * (1 + ArmyRestraint(hero->armyType(), toHero->armyType()))
                        * (1 + (crit_flag ? 1 : 0))  * (1 + 0/100 - 0/100) * pad_value;
                    magic_hurt = magic_hurt > 0.0 ? magic_hurt : 0.0;
                }

                // std::cout << " physical_hurt : " << physical_hurt << " magic_hurt: " << magic_hurt << std::endl;

                //攻击方加伤计算
                physical_hurt = AddPhysicalHurt(hero, physical_hurt);
                magic_hurt = AddMagicHurt(hero, magic_hurt);

                //防御方减伤计算
                physical_hurt = MinusPhysicalHurt(toHero, physical_hurt);
                magic_hurt = MinusMagicHurt(toHero, magic_hurt);

                // std::cout << " after effect physical_hurt : " << physical_hurt << " magic_hurt: " << magic_hurt << std::endl;
                //伤害计算  物理、法术、混合
                lastest_hurt = int(physical_hurt + magic_hurt);

                recordReport(-lastest_hurt, crit_flag);
                LOG_DEBUG("NormalAttack --- last_hurt: %d  damage_type:%d  ", lastest_hurt, hero->damage());
                toHero->OnBuffDirectDamage(*hero, lastest_hurt);  //目标扣血
            }

            return true;
        }

        void MixedCombat::GetEffectTargets(Hero& fromGroup, EffectRangeType rangeType, std::vector<Hero*>& targets,
            std::vector<Hero*>& effect_targets) {

            LOG_DEBUG("Effect range type --------- %d ", (int)rangeType);

            auto addOtherGroup = [&](int pos) {
                auto targetGroup = ToTeam(fromGroup.teamType()).GetHeroByPos(pos);
                if (targetGroup) {
                    if (!targetGroup->IsDie()) {
                        effect_targets.push_back(targetGroup);
                        return true;
                    }
                }
                return false;
            };

            auto addSelfGroup = [&](int pos) {
                //// std::cout << " teamType: " << (int)fromGroup.teamType() << " pos :  " << pos << std::endl;
                auto targetGroup = FromTeam(fromGroup.teamType()).GetHeroByPos(pos);
                if (targetGroup) {
                    if (!targetGroup->IsDie()) {
                        effect_targets.push_back(targetGroup);
                        return true;
                    }
                }
                return false;
            };

            auto addSelfAny = [&](int num, int except) {
                int slot[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
                int last = 9;
                int count = 0;

                while (last > 1) {
                    int rand = m_randEngine.RandBetween(1, last);
                    int temp = slot[rand - 1];
                    slot[rand - 1] = slot[last -1];
                    slot[last - 1] = temp;
                    last--;

                    if (except == temp) { continue; }

                    if (!addSelfGroup(temp)) { continue; }

                    if (effect_targets.size() < (unsigned)count) {
                        count++;
                    }

                    if (count >= num) {
                        break;
                    }
                }
            };

            int fromPos = fromGroup.Position();

            switch(rangeType) {
                case EffectRangeType::TARGET_SINGLE: {
                    //std::copy(targets.begin(), targets.end(), std::back_inserter(effect_targets));
                    effect_targets.assign(targets.begin(), targets.end());
                }
                break;
                case EffectRangeType::TARGET_ALL: {
                    for (int pos = 1; pos <= 9; ++pos) {
                        addOtherGroup(pos);
                    }
                }
                break;
                case EffectRangeType::ME: {
                    addSelfGroup(fromPos);
                }
                break;
                case EffectRangeType::SELF_RANDOM_SINGLE: {
                    addSelfAny(1, 0);
                }
                break;
                case EffectRangeType::ME_AND_RANDOM_SINGLE: {
                    addSelfAny(1, fromPos);
                    addSelfGroup(fromPos);
                }
                break;
                case EffectRangeType::SELF_EXCEPT_ME: {
                    for (int pos = 1; pos <= 9; ++pos) {
                        if (fromPos == pos) {continue;}
                        addSelfGroup(pos);
                    }
                }
                break;
                case EffectRangeType::SELF_ALL: {
                     for (int pos = 1; pos <= 9; ++pos) {
                        addSelfGroup(pos);
                    }
                }
                break;
                case EffectRangeType::RELATE_ASSIT: {
                    //Todo: 目前还不需要做
                }
                break;
                default:
                    break;
            }

            LOG_DEBUG("Effect map ---- loop ");
            for (auto& h : effect_targets) {
                LOG_DEBUG("Effect target position :  %d ", h->Position());
            }
        }
        //bool CastAreaEffect(const Hero* hero, const engine::tpl::T_Effect& effect, const std::vector<int>& targets, int level);
        void MixedCombat::CastAreaEffect(Hero* hero, const engine::tpl::T_Effect& effect, std::vector<Hero*>& targets,
            int level, WarReport::BuffData& buffData) {
            for (auto& target : targets) {
                CastSingleEffect(hero, effect,  target, level, 0, buffData);
            }
        }

        void MixedCombat::CastSingleEffect(Hero* hero, const engine::tpl::T_Effect& effect, Hero* target, int level,
            int hurt, WarReport::BuffData& buffData) {
            T_Buff tBuff;
            tBuff.id = 0;
            tBuff.base_value1 = effect.base_value;
            tBuff.level_value1 = effect.up_value;
            tBuff.extra_value = hurt;

            int eType = (int)effect.type;
            if (effect.type == EffectType::RADNOM ) {
                int rand = m_randEngine.RandBetween(0, 5);
                eType = (int)EffectType::SCARE + rand;
            }

            tBuff.type = static_cast<BuffType>(eType + (int)BuffType::COMPLEX);
            tBuff.rounds = 0;

            //状态效果概率触发
            if ( (tBuff.type >= BuffType::SCARE && tBuff.type <= BuffType::STUN)
                    || (tBuff.type == BuffType::CLEAN_NEGATIVE_STATE || tBuff.type == BuffType::CLEAN_POSITIVE_STATE)
                    || (tBuff.type == BuffType::DEDUCT_HP) ) {
                int rand = m_randEngine.RandBetween(0, 100);
                float chance = effect.base_value + (effect.up_value * level);
                LOG_DEBUG("State effect  rand:%d  chance:%0.3f ", rand, chance);
                if ((int) chance < 100 && (int)chance > rand) {
                    return;
                }

                // if (tBuff.type > BuffType::STUN) {
                //     tBuff.rounds = 1; //持续回合
                // }

                if (tBuff.type == BuffType::SCARE) {
                    tBuff.rounds = 4;
                }

                if (tBuff.type > BuffType::SCARE && tBuff.type <= BuffType::STUN) {
                    tBuff.rounds = 3;
                }
            }

            tBuff.trigger_state = TriggerBuffState::BIG_ROUND_START;  //在大回合开始时进行触发

            if (tBuff.type >= BuffType::ADD_WISDOM_PHYSICAL_DAMAGE && tBuff.type <= BuffType::MINUS_MAGIC_DAMAGE) {
                tBuff.rounds = 3;
            }

            int retVal = 0;
            Hero& targetHero = *target;
            hero->CastBuff(level, tBuff, targetHero, hurt, false, retVal);

            WarReport::Receiver receiver;
            receiver.armyId = GetAbsolutePos(target);
            receiver.value = retVal;
            buffData.receivers.push_back(receiver);
            buffData.keepRound = tBuff.rounds;
        }

        void MixedCombat::CastReleaseCondEffect(Hero* hero, std::vector<Hero*>& targets, int level,
            WarReport::SkillData& skillData) {
            auto skillTpl = hero->nirvanaTpl();

            if (skillTpl->effectCondition1 == (int)EffectCondType::AFTER_RELEASE) {
                // std::cout << "Cast ReleaseCond 1st " << std::endl;
                T_Effect effect;
                effect.range = static_cast<EffectRangeType>(skillTpl->effectRange1);
                effect.type = static_cast<EffectType>(skillTpl->addedEffect1);
                effect.base_value = skillTpl->additionalValue1;
                effect.up_value = skillTpl->upValue1;

                std::vector<Hero*> effect_targets;
                GetEffectTargets(*hero, effect.range, targets, effect_targets);

                WarReport::BuffData buffData;
                buffData.buffType = skillTpl->addedEffect1;
                buffData.receiverRound = m_bigRound;

                CastAreaEffect(hero, effect, effect_targets, level, buffData);

                skillData.buffs.push_back(buffData);
            }

            if (skillTpl->effectCondition2 == (int)EffectCondType::AFTER_RELEASE) {
                // std::cout << "Cast ReleaseCond 2st " << std::endl;
                T_Effect effect;
                effect.range = static_cast<EffectRangeType>(skillTpl->effectRange2);
                effect.type = static_cast<EffectType>(skillTpl->addedEffect2);
                effect.base_value = skillTpl->additionalValue2;
                effect.up_value = skillTpl->upValue2;

                std::vector<Hero*> effect_targets;
                GetEffectTargets(*hero, effect.range, targets, effect_targets);

                WarReport::BuffData buffData;
                buffData.buffType = skillTpl->addedEffect2;
                buffData.receiverRound = m_bigRound;

                CastAreaEffect(hero, effect, effect_targets, level, buffData);

                skillData.buffs.push_back(buffData);
            }
        }

        void MixedCombat::CastHurtEffect(Hero* hero, Hero* toHero, std::vector<Hero*>& targets,
            std::unordered_map<Hero*, int>& target_map, int level, WarReport::SkillData& skillData) {
            auto skillTpl = hero->nirvanaTpl();

            LOG_DEBUG("Effect condtion1 : %d  condtion2 : %d ", skillTpl->effectCondition1, skillTpl->effectCondition2);
            EffectCondType condType1 = static_cast<EffectCondType>(skillTpl->effectCondition1);
            EffectCondType condType2 = static_cast<EffectCondType>(skillTpl->effectCondition2);

            if ((condType1 == EffectCondType::AFTER_HURT
                || condType1 > EffectCondType::AFTER_RELEASE) && (CheckEffectCond(hero, toHero, condType1))) {
                T_Effect effect;
                effect.range = static_cast<EffectRangeType>(skillTpl->effectRange1);
                effect.type = static_cast<EffectType>(skillTpl->addedEffect1);
                effect.base_value = skillTpl->additionalValue1;
                effect.up_value = skillTpl->upValue1;

                std::vector<Hero*> effect_targets;
                GetEffectTargets(*hero, effect.range, targets, effect_targets);

                LOG_DEBUG("Output after hurt effect lst targets !!! ");
                for(auto h : effect_targets) {
                    LOG_DEBUG("effect hero position :  %d ", h->Position());
                }

                if (effect_targets.empty()) { return; }

                WarReport::BuffData buffData;
                buffData.buffType = skillTpl->addedEffect1;
                buffData.receiverRound = m_bigRound;
                buffData.keepRound = 0;

                for (auto targetPtr : effect_targets) {
                    auto iter = target_map.find(targetPtr);
                    int hurt = 0;
                    if (iter != target_map.end()) {
                        hurt = iter->second;
                    }

                    CastSingleEffect(hero, effect, targetPtr, level, hurt, buffData);
                }

                if (!buffData.receivers.empty()) {
                    skillData.buffs.push_back(buffData);
                }
            }

            // std::cout << " ######### Second Effect " << std::endl;

            if ((condType2 == EffectCondType::AFTER_HURT
                || condType2 > EffectCondType::AFTER_RELEASE) && (CheckEffectCond(hero, toHero, condType2))) {

                T_Effect effect;
                effect.range = static_cast<EffectRangeType>(skillTpl->effectRange2);
                effect.type = static_cast<EffectType>(skillTpl->addedEffect2);
                effect.base_value = skillTpl->additionalValue2;
                effect.up_value = skillTpl->upValue2;

                std::vector<Hero*> effect_targets;
                GetEffectTargets(*hero, effect.range, targets, effect_targets);

                LOG_DEBUG("Output after hurt effect 2st targets !!! ");
                for(auto h : effect_targets) {
                    LOG_DEBUG("effect hero position :  %d ", h->Position());
                }

                WarReport::BuffData buffData;
                buffData.buffType = skillTpl->addedEffect2;
                buffData.receiverRound = m_bigRound;
                buffData.keepRound = 0;

                for (auto targetPtr : effect_targets) {
                    auto iter = target_map.find(targetPtr);
                    int hurt = 0;
                    if (iter != target_map.end()) {
                        hurt = iter->second;
                    }
                    CastSingleEffect(hero, effect, targetPtr, level, hurt, buffData);
                }

                if (!buffData.receivers.empty()) {
                    skillData.buffs.push_back(buffData);
                }
            }
        }

        float MixedCombat::AddPhysicalHurt(Hero* hero, float hurt) {
            if (hero->HasStatus(TargetStatus::ADD_PHYSICAL_DAMAGE)) {
                int nirvana_level = hero->nirvanaLevel();
                auto skillTpl = hero->nirvanaTpl();
                float x = 0;
                if (skillTpl->addedEffect1 == (int)EffectType::ADD_WISDOM_PHYSICAL_DAMAGE) {
                    x = hero->wisdom() * (skillTpl->additionalValue1 + nirvana_level * skillTpl->upValue1) / 100;
                }

                if (skillTpl->addedEffect2 == (int)EffectType::ADD_WISDOM_PHYSICAL_DAMAGE) {
                    x = hero->wisdom() * (skillTpl->additionalValue2 + nirvana_level * skillTpl->upValue2) / 100;
                }
                hurt = hurt * (1 + x);
            }
            return hurt;
        }

        float MixedCombat::AddMagicHurt(Hero* hero, float hurt) {
            if (hero->HasStatus(TargetStatus::ADD_MAGIC_DAMAGE)) {
                int nirvana_level = hero->nirvanaLevel();
                auto skillTpl = hero->nirvanaTpl();
                float x = 0;
                if (skillTpl->addedEffect1 == (int)EffectType::ADD_WISDOM_MAGIC_DAMAGE) {
                    x = (skillTpl->additionalValue1 + nirvana_level * skillTpl->upValue1) / 100;
                }

                if (skillTpl->addedEffect2 == (int)EffectType::ADD_WISDOM_MAGIC_DAMAGE) {
                    x = (skillTpl->additionalValue2 + nirvana_level * skillTpl->upValue2) / 100;
                }
                hurt = hurt * (1 + x);
            }
            return hurt;
        }

        float MixedCombat::MinusPhysicalHurt(Hero* hero, float hurt) {
            if (hero->HasStatus(TargetStatus::MINUS_PHYSICAL_DAMAGE)) {
                int nirvana_level = hero->nirvanaLevel();
                auto skillTpl = hero->nirvanaTpl();
                float x = 0;
                if (skillTpl->addedEffect1 == (int)EffectType::MINUS_PHYSICAL_DAMAGE) {
                    x = (skillTpl->additionalValue1 + nirvana_level * skillTpl->upValue1) / 100;
                }

                if (skillTpl->addedEffect2 == (int)EffectType::MINUS_PHYSICAL_DAMAGE) {
                    x = (skillTpl->additionalValue2 + nirvana_level * skillTpl->upValue2) / 100;
                }
                hurt = hurt * (1 - x);
            }
            return hurt;
        }

        float MixedCombat::MinusMagicHurt(Hero* hero, float hurt) {
            if (hero->HasStatus(TargetStatus::MINUS_MAGIC_DAMAGE)) {
                int nirvana_level = hero->nirvanaLevel();
                auto skillTpl = hero->nirvanaTpl();
                float x = 0;
                if (skillTpl->addedEffect1 == (int)EffectType::MINUS_MAGIC_DAMAGE) {
                    x = (skillTpl->additionalValue1 + nirvana_level * skillTpl->upValue1) / 100;
                }

                if (skillTpl->addedEffect2 == (int)EffectType::MINUS_MAGIC_DAMAGE) {
                    x = (skillTpl->additionalValue2 + nirvana_level * skillTpl->upValue2) / 100;
                }
                hurt = hurt * (1 - x);
            }

            return hurt;
        }

        //实际命中
        bool MixedCombat::CheckHit(Hero* hero, Hero* target) {
            float param = hero->hit() + AssistSum(m_attackTeam, hero) - target->avoid() - AssistSum(m_defenseTeam, target)
            + (float)(target->HasStatus(TargetStatus::PARALYSIS) ? 50 : 0);
            //std::cout << " hit : " << hero->hit() << " assit " << AssistSum(m_attackTeam, hero) << " to->hit: "
            //    << target->avoid() << " to->assit: " << AssistSum(m_defenseTeam, target) << " target paralysis status "
            //    << target->HasStatus(TargetStatus::PARALYSIS) << " param: " << param << std::endl;
            float hit_rate = 90 * pow(1.005, param);

            //普通命中判断
            int rand = m_randEngine.RandBetween(1, 100);
            LOG_DEBUG("Hit rate: %.3f  ---  rand: %d ", hit_rate, rand);
            if ((int)hit_rate < 100  && (int)hit_rate < rand) {
                LOG_DEBUG("Not Hit --- ");
                return false;
            }
            return true;
        };

        //暴击命中判断
        bool MixedCombat::CheckCrit(Hero* hero, Hero* target) {
            float crit_hit = hero->crit_hit() + AssistSum(m_attackTeam, hero) - target->crit_avoid() - AssistSum(m_defenseTeam, target)
                + (target->HasStatus(TargetStatus::PARALYSIS) ? 50 : 0);
            float crit_rate = crit_hit > 0 ? crit_hit : 0;

            bool crit_flag = false;
            int rand = m_randEngine.RandBetween(1, 100);
            LOG_DEBUG("crit rate: %.3f , rand:%d ", crit_rate, rand);
            if ((int)crit_rate > 100 || (int)crit_rate < rand) {
               //暴击命中产生增益效果
               return crit_flag = true;
            }
            return false;
        };

            //产生伤害
        int MixedCombat::CalcHurt(Hero* hero, Hero* target, bool crit_flag) {
            int lastest_hurt = 0;
            float physical_hurt = 0;
            float magic_hurt = 0;
            int float_value = m_randEngine.RandBetween(1, 11) - 6; // -5 ~ 5
            DamageType type = static_cast<DamageType>(hero->damage());
            float pad_value =  hero->army_count() / hero->max_army_count();
            pad_value = pad_value < 0.5 ? 0.5 : pad_value;
            //std::cout << "float_value : " << float_value  << " pad_value: " << pad_value <<  "armyAdapt: " << hero->GetArmyAdapt()
            //    << " army restraint: " << ArmyRestraint(hero->armyType(), target->armyType())
            //    << "wound status "<< hero->HasStatus(TargetStatus::WOUND) << std::endl;
            if (type == DamageType::PHYSICAL_HURT || type == DamageType::MIX_HURT) {
                // std::cout << " physical_attack : " << hero->physical_attack() << " target->defense: " << target->physical_defense()
                //    << std::endl;
                physical_hurt = (pow(hero->physical_attack(), 2) / ((hero->physical_attack() + target->physical_defense()))
                    * pow(1.01, float_value - (hero->HasStatus(TargetStatus::WOUND) ? 50 : 0)
                    +  (target->HasStatus(TargetStatus::WOUND) ? 50 : 0)) + AssistSum(m_attackTeam, hero) - AssistSum(m_defenseTeam, target))
                    * hero->GetArmyAdapt() * (1 + ArmyRestraint(hero->armyType(), target->armyType()))
                    * (1 + (crit_flag ? 1 : 0)) * (1 + 0/100 - 0/100) * pad_value;
                physical_hurt = physical_hurt > 0.0 ? physical_hurt : 0.0;
            } else if (type == DamageType::MAGIC_HURT || type == DamageType::MIX_HURT) {
                magic_hurt = (pow(hero->wisdom_attack(), 2) / ( (hero->wisdom_attack() + target->wisdom_defense()))
                    * pow(1.01, float_value - (hero->HasStatus(TargetStatus::WOUND) ? 50 : 0)
                        +  (target->HasStatus(TargetStatus::WOUND) ? 50 : 0)) + AssistSum(m_attackTeam, hero) - AssistSum(m_defenseTeam, target))
                    * hero->GetArmyAdapt() * (1 + ArmyRestraint(hero->armyType(), target->armyType()))
                    * (1 + (crit_flag ? 1 : 0))  * (1 + 0/100 - 0/100) * pad_value;
                magic_hurt = magic_hurt > 0.0 ? magic_hurt : 0.0;
            }

            // std::cout << " physical_hurt : " << physical_hurt << " magic_hurt: " << magic_hurt << std::endl;

            //攻击方加伤计算
            physical_hurt = AddPhysicalHurt(hero, physical_hurt);
            magic_hurt = AddMagicHurt(hero, magic_hurt);

            //防御方减伤计算
            physical_hurt = MinusPhysicalHurt(target, physical_hurt);
            magic_hurt = MinusMagicHurt(target, magic_hurt);

            lastest_hurt = int(physical_hurt + magic_hurt);
            // std::cout << " physical_hurt : " << physical_hurt << " magic_hurt: " << magic_hurt << std::endl;
            LOG_DEBUG("Skill Attack --- last_hurt: %d  damage_type:%d  target-positon : %d , teamType: %d ", lastest_hurt,
                hero->damage(), target->Position(), target->teamType());

            return lastest_hurt;
        };

        bool MixedCombat::NirvarnaAttack(Hero* hero, Hero* toHero) {
            LOG_DEBUG(" NirvarnaAttack ------------------ ");

            //计算加成，触发必杀技buff
            int nirvana_level = 1;
            auto skillTpl = hero->nirvanaTpl();
            if (skillTpl == nullptr) {
                LOG_DEBUG("nirvarnaTpl is nullptr");
                return false;
            }
            for (auto spell : hero->iGroup().hero.spells) {
                if (skillTpl->id == spell.id ) {
                    nirvana_level = spell.level;
                    break;
                }
            }

            LOG_DEBUG(" SkillTpl --- id : %d ------------------ ", skillTpl->id);

            //基础伤害
            T_Buff tBuff;
            tBuff.id = skillTpl->id;
            tBuff.type = BuffType::NIRVARNA;
            tBuff.target = static_cast<BuffTarget>(skillTpl->skillRange);
            tBuff.base_value1 = skillTpl->basicParameters;
            tBuff.level_value1 = skillTpl->levelParameters;
            tBuff.trigger_state = TriggerBuffState::SMALL_ROUND_START;

            std::vector<Hero*> targets;
            GetBuffTargets(*hero, *toHero, tBuff, targets);

            LOG_DEBUG("Output attack area targets !!! ");
            for(auto hero : targets) {
                LOG_DEBUG("hero target-position :  %d  team : %d ", hero->Position(), hero->teamType());
            }

            //检测执行释放后效果
            WarReport::UnitRound unitRound;
            unitRound.armyId = GetAbsolutePos(hero);
            unitRound.targetId = GetAbsolutePos(toHero);
            unitRound.changeBatVal = 0;
            unitRound.changeRageVal = hero->needRage() * -1;
            unitRound.skill.skillId = skillTpl->id;
            unitRound.changeHp = 0;
            unitRound.castType = WarReport::AttackType::NIRVARNA;
            CastReleaseCondEffect(hero, targets, nirvana_level, unitRound.skill);

            if (targets.empty()) {
                LOG_DEBUG("NirvarnaAttack --- no targets !!!");
                for (auto& v : m_report.allBigRounds) {
                    if (v.bigRoundId == m_bigRound) {
                        v.allUnitRounds.push_back(unitRound);
                    }
                }
                return false;
            }

            std::unordered_map<Hero*, int> hurt_targets_map;
            std::vector<Hero*> hurt_targets;

            LOG_DEBUG(" --------- targets ---------- ");

            //执行BUFF
            for (auto targetPtr : targets) {
                if (targetPtr != nullptr && !targetPtr->IsDie()) {
                    Hero& target = *targetPtr;

                    bool hit_flag = CheckHit(hero, targetPtr);
                    if (!hit_flag) {
                        continue;
                    }

                    bool crit_flag = CheckCrit(hero, targetPtr);
                    int hurt = CalcHurt(hero, targetPtr, crit_flag);

                    if (skillTpl->professionalJob == (int)targetPtr->armyType()) {
                        hurt = hurt * ( 1 + skillTpl->restrainHurt/100);
                    }

                    LOG_DEBUG("NirvarnaAttack after --- lastest hurt: %d ", hurt);

                    int retVal = 0;
                    hero->CastBuff(nirvana_level, tBuff, target, hurt, false, retVal);

                    hurt_targets_map.emplace(targetPtr, hurt);
                    hurt_targets.push_back(targetPtr);
                }
            }

            LOG_DEBUG(" --------- hurt map ---------- ");
            for (auto& iter : hurt_targets_map) {
                LOG_DEBUG(" execute --- hurt ---- %d --- ", iter.second);
            }

            CastHurtEffect(hero, toHero, hurt_targets, hurt_targets_map, nirvana_level, unitRound.skill);

            //战报-必杀
            {
                for (auto& t : hurt_targets_map) {
                    WarReport::Receiver receiver;
                    receiver.armyId = GetAbsolutePos(t.first);
                    receiver.value = t.second * -1;
                    unitRound.skill.receivers.push_back(receiver);
                }

                for (auto& v : m_report.allBigRounds) {
                    if (v.bigRoundId == m_bigRound) {
                        v.allUnitRounds.push_back(unitRound);
                    }
                }
            }

            return true;
        }

        void MixedCombat::SmallRoundCombat()
        {
            if (m_roundNodes.empty()) {
                return;
            }

            auto roundNode = m_roundNodes.front()->ToSmallRoundNode();
            if (roundNode ==  nullptr) {
                return;
            }
            AttackNode attackNode =  roundNode->m_attackNode;
            m_roundNodes.pop_front();
            delete roundNode;

            //小回合战斗计算
            Hero* hero = FromTeam(attackNode.teamType).GetHeroByPos(attackNode.position);
            Hero* toHero = ToTeam(attackNode.teamType).GetHeroByPos(hero->attackTo());
            if (hero == nullptr || toHero == nullptr) {
                assert(0);
                return;
            }

            // std::cout << "========================= SmallRoundReady ======================== " << std::endl;
            // std::cout << " Attack TeamType: " << (int)attackNode.teamType << " attack pos: " << hero->Position()
            //    << " defense pos: " << toHero->Position() << std::endl;

            if (hero->HasStatus(TargetStatus::STUN)) {
                LOG_DEBUG("Attack: %d Target: %d Stun!!! ", hero->Position(), toHero->Position());
                //眩晕状态不记录战报，hero保持不动
                //RecordReport(hero, toHero, WarReport::AttackType::NORMAL, 0, 0);
                return;
            }

            if (toHero->IsDie()) {
                LOG_DEBUG(" Attack: %d Target: %d Die!!! ", hero->Position(), toHero->Position());
                RecordReport(hero, toHero, WarReport::AttackType::NORMAL, 0, 0);
                return;
            }

            //判断怒气值，决定普通攻击，还是必杀技
            bool nirvarna_flag = false;
            // std::cout << " hero rage : " << hero->rage()  << " need rage: " << hero->needRage() << std::endl;
            if ( hero->rage() >= hero->needRage() ) { //怒气值大于40时，触发必杀技
                nirvarna_flag = true;
            }

            if (hero->HasStatus(TargetStatus::TUMBLE)) {
                //混乱状态不能触发必杀，合体
                nirvarna_flag = false;
            }

            bool isAttackValid = false;
            if (nirvarna_flag) {
                isAttackValid = NirvarnaAttack(hero, toHero);
                //消耗怒气值
                hero->AddRage(hero->needRage() * -1);
            } else {
                isAttackValid = NormalAttack(hero, toHero);
                hero->AddRage(20); //命中即加20点怒气
            }

            if (isAttackValid) {
                CheckDie(hero->teamType());
            } else {
                //攻击失败
            }
        }

        void MixedCombat::CheckInvalidRound()
        {
            bool skip = false;
            do {
                if (m_roundNodes.empty()) {
                    return;
                }

                skip = false;

                auto roundNode = m_roundNodes.front();

                // auto disableNode = [&] {
                //     m_roundNodes.pop_front();
                //     delete roundNode;
                //     roundNode = nullptr;
                // };

                if (roundNode->position() > 0) {
                    auto fromGroup = FromTeam(roundNode->teamType()).GetHeroByPos(roundNode->position());
                    if (fromGroup) {
                        if (roundNode->ToSmallRoundNode()) {
                            // if (!fromGroup->canNormalAtk()) {
                            //     disableNode();
                            //     skip = true;
                            // }
                        }
                    }
                }
            } while (skip);
        }

        Hero* MixedCombat::FindRandomHero(Hero* hero) {
            int rand = m_randEngine.RandBetween(1, 18); // -5 ~ 5
            Team* toTeam = &(ToTeam(hero->teamType()));
            if (rand <= 9) {
                toTeam = &(FromTeam(hero->teamType()));
            }

            auto existHero = [&](Team* team, int pos) {
                auto targetHero = team->GetHeroByPos(pos);
                if (targetHero && !targetHero->IsDie()) {
                    return targetHero;
                }
                targetHero = nullptr;
                return targetHero;
            };


            int slot[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
            int last = 9;

            Hero* ptr = nullptr;

            while (last > 1) {
                int rand = m_randEngine.RandBetween(1, last);
                int temp = slot[rand - 1];
                slot[rand - 1] = slot[last -1];
                slot[last - 1] = temp;
                last--;

                if (hero->Position() == temp) { continue; }

                ptr = existHero(toTeam, temp);
                if (ptr) {
                    break;
                }
            }

            return ptr;
        }

        bool MixedCombat::CheckComplex(Hero* hero) {
            LOG_DEBUG(" --- check --- complex --- ");
            auto tpl = hero->heroTpl();
            bool flag = true;

            do {
                if (!hero->complexTpl()) {
                    flag = false;
                    break;
                }

                Team& team = FromTeam(hero->teamType());
                for (auto id : tpl->synergyRoles) {
                    if (!team.GetHeroById(id)) {
                        flag = false;
                        break;
                    }
                }

                if (tpl->synergyRoles.empty()) {
                    flag = false;
                    break;
                }

                if (hero->war() < hero->needWar()) {
                    LOG_DEBUG(" war is not enough !!!");
                    flag = false;
                    break;
                }
            } while(0);

            return flag;
        }

        bool MixedCombat::ComplexAttack(Hero* hero, Hero* toHero) {
             //计算加成，触发必杀技buff
            int complex_level = 1;
            auto skillTpl = hero->complexTpl();
            if (skillTpl == nullptr) {
                LOG_DEBUG("complexTpl is nullptr");
                return false;
            }
            for (auto spell : hero->iGroup().hero.spells) {
                if (skillTpl->id == spell.id ) {
                    complex_level = spell.level;
                    break;
                }
            }

            hero->AddWar(hero->needWar()*-1);

            LOG_DEBUG(" ComplexTpl --- id : %d ------------------ ", skillTpl->id);

            //基础伤害
            T_Buff tBuff;
            tBuff.id = skillTpl->id;
            tBuff.type = BuffType::COMPLEX;
            tBuff.target = static_cast<BuffTarget>(skillTpl->skillRange);
            tBuff.base_value1 = skillTpl->basicParameters;
            tBuff.level_value1 = skillTpl->levelParameters;
            tBuff.trigger_state = TriggerBuffState::SMALL_ROUND_START;

            std::vector<Hero*> targets;
            GetBuffTargets(*hero, *toHero, tBuff, targets);

            LOG_DEBUG("Output attack area targets !!! ");
            for(auto hero : targets) {
                LOG_DEBUG("hero target-position :  %d  team : %d ", hero->Position(), hero->teamType());
            }

            if (targets.empty()) {
                LOG_DEBUG("Complex Attack --- no targets !!!");
                return false;
            }

            //检测执行释放后效果
            WarReport::UnitRound unitRound;
            unitRound.armyId = GetAbsolutePos(hero);
            unitRound.targetId = GetAbsolutePos(toHero);
            unitRound.changeBatVal = hero->needWar() * -1;
            unitRound.changeRageVal = 0;
            unitRound.skill.skillId = skillTpl->id;
            unitRound.changeHp = 0;
            unitRound.castType = WarReport::AttackType::COMPLEX;
            CastReleaseCondEffect(hero, targets, complex_level, unitRound.skill);

            std::unordered_map<Hero*, int> hurt_targets_map;
            std::vector<Hero*> hurt_targets;

            LOG_DEBUG(" --------- targets ---------- ");


            //执行BUFF
            for (auto targetPtr : targets) {
                if (targetPtr != nullptr && !targetPtr->IsDie()) {
                    Hero& target = *targetPtr;

                    bool hit_flag = CheckHit(hero, targetPtr);
                    if (!hit_flag) {
                        continue;
                    }

                    bool crit_flag = CheckCrit(hero, targetPtr);
                    int hurt = CalcHurt(hero, targetPtr, crit_flag);

                    if (skillTpl->professionalJob == (int)targetPtr->armyType()) {
                        hurt = hurt * ( 1 + skillTpl->restrainHurt/100);
                    }

                    LOG_DEBUG("Complex Attack after --- lastest hurt: %d ", hurt);

                    int retVal = 0;
                    hero->CastBuff(complex_level, tBuff, target, hurt, false, retVal);

                    hurt_targets_map.emplace(targetPtr, hurt);
                    hurt_targets.push_back(targetPtr);
                }
            }

            LOG_DEBUG(" --------- hurt map ---------- ");
            for (auto& iter : hurt_targets_map) {
                LOG_DEBUG(" execute --- hurt ---- %d --- ", iter.second);
            }

            CastHurtEffect(hero, toHero, hurt_targets, hurt_targets_map, complex_level, unitRound.skill);

            //战报-必杀
            {
                for (auto& t : hurt_targets_map) {
                    WarReport::Receiver receiver;
                    receiver.armyId = GetAbsolutePos(t.first);
                    receiver.value = t.second * -1;
                    unitRound.skill.receivers.push_back(receiver);
                }

                for (auto& v : m_report.allBigRounds) {
                    if (v.bigRoundId == m_bigRound) {
                        v.complexRounds.push_back(unitRound);
                    }
                }
            }

            return true;
        }

        void MixedCombat::ComplexRoundCombat() {
            if (m_roundNodes.empty()) {
                return;
            }

            auto roundNode = m_roundNodes.front()->ToComplexRoundNode();
            if (roundNode ==  nullptr) {
                return;
            }
            AttackNode attackNode =  roundNode->m_attackNode;
            m_roundNodes.pop_front();
            delete roundNode;

            //小回合战斗计算
            Hero* hero = FromTeam(attackNode.teamType).GetHeroByPos(attackNode.position);
            Hero* toHero = ToTeam(attackNode.teamType).GetHeroByPos(hero->attackTo());
            if (hero == nullptr || toHero == nullptr) {
                assert(0);
                return;
            }

            // std::cout << "========= Complex Attack ========== " << std::endl;
            // std::cout << " Attack TeamType: " << (int)attackNode.teamType << " attack pos: " << hero->Position()
            //    << " defense pos: " << toHero->Position() << std::endl;

            if (toHero->IsDie()) {
                LOG_DEBUG(" Attack: %d Target: %d Die!!! ", hero->Position(), toHero->Position());
                RecordReport(hero, toHero, WarReport::AttackType::NORMAL, 0, 0);
                return;
            }

            // std::cout << " hero war : " << hero->war()  << " need war: " << hero->needWar() << std::endl;
            if (hero->war() <= hero->needWar()) {
                return;
            }

            bool isAttackValid = ComplexAttack(hero, toHero);

            if (isAttackValid) {
                CheckDie(hero->teamType());
            } else {
                //攻击失败
            }
        }

        void MixedCombat::BigRoundBegin()
        {
            m_attackTeam.UpdateBuff(TriggerBuffState::BIG_ROUND_START);
            LOG_DEBUG("########## BigRoundBegin-Update ----- buff --- ");
            m_defenseTeam.UpdateBuff(TriggerBuffState::BIG_ROUND_START);

            for (auto& hero : m_attackTeam.GetHeroList()) {
                LOG_DEBUG("Attack Position %d  amryHp: %d ", hero.Position(), hero.armyHp());
            }

            for (auto& hero : m_defenseTeam.GetHeroList()) {
                LOG_DEBUG("Defense Position %d  amryHp: %d ", hero.Position(), hero.armyHp());
            }

            ++m_bigRound;
            // std::cout << " !!!!!!!!!!!!!!!!!!!!!!!!!! BigRoundBegin index : " << m_bigRound << " !!! " << std::endl;

            //重新设置对位
            std::list<Hero>& attackHeros = m_attackTeam.GetHeroList();
            for (auto& fromHero : attackHeros) {
                if (fromHero.IsDie()) {
                    continue;
                }
                Hero* toNewHero = FindHeroAttackTo(&fromHero);
                if (toNewHero) {
                    fromHero.SetAttackTo(toNewHero->Position());
                    toNewHero->AddBeAttackFrom(fromHero.Position());
                } else {
                    fromHero.SetAttackTo(0);
                }
            }

            std::list<Hero>& defenseHero = m_defenseTeam.GetHeroList();
            for (auto& fromHero : defenseHero) {
                if (fromHero.IsDie()) {
                    continue;
                }
                Hero* toNewHero = FindHeroAttackTo(&fromHero);
                if (toNewHero) {
                    fromHero.SetAttackTo(toNewHero->Position());
                    toNewHero->AddBeAttackFrom(fromHero.Position());
                } else {
                    fromHero.SetAttackTo(0);
                }
            }

            for (auto& hero : m_attackTeam.GetHeroList()) {
                hero.AddWar(20);
            }

            for (auto& hero : m_defenseTeam.GetHeroList()) {
                hero.AddWar(20);
            }

            for (auto& hero : m_attackTeam.GetHeroList()) {
                if(hero.HasStatus(TargetStatus::TUMBLE)) {
                    continue;
                }

                if (CheckComplex(&hero)) {
                    LOG_DEBUG("-- Attack --- CheckComplex --- Success --- ");
                    m_roundNodes.emplace_front(new ComplexRoundNode(hero.teamType(),  hero.Position()));
                }
            }

            for (auto& hero : m_defenseTeam.GetHeroList()) {
                if(hero.HasStatus(TargetStatus::TUMBLE)) {
                    continue;
                }

                if (CheckComplex(&hero)) {
                    LOG_DEBUG("-- Defense --- CheckComplex --- Success --- ");
                    m_roundNodes.emplace_front(new ComplexRoundNode(hero.teamType(),  hero.Position()));
                }
            }

            CheckAttackNodeOrder();

            if (ExistTurret()) {
                m_roundNodes.emplace_front(new TurretRoundNode());
            }
            m_state = MixedCombatState::WAITING;

            //war report
            WarReport::BigRound bigRound;
            bigRound.bigRoundId = m_bigRound;

            // for (auto& hero : m_attackTeam.GetHeroList()) {
            //     WarReport::HeroState heroState;
            //     hero.OutPutEffects(heroState.status);
            //     heroState.armyId = GetAbsolutePos(&hero);
            //     bigRound.heroStatus.push_back(heroState);
            // }

            // for (auto& hero : m_defenseTeam.GetHeroList()) {
            //     WarReport::HeroState heroState;
            //     hero.OutPutEffects(heroState.status);
            //     heroState.armyId = GetAbsolutePos(&hero);
            //     bigRound.heroStatus.push_back(heroState);
            // }

            m_report.allBigRounds.push_back(bigRound);
        }

        void MixedCombat::BigRoundEnd()
        {
            CheckDie(m_attackTeam.teamType());
            CheckDie(m_defenseTeam.teamType());

            if (m_attackTeam.IsAllDie() || m_defenseTeam.IsAllDie()) {
                LOG_DEBUG("ALL--------- Die attack: %d  ", m_attackTeam.IsAllDie());
                m_isEnd = true;
            }

            if (IsEnd()) {
                return;
            }

            m_smallRound = 0;
            if (m_bigRound >= m_mixedMaxRound) {
                m_isEnd = true;
            }
            if (!IsEnd()) {
                BigRoundBegin();
            }
        }

        void MixedCombat::RoundEnd()
        {
            m_curAttacker = nullptr;

            // if (m_attackTeam.IsAllDie() || m_defenseTeam.IsAllDie()) {
            //      LOG_DEBUG("ALL--------- Die attack: %d  ", m_attackTeam.IsAllDie());
                    //Todo: 全死亡先不结束
                    //m_isEnd = true;
            // } else {
                switch (m_state) {
                    case MixedCombatState::TURRET_READY:
                        m_state = MixedCombatState::WAITING;
                        break;
                    case MixedCombatState::SMALL_READY:
                        if (m_roundNodes.empty()) {
                            BigRoundEnd();
                        } else {
                            //CheckSpellNodeOrder();
                            m_state = MixedCombatState::WAITING;
                        }
                        break;
                    case MixedCombatState::COMPLEX_READY:
                        if (m_roundNodes.empty()) {
                            BigRoundEnd();
                        } else {
                            m_state = MixedCombatState::WAITING;
                        }
                        break;
                    default:
                        m_isEnd = true;
                        break;
                }
            //}
        }

        bool MixedCombat::ExistTurret()
        {
            return m_defenseTeam.turretAtkPower() > 0;
        }

        bool MixedCombat::TurretRoundReady()
        {
            if (m_roundNodes.empty()) {
                return false;
            }

            auto roundNode = m_roundNodes.front()->ToTurretRoundNode();
            if (roundNode ==  nullptr) {
                return false;
            }

            for (int pos = 1; pos <=  9; ++pos) {
                Hero* toHero = m_attackTeam.GetHeroByPos(pos);
                if (toHero != nullptr && !toHero->IsDie()) {
                    break;
                }
            }
            return true;
        }

        void MixedCombat::TurretRoundCombat()
        {
            auto roundNode = m_roundNodes.front()->ToTurretRoundNode();
            if (roundNode ==  nullptr) {
                return;
            }
            m_roundNodes.pop_front();
            delete roundNode;


            //Todo: modify
            /*
            Hero* toHero = m_attackTeam.GetHeroByPos(m_roundInput.targetPos);
            if (toHero == nullptr) {
                assert(0);
                return;
            }

            m_roundOutput.Reset();

            //箭塔伤害
            //int turretAtkPower = m_defenseTeam.turretAtkPower();
            //int hurt = turretAtkPower * turretAtkPower / (turretAtkPower + toHero->armyDefense()) * 1;
            int hurt = 0;
            toHero->AddArmyHp(-hurt);

            CheckDie(m_defenseTeam.teamType());
            */
        }

        void MixedCombat::End()
        {
            if (!m_isEnd) {
                return;
            }

            m_result.attackNewPower = m_attackTeam.TotalPower();
            m_result.defenseNewPower = m_defenseTeam.TotalPower();
            m_endArmyCount = m_attackTeam.RemainTotalHp();

            m_attackTeam.OnEnd();
            m_defenseTeam.OnEnd();

             //if (m_attackTeam.RemainTotalHp() > m_defenseTeam.RemainTotalHp()) {
            if (m_defenseTeam.RemainTotalHp() <= 0) {
                m_result.isAttackWin = true;
            } else {
                m_result.isAttackWin = false;
            }

            m_result.attackNewPower = m_attackTeam.TotalPower();
            m_result.defenseNewPower = m_defenseTeam.TotalPower();

            std::list<Hero>& attackHeros = m_attackTeam.GetHeroList();
            for (auto& group : attackHeros) {
                MixedCombatResult::ArmyHurt armyHurt;
                armyHurt.heroId = group.tplId();
                armyHurt.heroHp = group.armyHp();
                armyHurt.armyType = (int)group.armyType();
                armyHurt.activeCount = group.activeCount();
                armyHurt.dieCount = group.dieCount();
                armyHurt.killCount = group.killCount();
                LOG_DEBUG("attack active : %d, die : %d , kill : %d", group.activeCount(), group.dieCount(), group.killCount());
                m_result.attackHurt.push_back(armyHurt);
            }

            for (auto& hero : attackHeros) {
                MixedCombatResult::Hurt hurt;
                hurt.heroId = hero.tplId();
                hurt.armyType = (int)hero.armyType();
                hurt.dieCount = hero.totalDieCount();
                hurt.armyLevel = hero.armyLevel();
                LOG_DEBUG("attack total  die : %d ", hero.totalDieCount());
                m_result.attackTotalHurt.push_back(hurt);

                m_result.attackTotalDie += hero.totalDieCount();
            }

            std::list<Hero>& defenseHeros = m_defenseTeam.GetHeroList();
            for (auto& group : defenseHeros) {
                MixedCombatResult::ArmyHurt armyHurt;
                armyHurt.heroId = group.tplId();
                armyHurt.heroHp = group.armyHp();
                armyHurt.armyType = (int)group.armyType();
                armyHurt.activeCount = group.activeCount();
                armyHurt.dieCount = group.dieCount();
                armyHurt.killCount = group.killCount();
                LOG_DEBUG("defense active : %d, die : %d , kill : %d", group.activeCount(), group.dieCount(), group.killCount());
                m_result.defenseHurt.push_back(armyHurt);
            }

            for (auto& hero : defenseHeros) {
                MixedCombatResult::Hurt hurt;
                hurt.heroId = hero.tplId();
                hurt.armyType = (int)hero.armyType();
                hurt.dieCount = hero.totalDieCount();
                hurt.armyLevel = hero.armyLevel();
                LOG_DEBUG("defense total  die : %d ", hero.totalDieCount());
                m_result.defenseTotalHurt.push_back(hurt);

                m_result.defenseTotalDie += hero.totalDieCount();
            }

            ClearNode();
        }

        void MixedCombat::ClearNode()
        {
            for (auto it = m_roundNodes.begin(); it !=  m_roundNodes.end(); ++it) {
                    delete *it;
            }
            m_roundNodes.clear();
        }

        void MixedCombat::CheckAttackNodeOrder()
        {
            //插入进攻节点
            auto insertAttackNode = [this](const Hero &group) {
                if (group.IsDie()) {
                    return;
                }

                Hero* toGroup = ToTeam(group.teamType()).GetHeroByPos(group.attackTo());
                if (!toGroup || toGroup->IsDie()) {
                    return;
                }

                AttackNode attackNode(group.teamType(), group.Position());

                auto itAttackNode = m_attackNodes.begin();
                for (; itAttackNode != m_attackNodes.end(); ++ itAttackNode) {
                    Hero* nodeGroup = FromTeam(itAttackNode->teamType).GetHeroByPos(itAttackNode->position);
                    if (nodeGroup != nullptr) {
                        if (group.armySpeed() > nodeGroup->armySpeed()) {
                            m_attackNodes.insert(itAttackNode, attackNode);
                            break;
                        } else if (group.armySpeed() == nodeGroup->armySpeed()) {
                            if (group.teamType() == m_singleWinner && group.teamType() != nodeGroup->teamType()) {
                                m_attackNodes.insert(itAttackNode, attackNode);
                                break;
                            }

                            if (m_singleWinner == TeamType::NONE && group.teamType() < nodeGroup->teamType()) {
                                m_attackNodes.insert(itAttackNode, attackNode);
                                break;
                            }
                        }
                    }
                }

                if (itAttackNode == m_attackNodes.end()) {
                    m_attackNodes.push_back(attackNode);
                }
            };

            const std::list<Hero>& attackGroups = m_attackTeam.GetHeroList();
            for (auto& group : attackGroups) {
                insertAttackNode(group);
            }

            const std::list<Hero>& defenseGroup = m_defenseTeam.GetHeroList();
            for (auto& group : defenseGroup) {
                insertAttackNode(group);
            }

            auto itAttackNode = m_attackNodes.begin();
            for (; itAttackNode != m_attackNodes.end(); ++ itAttackNode) {
                m_roundNodes.emplace_back(new SmallRoundNode(itAttackNode->teamType,  itAttackNode->position));
            }
            m_attackNodes.clear();
        }

        void MixedCombat::OnHeroDie(Hero* hero)
        {
            if (nullptr != hero) {
                // 清除回合节点
                for (auto it = m_roundNodes.begin(); it != m_roundNodes.end();) {
                    auto roundNode = *it;
                    if (roundNode->IsOwner(hero->teamType(),  hero->Position())) {
                        delete roundNode;
                        it = m_roundNodes.erase(it);
                    } else {
                        ++it;
                    }
                }

                //更新对位
                Hero* toGroup = ToTeam(hero->teamType()).GetHeroByPos(hero->attackTo());
                if (toGroup) {
                    toGroup->RemoveBeAttackFrom(hero->Position());
                }

                const std::list<int>& beAttackFrom = hero->beAttackFrom();
                for (auto fromPos : beAttackFrom) {
                    Hero* fromGroup  = ToTeam(hero->teamType()).GetHeroByPos(fromPos);
                    if (fromGroup) {
                        Hero* toNewGroup = FindHeroAttackTo(fromGroup);
                        if (toNewGroup) {
                            fromGroup->SetAttackTo(toNewGroup->Position());
                            toNewGroup->AddBeAttackFrom(fromGroup->Position());
                        } else {
                            fromGroup->SetAttackTo(0);
                        }
                    }
                }

                if (FromTeam(hero->teamType()).IsAllDie()) {
                    m_isEnd = true;
                }
//                 group->OnDie();
            }
        }

        Hero* MixedCombat::FindHeroAttackTo(Hero* hero)
        {
            Team& toTeam = ToTeam(hero->teamType());
            if (toTeam.IsAllDie()) {
                return nullptr;
            }

            if (nullptr != hero) {
                int posLine = hero->Position() % 3;
                if (posLine == 0) {
                    posLine = 3;
                }

                int prevLine = 0;
                for (int row = 0; row < 3; ++row)
                {
                    for (int line = 0; line < 3; ++line)
                    {
                        int toPos = posLine + 3 * line;
                        Hero* toGroup = toTeam.GetHeroByPos(toPos);
                        if (toGroup != nullptr && !toGroup->IsDie()) {
                            return toGroup;
                        }
                    }

                    int tempLine = posLine;
                    switch (posLine)
                    {
                        case 1:
                            if (row == 1 && prevLine == 2) {
                                posLine = 3;
                            } else {
                                posLine = 2;
                            }
                        break;
                        case 2:
                            if (row == 0 || prevLine == 3)
                            {
                                posLine = 1;
                            }
                            else
                            {
                                posLine = 3;
                            }
                        break;
                        case 3:
                            if (row == 0) {
                                posLine = 2;
                            }
                            else
                            {
                                posLine = 3;
                            }
                        break;
                        default:
                        break;
                    }

                   prevLine = tempLine;
                }
            }
            return nullptr;
        }

        //兵种克制
        int MixedCombat::ArmyRestraint(ArmyType type, ArmyType otherType)
        {
            if (type == ArmyType::MACHINE || otherType == ArmyType::MACHINE) {
                return 0;
            }

            if (type == otherType) {
                return 0;
            }

            if (type < otherType && otherType != ArmyType::ARCHER) {
                return -0.5;
            }

            return 0.5;
        }

        void MixedCombat::OnBeAttacked(Hero* toGroup, Hero* fromGroup)
        {
            if (toGroup && fromGroup) {
                if (toGroup->IsDie()) {
                    return;
                }
            }
        }

        void MixedCombat::GetBuffTargets(Hero& fromGroup, Hero& toGroup, const engine::tpl::T_Buff& tBuff, std::vector<Hero*> &targets)
        {
            //矩阵攻击范围
            auto calcAttackArea = [&]() {
                LOG_DEBUG("Get buff targets ---  id --- %d ", tBuff.id);

                auto skillTpl = model::tpl::g_tploader->FindSkill(tBuff.id);

                if (skillTpl->skillRange != 0) {
                    LOG_DEBUG("Get buff targets ---  skillRange --- %d ", skillTpl->skillRange);
                    auto attackRangeTpl =  model::tpl::g_tploader->FindAttackRange(skillTpl->skillRange);
                    auto iter = attackRangeTpl->m_attack_matrix.find(toGroup.Position());
                    if (iter != attackRangeTpl->m_attack_matrix.end()) {
                            for (auto pos : iter->second) {
                            auto targetGroup = ToTeam(fromGroup.teamType()).GetHeroByPos(pos);
                            if (targetGroup) {
                                if (!targetGroup->IsDie()) {
                                    targets.push_back(targetGroup);
                                }
                            }
                        }
                    }
                    return;
                }
            };

            //获取所在行
            auto getRow = [](int position) {
                return position / 3 + (position % 3 > 0 ? 1 : 0);
            };

            //获取所在列
            auto getLine = [](int position) {
                int line =  position % 3;
                if (line == 0) {
                    line = 3;
                }
                return line;
            };

            auto getPos = [](int row, int line) {
                int position = 0;
                if (row >= 1 && row <= 3 && line >= 1 && line <= 3) {
                    position = line + (row - 1) * 3;
                }
                return position;
            };

            auto addSelfGroup = [&](int pos) {
                //// std::cout << " teamType: " << (int)fromGroup.teamType() << " pos :  " << pos << std::endl;
                auto targetGroup = FromTeam(fromGroup.teamType()).GetHeroByPos(pos);
                if (targetGroup) {
                    if (!targetGroup->IsDie()) {
                        targets.push_back(targetGroup);
                        return true;
                    }
                }
                return false;
            };

            auto addOtherGroup = [&](int pos) {
                auto targetGroup = ToTeam(fromGroup.teamType()).GetHeroByPos(pos);
                if (targetGroup) {
                    if (!targetGroup->IsDie()) {
                        targets.push_back(targetGroup);
                        return true;
                    }
                }
                return false;
            };

            auto addAnyGroups = [&](int num) {
                int slot[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
                int last = 9;
                int count = 0;

                while (last > 1) {
                    int rand = m_randEngine.RandBetween(1, last);
                    int temp = slot[rand - 1];
                    slot[rand - 1] = slot[last -1];
                    slot[last - 1] = temp;
                    last--;

                    addOtherGroup(temp);
                    if (targets.size() < (unsigned)count) {
                        count++;
                    }

                    if (count >= num) {
                        break;
                    }
                }
            };

            //--------------------

            int fromPos = fromGroup.Position();
            int toPos = fromGroup.attackTo();
            if (toPos ==  0) {
                // std::cout << "toPos ==  0" << std::endl;
            }
            //assert(toPos > 0);

            // std::cout<< "FromPos : " << fromPos << " Buff Target ------ : " << (int)tBuff.target << std::endl;

            //Todo: 先处理被动技能
            switch (tBuff.target) {
                case BuffTarget::OTHER_SINGLE: {
                    addOtherGroup(toPos);
                }
                break;
                case BuffTarget::OTHER_DOUBLE: {
                    int row = getRow(toPos);
                    int frontRow = row - 1;
                    int backRow = row + 1;
                    int count = 0;
                    if (frontRow > 0) {
                        for (int line = 1; line <=3; line++) {
                            int pos = getPos(frontRow, line);
                            addOtherGroup(pos);
                            if (!targets.empty()) {
                                count++;
                                break;
                            }
                        }
                    }

                    if (backRow <= 3 && count == 0) {
                        for (int line = 1; line <=3; line++) {
                            int pos = getPos(backRow, line);
                            addOtherGroup(pos);
                            if (!targets.empty()) {
                                count++;
                                break;
                            }
                        }
                    }

                    assert(count <= 1);
                    addOtherGroup(toPos);
                }
                break;
                case BuffTarget::OTHER_THREE: {
                    int row = getRow(toPos);
                    int frontRow = row - 1;
                    int backRow = row + 1;
                    int count = 0;
                    if (frontRow > 0) {
                        for (int line = 1; line <=3; line++) {
                            int pos = getPos(frontRow, line);
                            addOtherGroup(pos);
                            if (!targets.empty()) {
                                count++;
                                break;
                            }
                        }
                    }

                    if (backRow <= 3 ) {
                        for (int line = 1; line <=3; line++) {
                            int pos = getPos(backRow, line);
                            addOtherGroup(pos);
                            if (!targets.empty()) {
                                count++;
                                break;
                            }
                        }
                    }

                    assert(count <= 2);
                    addOtherGroup(toPos);
                }
                break;
                case BuffTarget::ANY_SINGLE: {
                    addAnyGroups(1);
                }
                break;
                case BuffTarget::ANY_DOUBLE: {
                    addAnyGroups(2);
                }
                break;
                case BuffTarget::ANY_THREE: {
                    addAnyGroups(3);
                }
                break;
                case BuffTarget::OTHER_BACK_ONE: {
                    int line = getLine(toPos);
                    int row = getRow(toPos);
                    int count = 0;

                    int backest_line = 0;
                    if (line == 1) {
                        backest_line = 3;
                    } else {
                        backest_line +=1;
                    }

                    if (backest_line <= 3)
                    {
                        int pos = getPos(row, backest_line);
                        addOtherGroup(pos);
                        if (!targets.empty()) {
                            count++;
                        }
                    }

                    if (line == 1 && count== 0) {
                        int pos = getPos(row, 2);  //第二列
                        addOtherGroup(pos);
                        if (!targets.empty()) {
                            count++;
                        }
                    }
                }
                break;
                case BuffTarget::OTHER_BACK_COLUME: {
                    int line = getLine(toPos);
                    int count = 0;

                    int backest_line = 0;
                    if (line == 1) {
                        backest_line = 3;
                    } else {
                        backest_line +=1;
                    }

                    if (backest_line <= 3)
                    {
                        for (int i = 1; i <= 3; i++) {
                            int pos = getPos(i, backest_line);
                            addOtherGroup(pos);
                            if (!targets.empty()) {
                                count++;
                            }
                        }
                    }

                    if (line == 1 && count== 0) {
                        for (int i = 1; i <= 3; i++) {
                            int pos = getPos(i, 2);
                            addOtherGroup(pos);
                        }
                    }
                }
                break;
                case BuffTarget::ME: {
                    addSelfGroup(fromPos);
                }
                break;
                case BuffTarget::SELF_EXCEPT_ME: {
                    for (int pos = 1; pos <= 9; ++pos) {
                        if (pos != toPos) {
                            addSelfGroup(pos);
                        }
                    }
                }
                break;
                case BuffTarget::SELF_ALL: {
                    for (int pos = 1; pos <= 9; ++pos) {
                        addSelfGroup(pos);
                    }
                }
                break;
                case BuffTarget::OTHER_ALL: {
                    for (int pos = 1; pos <= 9; ++pos) {
                        addOtherGroup(pos);
                    }
                }
                break;
                default:{
                    calcAttackArea();
                }
                break;
            }
        }

        void MixedCombat::CastBuff(Hero& fromGroup, Hero& toGroup, const engine::tpl::T_Buff& tBuff,  int spellLevel, bool isPassive, int hurt)
        {
            std::vector<Hero*> targets;
            GetBuffTargets(fromGroup, toGroup, tBuff, targets);

            //执行BUFF
            for (auto targetPtr : targets) {
                if (targetPtr != nullptr && !targetPtr->IsDie()) {
                        Hero& target = *targetPtr;
                        int retVal = 0;
                        fromGroup.CastBuff(spellLevel, tBuff, target, 0, isPassive, retVal);
                }
            }
        }

        bool MixedCombat::CheckEffectCond(Hero* fromHero, Hero* target, EffectCondType condType)
        {
            bool ret = false;
            switch (condType) {
                case EffectCondType::AFTER_HURT: {
                    ret = true;
                }
                break;
                case EffectCondType::SELF_INFANTRY: {
                    if (fromHero->armyType() == ArmyType::INFANTRY) {
                        ret = true;
                    }
                }
                break;
                case EffectCondType::SELF_RIDER: {
                    if (fromHero->armyType() == ArmyType::RIDER) {
                        ret = true;
                    }
                }
                break;
                case EffectCondType::SELF_ARCHER: {
                    if (fromHero->armyType() == ArmyType::ARCHER) {
                        ret = true;
                    }
                }
                break;
                case EffectCondType::SELF_MACHINE: {
                    if (fromHero->armyType() == ArmyType::MACHINE) {
                        ret = true;
                    }
                }
                break;
                case EffectCondType::TARGET_INFANTRY: {
                    if (target->armyType() == ArmyType::INFANTRY) {
                        ret = true;
                    }
                }
                break;
                case EffectCondType::TARGET_RIDER: {
                    if (target->armyType() == ArmyType::RIDER) {
                        ret = true;
                    }
                }
                break;
                case EffectCondType::TARGET_ARCHER: {
                    if (target->armyType() == ArmyType::ARCHER) {
                        ret = true;
                    }
                }
                break;
                case EffectCondType::TARGET_MACHINE: {
                    if (target->armyType() == ArmyType::MACHINE) {
                        ret = true;
                    }
                }
                break;
                default:
                    break;
            }
            return ret;
        }
    }
}


