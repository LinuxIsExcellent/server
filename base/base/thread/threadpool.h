#ifndef BASE_THREAD_THREADPOOL_H
#define BASE_THREAD_THREADPOOL_H

#include "../global.h"
#include <memory>

namespace base
{
    namespace thread
    {
        class Task;
        class ThreadPoolImpl;
        class ThreadPool
        {
        public:
            static ThreadPool* getInstance();

            ThreadPool(int min = 0, int max = 8);
            ~ThreadPool();

            void start();
            void stop();

            void setThreadNum(int min, int max);

            void queueUserWorkItem(std::shared_ptr<Task> task);

        private:
            void* workerThread(void* args);

            int thread_min_;
            int thread_max_;
            bool run_;

            ThreadPoolImpl* impl_;
            friend class ThreadPoolImpl;
        };
    }
}

#endif
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
