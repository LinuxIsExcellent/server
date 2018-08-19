#ifndef MODEL_TPL_HERO_H
#define MODEL_TPL_HERO_H
#include "../metadata.h"
#include <vector>
#include "item.h"

namespace model
{
    namespace tpl
    {
        struct HeroSkill {
            HeroSkill(int _id, int _level)
                : id(_id), level(_level) {}

            int id = 0;
            int level = 0;
        };

        struct HeroEquip {
            HeroEquip(int _id, int _level)
                : id(_id), level(_level) {}

            int id = 0;
            int level = 0; //强化等级
        };

        struct HeroTpl {
            int id = 0;
            int sex = 0;     // '性别 1男 2女'
            short camp = 0;
            int level = 0;
            short quality = 0;
            int office = 0;                                  // '文官0,武将1.
            int leadership = 0; //统率力
            int hp = 0;
            int attack = 0;
            int defense = 0;
            int strategy = 0;       // 谋略
            int speed = 0;
            int intellect = 0;
            int star = 0;
            int rage = 0;           // 怒气值
            int challenge = 0; //单挑值
            int leadershipUp = 0; //统率力加成(等级)
            int hpUp = 0;
            int attackUp = 0;
            int defenseUp = 0;
            int strategyup = 0;
            int speedUp = 0;
            int saberPrac = 0;           // 剑兵亲和度
            int pikemanPrac = 0; //枪兵亲和度
            int halberdierPrac = 0;           // 戟兵亲和度
            int archerPrac = 0; //弓兵亲和度
            int riderPrac = 0;           // 骑兵亲和度
            int chariotPrac = 0; //冲车亲和度
            int stoneThrowerPrac = 0;           // 投石车亲和度
            int warElephantPrac = 0; //战象亲和度           
            int initSkill = 0;           // 武将初始化技能
            int uniqueSkill = 0;   //武将无双技能
            std::vector<int> singleSkillCalcul;
        };

        struct NHeroTpl {
            int id = 0;
            int sex = 0;     // '性别 1男 2女'

            int quality = 0;
            int job = 0;
            int damageType = 0;
    
            float power = 0;
            float defense = 0;
            float wisdom = 0; //智力
            float skill = 0; //技巧
            float agile = 0; //敏捷
            float lucky = 0; 
            float life = 0; //攻城 
            int soloHp = 0; //生命值
            int troops = 0; //兵力
    
            float pikemanSkill = 0;
            float cavalrySkill = 0;
            float archerSkill = 0;
            float mechanicsSkill = 0;
    
            //特技 
            int technicalSkill1 = 0;
            int technicalSkill2 = 0;
            int technicalSkill3 = 0;
            int technicalSkill4 = 0;
            int technicalSkill5 = 0;
            int technicalSkill6 = 0;
    
            int nirvanaSkill = 0;
            int cooperativeSkill = 0;
            
            //协力武将
            // int synergyRole1 = 0;
            // int synergyRole2 = 0;
            // int synergyRole3 = 0;
            // int synergyRole4 = 0;
            // int synergyRole5 = 0;
            std::vector<int> synergyRoles;
    
            //支援加成
            int supportPlus1 = 0;
            int supportPlus2 = 0;
            int supportPlus3 = 0;
            int supportPlus4 = 0;
            int supportPlus5 = 0;
    
            //支援效果 
            int supportValues1 = 0;
            int supportValues2 = 0;
            int supportValues3 = 0;
            int supportValues4 = 0;
            int supportValues5 = 0;

            //武将性格
            int trait = 0;
        };

        struct SkillTpl {
            int id = 0;
            int skillType = 0;
            int damageType = 0;
            int lableJob = 0;

            int angerConsumption = 0;
            int warConsumption = 0;
            int prestigeConsumption = 0;
            int levelCap = 0;

            int basicParameters = 0;
            int levelParameters = 0;
            int effectLast = 0;
            int hitJudgment = 0;
            int directHit = 0;
            int skillRange = 0;
            int attacks = 0;
            int professionalJob = 0;
            int restrainHurt = 0;

            int passiveType1 = 0;
            float passiveParameter1 = 0;
            float passiveCoefficient1 = 0;
            int passiveType2 = 0;
            float passiveParameter2 = 0;
            float passiveCoefficient2 = 0;

            int stuntType1 = 0;
            float stuntParameter1 = 0;
            float stuntCoefficient1 = 0;
            int stuntType2 = 0;
            float stuntParameter2 = 0;
            float stuntCoefficient2 = 0;
            int stuntType3 = 0;
            float stuntParameter3 = 0;
            float stuntCoefficient3 = 0;

            int effectCondition1 = 0;
            int effectRange1 = 0;
            int addedEffect1 = 0;
            int additionalValue1 = 0;
            int upValue1 = 0;
            int effectCondition2 = 0;
            int effectRange2 = 0;
            int addedEffect2 = 0;
            int additionalValue2 = 0;
            int upValue2 = 0;

            int nSkillicon = 0;
            int nSkillSpecial = 0;
            int nSkillSound = 0;
        };


        struct HeroSoulTpl {
            int id;
            int nDamageType;
            int soulLevel;
            AttributeType type;
            AttributeAdditionType addType;
            float value;
        };

        struct HeroLevelAttrTpl {
            int heroesId;
            int levelmin;
            int levelmax;
            float nGrowthPower;
            float nGrowthDefense;
            float nGrowthWisdom;
            float nGrowthSkill;
            float nGrowthAgile;
            float nGrowthLucky;
            float nGrowthLife;
            int nGrowthTroops;
            int nGrowthSolohp;
        };

        struct HeroStarLevelTpl {
            int heroesId;
            int level;
            float PowerAdd;
            float DefenseAdd;
            float WisdomAdd;
            float SkillAdd;
            float AgileAdd;
            float LuckyAdd;
            float LifeAdd;
            int SolohpAdd;
            int TroopsAdd;
        };
    }
}

#endif // MODEL_TPL_HERO_H
