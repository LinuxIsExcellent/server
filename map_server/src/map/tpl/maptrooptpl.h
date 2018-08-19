#ifndef MAP_TPL_MAPTROOPTPL_H
#define MAP_TPL_MAPTROOPTPL_H

#include <model/metadata.h>

namespace ms
{
    namespace map
    {
        namespace tpl
        {
            struct MapTroopTpl {
                model::MapTroopType type;
                int unitTime = 20;
                float baseSpeed = 0.05f;
                float speedAddition = 0.2f;
                float speedAlter = 1;
            };
        }
    }
}

#endif // MAP_TPL_MAPTROOPTPL_H
