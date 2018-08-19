#ifndef MAP_CAPATULT_H
#define MAP_CAPATULT_H
#include "unit.h"
#include "palace.h"
#include <base/objectmaintainer.h>

namespace ms
{
    namespace map
    {
        class Catapult : public Unit
        {
        public:
            Catapult(int id, const ms::map::tpl::MapUnitCatapultTpl* tpl, const Point& bornPoint);
            ~Catapult();

            const ms::map::tpl::MapUnitCatapultTpl& tpl() const {
                return *m_tpl;
            }
            const ms::map::tpl::MapCityTpl& CityTpl() const {
                return *m_cityTpl;
            }
            virtual void Init();

            model::PalaceState state() const {
                return m_state;
            }

            int GetDropId();
            
            bool isNotStart() const {
                return m_state == model::PalaceState::NOSTART || m_state == model::PalaceState::PROTECTED || m_state == model::PalaceState::CHOOSE;
            }

            bool isProtected() const {
                return m_state == model::PalaceState::PROTECTED;
            }
            
            int64_t occupierId() const {
                return m_occupierId;
            }

            const Agent* occupier() const {
                return m_occupier;
            }
            
            Agent* occupier() {
                return m_occupier;
            }

            int troopId() const ;

            const std::unordered_map<int64_t, Troop*>& defenders() const {
                return m_defenders;
            }

            int lastKills() const {
                return m_lastKills;
            }

            int kills() const {
                return m_kills;
            }

            int64_t lastKillTimestamp() const {
                return m_lastKillTimestamp;
            }

            const std::list<info::PalaceWarRecord>& palaceWarRecords() const {
                return m_palaceWarRecords;
            }

            void SetState(model::PalaceState state) {
                m_state = state;
                NoticeUnitUpdate();
            }

            void SetOccupier(Agent* agent) {
                m_occupier = agent;
                NoticeUnitUpdate();
            }

            const Point& bornPoint() const {
                return m_bornPoint;
            }

            // const std::string& nickname();

            const std::list<Troop*>& troops() const{
                return m_troops;
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

            const Troop* troop() const {
                return m_troop;
            }

            void ClearNpc();


            int Power();

            const int CurrentArmyTotal();

            const int ArmyListsTotal()
            {
                return m_armyListsTotal;
            }

            bool isOccupy() {
                return m_isOccupy;
            }
        public:
            void Refresh();
            //增加记录
            void AddPalaceWarRecord(model::PalaceWarRecordType type, const std::string& param);
            //全部遣返
            void RepatriateAllDefenders();

        public:
            virtual void Occupy(Troop* troop) override; 
            // 盟友是否可驻守
            virtual bool CanGarrison(Troop* troop);
            void Garrison(Troop* troop);
            void UpdateDefTroop();
            bool SwitchTroop();
            
            virtual void OnTroopLeave(Troop* troop) override;
            virtual void OnTroopBack(Troop* troop) override;
            //行军到达
            virtual void OnTroopReach(Troop* troop) override;

        public:
            void OnPlaceWarStart();
            void OnPlaceWarEnd();
            //行军加载完成
            void OnTroopLoadFinished(Troop* troop);
            //王城更换占领者联盟
            void OnPalaceOccupierIdChanged();
            //自己更换占领者联盟
            void OnSelfOccupierIdChanged();

        public:
            virtual std::string Serialize() override;
            virtual bool Deserialize(std::string data) override;
            virtual void FinishDeserialize() override;

        private:
            //清除所有防守军 但不会更新到前端
            void ClearAllDefenders();
            void AttackPalace();
            void ResetKillInfos();

        private:
            static constexpr int REFRESH = 1;       //弃用
            static constexpr int ATTACK_PALACE = 2;

            int64_t m_uid = 0;

            const ms::map::tpl::MapUnitCatapultTpl* m_tpl = nullptr;
            const ms::map::tpl::MapCityTpl* m_cityTpl = nullptr;
            const Point m_bornPoint;

            Troop* m_troop = nullptr;
            std::list<Troop*> m_troops;
            std::list<int> m_troopIds;
            //Agent* m_agent = nullptr;
            int m_troopId = 0;

            std::list<ArmyList> m_armyLists;      //首军列表
            int m_armyListsTotal;                   //NPC驻军总数

            bool m_isOccupy = false;                        //是否被占领过
            int64_t m_occupierId = 0; //占领者联盟ID
            std::unordered_map<int64_t, Troop*> m_defenders;    //uid为key, 同时只能存在一个玩家的队列
            model::PalaceState m_state = model::PalaceState::NOSTART;
            Agent* m_occupier = nullptr;    //占领者信息
            //PS:字段m_lastKills, m_kills, m_lastKillTimestamp 后来增加对龙造成伤害做记录
            int m_lastKills = 0;                    //上次魔法塔击杀王城士兵数量 
            int64_t m_lastKillTimestamp = 0;        //上次魔法塔击杀王城士兵时间戳 
            int m_kills = 0;                        //当前魔法塔击杀王城士兵数量

            std::list<info::PalaceWarRecord> m_palaceWarRecords;      //王城争霸记录 只记录100个
        };
    }
}
#endif // MAP_CAPATULT_H
