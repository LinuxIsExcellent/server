#ifndef LUA_LUAER_H
#define LUA_LUAER_H
#include <base/3rd/lua/lua.hpp>

namespace lua
{
    class Luaer
    {
    public:
        Luaer();
        virtual ~Luaer();
        
    public:
        lua_State* L() {
            return L_;
        }
        void Setup();
        
    private:
        lua_State* L_ = nullptr;
    };
}
#endif // LUA_LUAER_H
