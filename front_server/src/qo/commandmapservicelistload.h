#ifndef QO_COMMANDMAPSERVICELISTLOAD_H
#define QO_COMMANDMAPSERVICELISTLOAD_H

#include <base/command/commandcallback.h>
#include <vector>

namespace fs
{
    class MapServiceInfo;
    namespace qo
    {
        class CommandMapServiceListLoad : public base::command::Command
        {
        public:
            CommandMapServiceListLoad(const std::function<void(bool result, const std::vector<MapServiceInfo>& infos)>& cb) 
                : cb_(cb) {}
            virtual ~CommandMapServiceListLoad() {}

        private:
            virtual void OnCommandExecute();

            std::function<void(bool, const std::vector<MapServiceInfo>&)> cb_;
        };
    }
}

#endif // COMMANDMAPSERVICELISTLOAD_H
