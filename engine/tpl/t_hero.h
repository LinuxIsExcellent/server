#ifndef ENGINE_MODEL_T_HERO_H
#define ENGINE_MODEL_T_HERO_H
#include <vector>

namespace engine
{
    namespace tpl
    {
        struct T_Hero {
            int id = 0;
            int hp = 0;
            int attack = 0;
            int defense = 0;
            int strategy = 0;       // 谋略
            int speed = 0;
            int hpUp = 0;
            int attackUp = 0;
            int defenseUp = 0;
            int strategyup = 0;
            int speedUp = 0;
            int rage = 0;                                    // 怒气值
            int office = 0;                                  // '文官0,武将1.
            int sex = 0;                                    // '性别 1男 2女'
            std::vector<int> singleSkillCalcul;
        };

        // {wuli=42.5,zhili=42.5,fangyu=42.5,moulue=42.5,xueliang=85,sudu=17}
        struct T_BattleConfig {
            double wuli = 42.5;
            double zhili = 42.5;
            double fangyu = 42.5;
            double moulue = 42.5;
            double xueliang = 85;
            double sudu = 17;

            // 单挑结果转化
            int win_die_rage = 50;                              // 若击杀敌方武将，怒气增加
            double win_die_attr_up = 0.1;            // 若击杀敌方武将，混战时，己方所有部队属性+10%
            double win_active_attr_up = 0.05;     //若击败未击杀，混战时，己方所有部队属性+5%
            double lose_die_attr_down = -0.05;         // 若被击杀，混战时，己方所有部队属性-5%
            double lose_active_attr_down = 0;          // 若被击败未击杀，混战时，己方所有部队属性-0%
        };

        struct T_HeroPowerConfig {
            double wuli = 42.5;
            double zhili = 42.5;
            double fangyu = 42.5;
            double moulue = 42.5;
            double xueliang = 85;
            double sudu = 17;
            double divisor = 8;
        };
    }
}

#endif // ENGINE_MODEL_T_HERO_H
