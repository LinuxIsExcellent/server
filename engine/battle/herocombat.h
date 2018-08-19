#ifndef ENGINE_BATTLE_HEROCOMBAT_H
#define ENGINE_BATTLE_HEROCOMBAT_H
#include <model/tpl/templateloader.h>
#include <model/tpl/configure.h>

#include "../input_struct.h"
#include "../randengine.h"
#include "hero.h"

#include "singlecombat.h"

namespace engine
{
    namespace tpl
    {

        struct T_SpellNode;
    }

    namespace battle
    {
	    // 擂台
	    enum class HeroCombatState
	    {
	        INIT,
	        ALL_OVER,
	    };

		//擂台初始数据输入
	    struct  HeroCombatDataInput {
	        uint64_t uid = 0;                //玩家Id
	        int headIcon = 0;               //头像
	        std::string name;               // 名字
	        int level = 0;                        // 等级
	        std::vector<I_Hero> heroes; 
	    };

	    struct HeroCombatResult {
	        // 武将输出伤害
	        struct HeroOutputHurt {
	            HeroOutputHurt(int _heroId, int _heroHp, int _outputHurt, bool _isWin)
	                : heroId(_heroId), heroHp(_heroHp), outputHurt(_outputHurt), isWin(_isWin){}

	            int heroId = 0;
	            int heroHp = 0;
	            int outputHurt = 0;
	            bool isWin = false;
	        };

	        std::vector<HeroOutputHurt> attackOutputHurt;
	        std::vector<HeroOutputHurt> defenseOutputHurt;

	        int attackWinCount = 0;                           // 攻击方胜利次数
	        int defenceWinCount = 0;                          // 防守方胜利次数
	    };

	    class HeroCombat
	    {
	    public:
	        HeroCombat(int randSeed);
	        ~HeroCombat();

	        bool Init(HeroCombatDataInput& attackInput, HeroCombatDataInput& defenceInput);

	        HeroCombatState Next();

	        bool IsAttackWin();

	        int attackWinCount()
	        {
	        	return m_attackWinCount;
	        }

	        int maxBigRound()
	        {
	        	return m_maxBigRound;
	        }

	        const HeroCombatResult& result() {
	            return m_result;
	        }

	        std::vector<WarReport::ArenaData>& arenaReport() {
                return m_arenaDatas;
            }

	    protected:
	        int m_randSeed = 1;
	        HeroCombatState m_state = HeroCombatState::INIT;
	        int m_curBigRound = 0;
	        int m_maxBigRound = 0;
	        SingleCombat* m_singleCombat = nullptr;

	        HeroCombatDataInput m_attackInput;
	        HeroCombatDataInput m_defenceInput;

	        int m_attackWinCount = 0;                           // 攻击方胜利次数
	        int m_defenceWinCount = 0;                          // 防守方胜利次数

	        HeroCombatResult m_result;

	        std::vector<WarReport::ArenaData> m_arenaDatas;
	    };
    }
}

#endif // ENGINE_BATTLE_HEROCOMBAT_H

