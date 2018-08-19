#ifndef ENGINE_BATTLE_SINGLECOMBAT_H
#define ENGINE_BATTLE_SINGLECOMBAT_H
#include <model/tpl/templateloader.h>
#include <model/tpl/configure.h>

#include "../input_struct.h"
#include "../randengine.h"
#include "hero.h"

namespace engine
{
    namespace tpl
    {

        struct T_SpellNode;
    }

    namespace battle
    {
        using namespace engine;
        using namespace engine::tpl;
        using namespace model::tpl;
        enum SingleCombatState
        {
            INIT,
            READY,
            OVER,
        };

      /*  // 武将单挑出招技能
        enum HeroSoloSkill
        {
            NONE,
            STONE,    // 石头
            SHEAR,     // 剪刀
            CLOTH,      // 布
            NIRVARNA,    // 必杀
            ESCAPE,       // 逃跑
        };
*/
        // 战斗结果
        enum SoloCombatWin
        {
            DEUCE,      //平手
            WIN,        //胜利
            FAIL,         //失败
        };

        // 武将出招概率
        struct HeroSoloSkillProb
        {   
            int stone = 0;    // 石头
            int shear = 0;     // 剪刀
            int cloth = 0;      // 布
        };

        class SingleCombat;
        struct SingleCombatEventHandle {
            struct RoundBeginInput {
                int round = 0;                  //当前第几回合
                int hp = 0;       //生命值
                int rage = 0;   //怒气值
                int canHurt = 0; //普通攻击伤害值

                void Reset() {
                    round = 0;
                    hp = 0;
                    rage = 0;
                    canHurt = 0;
                }
            };

            struct RoundEndOutput {
                int round = 0;                  //当前第几回合
                int hurtHp = 0;                 //伤害
                int addRage = 0;            //增加怒气值， 为负表示消耗

                void Reset() {
                    round = 0;
                    hurtHp = 0;
                    addRage = 0;
                }
            };

            virtual void OnRoundBegin(RoundBeginInput& attackIntput, RoundBeginInput& defenceInput) = 0;
            virtual void OnRoundEnd(RoundEndOutput& m_attackOutput, RoundEndOutput& m_defenceOutput) = 0;
            virtual void OnEnd(SingleCombatResult& result) = 0;

            void SetSingleCombat(SingleCombat* singleCombat) {
                m_singleCombat = singleCombat;
            }

        protected:
            SingleCombat* m_singleCombat = nullptr;
        };

        //////服务器验证时用
        //单挑小回合释放技能信息
        struct SingleCombatRoundSpell {
            SingleCombatRoundSpell(int _round, int _spellBaseId)
                : round(_round), spellBaseId(_spellBaseId) {}

            int round; //第几回合
            int spellBaseId;//释放的技能
        };

        //单挑释放技能信息(服务器验证时用)
        struct SingleCombatReleaseSpells {
            std::vector<SingleCombatRoundSpell> roundSpells;
        };

        class Hero;
        class SingleCombat
        {
        public:
            SingleCombat(int randSeed)
                : m_randEngine(randSeed), m_attackHero(), m_defenseHero() {}
            virtual ~SingleCombat();

            const Hero& attackHero() const {
                return m_attackHero;
            }

            const Hero& defenseHero() const {
                return m_defenseHero;
            }

            bool IsEnd() {
                return m_isEnd;
            }

            const SingleCombatResult& result() {
                return m_result;
            }

            void Init(I_Team& attackTeam, I_Team& defenceTeam);
            void Init(I_Hero& attackHero, I_Hero& defenceHero);

            //服务调用的函数
            bool Start();

            //客户端调用的2个函数
            //直到Next返回SingleCombatState::OVER
            SingleCombatState Next();
            //释放技能
            bool AttackInsertSpell(int spellBaseId);

            void SetAutomatic(bool isAutomatic) {
                m_isAutomatic = isAutomatic;
            }

            // 返回攻击方单挑战报
            WarReport::SoloData& AttackSoloReport()
            {
                return m_attackSoloReport;
            }

            // 返回防守方单挑战报
            WarReport::SoloData& DefenseSoloReport()
            {
                return m_defenseSoloReport;
            }

        protected:
            ////技能伤害，需要通过BUFF来计算
            // 发起攻击
            void Attack(Hero& source, Hero& target, bool nirvarna_flag);

            int GetHurt(Hero& source, Hero& target, bool nirvarna_flag);

            //设置技能
            virtual void CheckSpell();

            bool IsEscape(const int hpPro);

            WarReport::SoloResult JudgeWin();

            void Ready();
            void Begin(I_Hero& attackHero, I_Hero& defenceHero);

            void RoundBegin();
            void RoundCombat();
            void RoundEnd();

            void End();

        private:
            bool m_isAutomatic = false; //是否自动模式
            RandEngine m_randEngine;
            SingleCombatState m_state = SingleCombatState::INIT;
            bool m_isEnd = false;
            uint16_t m_round = 0;
            SingleCombatEventHandle::RoundBeginInput m_attackInput;
            SingleCombatEventHandle::RoundBeginInput m_defenseInput;
            SingleCombatEventHandle::RoundEndOutput m_attackOutput;
            SingleCombatEventHandle::RoundEndOutput m_defenseOutput;
            Hero m_attackHero;                              // attacker
            Hero m_defenseHero;                             // defender

            const model::tpl::NHeroTpl* m_attackHeroTpl = nullptr;
            const model::tpl::NHeroTpl* m_defenseHeroTpl = nullptr;
            model::tpl::CharacterInfluence::HeroSoloSkillProb m_attackSkillProb;
            model::tpl::CharacterInfluence::HeroSoloSkillProb m_defenseSKillProb;

            // 最大体力
            int m_attackMaxPhysical = 0;
            int m_defenseMaxPhysical = 0;

            // 下一次出手技能
            WarReport::HeroSoloSkill m_attackNextSkill = WarReport::HeroSoloSkill::NONE;
            WarReport::HeroSoloSkill m_defenseNextSkill = WarReport::HeroSoloSkill::NONE;

            int m_attackRage;                       //攻击方怒气值
            int m_defenseRage;                       //防守方怒气值
            int m_attackMaxHp;                        // 攻击方总体力
            int m_defenseMaxHp;                        // 防守方总体力
            WarReport::SoloResult m_isWin;                 //当前回合的输赢
            
            SingleCombatResult m_result;

            WarReport::SoloData m_attackSoloReport;        // 攻击方单挑战报
            WarReport::SoloData m_defenseSoloReport;        // 防守方单挑战报

            friend class Hero;
        };
    }
}

#endif // ENGINE_BATTLE_SINGLECOMBAT_H

