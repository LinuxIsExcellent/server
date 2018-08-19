#include "mapunittpl.h"
#include <base/framework.h>

namespace ms
{
    namespace map
    {
        namespace tpl
        {
            using namespace std;
            using namespace base;

            MapUnitTpl::~MapUnitTpl()
            {
            }

            const MapUnitFamousCityTpl* MapUnitTpl::ToFamousCity() const
            {
                if (type == MapUnitType::CAPITAL
                        || type == MapUnitType::CHOW
                        || type == MapUnitType::PREFECTURE
                        || type == MapUnitType::COUNTY) {
                    return static_cast<const MapUnitFamousCityTpl*>(this);
                }
                return nullptr;
            }

            const MapUnitCapitalTpl* MapUnitTpl::ToCapital() const
            {
                return type == MapUnitType::CAPITAL ? static_cast<const MapUnitCapitalTpl*>(this) : nullptr;
            }

            const MapUnitChowTpl* MapUnitTpl::ToChow() const
            {
                return type == MapUnitType::CHOW ? static_cast<const MapUnitChowTpl*>(this) : nullptr;
            }

            const MapUnitPrefectureTpl* MapUnitTpl::ToPrefecture() const
            {
                return type == MapUnitType::PREFECTURE ? static_cast<const MapUnitPrefectureTpl*>(this) : nullptr;
            }

            const MapUnitCountyTpl* MapUnitTpl::ToCounty() const
            {
                return type == MapUnitType::COUNTY ? static_cast<const MapUnitCountyTpl*>(this) : nullptr;
            }

            const MapUnitCastleTpl* MapUnitTpl::ToCastle() const
            {
                return type == MapUnitType::CASTLE ? static_cast<const MapUnitCastleTpl*>(this) : nullptr;
            }

            const MapUnitCampTempTpl* MapUnitTpl::ToCampTemp() const
            {
                return type == MapUnitType::CAMP_TEMP ? static_cast<const MapUnitCampTempTpl*>(this) : nullptr;
            }
            const MapUnitCampFixedTpl* MapUnitTpl::ToCampFixed() const
            {
                return type == MapUnitType::CAMP_FIXED ? static_cast<const MapUnitCampFixedTpl*>(this) : nullptr;
            }

            const MapUnitResourceTpl* MapUnitTpl::ToResource() const
            {
                if (type == MapUnitType::FARM_FOOD
                        || type == MapUnitType::FARM_WOOD
                        || type == MapUnitType::MINE_IRON
                        || type == MapUnitType::MINE_STONE) {
                    return static_cast<const MapUnitResourceTpl*>(this);
                }
                return nullptr;
            }

            const MapUnitFarmFoodTpl* MapUnitTpl::ToFarmFood() const
            {
                return type == MapUnitType::FARM_FOOD ? static_cast<const MapUnitFarmFoodTpl*>(this) : nullptr;
            }

            const MapUnitFarmWoodTpl* MapUnitTpl::ToFarmWood() const
            {
                return type == MapUnitType::FARM_WOOD ? static_cast<const MapUnitFarmWoodTpl*>(this) : nullptr;
            }

            const MapUnitMineIronTpl* MapUnitTpl::ToMineIron() const
            {
                return type == MapUnitType::MINE_IRON ? static_cast<const MapUnitMineIronTpl*>(this) : nullptr;
            }

            const MapUnitMineSparTpl* MapUnitTpl::ToMineSpar() const
            {
                return type == MapUnitType::MINE_STONE ? static_cast<const MapUnitMineSparTpl*>(this) : nullptr;
            }

            const MapUnitMonsterTpl* MapUnitTpl::ToMonster() const
            {
                return IsMonster() ? static_cast<const MapUnitMonsterTpl*>(this) : nullptr;
            }

            const MapUnitTreeTpl* MapUnitTpl::ToTree() const
            {
                return type == MapUnitType::TREE ? static_cast<const MapUnitTreeTpl*>(this) : nullptr;
            }

            const MapUnitCatapultTpl* MapUnitTpl::ToCatapult() const
            {
                return type == MapUnitType::CATAPULT ? static_cast<const MapUnitCatapultTpl*>(this) : nullptr;
            }
            
            const MapUnitWorldBossTpl* MapUnitTpl::ToWorldBoss() const
            {
                return type == MapUnitType::WORLD_BOSS ? static_cast<const MapUnitWorldBossTpl*>(this) : nullptr;
            }
        }
    }
}

