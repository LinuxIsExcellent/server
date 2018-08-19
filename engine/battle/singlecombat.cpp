#include <stdint.h>
#include <base/logger.h>
#include <base/framework.h>
#include <algorithm>
#include "../randengine.h"
#include "../tpl/t_hero.h"

#include "singlecombat.h"

namespace engine
{
    namespace battle
    {
        SingleCombat::~SingleCombat()
        {
        }

        void SingleCombat::Init(I_Team& attackTeam, I_Team& defenceTeam)
        {

            // 找出双方战斗力最大并且体力不为0的武将
            auto getSingleHeroIndex = [this](I_Team & team, engine::TeamType type) {    
                int heroIndex = -1;
                int physical_attack = 0;
                
                // int rand = m_randEngine.RandBetween(1, totalChallenge);
                for (int i = 0; i < (int)team.groups.size(); ++i) {
                    if (team.groups[i].hero.physical > 0) {
                        Hero tempNhero;
                        tempNhero.Init(team.groups[i].hero,type);
                        if (tempNhero.physical_attack() > physical_attack)
                        {
                            heroIndex = i;
                            physical_attack = tempNhero.physical_attack();
                        }
                    }
                }
                return heroIndex;
            };
            int attackChallengeHero = getSingleHeroIndex(attackTeam, engine::TeamType::ATTACKER);
            int defenceChallengeHero=  getSingleHeroIndex(defenceTeam, engine::TeamType::DEFENDER);
            // 如果双方都没有合适的武将进行单挑
            if (attackChallengeHero < 0 && defenceChallengeHero < 0)
            {
                m_result.attackLoseArmyPercent = model::tpl::g_tploader->configure().soloLose.runaway;
                m_result.defenseLostArmyPercent = model::tpl::g_tploader->configure().soloLose.runaway;
                return;
            }
            else if (attackChallengeHero < 0 || defenceChallengeHero < 0)
            {
                if (attackChallengeHero < 0)
                {
                    m_result.isAttackWin = false;
                    m_result.attackLoseArmyPercent = model::tpl::g_tploader->configure().soloLose.death;
                }
                else if (defenceChallengeHero < 0)
                {
                    m_result.isAttackWin = true;
                    m_result.defenseLostArmyPercent = model::tpl::g_tploader->configure().soloLose.death;
                }
                return;
            }

            Begin(attackTeam.groups[attackChallengeHero].hero, defenceTeam.groups[defenceChallengeHero].hero);
            m_state = SingleCombatState::READY;
        }

        void SingleCombat::Init(I_Hero& attackHero, I_Hero& defenceHero)
        {
            m_attackSoloReport.rounds.clear();
            m_defenseSoloReport.rounds.clear();
            Begin(attackHero, defenceHero);
            m_state = SingleCombatState::READY;
        }

        bool SingleCombat::Start()
        {
            if (m_state != SingleCombatState::READY) {
                return false;
            }
            while (SingleCombatState::OVER != Next());
            return true;
        }

        SingleCombatState SingleCombat::Next()
        {
            switch (m_state) {
                case SingleCombatState::READY:
                    RoundBegin();
                    RoundCombat();
                    RoundEnd();
                    if (IsEnd()) {
                        End();
                        m_state = SingleCombatState::OVER;
                    } else {
                        Ready();
                        m_state = SingleCombatState::READY;
                    }
                    break;
                default:
                    break;
            }
            return m_state;
        }

        bool SingleCombat::AttackInsertSpell(int spellBaseId)
        {
            return false;
        }

        void SingleCombat::Ready()
        {
            ++m_round;
            m_attackInput.Reset();
            m_defenseInput.Reset();

            m_attackInput.round = m_round;
            m_attackInput.hp = m_attackHero.hp();
            m_attackInput.rage = m_attackRage;

            m_defenseInput.round = m_round;
            m_defenseInput.hp = m_defenseHero.hp();
            m_defenseInput.rage = m_defenseRage;


            /*m_attackInput.canHurt = GetHurt(m_attackHero, m_defenseHero);
            m_defenseInput.canHurt = GetHurt(m_defenseHero, m_attackHero);*/
            //计算双方攻击招式
            CheckSpell();
        }

