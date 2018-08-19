#ifndef MAP_RESNODE_H
#define MAP_RESNODE_H
#include "unit.h"
#include <base/event/dispatcher.h>

namespace ms
{
    namespace map
    {
        class Troop;

        // 资源点
        class ResNode : public Unit
        {
        public:
            
            static constexpr int RESOURCE_HURTED_KEEPTIME = 3 * 60 * 60; //残田“现场保护”时间
            
        public:
            ResNode(int id, const MapUnitResourceTpl* tpl, const Point& bornPoint);
            virtual ~ResNode();

            virtual void Init() override;

            const Point& bornPoint() const {
                return m_bornPoint;
            }

            int Capacity() const {
                return m_tpl->capacity;
            }

            int canGather() const {
                return m_canGather;
            }

            int hadGather() const {
                return m_hadGather;
            }

            virtual Troop* troop() override {
                return m_troop;
            }
            
            virtual ArmyList* DefArmyList() override{
                return &m_armyList;
            }

            int64_t hurtKeepTime() {
                return m_hurtKeepTime;
            }
            
            bool IsHurt() {
                return m_hadGather > 0;
            }
            
            bool IsHurtKeep() {
                 int64_t now = g_dispatcher->GetTimestampCache();
                return m_hurtKeepTime > 0 && now < m_hurtKeepTime;
            }
            
            int troopId() const;
            int64_t uid() const override;
            int64_t allianceId() const override;
            const std::string nickname() const;

            int64_t noviceuid() {
                return m_noviceuid;
            }
            
            ArmyList& armyList() {
                return m_armyList;
            }

            float gatherSpeed() const;
            float resourceLoad() const;

            int leftGather() const;
            int hasGather() const;
            float troopGatherSpeed() const;
            int CalculateGatherTime();
            int ReCalculateGatherTime();
            int TakeGather();
            model::SpecialPropIdType DropItemType();
            void Gather();
            bool CanOccupy(Troop* troop);
            void ClearTroop();
            virtual void Occupy(Troop* troop) override;

            //行军到达
            virtual void OnTroopReach(Troop* troop) override{
            }
            //行军离开
            virtual void OnTroopLeave(Troop* troop) override;

            virtual std::string Serialize() override;
            virtual bool Deserialize(std::string data) override;

            virtual void SetTpl(const MapUnitTpl* tpl) override;
            void SetNoviceuid(int64_t noviceuid) {
                m_noviceuid = noviceuid;
            }

            void Reset();
        
            void CheckHurt();
            
            virtual void FinishDeserialize() override;
            
        protected:
            const MapUnitResourceTpl* m_tpl = nullptr;
            int64_t m_noviceuid = 0;                            // 新手引导对应uid
            const Point m_bornPoint;
            int m_canGather = 0;
            int m_hadGather = 0;    //资源战后保存已采集资源
            int64_t m_hurtKeepTime = 0;    //残田保护时间
            Troop* m_troop = nullptr;
            ArmyList m_armyList;    //默认驻守军(包括陷阱)
        };
    }
}

#endif // RESNODE_H
