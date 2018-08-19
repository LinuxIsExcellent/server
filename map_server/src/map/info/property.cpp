#include "property.h"
#include "../agent.h"
#include "../unit/castle.h"

namespace ms
{
    namespace map
    {
        namespace info
        {
            using namespace std;
            using namespace base;
            using namespace model;

            void Property::ReadDataTable(const base::DataTable& tb, Agent& agent)
            {
                /*tb.ForeachMap([](const DataValue& k, const DataValue& v) {
                    cout << "k = " << k.ToString() << " v = " << v.ToNumber() << endl;
                    return false;
                });*/
                int oldCityDefense = cityDefense;

//                 --建筑数据1
//                 --军营
                infantryBarrackTrainNums = tb.Get("infantryBarrackTrainNums")->ToNumber();//               --军营每次训练士兵数量
                infantryBarrackTrainSpeedPct = tb.Get("infantryBarrackTrainSpeedPct")->ToNumber();//           --军营训练士兵速度(百分比)
                infantryBarrackTrainQueues = tb.Get("infantryBarrackTrainQueues")->ToNumber();//             --barrackTrainNums = tb.Get("barrackTrainNums")->ToNumber();//               --军营每次训练士兵数量
                riderBarrackTrainNums = tb.Get("riderBarrackTrainNums")->ToNumber();//           --军营训练士兵速度(百分比)
                riderBarrackTrainSpeedPct = tb.Get("riderBarrackTrainSpeedPct")->ToNumber();//             --barrackTrainNums = tb.Get("barrackTrainNums")->ToNumber();//               --军营每次训练士兵数量
                riderBarrackTrainQueues = tb.Get("riderBarrackTrainQueues")->ToNumber();//           --军营训练士兵速度(百分比)
                archerBarrackTrainNums = tb.Get("archerBarrackTrainNums")->ToNumber();//             --barrackTrainNums = tb.Get("barrackTrainNums")->ToNumber();//               --军营每次训练士兵数量
                archerBarrackTrainSpeedPct = tb.Get("archerBarrackTrainSpeedPct")->ToNumber();//           --军营训练士兵速度(百分比)
                archerBarrackTrainQueues = tb.Get("archerBarrackTrainQueues")->ToNumber();//             --
                mechanicalBarrackTrainNums = tb.Get("mechanicalBarrackTrainNums")->ToNumber();//               --军营每次训练士兵数量
                mechanicalBarrackTrainSpeedPct = tb.Get("mechanicalBarrackTrainSpeedPct")->ToNumber();//           --军营训练士兵速度(百分比)
                mechanicalBarrackTrainQueues = tb.Get("mechanicalBarrackTrainQueues")->ToNumber();//             --军营训练队列数量
//                 --城墙
                trapCapacity = tb.Get("trapCapacity")->ToNumber();//                   --陷阱容量
                trapMakeNums = tb.Get("trapMakeNums")->ToNumber();//                   --陷阱每次制造数量
                trapTrainSpeedPct = tb.Get("trapTrainSpeedPct")->ToNumber();//              --陷阱制造速度(百分比)
                cityDefense = tb.Get("cityDefense")->ToNumber();//                    --城防值
//                 --箭塔
                turretAttackForce = tb.Get("turretAttackForce")->ToNumber();//              --箭塔攻击力
//                 --仓库
                foodProtectedLimit = tb.Get("foodProtectedLimit")->ToNumber();//             --粮食保护上限
                woodProtectedLimit = tb.Get("woodProtectedLimit")->ToNumber();//             --木材保护上限
                stoneProtectedLimit = tb.Get("stoneProtectedLimit")->ToNumber();//            --石料保护上限
                ironProtectedLimit = tb.Get("ironProtectedLimit")->ToNumber();//             --铁矿保护上限
//                 --伤兵营
                woundedCapacity = tb.Get("woundedCapacity")->ToNumber();//                --伤兵容量

//                 --城内资源建筑
                foodOutPut = tb.Get("foodOutPut")->ToNumber();//                     --粮食产出/H
                foodCapacity = tb.Get("foodCapacity")->ToNumber();//                   --粮食容量
                woodOutPut = tb.Get("woodOutPut")->ToNumber();//                     --木材产出/H
                woodCapacity = tb.Get("woodCapacity")->ToNumber();//                   --木材容量
                stoneOutPut = tb.Get("stoneOutPut")->ToNumber();//                    --石料产出/H
                stoneCapacity = tb.Get("stoneCapacity")->ToNumber();//                  --石料容量
                ironOutPut = tb.Get("ironOutPut")->ToNumber();//                     --铁矿产出/H
                ironCapacity = tb.Get("ironCapacity")->ToNumber();//                   --铁矿容量
//                 --书院
                technologyQueues = tb.Get("technologyQueues")->ToNumber();//               --研究队列数量
//                 --城主府
                castleTaxSilver = tb.Get("castleTaxSilver")->ToNumber();//                 --城主府征收收益(银两)
                
//                 --部队1009~1020
                troopAttackPct = tb.Get("troopAttackPct")->ToNumber();//                  --部队攻击百分比加成
                troopAttackExt = tb.Get("troopAttackExt")->ToNumber();//                  --部队攻击额外加成
                troopDefensePct = tb.Get("troopDefensePct")->ToNumber();//                 --部队防御百分比加成
                troopDefenseExt = tb.Get("troopDefenseExt")->ToNumber();//                 --部队防御额外加成
                troopHpPct = tb.Get("troopHpPct")->ToNumber();//                      --部队生命百分比加成
                troopHpExt = tb.Get("troopHpExt")->ToNumber();//                      --部队生命额外加成
                troopSpeedPct = tb.Get("troopSpeedPct")->ToNumber();//                   --部队速度百分比加成
                troopSpeedExt = tb.Get("troopSpeedExt")->ToNumber();//                   --部队速度额外加成
                troopAttackCityPct = tb.Get("troopAttackCityPct")->ToNumber();//              --部队攻城攻击百分比加成
                troopAttackCityExt = tb.Get("troopAttackCityExt")->ToNumber();//              --部队攻城攻击额外加成
                troopMorale = tb.Get("troopMorale")->ToNumber();//                     --部队怒气
                troopLoadPct = tb.Get("troopLoadPct")->ToNumber();//                    --部队负重百分比加成
                troopLoadExt = tb.Get("troopLoadExt")->ToNumber();//                    --部队负重额外加成
                attackMonsterSpeedPct = tb.Get("attackMonsterSpeedPct")->ToNumber();//           --打怪行军速度加成(百分比)
                marchSpeedPct = tb.Get("marchSpeedPct")->ToNumber();//                   --行军速度加成(百分比)
                scoutSpeedPct = tb.Get("scoutSpeedPct")->ToNumber();//                   --侦查速度加成(百分比)
                robResourcePct = tb.Get("robResourcePct")->ToNumber();//                  --增加掠夺他人资源上限(百分比)

                camptempSpeedPct = tb.Get("camptempSpeedPct")->ToNumber();//               --驻扎行军速度(百分比)
                gatherSpeedPct = tb.Get("gatherSpeedPct")->ToNumber();//                 --采集速度(百分比)
                attackPlayerSpeedPct = tb.Get("attackPlayerSpeedPct")->ToNumber();//           --攻击玩家行军速度(百分比)
                reinforcementsSpeedPct = tb.Get("reinforcementsSpeedPct")->ToNumber();//         --援军行军速度(百分比)
//                 --2000
//                 --map地图相关
                foodGatherSpeedPct = tb.Get("foodGatherSpeedPct")->ToNumber();//             --粮食采集速度加成(百分比)
                woodGatherSpeedPct = tb.Get("woodGatherSpeedPct")->ToNumber();//             --木材采集速度加成(百分比)
                stoneGatherSpeedPct = tb.Get("stoneGatherSpeedPct")->ToNumber();//            --石料采集速度加成(百分比)
                ironGatherSpeedPct = tb.Get("ironGatherSpeedPct")->ToNumber();//             --铁采集速度加成(百分比)


//                 --
                upkeepReducePct = tb.Get("upkeepReducePct")->ToNumber();//                --部队粮食消耗减少(百分比)
                reduceResearchPct = tb.Get("reduceResearchPct")->ToNumber();//              --减少科技研究消耗(百分比)
//                 --3000
                buildingSpeedPct = tb.Get("buildingSpeedPct")->ToNumber();//               --建筑建造速度加成(百分比)
                healSpeedPct = tb.Get("healSpeedPct")->ToNumber();//                   --伤兵恢复速度加成(百分比)
                healReduceResourcePct = tb.Get("healReduceResourcePct")->ToNumber();//          --医疗所需资源减少(百分比)


                maxMarchingQueue = tb.Get("maxMarchingQueue")->ToNumber();//               --出征队伍数量上限
                maxHeroNum =tb.Get("maxHeroNum")->ToNumber();//                     --每个队伍可配将数量上限
                allianceReinforcementNum = tb.Get("allianceReinforcementNum")->ToNumber();    // 同盟增援部队数量
                transportSpeedUpPct = tb.Get("transportSpeedUp")->ToNumber();


                 heroPowerBase = tb.Get("heroPowerBase")->ToNumber();                //   --武将武力基础加成                                
                heroPowerPct = tb.Get("heroPowerPct")->ToNumber();              //   --武将武力百分比加成                 
                heroPowerExt = tb.Get("heroPowerExt")->ToNumber();              //   --武将武力额外加成                      
                heroDefenseBase = tb.Get("heroDefenseBase")->ToNumber();                //   --武将统帅基础加成    
                heroDefensePct = tb.Get("heroDefensePct")->ToNumber();              //   --武将统帅百分比加成    
                heroDefenseExt = tb.Get("heroDefenseExt")->ToNumber();              //   --武将统帅额外加成    
                heroWisdomBase = tb.Get("heroWisdomBase")->ToNumber();              //   --武将智力基础加成
                heroWisdomPct = tb.Get("heroWisdomPct")->ToNumber();                //   --武将智力百分比加成  
                heroWisdomExt = tb.Get("heroWisdomExt")->ToNumber();                //   --武将智力额外加成  
                heroSkillBase = tb.Get("heroSkillBase")->ToNumber();                //   --武将士气基础加成  
                heroSkillPct = tb.Get("heroSkillPct")->ToNumber();              //   --武将士气百分比加成 
                heroSkillExt = tb.Get("heroSkillExt")->ToNumber();              //   --武将士气额外加成  
                heroAgileBase = tb.Get("heroAgileBase")->ToNumber();                //   --武将速度基础加成  
                heroAgilePct = tb.Get("heroAgilePct")->ToNumber();              //   --武将速度百分比加成  
                heroAgileExt = tb.Get("heroAgileExt")->ToNumber();              //   --武将速度额外加成  
                heroLuckyBase = tb.Get("heroLuckyBase")->ToNumber();                //   --武将运气基础加成   
                heroLuckyPct = tb.Get("heroLuckyPct")->ToNumber();              //   --武将运气百分比加成  
                heroLuckyExt = tb.Get("heroLuckyExt")->ToNumber();              //   --武将运气额外加成  
                heroLifeBase = tb.Get("heroLifeBase")->ToNumber();              //   --武将攻城基础加成  
                heroLifePct = tb.Get("heroLifePct")->ToNumber();                //   --武将攻城百分比加成
                heroLifeExt = tb.Get("heroLifeExt")->ToNumber();                //   --武将攻城额外加成
                heroPhysicalAttackBase = tb.Get("heroPhysicalAttackBase")->ToNumber();              //   --武将物理攻击力基础加成  
                heroPhysicalAttackPct = tb.Get("heroPhysicalAttackPct")->ToNumber();                //   --武将物理攻击力百分比加成    
                heroPhysicalAttackExt = tb.Get("heroPhysicalAttackExt")->ToNumber();                //   --武将物理攻击力额外加成     
                heroPhysicalDefenseBase = tb.Get("heroPhysicalDefenseBase")->ToNumber();                //   --武将物理防御力基础加成      
                heroPhysicalDefensePct = tb.Get("heroPhysicalDefensePct")->ToNumber();              //   --武将物理防御力百分比加成      
                heroPhysicalDefenseExt = tb.Get("heroPhysicalDefenseExt")->ToNumber();              //   --武将物理防御力额外加成      
                heroWisdomAttackBase = tb.Get("heroWisdomAttackBase")->ToNumber();                //   --武将谋略攻击力基础加成      
                heroWisdomAttackPct = tb.Get("heroWisdomAttackPct")->ToNumber();              //   --武将谋略攻击力百分比加成      
                heroWisdomAttackExt = tb.Get("heroWisdomAttackExt")->ToNumber();              //   --武将谋略攻击力额外加成      
                heroWisdomDefenseBase = tb.Get("heroWisdomDefenseBase")->ToNumber();                //   --武将谋略防御力基础加成  
                heroWisdomDefensePct = tb.Get("heroWisdomDefensePct")->ToNumber();              //   --武将谋略防御力百分比加成      
                heroWisdomDefenseExt = tb.Get("heroWisdomDefenseExt")->ToNumber();              //   --武将谋略防御力额外加成  
                heroHitBase = tb.Get("heroHitBase")->ToNumber();                //   --武将命中值基础加成  
                heroHitPct = tb.Get("heroHitPct")->ToNumber();              //   --武将命中值百分比加成      
                heroHitExt = tb.Get("heroHitExt")->ToNumber();              //   --武将命中值额外加成  
                heroAvoidBase = tb.Get("heroAvoidBase")->ToNumber();                //   --武将回避值基础加成      
                heroAvoidPct = tb.Get("heroAvoidPct")->ToNumber();              //   --武将回避值百分比加成      
                heroAvoidExt = tb.Get("heroAvoidExt")->ToNumber();              //   --武将回避值额外加成      
                heroCritHitBase = tb.Get("heroCritHitBase")->ToNumber();                //   --武将暴击命中值基础加成              
                heroCritHitPct = tb.Get("heroCritHitPct")->ToNumber();              //   --武将暴击命中值百分比加成              
                heroCritHitExt = tb.Get("heroCritHitExt")->ToNumber();              //   --武将暴击命中值额外加成              
                heroCritAvoidBase = tb.Get("heroCritAvoidBase")->ToNumber();                //   --武将暴击回避值基础加成              
                heroCritAvoidPct = tb.Get("heroCritAvoidPct")->ToNumber();              //   --武将暴击回避值百分比加成              
                heroCritAvoidExt = tb.Get("heroCritAvoidExt")->ToNumber();              //   --武将暴击回避值额外加成              
                heroSpeedBase = tb.Get("heroSpeedBase")->ToNumber();                //   --武将攻击速度基础加成              
                heroSpeedPct = tb.Get("heroSpeedPct")->ToNumber();              //   --武将攻击速度百分比加成              
                heroSpeedExt = tb.Get("heroSpeedExt")->ToNumber();              //   --武将攻击速度额外加成          
                heroCityLifeBase = tb.Get("heroCityLifeBase")->ToNumber();              //   --武将攻城值基础加成          
                heroCityLifePct = tb.Get("heroCityLifePct")->ToNumber();                //   --武将攻城值百分比加成              
                heroCityLifeExt = tb.Get("heroCityLifeExt")->ToNumber();                //   --武将攻城值额外加成                  
                heroFightBase = tb.Get("heroFightBase")->ToNumber();                //   --武将兵力上限基础加成                      
                heroFightPct = tb.Get("heroFightPct")->ToNumber();              //   --武将兵力上限百分比加成                      
                heroFightExt = tb.Get("heroFightExt")->ToNumber();              //   --武将兵力上限额外加成                      
                heroSingleEnergyBase = tb.Get("heroSingleEnergyBase")->ToNumber();              //   --武将单挑血量基础加成                              
                heroSingleEnergyPct = tb.Get("heroSingleEnergyPct")->ToNumber();                //   --武将单挑血量百分比加成                  
                heroSingleEnergyExt = tb.Get("heroSingleEnergyExt")->ToNumber();                //   --武将单挑血量额外加成 

                riderPowerBase = tb.Get("riderPowerBase")->ToNumber();                         
                riderPowerPct = tb.Get("riderPowerPct")->ToNumber();                         
                riderPowerExt = tb.Get("riderPowerExt")->ToNumber();                         
                infantryPowerBase = tb.Get("infantryPowerBase")->ToNumber();                         
                infantryPowerPct = tb.Get("infantryPowerPct")->ToNumber();                         
                infantryPowerExt = tb.Get("infantryPowerExt")->ToNumber();                         
                archerPowerBase = tb.Get("archerPowerBase")->ToNumber();                         
                archerPowerPct = tb.Get("archerPowerPct")->ToNumber();                         
                archerPowerExt = tb.Get("archerPowerExt")->ToNumber();                         
                mechanicalPowerBase = tb.Get("mechanicalPowerBase")->ToNumber();                         
                mechanicalPowerPct = tb.Get("mechanicalPowerPct")->ToNumber();                         
                mechanicalPowerExt = tb.Get("mechanicalPowerExt")->ToNumber();                         
                riderDefenseBase = tb.Get("riderDefenseBase")->ToNumber();                         
                riderDefensePct = tb.Get("riderDefensePct")->ToNumber();                         
                riderDefenseExt = tb.Get("riderDefenseExt")->ToNumber();                         
                infantry_defenseBase = tb.Get("infantry_defenseBase")->ToNumber();                         
                infantry_defensePct = tb.Get("infantry_defensePct")->ToNumber();                         
                infantry_defenseExt = tb.Get("infantry_defenseExt")->ToNumber();                         
                archerDefenseBase = tb.Get("archerDefenseBase")->ToNumber();                         
                archerDefensePct = tb.Get("archerDefensePct")->ToNumber();                         
                archerDefenseExt = tb.Get("archerDefenseExt")->ToNumber();                         
                mechanicalDefenseBase = tb.Get("mechanicalDefenseBase")->ToNumber();                         
                mechanicalDefensePct = tb.Get("mechanicalDefensePct")->ToNumber();                         
                mechanicalDefenseExt = tb.Get("mechanicalDefenseExt")->ToNumber();    


                if (0) {
                    //6004 - 6007 
                    trapHorseDamagePct = tb.Get("trapHorseDamagePct")->ToNumber();   //拒马克骑兵伤害
                    trapHorseDamageExt = tb.Get("trapHorseDamageExt")->ToNumber();   //拒马克骑兵伤害
                    trapStoneDamagePct = tb.Get("trapStoneDamagePct")->ToNumber();   //镭石克步兵伤害
                    trapStoneDamageExt = tb.Get("trapStoneDamageExt")->ToNumber();   //镭石克步兵伤害
                    trapWoodDamagePct = tb.Get("trapWoodDamagePct")->ToNumber();    //滚木克弓兵伤害
                    trapWoodDamageExt = tb.Get("trapWoodDamageExt")->ToNumber();    //滚木克弓兵伤害
                    trapOilDamagePct = tb.Get("trapOilDamagePct")->ToNumber(); //火油克器械伤害
                    trapOilDamageExt = tb.Get("trapOilDamageExt")->ToNumber(); //火油克器械伤害

                    //6025 - 6028 
                    riderRestrainInfantryPct = tb.Get("riderRestrainInfantryPct")->ToNumber();     //骑兵克制步兵
                    infantryRestrainArcherPct = tb.Get("infantryRestrainArcherPct")->ToNumber();    //步兵克制弓兵
                    archerRestrainMachinePct = tb.Get("archerRestrainMachinePct")->ToNumber();     //弓兵克制器械
                    machineRestrainRiderPct = tb.Get("machineRestrainRiderPct")->ToNumber();      //器械克制骑兵
                }

                reinforceLimit = tb.Get("reinforceLimit")->ToNumber();
                canAttackMonsterLevel = tb.Get("canAttackMonsterLevel")->ToNumber();
                canAttackEliteMonsterLevel = tb.Get("canAttackEliteMonsterLevel")->ToNumber();
                
                attackMonsterDamageBase = tb.Get("attackMonsterDamageBase")->ToNumber();
                attackMonsterDamagePct = tb.Get("attackMonsterDamagePct")->ToNumber();
                attackMonsterDamageExt = tb.Get("attackMonsterDamageExt")->ToNumber();
                attackEliteMonsterDamageBase = tb.Get("attackEliteMonsterDamageBase")->ToNumber();
                attackEliteMonsterDamagePct = tb.Get("attackEliteMonsterDamagePct")->ToNumber();
                attackEliteMonsterDamageExt = tb.Get("attackEliteMonsterDamageExt")->ToNumber();
                attackWorldbossDamageBase = tb.Get("attackWorldbossDamageBase")->ToNumber();
                attackWorldbossDamagePct = tb.Get("attackWorldbossDamagePct")->ToNumber();
                attackWorldbossDamageExt = tb.Get("attackWorldbossDamageExt")->ToNumber();

                if (agent.castle()) {
                    agent.castle()->OnCityDefenseMaxChange(oldCityDefense, cityDefense);
                }
                //Debug();
            }

