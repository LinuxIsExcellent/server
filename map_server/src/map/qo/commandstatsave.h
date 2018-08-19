#ifndef MAP_QO_COMMANDSTATSAVE_H
#define MAP_QO_COMMANDSTATSAVE_H
#include <base/command/command.h>

namespace ms
{
    namespace map
    {
        namespace qo
        {
            class CommandStatSave : public base::command::Command
            {
            public:
                CommandStatSave(const std::string& k, const std::string& v)
                    : m_k(k), m_v(v) {}
                virtual ~CommandStatSave() {}

            private:
                virtual void OnCommandExecute();

            private:
                std::string m_k;
                std::string m_v;
            };
        }
    }
}

#endif // MAP_QO_COMMANDSTATSAVE_H
