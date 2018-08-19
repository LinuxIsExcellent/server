#ifndef MODEL_RPC_MAP_H
#define MODEL_RPC_MAP_H

namespace model
{
    namespace rpc
    {
        enum class MapCode
        {
            LUA_STUB = 0,
            ENTER = 1,
            QUIT,
            LUA_CALL,
            LUA_CAST,
            FORWARD,
            EVENT,
            CROSS_TELEPORT, //跨服迁城
            LOCK_PKTIN,
        };
    }
}

#endif
