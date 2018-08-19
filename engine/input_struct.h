#ifndef BATTLE_INPUT_STRUCT_H
#define BATTLE_INPUT_STRUCT_H
#include <stdint.h>
#include <vector>
#include <string>

#include "metadata.h"

namespace engine
{
    //技能
    struct I_Spell {
        int id = 0;
        int level = 0;
    };

    //武将
    struct I_Hero {
        int id = 0;
        //int tplId = 0;
        unsigned short level = 0;
        unsigned short star = 0;

        int physical = 0;    //生命值
        float heroPower = 0.0;   //武将武力
        float heroDefense = 0.0;   //武将统帅
        float heroWisdom = 0.0;   //武将智力
        float heroLucky = 0.0;  //武将运气
        float heroSkill = 0.0;   //武将士气
        float heroAgile = 0.0;   //武将速度
        float heroLife = 0.0;   //武将攻城
        float heroPhysicalPower = 0.0;       //武将物理攻击力
        float heroPhysicalDefense = 0.0;       //武将物理防御力
        float heroSkillPower = 0.0;       //武将谋略攻击力
        float heroSkillDefense = 0.0;       //武将谋略防御力
        float heroHit = 0.0;       //武将命中值
        float heroAvoid = 0.0;       //武将回避值
        float heroCritHit = 0.0;       //武将暴击命中值
        float heroCritAvoid = 0.0;       //武将暴击回避值
        float heroSpeed = 0.0;       //武将攻击速度
        float heroCityLife = 0.0;       //武将攻城值
        int heroSolohp = 0;    //武将单挑血量上限
        int heroTroops = 0;    //武将领导力

        std::vector<I_Spell> spells; 
    };

    //部队
    struct I_Army {
        int     armyType = 0;
        int     armyLevel = 0;
        int     armyCount = 0;
        int     armyCountMax = 0;                           // 兵上限

        int hp = 0;                    //生命值
        int attack = 0;            //攻击力
        int defense = 0;        //防御力
        int speed = 0;            //速度
    };

    struct I_Group {
        I_Hero hero;
        I_Army army;
        int position = 0;
    };

    struct I_Prop {
        //6004 - 6007 
        float trapHorseDamagePct = 0;  
        float trapHorseDamageExt = 0;   
        float trapStoneDamagePct = 0;   
        float trapStoneDamageExt = 0;   
        float trapWoodDamagePct = 0;    
        float trapWoodDamageExt = 0;    
        float trapOilDamagePct = 0; 
        float trapOilDamageExt = 0; 

        //6025 - 6028 
        float riderRestrainInfantryPct = 0;     
        float infantryRestrainArcherPct = 0;    
        float archerRestrainMachinePct = 0;     
        float machineRestrainRiderPct = 0;     
    };

    struct I_Team {
        std::vector<I_Group> groups;
        int turretAtkPower = 0;         // 箭塔攻击力
        int trapAtkPercentage = 10;     // 抽取陷阱百分比
        int rage = 0;                   // 怒气
    };

    //初始数据输入
    struct  InitialDataInput {
        uint64_t uid = 0;               //玩家Id
        int headIcon = 0;               //头像
        std::string name;               // 名字
        int level = 0;                  // 等级
        int singleCombatHero = 0;       //指定单挑英雄
        I_Team team;  //队伍信息
        I_Prop prop; //属性
    };

    // 单挑结果
    struct SingleCombatResult {
        bool isAttackWin = false;
        int attackHeroId = 0;
        int attackHeroHp = 0;     //攻击方单挑之后的血量
        int attackOutputHurt = 0;     //攻击方对防守方产生的减血量

        int defenseHeroId = 0;
        int defenseHeroHp = 0;      //防守方单挑之后的血量
        int defenseOutputHurt = 0;      //防守方对攻击方产生的减血量

        double attackLoseArmyPercent = 0;   //攻击方损失多少兵
        double defenseLostArmyPercent = 0;    //防守方损失多少兵

        /*double attackerAddAttrPercent = 0; //属性加成
        double attackerBuffDiscount =0; //武将对应混战时的属性BUFF，技能被动BUFF，按剩余血量比例折扣。

        double defenderAddAttrPercent = 0; //属性加成
        double defenderBuffDiscount =0; //武将对应混战时的属性BUFF，技能被动BUFF，按剩余血量比例折扣。*/
    };

    // 混战结果
    struct MixedCombatResult {
        bool isAttackWin = false;
        
        struct ArmyHurt {
            int heroId = 0;
            int heroHp = 0;
            int armyType = 0;
            int activeCount = 0;
            int dieCount = 0;
            int killCount = 0;
        };

        //伤害汇总
        struct Hurt {
            int heroId = 0;
            int armyType = 0;
            int dieCount = 0;
            int armyLevel = 0;
        };
       
