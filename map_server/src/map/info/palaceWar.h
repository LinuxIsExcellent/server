#ifndef MAP_INFO_PALACE_WAR_H
#define MAP_INFO_PALACE_WAR_H
#include <model/metadata.h>
#include <string>

namespace ms
{
    namespace map
    {
        namespace info
        {
            //历任国王信息
            struct SuccessiveKing {
                int64_t headId;
                std::string allianceNickname;
                std::string nickname;
                int n;              //第n任
                int64_t timestamp;
                std::string Serialize();
            };

            //王城争霸记录
            struct PalaceWarRecord {
                model::PalaceWarRecordType type;
                std::string param;
                int64_t timestamp;
            };

            //王城争霸打龙记录
            struct PalaceWarAttackDragonRecord {
                int64_t uid;
                int hurt;
                int64_t timestamp;

                static inline bool Comp(const PalaceWarAttackDragonRecord* lhs, const PalaceWarAttackDragonRecord* rhs) {
                    return lhs->hurt != rhs->hurt ? lhs->hurt > rhs->hurt : lhs->timestamp < rhs->timestamp;
                }
            };
        }
    }
}
#endif  //MAP_INFO_PALACE_WAR_H
