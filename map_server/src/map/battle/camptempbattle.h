#ifndef MAP_BATTLE_CAMPTEMPBATTLE_H
#define MAP_BATTLE_CAMPTEMPBATTLE_H
#include "battle.h"
#include <stdint.h>

namespace ms
{
    namespace map
    {
        class CampTemp;
        namespace battle
        {
            class CampTempBattle : public Battle
            {
            public:
                CampTempBattle(Troop* troop, CampTemp* campTemp);
                virtual ~CampTempBattle();

                virtual void OnStart() override;
                virtual void OnEnd(model::AttackType winner, int reportId, bool last_flag) override;
                
            protected:
                CampTemp* m_campTemp = nullptr;
            };
        }
    }
}

#endif // MAP_BATTLE_CAMPTEMPBATTLE_H
