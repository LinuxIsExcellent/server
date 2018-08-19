#ifndef BASE_COMMAND_RUNNER_H
#define BASE_COMMAND_RUNNER_H

#include "command.h"
#include "commandcallback.h"

namespace base
{
    namespace command
    {
        class Runner
        {
        public:
            Runner() ;
            virtual ~Runner() ;

            bool empty() const {
                return size() == 0u;
            }

            std::size_t size() const;

            void PushCommand(Command* cmd);

            template<typename T>
            void PushFunction(T fun) {
                PushCommand(CreateCommandCallback<T>(fun));
            }

            template<typename T>
            void WhenAllFinish(T cb) {
                if (empty()) {
                    cb();
                } else {
                    PushCommand(CreateCommandCallback<T>(cb));
                }
            }

        private:
            CommandQueue* queue_;
        };
    }
}

#endif // RUNNER_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