        bool SingleCombat::IsEscape(const int hpPro)
        {
            int pro = 0;
            for (std::vector<model::tpl::EscapePro>::const_iterator it = model::tpl::g_tploader->configure().escapePros.begin(); it != model::tpl::g_tploader->configure().escapePros.end(); ++it) {
                if (it->hpMin < hpPro && hpPro <= it->hpMax)
                {
                    pro = it->pro;
                }
            }
            int r = framework.random().GenRandomNum(10000);
            if ( pro - 1 >= r)
            {
                return true;
            }
            return false;
        }

        void SingleCombat::CheckSpell()
        {
            //  先判断是否逃跑
            if (IsEscape(m_attackHero.hp() * 10000 / m_attackMaxPhysical))
            {
                m_attackNextSkill = WarReport::HeroSoloSkill::ESCAPE;           
            }
            // 再判断是否是必杀
            else if (m_attackRage >= g_tploader->configure().soloRage.consume)
            {
                m_attackNextSkill = WarReport::HeroSoloSkill::SOLO_NIRVARNA;
            }
            // 最后判断普通技能
            else
            {
                int total_probability = m_attackSkillProb.stone + m_attackSkillProb.shear + m_attackSkillProb.cloth;
                if (total_probability > 0) {
                    int r = framework.random().GenRandomNum(total_probability);
                    if (m_attackSkillProb.stone - 1 >= r)
                    {
                        m_attackNextSkill = WarReport::HeroSoloSkill::STONE;
                    }
                    else if (m_attackSkillProb.stone + m_attackSkillProb.shear - 1 >= r)
                    {
                        m_attackNextSkill = WarReport::HeroSoloSkill::SHEAR;   
                    }
                    else if (m_attackSkillProb.stone + m_attackSkillProb.shear + m_attackSkillProb.cloth - 1 >= r)
                    {
                        m_attackNextSkill = WarReport::HeroSoloSkill::CLOTH;   
                    }
                }
            }
            // 先判断是否逃跑
            if (IsEscape(m_defenseHero.hp() * 10000 / m_defenseMaxPhysical))
            {
                m_defenseNextSkill = WarReport::HeroSoloSkill::ESCAPE;
            }
            // 再判断是否是必杀
            else if (m_defenseRage >= g_tploader->configure().soloRage.consume)
            {
                m_defenseNextSkill = WarReport::HeroSoloSkill::SOLO_NIRVARNA;
            }
            // 最后判断普通技能
            else
            {
                int total_probability = m_defenseSKillProb.stone + m_defenseSKillProb.shear + m_defenseSKillProb.cloth;
                if (total_probability > 0) {
                    int r = framework.random().GenRandomNum(total_probability);
                    if (m_defenseSKillProb.stone - 1 >= r)
                    {
                        m_defenseNextSkill = WarReport::HeroSoloSkill::STONE;
                    }
                    else if (m_defenseSKillProb.stone + m_defenseSKillProb.shear - 1 >= r)
                    {
                        m_defenseNextSkill = WarReport::HeroSoloSkill::SHEAR;   
                    }
                    else if (m_defenseSKillProb.stone + m_defenseSKillProb.shear + m_defenseSKillProb.cloth - 1 >= r)
                    {
                        m_defenseNextSkill = WarReport::HeroSoloSkill::CLOTH;   
                    }
                }
            }



            LOG_DEBUG("==============CheckSpell Begin==================");
            LOG_DEBUG("m_attackSkillProb, stone, shear, cloth:  %d, %d, %d", m_attackSkillProb.stone, m_attackSkillProb.shear, m_attackSkillProb.cloth);
            LOG_DEBUG("m_defenseSKillProb, stone, shear, cloth:  %d, %d, %d", m_defenseSKillProb.stone, m_defenseSKillProb.shear, m_defenseSKillProb.cloth);
            LOG_DEBUG("m_attackRage: %d", m_attackRage);
            LOG_DEBUG("m_defenseRage: %d", m_defenseRage);
            LOG_DEBUG("m_attackHeroId: %d", m_attackHero.tplId());
            LOG_DEBUG("m_defenseHeroId: %d", m_defenseHero.tplId());
            LOG_DEBUG("m_attackNextSkill: %d", m_attackNextSkill);
            LOG_DEBUG("m_defenseNextSkill: %d", m_defenseNextSkill);
            LOG_DEBUG("m_attackHero.hp: %d", m_attackHero.hp());
            LOG_DEBUG("m_defenseHero.hp: %d", m_defenseHero.hp());
            LOG_DEBUG("==============CheckSpell End==================");
        }

