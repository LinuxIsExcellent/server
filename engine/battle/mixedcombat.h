#ifndef ENGINE_BATTLE_MIXEDCOMBAT_H
#define ENGINE_BATTLE_MIXEDCOMBAT_H
#include <list>
#include "team.h"
#include "../input_struct.h"
#include "../randengine.h"
#include "../tpl/t_buff.h"

namespace model
{
    namespace tpl
    {
        struct SkillTpl;
    }
}

namespace engine
{
    namespace battle
    {

        //混战普通攻击加怒气
        const static int kAddRageForMixedNormalAttack = 3;

        enum class MixedCombatState
        {
            INIT,
            SMALL_READY,
            COMPLEX_READY,    
            WAITING,  //等待释放技能，客户端在这个状态下释放技能

            TURRET_READY,      //箭塔ready
            WAITING2,
            OVER,
        };

        //回合类型
        enum class MixedRoundType
        {
            ROUND_UNKOWN,
            ROUND_SPELL,        //技能回合
            ROUND_SMALL,        //小回合
            ROUND_COMPLEX,
            ROUND_TURRET,        //箭塔回合
        };

        enum class SmallRoundState 
        {
            PREPARE,
            DECIDE_MODEL, 
            CHOOSE_TARGET,
            DECIDE_HIT,
            PRODUCE_DAMAGE,
            OVER,
        };

        //进攻节点
        struct AttackNode {
            AttackNode(engine::TeamType _teamType , int _position)
                :teamType(_teamType), position(_position) {}
            engine::TeamType teamType = engine::TeamType::NONE;
            int position = 0;
        };

        class SmallRoundNode;
        class ComplexRoundNode;
        class TurretRoundNode;

        class RoundNode
        {
        public:
            RoundNode() {}
            virtual ~RoundNode() {}

            virtual bool IsOwner(engine::TeamType _teamType , int _position) {
                return false;
            }

            virtual engine::TeamType teamType() {
                return engine::TeamType::DEFENDER;
            }

            virtual int position() {
                return 0;
            }

            SmallRoundNode* ToSmallRoundNode();
            TurretRoundNode* ToTurretRoundNode();
            ComplexRoundNode* ToComplexRoundNode();

        protected:
            MixedRoundType m_type = MixedRoundType::ROUND_UNKOWN;
        };

        class SmallRoundNode : public RoundNode
        {
        public:
            SmallRoundNode(engine::TeamType _teamType , int _position)
                :m_attackNode(_teamType,  _position) {
                m_type = MixedRoundType::ROUND_SMALL;
            }

            virtual bool IsOwner(engine::TeamType _teamType , int _position) {
                return m_attackNode.teamType == _teamType && m_attackNode.position ==  _position;
            }

            virtual engine::TeamType teamType() {
                return m_attackNode.teamType;
            }

            virtual int position() {
                return m_attackNode.position;
            }

            AttackNode m_attackNode;
        };

        class ComplexRoundNode : public RoundNode 
        {
        public:
            ComplexRoundNode(engine::TeamType _teamType , int _position)
                :m_attackNode(_teamType,  _position) {
                m_type = MixedRoundType::ROUND_COMPLEX;
            }

            virtual engine::TeamType teamType() {
                return m_attackNode.teamType;
            }

            virtual int position() {
                return m_attackNode.position;
            }

            AttackNode m_attackNode;

        };

        class TurretRoundNode : public RoundNode
        {
        public:
            TurretRoundNode() {
                m_type = MixedRoundType::ROUND_TURRET;
            }
        };

        class MixedCombat
        {
        public:
            MixedCombat(int randSeed)
                : m_randEngine(randSeed) {}
            virtual ~MixedCombat() {
                ClearNode();
            }

            bool IsEnd() {
                return m_isEnd;
            }

            const MixedCombatResult& result() {
                return m_result;
            }

            WarReport& report() {
                return m_report;
            }

            bool Init(I_Team& attackTeam, I_Team& defenseTeam, const WarReport::SoloData* attackSoloReport, const WarReport::SoloData* defenseSoloReport);

            // 副本使用，为了评定星级
            int beginArmyCount()
            {
                return m_beginArmyCount;
            }

            int endArmyCount()
            {
                return m_endArmyCount;
            }

            int GetAbsolutePos(Hero* hero);
            Hero* GetAbsolutePosHero(int positon);
            Hero* GetAnyHero();
            //服务调用的函数
            bool Start();

            ////客户端调用的2个函数
            //直到Next返回MixedCombatState::OVER
            MixedCombatState Next();
            bool AttackInsertSpell(int position, int spellBaseId);

            void SetAutomatic(bool isAutomatic) {
                m_isAutomatic = isAutomatic;
            }

            void SetMaxRound(int mixedMaxRound) {
                m_mixedMaxRound = mixedMaxRound;
            }

        protected:
            //获取2个位置的距离
            int GetDistance(int fromPos, int toPos);

