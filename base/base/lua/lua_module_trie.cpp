#include "lua_module_trie.h"
#include "luaex.h"
#include <lua/lua.hpp>
#include "../trie/trietree.h"
#include "../logger.h"
#include <algorithm>

namespace base
{
    namespace lua
    {
        using namespace std;

        static int _lua_trie_addWord(lua_State* L)
        {
            std::string word = luaL_checkstring(L, 1);
            base::trie::g_trie->AddWord(word);
            return 0;
        }

        static int _lua_trie_isContain(lua_State* L)
        {
            std::string text = luaL_checkstring(L, 1);
            bool result = base::trie::g_trie->IsContain(text);
            lua_pushboolean(L, result);
            return 1;
        }

        static int _lua_trie_filter(lua_State* L)
        {
            std::string data = luaL_checkstring(L, 1);
            int argc = lua_gettop(L);
            if (argc == 1) {
                base::trie::g_trie->Filter(data);
            } else {
                size_t len = 0;
                const char* chars = luaL_checklstring(L, 2, &len);
                if (len > 0) {
                    base::trie::g_trie->Filter(data, chars[0]);
                } else {
                    base::trie::g_trie->Filter(data);
                }
            }
            lua_pushstring(L, data.c_str());
            return 1;
        }


        static int _lua_open_trie(lua_State* L)
        {
            luaL_Reg libFuns[] = {
                {"addWord", _lua_trie_addWord},
                {"isContain", _lua_trie_isContain},
                {"filter", _lua_trie_filter},
                {nullptr, nullptr}
            };

            luaL_newlib(L, libFuns);
            return 1;
        }

        void luaopen_trie(lua_State* L)
        {
            luaL_requiref(L, "trie", &_lua_open_trie, 0);
            lua_pop(L, 1);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
