#ifndef MAP_INFO_TURRETINFO_H
#define MAP_INFO_TURRETINFO_H
#include <string>
#include <base/gateway/packet_base.h>
#include <base/data.h>
#include <model/metadata.h>

namespace base
{
    namespace gateway
    {
        class PacketOutBase;
    }
}

namespace ms
{
    namespace map
    {
        namespace info
        {
            struct CollectInfo {
                int gridId;
                model::ResourceType type;
                int output;     //产出(每小时)
                int capacity;   //最大容量
                int64_t timestamp;
            };

            struct BuffInfo {
                model::BuffType type;
                int64_t endTimestamp;
                int param1;     //注意百分比的处理
                base::DataTable attr;
            };
            
            struct AllianceBuffInfo {
                int buffId;
                model::AllianceBuffType type;
                int64_t endTimestamp;
                base::DataTable attr;
                int param1;
            };

            struct AllianceScienceInfo {
                int groupId = 0;
                int tplId = 0;
                int level = 0;
            };

            struct EquipInfo {
                int tplid;
                std::string data;

                void WriteToPacketOut(base::gateway::PacketOutBase& pktout) const {
                    pktout << tplid;
                }
            };

            struct VipInfo {
                int level = 0;
            };
            
            struct RankInfo {
                model::RangeType type;
                int rank = 0;
            };
        }
    }
}

#endif  //MAP_INFO_TURRETINFO_H
