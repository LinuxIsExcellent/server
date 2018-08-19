#ifndef MAP_QO_COMMANDSUCCESSIVEKINGSLOAD_H
#define MAP_QO_COMMANDSUCCESSIVEKINGSLOAD_H
#include <base/command/command.h>

namespace ms
{
    namespace map
    {
        namespace qo
        {
            class CommandSuccessiveKingsLoad : public base::command::Command
            {
            public:
                CommandSuccessiveKingsLoad() {}
                virtual ~CommandSuccessiveKingsLoad() {}

            private:
                virtual void OnCommandExecute();
            };
        }
    }
}
#endif // MAP_QO_COMMANDSUCCESSIVEKINGSLOAD_H
