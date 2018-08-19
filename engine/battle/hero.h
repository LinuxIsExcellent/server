#ifndef ENGINE_BATTLE_NHERO_H
#define ENGINE_BATTLE_NHERO_H
#include <stdint.h>
#include <vector>
#include <list>
#include <unordered_map>
#include <functional>
#include "bufflist.h"
#include "../input_struct.h"
#include "../tpl/t_spell.h"
#include "../tpl/t_army.h"
#include "../tpl/t_hero.h"
#include "../metadata.h"

#include <model/tpl/hero.h>
#include <model/tpl/army.h>

namespace tpl
{
    namespace tpl
    {
        struct NHeroTpl;
        struct NArmyTpl;
        struct SkillTpl;
    }
}

namespace engine
{
    namespace battle
    {
        using namespace tpl;

        const static int kMaxRage = 100;

        class Team;
        class Hero
        {
        public:
            Hero():m_buffList(*this){}
            Hero(I_Group& iGroup, Team* team):m_buffList(*this) {
                InitMix(iGroup, team);
            }
            virtual ~Hero() {}

            void InitMix(I_Group& iGroup, Team* team);
            bool Init(I_Hero& iHero, engine::TeamType teamType);
      
            int tplId() const {
                return m_tplid;
            }

            int damage() const {
                return m_damage;
            }

            int level() const {
                return m_level;
            }

            float power() const {
                return m_power;
            }

            float defense() const {
                return m_defense;
            }

            float wisdom() const {
                return m_wisdom;
            }

            float skill() const {
                return m_skill;
            }

            float agile() const {
                return m_agile;
            }

            float lucky() const {
                return m_lucky;
            }

            float life() const {
                return m_life;
            }

            float physical_attack() {
                return m_physical_attack;
            }

            float physical_defense() {
                return m_physical_defense;
            }

            float wisdom_attack() {
                return m_wisdom_attack;
            }

            float wisdom_defense() {
                return m_wisdom_defense;
            }

            float hit() {
                return m_hit;
            }

            float avoid() {
                return m_avoid;
            }

            float crit_hit() {
                return m_crit_hit;
            }

            float crit_avoid() {
                return m_crit_avoid;
            }

            float speed() {
                return m_speed;
            }

            float life() {
                return m_life;
            }

            float fight() {
                return m_fight;
            }

            float single_energy() {
                return m_singel_energy;
            }

            //--必杀需要的怒气值
            int needRage() {
                return m_nirvanaTpl != nullptr ?  m_nirvanaTpl->angerConsumption : 0;
            }

            int needWar() {
                return m_complexTpl != nullptr ? m_complexTpl->warConsumption : 0;
            }

            int war() {
                return m_war;
            }

            void AddWar(int value) {
                if (HasStatus(TargetStatus::WEAK) && value > 0) {
                    return;
                }
                m_war += value;
            }

            int32_t AddRage(int32_t num) {
                if (HasStatus(TargetStatus::WEAK) && num > 0) {
                    return 0;
                }

                int oldRage = m_rage;
                int32_t realChange = num;

                m_rage += num;
                if (m_rage > kMaxRage) {
                    m_rage = kMaxRage;
                    realChange = oldRage - kMaxRage;
                }

                if (m_rage < 0) {
                    m_rage = 0;
                    realChange = -oldRage;
                }

                return realChange;
            }

            // --- 新增属性
            int IsDie() const {
                return m_activeCount <= 0;
            }

            float GetAddtionValue(int type);

            int assistSum();

            //目标状态判定
            bool IsStatus(TargetStatus type)  {
                auto it = m_effect_map.find((int)type);
                return it != m_effect_map.end()  ? it->second : false;
            }

            bool HasStatus(TargetStatus type)  {
                auto it = m_effect_map.find((int)type);
                return it != m_effect_map.end()  ? it->second : false;
            }

            void SetEffectStatus(TargetStatus type)  {
                auto it = m_effect_map.find((int)type);
                if (it == m_effect_map.end()) {
                    m_effect_map.emplace((int)type, true);
                } else {
                    it->second = true;
                }
            }

            void ClearEffectStatus(TargetStatus state) {
                auto it = m_effect_map.find((int)state);
                if (it != m_effect_map.end()) {
                    it->second = false;
                    m_effect_map.erase(it);
                }
            }

            void CleanNegativeEffectStatus() {
                for (auto it = m_effect_map.begin(); it != m_effect_map.end();) {
                    if (it->first >= (int)TargetStatus::SCARE  && it->first < (int)TargetStatus::ADD_PHYSICAL_DAMAGE) {
                        it = m_effect_map.erase(it++);
                    }
                    else
                    {
                        it++;
                    }
                }
            }

