#ifndef MAP_TROOP_EVENT_H
#define MAP_TROOP_EVENT_H
namespace ms
{
    namespace map
    {
        // 行军事件
        enum class MapTroopEvent
        {
            ADD = 1,                // 增加
            UPDATE = 2,        // 更新
            REMOVE = 3,       // 移除
        };
    }
}
#endif