        std::vector<ArmyHurt> attackHurt;
        std::vector<ArmyHurt> defenseHurt;

        std::vector<Hurt> attackTotalHurt;
        std::vector<Hurt> defenseTotalHurt;

        int attackOldPower = 0;
        int attackNewPower = 0;
        int defenseOldPower = 0;
        int defenseNewPower = 0;

        int attackTotalDie = 0;
        int defenseTotalDie = 0;
    };


    //////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////
    //战斗回放数据结构
    //类型 1,攻击方  2，防守方 3,平手
    enum EMAttackType {
        NONE = 0,
        ATTACK,
        DEFNESE,
    };

    //混战总数据
    struct WarReport {
        enum ArmyDataType
        {
            Init = 1,
            SoloEnd = 2,
        };

        enum AttackType 
        {
            NORMAL = 1,
            NIRVARNA = 2,
            COMPLEX = 3,
        };
        
        //武将部队数据
        struct HeroArmyData {
            //类型
            TeamType attackType;
            //武将部队ID
            int armyId;
            //武将ID
            int heroId;
            //士兵ID
            int soldierId;
            //血量即士兵数量
            int hp;
            //初始站位
            int initPosition;

            int power;
            int defense; 
            int wisdom;
            int skill;
            int agile;
            int lucky;
        };
    
        struct HeroArmyDatas {
            std::vector<HeroArmyData> heroArmyData;
            ArmyDataType armyDataType;
        };

        // struct HeroState {
        //     int armyId;
        //     std::vector<int> status;
        // };

        //对阵双方初始数据
        struct InitTeamData {
            //玩家id
            int64_t uid;
            //头像id
            int headId;
            //玩家昵称
            std::string nickName;
            //单挑武将id
            int64_t soloHeroId;
        };
    
        struct Receiver {
            int armyId;
            int value;  
        };

        struct BuffData {
            int buffType;
            int receiverRound;
            int keepRound;
            std::vector<Receiver> receivers;
        };

        //技能数据
        struct SkillData {
            //技能ID
            int skillId;
            //接受者
            std::vector<Receiver> receivers;
            std::vector<BuffData> buffs;
        };

        //合体技
       struct ComplexRound
        {
            int armyId;
            int skillId;
            std::vector<int> armyIds;
            std::vector<Receiver> receivers;
            std::vector<BuffData> buffs;
        };

        //小回合
        struct UnitRound {  
            AttackType castType;
            //武将部队ID
            int armyId;
            //对阵目标部队ID
            int targetId;   
            //普通技
            SkillData skill;
            int changeHp;
            int changeBatVal; //战意值
            int changeRageVal; //怒气值
            int crit = 0; //暴击
        };
    
        struct BigRound {
            //std::vector<HeroState> heroStatus;
            std::vector<UnitRound> complexRounds;
            std::vector<UnitRound> allUnitRounds;
            int bigRoundId;
        };
    
        //结果数据
        struct ResultData {
            //胜利方ID
            TeamType win;
        };

        // 武将单挑出招技能
        enum class HeroSoloSkill
        {
            NONE,
            STONE,    // 石头
            SHEAR,     // 剪刀
            CLOTH,      // 布
            SOLO_NIRVARNA,    // 必杀
            ESCAPE,       // 逃跑
        };

        // 单挑战斗结果
        enum class SoloResult
        {
            NONE,
            ATTACKER,     // 进攻方
            DEFNESE,       // 防守方
            DEUCE,          //平局
        };

        // 单挑小回合数据
        struct SoloRound {
            HeroSoloSkill action;   // 技能
            int rageChange;           // 怒气变化
            int hpChange;             // 血量变化
            SoloResult result;    // 本回合结果
        };

        struct SoloData {
            TeamType attackType;   // 攻击类型
            //玩家id
            int64_t uid;
            //头像id
            int headId;
            //玩家昵称
            std::string nickName;
            int Id;         // 英雄Id
            int hp;        // 初始血量
            int rage;       // 初始怒气
            int power;      // 战斗力
            std::vector<SoloRound> rounds;   // 小回合
            SoloResult result;            //单挑总结果
            double loseArmysPercent;      //削减兵力百分比
        };

        struct ArenaData {
            int roundId;
            std::vector<SoloData> datas;
        };

        //战斗ID
        int battleId;
        //对阵双方初始数据
        std::vector<InitTeamData> initTeamDatas;
        //箭塔陷阱回合数
        std::vector<int> trapIds;
        //武将部队数据
        std::vector<HeroArmyDatas> heroArmyDatas;
        //所有的大回合
        std::vector<BigRound> allBigRounds;
        //混战战斗结果
        ResultData result;
        // 单挑战斗数据
        std::vector<SoloData> soloDatas;
        // 擂台数据
        std::vector<ArenaData> arenaDatas;
    };
}

#endif
