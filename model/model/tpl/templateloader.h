#ifndef MODEL_TPL_TEMPLATELOADER_H
#define MODEL_TPL_TEMPLATELOADER_H

#include "../metadata.h"
#include <base/observer.h>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include <tuple>

namespace model
{
    namespace tpl
    {

        class ModuleTemplateLoader;
        struct Localization;
        struct ItemTpl;
        struct DropTpl;
        struct ArmyTpl;
        struct NArmyTpl;
        class  Configure;
        struct MiscConf;
        struct SkillBaseTpl;
        struct SkillNodeTpl;
        struct BuffTpl;
        struct HeroTpl;
        struct NHeroTpl;
        struct SkillTpl;
        struct AttackDiscountTpl;
        struct ArmySpellTpl;
        struct AllianceLevelTpl;
        struct AttackRangeTpl;
        struct BattleArrtTransformTpl;
        struct HeroSoulTpl;
        struct HeroStarLevelTpl;
        struct HeroLevelAttrTpl;
        struct BattleArrtTpl;
        struct BattleInjuredSoldiersTpl;

        typedef std::unordered_map<std::string, Localization*> localizations_map_t;
        typedef std::unordered_map<int, ItemTpl*> items_map_t;
        typedef std::unordered_map<int, DropTpl*> drops_map_t;
        typedef std::unordered_map<int, ArmyTpl*> armys_map_t;
        typedef std::unordered_map<int, NArmyTpl*> narmys_map_t;
        typedef std::map<std::tuple<int, int>, ArmyTpl*> armys_type_level_map_t;
        typedef std::map<std::tuple<int, int>, NArmyTpl*> narmys_type_level_map_t;
        typedef std::unordered_map<int, SkillBaseTpl*> skillbase_map_t;
        typedef std::unordered_map<int, SkillNodeTpl*> skillnode_map_t;
        typedef std::unordered_map<int, BuffTpl*> buff_map_t;
        typedef std::unordered_map<int, HeroTpl*> hero_map_t;
        typedef std::unordered_map<int, NHeroTpl*> nhero_map_t;
        typedef std::unordered_map<int, std::vector<int>> hero_own_fetter_map_t;
        typedef std::unordered_map<int, AttackDiscountTpl*> attack_discount_map_t;
        typedef std::unordered_map<int, ArmySpellTpl*> army_spell_map_t;
        typedef std::unordered_map<int, AllianceLevelTpl*> alliance_level_map_t;
        typedef std::unordered_map<int, SkillTpl*> skill_map_t;
        typedef std::unordered_map<int, AttackRangeTpl*> attackrange_map_t;
        typedef std::unordered_map<int, BattleArrtTransformTpl*> battle_arrt_transform_map_t;
        typedef std::unordered_map<int, HeroSoulTpl*> hero_soul_map_t;
        typedef std::unordered_map<int, std::vector<HeroStarLevelTpl*>> hero_star_level_map_t;
        typedef std::unordered_map<int, std::vector<HeroLevelAttrTpl*>> hero_level_attr_map_t;
        typedef std::unordered_map<int, BattleArrtTpl*> battle_Arrt_map_t;
        typedef std::unordered_map<int, BattleInjuredSoldiersTpl*> battle_injured_soldiers_map_t;

        class TemplateLoader
        {
        public:
            TemplateLoader();
            ~TemplateLoader();

            const localizations_map_t& localizations() const {
                return m_localizations;
            }

            const items_map_t& items() const {
                return m_items;
            }

            const drops_map_t& drops() const {
                return m_drops;
            }

            const skillbase_map_t& skillbase() const {
                return m_skillbases;
            }

            const skillnode_map_t& skillnode() const {
                return m_skillnodes;
            }

            const buff_map_t& buff() const {
                return m_buffs;
            }

            const hero_map_t& heroes() const {
                return m_heroes;
            }

            const armys_map_t& armies() const {
                return m_armies;
            }

            const attack_discount_map_t& attackDiscount() const {
                return m_attack_discount;
            }

            const army_spell_map_t& armySpell() const {
                return m_army_spell;
            }

            const ItemTpl* FindItem(int id) const {
                auto const it = m_items.find(id);
                return it == m_items.end() ? nullptr : it->second;
            }

            const DropTpl* FindDrop(int id) const {
                auto const it = m_drops.find(id);
                return it == m_drops.end() ? nullptr : it->second;
            }

            const HeroTpl* FindHero(int id) const {
                auto const it = m_heroes.find(id);
                return it == m_heroes.end() ? nullptr : it->second;
            }

            const NHeroTpl* FindNHero(int id) const {
                auto const it = m_nheroes.find(id);
                return it == m_nheroes.end() ? nullptr : it->second;
            }

            const ArmyTpl* FindArmy(int id) const {
                auto const it = m_armies.find(id);
                return it == m_armies.end() ? nullptr : it->second;
            }

