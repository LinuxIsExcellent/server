#ifndef MAP_ALLIANCE_H
#define MAP_ALLIANCE_H


#include <unordered_map>
#include <list>
#include <vector>
#include "info/agentinfo.h"

#include <base/cluster/configure.h>

namespace base
{
    class DataTable;
}
namespace ms
{
    namespace map
    {

        class Map;
        class FamousCity;
        class WorldBoss;
        class Catapult;

        struct AllianceSimpleInfo {
            int64_t id = 0;    // ID
            std::string name;      // 名称
            std::string nickname;  // 简称
            int level = 0;      //联盟等级
            int bannerId = 0;   // 联盟旗帜ID
            int64_t leaderId = 0;  // 盟主ID
            std::string leaderName;  // 盟主昵称
            int64_t leaderHeadId = 0;  //
            int toatlActive = 0;        //联盟活跃
        };

        struct AllianceSimple {
            AllianceSimpleInfo info;
            std::vector<int64_t> members;  // 成员UID
            //aliance buff
            std::unordered_map<int, info::AllianceBuffInfo> allianceBuffs;

            // alliance science
            std::vector<info::AllianceScienceInfo> sciences;

            // 获得名城
            void OnAllianceOwnCity(FamousCity* city);
            // 失去名城
            void OnAllianceLoseCity(FamousCity* city, int atkAllianceId);

            // 获得名城占领权
            void OnAllianceOwnOccupyCity(FamousCity* city);
            // 失去名城占领权
            void OnAllianceLoseOccupyCity(FamousCity* city, int atkAllianceId);

            //alliance buff
            void AllianceBuffOpen(const info::AllianceBuffInfo& info);
            void AllianceBuffClosed(int buffId);
            //世界BOSS联盟奖励
            void OnKillWorldBoss(const std::string& nickname, const WorldBoss& boss) const;
            //击败名城NPC奖励
            void OnKillCityNpc(const std::string& nickname, const FamousCity& boss) const;
            //击败箭塔NPC奖励
            void OnKillCatapultNpc(const std::string& nickname, const Catapult& boss) const;
        };

        // 战斗的信息
        struct TeamInfo {
            TeamInfo() {}
            TeamInfo(int64_t _uid, const std::string& _nickname, int64_t _allianceId, const std::string& _allianceNickname)
                : uid(_uid), nickname(_nickname), allianceId(_allianceId), allianceNickname(_allianceNickname) {}
            int64_t uid = 0;
            std::string nickname;
            int64_t allianceId = 0;
            std::string allianceNickname;
        };

        struct BattleInfo {
            int id = 0;
            TeamInfo red;
            TeamInfo blue;
            int winTeam = 0;    // 1 red, 2 blue
            int battleType = 0;
            int64_t battleTime = 0;
            //
            bool isRedDelete = false;
            bool isBlueDelete = false;
            bool isDirty = false;
        };

        class Alliance
        {
        public:
            Alliance() {};
            ~Alliance() {};

            const std::unordered_map<int64_t, AllianceSimple>& alliances() const {
                return m_alliances;
            }
            const std::unordered_map<int64_t, AllianceSimpleInfo&>& players() const {
                return m_players;
            }

            AllianceSimple* FindAllianceSimple(int64_t aid);

            const AllianceSimpleInfo* GetAllianceSimpleInfoByAid(const int64_t aid) const {
                auto it = m_alliances.find(aid);
                if (it == m_alliances.end()) {
                    return nullptr;
                }
                const AllianceSimple& al = it->second;
                return &(al.info);
            }

            const AllianceSimpleInfo* GetAllianceSimpleInfoByUid(const int64_t uid) const {
                auto it = m_players.find(uid);
                if (it == m_players.end()) {
                    return nullptr;
                }
                AllianceSimpleInfo& info = it->second;
                return &(info);
            }

            const std::vector<int64_t> GetAlliancMembers(const int64_t aid) const {
                auto it = m_alliances.find(aid);
                if (it == m_alliances.end()) {
                    std::vector<int64_t> members;
                    return members;
                }
                const AllianceSimple& al = it->second;
                return al.members;
            }

            const std::list<BattleInfo*> GetAllianceBattleList(const int64_t aid) const {
                auto it = m_battles.find(aid);
                if (it == m_battles.end()) {
                    std::list<BattleInfo*> bList;
                    return bList;
                }
                return it->second;
            }

        public:
            // alliance
            void HandleMessage(const std::string& method, const base::DataTable& dt);
            void OnBattleUpdate(BattleInfo* info, bool sync = true);

        private:
            void CleanAlliances();
            void UpdateAlliance(AllianceSimple alliance, bool isCSUp = false);   // all alliance (contain members)
            void UpdateAllianceInfo(const AllianceSimpleInfo& info);     // just simple info
            void Disband(int64_t aid);
            // member
            void AddMember(int64_t aid, int64_t uid);
            void RemoveMember(int64_t aid, int64_t uid);
            //invited
            void RecordInvited(int64_t uid);
            //alliance buff
            void AllianceBuffOpen(int64_t aid, const info::AllianceBuffInfo& info);
            void AllianceBuffClosed(int64_t aid, int buffId);
	    //alliance CD
	    void setAllianceCdTimestamp(int64_t uid, int64_t allianceCdTimestamp);

        private:
            std::unordered_map<int64_t, AllianceSimple> m_alliances;   //  key:aid
            std::unordered_map<int64_t, AllianceSimpleInfo&> m_players;   // key:uid, from alliances_
            std::unordered_map<int64_t, std::list<BattleInfo*>> m_battles; // key:aid list size 50
        };
    }
}

#endif // MAP_ALLIANCE_H
