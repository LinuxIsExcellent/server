#ifndef MODEL_TPL_CONFIGURE_H
#define MODEL_TPL_CONFIGURE_H
#include <base/data.h>
#include "drop.h"
#include <vector>

namespace model
{
    namespace tpl
    {
        struct ResourceLoadConf {
            int food = 1;
            int wood = 1;
            int iron = 6;
            int stone = 24;
            int gold = 1000;
        };
        
        // 怪物攻城
        struct MonsterSiegeConf {
            int intervalTime = 3600;  //怪物攻击时间间隔(秒)
            float extractionCity = 0.3f; //抽取所有玩家的百分比
            int continuedTime = 10800;    //持续时间反攻(秒)
            float resourcesPercent = 0.05f;  //抢夺资源百分比
            int mixGrade = 2;   //最小可攻击城堡等级
            int lvMinCoe = 2;     //怪物等级系数(求最小等级)
            int lvRange = 0;     //怪物等级区间范围
            std::vector<DropItem> extraDrops;
            
            int RandomMonsterLevel(int castleLevel) const;
        };
        
        struct TurretConf {
            float attackSpeed = 1.0f;    //攻击速度
            float hurtCoe = 20.0f;  //攻击系数
        };
        
        struct BattleConf {
            float injuryratio = 0.0f;      // 伤兵换算系数
            int traptake = 0;                  // 陷阱取值(百分比）
            float wallprotect=0.5;      // 城防伤害减免判定值
        };

        // ｛武力转化系数，智力转化系数，防御转化系数，谋略转化系数，血量转化系数，速度转化系数｝
        struct BattleConfig {
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
        
        // 武将战力
        struct HeroPowerConfig {
            double wuli = 42.5;
            double zhili = 42.5;
            double fangyu = 42.5;
            double moulue = 42.5;
            double xueliang = 85;
            double sudu = 17;
            double divisor = 8;
        };

        struct NeutralCastleConf {
            float attackPlayerTimeAttenuation = .0f;        //攻击玩家时间衰减
            int attackPlayerTimeMin = 14400;
            int attackPlayerTimeMax = 14400;
            int initCount = 88;                             //中立城池初始个数
            float troopPercent = 0.1f;                      //中立城池出兵数量百分比(玩家兵力占比)
            int castleLevel = 1;                            //攻击玩家的最低等级
            int playerProtectedTime = 3600;                 //玩家被攻击后的保护时间
        };
        
        struct PalaceWarConf {
            int firstStartTime = 300;         //第一次开始时间戳 = firstStartTime + 开服时间戳
            int recoverTime = 30;               //恢复间隔
            float dragonRecoverPercent = .01f; //龙恢复血量百分比
            int occupyTime = 300;             //结束争霸所需占领时间
            int chooseKingTime = 10;         //盟主选王时间限制
            int peaceTime = 300;              //和平期持续时间
        };
        
        struct CatapultConf {
            //打龙
            struct AttackDragon {
                float basePercent = .0002222f;         //基础百分比
                float correction = .0000000002949f;    //修正系数
                float gainCoe = .0625f;                //增益系数
            };
            //攻击玩家
            struct AttackPlayer {
                int baseAttack = 1569000;               //基础攻击力
                float correction = 1.0f;                //修正系数
                float gainCoe = 1.0f;                   //增益系数
            };
            int attackTime = 20;     //魔法塔攻击间隔
            int recoverTime = 20;    //恢复间隔
            AttackDragon attackDragon;
            AttackPlayer attackPlayer;
        };
        
        struct ResourceRateConf {
            int food = 3600;
            int wood = 3600;
            int iron = 1800;
            int stone = 1000;
        };
        
        // 王城争霸备战物资
        struct PalaceWarPrepareConf {
            int times = 0;
            int time = 0;       //距离开战时间(秒)
            int dropId = 0;     //物资 : 掉落id
        };
        
        // 新手保护配置
        struct NoviceProtectedConf {
            int level = 6;
            int time = 3600;
        };
        
        // 王城争霸打龙排行配置
        struct PalaceWarAttackDragonConf {
            int rank;
        };
        
        // 龙穴配置
        struct DragonNestConf {
            int initCount;
        };
        
        // 地精营地
        struct GoblinCampConf {
            int castleLevelLimit = 10;
            int digTime = 3600;
            int refreshInterval = 3600;
            int roundInterval = 10800;
            int aX = 300;
            int aY = 300;
            int bX = 900;
            int bY = 900;
            int ownCount = 5;
        };

        // 行军相关配置
        struct TroopConfig {
            // 创建行营消耗
            struct CampFixedConsumedConfig {
                int food = 0;
                int wood = 0;
                int iron = 0;
                int stone = 0;
            };

            struct ScoutConsumedConfig {
                int food = 0;
            };

            CampFixedConsumedConfig campFixedConfig;
            ScoutConsumedConfig scoutConfig;
            int patrolCD = 0;
        };

        // //掠夺补偿
        // //{foodbase=1000,foodr=20,woodbase=800,woodr=20,stonebase=600,stoner=20,ironbase=80,ironr=20}
        // struct CompensateConfig {
        //     int foodbase = 0;
        //     int foodr = 0;
        //     int woodbase = 0;
        //     int woodr = 0;
        //     int stonebase = 0;
        //     int stoner = 0;
        //     int ironbase = 0;
        //     int ironr = 0;
        // };

