#include "hero.h"
#include "team.h"
#include "singlecombat.h"

#include <model/tpl/templateloader.h>
#include <base/logger.h>

namespace engine
{
    namespace battle
    {
        using namespace std;
        using namespace model;
        using namespace model::tpl;

        void Hero::InitMix(I_Group& iGroup, Team* team) {
            m_iGroup = iGroup;

            m_tplid = iGroup.hero.id;
            m_level = iGroup.hero.level;
            m_star = iGroup.hero.star; 
            m_singel_energy = iGroup.hero.physical;
            m_army_count = iGroup.army.armyCount;  //目前只读了I_Group.army的这个属性
            m_army_type = static_cast<ArmyType>(iGroup.army.armyType);
            m_army_level = iGroup.army.armyLevel;
            m_activeCount = m_iGroup.army.armyCount; 
            m_initFightCount = m_iGroup.army.armyCount;

            LOG_DEBUG("Hero ------------ m_tplid: %d  level: %d, star : %d  armytype: %d, armylevel: %d position: %d ", 
                m_tplid, m_level, m_star, iGroup.army.armyType, m_army_level, m_iGroup.position);

            m_heroTpl = model::tpl::g_tploader->FindNHero(m_tplid);
            m_armyTpl = model::tpl::g_tploader->FindNArmy((int)m_army_type, m_army_level);
            m_nirvanaTpl = model::tpl::g_tploader->FindSkill(m_heroTpl->nirvanaSkill);
            m_complexTpl = model::tpl::g_tploader->FindSkill(m_heroTpl->cooperativeSkill);

            for (auto spell : iGroup.hero.spells) {
                if (m_heroTpl->nirvanaSkill == spell.id) {
                    m_nirvanaLevel = spell.level;
                    break;
                }
            }

            //一级属性
            m_power = iGroup.hero.heroPower;
            m_defense = iGroup.hero.heroDefense;
            m_wisdom = iGroup.hero.heroWisdom;
            m_lucky = iGroup.hero.heroLucky;
            m_skill = iGroup.hero.heroSkill;
            m_agile = iGroup.hero.heroAgile;
            m_life = iGroup.hero.heroLife;

            //二级属性
            m_physical_attack = iGroup.hero.heroPhysicalPower;
            m_physical_defense = iGroup.hero.heroPhysicalDefense;
            m_wisdom_attack = iGroup.hero.heroSkillPower;
            m_wisdom_defense =iGroup.hero.heroSkillDefense;
            m_hit = iGroup.hero.heroHit;
            m_avoid = iGroup.hero.heroAvoid;
            m_crit_hit = iGroup.hero.heroCritHit;
            m_crit_avoid = iGroup.hero.heroCritAvoid;
            m_speed = iGroup.hero.heroSpeed;
            m_city_life = iGroup.hero.heroCityLife;
            m_fight = iGroup.hero.heroTroops;

            m_armySpeed = m_speed;
            
            m_team = team;
            m_teamType = m_team->teamType();
            m_damage = m_heroTpl->damageType;
        }

        bool Hero::Init(I_Hero& iHero, engine::TeamType teamType) {
            m_tplid = iHero.id;
            m_level = iHero.level;
            m_star = iHero.star;
            m_singel_energy = iHero.physical; 
            
            LOG_DEBUG("Hero ------------ m_tplid: %d  level: %d, star : %d  ", m_tplid, m_level, m_star);

            m_heroTpl = model::tpl::g_tploader->FindNHero(m_tplid);
            m_armyTpl = model::tpl::g_tploader->FindNArmy(m_heroTpl->job);
            m_nirvanaTpl = model::tpl::g_tploader->FindSkill(m_heroTpl->nirvanaSkill);

            //一级属性
            m_power = iHero.heroPower;
            m_defense = iHero.heroDefense;
            m_wisdom = iHero.heroWisdom;
            m_lucky = iHero.heroLucky;
            m_skill = iHero.heroSkill;
            m_agile = iHero.heroAgile;
            m_life = iHero.heroLife;

            //二级属性
            m_physical_attack = iHero.heroPhysicalPower;
            m_physical_defense = iHero.heroPhysicalDefense;
            m_wisdom_attack = iHero.heroSkillPower;
            m_wisdom_defense = iHero.heroSkillDefense;
            m_hit = iHero.heroHit;
            m_avoid = iHero.heroAvoid;
            m_crit_hit = iHero.heroCritHit;
            m_crit_avoid = iHero.heroCritAvoid;
            m_speed = iHero.heroSpeed;
            m_city_life = iHero.heroCityLife;
            m_fight = iHero.heroSolohp;

            m_armySpeed = m_speed;

            m_teamType = teamType;
            m_damage = m_heroTpl->damageType;
            //Init();
        }

