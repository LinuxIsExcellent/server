#ifndef MAP_QO_COMMANDDICTLOAD_H
#define MAP_QO_COMMANDDICTLOAD_H
#include <base/command/command.h>

namespace ms
{
    namespace map
    {
        namespace qo
        {
            class CommandDictLoad : public base::command::Command
            {
            public:
                CommandDictLoad(const std::function<void(bool)>& cb)
                    : m_cb(cb) {}
                virtual ~CommandDictLoad() {}
                
            private:
                virtual void OnCommandExecute();
                
            private:
                std::function<void(bool)> m_cb;
            };
        }
    }
}

#endif // MAP_QO_COMMANDDICTLOAD_H
