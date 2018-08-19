#include "lua_module_utils.h"
#include "luaex.h"
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/document.h>
#include <lua/lua.hpp>
#include "../utils/crypto.h"
#include "../utils/utils_string.h"
#include "../utils/file.h"
#include "../configurehelper.h"
#include "../framework.h"
#include "../logger.h"
#include "../event/dispatcher.h"
#include "../dbo/dbpool.h"
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include <set>
#include <sys/statfs.h>
#include <sys/sysinfo.h>
#include <stdio.h>

namespace base
{
    extern uint32_t g_object_count;
    namespace lua
    {
        using namespace std;
        using namespace base::utils;

        static const size_t _convertStringBufferSize = 128;
        static char _convertStringBuffer[_convertStringBufferSize];

        static void _lua_to_json(lua_State* L, int idx, rapidjson::Writer<rapidjson::StringBuffer>& writer)
        {
            int type = lua_type(L, idx);
            switch (type) {
                case LUA_TNIL:
                    writer.Null();
                    break;
                case LUA_TBOOLEAN:
                    writer.Bool(lua_toboolean(L, idx));
                    break;
                case LUA_TSTRING:
                    writer.String(lua_tostring(L, idx));
                    break;
                case LUA_TNUMBER:
                    if (lua_isinteger(L, idx)) {
                        writer.Int64(lua_tointeger(L, idx));
                    } else {
                        writer.Double(lua_tonumber(L, idx));
                    }
                    break;
                case LUA_TTABLE: {
                    bool pureArray = true;
                    lua_pushnil(L);
                    while (lua_next(L, idx)) {
                        int keyType = lua_type(L, -2);
                        lua_pop(L, 1);
                        if (keyType == LUA_TSTRING) {
                            pureArray = false;
                            lua_pop(L, 1);
                            break;
                        }
                    }
                    if (pureArray) {
                        int len = lua_rawlen(L, idx);
                        writer.StartArray();
                        for (int i = 1; i <= len; ++i) {
                            lua_rawgeti(L, idx, i);
                            _lua_to_json(L, lua_gettop(L), writer);
                            lua_pop(L, 1);
                        }
                        writer.EndArray();
                    } else {
                        vector<const char*> keys;
                        writer.StartObject();
                        lua_pushnil(L);
                        while (lua_next(L, idx)) {
                            int keyType = lua_type(L, -2);
                            if (keyType == LUA_TSTRING) {
                                const char* key = lua_tostring(L, -2);
                                /* // without sort code
                                writer.String(key);
                                _lua_to_json(L, lua_gettop(L), writer);
                                */
                                keys.push_back(key);
                            } else if (keyType == LUA_TNUMBER) {
                                if (lua_isinteger(L, -2)) {
                                    snprintf(_convertStringBuffer, _convertStringBufferSize, "%lld", lua_tointeger(L, -2));
                                    writer.String(_convertStringBuffer);
                                } else {
                                    snprintf(_convertStringBuffer, _convertStringBufferSize, "%lf", lua_tonumber(L, -2));
                                    writer.String(_convertStringBuffer);
                                }
                                _lua_to_json(L, lua_gettop(L), writer);
                            }
                            lua_pop(L, 1);
                        }
                        sort(keys.begin(), keys.end(), [](const char * a, const char * b) {
                            return strcmp(b, a) > 0;
                        });
                        for (const char * key : keys) {
                            writer.String(key);
                            lua_pushstring(L, key);
                            lua_rawget(L, idx);
                            _lua_to_json(L, lua_gettop(L), writer);
                            lua_pop(L, 1);
                        }
                        writer.EndObject();
                    }
                }
                break;
                default:
                    writer.Null();
                    break;
            }
        }
        static int _lua_utils_toJson(lua_State* L)
        {
            luaL_checktype(L, 1, LUA_TTABLE);
            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
            _lua_to_json(L, 1, writer);
            lua_pushlstring(L, sb.GetString(), sb.GetSize());
            return 1;
        }

        string lua_utils_toJson(lua_State* L, int idx)
        {
            luaL_checktype(L, idx, LUA_TTABLE);
            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
            _lua_to_json(L, idx, writer);
            string ret(sb.GetString(), sb.GetSize());
            return ret;
        }

