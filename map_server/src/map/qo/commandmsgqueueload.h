#ifndef MAP_QO_COMMANDMSGQUEUELOAD_H
#define MAP_QO_COMMANDMSGQUEUELOAD_H

#include "../msgqueue/msgqueue.h"
#include <base/command/commandcallback.h>

namespace ms
{
    namespace map
    {
        namespace qo
        {
            class CommandMsgQueueLoad : public base::command::Command
            {
            public:
                CommandMsgQueueLoad(msgqueue::MsgQueue& msgQueue) : m_msgQueue(msgQueue) {}
                virtual ~CommandMsgQueueLoad() {}

            private:
                virtual void OnCommandExecute();

                msgqueue::MsgQueue& m_msgQueue;
            };
        }
    }
}

#endif // COMMANDMSGQUEUELOAD_H
