#ifndef MAP_INFO_PROPERTY_H
#define MAP_INFO_PROPERTY_H
#include <model/metadata.h>
#include <map>
#include <base/data.h>
#include <base/3rd/rapidjson/document.h>
#include <base/3rd/rapidjson/writer.h>
#include <base/3rd/rapidjson/stringbuffer.h>

namespace base
{
    class DataTable;
}

namespace ms
{
    namespace map
    {
        class Agent;
        namespace info
        {

            struct PropertyInfo {
                PropertyInfo(int b, int p, int e):base(b), pct(p), ext(e){}
                
                int base = 0;
                int pct = 0;
                int ext = 0;
            };

            class Property {
            public:
                Property() {}
                ~Property() {}

//                 --建筑数据1
//                 --军营
                //int barrackTrainNums = 0;//               --军营每次训练士兵数量
                //float barrackTrainSpeedPct = 0;//           --军营训练士兵速度(百分比)
               // int barrackTrainQueues = 0;//             --军营训练队列数量
                int infantryBarrackTrainNums = 0;//               --军营每次训练士兵数量
                float infantryBarrackTrainSpeedPct = 0;//           --军营训练士兵速度(百分比)
                int infantryBarrackTrainQueues = 0;//             --barrackTrainNums = tb.Get("barrackTrainNums")->ToNumber();//               --军营每次训练士兵数量
                int riderBarrackTrainNums = 0;//           --军营训练士兵速度(百分比)
                float riderBarrackTrainSpeedPct = 0;//             --barrackTrainNums = tb.Get("barrackTrainNums")->ToNumber();//               --军营每次训练士兵数量
                int riderBarrackTrainQueues = 0;//           --军营训练士兵速度(百分比)
                int archerBarrackTrainNums = 0;//             --barrackTrainNums = tb.Get("barrackTrainNums")->ToNumber();//               --军营每次训练士兵数量
                float archerBarrackTrainSpeedPct = 0;//           --军营训练士兵速度(百分比)
                int archerBarrackTrainQueues = 0;//             --
                int mechanicalBarrackTrainNums = 0;//               --军营每次训练士兵数量
                float mechanicalBarrackTrainSpeedPct = 0;//           --军营训练士兵速度(百分比)
                int mechanicalBarrackTrainQueues = 0;//  
//                 --城墙
                int trapCapacity = 0;//                   --陷阱容量
                int trapMakeNums = 0;//                   --陷阱每次制造数量
                float trapTrainSpeedPct = 0;//              --陷阱制造速度(百分比)
                int cityDefense = 0;//                    --城防值
                float cityTroopsDefensePct = 0;               // (武将和部队)守军防御加成(百分比)
//                 --箭塔
                int turretAttackForce = 0;//              --箭塔攻击力
//                 --仓库
                int foodProtectedLimit = 0;//             --粮食保护上限
                int woodProtectedLimit = 0;//             --木材保护上限
                int stoneProtectedLimit = 0;//            --石料保护上限
                int ironProtectedLimit = 0;//             --铁矿保护上限
//                 --伤兵营
                int woundedCapacity = 0;//                --伤兵容量

//                 --城内资源建筑
                int foodOutPut = 0;//                     --粮食产出/H
                int foodCapacity = 0;//                   --粮食容量
                int woodOutPut = 0;//                     --木材产出/H
                int woodCapacity = 0;//                   --木材容量
                int stoneOutPut = 0;//                    --石料产出/H
                int stoneCapacity = 0;//                  --石料容量
                int ironOutPut = 0;//                     --铁矿产出/H
                int ironCapacity = 0;//                   --铁矿容量
//                 --书院
                int technologyQueues = 0;//               --研究队列数量
//                 --城主府
                int castleTaxSilver = 0;//                 --城主府征收收益(银两)

//                 --部队1009~1020
                float troopAttackPct = 0;//                  --部队攻击百分比加成
                float troopAttackExt = 0;//                  --部队攻击额外加成
                float troopDefensePct = 0;//                 --部队防御百分比加成
                float troopDefenseExt = 0;//                 --部队防御额外加成
                float troopHpPct = 0;//                      --部队生命百分比加成
                float troopHpExt = 0;//                      --部队生命额外加成
                float troopSpeedPct = 0;//                   --部队速度百分比加成
                float troopSpeedExt = 0;//                   --部队速度额外加成
                float troopAttackCityPct = 0;//              --部队攻城攻击百分比加成
                float troopAttackCityExt = 0;//              --部队攻城攻击额外加成
                float troopMorale = 0;//                     --部队怒气
                float troopLoadPct = 0;//                    --部队负重百分比加成
                float troopLoadExt = 0;//                    --部队负重额外加成
                float attackMonsterSpeedPct = 0;//           --打怪行军速度加成(百分比)
                float marchSpeedPct = 0;//                   --行军速度加成(百分比)
                float scoutSpeedPct = 0;//                   --侦查速度加成(百分比)
                float robResourcePct = 0;//                  --增加掠夺他人资源上限(百分比)
                