        static int _lua_utils_fromJson(lua_State* L)
        {
            const char* str = luaL_checkstring(L, 1);
            lua_utils_fromJson(L, str);
            return 1;
        }

        static void _write_json_value_to_lua(lua_State* L, const rapidjson::Value& jsonValue)
        {
            switch (jsonValue.GetType()) {
                case rapidjson::Type::kFalseType:
                case rapidjson::Type::kTrueType:
                    lua_pushboolean(L, jsonValue.GetBool());
                    break;
                case rapidjson::Type::kNullType:
                    lua_pushnil(L);
                    break;
                case rapidjson::Type::kNumberType:
                    if (jsonValue.IsInt()) {
                        lua_pushinteger(L, jsonValue.GetInt());
                    } else if (jsonValue.IsInt64()) {
                        lua_pushinteger(L, jsonValue.GetInt64());
                    } else if (jsonValue.IsUint()) {
                        lua_pushinteger(L, jsonValue.GetUint());
                    } else if (jsonValue.IsUint64()) {
                        lua_pushinteger(L, jsonValue.GetUint64());
                    } else {
                        lua_pushnumber(L, jsonValue.GetDouble());
                    }
                    break;
                case rapidjson::Type::kStringType:
                    lua_pushlstring(L, jsonValue.GetString(), jsonValue.GetStringLength());
                    break;
                case rapidjson::Type::kObjectType:
                    lua_newtable(L);
                    for (auto it = jsonValue.MemberBegin(); it != jsonValue.MemberEnd(); ++it) {
                        _write_json_value_to_lua(L, it->name);
                        _write_json_value_to_lua(L, it->value);
                        lua_rawset(L, -3);
                    }
                    break;
                case rapidjson::Type::kArrayType:
                    int i = 1;
                    lua_newtable(L);
                    for (auto it = jsonValue.Begin(); it != jsonValue.End(); ++it) {
                        _write_json_value_to_lua(L, *it);
                        lua_rawseti(L, -2, i);
                        ++i;
                    }
                    break;
            }
        }

        void lua_utils_fromJson(lua_State* L, const char* json)
        {
            rapidjson::Document doc;
            try {
                doc.Parse<0>(json);
                _write_json_value_to_lua(L, doc);
            } catch (Exception& ex) {
                lua_pushnil(L);
            }
        }

        static int _lua_utils_toCompressedJson(lua_State* L)
        {
            luaL_checktype(L, 1, LUA_TTABLE);
            rapidjson::StringBuffer sb;
            rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
            _lua_to_json(L, 1, writer);
            string data = base::utils::zlib_compress(sb.GetString(), sb.GetSize());
            string hash = base::utils::sha1hex(sb.GetString(), sb.GetSize());
            lua_pushlstring(L, data.c_str(), data.length());
            lua_pushlstring(L, hash.c_str(), hash.length());
            return 2;
        }

        static int _lua_utils_sha1(lua_State* L)
        {
            size_t len = 0;
            const char* plain = luaL_checklstring(L, 1, &len);
            lua_pushstring(L, base::utils::sha1hex(plain, len).c_str());

            return 1;
        }

        static int _lua_utils_md5(lua_State* L)
        {
            size_t len = 0;
            const char* plain = luaL_checklstring(L, 1, &len);
            lua_pushstring(L, base::utils::md5hex(plain, len).c_str());

            return 1;
        }

        static int _lua_utils_base64Encode(lua_State* L)
        {
            std::string plain = luaL_checkstring(L, 1);
            lua_pushstring(L, base::utils::base64_encode(plain).c_str());
            return 1;
        }

        static int _lua_utils_base64Decode(lua_State* L)
        {
            std::string plain = luaL_checkstring(L, 1);
            lua_pushstring(L, base::utils::base64_decode(plain).c_str());
            return 1;
        }

        static int _lua_utils_compress(lua_State* L)
        {
            int n = lua_gettop(L);
            size_t len = 0;
            const char* data = luaL_checklstring(L, 1, &len);
            if (n == 1) {
                lua_pushstring(L, base::utils::zlib_compress(data, len).c_str());
            } else {
                int level = luaL_checkinteger(L, 2);
                lua_pushstring(L, base::utils::zlib_compress(data, len, level).c_str());
            }
            return 1;
        }

