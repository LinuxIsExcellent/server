#ifndef MAP_LUA_WRITE_H
#define MAP_LUA_WRITE_H
#include <base/cluster/message.h>
#include <base/data.h>

namespace ms
{
    namespace map
    {
        using namespace base::cluster;

        static void lua_write(MessageOut& msgout, bool v)
        {
            msgout << (int)base::ValueType::BOOLEAN;
            msgout << v;
        }

        static void lua_write(MessageOut& msgout, int v)
        {
            msgout << (int)base::ValueType::INTEGER;
            msgout << v;
        }

        static void lua_write(MessageOut& msgout, int64_t v)
        {
            msgout << (int)base::ValueType::INTEGER;
            msgout << v;
        }

        static void lua_write(MessageOut& msgout, double v)
        {
            msgout << (int)base::ValueType::DOUBLE;
            msgout << v;
        }

        static void lua_write(MessageOut& msgout, const char* v)
        {
            msgout << (int)base::ValueType::STRING;
            msgout << v;
        }

        static void lua_write(MessageOut& msgout, const std::string& v)
        {
            msgout << (int)base::ValueType::STRING;
            msgout << v;
        }

        static void lua_write(MessageOut& msgout, float v)
        {
            lua_write(msgout, (double)v);
        }

        template<typename T>
        static void lua_write(MessageOut& msgout, const std::vector<T>& v)
        {
            msgout << (int)base::ValueType::TABLE;
            msgout << v.size();
            for (int i = 0; i < v.size(); ++i) {
                lua_write(msgout, i + 1);
                lua_write(msgout, v[i]);
            }
        }

        template<typename T>
        static void lua_write(MessageOut& msgout, const std::list<T>& v)
        {
            msgout << (int)base::ValueType::TABLE;
            msgout << v.size();
            int i = 0;
            for (auto it = v.begin(); it != v.end(); ++it) {
                lua_write(msgout, ++i);
                lua_write(msgout, *it);
            }
        }

        template<typename T1, typename T2>
        static void lua_write(MessageOut& msgout, const std::unordered_map<T1, T2>& v)
        {
            msgout << (int)base::ValueType::TABLE;
            msgout << v.size();
            for (auto it = v.begin(); it != v.end(); ++it) {
                lua_write(msgout, it->first);
                lua_write(msgout, it->second);
            }
        }

        static void lua_write(MessageOut& msgout, const base::DataTable& v)
        {
            msgout << (int)base::ValueType::TABLE;
            msgout << v;
        }

        template<typename T>
        static void write_msgout(base::cluster::MessageOut& msgout, const T& t)
        {
            lua_write(msgout, t);
        }

        template<typename T, typename ... Args>
        static void write_msgout(base::cluster::MessageOut& msgout, const T& t, Args ...args)
        {
            lua_write(msgout, t);
            write_msgout(msgout, args...);
        }
    }
}
#endif //MAP_LUA_WRITE_H
