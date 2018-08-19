#ifndef MAP_FamousCity_H
#define MAP_FamousCity_H
#include "unit.h"
#include "../troop.h"
#include <base/event/dispatcher.h>

namespace ms
{
    namespace map
    {
        class FamousCity : public Unit
        {
        public:
            FamousCity(int id, const ms::map::tpl::MapUnitFamousCityTpl* tpl, const Point& bornPoint);
            virtual ~FamousCity();

            virtual void Init() override;

            int troopId() const ;
            int64_t uid() const override;
            int64_t allianceId() const override;
            const std::string nickname() const;

            const MapCityTpl* cityTpl() {
                return m_cityTpl;
            }

            int GetDropId();

            const std::list<Troop*>& troops() const{
                return m_troops;
            }

            std::list<ArmyList> armyLists()
            {
                return m_armyLists;
            }

            const Point& bornPoint() const {
                return m_bornPoint;
            }

            virtual Troop* troop() override {
                return m_troop;
            }

            virtual ArmyList* DefArmyList() {
                if (m_armyLists.empty()) {   
                    return nullptr;
                }
                else
                {
                    return &m_armyLists.back();
                }
            }

            TrapSet& trapSet() {
                return m_trapSet;
            }

            int turretAtkPower() {
                return m_turretAtkPower;
            }

            int cityDefense() {
                return m_cityDefense;
            }

            int cityDefenseMax() {
                return m_cityDefenseMax;
            }

            int cityDefenseRecover() {
                return m_cityDefenseRecover;
            }

            void AddCityDefense(int add);

            bool isOccupy() {
                return m_isOccupy;
            }

            int occupyLeftTime() {
                return m_occupyLeftTime;
            }

            int64_t occupyAllianceId() {
                return m_occupyallianceId;
            }

            //npc刷新时间
            int npcRefreshTime() {
                auto time = m_armyRecoveryTime -  g_dispatcher->GetTimestampCache();
                if (time < 0) {return 0;}
                return time;
            }

            model::CastleState state() const {
                return m_state;
            }

            void SetState(model::CastleState state) {
                m_state = state;
                SetDirty();
            }

            void Refresh();

            //virtual 
            // 是否可驻守
            virtual bool CanGarrison(Troop* troop);
            // 是否可占领
            virtual bool CanOccupy(Troop* troop);
            //是否
            void OnOccupyEnd();
            void Occupy(Troop* troop);
            void Garrison(Troop* troop);
            void UpdateDefTroop();
            bool SwitchTroop();

            //npc恢复
            void OnNpcReset();
            void HurtNpc();
            //主权改变
            void ChangeSovereign(int64_t old_allianceId , int64_t new_allianceId);
            //占领权改变
            void ChangeOccupy(int64_t old_allianceId , int64_t new_allianceId);

            virtual void OnTroopLeave(Troop* troop) override;

            virtual std::string Serialize() override;
            virtual bool Deserialize(std::string data) override;
            virtual void FinishDeserialize() override;
            virtual void SetTpl(const MapUnitTpl* tpl) override;

            void OnIntervalUpdate(size_t updateSeconds);
            void StartRecoverCityDefense();
            void UpdateCityDefRecover();

            void ClearNpc();
            virtual void ClearAllDefenders();
            /*virtual void OnTroopOcuppy(Troop* troop) override;
            virtual void OnTroopGarrison(Troop* troop) override;*/

            /*virtual void OnTroopOcuppy(Troop* troop) override;
            virtual void OnTroopGarrison(Troop* troop) override;*/

            const int CurrentArmyTotal();

            const int ArmyListsTotal()
            {
                return m_armyListsTotal;
            }

        protected:
            const MapCityTpl* m_cityTpl = nullptr;
            const MapUnitFamousCityTpl* m_tpl = nullptr;
            const Point m_bornPoint;
            Troop* m_troop = nullptr;
            std::list<ArmyList> m_armyLists;      //守军列表
            int m_armyListsTotal;                   //NPC驻军总数
            TrapSet m_trapSet;                              //陷阱
            int m_turretAtkPower = 0;                       //箭塔

            std::list<Troop*> m_troops;
            std::list<int> m_troopIds;

            int m_cityDefenseMax = 0;
            int m_cityDefense = 0;                          //城防值
            bool m_isOccupy = false;                        //是否被占领过
            int64_t m_cityDefenseRecoverTime = 0;           //城防值恢复时间
            size_t m_updateSeconds = 0;
            int m_cityDefenseRecover = 0;                   //城防值恢复量

            int64_t m_allianceId = 0;                       //联盟Id
            int64_t m_occupyallianceId = 0;                 //占领时的联盟Id 
            int64_t m_armyRecoveryTime = 0;                 //npc恢复时间

        private:
            static constexpr int TAG_OCCUPY_END = 1;
            static constexpr int TAG_NPC_RESET = 2;
            model::CastleState m_state = model::CastleState::NORMAL; //城市状态复用
            int64_t m_occupyLeftTime = 0;                    //攻占时间
        };
    }
}

#endif // MAP_FamousCity_H
