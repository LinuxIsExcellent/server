#include "lua_module_timer.h"
#include "luaex.h"
#include "luacallbackmgr.h"
#include "../framework.h"
#include <lua/lua.hpp>
#include "../event/dispatcher.h"
#include "../logger.h"
#include <functional>
#include "../utils/utils_time.h"

namespace base
{
    namespace lua
    {
        using namespace std;
        using namespace base::utils;

        static const char* MT_TIMER = "mt_timer";
        static const char TIMER_LIST_ADDR = 'c';

        class LuaTimer
        {
        public:
            LuaTimer() {}
            ~LuaTimer() {
                m_autoObserver.SetNotExist();
            }

            void StoreLuaFunction(lua_State* L, int udIdx, int funIdx) {
                lua_rawgetp(L, LUA_REGISTRYINDEX, &TIMER_LIST_ADDR);
                lua_pushvalue(L, udIdx);
                lua_rawsetp(L, -2, this);
                lua_pop(L, 1);
                lua_pushvalue(L, funIdx);
                lua_setuservalue(L, udIdx);
            }

            void TimeoutCallback(lua_State* L) {
                StackKeeper sk(L);
                lua_rawgetp(L, LUA_REGISTRYINDEX, &TIMER_LIST_ADDR);
                lua_rawgetp(L, -1, this);
                if (lua_isuserdata(L, -1)) {
                    lua_getuservalue(L, -1);
                    if (lua_isfunction(L, -1)) {
                        int err = lua_pcall(L, 0, 0, 0);
                        if (err) {
                            LOG_ERROR("[%s] exec fail: %s\n", "LuaTimer::TimeoutCallback", lua_tostring(L, -1));
                        }
                    }
                }
            }

            const base::AutoObserver& GetAutoObserver() const {
                return m_autoObserver;
            }

            void Cancel(lua_State* L) {
                m_autoObserver.SetNotExist();
            }

            void Clear(lua_State* L) {
                lua_rawgetp(L, LUA_REGISTRYINDEX, &TIMER_LIST_ADDR);
                lua_pushnil(L);
                lua_rawsetp(L, -2, this);
                lua_pop(L, 1);
            }

        private:
            base::AutoObserver m_autoObserver;
        };

        static int _lua_timer_cancel(lua_State* L)
        {
            LuaTimer* timer = (LuaTimer*)luaL_checkudata(L, 1, MT_TIMER);
            timer->Cancel(L);
            return 0;
        }

        static int _lua_timer___gc(lua_State* L)
        {
            LuaTimer* timer = (LuaTimer*)luaL_checkudata(L, 1, MT_TIMER);
            timer->Clear(L);
            timer->~LuaTimer();
            return 0;
        }

        static int _lua_setTimeout(lua_State* L)
        {
            luaL_checktype(L, 1, LUA_TFUNCTION);
            int64_t delay = luaL_checkinteger(L, 2);
            LuaTimer* timer = (LuaTimer*)lua_newuserdata(L, sizeof(LuaTimer));
            new(timer)LuaTimer();
            luaL_setmetatable(L, MT_TIMER);
            timer->StoreLuaFunction(L, lua_gettop(L), 1);
            lua_State* mainL = luaEx_get_main_thread(L);
            g_dispatcher->quicktimer().SetTimeout(std::bind(&LuaTimer::TimeoutCallback, timer, mainL), delay, timer->GetAutoObserver());
            return 1;
        }

        static int _lua_setInterval(lua_State* L)
        {
            luaL_checktype(L, 1, LUA_TFUNCTION);
            int64_t interval = luaL_checkinteger(L, 2);
            LuaTimer* timer = (LuaTimer*)lua_newuserdata(L, sizeof(LuaTimer));
            new(timer)LuaTimer();
            luaL_setmetatable(L, MT_TIMER);
            timer->StoreLuaFunction(L, lua_gettop(L), 1);
            lua_State* mainL = luaEx_get_main_thread(L);
            g_dispatcher->quicktimer().SetInterval(std::bind(&LuaTimer::TimeoutCallback, timer, mainL), interval, timer->GetAutoObserver());
            return 1;
        }

