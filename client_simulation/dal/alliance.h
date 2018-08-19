#ifndef DAL_ALLIANCE_H
#define DAL_ALLIANCE_H

#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace client
{
    class PacketIn;
}

namespace dal
{
    class ClientImpl;
    class FrontServer;
    
    struct ExploreInfo {
        int tplid = 0;
        bool accept = false;
        int fullTime = 0; // 完整时间(秒)
        int cdId = 0; // CD ID（已接取并为0时表示已完成）
        // dropList
    };
    
    struct MultiExploreInfo {
        int id = 0;
        std::vector<int64_t> uids;
    };
    
    class Alliance
    {
    public:
        Alliance(ClientImpl *impl, FrontServer *fs);
        virtual ~Alliance();

        int64_t aid() const {
            return aid_;
        }
        
        const std::string& name() const {
            return name_;
        }
        
        const std::string& nickname() const {
            return nickname_;
        }
        
        int64_t leaderId() const {
            return leaderId_;
        }
        
        int alliesCount() const {
            return alliesCount_;
        }
        
        int alliesMax() const {
            return alliesMax_;
        }
        
        void CheckCreate();
        void CheckQuit();
        
        // 联盟探险（一定机率使用刷新CD）
        void Explore();
        
        // 联盟帮助（单个或全部）
        void Help();
        
        // 联盟捐献
        void Donate();

    private:
        void HandleAllianceUpdate(client::PacketIn& pktin);
        void HandleAllianceSearchResponse(client::PacketIn& pktin);
        void HandleAllianceScienceUpdate(client::PacketIn& pktin);
        void HandleAllianceHelpUpdate(client::PacketIn& pktin);
        
        void Join();
        
    private:
        FrontServer* g_fs;
        
        int64_t aid_ = 0;
        std::string name_;
        std::string nickname_;
        int64_t leaderId_ = 0;
        std::string leaderName_;
        int64_t leaderHeadId_ = 0;
        int bannerId_ = 0;
        int alliesCount_ = 0;
        int alliesMax_ = 0;
        
        std::unordered_map<int, int> m_sciences;
        int honor_ = 0;
        int donateCDTime_ = 0; // 捐献CD时间（秒）
        int donateGoldCount_ = 0;
        
        std::unordered_map<int, bool> m_helps;
        
        std::unordered_map<int, ExploreInfo> m_explores;
        int refreshSeconds_ = 0; // 探险CD 剩余刷新时间(秒)
        
        std::unordered_map<int, MultiExploreInfo> m_multiExplores;
        
    };

}
#endif // ALLIANCE_H
