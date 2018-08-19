#ifndef MAP_INFI_DBINFO_H
#define MAP_INFI_DBINFO_H

#include <string>
#include <model/metadata.h>

namespace ms
{
    namespace map
    {
        namespace info
        {
            enum class DataClassify
            {
                AGENT = 1,
                UNIT,
                TROOP,
                MSGQUEUE,
                FIGHT_HISTORY,
                MONSTER_SIEGE,

                END,
            };

            struct DbInfo {
            public:
                DbInfo(info::DataClassify c) : m_classify(c) {}
                virtual ~DbInfo() {}

                info::DataClassify classify() const {
                    return m_classify;
                }

            private:
                info::DataClassify m_classify = info::DataClassify::END;
            };


            struct DbAgentInfo : DbInfo {
                DbAgentInfo() : DbInfo(info::DataClassify::AGENT) {}

                int64_t uid;
                std::string data;
            };

            struct DbUnitInfo : DbInfo {
                DbUnitInfo() : DbInfo(info::DataClassify::UNIT) {}

                int id;
                int tplid;
                int x;
                int y;
                int64_t uid = 0;
                std::string data;
            };

            struct DbTroopInfo : DbInfo {
                DbTroopInfo() : DbInfo(info::DataClassify::TROOP) {}

                int id;
                int64_t uid;
                std::string data;
                bool is_delete = false;
                int64_t create_time = 0;
            };

            struct DbMsgQueueInfo : DbInfo {
                DbMsgQueueInfo() : DbInfo(info::DataClassify::MSGQUEUE) {}

                int64_t uid;
                int mid;
                model::MessageQueueType type;
                std::string data;
                int64_t create_time = 0;
            };

            struct DbFightHistoryInfo : DbInfo {
                DbFightHistoryInfo() : DbInfo(info::DataClassify::FIGHT_HISTORY) {}

                int id;
                int64_t redUid;
                std::string redNickname;
                int64_t redAllianceId;
                std::string redAllianceNickname;
                int64_t blueUid;
                std::string blueNickname;
                int64_t blueAllianceId;
                std::string blueAllianceNickname;
                bool isRedDelete;
                bool isBlueDelete;
                int winTeam;
                int battleType;
                int64_t battleTime;
            };

            struct DbMonsterSiegeInfo : DbInfo {
                DbMonsterSiegeInfo() : DbInfo(info::DataClassify::MONSTER_SIEGE) {}

                int64_t uid;
                int32_t level;
                int32_t food;
                int32_t wood;
                int32_t iron;
                int32_t stone;
                int64_t timestamp;
            };

            /*** game db begin ***/
            struct DbUserInfo {
                int agent;
                std::string channel;
                std::string username;
            };
            /*** game db end ***/
        }
    }
}

#endif // MAP_INFI_DBINFO_H