        static int _lua_getTimestampCache(lua_State* L)
        {
            lua_pushinteger(L, g_dispatcher->GetTimestampCache());
            return 1;
        }

        static int _lua_getTickCache(lua_State* L)
        {
            lua_pushinteger(L, g_dispatcher->GetTickCache());
            return 1;
        }

        static int _lua_sleep(lua_State* L)
        {
            {
                int millisecond = luaL_checkinteger(L, 1);
                int r = lua_pushthread(L);
                if (r == 1) {
                    return luaL_error(L, "cat not do sleep in main thread");
                }
                int callbackKey = g_lua_callback_mgr->Store(L);
                g_dispatcher->quicktimer().SetTimeout([callbackKey]() {
                    lua_State* mainL = NULL;
                    if (g_lua_callback_mgr->Fetch(callbackKey, &mainL, true)) {
                        lua_State* L = lua_tothread(mainL, -1);
                        luaEx_resume(L, mainL, 0);
                        lua_pop(mainL, 1);
                    }
                }, millisecond);
            }
            return lua_yield(L, 0);
        }

        static int _lua_is_today_from_timestamp(lua_State* L)
        {
            int64_t timestamp = luaL_checkinteger(L, 1);
            lua_pushboolean(L, is_today_from_timestamp(timestamp));
            return 1;
        }

        static int _is_ts_should_refresh_at_hour(lua_State* L)
        {
            int64_t timestamp = luaL_checkinteger(L, 1);
            int hour = luaL_checkinteger(L, 2);
            lua_pushboolean(L, is_ts_should_refresh_at_hour(timestamp, hour));
            return 1;
        }

        static int _today_zero_timestamp(lua_State* L)
        {
            lua_pushinteger(L, today_zero_timestamp());
            return 1;
        }
        static int _lua_utils_getZone(lua_State* L)
        {
            int16_t zone = base::utils::dis_time_zone();
            lua_pushinteger(L, zone);
            return 1;
        }

        static int _lua_utils_getIsdst(lua_State* L)
        {
            int32_t isdst = base::utils::is_dst();
            lua_pushinteger(L, isdst);
            return 1;
        }

        static int _lua_open_timer(lua_State* L)
        {
            // 用于保存定时器的回调函数
            lua_newtable(L);
            lua_pushstring(L, "list_timer");
            lua_setfield(L, -2, "collect_name");
            lua_pushvalue(L, -1);
            lua_pushstring(L, "__mode");
            lua_pushstring(L, "v");
            lua_rawset(L, -3);
            lua_setmetatable(L, -2);
            lua_rawsetp(L, LUA_REGISTRYINDEX, &TIMER_LIST_ADDR);

            // class timer
            luaL_Reg dbFuncs[] = {
                {"cancel", _lua_timer_cancel},
                {"__gc", _lua_timer___gc},
                {nullptr, nullptr},
            };

            int ok = luaL_newmetatable(L, MT_TIMER);
            assert(ok == 1);
            luaL_setfuncs(L, dbFuncs, 0);
            lua_pushstring(L, "__index");
            lua_pushvalue(L, -2);
            lua_rawset(L, -3);
            lua_pop(L, 1);

            luaL_Reg libFuns[] = {
                {"setTimeout", _lua_setTimeout},
                {"setInterval", _lua_setInterval},
                {"getTimestampCache", _lua_getTimestampCache},
                {"getTickCache", _lua_getTickCache},
                {"sleep", _lua_sleep},
                {"isTodayFromTimestamp", _lua_is_today_from_timestamp}, //TODO 关于夏令时的问题
                {"isTsShouldRefreshAtHour", _is_ts_should_refresh_at_hour},
                {"todayZeroTimestamp", _today_zero_timestamp},
                {"getZone", _lua_utils_getZone},
                {"getIsdst", _lua_utils_getIsdst},
                {nullptr, nullptr}
            };

            luaL_newlib(L, libFuns);
            return 1;
        }

        void luaopen_timer(lua_State* L)
        {
            luaL_requiref(L, "timer", &_lua_open_timer, 0);
            lua_pop(L, 1);
        }

    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
