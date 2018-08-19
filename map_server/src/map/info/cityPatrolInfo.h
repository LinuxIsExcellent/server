#ifndef MAP_INFO_CITY_PATROL_H
#define MAP_INFO_CITY_PATROL_H
#include <map>

namespace ms
{
    namespace map
    {
        namespace info
        {

            struct CityPatrol {
                int patrolCount = 0;                        // 已巡逻次数
                std::map<int, int64_t> patrolCD;             // id, endTime
            };

        }
    }
}

#endif  //MAP_INFO_CITY_PATROL_H