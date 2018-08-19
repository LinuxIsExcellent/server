#ifndef MAP_MSGQUEUE_MSGRECORD_H
#define MAP_MSGQUEUE_MSGRECORD_H

#include <model/metadata.h>
#include <model/tpl/drop.h>
#include <string>
#include <vector>
#include "../troop.h"
#include "../info/agentinfo.h"
#include "../info/property.h"

namespace base
{
    namespace cluster
    {
        class MessageOut;
    }
    class DataTable;
}

namespace ms
{
    namespace map
    {
        class Agent;
        namespace msgqueue
        {
            struct MsgAllianceInfo {
                std::string allianceName;
                std::string allianceNickname;
                int bannerId = 0;
            };

            struct MsgPvpDetail {
//                 int64_t uid = 0;
//                 std::string nickname;
//                 int64_t headId = 0;
                ArmyList armyList;
                TrapSet trapSet;

//                 int lordLevel = 0;
//                 int castleLevel = 0;

                //except lordLevel and castleLevel
                void SetDataTable(base::DataTable& table) const;
                void SetData(Troop& tp);
            };

            struct MsgPlayerInfo {
                virtual ~MsgPlayerInfo() {}
                int64_t uid = 0;
                std::string nickname;
                int64_t headId = 0;
                int lordLevel = 0;
                int64_t allianceId = 0;
                std::string allianceName;
                std::string allianceNickname;
                int allianceBannerId = 0;
                Point castlePos;
                int tplId = 0;

                virtual void SetDataTable(base::DataTable& table);

                void SetData(const Agent& agent);
                void SetUnitData(const Unit* unit);
            };

            struct MsgPvpPlayerInfo : public MsgPlayerInfo {
                virtual ~MsgPvpPlayerInfo() {}

//                 int turretLevel = 0;
//                 int turretKills = 0;
//                 info::Property property;
//                 std::vector<MsgPvpDetail> details;
//                 int64_t beginPower;

                    MsgPvpDetail detail;

                virtual void SetDataTable(base::DataTable& table);
                void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer);
                void Deserialize(rapidjson::GenericValue<rapidjson::UTF8< char >>& gv);
            };

            struct MsgPvpDragonInfo {
                int beginHp = 0;
                int endHp = 0;
                int maxHp = 0;

                void SetDataTable(base::DataTable& table);
            };

            struct MsgScoutDefenderInfo : public MsgPlayerInfo {
                Point targetPos;
                int targetTplId = 0;
                base::DataTable result;                     // 侦查结果
                virtual void SetDataTable(base::DataTable& table, bool isWin);
            };

            struct MsgWorldBossInfo {
                int tplId = 0;
                int beginArmyCount = 0;
                int endArmyCount = 0;
                int maxArmyCount = 0;

                void SetDataTable(base::DataTable& table);
            };

            // 战报新界面显示的新增内容
            struct MsgReportInfo {
                int attackerBeginArmyCount = 0;    // 攻击方开始兵力
                int attackerEndArmyCount = 0;       // 攻击方结束兵力
                int defenderBeginArmyCount = 0;     // 防御方开始兵力
                int defenderEndArmyCount = 0;      // 防御方结束兵力
                std::string posName = "";              // 战斗地点
                int isLang = 0;                     // 战斗地点是否读多语言 0: 不读   1:读
                int level = 0;                       // 野怪，资源地等级
                void SetDataTable(base::DataTable& table);
                void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer);
                void Deserialize(rapidjson::GenericValue<rapidjson::UTF8< char >>& gv); 
            };

            // base class
            class MsgRecord
            {
            public:
                virtual ~MsgRecord() {}

                // 序列化与反序列化消息数据原，保存到数据库 实现服务器重启时的数据还
                virtual std::string Serialize() = 0;
                virtual bool Deserialize(const std::string& data) = 0;

                // 把消息数据给前端服务器
                virtual void Send(Agent* agent);

                const int id() const {
                    return m_id;
                }

                const int64_t uid() const {
                    return m_uid;
                }

                const model::MessageQueueType type() const {
                    return m_type;
                }

                const int64_t createTime() const {
                    return m_createTime;
                }

                const bool ShouldResend() const;

                const bool IsFromDb() const {
                    return m_isFromDb;
                }

                void SetFromDb() {
                    m_isFromDb = true;
                }

            protected:
                MsgRecord(int id, int64_t uid, model::MessageQueueType type);

