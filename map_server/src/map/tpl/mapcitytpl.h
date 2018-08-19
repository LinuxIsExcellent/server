#ifndef MAPCITYTPL_H
#define MAPCITYTPL_H
#include <vector>
#include "mapunittpl.h"
#include "model/tpl/drop.h"
#include <model/metadata.h>

namespace ms
{
    namespace map
    {
        namespace tpl
        {
            struct CityPatrolEventTpl
            {
                int id = 0;
                int groupId = 0;
                model::PatrolEvent type;
                int armyCount = 0;
                int hurtHp = 0;
                std::string buffs;
                std::vector<model::tpl::DropItem> dropList;
                std::vector<model::tpl::DropItem> removeList;
                int dropId = 0;
                int weight = 0;
            };

            struct CityPatrolEvtGroupTpl
            {
                std::vector<CityPatrolEventTpl*> normalEvents;
                std::vector<CityPatrolEventTpl*> rewardEvents;
                int normalTotalWeight = 0;
                int rewardTotalWeight = 0;

                const CityPatrolEventTpl* GetRandomEvent(bool isNormal) const;
            };

            struct MapCityTpl 
            {
                int cityId = 0;
                std::string name;
                int armyGroup = 0;
                int armyCount = 0;
                int turretAtkPower = 0;
                std::vector<TrapConfig> traps;
                int evtGroupId = 0;
                int armyNum = 0;
                int dropId = 0;
            };
        }
    }
}

#endif // MAPCITYTPL_H
