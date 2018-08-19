#include "eventio.h"
#include "dispatcher.h"
#include "../logger.h"
#include <unistd.h>

namespace base
{
    namespace event
    {
        using namespace std;

        EventIO::~EventIO()
        {
        }

        void EventIO::AddToDispatcher(int fd, int evt)
        {
            assert(!list_linked());
            ioevt_ = evt;
            if (fd != -1 && fd != fd_) {
                if (fd_ != -1) {
                    close(fd_);
                    // remote evt from epoll ?
                }
                fd_ = fd;
            }
            Dispatcher::instance().io_list_.push_front(this);
            int epfd = Dispatcher::instance().epfd_;
#ifdef __APPLE__
            struct kevent ke;
            if (ioevt_ & IO_READABLE) {
                EV_SET(&ke, fd_, EVFILT_READ, EV_ADD, 0, 0, this);
                int r = kevent(epfd, &ke, 1, NULL, 0, NULL);
                errno_assert(r != -1);
            }
            if (ioevt_ & IO_WRITEABLE) {
                EV_SET(&ke, fd_, EVFILT_WRITE, EV_ADD, 0, 0, this);
                int r = kevent(epfd, &ke, 1, NULL, 0, NULL);
                errno_assert(r != -1);
            }
#else
            epoll_event ee;
            ee.events = EPOLLET
                        | (ioevt_ & IO_READABLE ?  EPOLLIN : 0)
                        | (ioevt_ & IO_WRITEABLE ? EPOLLOUT : 0);
            ee.data.ptr = this;
            int r = epoll_ctl(epfd, EPOLL_CTL_ADD, fd_, &ee);
            errno_assert(r == 0);
#endif
            Retain();
        }

        void EventIO::ModifyIOEvent(int ioevt)
        {
            assert(list_linked());
            int epfd = Dispatcher::instance().epfd_;
#ifdef __APPLE__
            int oldIOEvt = ioevt_;
            ioevt_ = ioevt;
            struct kevent ke;
            if (ioevt_ & IO_READABLE) {
                if (!(oldIOEvt & IO_READABLE)) {
                    EV_SET(&ke, fd_, EVFILT_READ, EV_ADD, 0, 0, this);
                    int r = kevent(epfd, &ke, 1, NULL, 0, NULL);
                    errno_assert(r != -1);
                }
            } else {
                if (oldIOEvt & IO_READABLE) {
                    EV_SET(&ke, fd_, EVFILT_READ, EV_DELETE, 0, 0, this);
                    int r = kevent(epfd, &ke, 1, NULL, 0, NULL);
                    errno_assert(r != -1);
                }
            }
            if (ioevt_ & IO_WRITEABLE) {
                if (!(oldIOEvt & IO_WRITEABLE)) {
                    EV_SET(&ke, fd_, EVFILT_WRITE, EV_ADD, 0, 0, this);
                    int r = kevent(epfd, &ke, 1, NULL, 0, NULL);
                    errno_assert(r != -1);
                }
            } else {
                if (oldIOEvt & IO_WRITEABLE) {
                    EV_SET(&ke, fd_, EVFILT_WRITE, EV_DELETE, 0, 0, this);
                    int r = kevent(epfd, &ke, 1, NULL, 0, NULL);
                    errno_assert(r != -1);
                }
            }
#else
            ioevt_ = ioevt;
            epoll_event ee;
            ee.events = EPOLLET
                        | (ioevt_ & IO_READABLE ?  EPOLLIN : 0)
                        | (ioevt_ & IO_WRITEABLE ? EPOLLOUT : 0);
            ee.data.ptr = this;
            int r = epoll_ctl(epfd, EPOLL_CTL_MOD, fd_, &ee);
            errno_assert(r == 0);
#endif
        }

        void EventIO::CloseFD()
        {
            if (fd_ != -1) {
                close(fd_);
                fd_ = -1;
            }
        }

        void EventIO::Close()
        {
            if (!closed_ && list_linked()) {
                int epfd = Dispatcher::instance().epfd_;
#ifdef __APPLE__
                struct kevent ke;
                int r1, r2;
                {
                    EV_SET(&ke, fd_, EVFILT_READ, EV_DELETE, 0, 0, this);
                    r1 = kevent(epfd, &ke, 1, NULL, 0, NULL);
                }
                {
                    EV_SET(&ke, fd_, EVFILT_WRITE, EV_DELETE, 0, 0, this);
                    r2 = kevent(epfd, &ke, 1, NULL, 0, NULL);
                }
#else
                epoll_event ee;
                int r = epoll_ctl(epfd, EPOLL_CTL_DEL, fd_, &ee);
                errno_assert(r == 0);
#endif
                Dispatcher::instance().closed_io_list_.push(this);
                closed_ = true;
            }
            CloseFD();
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
