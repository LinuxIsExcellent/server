#ifndef CASTLE_H
#define CASTLE_H
#include <model/metadata.h>
#include "unit.h"
#include <base/objectmaintainer.h>

namespace ms
{
    namespace map
    {
        class Troop;
        namespace info
        {
            class TurretInfo;
        }

        class Agent;
        class ArmyList;
        enum ReinforceState {
            MARCH = 1,
            REACH = 2,
        };

        class Castle : public Unit
        {
        public:
            Castle(int id, const MapUnitCastleTpl* tpl, Agent* agent, int level);
            ~Castle();

            virtual void Init() override
            {
                //SetDirty();
            }
            
            virtual ArmyList* DefArmyList() override;

            int getDefTeam() const;

            const MapUnitCastleTpl& tpl() const {
                return *(m_tpl->ToCastle());
            }

            virtual Troop* troop() override {
                return m_troop;
            }

            const std::list<Troop*>& troops() const{
                return m_troops;
            }

            const std::list<int>& marchReinforcements() const {
                return m_marchReinforcementTroopIds;
            }

            const std::list<int>& reachReinforcements() const {
                return m_reachReinforcementTroopIds;
            }

            Agent& agent() const {
                return *m_agent;
            }

            int64_t uid() const override;
            int64_t allianceId() const override;
            const std::string& allianceNickname() const;
            const std::string& nickname() const;
            int allianceBannerId() const;

            int level() const {
                return m_level;
            }

            model::CastleState state() const {
                return m_state;
            }

            int cityDefense() const {
                return m_cityDefense;
            }

            void AddCityDefense(int add);

            int loseCityDefense() const;

            int cityDefenseMax() const;

            int burnLeftTime() const;

            int burnEndTimestamp() const {
                return m_burnEndTimestamp;
            }

            int burnTotalTime() const {
                return m_burnEndTimestamp - m_burnBeginTimestamp;
            }

            int silverRepairLeftTime() const;
            int goldRepairLeftTime() const;

            int burnTimes() const {
                return m_burnTimes;
            }

            bool IsBurning() const {
                return m_state == model::CastleState::BURN || m_state == model::CastleState::PROTECTED_AND_BURN;
            }

            bool IsProtected() const {
								return m_state == model::CastleState::PROTECTED
								    || m_state == model::CastleState::PROTECTED_AND_BURN
										|| m_state == model::CastleState::NOVICE;
            }

            bool CanAttack(Troop *troop);

            int titleId() const;

            int IsPeaceShield() const;
            //是否反侦察
            bool IsAntiScout() const;
            //资源保护
            bool IsNoPlunder() const;
            //是否处于中立城池攻击保护状态
            bool IsNcProtecting() const;

            const info::TurretInfo& turretInfo() const;

            void SetLevel(int level) {
                m_level = level;
                SetDirty();
            }

            void SetState(model::CastleState state) {
                m_state = state;
                SetDirty();
            }

            void Burn();
            bool OutFire();
            void PeaceShield(int time);
            void RemovePeaceShield();
            bool Repair(bool isGoldRepair);
            void OnRebuild();
            //检查是否新手 设置新手保护
            void CheckIsNovice();
            void CheckPeaceShield();

            std::array<ArmyList, 5>& armyListArray();
            bool CanGarrison(Troop* troop);
            void Garrison(Troop* troop);
            void UpdateDefTroop(); 
            bool SwitchTroop();
            bool IsLastTroop() override;
            void BackTroop(Troop* troop);

        public:
            //城堡升级
            void OnLevelUp(int level);
            //被行军
            virtual void OnTroopMarch(Troop* troop) override;
            //行军到达
            virtual void OnTroopReach(Troop* troop) override;
            //行军离开
            virtual void OnTroopLeave(Troop* troop) override;

            //迁移
            virtual void OnTeleport();
            //城防值上限改变
            void OnCityDefenseMaxChange(int oldV, int newV);

            // 增加正在增援过程中的部队
            bool AddMarchReinforcement(Troop* troop);

            // 增加已经到达的增援部队
            bool AddReachReinforcement(Troop* troop);

            // 删除已经到达的增援部队
            bool RemoveReachReinforcement(Troop* troop);

            // 删除正在增援过程中的部队
            bool RemoveMarchReinforcement(Troop* troop);

            // 撤回所有增援部队
            void ClearAllReinforcement();

            // 判断是否需要发送增援部队信息
            void CheckIsSendReinforceTroop();

        public:
            virtual std::string Serialize() override;
            virtual bool Deserialize(std::string data) override;
            virtual void FinishDeserialize() override;

        private:
            void OnBurnEnd();
            void OnPeaceEnd();
            void OnBurning();
        private:
            static constexpr int TAG_BURN_END = 1;
            static constexpr int TAG_PEACE_END = 2;
            static constexpr int TAG_BURNING = 3;
            static constexpr int TAG_NOVICE = 4;
            Agent* m_agent = nullptr;
            int m_level = 1;
            model::CastleState m_state = model::CastleState::NORMAL;

            Troop* m_troop = nullptr;
            //ArmyList m_armyList;    //默认驻守军
            // ArmySet m_trapSet;      //陷阱
            std::list<Troop*> m_troops;
            std::list<int> m_troopIds;

            // 同盟增援部队
            // std::map<ReinforceState, Troop*> m_reinforcementTroops;
            // 已经到达的增援部队
            std::list<Troop*> m_reachReinforcementTroops;
            std::list<int> m_reachReinforcementTroopIds;
            // 正在增援过程中的部队
            std::list<Troop*> m_marchReinforcementTroops;
            std::list<int> m_marchReinforcementTroopIds;


            int m_cityDefense = 0;
            int64_t m_silverRepairEndTimestamp = 0;       //银两修理剩余时间
            int64_t m_goldRepairEndTimestamp = 0;       //元宝修理剩余时间
            int m_burnTimes = 0;                    //燃烧时被攻破次数
            int64_t m_burnBeginTimestamp = 0; 
            int64_t m_burnEndTimestamp = 0; 
            int64_t m_ncProtectEndTime = 0;          //中立城池攻击保护结束时间戳
            int64_t m_noviceTimestamp = 0;          //新手时间戳 用来控制新手保护
        };
    }
}

#endif // CASTLE_H
