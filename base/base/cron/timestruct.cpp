#include "timestruct.h"
#include "../utils/utils_string.h"
#include <ctime>
#include <cstdlib>
#include <ostream>

namespace base
{
    namespace cron
    {
        using namespace std;

        void TimeStruct::Parse(const char* data)
        {
            sscanf(data, "%d:%d:%d", &hour, &minute, &second);
            CheckValue();
        }

        string TimeStruct::ToString(bool no_second) const
        {
            string ret;
            if (no_second) {
                base::utils::string_append_format(ret, "%02d:%02d", hour, minute);
            } else {
                base::utils::string_append_format(ret, "%02d:%02d:%02d", hour, minute, second);
            }
            return ret;
        }

        std::ostream& operator << (std::ostream& out, const TimeStruct& hm)
        {
            out << hm.hour << ":" << hm.minute << ":" << hm.second;
            return out;
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
