#ifndef MAP_QO_COMMANDSUCCESSIVEKINGSAVE_H
#define MAP_QO_COMMANDSUCCESSIVEKINGSAVE_H
#include "../info/palaceWar.h"
#include <base/command/command.h>

namespace ms
{
    namespace map
    {
        namespace qo
        {
            class CommandSuccessiveKingSave : public base::command::Command
            {
            public:
                CommandSuccessiveKingSave(const info::SuccessiveKing& sk) : m_sk(sk) {}
                ~CommandSuccessiveKingSave() {}

            private:
                virtual void OnCommandExecute();

            private:
                info::SuccessiveKing m_sk;
            };
        }
    }
}
#endif // MAP_QO_COMMANDSUCCESSIVEKINGSAVE_H
