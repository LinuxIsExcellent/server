#ifndef BASE_LUA_LUA_MODULE_CLUSTER_H
#define BASE_LUA_LUA_MODULE_CLUSTER_H

typedef struct lua_State lua_State;

namespace base
{
    namespace cluster
    {
        class MailboxID;
    }

    namespace lua
    {
        void luaopen_cluster(lua_State* L);

        base::cluster::MailboxID luaEx_check_mailboxId(lua_State* L, int idx);

        void luaEx_push_mailboxId(lua_State* L, const base::cluster::MailboxID& mbid);
    }
}

#endif // LUA_MODULE_CLUSTER_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
