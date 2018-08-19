#ifndef ENGINE_BATTLE_TRAP_H
#define ENGINE_BATTLE_TRAP_H
# include <list>
#include "../tpl/t_army.h"
#include "../input_struct.h"

namespace engine
{
    namespace battle
    {
        class Trap
        {
        public:
            Trap(I_Army iArmy)
                : m_iArmy(iArmy) {}
            ~Trap() {}

            bool Init();

            int attack() {
                return m_attack;
            }

            int Type() {
                return m_iArmy.armyType;
            }

            int Level() {
                return m_iArmy.armyLevel;
            }

            int count() {
                return m_count;
            }

            int dieCount() {
                return m_dieCount;
            }

            int AttackCount(int percentage);

            void Remove(int count) {
                m_count -=  count;
                m_dieCount +=  count;
            }

            int killCount() {
                return m_killCount;
            }

            void AddKillCount(int killCount) {
                m_killCount +=  killCount;
            }

            void SetAddAttrPercent(double addAttrPercent) {
                m_addAttrPercent  = addAttrPercent;
            }

        protected:
            I_Army m_iArmy;
            int32_t m_attack = 0; // 攻击力
            int32_t m_count = 0; // 数量
            int32_t m_dieCount = 0; // 死亡数量
            int m_killCount = 0;                            // 击杀数量

            double m_addAttrPercent = 0; //属性加成
            const tpl::T_Army* m_tArmy = nullptr;
        };


        class TrapExt :  public Trap
        {
        public:
            TrapExt(I_Army iArmy)
                : Trap(iArmy) {}
            ~TrapExt() {}

             int type() {
                    return m_iArmy.armyType;
             }

            int Count();

            // 是否可以攻击
            bool CanAttack(int armyType);

            int attackTo() {
                return m_attackTo;
            }

            void SetAttackTo(int toPos) {
                m_attackTo = toPos;
            }

        protected:

            int m_attackTo = 0;
        };

    }
}

#endif
