#ifndef BASE_UTILS_TIME_H
#define BASE_UTILS_TIME_H

#include <time.h>
#include <stdint.h>
#include <string>
#ifdef __APPLE__
#include <sys/time.h>
#endif

namespace base
{
    namespace utils
    {
        enum WeekDay {
            SUNDAY = 0,
            MONDAY = 1,
            TUESDAY = 2,
            WENDESDAY = 3,
            THURSDAY = 4,
            FRIDAY = 5 ,
            SATURDAY = 6,
        };

        static const int64_t SECONDS_OF_ONE_HOUR = 3600;
        static const int64_t TICKS_OF_ONE_HOUR = SECONDS_OF_ONE_HOUR * 1000;
        static const int64_t SECONDS_OF_ONE_DAY = SECONDS_OF_ONE_HOUR * 24;
        static const int64_t TICKS_OF_ONE_DAY = TICKS_OF_ONE_HOUR * 24;

        inline int64_t nowtick()
        {
#ifdef __APPLE__
            timeval now;
            gettimeofday(&now, NULL);
            return now.tv_sec * 1000 + now.tv_usec / 1000;
#else
            struct timespec now;
            clock_gettime(CLOCK_REALTIME, &now);
            return now.tv_sec * 1000 + now.tv_nsec / 1000000;
#endif
        }

        // 时间戳，单位为秒 (since epoch)
        inline int64_t timestamp()
        {
            return time(NULL);
        }

        //当前时区和标准时区差  如当前是北京时间(东八区) 则结果为8
        inline int16_t dis_time_zone()
        {
            return (int16_t) - (timezone / SECONDS_OF_ONE_HOUR) ;
        }

        inline int days_since_epoch()
        {
            return timestamp() / (24 * 60 * 60);
        }

        inline int32_t is_dst(time_t tick = 0)
        {
            if (tick == 0) {
                tick = time(NULL);
            }
            tm* p = localtime(&tick);
            return p->tm_isdst;
        }

        bool is_ts_should_refresh_at_hour(int64_t ts, uint32_t hour);

        // 获取指定时间戳所在的天数的某个指定的小时数的timestamp
        // today_hour 小时
        int64_t get_date_timestamp_at_hour(int64_t timestamp, uint32_t hour);

        // 检查指定时间是否为
        inline int64_t is_today_from_timestamp(int64_t timestamp)
        {
            int64_t nowts = time(NULL);
            return (nowts - timezone) / SECONDS_OF_ONE_DAY == (timestamp - timezone) / SECONDS_OF_ONE_DAY;
        }

        // 获取当天0点时间截
        // timestamp 单位秒
        inline int64_t today_zero_timestamp()
        {
            int64_t nowts = time(NULL);
            int64_t today = (nowts - timezone) / SECONDS_OF_ONE_DAY;
            return today * SECONDS_OF_ONE_DAY + timezone;
        }

        // 根据时间戳，获取星期，时间戳单位为毫秒
        inline WeekDay get_weekday(int64_t tick)
        {
            time_t tick_t = tick / 1000;
            tm* p = localtime(&tick_t);
            WeekDay result = (WeekDay)p->tm_wday;
            p = NULL;
            return result;
        }

        // 获取当前时间为星期几
        inline WeekDay now_weekday()
        {
            return get_weekday(nowtick());
        }

        // 根据时间戳 获取分钟，时间戳单位为毫秒
        inline int get_minute(int64_t tick)
        {
            time_t tick_t = tick / 1000;
            tm* p = localtime(&tick_t);
            int result = p->tm_min;
            p = NULL;
            return result;
        }

        // 获取当前时间为第几分钟
        inline int now_minute()
        {
            return get_minute(nowtick());
        }

        inline int64_t week_zero_timestamp()
        {
            uint8_t result;
            switch (now_weekday()) {
                case MONDAY:
                    result = 0;
                    break;
                case TUESDAY:
                    result = 1;
                    break;
                case WENDESDAY:
                    result = 2;
                    break;
                case THURSDAY:
                    result = 3;
                    break;
                case FRIDAY:
                    result = 4;
                    break;
                case SATURDAY:
                    result = 5;
                    break;
                case SUNDAY:
                    result = 6;
                    break;
            }
            return today_zero_timestamp() - result * SECONDS_OF_ONE_DAY;
        }

        inline const char* format_time_str(time_t tick = 0)
        {
            if (tick == 0) {
                tick = time(NULL);
            }

            static char buff[32];
            strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", localtime(&tick));
            return buff;
        }
    }
}

#endif



// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
