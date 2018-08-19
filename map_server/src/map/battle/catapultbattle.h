#ifndef MAP_BATTLE_CATAPULTBATTLE_H
#define MAP_BATTLE_CATAPULTBATTLE_H
#include "battle.h"
#include <stdint.h>

namespace ms
{
    namespace map
    {

        class Catapult;

        namespace battle
        {
            class CatapultBattle : public Battle
            {
            public:
                CatapultBattle(Troop* troop, Catapult* catapult);
                virtual ~CatapultBattle();

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
                Catapult*  m_catapult = nullptr;
                std::vector<model::tpl::DropItem> m_dorpItems;

                msgqueue::MsgWorldBossInfo* m_worldBossInfo = nullptr;

                ArmyList m_defCache;
            };
        }
    }
}
#endif // MAP_BATTLE_CATAPULTBATTLE_H
