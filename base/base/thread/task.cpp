#include "task.h"
#include "threadpool.h"
#include "../event/dispatcher.h"
#include <functional>

namespace base
{
    namespace thread
    {
        /// Task
        Task::~Task()
        {
        }

        /// AsyncTask
        AsyncTask::AsyncTask()
        {
            onPreExecute();
        }

        AsyncTask::~AsyncTask()
        {
        }

        void AsyncTask::OnTaskExecute()
        {
            doInWorkThread();
            std::shared_ptr<Task> pThis = shared_from_this();
            g_dispatcher->performAtNextLoop(CallbackObserver<void()>([this, pThis]() {
                this->invokePostExecute();
            }, autoObserver_));
        }

        void AsyncTask::invokePostExecute()
        {
            is_done_ = true;
            onPostExecute();
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
