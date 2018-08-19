#ifndef MODEL_TPL_ARMY_H
#define MODEL_TPL_ARMY_H
#include "../metadata.h"
#include <string>
#include <vector>

namespace model
{
    namespace tpl
    {
        struct ArmyTpl {
            struct ArmyEffect {
                ArmyEffect(int atype, double pt)
                :armyType(atype), percentage(pt) {}

                int armyType;
                double percentage;
            };
            
            int id;
            std::string name;    
            int mainType;
            int subType;
            int level;
            int hp;
            int attack;
            int defense;
            int speed;
            int attackRange;
            int attackWall;
            int loads;
            float foodConsumption;
            float power;
	        std::vector<int> priority;
            
            std::vector<ArmyEffect> armyEffects;
            std::vector<int> armyAttrList;
            	    
            bool IsTrap() const {
                return subType ==  (int)model::ArmyType::CHEVAL_DE_FRISE ||  subType ==  (int)model::ArmyType::ROLLING_LOGS || subType ==  (int)model::ArmyType::GRIND_STONE;
            }
        };

        struct NArmyTpl {
            struct ArmyEffect {
                ArmyEffect(int atype, double pt)
                :armyType(atype), percentage(pt) {}

                int armyType;
                double percentage;
            };

            struct AttrValue {
                int type;
                int addType;
                float value;
            };
            int id;
            std::string jobName;
            int jobType;
            int range;

            std::vector<AttrValue> attrList;
            // int   hp;
            std::string   description;
            int   level;
            int speed; 
            int loads;
            std::vector<int> priority;

            std::vector<ArmyEffect> armyEffects;

            bool IsTrap() const {
                return jobType ==  (int)model::ArmysType::ROLLING_LOGS ||  jobType ==  (int)model::ArmysType::GRIND_STONE || jobType ==  (int)model::ArmysType::CHEVAL_DE_FRISE || jobType ==  (int)model::ArmysType::KEROSENE;
            }
        };
   
        struct SkillBaseTpl {
            int id;
            int singleNodeId;
            int mixNodeId;
            bool active;
            bool super;
            int weight;
        };

        struct SkillNodeTpl {
            int id;
            int typeExt;                    // 1逃跑 2伪退
            std::vector<int> buffIds;
            int rage;
        };
          
         struct BuffTpl {             
            int id;
            int target;
            int type;               // 1单挑buff 2混战buff
            bool effectNow;
            int valueType;
            int value;
            unsigned int rounds;
            std::vector<int> coefficients;  // 系数
            int probability;
            bool isDebuff;
            std::vector<int> buffs;  // 被动buff
        };
        
        //攻击折扣
        struct AttackDiscountTpl {
            int target;
            int discount; //折扣（万分比）
        };
        
        //部队技能
        struct ArmySpellTpl {
            int id;
            std::vector<int> buffIds;
            int probability;
        };
        
        // 战斗伤兵恢复逻辑
        struct BattleInjuredSoldiersTpl {
            int action;
            float injuredSoldiers;
            float deadSoldiers;
            float defInjuredSoldiers;
            float defDeadSoldiers;
        };

    }
}

#endif // MODEL_TPL_ARMY_H
