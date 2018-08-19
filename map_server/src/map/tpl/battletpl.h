#ifndef MAP_TPL_BATTLETPL_H
#define MAP_TPL_BATTLETPL_H

#include <model/metadata.h>

namespace ms
{
    namespace map
    {
        namespace tpl
        {
            struct BattleTpl {
                model::BattleType type;
                float armyPowerRate = .0f;
            };
        }
    }
}

#endif // MAP_TPL_BATTLETPL_H
