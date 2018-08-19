#ifndef MAP_TPL_RESOURCE_H
#define MAP_TPL_RESOURCE_H
#include <model/metadata.h>
#include <string>
#include <vector>

namespace ms
{
    namespace map
    {
        namespace tpl
        {
            struct ResourceNumTpl {
                int resourceLv;
                int food;
                int wood;
                int iron;
                int ore; 
            };

            struct ResourceTpl {
                int regionLv; 
                std::vector<ResourceNumTpl> resourceNumTpl;
            };
        }
    }
}

#endif // MAP_TPL_RESOURCE_H

