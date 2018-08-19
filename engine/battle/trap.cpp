#include "trap.h"

namespace engine
{
    namespace battle
    {
        using namespace tpl;
        bool Trap::Init()
        {
            // m_tArmy = g_battleTploader.FindArmy(m_iArmy.armyType, m_iArmy.armyLevel);
            // m_attack = m_iArmy.attack;
            // m_attack = m_attack * (1 + m_addAttrPercent);
            // m_count = m_iArmy.armyCount;

            return false;
        }

        int Trap::AttackCount(int percentage)
        {
            return m_count * percentage / 100 + (m_count * percentage % 100 > 0  ?  1 : 0);
        }

        int TrapExt::Count()
        {
            return m_count;
        }

        bool TrapExt::CanAttack(int armyType)
        {
            bool canAttack = false;
            switch (Type()) {
                case 10:
                    canAttack = armyType ==  1 ||  armyType ==  2;
                    break;
                case 11:
                    canAttack = armyType ==  3 ||  armyType ==  5;
                    break;
                case 12:
                    canAttack = armyType ==  4 ||  armyType ==  8;
                    break;
                default:
                    break;
            }
            return canAttack;
        }
    }
}
