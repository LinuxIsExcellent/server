#include "hero.h"
#include "team.h"
#include <assert.h>
#include <iostream>

namespace engine
{
    namespace battle
    {
        using namespace tpl;

        bool Team::Init(I_Team& iTeam, engine::TeamType teamType)
        {
            std::cout <<  "teamType " << (int)teamType << std::endl;
            m_teamType = teamType;
            for (auto &iGroup : iTeam.groups) {
                assert(iGroup.position >=1 && iGroup.position <= 9);
                if (iGroup.army.armyCount <= 0 || ! (iGroup.position >=1 && iGroup.position <= 9)) {
                    continue;
                }

                //Todo 单挑结果对混战的影响
                m_listHero.emplace_back(iGroup, this);
                Hero& hero = m_listHero.back();
                
                hero.SetTeamType(teamType);
                //group.Init(isHeroDie);
                m_arrHeros[iGroup.position - 1] = &hero;
                m_mapHeros.emplace(hero.tplId(), &hero);
            }

            // 箭塔
            m_turretAtkPower = iTeam.turretAtkPower;

            return true;
        }

        Hero* Team::GetHeroByPos(int position)
        {
            Hero* hero = nullptr;
            if (position > 0 && position <= 9) {
                hero = m_arrHeros[position - 1];
            }
            return hero;
        }

        bool Team::GetHeroById(int id) 
        {
            auto it = m_mapHeros.find(id);
            return it == m_mapHeros.end() ? false : true;
        }

        void Team::OnDie(Hero* dieHero)
        {
            
        }

        int Team::RemainTotalHp()
        {
            int remainHp = 0;
            for (auto &hero : m_listHero) {
                if (!hero.IsDie()) {
                    remainHp += hero.armyHp();
                }
            }
           
            return remainHp;
        }

        void Team::OnEnd()
        {
          
        }

        void Team::UpdateBuff(TriggerBuffState state)
        {
            for (auto &hero : m_listHero) {
                hero.UpdateBuff(state);
            }
        }
    }
}

