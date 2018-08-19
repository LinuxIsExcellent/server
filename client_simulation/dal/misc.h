#ifndef DAL_MISC_H
#define DAL_MISC_H

#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <model/metadata.h>

namespace client
{
    class PacketIn;
}

namespace dal
{
    class ClientImpl;
    class FrontServer;
    
    enum class SomeAction {
        range_fetch = 1, // 查看排行榜（随机一类排行）
        open_queue2, // 开启第二队列
        research_tech, // 研究科技（5分钟内自动使用免费加速，己加入联盟时，80％机率请求联盟帮助，20％机率使用加速）
        upgrade_building, // 升级建筑（5分钟内自动使用免费加速，己加入联盟时，80％机率请求联盟帮助，20％机率使用加速）
        collect_resource, // 收集资源（每次收集3－6个资源）
        training_army, // 训练军队／陷阱（立即完成或普通训练）
        training_trap, // 创建联盟（每50人中其中1个创建，其它人加入）
        
        alliance_explore, // 联盟探险（自动接取，领取奖励）
        alliance_help, // 联盟帮助（单个或全部）
        alliance_donate, // 联盟捐献
        babel_fight,        // 爬塔出战
        tower_mopup,        // 爬塔扫荡
        
        ACTION_END
    };
    
    
    struct CDInfo {
        int id = 0;
        model::CDType type = model::CDType::COMMON;
        int64_t expireTime = 0; // timestamp
        int sumTime = 0; // seconds
        int remainTime = 0;
    };
    struct BuildingInfo {
        int id = 0;
        int gridId = 0;
        int tplId = 0;
        int level = 0;
        int maxLevel = 0;
        model::BuildingState state = model::BuildingState::NORMAL;
        std::vector<int> cdIds;
        int param1 = 0;
        int param2 = 0;
        int param3 = 0;
        int param4 = 0;
        int param5 = 0;
    };
    struct SkillInfo {
        int tplid = 0;
        int level = 0;
    };
    
    class Misc
    {
    public:
        Misc(ClientImpl *impl, FrontServer *fs);
        virtual ~Misc();

        int GetCastleLevel() const;
        int GetMarchDragonId() const;
        
        void DoSomething();
        
        // 国家聊天
        void KingdomChat();
        // 联盟聊天
        void AllianceChat();
        
        // 查看排行榜
        void RangeFetch();
        // 开启第二队列
        void OpenQueue2();
        // 研究科技
        void ResearchTech();
        // 升级建筑
        void UpgradeBuilding();
        // 收集资源
        void CollectResource();
        // 训练军队
        void TrainingArmy();
        // 制造陷阱
        void TrainingTrap();
        // 千重楼出兵
        void BabelFight();
        // 千重楼扫荡
        void BabelMopup();

        void SendBagUse(int tplid, int count, bool useGold, const std::string& p1, const std::string& p2);

    private:
        void HandleChatRecv(client::PacketIn& pktin);
        void HandleRangeFetchResponse(client::PacketIn& pktin);
        void HandleCDListUpdate(client::PacketIn& pktin);
        void HandleBuildingUpdate(client::PacketIn& pktin);
        void HandleTechUpdate(client::PacketIn& pktin);
        void HandleBabelDataUpdate(client::PacketIn& pktin);
        
    private:
        FrontServer* g_fs;
        
        std::unordered_map<int, int> m_range_vsns;
        
        std::unordered_map<int, CDInfo> m_cds;
        std::unordered_map<int, BuildingInfo> m_buildings;
        std::unordered_map<int, int> m_skills;
        // 千重楼
        int m_babelLayer = 0;
        
        int buider1_;
        int buider2_;
        int buider2LeftSeconds_;
    };

}
#endif // DAL_MISC_H
