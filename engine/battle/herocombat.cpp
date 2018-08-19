#include <stdint.h>
#include <base/logger.h>
#include <base/framework.h>
#include <algorithm>
#include "../randengine.h"
#include "../tpl/t_hero.h"

#include "herocombat.h"

namespace engine
{
    namespace battle
    {
	    HeroCombat::HeroCombat(int randSeed)
	      : m_randSeed(randSeed)
	    {
	        m_singleCombat = new SingleCombat(m_randSeed + 1);
	        m_singleCombat->SetAutomatic(true);
	    }

	    HeroCombat::~HeroCombat()
	    {
	        if (m_singleCombat != nullptr) {
	            delete m_singleCombat;
	            m_singleCombat = nullptr;
	        }
	    }

	    bool HeroCombat::Init(HeroCombatDataInput& attackInput, HeroCombatDataInput& defenceInput)
	    {
	        m_attackInput = attackInput;
	        m_defenceInput = defenceInput;

	        int attackHeroCnt = m_attackInput.heroes.size();
	        int defenseHeroCnt = m_defenceInput.heroes.size();
	        m_maxBigRound = attackHeroCnt > defenseHeroCnt  ?  defenseHeroCnt : attackHeroCnt;
	        if (m_maxBigRound == 0) {
	            m_state = HeroCombatState::ALL_OVER;
	            return false;
	        }
	        m_state = HeroCombatState::INIT;
	        m_curBigRound = 0;
	        // m_singleCombat->Init(m_attackInput.heroes[m_curBigRound-1], m_defenceInput.heroes[m_curBigRound-1]);
	        return true;
	    }

	    HeroCombatState HeroCombat::Next()
	    {
	        if (HeroCombatState::ALL_OVER ==  m_state) {
	            return m_state;
	        }

	        ++m_curBigRound;
	        m_singleCombat->Init(m_attackInput.heroes[m_curBigRound-1], m_defenceInput.heroes[m_curBigRound-1]);
	        m_singleCombat->Start();

	        if (m_singleCombat->result().isAttackWin) {
	            ++m_attackWinCount;
	        } else {
	            ++m_defenceWinCount;
	        }
	        bool isAttackWin = m_singleCombat->result().isAttackWin;
	        m_result.attackOutputHurt.emplace_back(m_singleCombat->result().attackHeroId, m_singleCombat->result().attackHeroHp, m_singleCombat->result().attackOutputHurt, isAttackWin);
	        m_result.defenseOutputHurt.emplace_back(m_singleCombat->result().defenseHeroId, m_singleCombat->result().defenseHeroHp, m_singleCombat->result().defenseOutputHurt, !isAttackWin);
	        if (m_curBigRound ==  m_maxBigRound) {
	            m_result.attackWinCount = m_attackWinCount;
	            m_result.defenceWinCount = m_defenceWinCount;
	            m_state = HeroCombatState::ALL_OVER;
	        }
	        WarReport::ArenaData arenaData;
	        arenaData.roundId = m_curBigRound;
			const WarReport::SoloData* attackSoloReport = nullptr;
		    const WarReport::SoloData* defenseSoloReport = nullptr;
		    if (m_singleCombat)
		    {
		        attackSoloReport = &m_singleCombat->AttackSoloReport();
		        defenseSoloReport = &m_singleCombat->DefenseSoloReport(); 
		    }
            arenaData.datas.push_back(*attackSoloReport);
            arenaData.datas.push_back(*defenseSoloReport);
            m_arenaDatas.push_back(arenaData);
	     
	        return m_state; 
	    }

	    bool HeroCombat::IsAttackWin()
	    {
	    	// 如果是副本的2v2或者4v4，只要攻击方赢的场数比防守方低，就算输
	        return m_attackWinCount > m_defenceWinCount;
	    }
    }
}

