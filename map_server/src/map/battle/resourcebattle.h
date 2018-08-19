#ifndef MAP_BATTLE_RESOURCEBATTLE_H
#define MAP_BATTLE_RESOURCEBATTLE_H
#include "battle.h"
#include <stdint.h>

namespace ms
{
    namespace map
    {
        class ResNode;
        namespace battle
        {
            class ResourceBattle : public Battle
            {
            public:
                ResourceBattle(Troop* troop, ResNode* res);
                virtual ~ResourceBattle();

                const ResNode* resNode() const {
                    return m_resNode;
                }


                virtual void OnStart() override;
                virtual void OnEnd(model::AttackType winner, int reportId, bool last_flag) override;

            private:
                ResNode* m_resNode = nullptr;
                std::vector<model::tpl::DropItem> m_dorpItems;
            };
        }
    }
}

#endif // MAP_BATTLE_RESOURCEBATTLE_H