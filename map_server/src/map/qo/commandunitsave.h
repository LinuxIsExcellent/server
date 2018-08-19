#ifndef MAP_QO_COMMANDUNITSAVE_H
#define MAP_QO_COMMANDUNITSAVE_H

#include "../map.h"
#include <base/command/commandcallback.h>
#include "../info/dbinfo.h"

namespace ms
{
    namespace map
    {
        class Unit;
        
        namespace qo
        {
            class CommandUnitSave : public base::command::Command
            {
            public:
                CommandUnitSave(Unit* unit) : m_unit(unit) {}
                virtual ~CommandUnitSave() {};

            private:
                virtual void OnCommandExecute();
                
                Unit* m_unit = nullptr;
            };
            
            class CommandUnitDelete : public base::command::Command
            {
            public:
                CommandUnitDelete(int id) : m_id(id) {}
                virtual ~CommandUnitDelete() {}
                
            private:
                virtual void OnCommandExecute() override;
                
            private:
                int m_id = 0;
            };
            
        }
    }
}

#endif // COMMANDFIGHTHISTORYSAVE_H
