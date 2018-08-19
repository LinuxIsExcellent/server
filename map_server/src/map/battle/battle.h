#ifndef MAP_BATTLE_BATTLE_H
#define MAP_BATTLE_BATTLE_H
#include <model/metadata.h>
#include <vector>
#include "interface.h"
#include "../msgqueue/msgrecord.h"
#include "../mapMgr.h"

namespace engine
{
    struct InitialDataInput;
    struct MixedCombatResult;
    struct WarReport;
}

namespace ms
{
    namespace map
    {
        class Unit;
        class Troop;

        namespace msgqueue
        {
            class MsgPvpTeamInfo;
        }

        namespace battle
        {
            class Battle
            {
            public:
                Battle(model::BattleType type, Troop* troop, Unit* unit);
                virtual ~Battle();

                model::BattleType battleType() {
                    return m_type;
                }

                msgqueue::MsgPvpPlayerInfo& attackerInfo() {
                    return m_attackerInfo;
                }

                msgqueue::MsgPvpPlayerInfo& defenderInfo() {
                    return m_defenderInfo;
                }

                ArmyList * atkArmyList() {
                    return m_atkArmyList;
                }

                ArmyList * defArmyList() {
                    return m_defArmyList;
                }

            public:

                void Init(engine::InitialDataInput& attackInput, engine::InitialDataInput& defenseInput);
                // 输出单挑结果
                void OutputSingleResult(const engine::SingleCombatResult& singleResult);
                // 输出混战结果  
                void OutputResult(const engine::MixedCombatResult& mixedResult);
                virtual void OnStart() = 0;
                virtual void OnEnd(model::AttackType winner, int reportId, bool last_flag) = 0;
                virtual void OnBattleEnd(model::AttackType winner) {}

                // 输出结束兵力
                void OutPutEndAmryCount();
                // 一场战斗结束之后，把需要恢复的兵变成正常兵
                void OnBattleRecoverArmy();
                //转换部队 
                virtual bool SwitchTroop(); 
                virtual bool IsLastTroop();

            protected:
                virtual void SetAttackInput(engine::InitialDataInput& attackInput);
                virtual void SetDefenseInput( engine::InitialDataInput& defenseInput);
                virtual void OutPutAttackArmyList(const engine::MixedCombatResult& mixedResult);
                virtual void OutPutDefenseArmyList(const engine::MixedCombatResult& mixedResult);

                void  inputArmyList(engine::InitialDataInput & input, ArmyList* armyList, TrapSet* trapSet);

                void ResetArmyList();

                void Compensate(model::AttackType attackType, const engine::MixedCombatResult& mixedResult, Agent& agent);

            protected:
                model::BattleType m_type;
                model::AttackType m_winner = model::AttackType::BOTH;
                Unit* m_unit = nullptr;
                ArmyList * m_atkArmyList = nullptr;
                ArmyList * m_defArmyList = nullptr;
                int m_level = 0;                      // 被攻击方的等级（野怪，资源）

                std::list<std::vector<engine::MixedCombatResult::ArmyHurt>> m_attackArmyHurts;

                std::string m_posName = "";   //战斗地点
                int m_isLang = 0;          // 战斗地点是否读多语言
                TrapSet* m_defTrapSet = nullptr;
                msgqueue::MsgPvpPlayerInfo m_attackerInfo;
                msgqueue::MsgPvpPlayerInfo m_defenderInfo;
                msgqueue::MsgReportInfo m_reportInfo;

                Troop* m_defTroop = nullptr;
                Troop* m_atkTroop = nullptr;
            };

            class DefaultBattle : public Battle
            {
            public:
                DefaultBattle(Troop* troop, Unit* unit)
                 : Battle(model::BattleType::UNKNOWN, troop, unit) {}
                virtual ~DefaultBattle() {}

                virtual void OnStart() {}
                virtual void OnEnd(model::AttackType winner, int reportId, bool last_flag) {}
            };
        }
    }
}

#endif // MAP_BATTLE_BATTLE_H
