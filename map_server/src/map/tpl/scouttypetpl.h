#ifndef MAP_TPL_SCOUTTYPETPL_H
#define MAP_TPL_SCOUTTYPETPL_H
# include <string>
#include <model/metadata.h>

namespace ms
{
    namespace map
    {
        namespace tpl
        {
            struct ScoutTypeTpl {
                model::WATCHTOWER_SCOUT_TYPE type;
                int watchtowerLevel = 0;
                std::string uiKeywords;
            };
        }
    }
}

#endif // MAP_TPL_SCOUTTYPETPL_H