            void Property::Debug()
            {
                cout << "=============================Property::Debug begin==========================" << endl;
//                         --建筑数据1
//                         --军营
//                cout << "barrackTrainNums = " << barrackTrainNums << endl; //               --军营每次训练士兵数量
//                cout << "barrackTrainSpeedPct = " << barrackTrainSpeedPct << endl; //           --军营训练士兵速度(百分比)
//                cout << "barrackTrainQueues = " << barrackTrainQueues << endl; //             --军营训练队列数量

                cout << "infantryBarrackTrainNums = " << infantryBarrackTrainNums << endl; //               --军营每次训练士兵数量
                cout << "infantryBarrackTrainSpeedPct = " << infantryBarrackTrainSpeedPct << endl; //           --军营训练士兵速度(百分比)
                cout << "infantryBarrackTrainQueues = " << infantryBarrackTrainQueues << endl; //             --军营训练队列数量

                cout << "riderBarrackTrainNums = " << riderBarrackTrainNums << endl; //               --军营每次训练士兵数量
                cout << "riderBarrackTrainSpeedPct = " << riderBarrackTrainSpeedPct << endl; //           --军营训练士兵速度(百分比)
                cout << "riderBarrackTrainQueues = " << riderBarrackTrainQueues << endl; //             --军营训练队列数量

                cout << "archerBarrackTrainNums = " << archerBarrackTrainNums << endl; //               --军营每次训练士兵数量
                cout << "archerBarrackTrainSpeedPct = " << archerBarrackTrainSpeedPct << endl; //           --军营训练士兵速度(百分比)
                cout << "archerBarrackTrainQueues = " << archerBarrackTrainQueues << endl; //             --军营训练队列数量
               
                cout << "mechanicalBarrackTrainNums = " << mechanicalBarrackTrainNums << endl; //               --军营每次训练士兵数量
                cout << "mechanicalBarrackTrainSpeedPct = " << mechanicalBarrackTrainSpeedPct << endl; //           --军营训练士兵速度(百分比)
                cout << "mechanicalBarrackTrainQueues = " << mechanicalBarrackTrainQueues << endl; //             --军营训练队列数量
//                         --城墙
                cout << "trapCapacity = " << trapCapacity << endl; //                   --陷阱容量
                cout << "trapMakeNums = " << trapMakeNums << endl; //                   --陷阱每次制造数量
                cout << "trapTrainSpeedPct = " << trapTrainSpeedPct << endl; //              --陷阱制造速度(百分比)
                cout << "cityDefense = " << cityDefense << endl; //                    --城防值
//                         --箭塔
                cout << "turretAttackForce = " << turretAttackForce << endl; //              --箭塔攻击力
//                         --仓库
                cout << "foodProtectedLimit = " << foodProtectedLimit << endl; //             --粮食保护上限
                cout << "woodProtectedLimit = " << woodProtectedLimit << endl; //             --木材保护上限
                cout << "stoneProtectedLimit = " << stoneProtectedLimit << endl; //            --石料保护上限
                cout << "ironProtectedLimit = " << ironProtectedLimit << endl; //             --铁矿保护上限
//                         --伤兵营
                cout << "woundedCapacity = " << woundedCapacity << endl; //                --伤兵容量

//                         --城内资源建筑
                cout << "foodOutPut = " << foodOutPut << endl; //                     --粮食产出/H
                cout << "foodCapacity = " << foodOutPut << endl; //                   --粮食容量
                cout << "woodOutPut = " << woodOutPut << endl; //                     --木材产出/H
                cout << "woodCapacity = " << woodCapacity << endl; //                   --木材容量
                cout << "stoneOutPut = " << stoneOutPut << endl; //                    --石料产出/H
                cout << "stoneCapacity = " << stoneCapacity << endl; //                  --石料容量
                cout << "ironOutPut = " << ironOutPut << endl; //                     --铁矿产出/H
                cout << "ironCapacity = " << ironCapacity << endl; //                   --铁矿容量
//                         --书院
                cout << "technologyQueues = " << technologyQueues << endl; //               --研究队列数量
//                         --城主府
                cout << "castleTaxSilver = " << castleTaxSilver << endl; //                 --城主府征收收益(银两)

//                         --部队1009~1020
                cout << "troopAttackPct = " << troopAttackPct << endl; //                  --部队攻击百分比加成
                cout << "troopAttackExt = " << troopAttackExt << endl; //                  --部队攻击额外加成
                cout << "troopDefensePct = " << troopDefensePct << endl; //                 --部队防御百分比加成
                cout << "troopDefenseExt = " << troopDefenseExt << endl; //                 --部队防御额外加成
                cout << "troopHpPct = " << troopHpPct << endl; //                      --部队生命百分比加成
                cout << "troopHpExt = " << troopHpExt << endl; //                      --部队生命额外加成
                cout << "troopSpeedPct = " << troopSpeedPct << endl; //                   --部队速度百分比加成
                cout << "troopSpeedExt = " << troopSpeedExt << endl; //                   --部队速度额外加成
                cout << "troopAttackCityPct = " << troopAttackCityPct << endl; //              --部队攻城攻击百分比加成
                cout << "troopAttackCityExt = " << troopAttackCityExt << endl; //              --部队攻城攻击额外加成
                cout << "troopMorale = " << troopMorale << endl; //                     --部队怒气
                cout << "troopLoadPct = " << troopLoadPct << endl; //                    --部队负重百分比加成
                cout << "troopLoadExt = " << troopLoadExt << endl; //                    --部队负重额外加成
                cout << "attackMonsterSpeedPct = " << attackMonsterSpeedPct << endl; //           --打怪行军速度加成(百分比)
                cout << "marchSpeedPct = " << marchSpeedPct << endl; //                   --行军速度加成(百分比)
                cout << "scoutSpeedPct = " << scoutSpeedPct << endl; //                   --侦查速度加成(百分比)
                cout << "robResourcePct = " << robResourcePct << endl; //                  --增加掠夺他人资源上限(百分比)

//                         --2000
//                         --map地图相关
                cout << "foodGatherSpeedPct = " << foodGatherSpeedPct << endl; //             --粮食采集速度加成(百分比)
                cout << "woodGatherSpeedPct = " << woodGatherSpeedPct << endl; //             --木材采集速度加成(百分比)
                cout << "stoneGatherSpeedPct = " << stoneGatherSpeedPct << endl; //            --石料采集速度加成(百分比)
                cout << "ironGatherSpeedPct = " << ironGatherSpeedPct << endl; //             --铁采集速度加成(百分比)
//                         --
                cout << "upkeepReducePct = " << upkeepReducePct << endl; //                --部队粮食消耗减少(百分比)
                cout << "reduceResearchPct = " << reduceResearchPct << endl; //              --减少科技研究消耗(百分比)
//                         --3000
                cout << "buildingSpeedPct = " << buildingSpeedPct << endl; //               --建筑建造速度加成(百分比)
                cout << "healSpeedPct = " << healSpeedPct << endl; //                   --伤兵恢复速度加成(百分比)
                cout << "healReduceResourcePct = " << healReduceResourcePct << endl; //          --医疗所需资源减少(百分比)

                // -- 武将属性相关
                // 5001-5019
                cout << "heroPowerBase = " << heroPowerBase << endl;     
                cout << "heroPowerPct = " << heroPowerPct << endl;     
                cout << "heroPowerExt = " << heroPowerExt << endl;     
                cout << "heroDefenseBase = " << heroDefenseBase << endl;     
                cout << "heroDefensePct = " << heroDefensePct << endl;     
                cout << "heroDefenseExt = " << heroDefenseExt << endl;     
                cout << "heroWisdomBase = " << heroWisdomBase << endl;     
                cout << "heroWisdomPct = " << heroWisdomPct << endl;     
                cout << "heroWisdomExt = " << heroWisdomExt << endl;     
                cout << "heroSkillBase = " << heroSkillBase << endl;     
                cout << "heroSkillPct = " << heroSkillPct << endl;     
                cout << "heroSkillExt = " << heroSkillExt << endl;     
                cout << "heroAgileBase = " << heroAgileBase << endl;     
                cout << "heroAgilePct = " << heroAgilePct << endl;     
                cout << "heroAgileExt = " << heroAgileExt << endl;     
                cout << "heroLuckyBase = " << heroLuckyBase << endl;     
                cout << "heroLuckyPct = " << heroLuckyPct << endl;     
                cout << "heroLuckyExt = " << heroLuckyExt << endl;     
                cout << "heroLifeBase = " << heroLifeBase << endl;     
                cout << "heroLifePct = " << heroLifePct << endl;     
                cout << "heroLifeExt = " << heroLifeExt << endl;     
                cout << "heroPhysicalAttackBase = " << heroPhysicalAttackBase << endl;     
                cout << "heroPhysicalAttackPct = " << heroPhysicalAttackPct << endl;     
                cout << "heroPhysicalAttackExt = " << heroPhysicalAttackExt << endl;     
                cout << "heroPhysicalDefenseBase = " << heroPhysicalDefenseBase << endl;     
                cout << "heroPhysicalDefensePct = " << heroPhysicalDefensePct << endl;     
                cout << "heroPhysicalDefenseExt = " << heroPhysicalDefenseExt << endl;     
                cout << "heroWisdomAttackBase = " << heroWisdomAttackBase << endl;     
                cout << "heroWisdomAttackPct = " << heroWisdomAttackPct << endl;     
                cout << "heroWisdomAttackExt = " << heroWisdomAttackExt << endl;     
                cout << "heroWisdomDefenseBase = " << heroWisdomDefenseBase << endl;     
                cout << "heroWisdomDefensePct = " << heroWisdomDefensePct << endl;     
                cout << "heroWisdomDefenseExt = " << heroWisdomDefenseExt << endl;     
                cout << "heroHitBase = " << heroHitBase << endl;     
                cout << "heroHitPct = " << heroHitPct << endl;     
                cout << "heroHitExt = " << heroHitExt << endl;     
                cout << "heroAvoidBase = " << heroAvoidBase << endl;     
                cout << "heroAvoidPct = " << heroAvoidPct << endl;     
                cout << "heroAvoidExt = " << heroAvoidExt << endl;     
                cout << "heroCritHitBase = " << heroCritHitBase << endl;     
                cout << "heroCritHitPct = " << heroCritHitPct << endl;     
                cout << "heroCritHitExt = " << heroCritHitExt << endl;     
                cout << "heroCritAvoidBase = " << heroCritAvoidBase << endl;     
                cout << "heroCritAvoidPct = " << heroCritAvoidPct << endl;     
                cout << "heroCritAvoidExt = " << heroCritAvoidExt << endl;     
                cout << "heroSpeedBase = " << heroSpeedBase << endl;     
                cout << "heroSpeedPct = " << heroSpeedPct << endl;     
                cout << "heroSpeedExt = " << heroSpeedExt << endl;     
                cout << "heroCityLifeBase = " << heroCityLifeBase << endl;     
                cout << "heroCityLifePct = " << heroCityLifePct << endl;     
                cout << "heroCityLifeExt = " << heroCityLifeExt << endl;     
                cout << "heroFightBase = " << heroFightBase << endl;     
                cout << "heroFightPct = " << heroFightPct << endl;     
                cout << "heroFightExt = " << heroFightExt << endl;     
                cout << "heroSingleEnergyBase = " << heroSingleEnergyBase << endl;     
                cout << "heroSingleEnergyPct = " << heroSingleEnergyPct << endl;     
                cout << "heroSingleEnergyExt = " << heroSingleEnergyExt << endl;

                cout << "=============================Property::Debug end==========================" << endl;
            }

