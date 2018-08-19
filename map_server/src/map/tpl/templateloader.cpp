#include "templateloader.h"
#include "npcarmytpl.h"
#include "mapunittpl.h"
#include "maptrooptpl.h"
#include "mapcitytpl.h"
#include "scouttypetpl.h"
#include "army.h"
#include "battletpl.h"
#include "resourcetpl.h"
#include "monstertpl.h"
#include <base/logger.h>
#include <base/dbo/connection.h>
#include <base/framework.h>
#include <base/lua/parser.h>
#include <base/utils/utils_string.h>
#include <vector>
#include <algorithm>
#include "../armylist.h"
#include "model/tpl/templateloader.h"

namespace ms
{
    namespace map
    {
        namespace tpl
        {
            using namespace std;
            using namespace base;
            using namespace base::dbo;
            using namespace base::lua;
            using namespace base::utils;

            TemplateLoader* m_tploader = nullptr;

            TemplateLoader::TemplateLoader()
            {
                assert(m_tploader == nullptr);
                m_tploader = this;
            }

            TemplateLoader::~TemplateLoader()
            {
                m_tploader = nullptr;

                for (auto it = m_map_units.begin(); it != m_map_units.end(); ++it) {
                    delete it->second;
                }
                m_map_units.clear();

                for (auto it = m_map_troops.begin(); it != m_map_troops.end(); ++it) {
                    delete it->second;
                }
                m_map_troops.clear();

//                 for (auto it = m_army_skills.begin(); it != m_army_skills.end(); ++it) {
//                     delete it->second;
//                 }
//                 for (auto it = m_army_battles.begin(); it != m_army_battles.end(); ++it) {
//                     delete it->second;
//                 }
//                 for (auto it = m_battles.begin(); it != m_battles.end(); ++it) {
//                     delete it->second;
//                 }

                for (auto it = m_map_npcarmy.begin(); it != m_map_npcarmy.end(); ++it) {
                    delete it->second;
                }
                m_map_npcarmy.clear();

                for (auto it = m_map_city.begin(); it != m_map_city.end(); ++it) {
                    delete it->second;
                }
                m_map_city.clear();

                for (auto it = m_city_patrol_event.begin(); it != m_city_patrol_event.end(); ++it) {
                    delete it->second;
                }
                m_city_patrol_event.clear();

                for (auto it = m_city_patrol_evt_group.begin(); it != m_city_patrol_evt_group.end(); ++it) {
                    delete it->second;
                }
                m_city_patrol_evt_group.clear();

                for (auto it = m_map_resource_group.begin(); it != m_map_resource_group.end(); ++it) {
                    delete it->second;
                }
                m_map_resource_group.clear();

            }

            void TemplateLoader::DebugDump()
            {
                cout << "map template dump:";
                cout << " map_units:" << m_map_units.size();
                cout << " map_troops:" << m_map_troops.size();
//                 cout << " army_skills:" << m_army_skills.size();
//                 cout << " army_battles:" << m_army_battles.size();
//                 cout << " battles:" << m_battles.size();

                cout << endl;
            }

            const MapUnitTpl* TemplateLoader::FindMapUnit(MapUnitType type, int level) const
            {
                for (auto it = m_map_units.begin(); it != m_map_units.end(); ++it) {
                    if (MapUnitTpl* tpl = it->second) {
                        if (tpl->type == type && tpl->level == level) {
                            return tpl;
                        }
                    }
                }
                return nullptr;
            }

            const NpcArmyTpl* TemplateLoader::FindNpcArmyTpl(int armyGroupId, int index)
            {
                auto itGroup = m_map_npcarmy_group.find(armyGroupId);
                if (itGroup != m_map_npcarmy_group.end()) {
                    auto& npcArmyGroup =itGroup->second;
                    if ((int)npcArmyGroup.size() >  index && index >=  0) {
                        return npcArmyGroup[index];
                    }
                }
                return nullptr;
            }