        static int _lua_utils_uncompress(lua_State* L)
        {
            size_t len = 0;
            const char* data = luaL_checklstring(L, 1, &len);
            lua_pushstring(L, base::utils::zlib_uncompress(data, len).c_str());
            return 1;
        }

        static int _lua_utils_getResourceDir(lua_State* L)
        {
            const string& resDir = framework.resource_dir();
            lua_pushlstring(L, resDir.c_str(), resDir.length());
            return 1;
        }

        static int _lua_utils_listFilesInDirectory(lua_State* L)
        {
            const char* dir = luaL_checkstring(L, 1);
            uint16_t depth = luaL_optinteger(L, 2, 1);
            vector<string> files = utils::dir_get_files(dir, depth);
            lua_newtable(L);
            int i = 1;
            for (const string & f : files) {
                lua_pushstring(L, f.c_str());
                lua_rawseti(L, -2, i);
                ++i;
            }
            return 1;
        }

        static int _lua_utils_test(lua_State* L)
        {
            return 0;
        }

        static int _lua_utils_dumpStack(lua_State* L)
        {
            return luaEx_dump_stack(L);
        }

        static string serialize(lua_State* L, int idx)
        {
            string str;
            lua_pushvalue(L, idx);
            int type = lua_type(L, -1);
            switch (type) {
                case LUA_TNUMBER:
                    if (lua_isinteger(L, -1)) {
                        str = to_string(lua_tointeger(L, -1));
                    } else {
                        str = to_string(lua_tonumber(L, -1));
                    }
                    break;
                case LUA_TSTRING:
                    str.append("\"");
                    str += lua_tostring(L, -1);
                    str.append("\"");
                    break;
                case LUA_TBOOLEAN:
                    str = lua_toboolean(L, -1) ? "true" : "false";
                    break;
                case LUA_TTABLE: {
                    str += "{";
                    lua_pushnil(L);
                    while (lua_next(L, -2)) {
                        str += "[" + serialize(L, -2) + "]=";
                        str += serialize(L, -1) + ",";
                        lua_pop(L, 1);
                    }
                    str += "}";
                }
                break;
                default:
                    break;
            }
            lua_pop(L, 1);
            return str;
        }

        static int _lua_utils_serialize(lua_State* L)
        {
            string str = serialize(L, 1);
            lua_pushstring(L, str.c_str());
            return 1;
        }

        static int _lua_utils_countTable(lua_State* L)
        {
            size_t count = 0;
            luaL_checktype(L, 1, LUA_TTABLE);
            lua_pushnil(L);
            while (lua_next(L, 1) != 0) {
                ++count;
                lua_pop(L, 1);
            }
            lua_pushinteger(L, count);
            return 1;
        }

        static int _lua_utils_getRandomNum(lua_State* L)
        {
            int min = luaL_checkinteger(L, 1);
            int max = min;
            if (lua_isinteger(L, 2)) {
                max = luaL_checkinteger(L, 2);
                lua_pushinteger(L, framework.random().GenRandomNum(min, max));
            } else {
                lua_pushinteger(L, framework.random().GenRandomNum(max));
            }
            return 1;
        }

        static int _lua_utils_log(lua_State* L)
        {
            const char* txt = luaL_checkstring(L, 1);
            LOG_ERROR(txt);
            return 0;
        }

        static int _lua_utils_debug(lua_State* L)
        {
            const char* txt = luaL_checkstring(L, 1);
            LOG_DEBUG(txt);
            return 0;
        }

        /* ===========   game utils   ========== */

