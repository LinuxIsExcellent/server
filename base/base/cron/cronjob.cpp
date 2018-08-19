#include "cronjob.h"
#include "../event/dispatcher.h"
#include "../utils/utils_string.h"
#include "timestruct.h"
#include "../utils/utils_time.h"
#include <ctime>
#include <iostream>
#include <algorithm>

namespace base
{
    namespace cron
    {
        using namespace std;

        /// CronJobState
        void CronJobState::Dump() const
        {
            cout << "job state:" << endl;
            cout << "now = " << now << "  " << ctime((time_t*)&now);
            cout << "begin = " << begin << "  " << ctime((time_t*)&begin);
            int64_t end = now + GetLeftSeconds();
            cout << "next_trigger = " << ctime((time_t*)&end);
            cout << "in_progress = " << boolalpha << in_progress() << endl;
            cout << "left_seconds = " << GetLeftSeconds() << endl;
        }

        /// CronJobInfoImpl
        class CronJobImpl
        {
        public:
            CronJobImpl(const CronJobInfo& raw_info, CallbackObserver<void(const CronJobState&)> cb)
                : raw_info_(raw_info), cb_(cb) {
            }

            uint32_t id() const {
                return id_;
            }

            bool IsExist() const {
                return cb_.IsExist();
            }

            bool Parse() {
                vector<string> weeks = base::utils::string_split(raw_info_.week, ',');
                for (const string & w : weeks) {
                    open_weeks_.push_back(atoi(w.c_str()));
                }
                sort(open_weeks_.begin(), open_weeks_.end());
                start_time_.Parse(raw_info_.open_time_config.c_str());
                duration_seconds_ = raw_info_.duration_seconds;
                return true;
            }

            void Update() {
                int64_t now_ts = g_dispatcher->GetTimestampCache();

                tm now_t;
                localtime_r((time_t*)&now_ts, &now_t);
                int32_t wday = now_t.tm_wday;   // 星期几
                if (wday == 0) {
                    wday = 7;
                }
                TimeStruct now_hms(now_t.tm_hour, now_t.tm_min, now_t.tm_sec);

                int64_t today_zero_ts = now_ts - now_hms.GetSeconds();
                int64_t start_time_offset_seconds = start_time_.GetSeconds();

                /*
                cout << "wday:" << wday << endl;
                cout << "today_zero_ts: ### [" << today_zero_ts << "]" << ctime((time_t*)&today_zero_ts);
                */

                // 计算相邻的开启时间点
                open_time_points_.clear();
                if (open_weeks_.empty()) {
                    open_time_points_.push_back(today_zero_ts + start_time_offset_seconds);
                    open_time_points_.push_back(today_zero_ts + utils::SECONDS_OF_ONE_DAY + start_time_offset_seconds);
                } else {
                    for (size_t i = 0u; i < open_weeks_.size(); ++i) {
                        int32_t w = open_weeks_[i];
                        open_time_points_.push_back(today_zero_ts + (utils::SECONDS_OF_ONE_DAY * (w - wday)) + start_time_offset_seconds);
                    }
                    for (size_t i = 0u; i < open_weeks_.size(); ++i) {
                        int32_t w = open_weeks_[i] + 7;
                        open_time_points_.push_back(today_zero_ts + (utils::SECONDS_OF_ONE_DAY * (w - wday)) + start_time_offset_seconds);
                    }
                }

                /*
                for (int64_t p : open_time_points_) {
                    cout << "open_time_points_[" << p << "]" << ctime((time_t*)&p);
                }
                */

                state_.now = now_ts;
                state_.duration = duration_seconds_;
                // 找到下一个开启时间点
                for (int64_t p : open_time_points_) {
                    if (now_ts < p + duration_seconds_) {
                        state_.begin = p;
                        break;
                    }
                }

                if (cb_.IsExist()) {
                    cb_(state_);
                    int64_t next_point_left_seconds = state_.GetNextPointRemainSeconds();
                    if (next_point_left_seconds == 0) {
                        next_point_left_seconds = 1;        // prevent too busy timer
                    }
                    if (next_point_left_seconds >= 0) {
                        g_dispatcher->quicktimer().SetTimeout([this]() {
                            Update();
                        }, next_point_left_seconds * 1000, auto_observer_);
                    }
                }
            }

        private:
            void SetID(uint32_t id) {
                id_ = id;
            }

            std::vector<int32_t> open_weeks_;
            std::vector<int64_t> open_time_points_;
            TimeStruct start_time_;
            int64_t duration_seconds_;

            uint32_t id_;
            CronJobInfo raw_info_;
            CronJobState state_;
            CallbackObserver<void(const CronJobState&)> cb_;
            base::AutoObserver auto_observer_;
            friend class CronJob;
        };


        /// CronJob
        CronJob::CronJob()
        {
            s_instance_ = this;
            jobs_.resize(1u, nullptr);
        }

        CronJob::~CronJob()
        {
            for (CronJobImpl * job : jobs_) {
                delete job;
            }
            jobs_.clear();
            s_instance_ = nullptr;
        }

        void CronJob::Add(const CronJobInfo& info, CallbackObserver<void(const CronJobState&)> cb)
        {
            CronJobImpl* job = new CronJobImpl(info, cb);
            if (!job->Parse()) {
                delete job;
                return;
            }
            bool add = false;
            for (size_t i = 0u; i < jobs_.size(); ++i) {
                CronJobImpl* j = jobs_[i];
                if (j != nullptr) {
                    if (!j->IsExist()) {
                        delete j;               // 清理掉过期的任务
                        jobs_[i] = nullptr;
                    }
                }
                if (!add && jobs_[i] == nullptr) {
                    jobs_[i] = job;
                    add = true;
                }
            }
            if (!add) {
                jobs_.push_back(job);
            }
            job->Update();
        }

        CronJob* CronJob::s_instance_ = nullptr;
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
