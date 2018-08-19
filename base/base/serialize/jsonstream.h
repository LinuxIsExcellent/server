#ifndef BASE_SERIALIZE_JSONSTREAM_H
#define BASE_SERIALIZE_JSONSTREAM_H

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <string>

namespace base
{
    namespace serialize
    {
        typedef rapidjson::Writer<rapidjson::StringBuffer> json_ostream_t;
        typedef rapidjson::Value json_istream_t;

        inline json_ostream_t& operator << (json_ostream_t& out, bool v)
        {
            out.Bool(v);
            return out;
        }

        inline json_ostream_t& operator << (json_ostream_t& out, int8_t v)
        {
            out.Int(v);
            return out;
        }

        inline json_ostream_t& operator << (json_ostream_t& out, uint8_t v)
        {
            out.Uint(v);
            return out;
        }

        inline json_ostream_t& operator << (json_ostream_t& out, int16_t v)
        {
            out.Int(v);
            return out;
        }

        inline json_ostream_t& operator << (json_ostream_t& out, uint16_t v)
        {
            out.Uint(v);
            return out;
        }

        inline json_ostream_t& operator << (json_ostream_t& out, int32_t v)
        {
            out.Int(v);
            return out;
        }

        inline json_ostream_t& operator << (json_ostream_t& out, uint32_t v)
        {
            out.Uint(v);
            return out;
        }

        inline json_ostream_t& operator << (json_ostream_t& out, int64_t v)
        {
            out.Int64(v);
            return out;
        }

        inline json_ostream_t& operator << (json_ostream_t& out, uint64_t v)
        {
            out.Uint64(v);
            return out;
        }

        inline json_ostream_t& operator << (json_ostream_t& out, const std::string& v)
        {
            out.String(v.c_str(), v.length(), true);
            return out;
        }

        inline json_istream_t& operator >> (json_istream_t& in, bool& v)
        {
            v = in.GetBool();
            return in;
        }

        inline json_istream_t& operator >> (json_istream_t& in, int8_t& v)
        {
            v = in.GetInt();
            return in;
        }

        inline json_istream_t& operator >> (json_istream_t& in, uint8_t& v)
        {
            v = in.GetUint();
            return in;
        }

        inline json_istream_t& operator >> (json_istream_t& in, int16_t& v)
        {
            v = in.GetInt();
            return in;
        }

        inline json_istream_t& operator >> (json_istream_t& in, uint16_t& v)
        {
            v = in.GetUint();
            return in;
        }

        inline json_istream_t& operator >> (json_istream_t& in, int32_t& v)
        {
            v = in.GetInt();
            return in;
        }

        inline json_istream_t& operator >> (json_istream_t& in, uint32_t& v)
        {
            v = in.GetUint();
            return in;
        }

        inline json_istream_t& operator >> (json_istream_t& in, int64_t& v)
        {
            v = in.GetInt64();
            return in;
        }

        inline json_istream_t& operator >> (json_istream_t& in, uint64_t& v)
        {
            v = in.GetUint64();
            return in;
        }

        inline json_istream_t& operator >> (json_istream_t& in, std::string& v)
        {
            v = in.GetString();
            return in;
        }
    }
}

#endif // JSONSTREAM_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
