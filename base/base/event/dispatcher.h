#ifndef BASE_EVENT_DISPATCHER_H
#define BASE_EVENT_DISPATCHER_H

#include "../utils/utils_time.h"
#include "../utils/intrusive_list.h"
#include "../timer.h"
#include "../callback.h"
#include <cstddef>
#include <stdint.h>
#include <map>
#include <queue>
#include <functional>
#ifdef __APPLE__
#include <sys/event.h>
#else
#include <sys/epoll.h>
#endif

namespace base
{
    namespace event
    {
        class EventIO;
        class DispatcherImpl;

        // 事件派发器　（定时器事件，ＩＯ事件)
        class Dispatcher
        {
        public:
            struct QuickTickProvider {
                int64_t operator()() {
                    return Dispatcher::instance().GetTickCache();
                }
            };
        public:
            static Dispatcher& instance() {
                static Dispatcher ins;
                return ins;
            }
            ~Dispatcher();

            Timer<QuickTickProvider>& quicktimer() {
                return quicktimer_;
            }
            uint64_t frame_no() const {
                return frame_no_;
            }

            void performAtNextLoop(const base::CallbackObserver<void()>& cb);

            // 开始事件主循环
            void Dispatch();

            // 获取当前时间tick(ms), 每次循环仅刷新一次, 对时间要求不严格的情况下，
            // 可使用该函数获取当前时间，避免系统调用
            int64_t inline GetTickCache() const {
                return tick_;
            }
            // 快速获取当前时间(second)
            int64_t inline GetTimestampCache() const {
                return GetTickCache() / 1000;
            }
            // 更新当前时间
            int64_t inline UpdateTickCache() {
                tick_ = base::utils::nowtick();
                return tick_;
            }

            // 强制清除所有事件
            void Clear();

            // 正常退出（直到没有未决的事件）
            void NormalExit();

            void DebugDump();

        private:
            Dispatcher();
            utils::IntrusiveList<EventIO> io_list_;
            std::queue<EventIO*> closed_io_list_;
            bool exit_;
            int64_t tick_last_;
            int64_t tick_;
            int epfd_;
            int wait_;
            static const int EVT_BUF_SIZE = 256;
#ifdef __APPLE__
            struct kevent evtbuf_[EVT_BUF_SIZE];
#else
            epoll_event evtbuf_[EVT_BUF_SIZE];
#endif
            Timer<QuickTickProvider> quicktimer_;
            uint64_t frame_no_;
            std::vector<base::CallbackObserver<void()>> perform_at_next_loop_;
            DispatcherImpl* impl_ = nullptr;
            friend class EventTimeout;
            friend class EventIO;
            friend class EventUpdate;
            friend class DispatcherImpl;
        };
    }
}

extern base::event::Dispatcher* g_dispatcher;

#endif // EVENTDISPATCHER_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
