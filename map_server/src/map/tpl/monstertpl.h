#ifndef MONSTER_TPL_RESOURCE_H
#define MONSTER_TPL_RESOURCE_H
#include <model/metadata.h>
#include <string>
#include <vector>

namespace ms
{
    namespace map
    {
        namespace tpl
        {
            // struct MonsterNumTpl {
            //     int resourceLv;
            //     int food;
            //     int wood;
            //     int iron;
            //     int ore; 
            // };

            struct MonsterNumTpl {
                int regionLv;
                std::unordered_map<int, int> level_dist;
            };

            struct MonsterTpl {
                int serverLv;
                std::unordered_map<int, MonsterNumTpl> distribute;
            };
        }
    }
}

#endif // MONSTER_TPL_RESOURCE_H