#ifndef MAP_QO_COMMANDAGENTLOAD_H
#define MAP_QO_COMMANDAGENTLOAD_H

#include "../mapMgr.h"
#include <base/command/commandcallback.h>
#include "../info/dbinfo.h"

namespace ms
{
    namespace map
    {
        namespace qo
        {
            class CommandAgentLoad : public base::command::Command
            {
            public:
                CommandAgentLoad(MapMgr& mapMgr, const std::function<void(bool)>& cb) : m_mapMgr(mapMgr), cb_(cb) {}
                virtual ~CommandAgentLoad() {}

            private:
                virtual void OnCommandExecute();

                MapMgr& m_mapMgr;
                std::function<void(bool)> cb_;
            };
        }
    }
}

#endif // MAP_QO_COMMANDAGENTLOAD_H
