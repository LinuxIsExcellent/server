#include "runner.h"
#include "runnermgr.h"

namespace base
{
    namespace command
    {
        Runner::Runner()
            : queue_(new CommandQueue)
        {
        }

        Runner::~Runner()
        {
            queue_->DeleteAllCommand();
            SAFE_RELEASE(queue_);
        }

        std::size_t Runner::size() const
        {
            return queue_->size();
        }

        void Runner::PushCommand(base::command::Command* cmd)
        {
            queue_->Push(cmd);
            RunnerMgr::instance().Register(queue_);
        }
    }
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