            void CleanPositiveEffectStatus() {
                for (auto it = m_effect_map.begin(); it != m_effect_map.end();) {
                    if (it->first >= (int)TargetStatus::ADD_PHYSICAL_DAMAGE) {
                        it = m_effect_map.erase(it++);
                    }
                    else
                    {
                        it++;
                    }
                }
            }

            void OutPutEffects(std::vector<int> effects) {
                for (auto e : m_effect_map) {
                    if (e.second) {
                        effects.push_back(e.first);
                    }
                }
            }

            void SetDieCb(std::function<void(Hero*)> cb)
            {
                m_dieCb = cb;
            }

            int attackTo() const {
                return m_attackTo;
            }

            void SetAttackTo(int toPos) {
                m_attackTo = toPos;
            }

            int Position() const {
                return m_iGroup.position;
            }

            void AddBeAttackFrom(int fromPos) {
                m_beAttackFrom.push_back(fromPos);
            }

            int army_count() const {
                return m_army_count;
            }

            int max_army_count() const {
                return m_fight;
            }

            const std::list<int>& beAttackFrom() {
                return m_beAttackFrom;
            }

            void RemoveBeAttackFrom(int fromPos) {
                m_beAttackFrom.remove(fromPos);
            }

            int attackRange() {
                return m_attackRange;
            }

            const tpl::T_Army* tArmy() {
                return m_tArmy;
            }

            int activeCount() {
                return m_activeCount;
            }

            int killCount() {
                return m_killCount;
            }

            int dieCount() {
                return m_dieCount;
            }

            void AddKillCount(int killCount) {
                m_killCount +=  killCount;
            }

            void CastBuff(int spellLevel,  const engine::tpl::T_Buff& tBuff, Hero& target, int prama1,
                bool isPassive, int& retVal);
            void UpdateBuff(TriggerBuffState state);

            buff::BuffList& buffList() {
                return m_buffList;
            }

            const buff::BuffList& buffList() const {
                return m_buffList;
            }

            int32_t armyHp() const {
                return m_activeCount;  //现在默认一个士兵为一点血量
            }

            int32_t armyHpBase() const  {
                return 1; //默认士兵为1点血量
            }

            int32_t armySpeedBase() const {
                return m_speed; //攻击速度
            }

            const model::tpl::SkillTpl* nirvanaTpl() const {
                return m_nirvanaTpl;
            }

            const model::tpl::SkillTpl* complexTpl() const {
                return m_complexTpl;
            }

            int nirvanaLevel() {
                return m_nirvanaLevel;
            }

            int32_t hp() const {
                return m_singel_energy;
            }

            void AddHp(int32_t num) {
                if (m_singel_energy <= 0 && num <= 0) {
                    return;
                }

                m_singel_energy += num;
                if (m_singel_energy < 0) {
                    m_singel_energy = 0;
                }
            }

            ArmyType armyType() const  {
                return m_army_type;
            }

            int armyLevel() const {
                return m_army_level;
            }

            float GetArmyAdapt();

            const model::tpl::NHeroTpl* heroTpl() {
                return m_heroTpl;
            }

            const model::tpl::NArmyTpl* armyTpl() {
                return m_armyTpl;
            }

            const I_Group& iGroup() {
                return m_iGroup;
            }

        public:
            float hitBase() const{
                return m_hit;
            }

            int32_t avoidBase() const {
                return m_avoid;
            }

            int32_t critHitBase() const {
                return m_crit_hit;
            }

            int32_t critAvoidBase() const {
                return m_crit_avoid;
            }

            void AddPower(float value) {
                m_power = m_power + value;
            }

            void AddDefense(float value) {
                m_defense = m_defense + value;
            }

            void AddWisdom(float value) {
                m_wisdom = m_wisdom + value;
            }

            void AddSkill(float value) {
                m_skill = m_skill + value;
            }

            void AddAgile(float value) {
                m_agile = m_agile + value;
            }

            void AddLucky(float value) {
                m_lucky = m_lucky + value;
            }

            void AddLife(float value) {
                m_life = m_life + value;
            }

            void AddHit(float value) {
                m_hit += value;
            }

            void AddAvoid(float value) {
                m_avoid += value;
            }

            void AddCritHit(float value) {
                m_crit_hit += value;
            }

            void AddCritAvoid(float value) {
                m_crit_avoid += value;
            }