        float Hero::GetAddtionValue(int type) {
            float value = 0;
            if (m_innateSkills1Tpl->passiveType1 == type)
            {
                value = m_innateSkills1Tpl->passiveParameter1;
            }

            if (m_innateSkills1Tpl->passiveType2 == type)
            {
                value = m_innateSkills1Tpl->passiveParameter2;
            }

            if (m_innateSkills2Tpl->passiveType1 == type)
            {
                value = m_innateSkills2Tpl->passiveParameter1;
            }
            
            if (m_innateSkills2Tpl->passiveType2 == type)
            {
                value = m_innateSkills2Tpl->passiveParameter2;
            }

            if (m_innateSkills3Tpl->passiveType1 == type)
            {
                value = m_innateSkills3Tpl->passiveParameter1;
            }

            if (m_innateSkills3Tpl->passiveType2 == type)
            {
                value = m_innateSkills3Tpl->passiveParameter2;
            }

            return value;
        }

        int Hero::assistSum() {
            int sum = 0;
            if (m_heroTpl->supportPlus1 != 0) {
                sum += m_heroTpl->supportValues1;
            }
            if (m_heroTpl->supportPlus2 != 0) {
                sum += m_heroTpl->supportValues2;
            }
            if (m_heroTpl->supportPlus3 != 0) {
                sum += m_heroTpl->supportValues3;
            }
            if (m_heroTpl->supportPlus4 != 0) {
                sum += m_heroTpl->supportValues4;
            }
            if (m_heroTpl->supportPlus5 != 0) {
                sum += m_heroTpl->supportValues5;
            }
            return sum;
        }

        void Hero::CastBuff(int spellLevel,  const engine::tpl::T_Buff& tBuff, Hero& target, int prama1, 
            bool isPassive, int& retVal) {
            target.buffList().CreateBuff(spellLevel,  tBuff, *this, prama1, isPassive, retVal);
        }

        void Hero::UpdateBuff(TriggerBuffState state) {
            LOG_DEBUG("Update --- buff position ----%d ", Position());
            buffList().Update(state);
        }

        void Hero::Debug() {
            cout << "Hero::Debug" << endl;
            cout << "=====================================================" << endl;
            cout << " tplid : " << m_tplid << endl;
            cout << " power : " << m_power << " physical_attack: " << m_physical_attack  << endl; 
            cout << " defense : " << m_defense << " physical_defense: " << m_physical_defense  << endl;
            cout << " wisdom : " << m_wisdom << " wisdom_attack: " << m_wisdom_attack << " wisdom_defense: " << m_wisdom_defense << endl;
            cout << " skill : " << m_skill << " hit: " << m_hit << " crit_hit: " << m_crit_hit << endl;
            cout << " agine : " << m_agile << " avoid: " << m_avoid << " crit_avoid: " << m_crit_avoid << endl;
            cout << " lucky : " << m_lucky << " speed: " << m_speed << endl;
            cout << " life : " << m_life << " city_life: " << m_city_life << endl;
            cout << " fight : " << m_fight << " single_energy: " << m_singel_energy << endl;
            cout << "=====================================================" << endl;
        }

        void Hero::OnBuffDirectDamage(Hero& caster, int hurt)
        {
            if (caster.armyHp() <= 0) {
                return;
            }

            int realHurt = AddArmyHp(-hurt);
            caster.OnMakeHurt(realHurt, armyHpBase());
        }

        void Hero::OnMakeHurt(int hurt, int baseHp)
        {
            if (hurt < 0) hurt = -hurt;
            int count = hurt / baseHp;
            AddKillCount(count);
        }

        int Hero::AddArmyHp(int32_t num)
        {
            int oldCount = m_activeCount;
            int32_t realNum = num;

            m_activeCount += num;
             
            if (m_activeCount > m_initFightCount)  {    
                m_activeCount = m_initFightCount;
                realNum = m_initFightCount - oldCount;
            }

            if (m_activeCount < 0) {
                realNum = -oldCount;
                m_activeCount = 0;
                //OnArmyDie();
            }

            if (realNum < 0) { //加血的时候，不用计算减少死亡人数 
                m_dieCount -= realNum;
            }

            return realNum;          
        }

        float Hero::GetArmyAdapt() {
            float value = 0;
            if (m_army_type == ArmyType::INFANTRY) {
                value = m_heroTpl->pikemanSkill;
            } else if (m_army_type == ArmyType::RIDER) {
                value = m_heroTpl->cavalrySkill;
            } else if (m_army_type == ArmyType::ARCHER) {
                value = m_heroTpl->archerSkill;
            } else if (m_army_type == ArmyType::MACHINE) {
                value = m_heroTpl->mechanicsSkill;
            } 
            return value;
        }

        void Hero::Reset()
        {
            m_rage = 0;                    // 怒气值

            m_teamType = TeamType::NONE;   // 队伍类型
            m_hpMax = 0;                   // 生命值
            m_speed = 0;

            m_buffList.Clear();
        }
    }
}





