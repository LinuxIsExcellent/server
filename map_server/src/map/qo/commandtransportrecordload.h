#ifndef MAP_QO_COMMANDTRANSPORTRECORDLOAD_H
#define MAP_QO_COMMANDTRANSPORTRECORDLOAD_H

#include "../mapMgr.h"
#include <base/command/commandcallback.h>

namespace ms
{

    namespace map
    {
        namespace qo
        {
            class CommandTransportRecordLoad : public base::command::Command
            {
            public:
                CommandTransportRecordLoad(MapMgr& mapMgr, const std::function<void(bool)>& cb) : m_mapMgr(mapMgr), cb_(cb) {}
                virtual ~CommandTransportRecordLoad() {}

            private:
                virtual void OnCommandExecute();

                MapMgr& m_mapMgr;
                std::function<void(bool)> cb_;
            };
        }
    }
}

#endif // MAP_QO_COMMANDTRANSPORTRECORDLOAD_H