            void Property::armyAttackAddition(model::ArmysType armyType, float& base, float& pct, float& ext) const
            {
                switch (armyType) {
                    case ArmysType::INFANTRY:
                        base = infantryPowerBase;
                        pct = infantryPowerPct;
                        ext = infantryPowerExt;
                    break;
                    case ArmysType::RIDER:
                        base = riderPowerBase;
                        pct = riderPowerPct;
                        ext = riderPowerExt;
                    break;
                    case ArmysType::ARCHER:
                        base = archerPowerBase;
                        pct = archerPowerPct;
                        ext = archerPowerExt;
                    break;
                    case ArmysType::MECHANICAL:
                        base = mechanicalPowerBase;
                        pct = mechanicalPowerPct;
                        ext = mechanicalPowerExt;
                    break;
                    
                    default:
                        break;
                }
            }

            void Property::armyDefenseAddition(model::ArmysType armyType, float& base, float& pct, float& ext) const
            {
                switch (armyType) {
                    case ArmysType::INFANTRY:
                        base = infantry_defenseBase;
                        pct = infantry_defensePct;
                        ext = infantry_defenseExt;
                    break;
                    case ArmysType::RIDER:
                        base = riderDefenseBase;
                        pct = riderDefensePct;
                        ext = riderDefenseExt;
                    break;
                    case ArmysType::ARCHER:
                        base = archerDefenseBase;
                        pct = archerDefensePct;
                        ext = archerDefenseExt;
                    break;
                    case ArmysType::MECHANICAL:
                        base = mechanicalDefenseBase;
                        pct = mechanicalDefensePct;
                        ext = mechanicalDefenseExt;
                    break;
                    
                    default:
                        break;
                }
            }

