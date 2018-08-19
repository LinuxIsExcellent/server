#include "configure.h"
#include <base/framework.h>

namespace model
{
    namespace tpl
    {
        int MonsterSiegeConf::RandomMonsterLevel(int castleLevel) const
        {
            int lv = 0;
            int min = castleLevel - lvMinCoe;
            if (min > 0) {
                lv = framework.random().GenRandomNum(min, min + lvRange);
            }
            return lv;
        }

    }
}
