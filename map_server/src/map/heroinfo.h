#ifndef MAP_HEROINFO_H
#define MAP_HEROINFO_H 
#include <unordered_map>
#include <model/metadata.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include <model/tpl/hero.h>
#include <vector>
#include <memory>
#include "info/property.h"

namespace model
{
    namespace tpl
    {
        struct ArmyTpl;
        struct NHeroTpl;
        struct NArmyTpl;
        struct SkillTpl;
        struct AttackRangeTpl;
    }
}

namespace ms
{
    namespace map
    {
        using namespace model::tpl;

        const static int kHeroAttrCount = 19;
        class HeroInfo
        {
        public:
            HeroInfo() {}
            HeroInfo(int tplId, int level, int star);
            ~HeroInfo() {}

            void InipProp(int armyType = 0, int level = 0);

            int tplId() const {
                return m_tplid;
            }

            int level() const {
                return m_level;
            }

            int star() const {
                return m_star;
            }

            int soul() const {
                return m_soul;
            }

            int physical() const {
                return m_physical;
            }

            float heroPower() const {
                return m_heroPower;
            }

            float heroDefense() const {
                return m_heroDefense;
            }

            float heroWisdom() const {
                return m_heroWisdom;
            }

            float heroLucky() const {
                return m_heroLucky;
            }

            float heroSkill() const {
                return m_heroSkill;
            }

            float heroAgile() const {
                return m_heroAgile;
            }

            float heroLife() const {
                return m_heroLife;
            }

            float heroPhysicalPower() const {
                return m_heroPhysicalPower;
            }

            float heroPhysicalDefense() const {
                return m_heroPhysicalDefense;
            }

            float heroSkillPower() const {
                return m_heroSkillPower;
            }

            float heroSkillDefense() const {
                return m_heroSkillDefense;
            }

            float heroHit() const {
                return m_heroHit;
            }

            float heroAvoid() const {
                return m_heroAvoid;
            }

            float heroCritHit() const {
                return m_heroCritHit;
            }

            float heroCritAvoid() const {
                return m_heroCritAvoid;
            }

            float heroSpeed() const {
                return m_heroSpeed;
            }

            float heroCityLife() const {
                return m_heroCityLife;
            }

            int heroSolohp() const {
                return m_heroSolohp;
            }

            int heroTroops() const {
                return m_heroTroops;
            }

            const std::vector<HeroSkill>& skill() const {
                return m_skill;
            }

            int power() const {
                return m_power;
            }

            void SetProp(model::AttributeType type, float base, float pct, float ext);

            void AddProp(model::AttributeType type, float base, float pct, float ext);

            float GetProp(model::AttributeType type) const;

            void SetBaseAttr(model::AttributeType type, float value, model::AttributeAdditionType addType);

            void AddBaseAttr(model::AttributeType type, float value, model::AttributeAdditionType addType);
            
            float GetBaseAttr(model::AttributeType type, model::AttributeAdditionType addType) const;

            void SetLevel(int level) {
                m_level = level;
            }

            void SetStar(int star) {
                m_star = star;
            }

            void SetSoul(int soul) {
                m_soul = soul;
            }

            void SetPhysical(int physical) {
                m_physical = physical;
            }

            void SetSkill(const std::vector<HeroSkill>& skill) {
                m_skill = skill;
            }
            void OnPropertyUpdate(const info::Property& property, int armyType = 0, int level = 0);
            void UpdatePower();

            //序列化 JSON
            void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const;
            //反序列化 JSON
            void Desrialize(rapidjson::GenericValue<rapidjson::UTF8< char >>& gv);

        protected:
            int GetIndex(model::AttributeType type) const;

        protected:
            const model::tpl::NHeroTpl* m_heroTpl = nullptr;
            int m_tplid = 0;
            int m_level = 0;
            int m_star = 0;
            int m_physical = 0;
            int m_soul = 0;
            std::vector<HeroSkill> m_skill;
            std::unordered_map<int, float> m_firstAttrMap;

            // 武将一级属性
            float m_heroPower = 0;      //武将武力
            float m_heroDefense = 0;      //武将统帅
            float m_heroWisdom = 0;      //武将智力
            float m_heroLucky = 0;      //武将运气
            float m_heroSkill = 0;      //武将士气
            float m_heroAgile = 0;      //武将速度
            float m_heroLife = 0;      //武将攻城

            // 武将二级属性
            float m_heroPhysicalPower = 0;       //武将物理攻击力
            float m_heroPhysicalDefense = 0;       //武将物理防御力
            float m_heroSkillPower = 0;       //武将谋略攻击力
            float m_heroSkillDefense = 0;       //武将谋略防御力
            float m_heroHit = 0;       //武将命中值
            float m_heroAvoid = 0;       //武将回避值
            float m_heroCritHit = 0;       //武将暴击命中值
            float m_heroCritAvoid = 0;       //武将暴击回避值
            float m_heroSpeed = 0;       //武将攻击速度
            float m_heroCityLife = 0;       //武将攻城值
            int m_heroSolohp = 0;    //武将单挑血量上限
            int m_heroTroops = 0;    //武将领导力


            float m_attrBase[kHeroAttrCount][4] = {{0, 0, 0, 0}}; 
            int m_power = 0; 
        };
    }
}
#endif