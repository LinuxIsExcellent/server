#include "utils_string.h"
#include <cxxabi.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <zlib.h>

namespace base
{
    namespace utils
    {
        using namespace std;

        string demangle_name(const type_info& typeInfo)
        {
            string ret;
            int status = 0;
            char* realname = abi::__cxa_demangle(typeInfo.name(), 0, 0, &status);
            if (realname != NULL) {
                ret = realname;
                free(realname);
            } else {
                ret = typeInfo.name();
            }
            return ret;
        }

        void string_path_append(string& path1, const string& path2)
        {
            if (!path1.empty() && !path2.empty()) {
                if (path1.back() == '/' && path2.front() == '/') {
                    path1.pop_back();
                }
            }
            if (!path1.empty() && path1.back() != '/' && !path2.empty() && path2.front() != '/') {
                path1.push_back('/');
            }
            path1.append(path2);
        }

        string string_join(const vector< string >& str_arr, char sep)
        {
            string ret;
            if (!str_arr.empty()) {
                for (const string & s : str_arr) {
                    ret.append(s);
                    ret.push_back(sep);
                }
            }
            ret.pop_back();
            return ret;
        }

        string string_join(const vector< int32_t >& int_arr, char sep)
        {
            string ret;
            if (!int_arr.empty()) {
                for (int32_t i : int_arr) {
                    ret.append(std::to_string(i));
                    ret.push_back(sep);
                }
                ret.pop_back();
            }
            return ret;
        }

        string string_lr_trim(const char* str)
        {
            string ret;

            const char* begin = str;
            while (*begin) {
                if (isblank(*begin)) {
                    ++begin;
                } else {
                    break;
                }
            }

            const char* end = str;
            const char* p = str;
            while (*p) {
                if (!isblank(*p)) {
                    end = p;
                }
                ++p;
            }

            for (p = begin; p <= end; ++p) {
                ret.push_back(*p);
            }
            return ret;
        }

        std::vector<string> string_split(const string& str , char separator , bool trim_empty)
        {
            std::vector<string> list;
            for (size_t pos = 0 ; pos < str.length(); ++pos) {
                size_t tag = str.find(separator , pos);
                string temp;
                if (tag != string::npos) {
                    temp = str.substr(pos , tag - pos);
                    pos = tag;
                } else {
                    temp = str.substr(pos);
                }
                if (!trim_empty || !temp.empty()) {
                    list.emplace_back(std::move(temp));
                }
                if (tag == string::npos) break;
            }
            return list;
        }

        vector< int32_t > string_split_int(const char* str, char sep)
        {
            vector<int32_t> ret;
            string tmp;
            const char* p = str;
            while (*p) {
                if (*p == sep) {
                    if (!tmp.empty()) {
                        ret.push_back(atoi(tmp.c_str()));
                    }
                    tmp.clear();
                } else {
                    tmp.push_back(*p);
                }
                ++p;
            }
            if (!tmp.empty()) {
                ret.push_back(atoi(tmp.c_str()));
            }
            return ret;
        }

        void string_append_format(std::string& str, const char* format, ...)
        {
            static char tmp[2048];
            va_list va;
            va_start(va, format);
            vsprintf(tmp, format, va);
            va_end(va);
            str.append(tmp);
        }

        string string_dump_binary(const string& src)
        {
            string ret;
            for (uint32_t i = 0; i < src.length(); ++i) {
                ret.append(base::utils::to_string((uint32_t)(uint8_t)src[i]));
                ret.append(",");
            }
            return ret;
        }

        size_t utf8strlen(const string& data)
        {
            size_t result = 0;
            mbstate_t s;
            memset(&s, 0, sizeof(s));
            //setlocale(LC_ALL, "en_US.utf8");
            const char* begin = data.data();
            const char* end = data.data() + data.length();
            size_t r = mbrlen(begin, end - begin,  &s);
            while (r > 0) {
                begin += r;
                ++result;
                r = mbrlen(begin, end - begin,  &s);
            }
            return result;
        }

        string zlib_compress(const char* data, size_t len, int level)
        {
            string result;
            result.resize(compressBound(len));
            uint8_t* dest = (uint8_t*)(const_cast<char*>(result.c_str()));
            size_t result_length = result.length();
            int r = compress2(dest, &result_length, (const uint8_t*)data, len, level);
            if (r != Z_OK) {
                result.clear();
            } else {
                result.resize(result_length);
            }
            return result;
        }

        string zlib_uncompress(const char* data, size_t len)
        {
            const int BLOCK = 2048;
            string result;
            result.resize(BLOCK);
            int real = 0;

            z_stream zstream;
            zstream.zalloc = Z_NULL;
            zstream.zfree = Z_NULL;
            zstream.opaque = Z_NULL;
            if (inflateInit2(&zstream, 15 + 32) != Z_OK) {
                return "";
            }
            zstream.next_in = (uint8_t*)data;
            zstream.avail_in = len;
            do {
                zstream.next_out = (uint8_t*)result.c_str() + real;
                zstream.avail_out = result.size() - real;
                int err = inflate(&zstream, Z_NO_FLUSH);
                if (err != Z_OK && err != Z_STREAM_END) {
                    return "";
                }
                int have = result.size() - zstream.avail_out - real;
                real += have;
                if (zstream.total_in < len) {
                    result.resize(result.size() + BLOCK);
                }
            } while (zstream.avail_out == 0);
            result.resize(real);
            inflateEnd(&zstream);
            return result;
        }

        static char __convert_buff[256];

        std::string convert(float data)
        {
            snprintf(__convert_buff, 256, "%g", data);
            return __convert_buff;
        }

        std::string convert(double data)
        {
            snprintf(__convert_buff, 256, "%g", data);
            return __convert_buff;
        }

        string urldecode(const char* cd, size_t len)
        {
            char p[2];
            string decd;
            for (size_t i = 0; i < len; i++) {
                memset(p, 0, 2);
                if (cd[i] != '%') {
                    decd.push_back(cd[i]);
                    continue;
                }

                p[0] = cd[++i];
                p[1] = cd[++i];

                p[0] = p[0] - 48 - ((p[0] >= 'A') ? 7 : 0) - ((p[0] >= 'a') ? 32 : 0);
                p[1] = p[1] - 48 - ((p[1] >= 'A') ? 7 : 0) - ((p[1] >= 'a') ? 32 : 0);
                decd.push_back(p[0] * 16 + p[1]);
            }
            return decd;
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