                float camptempSpeedPct = 0;//               --驻扎行军速度(百分比)
                float gatherSpeedPct = 0;//                 --采集速度(百分比)
                float attackPlayerSpeedPct = 0;//           --攻击玩家行军速度(百分比)
                float reinforcementsSpeedPct = 0;//         --援军行军速度(百分比)

//                 --2000
//                 --map地图相关
                float foodGatherSpeedPct = 0;//             --粮食采集速度加成(百分比)
                float woodGatherSpeedPct = 0;//             --木材采集速度加成(百分比)
                float stoneGatherSpeedPct = 0;//            --石料采集速度加成(百分比)
                float ironGatherSpeedPct = 0;//             --铁采集速度加成(百分比)

//                 --
                float upkeepReducePct = 0;//                --部队粮食消耗减少(百分比)
                float reduceResearchPct = 0;//              --减少科技研究消耗(百分比)
//                 --3000
                float buildingSpeedPct = 0;//               --建筑建造速度加成(百分比)
                float healSpeedPct = 0;//                   --伤兵恢复速度加成(百分比)
                float healReduceResourcePct = 0;//          --医疗所需资源减少(百分比)

                int maxMarchingQueue = 0;//               --出征队伍数量上限
                int maxHeroNum = 0;   //                     --每个队伍可配将数量上限

                int allianceReinforcementNum = 0;          // 同盟增援部队数量
                float transportSpeedUpPct = 0;                  //增加运输资源速度(百分比)

                //4012
                float trapAttackBase = 0;                  // --陷阱攻击力基础值
                float trapAttackPct = 0;                   // --陷阱攻击百分比加成
                float trapAttackExt = 0;                   // --陷阱攻击额外加成