        // 只能判断技能不相同的情况
        WarReport::SoloResult SingleCombat::JudgeWin()
        {
            // 有必杀技的一方肯定会赢  逃跑的一方肯定要输
            if (m_attackNextSkill == WarReport::HeroSoloSkill::SOLO_NIRVARNA || m_defenseNextSkill == WarReport::HeroSoloSkill::ESCAPE)
            {
                return WarReport::SoloResult::ATTACKER;
            }
            if (m_defenseNextSkill == WarReport::HeroSoloSkill::SOLO_NIRVARNA || m_attackNextSkill == WarReport::HeroSoloSkill::ESCAPE)
            {
                return WarReport::SoloResult::DEFNESE;
            }
            // 普通技能就按照石头剪刀布来判断输赢
            if (m_attackNextSkill == WarReport::HeroSoloSkill::STONE)
            {
                if (m_defenseNextSkill == WarReport::HeroSoloSkill::SHEAR)
                {
                    return WarReport::SoloResult::ATTACKER;
                }
                else if (m_defenseNextSkill == WarReport::HeroSoloSkill::CLOTH)
                {
                    return WarReport::SoloResult::DEFNESE;
                }
            }
            else if (m_attackNextSkill == WarReport::HeroSoloSkill::SHEAR)
            {
                if (m_defenseNextSkill == WarReport::HeroSoloSkill::STONE)
                {
                    return WarReport::SoloResult::DEFNESE;
                }
                else if (m_defenseNextSkill == WarReport::HeroSoloSkill::CLOTH)
                {
                    return WarReport::SoloResult::ATTACKER;
                }
            }
            else if (m_attackNextSkill == WarReport::HeroSoloSkill::CLOTH)
            {
                if (m_defenseNextSkill == WarReport::HeroSoloSkill::STONE)
                {
                    return WarReport::SoloResult::ATTACKER;
                }
                else if (m_defenseNextSkill == WarReport::HeroSoloSkill::SHEAR)
                {
                    return WarReport::SoloResult::DEFNESE;
                }
            }
            else
            {
                LOG_DEBUG("JudgeWin error: %d, %d", (int)m_attackNextSkill, (int)m_defenseNextSkill);
            }
        }

        void SingleCombat::Begin(I_Hero& attackHero, I_Hero& defenceHero)
        {
            m_attackHero.Init(attackHero, engine::TeamType::ATTACKER);
            m_defenseHero.Init(defenceHero, engine::TeamType::DEFENDER);
            
            // 初始化武将配置表
            m_attackHeroTpl = model::tpl::g_tploader->FindNHero(m_attackHero.tplId());
            m_defenseHeroTpl = model::tpl::g_tploader->FindNHero(m_defenseHero.tplId());
            // 初始化武将性格
            m_attackSkillProb = g_tploader->configure().characterInfluence.heroSoloSkillProbs.at(m_attackHeroTpl->trait - 1);
            m_defenseSKillProb = g_tploader->configure().characterInfluence.heroSoloSkillProbs.at(m_defenseHeroTpl->trait - 1);
            // 初始化怒气值
            m_attackRage = g_tploader->configure().soloRage.init;                     
            m_defenseRage = g_tploader->configure().soloRage.init;
            // 初始化体力上限
            m_attackMaxPhysical = g_tploader->configure().heroPhysical.initial + m_attackHero.level() * g_tploader->configure().heroPhysical.increase;
            m_defenseMaxPhysical = g_tploader->configure().heroPhysical.initial + m_defenseHero.level() * g_tploader->configure().heroPhysical.increase; 
            // 初始化双方体力
            m_attackMaxHp = m_attackHero.hp();
            m_defenseMaxHp = m_defenseHero.hp();
          /*  m_attackHero.InitDefense();
            m_defenseHero.InitDefense();*/


            m_attackSoloReport.attackType = engine::TeamType::ATTACKER;
            m_attackSoloReport.Id = m_attackHero.tplId();
            m_attackSoloReport.hp = m_attackMaxHp;
            m_attackSoloReport.rage = m_attackRage;
            m_attackSoloReport.power = m_attackHero.physical_attack();

            m_defenseSoloReport.attackType = engine::TeamType::DEFENDER;
            m_defenseSoloReport.Id = m_defenseHero.tplId();
            m_defenseSoloReport.hp = m_defenseMaxHp;
            m_defenseSoloReport.rage = m_defenseRage;
            m_defenseSoloReport.power = m_defenseHero.physical_attack();

            m_round = 0;
            m_isEnd = false;                      
            Ready();
        }

