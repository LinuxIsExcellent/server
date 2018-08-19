#ifndef MAP_TPL_ARMY_H
#define MAP_TPL_ARMY_H
#include <model/metadata.h>
#include <string>
#include <vector>

namespace ms
{
    namespace map
    {
        namespace tpl
        {
            struct ArmySkillTpl {
                int id;
                model::AttackType attackType; // 攻击类型 1所有 2攻击 3防守
                float attackHurt;

                //todel
                model::ArmySkillType type; // 效果类型
                std::vector<model::BattleType> battleTypes; //适用战斗类型
                float param1;
                float param2;
                float param3;
                float param4;

                bool ContainBattleType(model::BattleType type) const;
            };

            struct ArmyBattleTpl {
                struct ActiveSkill {
                    int id;
                    int rate;
                };
                model::ArmyType armyType;
                int x;
                int y;

                float armyType0; //野怪
                float armyType1; //盾兵
                float armyType2; //枪兵
                float armyType3; //战骑
                float armyType4; //骑射
                float armyType5; //弓手
                float armyType6; //弩手
                float armyType7; //近战车
                float armyType8; //远程车

                std::vector<ActiveSkill> activeSkills; //主动技能列表
                std::vector<int> passiveSkills; //被动动技能列表

                int GetActiveSkill() const;
                //获取相克系数
                float GetRestraint(model::ArmyType type) const;
            };

        }
    }
}

#endif // ARMY_H
