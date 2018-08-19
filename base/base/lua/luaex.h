#ifndef BASE_LUA_LUAEX_H
#define BASE_LUA_LUAEX_H

typedef struct lua_State lua_State;

namespace base
{
    namespace lua
    {
        lua_State* luaEx_get_main_thread(lua_State* L);

        int luaEx_dump_stack(lua_State* L);

        int luaEx_resume(lua_State* L, lua_State* from, int nargs);

        int luaEx_pcall(lua_State* L, int nargs, int nresults, int msgh);

        void luaEx_createPrivateTableOnRegistry(lua_State* L, void* addr, const char* name, const char* weakFlag);

        int luaEx_getPrivateTableOnRegistry(lua_State* L, void* addr);

        class StackKeeper
        {
        public:
            StackKeeper(lua_State* L);
            ~StackKeeper();

        private:
            StackKeeper(const StackKeeper& rhs) = delete;
            StackKeeper& operator = (StackKeeper& rhs) = delete;
            lua_State* m_L;
            int m_oldIndex;
        };
    }
}

#endif // LUAEX_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
