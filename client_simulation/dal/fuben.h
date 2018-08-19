#ifndef DAL_FUBEN_H
#define DAL_FUBEN_H

#include <stdint.h>
#include <string>
#include <unordered_map>

namespace client
{
    class PacketIn;
}

namespace dal
{
    class ClientImpl;
    class FrontServer;

    class FuBen
    {
    public:
        FuBen(ClientImpl *impl, FrontServer *fs);
        virtual ~FuBen();



    private:
        void HandleScenarioChapterUpdate(client::PacketIn& pktin);

    private:
        FrontServer* g_fs;
    };

}
#endif // MAP_H
