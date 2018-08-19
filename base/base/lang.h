#ifndef BASE_LANG_H
#define BASE_LANG_H

#include "global.h"
#include <string>
#include <unordered_map>

namespace base
{
    class Lang
    {
    public:
        static Lang& instance() {
            static Lang ins;
            return ins;
        }

        const char* Query(const std::string& key) const {
            std::unordered_map<std::string, std::string>::const_iterator it = langs_.find(key);
            return it == langs_.end() ? key.c_str() : it->second.c_str();
        }

        void Setup(const std::string& res_dir);

    private:
        Lang();
        std::unordered_map<std::string, std::string> langs_;
    };
}

#define LANG(key) base::Lang::instance().Query(key)

#endif // LANG_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
