#ifndef MAP_BATTLE_MONSTERBATTLE_H
#define MAP_BATTLE_MONSTERBATTLE_H
#include "battle.h"
#include <stdint.h>

namespace ms
{
    namespace map
    {
        class Monster;
        namespace battle
        {
            class MonsterBattle : public Battle
            {
            public:
                MonsterBattle(Troop* troop, Monster* monster);
                virtual ~MonsterBattle();

                const Monster* monster() const {
                    return m_monster;
                }

                virtual void OnStart() override;
                virtual void OnEnd(model::AttackType winner, int reportId, bool last_flag) override;

            private:
                Monster* m_monster = nullptr;
                
                std::vector<model::tpl::DropItem> m_dorpItems;
            };
        }
    }
}

#endif // MAP_BATTLE_RESOURCEBATTLE_H