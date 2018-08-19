#ifndef WORLDMGR_H
#define WORLDMGR_H

#include <base/global.h>
#include <base/utils/intrusive_list.h>
#include <base/observer.h>
#include <base/command/runner.h>
#include <base/cluster/nodemonitor.h>
#include <functional>
#include <unordered_map>
#include <set>

namespace base
{
    namespace gateway
    {
        class UserClient;
        class PacketIn;
        class PacketOut;
    }
}

namespace fs
{
    namespace qo
    {
        class CommandMapServiceListLoad;
    }

    class PlayerSession;

    struct MapServiceInfo {
        int id = 0;
        int merge_to_id = 0;
        std::string name;
        std::string ip;
        int port = 0;
    };

    struct PacketInfo {
        uint16_t code;
        bool isSend;
        uint32_t count;  // 次数
        uint64_t size;
    };

    struct BandWidthInfo {
        uint64_t size = 0;
        uint64_t lastSize = 0;
        uint32_t count = 0;
        uint32_t lastCount = 0;

        void LogLast() {
            lastSize = size;
            lastCount = count;
        }
        void Clear() {
            size = 0;
            lastSize = 0;
            count = 0;
            lastCount = 0;
        }
    };

    class WorldMgr
    {
    public:
        static WorldMgr* Create(int msServiceId);
        static void Destroy();

        uint32_t GetPlayerCount() const {
            return m_players.size();
        }

        PlayerSession* AddPlayer(base::gateway::UserClient* client);
        void RemovePlayer(PlayerSession* ps);
        void DisconnectAll(const std::function<void()>& cb);

        void OnRecvPacket(base::gateway::PacketIn& pktin);
        void OnSendPacket(base::gateway::PacketOut& pktout);
        void SavePacketData();

        int GetTrueMsId(int msId) const;
        const std::string MsIdToName(int msId) const;
        bool IsMsUp(int msId) const {
            return FindMsMbid(msId);
        }
        bool IsMsUp(const std::string& name) const {
            return FindMsMbid(name);
        }
        const base::cluster::MailboxID* FindMsMbid(const std::string& name) const {
            auto it = m_msMbids.find(name);
            return it != m_msMbids.end() ? &it->second : nullptr;
        }
        const base::cluster::MailboxID* FindMsMbid(int msId) const {
            return FindMsMbid(MsIdToName(msId));
        }

    private:
        static constexpr int SAVE_PACKET_INTERVAL = 10;
        static constexpr int RELOAD_SERVER_INTERVAL = 30;

        PacketInfo* FindPacketInfo(uint16_t code) {
            auto it = m_packetInfos.find(code);
            return it != m_packetInfos.end() ? &it->second : nullptr;
        }

        void OnTimerUpdate();
        void OnNodeDown(const base::cluster::NodeInfo& node);
        void OnNodeMailBoxUp(const char* name, const base::cluster::MailboxID& mbid);
        void OnNodeMailBoxDown(const char* name);
        void ReLoadServerList();
        
    private:
        WorldMgr(int msServiceId);
        ~WorldMgr();

        int m_msServiceId = 0;
        base::utils::IntrusiveList<PlayerSession> m_players;
        base::AutoObserver m_autoObserver;
        base::command::Runner m_runner;

        std::vector<MapServiceInfo> m_servers;
        std::unordered_map<std::string, base::cluster::MailboxID> m_msMbids;

        int64_t m_timerIndex = 0;
        int64_t m_lastLogClearTime = 0;
        // 记录协议包输出流量
        BandWidthInfo m_sendInfo;
        BandWidthInfo m_recvInfo;
        // 统计协议包具体信息
        std::unordered_map<uint16_t, PacketInfo> m_packetInfos;
    };

    extern WorldMgr* gWorldMgr;
}


#endif // WORLDMGR_H