        //掠夺相关配置
        struct PlunderConfig {
            int plunderlimit = 0;
            int perplunderlimit = 0;
            float recovery = 0.0f;
            float foodoutresourcelimit = 0.0f;
            float woodoutresourcelimit = 0.0f;
            float stoneoutresourcelimit = 0.0f;
            float ironoutresourcelimit = 0.0f;
            float foodinnerMax = 0.0f;     
            float woodinnerMax = 0.0f;     
            float stoneinnerMax = 0.0f;     
            float ironinnerMax = 0.0f;     
            std::vector<std::tuple<int, float>> levelLimits; // 等级差减免
            std::vector<std::tuple<int, float>> timeDecays;  //次数衰减系数

            //CompensateConfig compensateConfig;
        };

        struct CityWallConfig {
            float recovery = 0.0f;
        };

        //名城相关
        struct CityConfig {
            int occupation = 0; //名城占领转主权时间
            int armyRecoveryTime = 0; //名城守军恢复时间
        };

        // {burnbasetime=1800,burnincreasetime=300,gold=1,recovery=0.3,born=0.1,base=800,ratio1=0.1,ratio2=200,span=10}
        struct WallValueConfig {
            int burnbasetime=1800;
            int burnincreasetime=300;
            int gold=1;
            float recovery=0.3f;
            float born=0.1f;
            int base=800;
            float ratio1=0.1;
            int ratio2=200;
            int span=10;
            int reduce=1;
        };

        // {price=50,monneycover=0.001,monneycoverbase=500,monneycd=3600,gold=5,goldcd=7200}
        struct WallsRecoveryConfig {
            int price=50;
            float monneycover=0.001;
            int monneycoverbase=500;
            int monneycd=3600;
            int gold=5;
            int goldcd=7200;
        };

        // {prefecture=10,chow=5}
        struct  CityNumLimit 
        {
            int prefecture = 10;
            int chow = 5;
        };

        struct HeroPhysical
        {   
            int initial = 1000;  //初始值
            int increase = 300;     //每等级增加
            float recovery = 0.01;     //恢复百分比
            int interval = 5;        //恢复时间间隔
        }; 

        // 单挑武将性格参数
        struct CharacterInfluence
        {
            struct HeroSoloSkillProb
            {   
                int stone = 0;
                int shear = 0;
                int cloth = 0;
            };

            std::vector<HeroSoloSkillProb> heroSoloSkillProbs;
        };

        // 单挑怒气
        struct SoloRage
        {   
            int init = 0;
            int max = 0;
            int success = 0;
            int flat = 0;
            int fail = 0;
            int consume = 0;
        };

        // 逃跑概率
        struct EscapePro
        {
            int hpMin = 0;
            int hpMax = 0;
            int pro = 0;
        };

        // 单挑减兵机制
        struct SoloLose
        {
            float runaway = 0.05;
            float death = 0.1;
        };

        // 行营耐久
        struct CortressDurable
        {
            int durable = 0;       // 初始耐久值
            int recoveryTime = 0;     // 恢复时间间隔 
            int recovery = 0;           // 恢复值
        };

        //伤病补偿
        struct Compensate 
        {
            int resourceBase = 0;
            int food = 0;
            int wood = 0;
            int stone = 0;
            int iron = 0;
            int first = 0;
            int second = 0;
            int third = 0;
            int fourth = 0;
            int fifth = 0;
        };
        

        // 尽量不要用DataTable 先读取到自定义结构体 以免配置错误导致运行时出错
        struct Configure {            
            ResourceLoadConf resourceLoad;
            WallValueConfig wallsValue;
            WallsRecoveryConfig wallsRecovery;
            MonsterSiegeConf monsterSiege;
            TurretConf turret;
            BattleConf battle;
            BattleConfig battleConfig;
            HeroPowerConfig heroPowerConfig;
            NeutralCastleConf neutralCastle;
            PalaceWarConf palaceWar;
            CatapultConf catapult;
            std::vector<ResourceRateConf> resourceRates;
            std::vector<PalaceWarPrepareConf> palaceWarPrepares;
            int castleProtectedLevel = 6;
            NoviceProtectedConf noviceProtected;
            PalaceWarAttackDragonConf palaceWarAttackDragon;
            int activeUserTime = 14;        //活跃用户时间 单位：天
            DragonNestConf dragonNest;
            GoblinCampConf goblinCamp;
            TroopConfig troopConfig;
            PlunderConfig plunderConfig;
            CityWallConfig cityWallConfig;
            CityConfig cityConfig;
            CityNumLimit cityNumLimit;
            HeroPhysical heroPhysical;
            
            CharacterInfluence characterInfluence;
            SoloRage soloRage;
            std::vector<EscapePro> escapePros;
            SoloLose soloLose;
            int escapeSuc = 5000;                     //单挑中逃跑成功概率
            CortressDurable cortressDurable;
            int searchScope = 6;          // 大地图搜索范围
            int scoutLimit = 5;         //侦察队列数量

            Compensate compensate; //战斗损兵补偿 
        };
    }
}
#endif // MODEL_TPL_CONFIGURE_H
