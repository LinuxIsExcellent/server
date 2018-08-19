#ifndef MAP_QO_COMMANDFIGHTHISTORYLOAD_H
#define MAP_QO_COMMANDFIGHTHISTORYLOAD_H

#include "../mapMgr.h"
#include <base/command/commandcallback.h>

namespace ms
{

    namespace map
    {
        namespace qo
        {
            class CommandFightHistoryLoad : public base::command::Command
            {
            public:
                CommandFightHistoryLoad(MapMgr& mapMgr, const std::function<void(bool)>& cb) : m_mapMgr(mapMgr), cb_(cb) {}
                virtual ~CommandFightHistoryLoad() {}

            private:
                virtual void OnCommandExecute();

                void Load();

                MapMgr& m_mapMgr;
                std::function<void(bool)> cb_;
            };
        }
    }
}

#endif // COMMANDFIGHTHISTORYLOAD_H