            const ArmyList TemplateLoader::GetRandomArmyList(int armyGroupId, int armyCount) const
            {
                ArmyList armyList;
                auto itGroup = m_map_npcarmy_group.find(armyGroupId);
                if (itGroup != m_map_npcarmy_group.end()) {
                    auto armyGroup = itGroup->second;

                    int totalWeight = 0;
                    for (auto & tpl : armyGroup) {
                        totalWeight += tpl->weight;
                    }

                    int size = armyGroup.size();
                    int count = 0;
                    armyCount > size ? count = size : count = armyCount;
                    while (count-- > 0) {
                        int temp = 0;
                        int rand = framework.random().GenRandomNum(totalWeight);
                        for (int i = 0; i < size; ++i) {
                            auto tpl = armyGroup[i];
                            temp += tpl->weight;

                            if (temp >= rand) {
                                if (tpl->army.type >=  1 && tpl->army.type <=  9) {
                                    armyList.AddArmyGroup(tpl);
                                }

                                std::swap(armyGroup[i], armyGroup[size - 1]);

                                totalWeight -= tpl->weight;
                                --size;
                                break;
                            }
                        }
                    }
                }
                
                armyList.AssignPosition();
                armyList.InitProp();
                return armyList;
            }

            void TemplateLoader::GetScoutType(int watchtowerLevel, std::vector< ScoutTypeTpl >& scoutTypes, bool isScout)
            {
                    for (auto &tpl :  m_vec_scout_type) {
                        if (tpl.watchtowerLevel <=  watchtowerLevel) {
                            // 侦擦相关
                            if (isScout && (int)tpl.type / 100 != 5)
                            {
                                scoutTypes.push_back(tpl);
                            }else
                            {
                                scoutTypes.push_back(tpl);
                            }
                        } else {
                            break;
                        }
                    }
            }

            void TemplateLoader::BeginSetup(std::function<void(bool)> cb)
            {
                m_cb_setup = cb;
                LoadMapNpcArmy();
            }

            /******** load template **********/

