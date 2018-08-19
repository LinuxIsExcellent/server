#ifndef BASE_LUA_LUA_MODULE_VM_H
#define BASE_LUA_LUA_MODULE_VM_H

typedef struct lua_State lua_State;

namespace base
{
    namespace lua
    {
        int lua_vm_execute_cached_script(lua_State* L, const char* name, int argsBeginIdx);
        void luaopen_vm(lua_State* L);
    }
}

#endif // LUA_MODULE_VM_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
