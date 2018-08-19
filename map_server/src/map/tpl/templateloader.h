#ifndef MAP_TPL_TEMPLATELOADER_H
#define MAP_TPL_TEMPLATELOADER_H
#include <model/metadata.h>
#include <base/observer.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include "../armylist.h"

namespace ms
{
    namespace map
    {
        namespace tpl
        {

            class ModuleTemplateLoader;
            struct NpcArmyTpl;
            struct MapCityTpl;
            struct CityPatrolEventTpl;
            struct CityPatrolEvtGroupTpl;
            struct MapUnitTpl;
            struct MapTroopTpl;
//             struct ArmySkillTpl;
//             struct ArmyBattleTpl;
//             struct BattleTpl;
            struct ScoutTypeTpl;
            struct ResourceNumTpl;
            struct ResourceTpl;
            struct MonsterTpl;

            typedef std::unordered_map<int, NpcArmyTpl*> map_npcarmy_map_t;
            typedef std::unordered_map<int, std::vector<NpcArmyTpl*>> map_npcarmy_group_map_t;
            typedef std::unordered_map<int, MapCityTpl*> map_city_map_t;
            typedef std::unordered_map<int, CityPatrolEventTpl*> city_patrol_event_t;
            typedef std::unordered_map<int, CityPatrolEvtGroupTpl*> city_patrol_evt_group_t;
            
            typedef std::unordered_map<int, MapUnitTpl*> map_unit_map_t;
            typedef std::unordered_map<int, MapTroopTpl*> map_troop_map_t;
//             typedef std::unordered_map<int, ArmySkillTpl*> map_army_skill_map_t;
//             typedef std::unordered_map<int, ArmyBattleTpl*> map_army_battle_map_t;
//             typedef std::unordered_map<int, BattleTpl*> map_battle_map_t;
            typedef std::vector< ScoutTypeTpl> vec_scout_type_t;
            //typedef std::map<std::tuple<int, int>, ResourceTpl*> map_resource_map_t;
            typedef std::unordered_map<int, ResourceTpl*> map_resource_map_t;
            typedef std::unordered_map<int, MonsterTpl*> map_monster_map_t;

            class TemplateLoader
            {
            public:
                TemplateLoader();
                ~TemplateLoader();

                const map_unit_map_t& map_units() const {
                    return m_map_units;
                }

                const MapUnitTpl* FindMapUnit(int id) const {
                    auto const it = m_map_units.find(id);
                    return it == m_map_units.end() ? nullptr : it->second;
                }

                const MapTroopTpl* FindMapTroop(model::MapTroopType type) const {
                    auto it = m_map_troops.find((int)type);
                    return it == m_map_troops.end() ? nullptr : it->second;
                }

//                 const ArmySkillTpl* FindArmySkill(int id) const {
//                     auto it = m_army_skills.find(id);
//                     return it == m_army_skills.end() ? nullptr : it->second;
//                 }
//
//                 const ArmyBattleTpl* FindArmyBattle(model::ArmyType type) const {
//                     auto it = m_army_battles.find((int)type);
//                     return it == m_army_battles.end() ? nullptr : it->second;
//                 }

                const MapUnitTpl* FindMapUnit(model::MapUnitType type, int level) const;
                
                const MapCityTpl* FindMapCity(int cityId) const {
                    auto it = m_map_city.find(cityId);
                    return it == m_map_city.end() ? nullptr : it->second;
                }

                const CityPatrolEventTpl* FindCityPatrolEvent(int id) const {
                    auto it = m_city_patrol_event.find(id);
                    return it == m_city_patrol_event.end() ? nullptr : it->second;
                }

                const CityPatrolEvtGroupTpl* FindCityPatrolEvtGroup(int groupId) const {
                    auto it = m_city_patrol_evt_group.find(groupId);
                    return it == m_city_patrol_evt_group.end() ? nullptr : it->second;
                }

                const map_resource_map_t& GetResourceRange() const {
                     return m_map_resource_group;
                }

                const MonsterTpl* GetMonsterDist(int serverLv) const {
                    auto it = m_map_monster_group.find(serverLv);
                    return it == m_map_monster_group.end() ? nullptr : it->second;
                }

                const NpcArmyTpl* FindNpcArmyTpl(int armyGroupId, int index = 0);
                
                const ArmyList GetRandomArmyList(int armyGroupId, int armyCount) const;

                void GetScoutType(int watchtowerLevel, std::vector< ScoutTypeTpl >& scoutTypes, bool isScout = true);

                void DebugDump();

            private:
                void BeginSetup(std::function<void(bool)> cb);

            private:
                
                void LoadMapNpcArmy();
                void LoadCityPatrolEvent();
                void LoadMapCity();
                void LoadMapUnit();
                void LoadMapTroop();
//                 void LoadArmySkill();
//                 void LoadArmyBattle();
//                 void LoadBattle();
                void LoadScoutType();
                void LoadMapResource();
                void LoadMapMonster();

                void LoadEnd();

            private:
                std::function<void(bool)> m_cb_setup;
                base::AutoObserver m_auto_observer;
                friend class ModuleTemplateLoader;

                map_unit_map_t m_map_units;
                map_troop_map_t m_map_troops;
//                 map_army_skill_map_t m_army_skills;
//                 map_army_battle_map_t m_army_battles;
//                 map_battle_map_t m_battles;
                
                map_npcarmy_map_t m_map_npcarmy;
                map_npcarmy_group_map_t m_map_npcarmy_group;
                map_city_map_t m_map_city;
                vec_scout_type_t m_vec_scout_type;
                city_patrol_event_t m_city_patrol_event;
                city_patrol_evt_group_t m_city_patrol_evt_group;
                map_resource_map_t m_map_resource_group;
                map_monster_map_t m_map_monster_group;
            };

            extern TemplateLoader* m_tploader;
        }
    }
}
#endif // TEMPLATELOADER_H
