#ifndef MAP_QO_COMMANDTROOPLOAD_H
#define MAP_QO_COMMANDTROOPLOAD_H

#include "../mapMgr.h"
#include <base/command/commandcallback.h>

namespace ms
{

    namespace map
    {
        namespace qo
        {
            class CommandTroopLoad : public base::command::Command
            {
            public:
                CommandTroopLoad(MapMgr& mapMgr, const std::function<void(bool)>& cb) : m_mapMgr(mapMgr), cb_(cb) {}
                virtual ~CommandTroopLoad() {}

            private:
                virtual void OnCommandExecute();

                MapMgr& m_mapMgr;
                std::function<void(bool)> cb_;
            };
        }
    }
}

#endif // MAP_QO_COMMANDTROOPLOAD_H
