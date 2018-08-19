#ifndef BASE_LUA_LUA_MODULE_TRIE_H
#define BASE_LUA_LUA_MODULE_TRIE_H

#include <string>

typedef struct lua_State lua_State;

namespace base
{
    namespace lua
    {
        void luaopen_trie(lua_State* L);
    }
}

#endif // LUA_MODULE_TRIE_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
