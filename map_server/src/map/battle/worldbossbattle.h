#ifndef MAP_BATTLE_WORLDBOSSBATTLE_H
#define MAP_BATTLE_WORLDBOSSBATTLE_H
#include "battle.h"
#include <stdint.h>

namespace ms
{
    namespace map
    {

        class WorldBoss;

        namespace battle
        {
            class WorldBossBattle : public Battle
            {
            public:
                WorldBossBattle(Troop* troop, WorldBoss* worldboss);
                virtual ~WorldBossBattle();

                virtual void OnStart() override;
                virtual void OnEnd(model::AttackType winner, int reportId, bool last_flag) override;
                virtual void OnBattleEnd(model::AttackType winner) override;

                msgqueue::MsgWorldBossInfo& worldBossInfo() const {
                    return *m_worldBossInfo;
                }
            protected:
                virtual void SetDefenseInput( engine::InitialDataInput& defenseInput) override;

            protected:
                //FamousCity* m_city = nullptr;
                WorldBoss*  m_worldboss = nullptr;

                msgqueue::MsgWorldBossInfo* m_worldBossInfo = nullptr;

                ArmyList m_defCache;
            };
        }
    }
}
#endif // MAP_BATTLE_WORLDBOSSBATTLE_H
