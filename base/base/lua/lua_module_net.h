#ifndef BASE_LUA_NET_LUA_MODULE_NET_H
#define BASE_LUA_NET_LUA_MODULE_NET_H

#include <string>

typedef struct lua_State lua_State;


namespace base
{
    namespace gateway
    {
        class PacketIn;
        class UserClient;
    }

    namespace cluster
    {
        class MessageIn;
    }

    namespace lua
    {
        int lua_net_send_values(base::gateway::UserClient* client, lua_State* L, int beginIdx, int session = 0);
        void lua_net_push_packet_in(lua_State* L, base::gateway::PacketIn& pktin);
        base::gateway::PacketIn* lua_check_packet_in(lua_State* L, int idx);
        void lua_net_push_message_in(lua_State* L, base::cluster::MessageIn& msgin);
        int lua_net_new_pktout(base::gateway::UserClient* client, lua_State* L, int code, int size, int session = 0);

        void luaopen_net(lua_State* L);
    }
}

#endif
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
