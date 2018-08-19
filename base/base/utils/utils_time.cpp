#include "utils_time.h"
#include "../event/dispatcher.h"

namespace base
{
    namespace utils
    {
        bool is_ts_should_refresh_at_hour(int64_t ts, uint32_t hour)
        {
            int64_t now_ts = g_dispatcher->GetTimestampCache();
            int64_t today_pot = get_date_timestamp_at_hour(now_ts, hour);
            int64_t ts_pot = get_date_timestamp_at_hour(ts, hour);

            if (ts_pot < today_pot) {
                if (now_ts > today_pot) {
                    return true;
                } else if (ts < today_pot - SECONDS_OF_ONE_DAY) {
                    return true;
                }
            } else if (ts_pot == today_pot) {
                if (ts < today_pot && now_ts > today_pot) {
                    return true;
                }
            }
            return false;
        }

        int64_t get_date_timestamp_at_hour(int64_t timestamp, uint32_t hour)
        {
            int64_t today = (timestamp - timezone) / SECONDS_OF_ONE_DAY;
            return today * SECONDS_OF_ONE_DAY + hour * SECONDS_OF_ONE_HOUR + timezone;
        }

    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
