#include "runnermgr.h"

namespace base
{
    namespace command
    {
        bool CommandQueue::TryExecute(int64_t tick)
        {
            if (cmds_.empty()) {
                return true;
            }
            Command* top = cmds_.front();
            if (!top->executed_) {
                if (top->OnSetup()) {
                    top->Execute();
                } else {
                    if (!stop_) {
                        delete top;
                        top = nullptr;
                        cmds_.pop();
                    }
                }
            } else {
                if (tick - top->beginExecuteTick() > 10 * 1000) {
                    top->OnCostTooMuchTimeWarning();
                }
            }

            if (!stop_ && top && top->finish_) {
                top->Cleanup();
                if (!stop_) {
                    delete top;
                    cmds_.pop();
                }
            }
            return cmds_.empty();
        }

        RunnerMgr::RunnerMgr()
        {
        }

        RunnerMgr::~RunnerMgr()
        {
            CommandQueue* q = list_.front();
            while (q) {
                CommandQueue* tmp = q;
                q = list_.erase(q);
                tmp->Release();
            }
        }

        void RunnerMgr::Update(int64_t tick)
        {
            CommandQueue* q = list_.front();
            while (q) {
                if (q->reference_count() == 2) {
                    if (q->TryExecute(tick)) {
                        // 执行完毕，从队列中移除
                        CommandQueue* tmp = q;
                        q = list_.erase(q);
                        tmp->Release();
                    } else {
                        q = q->list_next();
                    }
                } else {
                    CommandQueue* tmp = q;
                    q = list_.erase(q);
                    tmp->Release();
                }
            }
        }
    }
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