            const NArmyTpl* FindNArmy(int id) const {
                auto const it = m_narmies.find(id);
                return it == m_narmies.end() ? nullptr : it->second;
            }

            const ArmyTpl* FindArmy(int type, int level) const {
                auto const it = m_armys_type_level_map.find(std::make_tuple(type, level));
                return it == m_armys_type_level_map.end() ? nullptr : it->second;
            }

            const NArmyTpl* FindNArmy(int type, int level) const {
                auto const it = m_narmys_type_level_map.find(std::make_tuple(type, level));
                return it == m_narmys_type_level_map.end() ? nullptr : it->second;
            }

            const SkillTpl* FindSkill(int id) const {
                auto const it = m_skills.find(id);
                return it == m_skills.end() ? nullptr : it->second;
            }

            const AttackRangeTpl* FindAttackRange(int id) const {
                auto const it = m_attackranges.find(id);
                return it == m_attackranges.end() ? nullptr : it->second;
            }

            const AllianceLevelTpl* FindAllianceLevel(int level) const {
                auto const it = m_alliance_level_map.find(level);
                return it == m_alliance_level_map.end() ? nullptr : it->second;
            }

            const BattleArrtTransformTpl* FindBattleArrtTransform (int type) const {
                auto const it = m_battleArrtTransform.find(type);
                return it == m_battleArrtTransform.end() ? nullptr : it->second;
            }

            const hero_soul_map_t GetHeroSoul() const {
                return m_heroSoul;
            }

            const std::vector<HeroStarLevelTpl*>* FindHeroStarLevel(int heroId) const {
                auto const it = m_heroStarLevel.find(heroId);
                return it == m_heroStarLevel.end() ? nullptr : &(it->second);
            }

            const std::vector<HeroLevelAttrTpl*>* FindHeroLevelAttr(int heroId) const {
                auto const it = m_heroLevelAttr.find(heroId);
                return it == m_heroLevelAttr.end() ? nullptr : &(it->second);
            }

            const BattleArrtTpl* FindBattleArrt(AttributeType type) const {
                auto const it = m_battleArrt.find((int)type);
                return it == m_battleArrt.end() ? nullptr : it->second;
            }

            const BattleInjuredSoldiersTpl* FindBattleInjuredSoldiers(BattleType action) const {
                auto const it = m_battleInjuredSoldiers.find((int)action);
                return it == m_battleInjuredSoldiers.end() ? nullptr : it->second;
            }

            const Configure& configure() const {
                return *m_configures;
            }
            
            const MiscConf& miscConf() const {
                return *m_miscConf;
            }
            
            std::string FindLocalizations(std::string name, LangType lang) const;

            void DebugDump();

        private:
            void BeginSetup(std::function<void(bool)> cb);

        private:
            void ParseMiscConf();
            void LoadConfigure();
            void LoadLocalization();
            void LoadItem();
            void LoadDrop();
            void LoadArmy();
            void LoadNArmy(); //Todo
            void LoadAttackRange(); 
            void LoadSkillBase();
            void LoadSkillNode();
            void LoadBuff();
            void LoadHero();
            void LoadNHero();  //Todo: D
            void LoadSkill();
            void LoadAttackDiscount();
            void LoadArmySpell();
            void LoadAllianceLevel();
            void LoadBattleArrtTransform();
            void LoadHeroSoul();
            void LoadHeroLevelAttr();
            void LoadHeroStarLevel();
            void LoadBattleArrt();
            void LoadBattleInjuredSoldiers();

            void LoadEnd();

        private:
            std::function<void(bool)> m_cb_setup;
            base::AutoObserver m_auto_observer;
            friend class ModuleTemplateLoader;

            localizations_map_t m_localizations;
            items_map_t m_items;
            drops_map_t m_drops;
            armys_map_t m_armies;
            narmys_map_t m_narmies;
            armys_type_level_map_t m_armys_type_level_map;
            narmys_type_level_map_t m_narmys_type_level_map;
            Configure* m_configures = nullptr;
            MiscConf* m_miscConf = nullptr;

            skillbase_map_t m_skillbases;
            skillnode_map_t m_skillnodes;
            buff_map_t m_buffs;
            hero_map_t m_heroes;
            nhero_map_t m_nheroes;
            skill_map_t m_skills;
            attack_discount_map_t m_attack_discount;
            army_spell_map_t m_army_spell;
            alliance_level_map_t m_alliance_level_map;
            attackrange_map_t m_attackranges;
            battle_arrt_transform_map_t m_battleArrtTransform;
            hero_soul_map_t m_heroSoul;
            hero_star_level_map_t m_heroStarLevel;
            hero_level_attr_map_t m_heroLevelAttr;
            battle_Arrt_map_t m_battleArrt;
            battle_injured_soldiers_map_t m_battleInjuredSoldiers;
        };

        extern TemplateLoader* g_tploader;
    }
}
#endif // TEMPLATELOADER_H
