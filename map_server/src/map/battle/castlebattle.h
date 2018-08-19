#ifndef MAP_BATTLE_CASTLEBATTLE_H
#define MAP_BATTLE_CASTLEBATTLE_H
#include "battle.h"
#include <stdint.h>

namespace ms
{
    namespace map
    {
        class Castle;
        namespace battle
        {
            class CastleBattle : public Battle
            {
            public:
                CastleBattle(Troop* troop, Castle* castle);
                virtual ~CastleBattle();

                virtual void OnStart() override;
                virtual void OnEnd(model::AttackType winner, int reportId, bool last_flag) override;
                virtual void OnBattleEnd(model::AttackType winner);

                virtual bool SwitchTroop() override;
                virtual bool IsLastTroop() override;
            private:
                void ResourcePlunder(int& foodReal, int& woodReal, int& stoneReal, int& ironReal, int& foodRemove, int& woodRemove, int& stoneRemove, int& ironRemove);
            protected:
                virtual void SetDefenseInput( engine::InitialDataInput& defenseInput) override;
                virtual void OutPutDefenseArmyList(const engine::MixedCombatResult& mixedResult) override;

            protected:
                Castle* m_castle = nullptr;
                int m_index = 0;
                bool m_isCastleAgent = true;
                bool m_isDefener = true;     // 是否是默认驻防部队
            };
        }
    }
}

#endif // MAP_BATTLE_CASTLEBATTLE_H