            float Property::armyHealthAddition(model::ArmyType armyType) const
            {
                float res = .0f;
//                 switch (armyType) {
//                     case ArmyType::INFANTRY:
//                     case ArmyType::PIKEMAN:
//                         res = infantryHealth;
//                         break;
//                     case ArmyType::CAVALRY_RIDER:
//                     case ArmyType::CAVALRY_SHOOTER:
//                         res = cavalryHealth;
//                         break;
//                     case ArmyType::ARCHER:
//                     case ArmyType::CROSSBOWMAN:
//                         res = archerHealth;
//                         break;
//                     case ArmyType::CHARIOT_SHORT:
//                     case ArmyType::CHARIOT_LONG:
//                         res = chariotHealth;
//                         break;
//                     default:
//                         break;
//                 }
                return res;
            }

            float Property::armyDamageAddition(model::ArmyType atkArmyType) const
            {
                float res = .0f;
//                 switch (atkArmyType) {
//                     case ArmyType::ARCHER:
//                         res = archerDamage;
//                         break;
//                     case ArmyType::FIRE_ARROWS:
//                         res = fireArrowDamageToCavalry;
//                         break;
//                     case ArmyType::ROCKFALL:
//                         res = rockfallDamageToInfantry;
//                         break;
//                     case ArmyType::ROLLING_LOGS:
//                         res = rollingLogDamageToArcher;
//                         break;
//                     default:
//                         break;
//                 }
                return res;
            }

