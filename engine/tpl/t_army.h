#ifndef ENGINE_MODEL_T_ARMY_H
#define ENGINE_MODEL_T_ARMY_H
#include <vector>
#include <array>

namespace engine
{
    namespace tpl
    {
        struct T_Army {
            struct ArmyEffect {
                ArmyEffect(int type, double pt)
                    :armyType(type), percentage(pt) {}

                int armyType;
                double percentage;
            };

            void Init();

            int id;
            int mainType;
            int subType;
            int level;
            int hp;
            int attack;
            int defense;
            int speed;
            int attackRange;
            double power;
            std::vector<ArmyEffect> armyEffects;
            std::vector<int> armyAttrList; //部队技能

            //在Init();里初始化
            std::array<double, 9> armyRestrain; //部队克制关系
        };

        //部队技能
        struct T_ArmySpell {
            int id;
            std::vector<int> buffIds;
            int probability;
        };
    }
}

#endif // ENGINE_MODEL_T_ARMY_H
