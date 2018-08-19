#ifndef MAP_TROOP_H
#define MAP_TROOP_H

#include <unordered_map>
#include "point.h"
#include <model/metadata.h>
#include <model/dirtyflag.h>
#include <base/framework.h>
#include <base/objectmaintainer.h>
#include <base/event/dispatcher.h>
#include <lua/llimits.h>
#include "armylist.h"
#include "troopImpl.h"
#include "trapset.h"

namespace model
{
    namespace tpl
    {
        struct DropItem;
    }
}

namespace base
{
    class TimeoutLinker;
}

namespace ms
{
    namespace map
    {

        class Unit;
        class ResNode;
        class Map;
        class Agent;
        class TroopImpl;
        namespace qo
        {
            class CommandTroopUpdate;
        }
        class Troop;
        class TroopFactory
        {
        public:
            static TroopFactory& instance();

            Troop* CreateTroop(int id,const std::string& data);
            Troop* CreateTroop(int id, Agent* agent, model::MapTroopType type, const Point& from, const Point& to, int teamId);
        };

        //行军
        class Troop : public model::Dirty
        {
        protected:
            Troop(int id);
            Troop(int id, Agent* agent, model::MapTroopType type, const Point& from, const Point& to, float speed, int teamId);
        
        public:
            virtual ~Troop();

            int id() const {
                return m_id;
            }

            int64_t uid() const;

            Agent& agent() const {
                return *m_agent;
            }
            Agent* transportAgent() const {
                return m_transportAgent;
            }

            void SetTransportAgent(Agent* agent) {
                m_transportAgent = agent;   
            }

            const int32_t transportRecordId1()
            {
                return m_transportId1;
            }

            const int32_t transportRecordId2()
            {
                return m_transportId2;   
            }

            void SetTransportRecordId1(int32_t id) {
                m_transportId1 = id;
            }

            void SetTransportRecordId2(int32_t id) {
                m_transportId2 = id;
            }

            model::MapTroopType type() const {
                return m_type;
            }

            void SetType(model::MapTroopType type) {
                m_type = type;
                SwitchTroopImpl();
            }

            model::MapTroopType parentType() const {
                return m_parentType;
            }
            
             void SetParentType(model::MapTroopType type) {
                m_parentType = type;
            }
            
            void ResetParent() {
                if (m_parentType ==  model::MapTroopType::CAMP_FIXED || m_parentType ==  model::MapTroopType::ASSIGN) {
                    m_parentType = model::MapTroopType::UNKNOWN;
                    m_source = m_parentSource;
                    m_parentSource = Point(-1, -1);
                }
            }
            
            model::MapTroopState state() const {
                return m_state;
            }

             void SetState(model::MapTroopState state) {
                m_state = state;
            }
            
            bool IsMarching() const {
                return m_state == model::MapTroopState::MARCH;
            }

            bool IsReach() const {
                return m_state == model::MapTroopState::REACH;
            }

            bool IsGoHome() const {
                return m_state == model::MapTroopState::GO_HOME;
            }

            bool IsRemove() const {
                return m_state == model::MapTroopState::REMOVE;
            }

            float speed() const {
                return m_speed;
            }

             void SetSpeed(float speed)  {
                m_speed = speed;
            }
            
            float initialSpeed() const {
                return m_initialSpeed;
            }

            void SetInitialSpeed(float initialSpeed)  {
                m_initialSpeed = initialSpeed;
            }

            float speedUp() const {
                return m_speedUp;
            }

            void SetSpeedUp(float speedUp)  {
                m_speedUp = speedUp;
            }

            const Point& from() const {
                return m_from;
            }

            const Point& to() const {
                return m_to;
            }

            const Point& source() const {
                return m_source;
            }
            
            const FloatPoint& currentPos() const {
                return m_currentPos;
            }

            int64_t beginTimestamp() const {
                return m_beginTimestamp;
            }

            void SetBeginTimestamp(int64_t beginTimestamp) {
                m_beginTimestamp = beginTimestamp;
            }
            
            int64_t endTimestamp() const {
                return m_endTimestamp;
            }
            
            void SetEndTimestamp(int64_t endTimestamp) {
                m_endTimestamp = endTimestamp;
            }

            int leftSecond() const {
                int64_t now = g_dispatcher->GetTimestampCache();
                return m_endTimestamp != 0 && m_endTimestamp > now ? m_endTimestamp - now : 0;
            }

            int totalSecond() const {
                return m_endTimestamp > m_beginTimestamp ? m_endTimestamp - m_beginTimestamp : 0;
            }

            int64_t allianceId() const;
            int allianceLv() const;
            const std::string& allianceNickname() const;
            const std::string& allianceName() const;
            int allianceBannerId() const;
            const std::string& nickname() const;
            int64_t headId() const;
            int lordLevel() const;

            int teamId() {
                return m_teamId;
            }
            ArmyList* armyList();
            int GetArmyTotal();
            int GetLoadTotal();

            int gold() const {
                return m_gold;
            }

            int food() const {
                return m_food;
            }

            int wood() const {
                return m_wood;
            }

            int iron() const {
                return m_iron;
            }

            int stone() const {
                return m_stone;
            }

            int reachTplId() const {
                return m_reachTplId;
            }
            
            void SetReachTplId(int reachTplId) {
                m_reachTplId = reachTplId;
            }

            int gatherRemain() const{
                return m_gatherRemain;
            }

            void SetGatherRemain(int gatherRemain) {
                m_gatherRemain = gatherRemain;
            }

            const std::vector<model::tpl::DropItem>& dropItems() const {
                return m_dropItems;
            }