            // 被攻击
            void OnBeAttacked(Hero* toGroup, Hero* fromGroup);

            //执行BUFF
            void CastBuff(Hero& fromGroup, Hero& toGroup, const engine::tpl::T_Buff& tBuff,  int spellLevel, bool isPassive, 
                int hurt);
            void GetBuffTargets(Hero& fromGroup, Hero& toGroup, const engine::tpl::T_Buff& tBuff, std::vector<Hero*> &targets);

            void CastPassiveSkill(Hero& nhero, const model::tpl::SkillTpl* skillTpl, int level, int hurt);
 
            void CheckDie(engine::TeamType teamType);
            void OnGroupDie(Hero* hero);

            //兵种克制系数
            int ArmyRestraint(ArmyType type, ArmyType otherType);

            bool Ready();
            //小回合
            bool SmallRoundReady();
            void SmallRoundCombat();

            bool CheckComplex(Hero* hero);
            bool ComplexRoundReady();
            void ComplexRoundCombat();
            bool ComplexAttack(Hero* hero, Hero* toHero);

            //检查无效回合
            void CheckInvalidRound();
            //大回合开始
            void BigRoundBegin();
            //大回合结束
            void BigRoundEnd();

            // 各种回合结束
            void RoundEnd();

            bool ExistTurret();
            bool TurretRoundReady();
            void TurretRoundCombat();

            //战斗结束
            void End();
            void ClearNode();

            //重置出手顺序
            void CheckAttackNodeOrder();
            
            //增援效果之和    
            int AssistSum(Team& team, Hero* hero);
            
            //处理死亡
            void OnHeroDie(Hero* hero);

            //查找攻击对象
            Hero* FindHeroAttackTo(Hero* fromHero);

            Team& FromTeam(engine::TeamType teamType) {
                if (teamType == engine::TeamType::ATTACKER) {
                    return m_attackTeam;
                } else {
                    return m_defenseTeam;
                }
            }

            Team& ToTeam(engine::TeamType teamType) {
                if (teamType == engine::TeamType::ATTACKER) {
                    return m_defenseTeam;
                } else {
                    return m_attackTeam;
                }
            }

            bool CheckHit(Hero* hero, Hero* target);
            bool CheckCrit(Hero* hero, Hero* target);
            int CalcHurt(Hero* hero, Hero* target, bool crit_flag);

            bool NormalAttack(Hero* hero, Hero* toHero);
            void GetEffectTargets(Hero& fromGroup, EffectRangeType rangeType, std::vector<Hero*>& targets, 
                std::vector<Hero*>& effect_targets);
            void CastAreaEffect(Hero* hero, const engine::tpl::T_Effect& effect, std::vector<Hero*>& targets, int level, 
                WarReport::BuffData& buffData);
            void CastSingleEffect(Hero* hero, const engine::tpl::T_Effect& effect, Hero* target, int level, int hurt, 
                WarReport::BuffData& buffData);
            void CastReleaseCondEffect(Hero* hero, std::vector<Hero*>& targets, int level, WarReport::SkillData& skillData);
            void CastHurtEffect(Hero* hero, Hero* toHero, std::vector<Hero*>& targets, 
                std::unordered_map<Hero*, int>& target_map, int level, WarReport::SkillData& skillData);
            bool NirvarnaAttack(Hero* hero, Hero* toHero);
            bool CheckEffectCond(Hero* fromHero, Hero* toHero, EffectCondType condType);

            void RecordReport(Hero* hero, Hero* toHero, WarReport::AttackType attackType, 
                int hurt, int changeRage);

            float AddPhysicalHurt(Hero* hero, float hurt);
            float AddMagicHurt(Hero* hero, float hurt);
            float MinusPhysicalHurt(Hero* hero, float hurt);
            float MinusMagicHurt(Hero* hero, float hurt);

            Hero* FindRandomHero(Hero* hero);

        private:
            bool m_isAutomatic = false; //是否自动模式
            int m_mixedMaxRound = 10;// 混战最大回合数
            RandEngine m_randEngine;
            MixedCombatState m_state = MixedCombatState::INIT;
            bool m_isEnd = false;
            uint16_t m_bigRound = 0;
            uint16_t m_smallRound = 0;
            uint16_t m_spellRound = 0;
            uint16_t m_complexRound = 0;
            Team m_attackTeam;
            Team m_defenseTeam;

            uint16_t m_beginArmyCount = 0;
            uint16_t m_endArmyCount = 0;

            SmallRoundState m_smallRound_state = SmallRoundState::PREPARE;

            MixedCombatResult m_result;

            WarReport m_report;

            std::list<AttackNode> m_attackNodes;        //当前大回合小回合节点（临时保存）

            std::list<RoundNode*> m_roundNodes;             // 当前所有回合节点

            engine::TeamType m_singleWinner = engine::TeamType::NONE;   //单挑胜利方
            Hero* m_curAttacker = nullptr;
        };
    }
}

#endif // ENGINE_BATTLE_MIXEDCOMBAT_H