                int m_id;
                int64_t m_uid;
                model::MessageQueueType m_type;
                int64_t m_sendTime = 0;
                int64_t m_createTime = 0;

                bool m_isFromDb = false;
            };


            /*
             * MsgMarch
             */
            class MsgMarch : public MsgRecord
            {
            public:
                MsgMarch(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::MARCH) {}
                virtual ~MsgMarch() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(model::MapTroopType troopType, const Point& toPos, const ArmyList* armyList, const int troopId);

            private:
                model::MapTroopType m_troopType;
                Point m_toPos;
                ArmyList m_armyList;
                int m_troopId;
            };

            /*
             * MsgMarchBack
             */
            class MsgMarchBack : public MsgRecord
            {
            public:
                MsgMarchBack(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::MARCH_BACK) {}
                virtual ~MsgMarchBack() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(bool isArriveCastle, model::MapTroopType troopType, const Point& toPos, int food, int wood, int iron, int stone, const ArmyList* armyList);

            private:
                model::MapTroopType m_troopType;
                Point m_toPos;
                int m_food = 0;
                int m_wood = 0;
                int m_iron = 0;
                int m_stone = 0;
                ArmyList m_armyList;
                bool m_isArriveCastle = false;
            };

            /*
             * MsgTroopReachInvalid
             */
            class MsgTroopReachInvalid : public MsgRecord
            {
            public:
                MsgTroopReachInvalid(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::TROOP_REACH_INVALID) {}
                virtual ~MsgTroopReachInvalid() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(model::MapTroopType type);

            private:
                model::MapTroopType m_type;
            };

            /*
             * MsgAttackMonsterInvalid
             */
            class MsgAttackMonsterInvalid : public MsgRecord
            {
            public:
                MsgAttackMonsterInvalid(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::ATTACK_MONSTER_INVALID) {}
                virtual ~MsgAttackMonsterInvalid() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData();
            };

            /*
             * MsgCreateCampFixedFailed
             */
            class MsgCreateCampFixedFailed : public MsgRecord
            {
            public:
                MsgCreateCampFixedFailed(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::CREATE_CAMP_FIXED_FAILED) {}
                virtual ~MsgCreateCampFixedFailed() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData();
            };

            /*
             * MsgGatherResource
             */
            class MsgGatherResource : public MsgRecord
            {
            public:
                MsgGatherResource(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::GATHER_RESOURCE) {}
                virtual ~MsgGatherResource() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(const Point& toPos, int resTplId, const std::vector<model::tpl::DropItem>& dropItems, int gatherRemain, int64_t timestamp);

            private:
                Point m_pos;
                int m_resTplId = 0;
                int m_gatherRemain = 0;
                std::vector<model::tpl::DropItem> m_drops;
                int64_t m_timestamp = 0;
            };

            /*
             * MsgAttackResource
             **/
            class MsgAttackResource : public MsgRecord
            {
            public:
                MsgAttackResource(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::ATTACK_RESOURCE) {}
                virtual ~MsgAttackResource() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, 
                    const MsgPvpPlayerInfo& defender, int reportId, const Point& unitPos,  int tplId, 
                    const std::vector<model::tpl::DropItem>& dropItems, int64_t timestamp, int unitId, int troopId, const MsgReportInfo& reportInfo);