            void TemplateLoader::LoadMapNpcArmy()
            {
                Statement* stmt = Statement::Create(1, "select id, groupId, headId, name, level, hero, heroSkill, army, weight  from tpl_map_npc_army");

                stmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        m_cb_setup(false);
                        LOG_DEBUG("load map npc army fail");
                    } else {
                        while (rs.Next()) {
                            int idx = 1;
                            NpcArmyTpl* tpl = new NpcArmyTpl;
                            tpl->id = rs.GetInt32(idx++);
                            tpl->groupId = rs.GetInt32(idx++);
                            tpl->headId = rs.GetInt32(idx++);
                            tpl->name = rs.GetString(idx++);
                            tpl->level = rs.GetInt32(idx++);
                            string heroConfig = rs.GetString(idx++);
                            int soul = 0/*rs.GetInt32(idx++)*/;
                            string heroSkill = rs.GetString(idx++);
                            string armyConfig = rs.GetString(idx++);
                            tpl->weight = rs.GetInt32(idx++);
                            Parser psr;
                            std::vector<int> hero = psr.ParseAsIntArray(heroConfig);
                            tpl->hero.id = hero.at(0);
                            tpl->hero.level = hero.at(1);
                            tpl->hero.star = hero.at(2);
                            tpl->hero.soul = soul;
                            assert(tpl->hero.level > 0);
                            if (hero.size() == 4) {
                                tpl->hero.position = hero.at(3);
                            }

                            /*DataTable dtEquipment = psr.ParseAsDataTable(heroeEquipment);
                            dtEquipment.ForeachVector([&](const int64_t  k, const DataValue & v) {
                                if (v.IsTable()) {
                                    auto& vTable = v.ToTable();
                                    tpl->hero.equipment.emplace_back(vTable.Get(1)->ToInteger(), vTable.Get(2)->ToInteger());
                                }
                                return false;
                            });*/

                            DataTable dtSkill = psr.ParseAsDataTable(heroSkill);
                            dtSkill.ForeachVector([&](const int64_t  k, const DataValue & v) {
                                if (v.IsTable()) {
                                    auto& vTable = v.ToTable();
                                    if (vTable.Count() >= 2) {
                                        tpl->hero.skill.emplace_back(vTable.Get(1)->ToInteger(), vTable.Get(2)->ToInteger());
                                    }
                                }
                                return false;
                            });

                            std::vector<int> army = psr.ParseAsIntArray(armyConfig);
                            tpl->army.type = army.at(0);
                            tpl->army.level = army.at(1);
                            tpl->army.count = army.at(2);

                            assert(tpl->army.level > 0);
                            assert(tpl->army.count > 0);

                            m_map_npcarmy.emplace(tpl->id, tpl);

                            auto armyGroup = m_map_npcarmy_group.find(tpl->groupId);
                            if (armyGroup != m_map_npcarmy_group.end()) {
                                armyGroup->second.push_back(tpl);
                            } else {
                                std::vector<NpcArmyTpl*> group;
                                group.push_back(tpl);
                                m_map_npcarmy_group.emplace(tpl->groupId, group);
                            }
                        }
                        LoadCityPatrolEvent();
                    }
                }, m_auto_observer);
            }

            void TemplateLoader::LoadCityPatrolEvent()
            {
                Statement* stmt = Statement::Create(1, "select id, groupId, type, armyCount, hurtHp, buffs, dropList, dropId, removeList, weight from tpl_city_patrol_event");

                stmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        m_cb_setup(false);
                        LOG_DEBUG("load tpl_city_patrol_event fail");
                    } else {
                        while (rs.Next()) {
                            int idx = 1;
                            CityPatrolEventTpl* tpl = new CityPatrolEventTpl;
                            tpl->id =  rs.GetInt32(idx++);
                            tpl->groupId = rs.GetInt32(idx++);
                            tpl->type = static_cast<model::PatrolEvent>(rs.GetInt32(idx++));
                            tpl->armyCount = rs.GetInt32(idx++);
                            tpl->hurtHp = rs.GetInt32(idx++);
                            tpl->buffs = rs.GetString(idx++);
                            std::string dropList = rs.GetString(idx++);
                            tpl->dropId = rs.GetInt32(idx++);
                            std::string removeList =  rs.GetString(idx++);
                            tpl->weight = rs.GetInt32(idx++);

                            Parser psr;
                            DataTable dtDropList = psr.ParseAsDataTable(dropList);
                            dtDropList.ForeachAll([&](const DataValue& k, const DataValue & v) {
                                if (k.IsInteger() && v.IsInteger()) {
                                    int itemId = k.ToInteger();
                                    int count = v.ToInteger();
                                    auto itemTpl = model::tpl::g_tploader->FindItem(itemId);
                                    if (itemTpl) {
                                        model::tpl::DropItem dropItem(*itemTpl, count);
                                        tpl->dropList.emplace_back(dropItem);
                                    }
                                }
                                return false;
                            });

                            DataTable dtRemoveList = psr.ParseAsDataTable(removeList);
                            dtRemoveList.ForeachAll([&](const DataValue& k, const DataValue & v) {
                                if (k.IsInteger() && v.IsInteger()) {
                                    int itemId = k.ToInteger();
                                    int count = v.ToInteger();
                                    auto itemTpl = model::tpl::g_tploader->FindItem(itemId);
                                    if (itemTpl) {
                                        model::tpl::DropItem dropItem(*itemTpl, count);
                                        tpl->removeList.emplace_back(dropItem);
                                    }
                                }
                                return false;
                            });

                            m_city_patrol_event.emplace(tpl->id, tpl);

                            auto itGroup = m_city_patrol_evt_group.find(tpl->groupId);
                            if (itGroup ==  m_city_patrol_evt_group.end()) {
                                CityPatrolEvtGroupTpl*  group = new CityPatrolEvtGroupTpl;
                                itGroup = m_city_patrol_evt_group.emplace(tpl->groupId,  group).first;
                            }

                            if (itGroup !=  m_city_patrol_evt_group.end()) {
                                auto group = itGroup->second;
                                if (tpl->type ==  model::PatrolEvent::REWARD) {
                                    group->rewardEvents.emplace_back(tpl);
                                } else {
                                    group->normalEvents.emplace_back(tpl);
                                }
                            }
                        }

                        for (auto value : m_city_patrol_evt_group) {
                            auto group = value.second;
                            for (auto evt : group->normalEvents) {
                                if (evt) {
                                    group->normalTotalWeight +=  evt->weight;
                                }
                            }

                            for (auto evt : group->rewardEvents) {
                                if (evt) {
                                    group->rewardTotalWeight +=  evt->weight;
                                }
                            }
                        }

                        LoadMapCity();
                    }
                }, m_auto_observer);
            }

            void TemplateLoader::LoadMapCity()
            {
                Statement* stmt = Statement::Create(1, "SELECT id, name, armyGroup, armyCount, paramExt, armyNum, dropId FROM tpl_map_city");

                stmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        m_cb_setup(false);
                        LOG_DEBUG("load map unit fail");
                    } else {
                        while (rs.Next()) {
                            int idx = 1;
                            MapCityTpl* tpl = new MapCityTpl;
                            tpl->cityId = rs.GetInt32(idx++);
                            tpl->name = rs.GetString(idx++);
                            tpl->armyGroup = rs.GetInt32(idx++);
                            tpl->armyCount = rs.GetInt32(idx++);
                            string paramExt = rs.GetString(idx++);
                            tpl->armyNum = rs.GetInt32(idx++);
                            tpl->dropId = rs.GetInt32(idx++);
                            Parser psr;
                            DataTable dtParamExt = psr.ParseAsDataTable(paramExt);
                            DataValue* dataValue = dtParamExt.Get("turretAtkPower");
                            tpl->turretAtkPower = dataValue ? dataValue->ToInteger() : 0;
                            dataValue = dtParamExt.Get("traps");
                            if (dataValue) {
                                auto& trapTable = dataValue->ToTable();
                                trapTable.ForeachVector([&](int k, const DataValue & v) {
                                    if (v.IsTable()) {
                                        auto& confgTable = v.ToTable();
                                        TrapConfig config;
                                        config.type = confgTable.Get(1)->ToInteger();
                                        config.level = confgTable.Get(2)->ToInteger();
                                        config.count = confgTable.Get(3)->ToInteger();
                                        tpl->traps.emplace_back(config);
                                    }
                                    return false;
                                });
                            }

                            dataValue = dtParamExt.Get("patrolEvent");
                            if (dataValue) {
                                    int evtGroupId = dataValue->ToInteger();
                                    tpl->evtGroupId = evtGroupId;

//                                 auto& patrolEventTable = dataValue->ToTable();
//                                 patrolEventTable.ForeachVector([&](int k, const DataValue & v) {
//                                     if (v.IsInteger()) {
//                                         int id = v.ToInteger();
//                                        auto patrolEventTpl = FindCityPatrolEvent(id);
//                                         if (patrolEventTpl) {
//                                             if (patrolEventTpl->type ==  model::PatrolEvent::REWARD) {
//                                                 tpl->rewardEvents.emplace_back(patrolEventTpl);
//                                             } else {
//                                                 tpl->normalEvents.emplace_back(patrolEventTpl);
//                                             }
//                                         } else {
//                                             LOG_WARN("wrong patrolEvent %d", id);
//                                         }
//                                     }
//                                     return false;
//                                 });
                            }

//                             for (auto evt : tpl->normalEvents) {
//                                 if (evt) {
//                                     tpl->normalTotalWeight +=  evt->weight;
//                                 }
//                             }
//
//                             for (auto evt : tpl->rewardEvents) {
//                                 if (evt) {
//                                     tpl->rewardTotalWeight +=  evt->weight;
//                                 }
//                             }

                            m_map_city.emplace(tpl->cityId, tpl);
                        }
                        LoadMapUnit();
                    }
                }, m_auto_observer);
            }

            void TemplateLoader::LoadMapUnit()
            {
                Statement* stmt = Statement::Create(1, "SELECT id, unitType, level, name, headId, requireCastleLevel, cellSizeX, cellSizeY,\
                 dropId, armyGroup, armyCount, armyNum, paramExt FROM tpl_map_unit");

                stmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        m_cb_setup(false);
                        LOG_DEBUG("load map unit fail");
                    } else {
                        while (rs.Next()) {
                            int idx = 1;
                            int id = rs.GetInt32(idx++);
                            MapUnitType unitType = static_cast<MapUnitType>(rs.GetInt32(idx++));
                            int level = rs.GetInt32(idx++);
                            string name = rs.GetString(idx++);
                            int headId = rs.GetInt32(idx++);
                            int requireCastleLevel = rs.GetInt32(idx++);
                            int cellSizeX = rs.GetInt32(idx++);
                            int cellSizeY = rs.GetInt32(idx++);
                            int dropId = rs.GetInt32(idx++);
                            int armyGroup = rs.GetInt32(idx++);
                            int armyCount = rs.GetInt32(idx++);
                            int armyNum = rs.GetInt32(idx++);
                            string paramExt = rs.GetString(idx++);
                            DataTable paramDt = Parser::ParseAsDataTable(paramExt);

                            MapUnitTpl* tpl = nullptr;
                            switch (unitType) {
                                case MapUnitType::CAPITAL: {
                                    tpl = new MapUnitCapitalTpl();
                                }
                                break;
                                case MapUnitType::CHOW: {
                                    tpl = new MapUnitChowTpl();
                                }
                                break;
                                case MapUnitType::PREFECTURE: {
                                    tpl = new MapUnitPrefectureTpl();
                                }
                                break;
                                case MapUnitType::COUNTY: {
                                    tpl = new MapUnitCountyTpl();
                                }
                                break;
                                case MapUnitType::CASTLE: {
                                    tpl = new MapUnitCastleTpl();
                                }
                                break;
                                case MapUnitType::CAMP_TEMP: {
                                    tpl = new MapUnitCampTempTpl();
                                }
                                break;
                                case MapUnitType::CAMP_FIXED: {
                                    tpl = new MapUnitCampFixedTpl();
                                }
                                break;
                                case MapUnitType::FARM_FOOD: {
                                    tpl = new MapUnitFarmFoodTpl();
                                }
                                break;
                                case MapUnitType::FARM_WOOD: {
                                    tpl = new MapUnitFarmWoodTpl();
                                }
                                break;
                                case MapUnitType::MINE_IRON: {
                                    tpl = new MapUnitMineIronTpl();
                                }
                                break;
                                case MapUnitType::MINE_STONE: {
                                    tpl = new MapUnitMineSparTpl();
                                }
                                break;
                                case MapUnitType::MONSTER: {
                                    tpl = new MapUnitCommonMonsterTpl();
//                                     MapUnitCommonMonsterTpl* t = (MapUnitCommonMonsterTpl*)tpl;
                                }
                                break;
                                case MapUnitType::TREE: {
                                    tpl = new MapUnitTreeTpl();
                                }
                                break;
                                case MapUnitType::CATAPULT:
                                {
                                    tpl = new MapUnitCatapultTpl();
                                    //cout << "LoadCatapultTpl............" << endl;
                                    /*cout << "id, level, name, headId, requireCastleLevel, cellSizeX, cellSizeY, dropId, armyGroup, armyCount, paramExt....."
                                        << id << level << name << headId << requireCastleLevel << cellSizeX << cellSizeY << dropId << armyGroup << armyCount << paramExt
                                            << endl;
                                            */
                                }
                                break;
                                case MapUnitType::WORLD_BOSS: {
                                    tpl = new MapUnitWorldBossTpl();
                                    MapUnitWorldBossTpl* t = (MapUnitWorldBossTpl*)tpl;
                                    t->armyNum = armyNum;
                                    if (DataValue* dv = paramDt.Get("hurtDrops")) {
                                        t->hurtDropId = dv->ToInteger();
                                    }
                                    if (DataValue* dv = paramDt.Get("killDropId")) {
                                        t->killDropId = dv->ToInteger();
                                    }
                                    if (DataValue* dv = paramDt.Get("allianceDropId")) {
                                        t->allianceDropId = dv->ToInteger();
                                    }
                                    /*cout << "@@@MapUnitType::WORLD_BOSS" << endl;
                                    cout << "id, level, name, headId, requireCastleLevel, cellSizeX, cellSizeY, dropId, armyGroup, armyCount, paramExt....."
                                        << id << level << name << headId << requireCastleLevel << cellSizeX << cellSizeY << dropId << armyGroup << armyCount << paramExt
                                            << endl;*/
                                            
                                }
                                break;
                                default:
                                    cout << "undefind MapUnitType:" << (int)unitType;
                            }

                            if (tpl != nullptr) {
                                tpl->id = id;
                                tpl->type = unitType;
                                tpl->name = name;
                                tpl->level = level;
                                tpl->headId = headId;
                                tpl->requireCastleLevel = requireCastleLevel;
                                tpl->cellSizeX = cellSizeX;
                                tpl->cellSizeY = cellSizeY;
                                tpl->dropId = dropId;
                                tpl->armyGroup = armyGroup;
                                tpl->armyCount = armyCount;
                                Parser psr;
                                DataTable dtParamExt = psr.ParseAsDataTable(paramExt);
                                DataValue* dataValue = dtParamExt.Get("allianceDropId");
                                tpl->allianceDropId = dataValue ? dataValue->ToInteger() : 0;
                                if (tpl->ToFamousCity()) {
                                    MapUnitFamousCityTpl* t =  static_cast<MapUnitFamousCityTpl*>(tpl);
                                    dataValue = dtParamExt.Get("cityDefense");
                                    t->cityDefense = dataValue ? dataValue->ToInteger() : 0;
                                    dataValue = dtParamExt.Get("turretAtkPower");
                                    t->turretAtkPower = dataValue ? dataValue->ToInteger() : 0;
                                    dataValue = dtParamExt.Get("buffRange");
                                    if (dataValue) {
                                        auto& rangeTable = dataValue->ToTable();
                                        t->range.x = rangeTable.Get(1)->ToInteger();
                                        t->range.y = rangeTable.Get(2)->ToInteger();
                                    }
                                    dataValue = dtParamExt.Get("traps");
                                    if (dataValue) {
                                        auto& trapTable = dataValue->ToTable();
                                        trapTable.ForeachVector([&](int k, const DataValue & v) {
                                            if (v.IsTable()) {
                                                auto& confgTable = v.ToTable();
                                                TrapConfig config;
                                                config.type = confgTable.Get(1)->ToInteger();
                                                config.level = confgTable.Get(2)->ToInteger();
                                                config.count = confgTable.Get(3)->ToInteger();
                                                t->traps.emplace_back(config);
                                            }
                                            return false;
                                        });
                                    }
                                    dataValue = dtParamExt.Get("allianceDropId");
                                    t->allianceDropId = dataValue ? dataValue->ToInteger() : 0;
                                } else if (tpl->ToResource()) {
                                    MapUnitResourceTpl* t =  static_cast<MapUnitResourceTpl*>(tpl);
                                    Parser psr;
                                    DataTable dtParamExt = psr.ParseAsDataTable(paramExt);
                                    DataValue* dataValue = dtParamExt.Get("resourceCapacity");
                                    t->capacity = dataValue ? dataValue->ToInteger() : 0;
                                    dataValue = dtParamExt.Get("resourceSpeed");
                                    t->speed = dataValue ? dataValue->ToDouble() : 0.0f;
                                    dataValue = dtParamExt.Get("resourceLoad");
                                    t->load = dataValue ? dataValue->ToInteger() : 0;
                                }

                                m_map_units.emplace(id, tpl);
                            }
                        }

                        LoadMapTroop();
                    }
                }, m_auto_observer);
            }

            void TemplateLoader::LoadMapTroop()
            {
                Statement* stmt = Statement::Create(1, "select type, speedAddition, speedAlter, unitTime, baseSpeed from tpl_map_troop");

                stmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        m_cb_setup(false);
                        LOG_DEBUG("load map troop fail");
                    } else {
                        while (rs.Next()) {
                            int idx = 1;
                            MapTroopTpl* tpl = new MapTroopTpl;
                            tpl->type = static_cast<MapTroopType>(rs.GetInt32(idx++));
                            tpl->speedAddition = rs.GetFloat(idx++);
                            tpl->speedAlter = rs.GetFloat(idx++);
                            tpl->unitTime = rs.GetInt32(idx++);
                            tpl->baseSpeed = rs.GetFloat(idx++);
                            m_map_troops.emplace((int)tpl->type, tpl);
                        }
                        LoadScoutType();
                    }
                }, m_auto_observer);
            }

            void TemplateLoader::LoadScoutType()
            {
                Statement* stmt = Statement::Create(1, "select id, watchtowerLevel, uiKeywords from tpl_scout_type");

                stmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        m_cb_setup(false);
                        LOG_DEBUG("load tpl_scout_type fail");
                    } else {
                        while (rs.Next()) {
                            int idx = 1;
                            ScoutTypeTpl tpl;
                            tpl.type = static_cast<model::WATCHTOWER_SCOUT_TYPE>(rs.GetInt32(idx++));
                            tpl.watchtowerLevel = rs.GetInt32(idx++);
                            tpl.uiKeywords = rs.GetString(idx++);
                            m_vec_scout_type.emplace_back(tpl);
                        }

                        std::sort(m_vec_scout_type.begin(), m_vec_scout_type.end(),  [](const ScoutTypeTpl& tpl1,  const ScoutTypeTpl& tpl2) {
                            return tpl1.watchtowerLevel < tpl2.watchtowerLevel;
                        });

                        LoadMapResource();
                    }
                }, m_auto_observer);
            }

            /******** load map resource **********/
            void TemplateLoader::LoadMapResource()
            {
                Statement* stmt = Statement::Create(1, "select id, regionLv, resourcesLv, foodstuff, wood, iron, ore from tpl_map_resource");

                stmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        m_cb_setup(false);
                        LOG_DEBUG("load map resource fail");
                    } else {
                            
                        while (rs.Next()) {
                            int idx = 1;
                            rs.GetInt32(idx++);  //skip one 
                            int regionLv = rs.GetInt32(idx++);

                            auto it = m_map_resource_group.find(regionLv);
                            if (it == m_map_resource_group.end()) {
                                ResourceTpl* tpl = new ResourceTpl;
                                tpl->regionLv = regionLv;

                                ResourceNumTpl num_tpl;
                                num_tpl.resourceLv = rs.GetInt32(idx++);
                                num_tpl.food = rs.GetInt32(idx++);
                                num_tpl.wood = rs.GetInt32(idx++);
                                num_tpl.iron = rs.GetInt32(idx++);
                                num_tpl.ore = rs.GetInt32(idx++);
                                tpl->resourceNumTpl.push_back(num_tpl);

                                m_map_resource_group.emplace(regionLv, tpl);
                            } else {
                                ResourceTpl* tpl = it->second;

                                ResourceNumTpl num_tpl;
                                num_tpl.resourceLv = rs.GetInt32(idx++);
                                num_tpl.food = rs.GetInt32(idx++);
                                num_tpl.wood = rs.GetInt32(idx++);
                                num_tpl.iron = rs.GetInt32(idx++);
                                num_tpl.ore = rs.GetInt32(idx++);
                                tpl->resourceNumTpl.push_back(num_tpl);
                            }
                        }
                        LoadMapMonster();
                    }
                }, m_auto_observer);
            }

            /******** load map resource **********/
            void TemplateLoader::LoadMapMonster()
            {
                Statement* stmt = Statement::Create(1, "select id, serverLv, regionLv, monsterLv, monsterNum from tpl_map_monster");

                stmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        m_cb_setup(false);
                        LOG_DEBUG("load map resource fail");
                    } else {
                            
                        while (rs.Next()) {
                            int idx = 1;
                            rs.GetInt32(idx++); //skip one
                            int serverLv = rs.GetInt32(idx++);
                            auto it = m_map_monster_group.find(serverLv);
                            if (it == m_map_monster_group.end()) {
                                MonsterTpl* monster_tpl = new MonsterTpl;
                                int regionLv = rs.GetInt32(idx++);
                                auto iter = monster_tpl->distribute.find(regionLv);
                                if (iter == monster_tpl->distribute.end()) {
                                    MonsterNumTpl num_tpl;
                                    num_tpl.regionLv = regionLv;
                                    int monster_level = rs.GetInt32(idx++);
                                    int num = rs.GetInt32(idx++);
                                    num_tpl.level_dist.emplace(monster_level, num);

                                    monster_tpl->distribute.emplace(regionLv, num_tpl);
                                } else {
                                    MonsterNumTpl& num_tpl = iter->second;
                                    int monster_level = rs.GetInt32(idx++);
                                    int num = rs.GetInt32(idx++);
                                    num_tpl.level_dist.emplace(monster_level, num);
                                }

                                m_map_monster_group.emplace(serverLv, monster_tpl);
                            } else {
                                MonsterTpl* monster_tpl = it->second;
                                int regionLv = rs.GetInt32(idx++);
                                auto iter = monster_tpl->distribute.find(regionLv);
                                if (iter == monster_tpl->distribute.end()) {
                                    MonsterNumTpl num_tpl;
                                    num_tpl.regionLv = regionLv;
                                    int monster_level = rs.GetInt32(idx++);
                                    int num = rs.GetInt32(idx++);
                                    num_tpl.level_dist.emplace(monster_level, num);

                                    monster_tpl->distribute.emplace(regionLv, num_tpl);
                                } else {
                                    MonsterNumTpl& num_tpl = iter->second;
                                    int monster_level = rs.GetInt32(idx++);
                                    int num = rs.GetInt32(idx++);
                                    num_tpl.level_dist.emplace(monster_level, num);
                                }
                            }
                        }
                        LoadEnd();
                    }
                }, m_auto_observer);
            }


            void TemplateLoader::LoadEnd()
            {
                m_cb_setup(true);
            }

        }
    }
}

