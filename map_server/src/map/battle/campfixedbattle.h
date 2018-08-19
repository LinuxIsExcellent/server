#ifndef MAP_BATTLE_CAMPFIXEDBATTLE_H
#define MAP_BATTLE_CAMPFIXEDBATTLE_H
#include "battle.h"
#include <stdint.h>

namespace ms
{
    namespace map
    {
        class CampFixed;
        namespace battle
        {
            class CampFixedBattle : public Battle
            {
            public:
                CampFixedBattle(Troop* troop, CampFixed* campFixed);
                virtual ~CampFixedBattle();

                virtual void OnStart() override;
                virtual void OnEnd(model::AttackType winner, int reportId, bool last_flag) override;
                virtual void OnBattleEnd(model::AttackType winner) override;
                
            protected:
                CampFixed* m_campFixed = nullptr;
            };
        }
    }
}

#endif // MAP_BATTLE_CITYBATTLE_H
