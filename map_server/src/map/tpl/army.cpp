#include "army.h"
#include <base/framework.h>
namespace ms
{
    namespace map
    {
        namespace tpl
        {
            using namespace model;

            bool ArmySkillTpl::ContainBattleType(BattleType type) const
            {
                for (BattleType t : battleTypes) {
                    if (t == type) {
                        return true;
                    }
                }
                return false;
            }

            int ArmyBattleTpl::GetActiveSkill() const
            {
                int rand = framework.random().GenRandomNum(10000);
                for (const ActiveSkill& skill : activeSkills) {
                    if (rand < skill.rate) {
                        return skill.id;
                    }
                }
                return 0;
            }

            float ArmyBattleTpl::GetRestraint(model::ArmyType type) const
            {
//                 switch (type) {
//                     case ArmyType::MONSTER:
//                         return armyType0;
//                     case ArmyType::INFANTRY:
//                         return armyType1;
//                     case ArmyType::PIKEMAN:
//                         return armyType2;
//                     case ArmyType::CAVALRY_RIDER:
//                         return armyType3;
//                     case ArmyType::CAVALRY_SHOOTER:
//                         return armyType4;
//                     case ArmyType::ARCHER:
//                         return armyType5;
//                     case ArmyType::CROSSBOWMAN:
//                         return armyType6;
//                     case ArmyType::CHARIOT_SHORT:
//                         return armyType7;
//                     case ArmyType::CHARIOT_LONG:
//                         return armyType8;
//                     default:
//                         break;
//                 }
                return 1.0;
            }

        }
    }
}
