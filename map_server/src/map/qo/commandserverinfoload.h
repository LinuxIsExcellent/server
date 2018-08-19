#ifndef MAP_QO_COMMANDSERVERINFOLOAD_H
#define MAP_QO_COMMANDSERVERINFOLOAD_H
#include <base/command/command.h>

namespace ms
{
    namespace map
    {
        namespace qo
        {
            class CommandServerInfoLoad : public base::command::Command
            {
            public:
                CommandServerInfoLoad(const std::function<void(bool)>& cb)
                    : m_cb(cb) {}
                virtual ~CommandServerInfoLoad() {}

            private:
                virtual void OnCommandExecute();

            private:
                std::function<void(bool)> m_cb;
            };
        }
    }
}
#endif // MAP_QO_COMMANDSERVERINFOLOAD_H