            float Property::armyDamagedReduceEx(ArmyType myArmyType, ArmyType enemyArmyType, AttackType myAttackType) const
            {
                float res = .0f;
//                 switch (myArmyType) {
//                     case ArmyType::INFANTRY:
//                     case ArmyType::PIKEMAN:
//                         res = damageToInfantryReduce;
//                         break;
//                     default:
//                         break;
//                 }
//                 switch (enemyArmyType) {
//                     case ArmyType::INFANTRY:
//                     case ArmyType::PIKEMAN:
//                         res += reduceEnemyInfantryDamage;
//                         break;
//                     case ArmyType::CAVALRY_RIDER:
//                     case ArmyType::CAVALRY_SHOOTER:
//                         res += reduceEnemyCavalryDamage;
//                         break;
//                     case ArmyType::ARCHER:
//                     case ArmyType::CROSSBOWMAN:
//                         res += reduceEnemyArcherDamage;
//                         break;
//                     case ArmyType::CHARIOT_SHORT:
//                     case ArmyType::CHARIOT_LONG:
//                         res += reduceEnemyChariotDamage;
//                     default:
//                         break;
//                 }
//                 if (myAttackType == AttackType::DEFENSE) {
//                     res += damageToDefenseReduce;
//                 }
                return res;
            }

            float Property::armyDamageToXReduce(ArmyType myArmyType) const
            {
                float res = .0f;
//                 switch (myArmyType) {
//                     case ArmyType::INFANTRY:
//                     case ArmyType::PIKEMAN:
//                         res = damageToInfantryReduce;
//                         break;
//                     default:
//                         break;
//                 }
                return res;
            }

            float Property::armyReduceEnemyXDamage(ArmyType enemyArmyType) const
            {
                float res = .0f;
//                 switch (enemyArmyType) {
//                     case ArmyType::INFANTRY:
//                     case ArmyType::PIKEMAN:
//                         res += reduceEnemyInfantryDamage;
//                         break;
//                     case ArmyType::CAVALRY_RIDER:
//                     case ArmyType::CAVALRY_SHOOTER:
//                         res += reduceEnemyCavalryDamage;
//                         break;
//                     case ArmyType::ARCHER:
//                     case ArmyType::CROSSBOWMAN:
//                         res += reduceEnemyArcherDamage;
//                         break;
//                     case ArmyType::CHARIOT_SHORT:
//                     case ArmyType::CHARIOT_LONG:
//                         res += reduceEnemyChariotDamage;
//                     default:
//                         break;
//                 }
                return res;
            }

            float Property::armyDamageToXReduce(AttackType myAttackType) const
            {
                float res = .0f;
//                 if (myAttackType == AttackType::DEFENSE) {
//                     res += damageToDefenseReduce;
//                 }
                return res;
            }


