#ifndef BASE_UTILS_CRYPTO_H
#define BASE_UTILS_CRYPTO_H

#include <string>

namespace base
{
    namespace utils
    {
        std::string md5hex(const char* plain, size_t len);
        inline std::string md5hex(const std::string& plain)
        {
            return md5hex(plain.c_str(), plain.length());
        }

        std::string sha1hex(const char* plain, size_t len);
        inline std::string sha1hex(const std::string& plain)
        {
            return sha1hex(plain.c_str(), plain.length());
        }

        void sha1(const std::string& input, std::string& output);

        // base64 encode and decode
        // toke from http://stackoverflow.com/questions/5288076/doing-base64-encoding-and-decoding-in-openssl-c

        std::string base64_encode(const std::string& plain);

        std::string base64_decode(const std::string& cipher);
    }
}

#endif
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
