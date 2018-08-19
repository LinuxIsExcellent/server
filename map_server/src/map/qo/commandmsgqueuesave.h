#ifndef MAP_QO_COMMANDMSGQUEUESAVE_H
#define MAP_QO_COMMANDMSGQUEUESAVE_H

#include "../info/dbinfo.h"
#include "../msgqueue/msgrecord.h"
#include "../msgqueue/msgqueue.h"

namespace ms
{
    namespace map
    {
        namespace qo
        {
            class CommandMsgQueueDelete : public base::command::Command
            {
            public:
                CommandMsgQueueDelete(msgqueue::MsgRecord* msg) : m_msg(msg) {}
                virtual ~CommandMsgQueueDelete() {};

            private:
                virtual void OnCommandExecute();

                msgqueue::MsgRecord* m_msg = nullptr;
            };

        }
    }
}

#endif // COMMANDMSGQUEUESAVE_H
