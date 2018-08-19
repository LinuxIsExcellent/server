#ifndef BASE_CRON_TIMESTRUCT_H
#define BASE_CRON_TIMESTRUCT_H

#include "../global.h"
#include <iosfwd>

namespace base
{
    namespace cron
    {
        struct TimeStruct {
            TimeStruct() : hour(0), minute(0), second(0) {}
            TimeStruct(int32_t h, int32_t m, int32_t s = 0) : hour(h), minute(m), second(s) {
                CheckValue();
            }

            int32_t hour;
            int32_t minute;
            int32_t second;

            bool operator == (const TimeStruct& rhs) const {
                return hour == rhs.hour && minute == rhs.minute && second == rhs.second;
            }

            bool operator != (const TimeStruct& rhs) const {
                return !(*this == rhs);
            }

            bool operator > (const TimeStruct& rhs) const {
                return hour > rhs.hour
                       || (hour == rhs.hour && minute > rhs.minute)
                       || (hour == rhs.hour && minute == rhs.minute && second > rhs.second);
            }

            bool operator < (const TimeStruct& rhs) const {
                return hour < rhs.hour
                       || (hour == rhs.hour && minute < rhs.minute)
                       || (hour == rhs.hour && minute == rhs.minute && second < rhs.second);
            }

            TimeStruct operator - (const TimeStruct& rhs) const {
                if (rhs == *this) {
                    return TimeStruct();
                }
                if (*this > rhs) {
                    TimeStruct ret;
                    if (second >= rhs.second) {
                        ret.second = second - rhs.second;
                    } else {
                        ret.second = second + (60 - rhs.second);
                        ret.minute = -1;
                    }
                    if (minute >= rhs.minute) {
                        ret.minute += (minute - rhs.minute);
                    } else {
                        ret.minute += (minute + (60 - rhs.minute));
                        ret.hour = -1;
                    }
                    if (hour >= rhs.hour) {
                        ret.hour += (hour - rhs.hour);
                    } else {
                        ; // never execute
                    }
                    return ret;
                } else {
                    return rhs - *this;
                }
            }

            int64_t GetSeconds() const {
                return hour * 60 * 60 + minute * 60 + second;
            }

            void Parse(const char* data);

            std::string ToString(bool no_second = false) const;

        private:
            void CheckValue() {
                if (hour == 24 && minute == 0 && second == 0) {
                    ;
                } else {
                    if (hour > 23) {
                        hour = 23;
                    }
                    if (minute > 59) {
                        minute = 59;
                    }
                    if (second > 59) {
                        second = 59;
                    }
                }
            }
        };

        std::ostream& operator << (std::ostream& out, const TimeStruct& hm);
    }
}

#endif // TIMESTRUCT_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