        static int _lua_utils_createUid(lua_State* L)
        {
            int64_t sid = luaL_checkinteger(L, 1);
            int64_t uid = luaL_checkinteger(L, 2);
            int64_t newUid = sid << 32 | uid;
            lua_pushinteger(L, newUid);
            return 1;
        }
        static int _lua_utils_getServerId(lua_State* L)
        {
            int64_t uid = luaL_checkinteger(L, 1);
            int64_t sid = uid >> 32;
            lua_pushinteger(L, sid);
            return 1;
        }
        static int _getMapServiceId()
        {
            static int id = -1;
            if (id == -1) {
                do {
                    base::ConfigureHelper helper;
                    tinyxml2::XMLElement* msNode = helper.fetchConfigNodeWithServerRule("ms", 0);
                    if (msNode == nullptr) {
                        break;
                    }
                    tinyxml2::XMLElement* mapServiceNode = helper.FirstChildElementWithPath(msNode, "mapService");
                    if (mapServiceNode == nullptr) {
                        break;
                    }
                    id = mapServiceNode->IntAttribute("id");
                } while (0);
            }
            return id;
        }
        static int _lua_utils_getMapServiceId(lua_State* L)
        {
            int sid = _getMapServiceId();
            lua_pushinteger(L, sid);
            return 1;
        }
        static int _lua_utils_createItemId(lua_State* L)
        {
            static int MAX_SEQUENCE = 256; // 2 ^ 8,
            static int MAX_SEGMENT = 8 + 1; // front-server (1 ~ 8) max 8 + 1 center-server
            //static int MAX_SERVERID = 16384; // max 16384 2 ^ 14
            static int START_SEQUENCE = 0;
            static int64_t current_tick = 0;
            static int sequence = 0;

            int server_id = _getMapServiceId();
            int node_id = framework.server_rule().id();
            if (node_id == 0) {
                node_id = 9; // for center-server
            }

            // init
            if (current_tick == 0) {
                current_tick = g_dispatcher->GetTickCache();
                MAX_SEQUENCE = (int)floor(MAX_SEQUENCE / MAX_SEGMENT);
                START_SEQUENCE = MAX_SEQUENCE * (node_id - 1);
                //std::cout << "START_SEQUENCE=" << START_SEQUENCE << std::endl;
            }

            ++sequence;
            if (sequence > MAX_SEQUENCE) {
                sequence = 1;
                ++current_tick;
            }
            //std::cout << "current_tick=" << current_tick << " sequence=" << sequence << std::endl;

            int64_t true_sequence = START_SEQUENCE + sequence;
            int64_t itemid = (int64_t)current_tick << 22 | (int64_t)server_id << 8 | true_sequence;

            lua_pushinteger(L, itemid);
            return 1;
        }

        static int _lua_utils_get_server_stat(lua_State* L)
        {
            uint64_t memTotal = 0, memFree = 0; // kB
            uint64_t diskFree = 0, diskTotal = 0;
            float cpuUsage = .0f;

            /* get system wide memory usage */
            {
                int64_t free = 0, buffers = 0, cached = 0; // kB
                set<string> paths;
                ifstream infile;
                string line;
                infile.open("/proc/meminfo");
                if (infile) {
                    while (!infile.eof()) {
                        char key[100] = {0};
                        int64_t size = 0;
                        getline(infile, line);
                        //cout << line << endl;
                        sscanf(line.c_str(), "%s%ld", key, &size);
                        //cout << key << " " << size << endl;
                        if (!strcmp(key, "MemTotal:")) {
                            memTotal = size;
                        } else if (!strcmp(key, "MemFree:")) {
                            free = size;
                        } else if (!strcmp(key, "Buffers:")) {
                            buffers = size;
                        } else if (!strcmp(key, "Cached:")) {
                            cached = size;
                            break;
                        }
                    }
                    memFree = free + buffers + cached;
                    //printf("memTotal=%lld,memFree=%lld,free=%lld,buffers=%lld,cached=%lld\n", memTotal, memFree, free, buffers, cached);
                }
                infile.close();
            }

            /* disk usage */
            {
                set<string> paths;
                ifstream infile;
                string line;
                infile.open("/proc/mounts");
                if (infile) {
                    while (!infile.eof()) {
                        char fileSystem[100] = {0}, path[100] = {0};
                        getline(infile, line);
                        //cout << line << endl;
                        sscanf(line.c_str(), "%s%s", fileSystem, path);
                        //cout << fileSystem << " " << path << endl;
                        if (strcmp(path, "")) {
                            paths.emplace(path);
                        }
                    }
                }
                infile.close();

                struct statfs diskInfo = {0};
                for (const string& path : paths) {
                    //cout << "path = " << path << endl;
                    statfs(path.c_str(), &diskInfo);
                    diskFree += (uint64_t)diskInfo.f_bfree * (uint64_t)diskInfo.f_bsize;

                    diskTotal += (uint64_t)diskInfo.f_blocks * (uint64_t)diskInfo.f_bsize;
                }
                // printf("mbTotalDisk=%dMB, mbFreeDisk=%dMB\n", totalDisk, freeDisk);
            }

            /* cpu usage */
            {
                FILE* fp = nullptr;
                char buf[128] = {0};
                char cpu[5] = {0};
                uint64_t user = 0, nice = 0, sys = 0, idle = 0, iowait = 0, irq = 0, softirq = 0;

                uint64_t all1 = 0, all2 = 0, idle1 = 0, idle2 = 0;

                fp = fopen("/proc/stat", "r");
                if (fp) {
                    if (!fgets(buf, sizeof(buf), fp)) {
                        sscanf(buf, "%s%ld%ld%ld%ld%ld%ld%ld", cpu, &user, &nice, &sys, &idle, &iowait, &irq, &softirq);
                    }
                    all1 = user + nice + sys + idle + iowait + irq + softirq;
                    idle1 = idle;
                    rewind(fp);

                    usleep(500000);

                    memset(buf, 0, sizeof(buf));
                    cpu[0] = '\0';
                    user = nice = sys = idle = iowait = irq = softirq = 0;
                    if (fgets(buf, sizeof(buf), fp)) {
                        sscanf(buf, "%s%ld%ld%ld%ld%ld%ld%ld", cpu, &user, &nice, &sys, &idle, &iowait, &irq, &softirq);
                    }
                    all2 = user + nice + sys + idle + iowait + irq + softirq;
                    idle2 = idle;

                    cpuUsage = (float)(all2 - all1 - (idle2 - idle1)) / (all2 - all1) * 100;

                    // printf("cpu use = %.2f\%\n", cpuUsage);
                }
                fclose(fp);
            }

            lua_pushinteger(L, memTotal);
            lua_pushinteger(L, memFree);
            lua_pushinteger(L, diskTotal);
            lua_pushinteger(L, diskTotal - diskFree);
            lua_pushnumber(L, cpuUsage);
            return 5;
        }

