
#ifndef NPCARMYTPL_H
#define NPCARMYTPL_H
#include <vector>
# include <string>
# include<model/tpl/hero.h>

namespace ms
{
    namespace map
    {
        namespace tpl
        {
//             struct SkillConfig {
//                 SkillConfig(int _id, int _level)
//                     : id(_id), level(_level) {}
//                 int id = 0;
//                 int level = 0;
//             };
//
//             struct EquipConfig {
//                 EquipConfig(int _id, int _level)
//                     : id(_id), level(_level) {}
//                 int id = 0;
//                 int level = 0; //强化等级
//             };

            struct HeroConfig {
                int id = 0;
                int level = 0;
                int star = 0;
                int soul = 0;
                std::vector<model::tpl::HeroSkill> skill;
                int position = 0;
            };

            struct ArmyConfig {
                int type = 0;
                int level = 0;
                int count = 0;
            };

            struct NpcArmyTpl {
                int id = 0;//                               int not null comment 'ID',
                int groupId = 0;//                          int not null comment '组ID',
                HeroConfig hero;//                             varchar(64) not null comment '武将ID_等级 格式 {131000,10}',
                ArmyConfig army;//                            varchar(128) not null comment '部队_等级_数量 格式 {10001,10,100}',
                int weight = 0;                              //                           int not null comment '权重',
                int headId = 0;
                std::string name;
                int level = 0;
            };

        }
    }
}

#endif // NPCARMYTPL_H
