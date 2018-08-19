#ifndef MAP_UNIT_EVENT_H
#define MAP_UNIT_EVENT_H
namespace ms
{
    namespace map
    {
        // Unit事件
        enum class MapUnitEvent
        {
            ADD = 1,                // 增加
            UPDATE = 2,        // 更新
            REMOVE = 3,       // 移除
        };
    }
}
#endif