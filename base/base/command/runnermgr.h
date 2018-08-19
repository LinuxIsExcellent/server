#ifndef BASE_COMMAND1_RUNNERMGR_H
#define BASE_COMMAND1_RUNNERMGR_H

#include "command.h"
#include "../object.h"
#include "../utils/intrusive_list.h"
#include <queue>

namespace base
{
    namespace command
    {
        class CommandQueue : public base::Object
        {
            INTRUSIVE_LIST(CommandQueue)
        public:
            CommandQueue() : stop_(false) {}
            virtual ~CommandQueue() {
                DeleteAllCommand();
            }

            virtual const char* GetObjectName() {
                return "base::command::CommandQueue";
            }

            void DeleteAllCommand() {
                stop_ = true;
                while (!cmds_.empty()) {
                    delete cmds_.front();
                    cmds_.pop();
                }
            }

            bool empty() const {
                return cmds_.empty();
            }

            std::size_t size() const {
                return cmds_.size();
            }

            void Push(Command* cmd) {
                cmds_.push(cmd);
            }

            bool TryExecute(int64_t tick);

        private:
            std::queue<Command*> cmds_;
            bool stop_;
        };

        class RunnerMgr
        {
        public:
            static RunnerMgr& instance() {
                static RunnerMgr ins;
                return ins;
            }
            virtual ~RunnerMgr();

            bool empty() {
                return list_.empty();
            }

            // 注册执行队列
            void Register(CommandQueue* q) {
                if (list_.contains(q)) {
                    return;
                } else {
                    list_.push_front(q);
                    q->Retain();
                }
            }
            // 更新，执行所有未执行的命令
            void Update(int64_t tick);

        private:
            RunnerMgr();
            base::utils::IntrusiveList<CommandQueue> list_;
        };
    }
}

#endif // RUNNERMGR_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
