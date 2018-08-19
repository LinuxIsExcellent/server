#ifndef DAL_MAP_H
#define DAL_MAP_H

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
    
    struct MapUnit {
        int tplid = 0;
        int x = 0;
        int y = 0;
        int64_t uid = 0;
    };
    
    struct MapTroop {
//         MapTroop(int tid, int tm)
//           :troopId(tid), team(tm){
//         };

        int troopId = 0;
        int team = 0;
    };

    class Map
    {
    public:
        Map(ClientImpl *impl, FrontServer *fs);
        virtual ~Map();

        int k() const {
            return k_;
        }

        int x() const {
            return x_;
        }

        int y() const {
            return y_;
        }

        void MapView();
        void MapLeave();
        void MapMarch();
        void Teleport();


    private:
        void HandleInfoUpdate(client::PacketIn& pktin);
        void HandleUnitUpdate(client::PacketIn& pktin);
        void HandleTroopUpdate(client::PacketIn& pktin);
        
    private:
        FrontServer* g_fs;
        int k_ = 0;
        int x_ = 0;
        int y_ = 0;
        
        std::unordered_map<int, MapUnit> m_units;
        std::unordered_map<int, MapTroop> m_troops;
    };

}
#endif // MAP_H