            int64_t createTime() const {
                return m_createTime;
            }

            float gatherSpeed() const {
                return m_gatherSpeed;
            }

            void SetGatherSpeed(float gatherSpeed) {
                m_gatherSpeed = gatherSpeed;
            }
            
            int gatherLoad() const {
                return m_gatherLoad;
            }

            void SetGatherLoad(int gatherLoad) {
                m_gatherLoad = gatherLoad;
            }
            
            int64_t removeTime() const {
                return m_removeTime;
            }

            bool isSameAlliance(int64_t aid) const {
                return allianceId() == aid;
            }

            void SetPoint(const Point& from, const Point& to) {
                m_from = from;
                m_to = to;
                SetDirty();
            }

            void SetCurrentPos(const FloatPoint& fp) {
                m_currentPos = fp;
                SetDirty();
            }

            void ClearResources() {
                m_food = 0;
                m_wood = 0;
                m_iron = 0;
                m_stone = 0;
                m_gold = 0;
                SetDirty();
            }

            bool isOccupyCity() {
                return m_isOccupyCity;
            }

            void SetIsOccupyCity(bool isOccupy) {
                m_isOccupyCity = isOccupy;
            }

            int power();

            void Debug();

        public:
            //定时更新
            void OnUpdate(int64_t tick, int32_t span);

            void AddResource(model::ResourceType type, int count);
            void AddDropItem(int tplid, int count);
            void AddDropItem(std::vector<model::tpl::DropItem>& drops);
            int DropItemCount();
            void ClearDropItem();

            void March();
            model::MarchErrorNo ReMarch(model::MapTroopType type, const Point& to);
            bool GoHome();
            void ImmediateReturn();

            bool Recall();
            bool AdvancedRecall();
            bool SpeedUp(float speedUp);

            //遣返
            bool Repatriate();

            void OnBattleEnd();

            void NoticeTroopUpdate(bool resetCross = false);
            // 属性改变
            void OnPropertyChanged();
            bool Gather(ResNode* res);
            bool InitGather(ResNode* res);

            //获取兵种最低基础速度
            float GetBaseSpeed();
            //获取速度属性加成
            float GetSpeedAddition();
            //获取采集加成
            float GetGatherSpeedAddition(model::MapUnitType type);

            //计算出征时间速度等 返回时间
            int CalculateMarchTime(const Point& start, const Point& end);
            //重新计算当前行军所有状态
            void ReCalculateMarch();
            void ReCalculateMarch(float speedUp);
            void ResetLinker();
            void Remove();
            
            std::string Serialize() const;
            bool Deserialize(rapidjson::Document& doc);

            //TroopImpl
            void FinishDeserialize();
            //攻击失败
            void OnAttackFailed(Unit* unit);
            //攻击成功
            void OnAttackWin(Unit* unit);
            //防守失败
            void OnDefenceFailed(Unit* unit);
            //防守成功
            void OnDefenceWin(Unit* unit);
        
        protected:
            
            void OnReach();
            bool OnReachProc(Unit* to);
            void OnLeave();
            void OnArriveHome();
            void OnBack();
            //到达失效
            void OnReachInvalid();
            // 到达返回(目标失去，自己迁城)
            void OnMarchBack();
            
            void SwitchTroopImpl();

        protected:
            int m_id = 0;
            Agent* m_agent = nullptr;
            Agent* m_transportAgent = nullptr;
            model::MapTroopType m_parentType = model::MapTroopType::UNKNOWN;
            model::MapTroopType m_type;
            model::MapTroopState m_state = (model::MapTroopState)0;
            int64_t m_removeTime = 0;
            Point m_from;
            Point m_to;
            Point m_parentSource;
            Point m_source;
            float m_speed = .0f;                  //当前速度
            float m_initialSpeed = .0f;                        // 初始速度
            float m_speedUp = .0f;                          // 加速
            FloatPoint m_currentPos;
            int64_t m_beginTimestamp = 0;
            int64_t m_endTimestamp = 0;
            int m_teamId = 0;
            base::ObjectMaintainer m_maintainer;
            base::TimeoutLinker* m_linker = nullptr;
            int m_reachTplId = 0;   //回城后发邮件时拿这个值
            int m_gold = 0;
            int m_food = 0;
            int m_wood = 0;
            int m_iron = 0;
            int m_stone = 0;
            std::vector<model::tpl::DropItem> m_dropItems;
            int64_t m_createTime = 0;
            int64_t m_lockUid = 0;
            float m_gatherSpeed = .0f;          //采集速度
            int m_gatherLoad = 0;               //采集负重
            int m_gatherRemain = 0;                         // 采集剩余
            bool m_isOccupyCity = false;

            // 对应运输记录的两个Id
            int32_t m_transportId1 = 0;
            int32_t m_transportId2 = 0;

            TroopImpl* m_troopImpl = nullptr;

            DefaultTroopImpl m_defaultTroopImpl;
            TroopToRes m_troopToRes;
            TroopToMonster m_troopToMonster;
            TroopToCity m_troopToCity;
            TroopToCastle m_troopToCastle;
            TroopToCampTemp m_troopToCampTemp;
            TroopToCampFixed m_troopToCampFixed;
            TroopToScout m_troopToScout;
            TroopToReinforcements m_troopToReinforcements;
            TroopToCatapult m_troopToCatapult;
            TroopToWorldBoss m_troopToWolrdBoss;
            TroopToTransport m_troopToTransport;

            friend class qo::CommandTroopUpdate;
            friend class TroopFactory;
        };
    }
}

#endif // MAP_TROOP_H

