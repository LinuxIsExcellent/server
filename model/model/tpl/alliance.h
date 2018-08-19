#ifndef MODEL_TPL_ALLIANCE_H
#define MODEL_TPL_ALLIANCE_H

#include "../metadata.h"
#include <string>
#include <map>

namespace model
{
    namespace tpl
    {
        struct AllianceLevelTpl
        {
            int level = 0;
            int exp = 0;
            int cityMax = 0;
            int castleMax = 0;
            int troopsMax = 0;
            int towerMax = 0;
            int patrolMax = 0;
        };
    }
}

#endif // MODEL_TPL_ALLIANCE_H