        void SingleCombat::RoundBegin()
        {
            // 必杀消耗怒气
            if (m_attackNextSkill == WarReport::HeroSoloSkill::SOLO_NIRVARNA)
            {
                m_attackRage = m_attackRage - g_tploader->configure().soloRage.consume;
            }
            if (m_defenseNextSkill == WarReport::HeroSoloSkill::SOLO_NIRVARNA)
            {
                m_defenseRage = m_defenseRage - g_tploader->configure().soloRage.consume;   
            }

            // 如果双方都逃跑则算逃跑成功
            if (m_attackNextSkill == WarReport::HeroSoloSkill::ESCAPE && m_defenseNextSkill == WarReport::HeroSoloSkill::ESCAPE)
            {
                m_attackNextSkill = WarReport::HeroSoloSkill::ESCAPE;
                m_defenseNextSkill = WarReport::HeroSoloSkill::ESCAPE;

                m_isEnd = true;
                m_isWin = WarReport::SoloResult::DEUCE;
                return;
            }
            // 判断一方是否逃跑成功
            if (m_attackNextSkill == WarReport::HeroSoloSkill::ESCAPE)
            {
                // 逃跑概率后期还要加上宝物的加成  Todo...
                int r = framework.random().GenRandomNum(10000);
                if (g_tploader->configure().escapeSuc - 1 >= r)
                {
                    m_isEnd = true;
                    m_isWin = WarReport::SoloResult::DEFNESE;
                    // 攻击方逃跑成功
                    m_defenseNextSkill = WarReport::HeroSoloSkill::NONE;
                    return;
                }
            }
            if (m_defenseNextSkill == WarReport::HeroSoloSkill::ESCAPE)
            {
                // 逃跑概率后期还要加上宝物的加成  Todo...
                int r = framework.random().GenRandomNum(10000);
                if (g_tploader->configure().escapeSuc - 1 >= r)
                {
                    m_isEnd = true;
                    m_isWin = WarReport::SoloResult::ATTACKER;

                    // 攻击方胜利
                    m_attackNextSkill = WarReport::HeroSoloSkill::NONE;
                    return;
                }
            }
        }

        void SingleCombat::RoundCombat()
        {
            if (IsEnd())
            {
                return;
            }

            m_attackOutput.Reset();
            m_defenseOutput.Reset();

            m_attackOutput.round = m_round;
            m_defenseOutput.round = m_round;
            if (m_attackNextSkill == m_defenseNextSkill)
            {
                m_isWin = WarReport::SoloResult::DEUCE;
            }
            // 
            else if (m_attackNextSkill != WarReport::HeroSoloSkill::NONE && m_defenseNextSkill != WarReport::HeroSoloSkill::NONE)
            {   
                m_isWin = JudgeWin();
            }
            //计算战斗结果
            do {
                bool attack_nirvarna_flag = false;
                bool defense_nirvarna_flag = false;

                if (m_isWin == WarReport::SoloResult::ATTACKER)
                {
                    if (m_attackNextSkill == WarReport::HeroSoloSkill::SOLO_NIRVARNA)
                    {
                        attack_nirvarna_flag = true;   
                    }
                    Attack(m_attackHero, m_defenseHero, attack_nirvarna_flag);
                    // 特殊技不加怒气
                    if (m_attackNextSkill != WarReport::HeroSoloSkill::SOLO_NIRVARNA)
                    {
                        m_attackRage = (m_attackRage + g_tploader->configure().soloRage.success) > g_tploader->configure().soloRage.max ? g_tploader->configure().soloRage.max : (m_attackRage + g_tploader->configure().soloRage.success);
                    }
                    m_defenseRage = (m_defenseRage + g_tploader->configure().soloRage.fail) > g_tploader->configure().soloRage.max ? g_tploader->configure().soloRage.max : (m_defenseRage + g_tploader->configure().soloRage.fail);
                }
                else if (m_isWin == WarReport::SoloResult::DEFNESE)
                {
                    if (m_defenseNextSkill == WarReport::HeroSoloSkill::SOLO_NIRVARNA)
                    {
                        defense_nirvarna_flag = true;   
                    }
                    Attack(m_defenseHero, m_attackHero, defense_nirvarna_flag);
                    // 特殊技不加怒气
                    if (m_defenseNextSkill != WarReport::HeroSoloSkill::SOLO_NIRVARNA)
                    {
                        m_defenseRage = (m_defenseRage + g_tploader->configure().soloRage.success) > g_tploader->configure().soloRage.max ? g_tploader->configure().soloRage.max : (m_defenseRage + g_tploader->configure().soloRage.success);
                    }
                    m_attackRage = (m_attackRage + g_tploader->configure().soloRage.fail) > g_tploader->configure().soloRage.max ? g_tploader->configure().soloRage.max : (m_attackRage + g_tploader->configure().soloRage.fail);
                }
                else if (m_isWin == WarReport::SoloResult::DEUCE)
                {
                    // 特殊技不加怒气
                    if (m_attackNextSkill != WarReport::HeroSoloSkill::SOLO_NIRVARNA)
                    {
                        m_attackRage = (m_attackRage + g_tploader->configure().soloRage.flat) > g_tploader->configure().soloRage.max ? g_tploader->configure().soloRage.max : (m_attackRage + g_tploader->configure().soloRage.flat);
                    }
                    // 特殊技不加怒气
                    if (m_defenseNextSkill != WarReport::HeroSoloSkill::SOLO_NIRVARNA)
                    {
                        m_defenseRage = (m_defenseRage + g_tploader->configure().soloRage.flat) > g_tploader->configure().soloRage.max ? g_tploader->configure().soloRage.max : (m_defenseRage + g_tploader->configure().soloRage.flat);
                    }
                }
            } while (0);
        }