            private:
                model::AttackType m_winner;
                model::AttackType m_myAttackType;
                Point m_unitPos;
                std::vector<model::tpl::DropItem> m_drops;
                MsgPvpPlayerInfo m_attacker;
                MsgPvpPlayerInfo m_defender;
                MsgReportInfo  m_reportInfo;
                int64_t m_timestamp = 0;
                int m_unitId = 0;
                int m_troopId = 0;
                int m_tplId = 0;
                int m_reportId = 0;
            };
            
            /*
             * MsgAttackMonster
             **/
            class MsgAttackMonster : public MsgRecord
            {
            public:
                MsgAttackMonster(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::ATTACK_MONSTER) {}
                virtual ~MsgAttackMonster() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(model::AttackType winner, model::AttackType myAttackType, 
                    const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, const Point& unitPos, int tplId, 
                    int monsterHpBegin, int monsterHpEnd, const std::vector<model::tpl::DropItem>& dropItems, 
                    const ArmyList& armyList, bool isCaptive, int64_t timestamp, int unitId, int troopId, int reportId, const MsgReportInfo& reportInfo);

            private:
                model::AttackType m_winner;
                model::AttackType m_myAttackType;
                Point m_unitPos;
                int m_tplId = 0;
                std::vector<model::tpl::DropItem> m_drops;
                MsgPvpPlayerInfo m_attacker;
                MsgPvpPlayerInfo m_defender;
                MsgReportInfo m_reportInfo;
                ArmyList m_armyList;
                int64_t m_timestamp = 0;
                int m_unitId = 0;
                int m_troopId = 0;
                int m_monsterHpBegin = 0;
                int m_monsterHpEnd = 0;
                bool m_isCaptive = false;
                int m_reportId = 0;
            };

            /*
             * MsgCityBattle
             **/
            class MsgAttackCity : public MsgRecord
            {
            public:
                MsgAttackCity(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::ATTACK_CITY) {}
                virtual ~MsgAttackCity() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, 
                    const MsgPvpPlayerInfo& defender, int reportId, const Point& unitPos,  int tplId, 
                    const std::vector<model::tpl::DropItem>& dropItems, int64_t timestamp, int unitId, int troopId, const MsgReportInfo& reportInfo);

            private:
                model::AttackType m_winner;
                model::AttackType m_myAttackType;
                Point m_unitPos;
                int m_tplId;
                std::vector<model::tpl::DropItem> m_drops;
                MsgPvpPlayerInfo m_attacker;
                MsgPvpPlayerInfo m_defender;
                MsgReportInfo m_reportInfo;
                int64_t m_timestamp = 0;
                int m_unitId = 0;
                int m_troopId = 0;
                int m_reportId = 0;
            };


            /*
             * MsgCastleBattle
             **/
            class MsgAttackCastle : public MsgRecord
            {
            public:
                MsgAttackCastle(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::ATTACK_CASTLE) {}
                virtual ~MsgAttackCastle() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, 
                    const MsgPvpPlayerInfo& defender, int reportId, const Point& unitPos,  int tplId, int armyHurt,  
                    int food, int wood, int iron, int stone, int64_t timestamp, int foodRemove, int woodRemove, int ironRemove, 
                    int mithrilRemove, const std::vector<info::CollectInfo>& collectInfos, int64_t burnEndTs, int unitId, 
                    int troopId, const MsgReportInfo& reportInfo);

            private:
                model::AttackType m_winner;
                model::AttackType m_myAttackType;
                Point m_unitPos;
                int m_tplId;
                int m_armyHurt = 0;
                int m_food = 0;
                int m_wood = 0;
                int m_iron = 0;
                int m_stone = 0;
                MsgPvpPlayerInfo m_attacker;
                MsgPvpPlayerInfo m_defender;
                MsgReportInfo m_reportInfo;
                std::vector<info::CollectInfo> m_collectInfos;
                int64_t m_timestamp = 0;
                int m_unitId = 0;
                int m_troopId = 0;
                int m_foodRemove = 0;
                int m_woodRemove = 0;
                int m_ironRemove = 0;
                int m_stoneRemove = 0;
                int64_t m_burnEndTimestamp = 0;
                int m_reportId = 0;
            };

            /*
             * MsgBeatCastle
             **/
            class MsgBeatCastle : public MsgRecord
            {
            public:
                MsgBeatCastle(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::BEAT_CASTLE) {}
                virtual ~MsgBeatCastle() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender,
                    int food, int wood, int iron, int stone, int64_t timestamp, int foodRemove, int woodRemove, int ironRemove, 
                    int mithrilRemove, const std::vector<info::CollectInfo>& collectInfos, int64_t burnEndTs, int unitId, 
                    int troopId);

            private:
                model::AttackType m_winner;
                model::AttackType m_myAttackType;
                int m_food = 0;
                int m_wood = 0;
                int m_iron = 0;
                int m_stone = 0;
                MsgPvpPlayerInfo m_attacker;
                MsgPvpPlayerInfo m_defender;
                std::vector<info::CollectInfo> m_collectInfos;
                int64_t m_timestamp = 0;
                int m_unitId = 0;
                int m_troopId = 0;
                int m_foodRemove = 0;
                int m_woodRemove = 0;
                int m_ironRemove = 0;
                int m_stoneRemove = 0;
                int64_t m_burnEndTimestamp = 0;

            };

            /*
             * MsgCamp
             **/
            class MsgCamp : public MsgRecord
            {
            public:
                MsgCamp(int id, int64_t uid , model::MessageQueueType type) : MsgRecord(id, uid, type) {}
                virtual ~MsgCamp() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                virtual int battleType() = 0;
                virtual const char* EventName() = 0;

                void SetData(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, 
                    const MsgPvpPlayerInfo& defender, int reportId, const Point& unitPos, int tplId, int64_t timestamp, 
                    int unitId, int troopId, const MsgReportInfo& reportInfo);

            private:
                model::AttackType m_winner;
                model::AttackType m_myAttackType;
                Point m_unitPos;
                int m_tplId;
                MsgPvpPlayerInfo m_attacker;
                MsgPvpPlayerInfo m_defender;
                MsgReportInfo  m_reportInfo;
                int64_t m_timestamp = 0;
                int m_unitId = 0;
                int m_troopId = 0;
                int m_reportId = 0;
            };

            /*
            * MsgAttackCampFixed
            **/
            class MsgAttackCampFixed : public MsgCamp
            {
            public:
                MsgAttackCampFixed(int id, int64_t uid) : MsgCamp(id, uid, model::MessageQueueType::ATTACK_CAMP_FIXED) {}
                virtual ~MsgAttackCampFixed() {}

                virtual int battleType();
                virtual const char* EventName() {
                    return "onAttackCampFixed";
                }
            };

            /*
            * MsgOccupyCampFixed
            **/
            class MsgOccupyCampFixed : public MsgCamp
            {
            public:
                MsgOccupyCampFixed(int id, int64_t uid) : MsgCamp(id, uid, model::MessageQueueType::OCCUPY_CAMP_FIXED) {}
                virtual ~MsgOccupyCampFixed() {}

                virtual int battleType();
                virtual const char* EventName() {
                    return "onOccupyCampFixed";
                }
            };

            /*
            * MsgAttackCampTemp
            **/
            class MsgAttackCampTemp : public MsgCamp
            {
            public:
                MsgAttackCampTemp(int id, int64_t uid) : MsgCamp(id, uid, model::MessageQueueType::ATTACK_CAMP_TEMP) {}
                virtual ~MsgAttackCampTemp() {}

                virtual int battleType();
                virtual const char* EventName() {
                    return "onAttackCampTemp";
                }
            };

            /*
             * MsgAttackPalace
             **/
            class MsgAttackPalace : public MsgRecord
            {
            public:
                MsgAttackPalace(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::ATTACK_PALACE) {}
                virtual ~MsgAttackPalace() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int64_t timestamp, int unitId, int troopId, const MsgPvpDragonInfo& dragon);

            private:
                model::AttackType m_winner;
                model::AttackType m_myAttackType;
                MsgPvpPlayerInfo m_attacker;
                MsgPvpPlayerInfo m_defender;
                int64_t m_timestamp = 0;
                int m_unitId = 0;
                int m_troopId = 0;
                MsgPvpDragonInfo m_dragon;
            };

            /*
             * MsgAttackNeutralCastle
             **/
            class MsgAttackNeutralCastle : public MsgRecord
            {
            public:
                MsgAttackNeutralCastle(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::ATTACK_NEUTRAL_CASTLE) {}
                virtual ~MsgAttackNeutralCastle() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int64_t timestamp, int unitId, int troopId);

            private:
                model::AttackType m_winner;
                model::AttackType m_myAttackType;
                MsgPvpPlayerInfo m_attacker;
                MsgPvpPlayerInfo m_defender;
                int64_t m_timestamp = 0;
                int m_unitId = 0;
                int m_troopId = 0;
            };

            /*
             * MsgAttackCatapult
             **/
            class MsgAttackCatapult : public MsgRecord
            {
            public:
                MsgAttackCatapult(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::ATTACK_CATAPULT) {}
                virtual ~MsgAttackCatapult() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, const std::vector<model::tpl::DropItem>& dropItems, int reportId, int64_t timestamp, int unitId, int troopId, const MsgReportInfo& reportInfo);

            private:
                model::AttackType m_winner;
                model::AttackType m_myAttackType;
                MsgPvpPlayerInfo m_attacker;
                MsgPvpPlayerInfo m_defender;
                MsgReportInfo m_reportInfo;
                std::vector<model::tpl::DropItem> m_dropItems;
                int64_t m_timestamp = 0;
                int m_unitId = 0;
                int m_troopId = 0;
                int m_reportId = 0;
            };

            /*
             * MsgAttackGoblinCamp
             **/
            class MsgAttackGoblinCamp : public MsgRecord
            {
            public:
                MsgAttackGoblinCamp(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::ATTACK_GOBLIN_CAMP) {}
                virtual ~MsgAttackGoblinCamp() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int64_t timestamp, int unitId, int troopId);

            private:
                model::AttackType m_winner;
                model::AttackType m_myAttackType;
                MsgPvpPlayerInfo m_attacker;
                MsgPvpPlayerInfo m_defender;
                int64_t m_timestamp = 0;
                int m_unitId = 0;
                int m_troopId = 0;
            };

            /*
             * MsgScout
             **/
            class MsgScout : public MsgRecord
            {
            public:
                MsgScout(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::SCOUT_RESULT) {}
                virtual ~MsgScout() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(model::AttackType winner, model::AttackType myAttackType, const std::vector<model::WATCHTOWER_SCOUT_TYPE>& scoutTypes, const MsgPlayerInfo& attacker, const MsgScoutDefenderInfo& defender, int64_t timestamp);

            private:
                model::AttackType m_winner;
                model::AttackType m_myAttackType;
                std::vector<model::WATCHTOWER_SCOUT_TYPE> m_scoutTypes;
                MsgPlayerInfo m_attacker;
                MsgScoutDefenderInfo m_defender;
                int64_t m_timestamp = 0;
            };

            /*
            * MsgBuffRemove
            **/
            class MsgBuffRemove : public MsgRecord
            {
            public:
                MsgBuffRemove(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::BUFF_REMOVE) {}
                virtual ~MsgBuffRemove() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(model::BuffType type);

            private:
                model::BuffType m_type;
            };

            /*
             *  MsgResourceHelp
             **/
            class MsgResourceHelp : public MsgRecord
            {
            public:
                MsgResourceHelp(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::RESOURCE_HELP) {}
                virtual ~MsgResourceHelp() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(const MsgPlayerInfo& player, int food, int wood, int iron, int stone, model::AttackType myAttackType, int64_t timestamp);

            private:
                MsgPlayerInfo m_player;
                std::vector< model::tpl::DropItem > m_drops;
                model::AttackType m_myAttackType;
                int64_t m_timestamp = 0;
            };

            /*
             * MsgReinforcements
             **/
            class MsgReinforcements : public MsgRecord
            {
            public:
                MsgReinforcements(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::REINFORCEMENTS) {}
                virtual ~MsgReinforcements() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(const MsgPlayerInfo& player, model::AttackType myAttackType, const ArmyList& armyList, int64_t timestamp);

            private:
                MsgPlayerInfo m_player;
                ArmyList m_armyList;
                model::AttackType m_myAttackType;
                int64_t m_timestamp = 0;
            };

            /*
             * MsgNeutralCastleNoticeMail
             **/
            class MsgNeutralCastleNoticeMail: public MsgRecord
            {
            public:
                MsgNeutralCastleNoticeMail(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::NEUTRAL_CASTLE_NOTICE_MAIL) {}
                virtual ~MsgNeutralCastleNoticeMail() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(model::MailSubType mailSubType, const MsgAllianceInfo& alliance, int64_t timestamp, const std::string& param1);

            private:
                model::MailSubType m_mailSubType = (model::MailSubType)0;
                MsgAllianceInfo m_alliance;
                std::string m_param1;
                int64_t m_timestamp = 0;
            };

            /*
             * MsgMonsterSiege
             **/
            class MsgMonsterSiege : public MsgRecord
            {
            public:
                MsgMonsterSiege(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::MONSTER_SIEGE) {}
                virtual ~MsgMonsterSiege() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(int level, int food, int wood, int iron, int stone, int foodRemove, int woodRemove, int ironRemove, int stoneRemove, const std::vector< info::CollectInfo >& collectInfos, int64_t timestamp);

            private:
                int m_level = 0;
                std::vector< model::tpl::DropItem > m_drops;
                int m_foodRemove = 0;
                int m_woodRemove = 0;
                int m_ironRemove = 0;
                int m_stoneRemove = 0;
                std::vector<info::CollectInfo> m_collectInfos;
                int64_t m_timestamp = 0;
            };

            /*
             * MsgMonsterSiegeResourceGetBack
             **/
            class MsgMonsterSiegeResourceGetBack : public MsgRecord
            {
            public:
                MsgMonsterSiegeResourceGetBack(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::MONSTER_SIEGE_RESOURCE_GET_BACK) {}
                virtual ~MsgMonsterSiegeResourceGetBack() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(int level, int food, int wood, int iron, int stone, int64_t timestamp);

            private:
                int m_level = 0;
                std::vector< model::tpl::DropItem > m_drops;
                int64_t m_timestamp = 0;
            };

            /*
             * MsgBeAttackedByCatapult
             **/
            class MsgBeAttackedByCatapult : public MsgRecord
            {
            public:
                MsgBeAttackedByCatapult(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::BE_ATTACK_BY_CATAPULT) {}
                virtual ~MsgBeAttackedByCatapult() {}
                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(const ArmyList& dieList, bool isCaptive);

            private:
                ArmyList m_dieList;
                bool m_isCaptive = false;
            };

            /*
             * MsgCastleRebuild
             **/
            class MsgCastleRebuild : public MsgRecord
            {
            public:
                MsgCastleRebuild(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::CASTLE_REBUILD) {}
                virtual ~MsgCastleRebuild() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(Point castlePos, int64_t timestamp);

            private:
                Point m_castlePos;
                int64_t m_timestamp = 0;
            };

            /*
             * MsgKillDragonRank
             **/
            class MsgKillDragonRank : public MsgRecord
            {
            public:
                MsgKillDragonRank(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::KILL_DRAGON_RANK) {}
                virtual ~MsgKillDragonRank() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(int rank, int64_t timestamp);

            private:
                int m_rank = 0;
                int64_t m_timestamp = 0;
            };

            /*
             * MsgKillDragonLastAttack
             **/
            class MsgKillDragonLastAttack : public MsgRecord
            {
            public:
                MsgKillDragonLastAttack(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::KILL_DRAGON_LAST_ATTACK) {}
                virtual ~MsgKillDragonLastAttack() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(int64_t timestamp);

            private:
                int64_t m_timestamp = 0;
            };

            /*
             *  MsgExploreMysteriousCity
             **/
            class MsgExploreMysteriousCity : public MsgRecord
            {
            public:
                MsgExploreMysteriousCity(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::EXPLORE_MYSTERIOUS_CITY) {}
                virtual ~MsgExploreMysteriousCity() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(const std::vector<model::tpl::DropItem>& drops);

            private:
                std::vector<model::tpl::DropItem> m_drops;
            };

            /*
             * MsgCityDefenseUpdate
             */
            class MsgCityDefenseUpdate : public MsgRecord
            {
            public:
                MsgCityDefenseUpdate(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::CITYDEFENSE_UPDATE) {}
                virtual ~MsgCityDefenseUpdate() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(int cityDefense);

            private:
                int m_cityDefense = 0;
            };

            struct MsgCityInfo {
                Point targetPos;
                int targetTplId = 0;
                int cityId = 0;
                std::string cityName;

                int64_t allianceId = 0;
                std::string allianceName;
                std::string allianceNickname;
                int allianceBannerId = 0;

                int troopCount = 0;
                int armyTotalCount = 0;

                void SetData(FamousCity* city);
                void SetDataTable(base::DataTable& table);
                void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer);
                void Deserialize(rapidjson::GenericValue<rapidjson::UTF8< char >>& gv);
            };


            struct MsgCityPatrolEvent
            {
                int id = 0;
                std::vector<model::tpl::DropItem> drops;
                std::vector<model::tpl::DropItem> removes;
                ArmyList armyList;
                
                void SetDataTable(base::DataTable& table) const;
            };

            class MsgCityPatrol :  public MsgRecord
            {
            public:
                MsgCityPatrol(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::CITY_PATROL) {}
                virtual ~MsgCityPatrol() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(const MsgCityInfo& cityInfo, const std::vector<MsgCityPatrolEvent> &events, int64_t timestamp);

            private:

                MsgCityInfo m_cityInfo;
                std::vector<MsgCityPatrolEvent> m_events;
                int64_t m_timestamp = 0;
            };

            /*
             * MsgAttackWorldBoss
             **/
            class MsgAttackWorldBoss : public MsgRecord
            {
            public:
                MsgAttackWorldBoss(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::ATTACK_WORLDBOSS) {}
                virtual ~MsgAttackWorldBoss() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int reportId, int64_t timestamp, int unitId, int troopId, const MsgReportInfo& reportInfo);

            private:
                model::AttackType m_winner;
                model::AttackType m_myAttackType;
                MsgPvpPlayerInfo m_attacker;
                MsgPvpPlayerInfo m_defender;
                MsgReportInfo m_reportInfo;
                int64_t m_timestamp = 0;
                int m_unitId = 0;
                int m_troopId = 0;
                int m_reportId = 0;
            };

            /*
             * MsgAtackWorldBossEnd
             */
            class MsgAttackWorldBossEnd : public MsgRecord
            {
            public:
                MsgAttackWorldBossEnd(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::ATTACK_WORLDBOSS_END) {}
                virtual ~MsgAttackWorldBossEnd() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, const MsgWorldBossInfo& worldBossInfo, const int64_t timestamp, int unitId, int troopId);

            private:
                MsgPvpPlayerInfo m_attacker;
                MsgPvpPlayerInfo m_defender;
                MsgWorldBossInfo m_worldboss;
                int64_t m_timestamp = 0;
                int m_unitId = 0;
                int m_troopId = 0;
            };

             /*
             * MsgAttackCityEnd
             */
            class MsgAttackCityEnd : public MsgRecord
            {
            public:
                MsgAttackCityEnd(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::ATTACK_CITY_END) {}
                virtual ~MsgAttackCityEnd() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, const MsgWorldBossInfo& worldBossInfo, const std::vector<model::tpl::DropItem>& dropItems,  const ArmyList& armyList, const int64_t timestamp, int unitId, int troopId);

            private:
                MsgPvpPlayerInfo m_attacker;
                MsgPvpPlayerInfo m_defender;
                MsgWorldBossInfo m_worldboss;
                std::vector<model::tpl::DropItem> m_dropItems;
                ArmyList m_armyList;
                int64_t m_timestamp = 0;
                int m_unitId = 0;
                int m_troopId = 0;
            };

            /*
             * MsgAttackCatapultEnd
             */
            class MsgAttackCatapultEnd : public MsgRecord
            {
            public:
                MsgAttackCatapultEnd(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::ATTACK_CATAPULT_END) {}
                virtual ~MsgAttackCatapultEnd() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, const MsgWorldBossInfo& worldBossInfo, const int64_t timestamp, int unitId, int troopId);

            private:
                MsgPvpPlayerInfo m_attacker;
                MsgPvpPlayerInfo m_defender;
                MsgWorldBossInfo m_worldboss;
                int64_t m_timestamp = 0;
                int m_unitId = 0;
                int m_troopId = 0;
            };


            /*
             * MsgTransport
             */
            class MsgTransport : public MsgRecord
            {
            public:
                MsgTransport(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::TRANSPORT) {}
                virtual ~MsgTransport() {}

                virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(model::TransportType myTransportType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int food, int wood, int iron, int stone, bool isSuccess, const int64_t timestamp);

            private:
                MsgPvpPlayerInfo m_attacker;
                MsgPvpPlayerInfo m_defender;
                int m_food = 0;
                int m_wood = 0;
                int m_iron = 0;
                int m_stone = 0;
                bool m_isSuccess = false;
                model::TransportType m_myTransportType;

                int64_t m_timestamp = 0;
            };

            /*
             * MsgCompensate
             **/
            class MsgCompensate : public MsgRecord
            {
            public:
                MsgCompensate(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::COMPENSATE) {}
                virtual ~MsgCompensate() {}

                 virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(int food, int wood, int stone, int iron, const int64_t timestamp);

            private:
                int m_food = 0;
                int m_wood = 0;
                int m_stone = 0;
                int m_iron = 0;
                std::vector< model::tpl::DropItem > m_drops;
                int64_t m_timestamp = 0;
            };

            /*
             *MsgCityDefenerFill
             **/
            class MsgCityDefenerFill : public MsgRecord
            {
            public:
                MsgCityDefenerFill(int id, int64_t uid) : MsgRecord(id, uid, model::MessageQueueType::CASTLE_DEFENER_FILL) {}
                virtual ~MsgCityDefenerFill() {}

                 virtual std::string Serialize();
                virtual bool Deserialize(const std::string& data);
                virtual void Send(Agent* agent);

                void SetData(const ArmyList* fillList,const int64_t timestemp);

            private:
                ArmyList m_armyList;
                int64_t m_timestemp = 0;
            };
        }
    }
}

#endif // MSGRECORD_H
