#ifndef MAP_TPL_MAPUNITTPL_H
#define MAP_TPL_MAPUNITTPL_H
#include <model/metadata.h>
#include <model/tpl/drop.h>
#include <base/data.h>
#include <string>
#include <vector>

namespace ms
{
    namespace map
    {
        namespace tpl
        {
            using namespace model;

            struct MapUnitFamousCityTpl;
            struct MapUnitCapitalTpl;
            struct MapUnitChowTpl;
            struct MapUnitPrefectureTpl;
            struct MapUnitCountyTpl;
            struct MapUnitCastleTpl;
            struct MapUnitCampTempTpl;
            struct MapUnitCampFixedTpl;

            struct MapUnitResourceTpl;
            struct MapUnitFarmFoodTpl;
            struct MapUnitFarmWoodTpl;
            struct MapUnitMineIronTpl;
            struct MapUnitMineSparTpl;
            struct MapUnitMonsterTpl;
            struct MapUnitTreeTpl;

            struct MapUnitCatapultTpl;
            struct MapUnitWorldBossTpl;

            struct MapUnitTpl {
            public:
                MapUnitTpl(MapUnitType t) : type(t) {}
                virtual ~MapUnitTpl();

                int id = 0;
                MapUnitType type;

                std::string name;
                int level = 0;
                int headId = 0;
                int requireCastleLevel = 0;
                int cellSizeX = 0;
                int cellSizeY = 0;
                int dropId = 0;
                int armyGroup = 0;
                int armyCount = 0;
                int allianceDropId = 0;                            //击败NPC，联盟获得的奖励
                
                bool IsMonster() const {
                    return type == model::MapUnitType::MONSTER;
                }

                bool IsWorldBoss() const {
                    return type == model::MapUnitType::WORLD_BOSS;
                }

                const MapUnitFamousCityTpl* ToFamousCity() const;
                const MapUnitCapitalTpl* ToCapital() const;
                const MapUnitChowTpl* ToChow() const;
                const MapUnitPrefectureTpl* ToPrefecture() const;
                const MapUnitCountyTpl* ToCounty() const;
                const MapUnitCastleTpl* ToCastle() const;
                const MapUnitCampTempTpl* ToCampTemp() const;
                const MapUnitCampFixedTpl* ToCampFixed() const;

                const MapUnitResourceTpl* ToResource() const;
                const MapUnitFarmFoodTpl* ToFarmFood() const;
                const MapUnitFarmWoodTpl* ToFarmWood() const;
                const MapUnitMineIronTpl* ToMineIron() const;
                const MapUnitMineSparTpl* ToMineSpar() const;
                const MapUnitMonsterTpl* ToMonster() const;
                const MapUnitTreeTpl* ToTree() const;
                const MapUnitCatapultTpl* ToCatapult() const;
                const MapUnitWorldBossTpl* ToWorldBoss() const;
            };

            struct TrapConfig {
                int type = 0;
                int level = 0;
                int count = 0;
            };

            struct BuffRange {
                int x = 0;
                int y = 0;
            };

            // city base
            struct MapUnitFamousCityTpl : public MapUnitTpl {
                MapUnitFamousCityTpl(MapUnitType t) : MapUnitTpl(t) {}
                
                int cityDefense = 0;                        // 城防值
                int turretAtkPower = 0;
                BuffRange range;
                std::vector<TrapConfig> traps;
            };

            struct MapUnitCapitalTpl : public MapUnitFamousCityTpl {
                MapUnitCapitalTpl() : MapUnitFamousCityTpl(MapUnitType::CAPITAL) {}

            };

            struct MapUnitChowTpl : public MapUnitFamousCityTpl {
                MapUnitChowTpl() : MapUnitFamousCityTpl(MapUnitType::CHOW) {}

            };

            struct MapUnitPrefectureTpl : public MapUnitFamousCityTpl {
                MapUnitPrefectureTpl() : MapUnitFamousCityTpl(MapUnitType::PREFECTURE) {}

            };

            struct MapUnitCountyTpl : public MapUnitFamousCityTpl {
                MapUnitCountyTpl() : MapUnitFamousCityTpl(MapUnitType::COUNTY) {}

            };

            struct MapUnitCastleTpl : public MapUnitTpl {
                MapUnitCastleTpl() : MapUnitTpl(MapUnitType::CASTLE) {}

            };

            struct MapUnitCampTempTpl : public MapUnitTpl {
                MapUnitCampTempTpl() : MapUnitTpl(MapUnitType::CAMP_TEMP) {}
            };
            
            struct MapUnitCampFixedTpl : public MapUnitTpl {
                MapUnitCampFixedTpl() : MapUnitTpl(MapUnitType::CAMP_FIXED) {}

            };
            
            // resource base
            struct MapUnitResourceTpl : public MapUnitTpl {
                MapUnitResourceTpl(MapUnitType t) : MapUnitTpl(t) {}

                int capacity;
                float speed;
                int load;
            };

            struct MapUnitFarmFoodTpl : public MapUnitResourceTpl {
                MapUnitFarmFoodTpl() : MapUnitResourceTpl(MapUnitType::FARM_FOOD) {}
            };

            struct MapUnitFarmWoodTpl : public MapUnitResourceTpl {
                MapUnitFarmWoodTpl() : MapUnitResourceTpl(MapUnitType::FARM_WOOD) {}
            };

            struct MapUnitMineIronTpl : public MapUnitResourceTpl {
                MapUnitMineIronTpl() : MapUnitResourceTpl(MapUnitType::MINE_IRON) {}
            };

            struct MapUnitMineSparTpl : public MapUnitResourceTpl {
                MapUnitMineSparTpl() : MapUnitResourceTpl(MapUnitType::MINE_STONE) {}
            };

            struct MapUnitMonsterTpl : public MapUnitTpl {
                MapUnitMonsterTpl(MapUnitType type) : MapUnitTpl(type) {}
            };

            struct MapUnitCommonMonsterTpl : public MapUnitMonsterTpl {
                MapUnitCommonMonsterTpl() : MapUnitMonsterTpl(MapUnitType::MONSTER) {}
            };

            struct MapUnitTreeTpl : public MapUnitTpl {
                MapUnitTreeTpl() : MapUnitTpl(MapUnitType::TREE) {}
            };
            struct MapUnitCatapultTpl : public MapUnitTpl {
                MapUnitCatapultTpl() : MapUnitTpl(MapUnitType::CATAPULT) {}
            };

            struct MapUnitWorldBossTpl : public MapUnitMonsterTpl {
             MapUnitWorldBossTpl() : MapUnitMonsterTpl(MapUnitType::WORLD_BOSS) {}
             
             int hurtDropId = 0;
             int killDropId = 0;
             int allianceDropId = 0;
             int armyNum = 0;
            };
        }
    }
}
#endif // MAPUNITTPL_H