        void  SingleCombat::RoundEnd()
        {   
           /* m_attackOutput.hurtHp = m_attackInput.hp - m_attackHero.hp();
            m_attackOutput.addRage = m_attackRage - m_attackHero.rage();

            m_defenseOutput.hurtHp = m_defenseInput.hp - m_defenseHero.hp();
            m_defenseOutput.addRage = m_defenseRage - m_defenseHero.rage();*/
            if (m_isWin == WarReport::SoloResult::NONE)
            {
                LOG_DEBUG("RoundEnd error: %d,  %d,  %d", (int)m_isWin, (int)m_attackNextSkill, (int)m_defenseNextSkill);                
            }
            // 如果同时逃跑
            if (m_attackNextSkill == WarReport::HeroSoloSkill::ESCAPE && m_defenseNextSkill == WarReport::HeroSoloSkill::ESCAPE)
            {
                WarReport::SoloRound soloRound1;
                soloRound1.action = WarReport::HeroSoloSkill::ESCAPE;
                soloRound1.rageChange = 0;
                soloRound1.hpChange = 0;
                soloRound1.result = m_isWin;

                WarReport::SoloRound soloRound2;
                soloRound2.action = WarReport::HeroSoloSkill::ESCAPE;
                soloRound2.rageChange = 0;
                soloRound2.hpChange = 0;
                soloRound2.result = m_isWin;

                m_attackSoloReport.rounds.push_back(soloRound1);
                m_defenseSoloReport.rounds.push_back(soloRound2);
            }
            // 防守方逃跑成功
            else if (m_attackNextSkill == WarReport::HeroSoloSkill::NONE)
            {
                WarReport::SoloRound soloRound1;
                soloRound1.action = WarReport::HeroSoloSkill::NONE;
                soloRound1.rageChange = 0;
                soloRound1.hpChange = 0;
                soloRound1.result = m_isWin;
                // 防守方逃跑成功
                WarReport::SoloRound soloRound2;
                soloRound2.action = m_defenseNextSkill;
                soloRound2.rageChange = 0;
                soloRound2.hpChange = 0;
                soloRound2.result = m_isWin;

                m_attackSoloReport.rounds.push_back(soloRound1);
                m_defenseSoloReport.rounds.push_back(soloRound2);
            }
            // 攻击方逃跑成功
            else if (m_defenseNextSkill == WarReport::HeroSoloSkill::NONE)
            {
                // 攻击方逃跑成功
                WarReport::SoloRound soloRound1;
                soloRound1.action = m_attackNextSkill;
                soloRound1.rageChange = 0;
                soloRound1.hpChange = 0;
                soloRound1.result = m_isWin;

                // 防守方胜利
                WarReport::SoloRound soloRound2;
                soloRound2.action = WarReport::HeroSoloSkill::NONE;
                soloRound2.rageChange = 0;
                soloRound2.hpChange = 0;
                soloRound2.result = m_isWin;

                m_attackSoloReport.rounds.push_back(soloRound1);
                m_defenseSoloReport.rounds.push_back(soloRound2);
            }
            // 非逃跑判定
            else
            {
                // 回合结算
                // 攻击方胜利
                WarReport::SoloRound soloRound1;
                soloRound1.action = m_attackNextSkill;
                soloRound1.hpChange = m_attackHero.hp() - m_attackInput.hp;
                soloRound1.rageChange = m_attackRage - m_attackInput.rage;
                soloRound1.result = m_isWin;
                // 防守方逃跑成功
                WarReport::SoloRound soloRound2;
                soloRound2.action = m_defenseNextSkill;
                soloRound2.hpChange = m_defenseHero.hp() - m_defenseInput.hp;
                soloRound2.rageChange = m_defenseRage - m_defenseInput.rage;
                soloRound2.result = m_isWin;
    
                m_attackSoloReport.rounds.push_back(soloRound1);
                m_defenseSoloReport.rounds.push_back(soloRound2);
            }

            if (m_attackHero.hp() <= 0 || m_defenseHero.hp() <= 0/* || m_round >= 5*/) {
                m_isEnd = true;
            }
        }

