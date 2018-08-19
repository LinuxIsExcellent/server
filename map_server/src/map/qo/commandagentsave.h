#ifndef MAP_QO_COMMANDAGENTSAVE_H
#define MAP_QO_COMMANDAGENTSAVE_H

#include "../map.h"
#include <base/command/commandcallback.h>
#include "../info/dbinfo.h"

namespace ms
{
    namespace map
    {
        namespace msgqueue
        {
            class MsgQueue;
        }

        class Agent;

        namespace qo
        {
            class CommandAgentSave : public base::command::Command
            {
            public:
                CommandAgentSave(Agent* agent) : m_agent(agent) {}
                virtual ~CommandAgentSave() {};

            private:
                virtual void OnCommandExecute();

                Agent* m_agent = nullptr;
            };

            class CommandAgentDelete : public base::command::Command
            {
            public:
                CommandAgentDelete(int64_t uid) : m_uid(uid) {}
                virtual ~CommandAgentDelete() {}

            private:
                virtual void OnCommandExecute() override;

            private:
                int64_t m_uid = 0;
            };

        }
    }
}

#endif // MAP_QO_COMMANDAGENTSAVE_H
