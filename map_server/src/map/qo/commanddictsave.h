#ifndef MAP_QO_COMMANDDICTSAVE_H
#define MAP_QO_COMMANDDICTSAVE_H
#include <base/command/command.h>

namespace ms
{
    namespace map
    {
        namespace qo
        {
            class CommandDictSave : public base::command::Command
            {
            public:
                CommandDictSave(const std::string& k, const std::string& v)
                    : m_k(k), m_v(v) {}
                virtual ~CommandDictSave() {}
                    
            private:
                virtual void OnCommandExecute();
                
            private:
                std::string m_k;
                std::string m_v;
            };
        }
    }
}

#endif // MAP_QO_COMMANDDICTSAVE_H
