#include "luaer.h"
#include <base/utils/file.h>
#include <base/framework.h>
#include <base/lua/lua_module_utils.h>
#include <base/lua/lua_module_timer.h>
#include "lua_olibs.h"
#include <base/lua/lua_module_net.h>

namespace lua
{
    using namespace std;
    using namespace base;
    using namespace base::utils;
    using namespace base::lua;

    Luaer::Luaer()
    {

    }

    Luaer::~Luaer()
    {
        lua_close(L_);
    }

    void Luaer::Setup()
    {
        L_ = luaL_newstate();

        luaL_openlibs(L_);
        luaopen_utils(L_);
        luaopen_olibs(L_);
        luaopen_timer(L_);
        luaopen_net(L_);
        
        //lua_append_package_path(L_, framework.resource_dir() + "/ts-script");
        lua_getglobal(L_, "package");
        lua_getfield(L_, -1, "path");
        string path = lua_tostring(L_, -1);
        path.append(";" + framework.resource_dir() + "/ots-script/?.lua");
        cout << "path = " << path << endl;
        lua_pop(L_, 1);
        lua_pushstring(L_, path.c_str());
        lua_setfield(L_, -2, "path");
        lua_pop(L_, 1);
        
        //loadfiles
        string dir = framework.resource_dir() + "/ots-script";
        cout << "#On luaer Setup loadfiles " << dir << endl;
        std::vector<std::string> files = dir_get_files(dir.c_str(), 1);
        for (const string & file : files) {
            cout << file << endl;
            luaL_loadfile(L_, file.c_str());
            lua_pcall(L_, 0, 0, 0);
        }
    }

}
