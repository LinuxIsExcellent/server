#ifndef BASE_CRON_CRONJOB_H
#define BASE_CRON_CRONJOB_H

#include "../global.h"
#include "../callback.h"
#include <vector>

namespace base
{
    namespace cron
    {
        struct CronJobInfo {
            std::string week;
            std::string open_time_config;
            uint32_t duration_seconds;
        };

        struct CronJobState {
            int64_t now;
            int64_t begin;
            int64_t duration;

            int64_t GetNextPointRemainSeconds() const {
                if (now < begin) {
                    return begin - now;
                } else {
                    int64_t end = begin + duration;
                    if (now < end) {
                        return end - now;
                    }
                }
                return 10;
            }

            int64_t GetLeftSeconds() const {
                if (in_progress()) {
                    return begin + duration - now;
                } else {
                    return begin - now;
                }
            }

            bool in_progress() const {
                return begin <= now && now <= begin + duration;
            }

            void Dump() const;
        };

        class CronJobImpl;

        class ModuleCronJob;

        class CronJob
        {
        public:
            static CronJob* instance() {
                return s_instance_;
            }

            void Add(const CronJobInfo& info, CallbackObserver<void(const CronJobState&)> cb);

        private:
            CronJob();
            ~CronJob();

            static CronJob* s_instance_;

            std::vector<CronJobImpl*> jobs_;

            friend class ModuleCronJob;
        };
    }
}

#endif // CRONJOB_H

// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