            void Property::Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const
            {
                writer.StartObject();
                               
                writer.Key("infantryBarrackTrainNums");
                writer.Int(infantryBarrackTrainNums);

                writer.Key("infantryBarrackTrainSpeedPct");
                writer.Double((double)infantryBarrackTrainSpeedPct);

                writer.Key("infantryBarrackTrainQueues");
                writer.Int(infantryBarrackTrainQueues);
                                
                writer.Key("riderBarrackTrainNums");
                writer.Int(riderBarrackTrainNums);

                writer.Key("riderBarrackTrainSpeedPct");
                writer.Double((double)riderBarrackTrainSpeedPct);

                writer.Key("riderBarrackTrainQueues");
                writer.Int(riderBarrackTrainQueues);
                
                writer.Key("archerBarrackTrainNums");
                writer.Int(archerBarrackTrainNums);

                writer.Key("archerBarrackTrainSpeedPct");
                writer.Double((double)archerBarrackTrainSpeedPct);

                writer.Key("archerBarrackTrainQueues");
                writer.Int(archerBarrackTrainQueues);
                
                writer.Key("mechanicalBarrackTrainNums");
                writer.Int(mechanicalBarrackTrainNums);

                writer.Key("mechanicalBarrackTrainSpeedPct");
                writer.Double((double)mechanicalBarrackTrainSpeedPct);

                writer.Key("mechanicalBarrackTrainQueues");
                writer.Int(mechanicalBarrackTrainQueues);

                writer.Key("trapCapacity");
                writer.Int(trapCapacity);

                writer.Key("trapMakeNums");
                writer.Int(trapMakeNums);

                writer.Key("trapTrainSpeedPct");
                writer.Double((double)trapTrainSpeedPct);

                writer.Key("cityDefense");
                writer.Int(cityDefense);

                writer.Key("cityTroopsDefensePct");
                writer.Double((double)cityTroopsDefensePct);

                writer.Key("turretAttackForce");
                writer.Int(turretAttackForce);

                writer.Key("foodProtectedLimit");
                writer.Int(foodProtectedLimit);

                writer.Key("woodProtectedLimit");
                writer.Int(woodProtectedLimit);

                writer.Key("stoneProtectedLimit");
                writer.Int(stoneProtectedLimit);

                writer.Key("ironProtectedLimit");
                writer.Int(ironProtectedLimit);

                writer.Key("woundedCapacity");
                writer.Int(woundedCapacity);

                writer.Key("foodOutPut");
                writer.Int(foodOutPut);

                writer.Key("foodCapacity");
                writer.Int(foodCapacity);

                writer.Key("woodOutPut");
                writer.Int(woodOutPut);

                writer.Key("woodCapacity");
                writer.Int(woodCapacity);

                writer.Key("stoneOutPut");
                writer.Int(stoneOutPut);

                writer.Key("stoneCapacity");
                writer.Int(stoneCapacity);

                writer.Key("ironOutPut");
                writer.Int(ironOutPut);

                writer.Key("ironCapacity");
                writer.Int(ironCapacity);

                writer.Key("technologyQueues");
                writer.Int(technologyQueues);

                writer.Key("castleTaxSilver");
                writer.Int(castleTaxSilver);

                writer.Key("troopAttackPct");
                writer.Double((double)troopAttackPct);

                writer.Key("troopAttackExt");
                writer.Double((double)troopAttackExt);

                writer.Key("troopDefensePct");
                writer.Double((double)troopDefensePct);

                writer.Key("troopDefenseExt");
                writer.Double((double)troopDefenseExt);

                writer.Key("troopHpPct");
                writer.Double((double)troopHpPct);

                writer.Key("troopHpExt");
                writer.Double((double)troopHpExt);

                writer.Key("troopSpeedPct");
                writer.Double((double)troopSpeedPct);

                writer.Key("troopSpeedExt");
                writer.Double((double)troopSpeedExt);

                writer.Key("troopAttackCityPct");
                writer.Double((double)troopAttackCityPct);

                writer.Key("troopAttackCityExt");
                writer.Double((double)troopAttackCityExt);

                writer.Key("troopMorale");
                writer.Double((double)troopMorale);

                writer.Key("troopLoadPct");
                writer.Double((double)troopLoadPct);

                writer.Key("troopLoadExt");
                writer.Double((double)troopLoadExt);

                writer.Key("attackMonsterSpeedPct");
                writer.Double((double)attackMonsterSpeedPct);

                writer.Key("marchSpeedPct");
                writer.Double((double)marchSpeedPct);

                writer.Key("scoutSpeedPct");
                writer.Double((double)scoutSpeedPct);

                writer.Key("robResourcePct");
                writer.Double((double)robResourcePct);

                writer.Key("camptempSpeedPct");
                writer.Double((double)camptempSpeedPct);
                writer.Key("gatherSpeedPct");
                writer.Double((double)gatherSpeedPct);
                writer.Key("attackPlayerSpeedPct");
                writer.Double((double)attackPlayerSpeedPct);
                writer.Key("reinforcementsSpeedPct");
                writer.Double((double)reinforcementsSpeedPct);

                //2000+
                writer.Key("foodGatherSpeedPct");
                writer.Double((double)foodGatherSpeedPct);

                writer.Key("woodGatherSpeedPct");
                writer.Double((double)woodGatherSpeedPct);

                writer.Key("stoneGatherSpeedPct");
                writer.Double((double)stoneGatherSpeedPct);

                writer.Key("ironGatherSpeedPct");
                writer.Double((double)ironGatherSpeedPct);

                writer.Key("upkeepReducePct");
                writer.Double((double)upkeepReducePct);

                writer.Key("reduceResearchPct");
                writer.Double((double)reduceResearchPct);

                //3000+
                writer.Key("buildingSpeedPct");
                writer.Double((double)buildingSpeedPct);

                writer.Key("healSpeedPct");
                writer.Int(healSpeedPct);

                writer.Key("healReduceResourcePct");
                writer.Double((double)healReduceResourcePct);

                writer.Key("maxMarchingQueue");
                writer.Int(maxMarchingQueue);

                writer.Key("maxHeroNum");
                writer.Int(maxHeroNum);

                writer.Key("allianceReinforcementNum");
                writer.Int(allianceReinforcementNum);

                writer.Key("transportSpeedUpPct");
                writer.Double((double)transportSpeedUpPct);

                //5000+
                //武将属性类
                writer.Key("heroPowerBase");
                writer.Double((double)heroPowerBase);
                writer.Key("heroPowerPct");
                writer.Double((double)heroPowerPct);
                writer.Key("heroPowerExt");
                writer.Double((double)heroPowerExt);
                writer.Key("heroDefenseBase");
                writer.Double((double)heroDefenseBase);
                writer.Key("heroDefensePct");
                writer.Double((double)heroDefensePct);
                writer.Key("heroDefenseExt");
                writer.Double((double)heroDefenseExt);
                writer.Key("heroWisdomBase");
                writer.Double((double)heroWisdomBase);
                writer.Key("heroWisdomPct");
                writer.Double((double)heroWisdomPct);
                writer.Key("heroWisdomExt");
                writer.Double((double)heroWisdomExt);
                writer.Key("heroSkillBase");
                writer.Double((double)heroSkillBase);
                writer.Key("heroSkillPct");
                writer.Double((double)heroSkillPct);
                writer.Key("heroSkillExt");
                writer.Double((double)heroSkillExt);
                writer.Key("heroAgileBase");
                writer.Double((double)heroAgileBase);
                writer.Key("heroAgilePct");
                writer.Double((double)heroAgilePct);
                writer.Key("heroAgileExt");
                writer.Double((double)heroAgileExt);
                writer.Key("heroLuckyBase");
                writer.Double((double)heroLuckyBase);
                writer.Key("heroLuckyPct");
                writer.Double((double)heroLuckyPct);
                writer.Key("heroLuckyExt");
                writer.Double((double)heroLuckyExt);
                writer.Key("heroLifeBase");
                writer.Double((double)heroLifeBase);
                writer.Key("heroLifePct");
                writer.Double((double)heroLifePct);
                writer.Key("heroLifeExt");
                writer.Double((double)heroLifeExt);
                writer.Key("heroPhysicalAttackBase");
                writer.Double((double)heroPhysicalAttackBase);
                writer.Key("heroPhysicalAttackPct");
                writer.Double((double)heroPhysicalAttackPct);
                writer.Key("heroPhysicalAttackExt");
                writer.Double((double)heroPhysicalAttackExt);
                writer.Key("heroPhysicalDefenseBase");
                writer.Double((double)heroPhysicalDefenseBase);
                writer.Key("heroPhysicalDefensePct");
                writer.Double((double)heroPhysicalDefensePct);
                writer.Key("heroPhysicalDefenseExt");
                writer.Double((double)heroPhysicalDefenseExt);
                writer.Key("heroWisdomAttackBase");
                writer.Double((double)heroWisdomAttackBase);
                writer.Key("heroWisdomAttackPct");
                writer.Double((double)heroWisdomAttackPct);
                writer.Key("heroWisdomAttackExt");
                writer.Double((double)heroWisdomAttackExt);
                writer.Key("heroWisdomDefenseBase");
                writer.Double((double)heroWisdomDefenseBase);
                writer.Key("heroWisdomDefensePct");
                writer.Double((double)heroWisdomDefensePct);
                writer.Key("heroWisdomDefenseExt");
                writer.Double((double)heroWisdomDefenseExt);
                writer.Key("heroHitBase");
                writer.Double((double)heroHitBase);
                writer.Key("heroHitPct");
                writer.Double((double)heroHitPct);
                writer.Key("heroHitExt");
                writer.Double((double)heroHitExt);
                writer.Key("heroAvoidBase");
                writer.Double((double)heroAvoidBase);
                writer.Key("heroAvoidPct");
                writer.Double((double)heroAvoidPct);
                writer.Key("heroAvoidExt");
                writer.Double((double)heroAvoidExt);
                writer.Key("heroCritHitBase");
                writer.Double((double)heroCritHitBase);
                writer.Key("heroCritHitPct");
                writer.Double((double)heroCritHitPct);
                writer.Key("heroCritHitExt");
                writer.Double((double)heroCritHitExt);
                writer.Key("heroCritAvoidBase");
                writer.Double((double)heroCritAvoidBase);
                writer.Key("heroCritAvoidPct");
                writer.Double((double)heroCritAvoidPct);
                writer.Key("heroCritAvoidExt");
                writer.Double((double)heroCritAvoidExt);
                writer.Key("heroSpeedBase");
                writer.Double((double)heroSpeedBase);
                writer.Key("heroSpeedPct");
                writer.Double((double)heroSpeedPct);
                writer.Key("heroSpeedExt");
                writer.Double((double)heroSpeedExt);
                writer.Key("heroCityLifeBase");
                writer.Double((double)heroCityLifeBase);
                writer.Key("heroCityLifePct");
                writer.Double((double)heroCityLifePct);
                writer.Key("heroCityLifeExt");
                writer.Double((double)heroCityLifeExt);
                writer.Key("heroFightBase");
                writer.Double((double)heroFightBase);
                writer.Key("heroFightPct");
                writer.Double((double)heroFightPct);
                writer.Key("heroFightExt");
                writer.Double((double)heroFightExt);
                writer.Key("heroSingleEnergyBase");
                writer.Double((double)heroSingleEnergyBase);
                writer.Key("heroSingleEnergyPct");
                writer.Double((double)heroSingleEnergyPct);
                writer.Key("heroSingleEnergyExt");
                writer.Double((double)heroSingleEnergyExt);

                writer.Key("riderPowerBase");
                writer.Double((double)riderPowerBase);
                writer.Key("riderPowerPct");
                writer.Double((double)riderPowerPct);
                writer.Key("riderPowerExt");
                writer.Double((double)riderPowerExt);
                writer.Key("infantryPowerBase");
                writer.Double((double)infantryPowerBase);
                writer.Key("infantryPowerPct");
                writer.Double((double)infantryPowerPct);
                writer.Key("infantryPowerExt");
                writer.Double((double)infantryPowerExt);
                writer.Key("archerPowerBase");
                writer.Double((double)archerPowerBase);
                writer.Key("archerPowerPct");
                writer.Double((double)archerPowerPct);
                writer.Key("archerPowerExt");
                writer.Double((double)archerPowerExt);
                writer.Key("mechanicalPowerBase");
                writer.Double((double)mechanicalPowerBase);
                writer.Key("mechanicalPowerPct");
                writer.Double((double)mechanicalPowerPct);
                writer.Key("mechanicalPowerExt");
                writer.Double((double)mechanicalPowerExt);
                writer.Key("riderDefenseBase");
                writer.Double((double)riderDefenseBase);
                writer.Key("riderDefensePct");
                writer.Double((double)riderDefensePct);
                writer.Key("riderDefenseExt");
                writer.Double((double)riderDefenseExt);
                writer.Key("infantry_defenseBase");
                writer.Double((double)infantry_defenseBase);
                writer.Key("infantry_defensePct");
                writer.Double((double)infantry_defensePct);
                writer.Key("infantry_defenseExt");
                writer.Double((double)infantry_defenseExt);
                writer.Key("archerDefenseBase");
                writer.Double((double)archerDefenseBase);
                writer.Key("archerDefensePct");
                writer.Double((double)archerDefensePct);
                writer.Key("archerDefenseExt");
                writer.Double((double)archerDefenseExt);
                writer.Key("mechanicalDefenseBase");
                writer.Double((double)mechanicalDefenseBase);
                writer.Key("mechanicalDefensePct");
                writer.Double((double)mechanicalDefensePct);
                writer.Key("mechanicalDefenseExt");
                writer.Double((double)mechanicalDefenseExt);

                writer.Key("reinforceLimit");
                writer.Int(reinforceLimit);

                writer.EndObject();
            }

                
            bool Property::Deserialize(rapidjson::Document& doc)
            {
                auto& temp = doc["properties"];
                infantryBarrackTrainNums = temp["infantryBarrackTrainNums"].GetInt();
                infantryBarrackTrainSpeedPct = temp["infantryBarrackTrainSpeedPct"].GetDouble();
                infantryBarrackTrainQueues = temp["infantryBarrackTrainQueues"].GetInt();
                riderBarrackTrainNums = temp["riderBarrackTrainNums"].GetInt();
                riderBarrackTrainSpeedPct = temp["riderBarrackTrainSpeedPct"].GetDouble();
                riderBarrackTrainQueues = temp["riderBarrackTrainQueues"].GetInt();
                archerBarrackTrainNums = temp["archerBarrackTrainNums"].GetInt();
                archerBarrackTrainSpeedPct = temp["archerBarrackTrainSpeedPct"].GetDouble();
                archerBarrackTrainQueues = temp["archerBarrackTrainQueues"].GetInt();
                mechanicalBarrackTrainNums = temp["mechanicalBarrackTrainNums"].GetInt();
                mechanicalBarrackTrainSpeedPct = temp["mechanicalBarrackTrainSpeedPct"].GetDouble();
                mechanicalBarrackTrainQueues = temp["mechanicalBarrackTrainQueues"].GetInt();
                trapCapacity = temp["trapCapacity"].GetInt();
                trapMakeNums = temp["trapMakeNums"].GetInt();
                trapTrainSpeedPct = temp["trapTrainSpeedPct"].GetDouble();
                cityDefense = temp["cityDefense"].GetInt();
                cityTroopsDefensePct = temp["cityTroopsDefensePct"].GetDouble();
                turretAttackForce = temp["turretAttackForce"].GetInt();
                foodProtectedLimit = temp["foodProtectedLimit"].GetInt();
                woodProtectedLimit = temp["woodProtectedLimit"].GetInt();
                stoneProtectedLimit = temp["stoneProtectedLimit"].GetInt();
                ironProtectedLimit = temp["ironProtectedLimit"].GetInt();
                woundedCapacity = temp["woundedCapacity"].GetInt();
                foodOutPut = temp["foodOutPut"].GetInt();
                foodCapacity = temp["foodCapacity"].GetInt();
                woodOutPut = temp["woodOutPut"].GetInt();
                woodCapacity = temp["woodCapacity"].GetInt();
                stoneOutPut = temp["stoneOutPut"].GetInt();
                stoneCapacity = temp["stoneCapacity"].GetInt();
                ironOutPut = temp["ironOutPut"].GetInt();
                ironCapacity = temp["ironCapacity"].GetInt();
                technologyQueues = temp["technologyQueues"].GetInt();
                castleTaxSilver = temp["castleTaxSilver"].GetInt();
                troopAttackPct = temp["troopAttackPct"].GetDouble();
                troopAttackExt = temp["troopAttackExt"].GetDouble();
                troopDefensePct = temp["troopDefensePct"].GetDouble();
                troopDefenseExt = temp["troopDefenseExt"].GetDouble();
                troopHpPct = temp["troopHpPct"].GetDouble();
                troopHpExt = temp["troopHpExt"].GetDouble();
                troopSpeedPct = temp["troopSpeedPct"].GetDouble();
                troopSpeedExt = temp["troopSpeedExt"].GetDouble();
                troopAttackCityPct = temp["troopAttackCityPct"].GetDouble();
                troopAttackCityExt = temp["troopAttackCityExt"].GetDouble();
                troopMorale = temp["troopMorale"].GetDouble();
                troopLoadPct = temp["troopLoadPct"].GetDouble();
                troopLoadExt = temp["troopLoadExt"].GetDouble();
                attackMonsterSpeedPct = temp["attackMonsterSpeedPct"].GetDouble();
                marchSpeedPct = temp["marchSpeedPct"].GetDouble();
                scoutSpeedPct = temp["scoutSpeedPct"].GetDouble();
                robResourcePct = temp["robResourcePct"].GetDouble();
                camptempSpeedPct = temp["camptempSpeedPct"].GetDouble();
                gatherSpeedPct = temp["gatherSpeedPct"].GetDouble();
                attackPlayerSpeedPct = temp["attackPlayerSpeedPct"].GetDouble();
                reinforcementsSpeedPct = temp["reinforcementsSpeedPct"].GetDouble();
                foodGatherSpeedPct = temp["foodGatherSpeedPct"].GetDouble();
                woodGatherSpeedPct = temp["woodGatherSpeedPct"].GetDouble();
                stoneGatherSpeedPct = temp["stoneGatherSpeedPct"].GetDouble();
                ironGatherSpeedPct = temp["ironGatherSpeedPct"].GetDouble();
                upkeepReducePct = temp["upkeepReducePct"].GetDouble();
                reduceResearchPct = temp["reduceResearchPct"].GetDouble();
                buildingSpeedPct = temp["buildingSpeedPct"].GetDouble();
                healSpeedPct = temp["healSpeedPct"].GetInt();
                healReduceResourcePct = temp["healReduceResourcePct"].GetDouble();
                maxMarchingQueue = temp["maxMarchingQueue"].GetInt();
                maxHeroNum = temp["maxHeroNum"].GetInt();
                allianceReinforcementNum = temp["allianceReinforcementNum"].GetInt();
                transportSpeedUpPct = temp["transportSpeedUpPct"].GetDouble();
                heroPowerBase = temp["heroPowerBase"].GetDouble();
                heroPowerPct = temp["heroPowerPct"].GetDouble();
                heroPowerExt = temp["heroPowerExt"].GetDouble();
                heroDefenseBase = temp["heroDefenseBase"].GetDouble();
                heroDefensePct = temp["heroDefensePct"].GetDouble();
                heroDefenseExt = temp["heroDefenseExt"].GetDouble();
                heroWisdomBase = temp["heroWisdomBase"].GetDouble();
                heroWisdomPct = temp["heroWisdomPct"].GetDouble();
                heroWisdomExt = temp["heroWisdomExt"].GetDouble();
                heroSkillBase = temp["heroSkillBase"].GetDouble();
                heroSkillPct = temp["heroSkillPct"].GetDouble();
                heroSkillExt = temp["heroSkillExt"].GetDouble();
                heroAgileBase = temp["heroAgileBase"].GetDouble();
                heroAgilePct = temp["heroAgilePct"].GetDouble();
                heroAgileExt = temp["heroAgileExt"].GetDouble();
                heroLuckyBase = temp["heroLuckyBase"].GetDouble();
                heroLuckyPct = temp["heroLuckyPct"].GetDouble();
                heroLuckyExt = temp["heroLuckyExt"].GetDouble();
                heroLifeBase = temp["heroLifeBase"].GetDouble();
                heroLifePct = temp["heroLifePct"].GetDouble();
                heroLifeExt = temp["heroLifeExt"].GetDouble();
                heroPhysicalAttackBase = temp["heroPhysicalAttackBase"].GetDouble();
                heroPhysicalAttackPct = temp["heroPhysicalAttackPct"].GetDouble();
                heroPhysicalAttackExt = temp["heroPhysicalAttackExt"].GetDouble();
                heroPhysicalDefenseBase = temp["heroPhysicalDefenseBase"].GetDouble();
                heroPhysicalDefensePct = temp["heroPhysicalDefensePct"].GetDouble();
                heroPhysicalDefenseExt = temp["heroPhysicalDefenseExt"].GetDouble();
                heroWisdomAttackBase = temp["heroWisdomAttackBase"].GetDouble();
                heroWisdomAttackPct = temp["heroWisdomAttackPct"].GetDouble();
                heroWisdomAttackExt = temp["heroWisdomAttackExt"].GetDouble();
                heroWisdomDefenseBase = temp["heroWisdomDefenseBase"].GetDouble();
                heroWisdomDefensePct = temp["heroWisdomDefensePct"].GetDouble();
                heroWisdomDefenseExt = temp["heroWisdomDefenseExt"].GetDouble();
                heroHitBase = temp["heroHitBase"].GetDouble();
                heroHitPct = temp["heroHitPct"].GetDouble();
                heroHitExt = temp["heroHitExt"].GetDouble();
                heroAvoidBase = temp["heroAvoidBase"].GetDouble();
                heroAvoidPct = temp["heroAvoidPct"].GetDouble();
                heroAvoidExt = temp["heroAvoidExt"].GetDouble();
                heroCritHitBase = temp["heroCritHitBase"].GetDouble();
                heroCritHitPct = temp["heroCritHitPct"].GetDouble();
                heroCritHitExt = temp["heroCritHitExt"].GetDouble();
                heroCritAvoidBase = temp["heroCritAvoidBase"].GetDouble();
                heroCritAvoidPct = temp["heroCritAvoidPct"].GetDouble();
                heroCritAvoidExt = temp["heroCritAvoidExt"].GetDouble();
                heroSpeedBase = temp["heroSpeedBase"].GetDouble();
                heroSpeedPct = temp["heroSpeedPct"].GetDouble();
                heroSpeedExt = temp["heroSpeedExt"].GetDouble();
                heroCityLifeBase = temp["heroCityLifeBase"].GetDouble();
                heroCityLifePct = temp["heroCityLifePct"].GetDouble();
                heroCityLifeExt = temp["heroCityLifeExt"].GetDouble();
                heroFightBase = temp["heroFightBase"].GetDouble();
                heroFightPct = temp["heroFightPct"].GetDouble();
                heroFightExt = temp["heroFightExt"].GetDouble();
                heroSingleEnergyBase = temp["heroSingleEnergyBase"].GetDouble();
                heroSingleEnergyPct = temp["heroSingleEnergyPct"].GetDouble();
                heroSingleEnergyExt = temp["heroSingleEnergyExt"].GetDouble();
                riderPowerBase = temp["riderPowerBase"].GetDouble();
                riderPowerPct = temp["riderPowerPct"].GetDouble();
                riderPowerExt = temp["riderPowerExt"].GetDouble();
                infantryPowerBase = temp["infantryPowerBase"].GetDouble();
                infantryPowerPct = temp["infantryPowerPct"].GetDouble();
                infantryPowerExt = temp["infantryPowerExt"].GetDouble();
                archerPowerBase = temp["archerPowerBase"].GetDouble();
                archerPowerPct = temp["archerPowerPct"].GetDouble();
                archerPowerExt = temp["archerPowerExt"].GetDouble();
                mechanicalPowerBase = temp["mechanicalPowerBase"].GetDouble();
                mechanicalPowerPct = temp["mechanicalPowerPct"].GetDouble();
                mechanicalPowerExt = temp["mechanicalPowerExt"].GetDouble();
                riderDefenseBase = temp["riderDefenseBase"].GetDouble();
                riderDefensePct = temp["riderDefensePct"].GetDouble();
                riderDefenseExt = temp["riderDefenseExt"].GetDouble();
                infantry_defenseBase = temp["infantry_defenseBase"].GetDouble();
                infantry_defensePct = temp["infantry_defensePct"].GetDouble();
                infantry_defenseExt = temp["infantry_defenseExt"].GetDouble();
                archerDefenseBase = temp["archerDefenseBase"].GetDouble();
                archerDefensePct = temp["archerDefensePct"].GetDouble();
                archerDefenseExt = temp["archerDefenseExt"].GetDouble();
                mechanicalDefenseBase = temp["mechanicalDefenseBase"].GetDouble();
                mechanicalDefensePct = temp["mechanicalDefensePct"].GetDouble();
                mechanicalDefenseExt = temp["mechanicalDefenseExt"].GetDouble();
                reinforceLimit = temp["reinforceLimit"].GetInt();
            }

        }
    }
}