        static int _lua_utils_getSqlCountsJson(lua_State* L)
        {
            string json = base::dbo::g_dbpool->SqlCountsToJson();
            lua_pushstring(L, json.c_str());
            return 1;
        }

        static int _lua_utils_getGObjectCount(lua_State* L)
        {
            lua_pushinteger(L, g_object_count);
            return 1;
        }

        static int _lua_utils_getObjectCountsJson(lua_State* L)
        {
            string json = gObjectTracker->SqlCountsToJson();
            lua_pushstring(L, json.c_str());
            return 1;
        }

        static int _lua_open_utils(lua_State* L)
        {
            luaL_Reg libFuns[] = {
                {"toJson", _lua_utils_toJson},
                {"fromJson", _lua_utils_fromJson},
                {"toCompressedJson", _lua_utils_toCompressedJson},
                {"sha1", _lua_utils_sha1},
                {"md5", _lua_utils_md5},
                {"base64Encode", _lua_utils_base64Encode},
                {"base64Decode", _lua_utils_base64Decode},
                {"compress", _lua_utils_compress},
                {"uncompress", _lua_utils_uncompress},
                {"getResourceDir", _lua_utils_getResourceDir},
                {"listFilesInDirectory", _lua_utils_listFilesInDirectory},
                {"test", _lua_utils_test},
                {"dumpStack", _lua_utils_dumpStack},
                {"serialize", _lua_utils_serialize},
                {"countTable", _lua_utils_countTable},
                {"getRandomNum", _lua_utils_getRandomNum},
                {"log", _lua_utils_log},
                {"debug", _lua_utils_debug},
                // game utils
                {"createUid", _lua_utils_createUid},
                {"getServerId", _lua_utils_getServerId},
                {"getMapServiceId", _lua_utils_getMapServiceId},
                {"createItemId", _lua_utils_createItemId},
                {"getServerStat", _lua_utils_get_server_stat},
                {"getSqlCountsJson", _lua_utils_getSqlCountsJson},
                {"getGObjectCount", _lua_utils_getGObjectCount},
                {"getObjectCountsJson", _lua_utils_getObjectCountsJson},
                {nullptr, nullptr}
            };

            luaL_newlib(L, libFuns);
            return 1;
        }

        void luaopen_utils(lua_State* L)
        {
            luaL_requiref(L, "utils", &_lua_open_utils, 0);
            lua_pop(L, 1);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
