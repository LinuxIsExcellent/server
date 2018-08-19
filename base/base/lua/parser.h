#ifndef BASE_LUA_CONFIGUREPARSER_H
#define BASE_LUA_CONFIGUREPARSER_H

#include <vector>
#include "../data.h"

namespace base
{
    namespace lua
    {
        class Parser
        {
        public:
            static std::vector<int> ParseAsIntArray(const std::string& code, bool* hasError = nullptr);

            static std::vector<std::string> ParseAsStringArray(const std::string& code, bool* hasError = nullptr);

            static DataValue ParseAsDataValue(const std::string& code, bool* hasError = nullptr);

            static DataTable ParseAsDataTable(const std::string& code, bool* hasError = nullptr);

        };
    }
}


#endif // CONFIGUREPARSER_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