                // 武将属性相关
                // 5001-5019
                float heroPowerBase = 0;                //    --武将武力基础加成
                float heroPowerPct = 0;             //    --武将武力百分比加成
                float heroPowerExt = 0;             //    --武将武力额外加成
                float heroDefenseBase = 0;              //    --武将统帅基础加成
                float heroDefensePct = 0;               //    --武将统帅百分比加成
                float heroDefenseExt = 0;               //    --武将统帅额外加成
                float heroWisdomBase = 0;               //    --武将智力基础加成
                float heroWisdomPct = 0;                //    --武将智力百分比加成
                float heroWisdomExt = 0;                //    --武将智力额外加成
                float heroSkillBase = 0;                //    --武将士气基础加成
                float heroSkillPct = 0;             //    --武将士气百分比加成
                float heroSkillExt = 0;             //    --武将士气额外加成
                float heroAgileBase = 0;                //    --武将速度基础加成
                float heroAgilePct = 0;             //    --武将速度百分比加成
                float heroAgileExt = 0;             //    --武将速度额外加成
                float heroLuckyBase = 0;                //    --武将运气基础加成
                float heroLuckyPct = 0;             //    --武将运气百分比加成
                float heroLuckyExt = 0;             //    --武将运气额外加成
                float heroLifeBase = 0;             //    --武将攻城基础加成
                float heroLifePct = 0;              //    --武将攻城百分比加成
                float heroLifeExt = 0;              //    --武将攻城额外加成
                float heroPhysicalAttackBase = 0;               //    --武将物理攻击力基础加成
                float heroPhysicalAttackPct = 0;                //    --武将物理攻击力百分比加成
                float heroPhysicalAttackExt = 0;                //    --武将物理攻击力额外加成
                float heroPhysicalDefenseBase = 0;              //    --武将物理防御力基础加成
                float heroPhysicalDefensePct = 0;               //    --武将物理防御力百分比加成
                float heroPhysicalDefenseExt = 0;               //    --武将物理防御力额外加成
                float heroWisdomAttackBase = 0;                //    --武将谋略攻击力基础加成
                float heroWisdomAttackPct = 0;             //    --武将谋略攻击力百分比加成
                float heroWisdomAttackExt = 0;             //    --武将谋略攻击力额外加成
                float heroWisdomDefenseBase = 0;                //    --武将谋略防御力基础加成
                float heroWisdomDefensePct = 0;             //    --武将谋略防御力百分比加成
                float heroWisdomDefenseExt = 0;             //    --武将谋略防御力额外加成
                float heroHitBase = 0;              //    --武将命中值基础加成
                float heroHitPct = 0;               //    --武将命中值百分比加成
                float heroHitExt = 0;               //    --武将命中值额外加成
                float heroAvoidBase = 0;                //    --武将回避值基础加成
                float heroAvoidPct = 0;             //    --武将回避值百分比加成
                float heroAvoidExt = 0;             //    --武将回避值额外加成
                float heroCritHitBase = 0;              //    --武将暴击命中值基础加成
                float heroCritHitPct = 0;               //    --武将暴击命中值百分比加成
                float heroCritHitExt = 0;               //    --武将暴击命中值额外加成
                float heroCritAvoidBase = 0;                //    --武将暴击回避值基础加成
                float heroCritAvoidPct = 0;             //    --武将暴击回避值百分比加成
                float heroCritAvoidExt = 0;             //    --武将暴击回避值额外加成
                float heroSpeedBase = 0;                //    --武将攻击速度基础加成
                float heroSpeedPct = 0;             //    --武将攻击速度百分比加成
                float heroSpeedExt = 0;             //    --武将攻击速度额外加成
                float heroCityLifeBase = 0;             //    --武将攻城值基础加成
                float heroCityLifePct = 0;              //    --武将攻城值百分比加成
                float heroCityLifeExt = 0;              //    --武将攻城值额外加成
                float heroFightBase = 0;                //    --武将兵力上限基础加成
                float heroFightPct = 0;             //    --武将兵力上限百分比加成
                float heroFightExt = 0;             //    --武将兵力上限额外加成
                float heroSingleEnergyBase = 0;             //    --武将单挑血量基础加成
                float heroSingleEnergyPct = 0;              //    --武将单挑血量百分比加成
                float heroSingleEnergyExt = 0;              //    --武将单挑血量额外加成