        void SingleCombat::End()
        {
            if (!m_isEnd) {
                return;
            }

            m_result.attackHeroId = m_attackHero.tplId();
            m_result.attackHeroHp = m_attackHero.hp();
            m_result.defenseHeroId = m_defenseHero.tplId();
            m_result.defenseHeroHp = m_defenseHero.hp();
            int defenseHeroHp = m_defenseHero.hp() > 0  ?  m_defenseHero.hp() : 0;
            int attackHeroHp = m_attackHero.hp() > 0  ?  m_attackHero.hp() : 0;
            m_result.attackOutputHurt = m_defenseMaxHp - defenseHeroHp;
            m_result.defenseOutputHurt = m_attackMaxHp - attackHeroHp;

            m_attackSoloReport.result = m_isWin;
            m_defenseSoloReport.result = m_isWin;
            if (m_isWin == WarReport::SoloResult::DEUCE)
            {
                m_result.isAttackWin = false;
                m_result.attackLoseArmyPercent = model::tpl::g_tploader->configure().soloLose.runaway;
                m_result.defenseLostArmyPercent = model::tpl::g_tploader->configure().soloLose.runaway;
            }
            else if (m_isWin == WarReport::SoloResult::ATTACKER)
            {
                m_result.isAttackWin = true;
                m_result.defenseLostArmyPercent = model::tpl::g_tploader->configure().soloLose.death;
            }
            else if (m_isWin == WarReport::SoloResult::DEFNESE)
            {
                m_result.isAttackWin = false;
                m_result.attackLoseArmyPercent = model::tpl::g_tploader->configure().soloLose.death;
            }
            m_attackSoloReport.loseArmysPercent = m_result.attackLoseArmyPercent;
            m_defenseSoloReport.loseArmysPercent = m_result.defenseLostArmyPercent;

        }

        int SingleCombat::GetHurt(Hero& source, Hero& target, bool nirvarna_flag)
        {

            //单挑伤害计算
            int hurt = 0; 
            float real_hurt = 0;
            int floatvalue = m_randEngine.RandBetween(1, 11) - 6; // -5 ~ 5  
            real_hurt = (std::pow(source.physical_attack(), 2) / (source.physical_attack() + target.defense()))
                * (std::pow(1.01, floatvalue + source.assistSum() - target.assistSum())) * (nirvarna_flag ? 2 : 1);
            real_hurt = real_hurt > 0.0 ? real_hurt : 0.0;
            hurt = int(real_hurt);

            /*LOG_DEBUG("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");
            LOG_DEBUG("attackerId : %d", source.tplId());
            LOG_DEBUG("nirvarna_flag : %d", (int)nirvarna_flag);
            LOG_DEBUG("floatvalue : %d", floatvalue);
            LOG_DEBUG("attack.attack : %f", source.physical_attack());
            LOG_DEBUG("target.defense : %f", target.defense());
            LOG_DEBUG("source.assistSum : %d", source.assistSum());
            LOG_DEBUG("target.assistSum : %d", target.assistSum());
            LOG_DEBUG("real_hurt : %f", real_hurt);
            LOG_DEBUG("hurt : %d", hurt);
            LOG_DEBUG("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");*/

            return hurt;
        }

        void SingleCombat::Attack(Hero& source, Hero& target, bool nirvarna_flag)
        {
            /*if (source.IsDie() || target.IsDie()) {
                return;
            }*/

            int hurt = GetHurt(source, target, nirvarna_flag);  

            if (target.hp() > 0)
            {
                target.AddHp(-hurt);
            }
        }
    }
}