            void AddAttackSpeed(float value) {
                m_speed += value;
            }

        public:
            void Debug();
            void Reset();

        public:
            void OnBuffDirectDamage(Hero& caster, int hurt) ;
            void OnMakeHurt(int hurt, int baseHp);
            void OnDirectDamage(Hero& caster, int hurt) {
                if (caster.hp() > 0) {
                    AddHp(-hurt);
                }
            }
            int AddArmyHp(int32_t num) ;

        public:
            int32_t hpMax() const {
                return m_hpMax;
            }

            TeamType teamType() const {
                return m_teamType;
            }

            void SetTeamType(engine::TeamType teamType) {
                m_teamType = teamType;
            }

            void AddHpMax(int32_t num) {
                m_hpMax += num;
            }

            void AddSpeed(int32_t num) {
                m_speed += num;
            }

            int32_t armySpeed() const {
                return m_armySpeed;
            }

            void AddArmySpeed(int32_t num) {
                m_armySpeed += num;
            }

            int rage() const {
                return m_rage;
            }

            void AddAll(float value) {
                AddPower(value);
                AddDefense(value);
                AddWisdom(value);
                AddSkill(value);
                AddAgile(value);
                AddLucky(value);
                AddLife(value);
            }

        public:
            int totalDieCount() {
                return (m_initFightCount - m_activeCount) > 0 ? (m_initFightCount - m_activeCount) : m_initFightCount;
            }

        protected:
            const model::tpl::NHeroTpl* m_heroTpl = nullptr;
            const model::tpl::NArmyTpl* m_armyTpl = nullptr;
            const model::tpl::SkillTpl* m_innateSkills1Tpl = nullptr;
            const model::tpl::SkillTpl* m_innateSkills2Tpl = nullptr;
            const model::tpl::SkillTpl* m_innateSkills3Tpl = nullptr;
            const model::tpl::SkillTpl* m_nirvanaTpl = nullptr;
            const model::tpl::SkillTpl* m_complexTpl = nullptr;
            int m_nirvanaLevel = 0;

            int m_tplid = 0;
            int m_level = 0;
            int m_star = 0;
            int m_damage = 0;
            float m_power = 0;
            float m_defense = 0;
            float m_wisdom = 0;
            float m_skill = 0;
            float m_agile = 0;
            float m_lucky = 0;
            float m_life = 0;

            float m_physical_attack = 0;
            float m_physical_defense = 0;
            float m_wisdom_attack = 0;
            float m_wisdom_defense = 0;
            float m_hit = 0;
            float m_avoid = 0;
            float m_crit_hit = 0;
            float m_crit_avoid = 0;
            float m_speed = 0;
            float m_city_life = 0;  //攻城值
            float m_fight = 0; //兵力
            float m_singel_energy = 0; //单挑体力

            //-------------------------------------
            I_Hero m_iHero;
            I_Group m_iGroup;
            Team* m_team;
            std::function<void(Hero*)> m_dieCb;
            int m_war; //战意值

            // --- 原group的一些属性
            //int32_t m_rage = 0;         // 怒气值
            int m_attackRange = 0;      //攻击距离
            int m_dieCount = 0;         //死亡数量
            int m_killCount = 0;        // 击杀数量

            int m_attackTo = 0;           //攻击目标
            std::list<int> m_beAttackFrom;  //被谁攻击

            double m_addAttrPercent = 0.0;                   //属性加成
            double m_buffDiscount = 1.0;                    //buff折扣(1.为不折扣)

            const tpl::T_Army* m_tArmy = nullptr;
            const tpl::T_Hero* m_tHero= nullptr;

            bool m_heroDie = false;

            //--------------------------------------
            int m_activeCount = 0; //兵数量
            int m_initFightCount = 0; //初始兵力
            //TeamType m_teamType = TeamType::NONE;   //队伍类型
            //TargetStatus m_target_type = TargetStatus::NONE;
            int m_max_army_count = 0;

            int m_army_hp_base = 1;
            int m_army_count = 0;
            ArmyType m_army_type = ArmyType::NONE;
            int m_army_level = 1;

            //--------------------------------- 效果状态
            std::unordered_map<int, bool> m_effect_map;

            int32_t m_rage = 0;                    // 怒气值

            TeamType m_teamType = TeamType::NONE;   // 队伍类型
            int32_t m_hpMax = 0;                   // 生命值

            int32_t m_armySpeed = 0;                    // 速度

            buff::BuffList m_buffList;
        };
    }
}

#endif // ENGINE_BATTLE_NHERO_H
