#ifndef BASE_COMMAND_COMMANDCALLBACK_H
#define BASE_COMMAND_COMMANDCALLBACK_H

#include "command.h"
#include <functional>

namespace base
{
    namespace command
    {
        template<typename T>
        class CommandCallback : public Command
        {
        public:
            CommandCallback(T cb) : cb_(cb) {}
            virtual ~CommandCallback() {}

        private:
            virtual void OnCleanup() override {
                cb_();
            }
            virtual void OnCommandExecute() override {
                Stop();
            }
            T cb_;
        };

        template<typename T>
        CommandCallback<T>* CreateCommandCallback(T cb)
        {
            return new CommandCallback<T>(cb);
        }
    }
}

#endif // COMMANDCALLBACK_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
