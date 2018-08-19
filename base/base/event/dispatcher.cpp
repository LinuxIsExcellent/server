#include "dispatcher.h"
#include "eventio.h"
#include "../autoreleasepool.h"
#include "../logger.h"
#include "../action/actionmanager.h"
#include "../command/runnermgr.h"
#include <unistd.h>
#ifdef __APPLE__
#include <sys/event.h>
#else
#include <sys/epoll.h>
#endif
#include <mutex>

base::event::Dispatcher* g_dispatcher = nullptr;

namespace base
{
    namespace event
    {
        using namespace std;

        /// DispatcherImpl;
        class DispatcherImpl
        {
        public:

            std::mutex perform_at_next_loop_mutex;

        };

        /// EventDispatcher
        Dispatcher::Dispatcher()
            : exit_(false), tick_last_(0), tick_(0), epfd_(-1), wait_(25), frame_no_(0)
        {
            impl_ = new DispatcherImpl;
            assert(g_dispatcher == nullptr);
#ifdef __APPLE__
            epfd_ = kqueue();
            errno_assert(epfd_ > 0);
#else
            epfd_ = epoll_create(9999);
            errno_assert(epfd_ > 0);
#endif
            UpdateTickCache();
            g_dispatcher = this;
        }

        Dispatcher::~Dispatcher()
        {
            delete impl_;
            close(epfd_);
            g_dispatcher = nullptr;
        }

        void Dispatcher::NormalExit()
        {
            exit_ = true;
        }

        void Dispatcher::Clear()
        {
            // TODO
        }

        void Dispatcher::DebugDump()
        {
            cout << "Dispatcher: is_exit=" << boolalpha << exit_ <<
                 ", io_list_=" << io_list_.size() <<
                 ", closed_io_list_=" << closed_io_list_.size() <<
                 ", nodes_size:" << action::ActionManager::instance().nodes_size() << endl;
        }

        void Dispatcher::performAtNextLoop(const CallbackObserver<void()>& cb)
        {
            std::lock_guard<std::mutex> lock_guard(impl_->perform_at_next_loop_mutex);
            perform_at_next_loop_.push_back(cb);
        }

        void Dispatcher::Dispatch()
        {
#ifdef __APPLE__
            struct timespec ts_wait;
            ts_wait.tv_sec = 0;
            ts_wait.tv_nsec = wait_ * 1000 * 1000;
#endif
            int32_t BUSY_WEIGHT = wait_ * 2;

            UpdateTickCache();
            tick_last_ = tick_;
            int64_t tick_span = 0;
            int64_t last_log_ts = 0;
            int32_t busy_count = 0;
            int64_t max_cost_time = 0;
            int n = 0;
            while (true) {
                ++frame_no_;
                // update tick
                UpdateTickCache();
                tick_span = tick_ - tick_last_;
                tick_last_ = tick_;

                if (tick_span > BUSY_WEIGHT) {
                    ++busy_count;
                    if (tick_span > max_cost_time) {
                        max_cost_time = tick_span;
                    }
                    if (last_log_ts < tick_ - 10000) {
                        // 避免在繁忙时日志记录过于频繁
                        LOG_WARN("process is too busy, cost_time=%dms, max_cost_time=%d, busy_count=%d\n", tick_span, max_cost_time, busy_count);
                        last_log_ts = tick_;
                        max_cost_time = 0;
                        busy_count = 0;
                    }
                }

                // execute quick timer
                quicktimer_.Update(tick_);

                // execute command
                command::RunnerMgr::instance().Update(tick_);

                action::ActionManager::instance().Update(tick_, (int32_t)tick_span);

                // execute next loop call
                if (!perform_at_next_loop_.empty()) {
                    std::vector<base::CallbackObserver<void()>> tmp;
                    {
                        std::lock_guard<std::mutex> lock_guard(impl_->perform_at_next_loop_mutex);
                        tmp = perform_at_next_loop_;
                        perform_at_next_loop_.clear();
                    }
                    for (const auto & cb : tmp) {
                        cb();
                    }
                }

                // auto release memory
                PoolManager::instance()->Pop();

                if (exit_) {
                    if (io_list_.empty()
                            && closed_io_list_.empty()
                            && action::ActionManager::instance().nodes().empty()
                            && perform_at_next_loop_.empty()
                            && command::RunnerMgr::instance().empty()) {
                        // there is no pending event, then stop the dispatcher
                        break;
                    } else {
                        ;
                    }
                }

#ifdef __APPLE__
                n = kevent(epfd_, NULL, 0, evtbuf_, EVT_BUF_SIZE, &ts_wait);
                if (n == -1) {
                    cout << strerror(errno) << endl;
                }
                for (int i = 0; i < n; ++i) {
                    EventIO* evtobj = static_cast<EventIO*>(evtbuf_[i].udata);
                    if (!evtobj->closed() && evtbuf_[i].filter == EVFILT_READ) {
                        evtobj->OnEventIOReadable();
                    }
                    if (evtbuf_[i].filter == EVFILT_WRITE) {
                        evtobj->OnEventIOWriteable();
                    }
                }
#else
                n = epoll_wait(epfd_, evtbuf_, EVT_BUF_SIZE, wait_);

                if (n <= -1) {
                    // error
                    // TODO error handle
                } else if (n == 0) {
                    // timeout
                } else {
                    // handle io event
                    for (int i = 0; i < n; ++i) {
                        EventIO* evobj = static_cast<EventIO*>(evtbuf_[i].data.ptr);
                        if (!evobj->closed() && ((evtbuf_[i].events & EPOLLIN) | (evtbuf_[i].events & EPOLLERR))) {
                            evobj->OnEventIOReadable();
                        }
                        if (!evobj->closed() && (evtbuf_[i].events & EPOLLOUT)) {
                            evobj->OnEventIOWriteable();
                        }
                    }
                }
#endif

                while (!closed_io_list_.empty()) {
                    EventIO* cur = closed_io_list_.front();
                    cur->OnEventIOClose();
                    io_list_.erase(cur);
                    cur->Release();
                    closed_io_list_.pop();
                }
            }

            quicktimer_.StopAll();
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
