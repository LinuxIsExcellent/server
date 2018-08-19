#ifndef ENGINE_BATTLE_TEAM_H
#define ENGINE_BATTLE_TEAM_H
#include <stdint.h>
#include <array>
#include <functional>
#include "bufflist.h"
#include "../input_struct.h"
#include "../tpl/t_spell.h"
#include "../tpl/t_army.h"
#include "../tpl/t_hero.h"
#include "../metadata.h"
#include "hero.h"

namespace engine
{
    namespace battle
    {
        class Hero;
        class Team
        {
        public:
            Team() {
                m_arrHeros.fill(nullptr);
            }

            virtual ~Team() {}

            bool Init(I_Team& iTeam, engine::TeamType teamType);

            Hero* GetHeroByPos(int position);

            bool GetHeroById(int id);

            std::list<Hero>& GetHeroList() {
                return m_listHero;
            }

            bool IsAllDie() {
                for (auto &hero : m_listHero) {
                    if (!hero.IsDie()) {
                        return false;
                    }
                }
                return true;
            }

            void OnDie(Hero* dieHero);

            //战力
            int TotalPower() {
                int totalPower = 0;
                for (auto &hero : m_listHero) {
                    if (!hero.IsDie()) {
                        totalPower += hero.power();
                    }
                }
                return totalPower;
            }

            int RemainTotalHp();

            void OnEnd();

            void UpdateBuff(TriggerBuffState state);

            engine::TeamType teamType() {
                return m_teamType;
            }

            int turretAtkPower() {
                return m_turretAtkPower;
            }

        protected:
            engine::TeamType m_teamType = engine::TeamType::NONE;

            std::list<Hero*> m_activeHeros; //活着的英雄
            std::array<Hero*, 9> m_arrHeros;
            std::list<Hero> m_listHero;
            std::unordered_map<int, Hero*> m_mapHeros;

            int m_turretAtkPower = 0;         // 箭塔攻击力
        };
    }
}

#endif
