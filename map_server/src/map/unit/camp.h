#ifndef MAP_CAMP_H
#define MAP_CAMP_H
#include "unit.h"
#include "../troop.h"
#include <base/event/dispatcher.h>

namespace ms
{
    namespace map
    {
        class CampTemp : public Unit
        {
        public:
            CampTemp(int id, const ms::map::tpl::MapUnitCampTempTpl* tpl, Troop* troop);
            virtual ~CampTemp();

            virtual void Init() override
            {
                //SetDirty();
            }

            int troopId() const {
                if (!m_troop) {
                    return 0;
                }
                return m_troop->id();
            }

            virtual int64_t uid() const override{
                if (!m_troop) {
                    return 0;
                }
                return m_troop->uid();
            }

            const std::string nickname() const {
                if (!m_troop) {
                    return "";
                }
                return m_troop->nickname();
            }
            
            virtual int64_t allianceId() const override;

            virtual Troop* troop() override {
                return m_troop;
            }

            bool CanAttack(Troop* troop) {
                return (!m_troop || (m_troop->uid() != troop->uid() && (m_troop->allianceId() == 0 || m_troop->allianceId() != troop->allianceId())));
            }

            virtual bool RemoveSelf() override;
            virtual void OnTroopLeave(Troop* troop) override;

        private:
            Troop* m_troop = nullptr;
        };

        //行营
        class CampFixed : public Unit
        {
        public:
            CampFixed(int id, const ms::map::tpl::MapUnitCampFixedTpl* tpl, Troop* troop);
            virtual ~CampFixed();

            virtual void Init() override
            {
                //SetDirty();
            }

            int troopId() const {
                if (!m_troop) {
                    return 0;
                }
                return m_troop->id();
            }

            int bindTroopId() const {
                return m_bindTroopId;
            }

            const std::list<Troop*>& troops() const{
                return m_troops;
            }

            void SetBindTroopId(int bindTroopId) {
                m_bindTroopId = bindTroopId;
            }

            int64_t uid() const override{
                return m_uid;
            }

            const std::string nickname() const;

            int64_t allianceId() const override;

            virtual Troop* troop() override {
                return m_troop;
            }

            Agent* agent() {
                return m_agent;
            }

            bool CanAttack(Troop* troop) {
                if (troop) {
                    return (uid() != troop->uid() && (allianceId() == 0 || allianceId() != troop->allianceId()));
                }
                return false;
            }

            virtual void Occupy(Troop* troop) override;
            // 盟友是否可驻守
            virtual bool CanGarrison(Troop* troop);
            void Garrison(Troop* troop);
            void UpdateDefTroop();
            bool SwitchTroop();

            virtual bool RemoveSelf() override;
            virtual void OnTroopLeave(Troop* troop) override;
            virtual void OnTroopBack(Troop* troop) override;
            //行军到达
            virtual void OnTroopReach(Troop* troop) override;

            virtual std::string Serialize() override;
            virtual bool Deserialize(std::string data) override;

            virtual void FinishDeserialize() override;

            void OnIntervalUpdate(size_t updateSeconds);
            void StartRecoverCampFixedDurable();
            void UpdateCampFixedDurable();

            int campFixedDurable() {
                return m_campFixedDurable;
            }

            int campFixedDurableMax() {
                return m_campFixedDurableMax;
            }

            int campFixedDurableRecover() {
                return m_campFixedDurableRecover;
            }

            void AddCampFixedDurable(int add);

        private:
            Troop* m_troop = nullptr;
            int m_troopId = 0;
            int m_bindTroopId = 0;// 绑定的行军Id
            int64_t m_uid = 0;

            int m_campFixedDurableMax = 0;     //耐久值上限
            int m_campFixedDurable = 0;              // 耐久值

            int64_t m_campFixedDurableRecoverTime = 0;           //耐久值恢复时间
            size_t m_updateSeconds = 0;
            int m_campFixedDurableRecover = 0;                   //耐久值恢复量


            Agent* m_agent = nullptr;

            std::list<Troop*> m_troops;
            std::list<int> m_troopIds;
        };
    }
}

#endif // CAMP_H
