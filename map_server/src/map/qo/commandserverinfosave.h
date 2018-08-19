#ifndef MAP_QO_COMMANDSERVERINFOSAVE_H
#define MAP_QO_COMMANDSERVERINFOSAVE_H
#include <base/command/command.h>

namespace ms
{
    namespace map
    {
        namespace qo
        {
            class CommandServerInfoSave : public base::command::Command
            {
            public:
                CommandServerInfoSave() {}
                virtual ~CommandServerInfoSave() {}

            private:
                virtual void OnCommandExecute();
            };
        }
    }
}
#endif // MAP_QO_COMMANDSERVERINFOSAVE_H
