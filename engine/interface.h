#ifndef BATTLE_INTERFACE_H
#define BATTLE_INTERFACE_H
#include <vector>
#include "input_struct.h"
#include "randengine.h"
#include "battle/singlecombat.h"
#include "battle/mixedcombat.h"
#include "battle/herocombat.h"

namespace engine
{
    // namespace battle
    // {
    //     class SingleCombat;
    //     class MixedCombat;
    // }

    namespace tpl
    {
        struct T_SpellNode;
        struct T_SpellBase;
        struct T_Buff;
        struct T_Hero;
        struct T_Army;
        struct T_ArmySpell;
        struct T_AttackDiscount;
        struct T_BattleConfig;
        struct T_HeroPowerConfig;
    }

    // init tpl (初始化模板)
    bool InitBattleTemplate(std::vector<tpl::T_SpellNode>& skillNodes, std::vector<tpl::T_SpellBase>& skillBases,
                            std::vector<tpl::T_Buff>& buffs, std::vector<tpl::T_Hero>& heroes);
    bool InitBattleTemplate2(std::vector<tpl::T_Army>& armies, std::vector<tpl::T_ArmySpell>& armySpells,
                             std::vector<tpl::T_AttackDiscount>& attackDiscount);

    bool InitBattleConfig(tpl::T_BattleConfig& battleConfig);
    bool InitHeroPowerConfig(tpl::T_HeroPowerConfig& heroPowerConfig);

    //战斗
    class Combat
    {
    public:

        Combat(int randSeed, bool mustSingleCombat, bool isAutomatic, int levelType);
        ~Combat();

        // 触发单挑返回true，如果没有触发单挑返回false
        bool Init(InitialDataInput& attackInput, InitialDataInput& defenceInput);

        // single combat (单挑)
        bool CreateSingleCombat();

        // mixed battle (混战)
        bool CreateMixedCombat();

        //服务端调用
        bool StartSingleCombat();
        bool StartMixedCombat();

        // 剧情任务减兵处理（这里是虚减兵处理, 并没有减实际的兵，只是减战斗中的兵力）
        void OutputSingleResult(const engine::SingleCombatResult& singleResult);

        battle::SingleCombat* singleCombat() {
            return m_singleCombat;
        }
        
        battle::MixedCombat* mixedCombat() {
            return m_mixedCombat;
        }

        //序列号战报
        static std::string SerializeReportData(engine::WarReport& report, int reportId);
        
    protected:
        int m_levelType = 0;    //战斗类型      0: 单挑  1: 混战   2:混合战斗
        bool m_mustSingleCombat = false; //必须单挑
        bool m_isAutomatic = false; //是否自动模式
        int m_randSeed = 1;
        RandEngine m_randEngine;
        InitialDataInput m_attackInput;
        InitialDataInput m_defenceInput;

       /* I_Hero* m_attackHero = nullptr;//进攻方单挑Hero
        I_Hero* m_defenceHero = nullptr;//防守方单挑Hero*/
        battle::SingleCombat* m_singleCombat = nullptr;
        battle::MixedCombat* m_mixedCombat = nullptr;
    };

    /////////////////////////////////////////////////////////////
   

}

#endif
