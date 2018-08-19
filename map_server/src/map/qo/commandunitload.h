#ifndef MAP_QO_COMMANDUNITLOAD_H
#define MAP_QO_COMMANDUNITLOAD_H

#include "../mapMgr.h"
#include <base/command/commandcallback.h>

namespace ms
{

    namespace map
    {
        namespace qo
        {
            class CommandUnitLoad : public base::command::Command
            {
            public:
                CommandUnitLoad(MapMgr& mapMgr, const std::function<void(bool)>& cb) : m_mapMgr(mapMgr), cb_(cb) {}
                virtual ~CommandUnitLoad() {}

            private:
                virtual void OnCommandExecute();

                MapMgr& m_mapMgr;
                std::function<void(bool)> cb_;
            };
        }
    }
}

#endif // MAP_QO_COMMANDUNITLOAD_H