                //6011-6018
                float riderPowerBase = 0;                  //骑兵攻击力基础加成
                float riderPowerPct = 0;                  //骑兵攻击力百分比加成
                float riderPowerExt = 0;                  //骑兵攻击力额外加成
                float infantryPowerBase = 0;                  //步兵攻击力基础加成
                float infantryPowerPct = 0;                  //步兵攻击力百分比加成
                float infantryPowerExt = 0;                  //步兵攻击力额外加成
                float archerPowerBase = 0;                  //弓兵攻击力基础加成
                float archerPowerPct = 0;                  //弓兵攻击力百分比加成
                float archerPowerExt = 0;                  //弓兵攻击力额外加成
                float mechanicalPowerBase = 0;                  //器械攻击力基础加成
                float mechanicalPowerPct = 0;                  //器械攻击力百分比加成
                float mechanicalPowerExt = 0;                  //器械攻击力额外加成
                float riderDefenseBase = 0;                  //骑兵防御力基础加成
                float riderDefensePct = 0;                  //骑兵防御力百分比加成
                float riderDefenseExt = 0;                  //骑兵防御力额外加成
                float infantry_defenseBase = 0;                  //步兵防御力基础加成
                float infantry_defensePct = 0;                  //步兵防御力百分比加成
                float infantry_defenseExt = 0;                  //步兵防御力额外加成
                float archerDefenseBase = 0;                  //弓兵防御力基础加成
                float archerDefensePct = 0;                  //弓兵防御力百分比加成
                float archerDefenseExt = 0;                  //弓兵防御力额外加成
                float mechanicalDefenseBase = 0;                  //器械防御力基础加成
                float mechanicalDefensePct = 0;                  //器械防御力百分比加成
                float mechanicalDefenseExt = 0;                  //器械防御力额外加成

                int reinforceLimit = 0;                         //援兵容纳队列上限


                int canAttackMonsterLevel = 0;                  //可攻击野怪等级
                int canAttackEliteMonsterLevel = 0;             //可攻击精英野怪等级

                //6004 - 6007 
                float trapHorseDamagePct = 0;   //拒马克骑兵伤害
                float trapHorseDamageExt = 0;   //拒马克骑兵伤害
                float trapStoneDamagePct = 0;   //镭石克步兵伤害
                float trapStoneDamageExt = 0;   //镭石克步兵伤害
                float trapWoodDamagePct = 0;    //滚木克弓兵伤害
                float trapWoodDamageExt = 0;    //滚木克弓兵伤害
                float trapOilDamagePct = 0; //火油克器械伤害
                float trapOilDamageExt = 0; //火油克器械伤害
                //6025 - 6028 
                float riderRestrainInfantryPct = 0;     //骑兵克制步兵
                float infantryRestrainArcherPct = 0;    //步兵克制弓兵
                float archerRestrainMachinePct = 0;     //弓兵克制器械
                float machineRestrainRiderPct = 0;      //器械克制骑兵

                //6034-6036
                float attackMonsterDamageBase = 0;                  //攻击野怪伤害基础加成
                float attackMonsterDamagePct = 0;                  //攻击野怪伤害百分比加成
                float attackMonsterDamageExt = 0;                  //攻击野怪伤害额外加成
                float attackEliteMonsterDamageBase = 0;                  //攻击精英野怪伤害基础加成
                float attackEliteMonsterDamagePct = 0;                  //攻击精英野怪伤害百分比加成
                float attackEliteMonsterDamageExt = 0;                  //攻击精英野怪伤害额外加成
                float attackWorldbossDamageBase = 0;                  //攻击BOSS伤害基础加成
                float attackWorldbossDamagePct = 0;                  //攻击BOSS伤害百分比加成
                float attackWorldbossDamageExt = 0;                  //攻击BOSS伤害额外加成

                void ReadDataTable(const base::DataTable& tb, Agent& agent);

                void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const;

                bool Deserialize(rapidjson::Document& doc);

                void Debug();

                //获取兵种攻击力加成
                void armyAttackAddition(model::ArmysType armyType, float& base, float& pct, float& ext) const;
                void armyDefenseAddition(model::ArmysType armyType, float& base, float& pct, float& ext) const;

                float armyHealthAddition(model::ArmyType armyType) const;
                float armyDamageAddition(model::ArmyType atkArmyType) const;

                //保留
                float armyDamagedReduceEx(model::ArmyType myArmyType, model::ArmyType enemyArmyType, model::AttackType myAttackType) const;   //DELETE

                float armyDamageToXReduce(model::ArmyType myArmyType) const;
                float armyReduceEnemyXDamage(model::ArmyType enemyArmyType) const;
                float armyDamageToXReduce(model::AttackType myAttackType) const;
            };
        }
    }
}
#endif  //MAP_INFO_PROPERTY_H

