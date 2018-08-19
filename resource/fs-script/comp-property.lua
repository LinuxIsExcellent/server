local agent = ...
local p = require('protocol')
local t = require('tploader')
local utils = require('utils')
local timer = require('timer')
local event = require('libs/event')
local arenaStub = require('stub/arena')
local allianceStub = require('stub/alliance')

local user = agent.user
local building = agent.building
local technology = agent.technology
local hero = agent.hero
local buff = agent.buff
local vip = agent.vip
local palaceWar = agent.palaceWar
local alliance = agent.alliance

local propertyInfo = {
    base = 0,
    pct = 0,
    ext = 0,
}

local list = {
    --建筑数据1
    --军营

    infantryBarrackTrainNums = 0,             --步兵营每次训练士兵数量
    infantryBarrackTrainSpeedPct = 0,         --步兵营训练士兵速度(百分比)
    infantryBarrackTrainQueues = 0,           --步兵营训练队列数量    
    riderBarrackTrainNums = 0,               --骑兵营每次训练士兵数量
    riderBarrackTrainSpeedPct = 0,           --骑兵营训练士兵速度(百分比)
    riderBarrackTrainQueues = 0,             --骑兵营训练队列数量
    archerBarrackTrainNums = 0,               --弓兵每次训练士兵数量
    archerBarrackTrainSpeedPct = 0,           --弓兵训练士兵速度(百分比)
    archerBarrackTrainQueues = 0,             --弓兵训练队列数量
    mechanicalBarrackTrainNums = 0,           --器械营每次训练士兵数量
    mechanicalBarrackTrainSpeedPct = 0,       --器械营训练士兵速度(百分比)
    mechanicalBarrackTrainQueues = 0,         --器械营训练队列数量

    --城墙
    trapCapacity = 0,                   --陷阱容量
    trapMakeNums = 0,                   --陷阱每次制造数量
    trapTrainSpeedPct = 0,              --陷阱制造速度(百分比)
    trapAttackPct = 0,                  --陷阱攻击加成(百分比)
    cityDefense = 0,                    --城防值(最大值)
    cityTroopsDefensePct = 0,           --(武将和部队)守军防御加成(百分比)
    --箭塔
    turretAttackForce = 0,              --箭塔攻击力
    --仓库
    foodProtectedLimit = 0,             --粮食保护上限
    woodProtectedLimit = 0,             --木材保护上限
    stoneProtectedLimit = 0,            --石料保护上限
    ironProtectedLimit = 0,             --铁矿保护上限
    --伤兵营
    woundedCapacity = 0,                --伤兵容量

    --城内资源建筑
    foodOutPut = 0,                     --粮食产出/H
    foodCapacity = 0,                   --粮食容量
    woodOutPut = 0,                     --木材产出/H
    woodCapacity = 0,                   --木材容量
    stoneOutPut = 0,                    --石料产出/H
    stoneCapacity = 0,                  --石料容量
    ironOutPut = 0,                     --铁矿产出/H
    ironCapacity = 0,                   --铁矿容量
    --书院
    technologyQueues = 0,               --研究队列数量
    --城主府
    castleTaxSilver = 0,                 --城主府征收收益(银两)
   
    --部队1009~1020
    troopAttackPct = 0,                  --部队攻击百分比加成
    troopAttackExt = 0,                  --部队攻击额外加成
    troopDefensePct = 0,                 --部队防御百分比加成
    troopDefenseExt = 0,                 --部队防御额外加成
    troopHpPct = 0,                      --部队生命百分比加成
    troopHpExt = 0,                      --部队生命额外加成
    troopSpeedPct = 0,                   --部队速度百分比加成
    troopSpeedExt = 0,                   --部队速度额外加成
    troopAttackCityPct = 0,              --部队攻城攻击百分比加成
    troopAttackCityExt = 0,              --部队攻城攻击额外加成
    troopMorale = 0,                     --部队怒气
    troopLoadPct = 0,                    --部队负重百分比加成
    troopLoadExt = 0,                    --部队负重额外加成
    attackMonsterSpeedPct = 0,           --打怪行军速度加成(百分比)
    marchSpeedPct = 0,                   --行军速度加成(百分比)
    scoutSpeedPct = 0,                   --侦查速度加成(百分比)
    robResourcePct = 0,                  --增加掠夺他人资源上限(百分比)

    camptempSpeedPct = 0,               --驻扎行军速度(百分比)
    gatherSpeedPct = 0,                 --采集速度(百分比)
    attackPlayerSpeedPct = 0,           --攻击玩家行军速度(百分比)
    reinforcementsSpeedPct = 0,         --援军行军速度(百分比)

    --2000
    --map地图相关
    foodGatherSpeedPct = 0,             --粮食采集速度加成(百分比)
    woodGatherSpeedPct = 0,             --木材采集速度加成(百分比)
    stoneGatherSpeedPct = 0,            --石料采集速度加成(百分比)
    ironGatherSpeedPct = 0,             --铁采集速度加成(百分比)
    --
    upkeepReducePct = 0,                --部队粮食消耗减少(百分比)
    reduceResearchPct = 0,              --减少科技研究消耗(百分比)
    --3000
    buildingSpeedPct = 0,               --建筑建造速度加成(百分比)
    healSpeedPct = 0,                   --伤兵恢复速度加成(百分比)
    healReduceResourcePct = 0,          --医疗所需资源减少(百分比)

    --tpl_configure初始数据
    maxMarchingQueue = 0,               --出征队伍数量上限
    maxHeroNum = 0,                     --队伍可配将数量上限
    helpMax = 0,                        --联盟帮助次数上限
    helpReduceTime = 0,                 --联盟帮助减少的时间
    healNum = 0,                        --治疗数量
    healQueueNum = 0,                   --治疗队列数量
    allianceReinforcementNum = 0,       --同盟增援部队数量
    drillGroundsSoldierNumMax = 0,     --校场士兵数量上限
    -- 市场
    transportNum = 0,                    --市场部队运输数量
    transportTaxPct = 0,                    --市场运输税率
    transportSpeedUp = 0,                 -- 运输加速(百分比)

    --4000
    trapAttackBase = 0,                 --陷阱攻击力基础值
    trapAttackPct = 0,                  --陷阱攻击力百分比
    trapAttackExt = 0,                  --陷阱攻击力额外加成

    --5001-5019
    --武将属性
    --一级属性
    heroPowerBase = 0,                  --武将武力基础加成                                
    heroPowerPct = 0,                   --武将武力百分比加成                 
    heroPowerExt = 0,                   --武将武力额外加成                      
    heroDefenseBase = 0,                --武将统帅基础加成    
    heroDefensePct = 0,                 --武将统帅百分比加成    
    heroDefenseExt = 0,                 --武将统帅额外加成    
    heroWisdomBase = 0,                 --武将智力基础加成
    heroWisdomPct = 0,                  --武将智力百分比加成  
    heroWisdomExt = 0,                  --武将智力额外加成  
    heroSkillBase = 0,                  --武将士气基础加成  
    heroSkillPct = 0,                   --武将士气百分比加成 
    heroSkillExt = 0,                   --武将士气额外加成  
    heroAgileBase = 0,                  --武将速度基础加成  
    heroAgilePct = 0,                   --武将速度百分比加成  
    heroAgileExt = 0,                   --武将速度额外加成  
    heroLuckyBase = 0,                  --武将运气基础加成   
    heroLuckyPct = 0,                   --武将运气百分比加成  
    heroLuckyExt = 0,                   --武将运气额外加成  
    heroLifeBase = 0,                   --武将攻城基础加成  
    heroLifePct = 0,                    --武将攻城百分比加成
    heroLifeExt = 0,                    --武将攻城额外加成

    --二级属性
    heroPhysicalAttackBase = 0,         --武将物理攻击力基础加成  
    heroPhysicalAttackPct = 0,          --武将物理攻击力百分比加成    
    heroPhysicalAttackExt = 0,          --武将物理攻击力额外加成     
    heroPhysicalDefenseBase = 0,        --武将物理防御力基础加成      
    heroPhysicalDefensePct = 0,         --武将物理防御力百分比加成      
    heroPhysicalDefenseExt = 0,         --武将物理防御力额外加成      
    heroWisdomAttackBase = 0,          --武将谋略攻击力基础加成      
    heroWisdomAttackPct = 0,           --武将谋略攻击力百分比加成      
    heroWisdomAttackExt = 0,           --武将谋略攻击力额外加成      
    heroWisdomDefenseBase = 0,          --武将谋略防御力基础加成  
    heroWisdomDefensePct = 0,           --武将谋略防御力百分比加成      
    heroWisdomDefenseExt = 0,           --武将谋略防御力额外加成  
    heroHitBase = 0,                    --武将命中值基础加成  
    heroHitPct = 0,                     --武将命中值百分比加成      
    heroHitExt = 0,                     --武将命中值额外加成  
    heroAvoidBase = 0,                  --武将回避值基础加成      
    heroAvoidPct = 0,                   --武将回避值百分比加成      
    heroAvoidExt = 0,                   --武将回避值额外加成      
    heroCritHitBase = 0,                --武将暴击命中值基础加成              
    heroCritHitPct = 0,                 --武将暴击命中值百分比加成              
    heroCritHitExt = 0,                 --武将暴击命中值额外加成              
    heroCritAvoidBase = 0,              --武将暴击回避值基础加成              
    heroCritAvoidPct = 0,               --武将暴击回避值百分比加成              
    heroCritAvoidExt = 0,               --武将暴击回避值额外加成              
    heroSpeedBase = 0,                  --武将攻击速度基础加成              
    heroSpeedPct = 0,                   --武将攻击速度百分比加成              
    heroSpeedExt = 0,                   --武将攻击速度额外加成          
    heroCityLifeBase = 0,               --武将攻城值基础加成          
    heroCityLifePct = 0,                --武将攻城值百分比加成              
    heroCityLifeExt = 0,                --武将攻城值额外加成                  
    heroFightBase = 0,                  --武将兵力上限基础加成                      
    heroFightPct = 0,                   --武将兵力上限百分比加成                      
    heroFightExt = 0,                   --武将兵力上限额外加成                      
    heroSingleEnergyBase = 0,           --武将单挑血量基础加成                              
    heroSingleEnergyPct = 0,            --武将单挑血量百分比加成                  
    heroSingleEnergyExt = 0,            --武将单挑血量额外加成

    riderPowerBase = 0,                --骑兵攻击力基础加成
    riderPowerPct = 0,                --骑兵攻击力百分比加成
    riderPowerExt = 0,                --骑兵攻击力额外加成
    infantryPowerBase = 0,             --步兵攻击力基础加成
    infantryPowerPct = 0,             --步兵攻击力百分比加成
    infantryPowerExt = 0,             --步兵攻击力额外加成
    archerPowerBase = 0,               --弓兵攻击力基础加成
    archerPowerPct = 0,               --弓兵攻击力百分比加成
    archerPowerExt = 0,               --弓兵攻击力额外加成
    mechanicalPowerBase = 0,           --器械攻击力基础加成
    mechanicalPowerPct = 0,           --器械攻击力百分比加成
    mechanicalPowerExt = 0,           --器械攻击力额外加成
    riderDefenseBase = 0,              --骑兵防御力基础加成
    riderDefensePct = 0,              --骑兵防御力百分比加成
    riderDefenseExt = 0,              --骑兵防御力额外加成
    infantry_defenseBase = 0,          --步兵防御力基础加成
    infantry_defensePct = 0,          --步兵防御力百分比加成
    infantry_defenseExt = 0,          --步兵防御力额外加成
    archerDefenseBase = 0,            --弓兵防御力基础加成
    archerDefensePct = 0,            --弓兵防御力百分比加成
    archerDefenseExt = 0,            --弓兵防御力额外加成
    mechanicalDefenseBase = 0,        --器械防御力基础加成
    mechanicalDefensePct = 0,        --器械防御力百分比加成
    mechanicalDefenseExt = 0,        --器械防御力额外加成
    reinforceLimit = 0,              --援兵容纳队列上限

    reduceRiderTrainConsumePct = 0,      --骑兵训练消耗降低
    reduceInfantryTrainConsumePct = 0,   --步兵训练消耗降低
    reduceArcherTrainConsumePct = 0,     --弓兵训练消耗降低
    reduceMechanicalTrainConsumePct = 0,     --器械消耗降低

    canAttackMonsterLevel = 0,        --可攻击野怪等级
    canAttackEliteMonsterLevel = 0,     --可攻击精英野怪等级

    attackMonsterDamageBase = 0,           --攻击野怪伤害基础加成
    attackMonsterDamagePct = 0,           --攻击野怪伤害百分比加成
    attackMonsterDamageExt = 0,           --攻击野怪伤害额外加成
    attackEliteMonsterDamageBase = 0,       --攻击精英野怪伤害基础加成
    attackEliteMonsterDamagePct = 0,       --攻击精英野怪伤害百分比加成
    attackEliteMonsterDamageExt = 0,       --攻击精英野怪伤害额外加成
    attackWorldbossDamageBase = 0,         --攻击BOSS伤害基础加成
    attackWorldbossDamagePct = 0,         --攻击BOSS伤害百分比加成
    attackWorldbossDamageExt = 0,         --攻击BOSS伤害额外加成

    --各种陷阱加成
    trapHorseDamageBase = 0,
    trapHorseDamagePct = 0,
    trapHorseDamageExt = 0,
    trapStoneDamageBase = 0,
    trapStoneDamagePct = 0,
    trapStoneDamageExt = 0,
    trapWoodDamageBase = 0,
    trapWoodDamagePct = 0,
    trapWoodDamageExt = 0,
    trapMachineDamageBase = 0,
    trapOilDamagePct = 0,
    trapOilDamageExt = 0,
    riderRestrainInfantryPct = 0,
    infantryRestrainArcherPct = 0,
    archerRestrainMachinePct = 0,
    machineRestrainRiderPct = 0,
}

local property = {
    data = {},      --map<attributeType, propertyInfo>
    list = list,

    evtPropertyChange = event.new(),
    evtResourceChange = event.new(),
}


function propertyInfo:new(o)
    o = o or {}
    setmetatable(o,self)
    self.__index = self
    return o
end

function property.onInit()
end

function property.onAllCompInit()
    property.setProperty(false)
end

function property.onReady()
    --building
    building.evtBuildingLevelUp:attachRaw(property.onBuildingChange)
    building.evtBuildingDemolish:attachRaw(property.onBuildingChange)

    --buff
    buff.evtBuffAdd:attachRaw(property.onBuffAdd)
    buff.evtBuffRemove:attachRaw(property.onBuffRemove)

    --vip
    vip.evtUpgrade:attachRaw(property.onVipUpgrade)

    --alliance
    alliance.evtAllianceJoin:attachRaw(property.onAllianceJoin)
    alliance.evtAllianceQuit:attachRaw(property.onAllianceQuit)
    alliance.evtScienceUpgrade:attachRaw(property.onScienceUpgrade)
    alliance.evtAllianceBuffAdd:attachRaw(property.onAllianceBuffAdd)
    alliance.evtAllianceBuffRemove:attachRaw(property.onAllianceBuffRemove)

    --user
    user.evtLevelUp:attachRaw(property.onUserLevelUp)

    --castle
    agent.map.evtTeleport:attachRaw(property.onCastleMove)
    agent.map.evtCastleRebuild:attachRaw(property.onCastleMove)

    --palaceWar title
    palaceWar.evtTitleUpdate:attachRaw(property.onPalaceTitleUpdate)

    --technology
    technology.evTechnologyUpdate:attachRaw(property.OnTechnologyUpdate)
    property.setProperty()
end

function property.onClose()
end

function property.addPropertyByConf(conf, mul)
    -- print('property.addPropertyByConf...', utils.serialize(conf))
    if conf == nil then
        return
    end
    if mul == nil then
        mul = 1
    end
  -- print('conf[1],conf[2],conf[3]', conf[1], conf[2], conf[3])
    local attributeType = conf[1] or 0
    local additionType = conf[2] or 0
    local value = conf[3] or 0
    property.addProperty(attributeType, additionType, value * mul)
end

function property.addProperty(attributeType, additionType, value)
    -- print('property.addProperty.....attributeType, additionType, value', attributeType, additionType, value)
    if additionType < p.AttributeAdditionType.BASE or additionType > p.AttributeAdditionType.EXT then
         --print('property.addProperty... is not in the range, additionType=', additionType)
        return
    end

    local base, pct, ext = 0, 0, 0
    if additionType == p.AttributeAdditionType.BASE then
        base = value
    elseif additionType == p.AttributeAdditionType.PCT then
        pct = value
    elseif additionType == p.AttributeAdditionType.EXT then
        ext = value
    end
    local prop = property.data[attributeType]  
    if not prop then
        property.data[attributeType] = propertyInfo:new({ base = base, pct = pct, ext = ext })
    else
        prop.base = prop.base + base
        prop.pct = prop.pct + pct
        prop.ext = prop.ext + ext
    end
end

function property.setDetailPropertyList()
    for attributeType, info in pairs(property.data) do
        --军营
       -- if attributeType == p.AttributeType.TRAINING_NUM then
       --     list.barrackTrainNums = info.base * ( 1 + info.pct/10000) + info.ext
       -- elseif attributeType == p.AttributeType.TRAINING_SPEED then
        --    list.barrackTrainSpeedPct = info.pct/10000
        --elseif attributeType == p.AttributeType.TRAINING_QUEUE_NUM then
        --    list.barrackTrainQueues = info.base
       --步兵营
        if attributeType == p.AttributeType.INFANTRY_TRAINING_NUM then
            list.infantryBarrackTrainNums = info.base * ( 1 + info.pct/10000) + info.ext
        elseif attributeType == p.AttributeType.INFANTRY_TRAINING_SPEED then
            list.infantryBarrackTrainSpeedPct = info.pct/10000
        elseif attributeType == p.AttributeType.INFANTRY_TRAINING_QUEUE_NUM then
            list.infantryBarrackTrainQueues = info.base
        --骑兵营
        elseif attributeType == p.AttributeType.RIDER_TRAINING_NUM then
            list.riderBarrackTrainNums = info.base * ( 1 + info.pct/10000) + info.ext
        elseif attributeType == p.AttributeType.RIDER_TRAINING_SPEED then
            list.riderBarrackTrainSpeedPct = info.pct/10000
        elseif attributeType == p.AttributeType.RIDER_TRAINING_QUEUE_NUM then
            list.riderBarrackTrainQueues = info.base
        --弓兵营
        elseif attributeType == p.AttributeType.ARCHER_TRAINING_NUM then
            list.archerBarrackTrainNums = info.base * ( 1 + info.pct/10000) + info.ext
        elseif attributeType == p.AttributeType.ARCHER_TRAINING_SPEED then
            list.archerBarrackTrainSpeedPct = info.pct/10000
        elseif attributeType == p.AttributeType.ARCHER_TRAINING_QUEUE_NUM then
            list.archerBarrackTrainQueues = info.base
        --器械营
        elseif attributeType == p.AttributeType.MECHANICAL_TRAINING_NUM then
            list.mechanicalBarrackTrainNums = info.base * ( 1 + info.pct/10000) + info.ext
        elseif attributeType == p.AttributeType.MECHANICAL_TRAINING_SPEED then
            list.mechanicalBarrackTrainSpeedPct = info.pct/10000
        elseif attributeType == p.AttributeType.MECHANICAL_TRAINING_QUEUE_NUM then
            list.mechanicalBarrackTrainQueues = info.base
        --城墙
        elseif attributeType == p.AttributeType.TRAP_CAPACITY then
            list.trapCapacity = info.base * ( 1 + info.pct/10000) + info.ext
        elseif attributeType == p.AttributeType.TRAP_MAKE_NUM then
            list.trapMakeNums = info.base * ( 1 + info.pct/10000) + info.ext
        elseif attributeType == p.AttributeType.TRAP_BUILDING_SPEED then
            list.trapTrainSpeedPct = info.pct/10000
        elseif attributeType == p.AttributeType.CITY_DEFENSE then
            list.cityDefense = info.base * ( 1 + info.pct/10000) + info.ext
        elseif attributeType == p.AttributeType.CITY_TROOPS_DEFENSE then
            list.cityTroopsDefensePct = info.pct/10000
        -- elseif attributeType == p.AttributeType.TRAP_ATTACK then
        --     list.trapAttackPct = info.pct/10000
        --箭塔
        elseif attributeType == p.AttributeType.TURRET_ATTACK_FORCE then
            list.turretAttackForce = info.base * ( 1 + info.pct/10000) + info.ext
        --仓库
        elseif attributeType == p.AttributeType.RESOURCE_PROTECT_LIMIT then
            list.foodProtectedLimit = info.base * ( 1 + info.pct/10000) + info.ext
            list.woodProtectedLimit = info.base * ( 1 + info.pct/10000) + info.ext
            list.stoneProtectedLimit = info.base * ( 1 + info.pct/10000) + info.ext
            list.ironProtectedLimit = info.base * ( 1 + info.pct/10000) + info.ext
        elseif attributeType == p.AttributeType.FOOD_PROTECT_LIMIT then
            list.foodProtectedLimit = info.base * ( 1 + info.pct/10000) + info.ext
        elseif attributeType == p.AttributeType.WOOD_PROTECT_LIMIT then
            list.woodProtectedLimit = info.base * ( 1 + info.pct/10000) + info.ext
        elseif attributeType == p.AttributeType.STONE_PROTECT_LIMIT then
            list.stoneProtectedLimit = info.base * ( 1 + info.pct/10000) + info.ext
        elseif attributeType == p.AttributeType.IRON_PROTECT_LIMIT then
            list.ironProtectedLimit = info.base * ( 1 + info.pct/10000) + info.ext
        --伤兵营
        elseif attributeType == p.AttributeType.WOUNDED_CAPACITY then
            list.woundedCapacity = info.base * ( 1 + info.pct/10000) + info.ext
        elseif attributeType == p.AttributeType.HEAL_SPEED then
            list.healSpeedPct = info.pct/10000
        elseif attributeType == p.AttributeType.REDUCE_HEAL_RESOURCES then
            list.healReduceResourcePct = info.pct/10000
        --城内资源建筑
        elseif attributeType == p.AttributeType.FOOD_OUTPUT then
            list.foodOutPut = info.base * ( 1 + info.pct/10000) + info.ext
            -- 任务 每小时资源单位产量统计
            property.evtResourceChange:trigger(p.AttributeType.FOOD_OUTPUT, list.foodOutPut)
        elseif attributeType == p.AttributeType.FOOD_CAPACITY then
            list.foodCapacity = info.base * ( 1 + info.pct/10000) + info.ext
        elseif attributeType == p.AttributeType.WOOD_OUTPUT then
            list.woodOutPut = info.base * ( 1 + info.pct/10000) + info.ext
            property.evtResourceChange:trigger(p.AttributeType.WOOD_OUTPUT, list.woodOutPut)
        elseif attributeType == p.AttributeType.WOOD_CAPACITY then
            list.woodCapacity = info.base * ( 1 + info.pct/10000) + info.ext
        elseif attributeType == p.AttributeType.STONE_OUTPUT then
            list.stoneOutPut = info.base * ( 1 + info.pct/10000) + info.ext
            property.evtResourceChange:trigger(p.AttributeType.STONE_OUTPUT, list.stoneOutPut)
        elseif attributeType == p.AttributeType.STONE_CAPACITY then
            list.stoneCapacity = info.base * ( 1 + info.pct/10000) + info.ext
        elseif attributeType == p.AttributeType.IRON_OUTPUT then
            list.ironOutPut = info.base * ( 1 + info.pct/10000) + info.ext
            property.evtResourceChange:trigger(p.AttributeType.IRON_OUTPUT, list.ironOutPut)
        elseif attributeType == p.AttributeType.IRON_CAPACITY then
            list.ironCapacity = info.base * ( 1 + info.pct/10000) + info.ext
        --城主府
        elseif attributeType == p.AttributeType.CASTLE_TAX then
            list.castleTaxSilver = info.base * ( 1 + info.pct/10000) + info.ext
         --市场
        elseif attributeType == p.AttributeType.TRANSPORT_NUM then
            list.transportNum = info.base
        elseif attributeType == p.AttributeType.TRANSPORT_TAX then
            list.transportTaxPct = info.pct/10000
        --部队
        elseif attributeType == p.AttributeType.TROOPS_ATTACK then
            list.troopAttackPct = info.pct/10000
            list.troopAttackExt = info.ext
        elseif attributeType == p.AttributeType.TROOPS_DEFENSE then
            list.troopDefensePct = info.pct/10000
            list.troopDefenseExt = info.ext
        elseif attributeType == p.AttributeType.TROOPS_HEALTH then
            list.troopHpPct = info.pct/10000
            list.troopHpExt = info.ext
        elseif attributeType == p.AttributeType.TROOPS_SPEED then
            list.troopSpeedPct = info.pct/10000
            list.troopSpeedExt = info.ext
        elseif attributeType == p.AttributeType.TROOPS_MORALE then
            list.troopMorale = info.base
        elseif attributeType == p.AttributeType.TROOPS_ATTACK_CITY then
            list.troopAttackCityPct = info.pct/10000
            list.troopAttackCityExt = info.ext
        elseif attributeType == p.AttributeType.TROOPS_LOAD then
            list.troopLoadPct = info.pct/10000
            list.troopLoadExt = info.ext
        --
        elseif attributeType == p.AttributeType.ATTACK_MONSTER_MARCH_SPEED then
            list.attackMonsterSpeedPct = info.pct/10000
        elseif attributeType == p.AttributeType.MARCH_SPEED then
            list.marchSpeedPct = info.pct/10000
        elseif attributeType == p.AttributeType.SCOUT_SPEED then
            list.scoutSpeedPct = info.pct/10000
        elseif attributeType == p.AttributeType.ROB_RESOURCE_MAX then
            list.robResourcePct = info.pct/10000
        elseif attributeType == p.AttributeType.TRANSPORT_SPEED_UP then
            list.transportSpeedUp = info.pct/10000
        --map地图相关
        elseif attributeType == p.AttributeType.FOOD_GATHER_SPEED then
            list.foodGatherSpeedPct = info.pct/10000
        elseif attributeType == p.AttributeType.WOOD_GATHER_SPEED then
            list.woodGatherSpeedPct = info.pct/10000
        elseif attributeType == p.AttributeType.STONE_GATHER_SPEED then
            list.stoneGatherSpeedPct = info.pct/10000
        elseif attributeType == p.AttributeType.IRON_GATHER_SPEED then
            list.ironGatherSpeedPct = info.pct/10000

        elseif attributeType == p.AttributeType.CAMPTEMP_SPEED then
            list.camptempSpeedPct = info.pct/10000
        elseif attributeType == p.AttributeType.GATHER_SPEED then
            list.gatherSpeedPct = info.pct/10000
        elseif attributeType == p.AttributeType.ATTACK_PLAYER_SPEED then
            list.attackPlayerSpeedPct = info.pct/10000
        elseif attributeType == p.AttributeType.REINFORCEMENTS_SPEED then
            list.reinforcementsSpeedPct = info.pct/10000
        --
        elseif attributeType == p.AttributeType.UPKEEP_REDUCE then
            list.upkeepReducePct = info.pct/10000
        elseif attributeType == p.AttributeType.REDUCE_RESEARCH_CONSUME then
            list.reduceResearchPct = info.pct/10000
        elseif attributeType == p.AttributeType.BUILDING_SPEED then
            list.buildingSpeedPct = info.pct/10000

        elseif attributeType == p.AttributeType.ALLIANCE_HELP_COUNT then
            list.helpMax = info.base
        elseif attributeType == p.AttributeType.ALLIANCE_HELP_REDUSE_TIME then
            list.helpReduceTime = info.base
        elseif attributeType == p.AttributeType.MARCH_QUEUE_MAX then
            list.maxMarchingQueue = info.base
        elseif attributeType == p.AttributeType.MARCH_HERO_MAX then
            list.maxHeroNum = info.base
        elseif attributeType == p.AttributeType.HEAL_NUM then
            list.healNum = info.base
        elseif attributeType == p.AttributeType.HEAL_QUEUE_NUM then
            list.healQueueNum = info.base
        elseif attributeType == p.AttributeType.ALLIANCE_REINFORCEMENT_NUM then
            list.allianceReinforcementNum = info.base
        elseif attributeType == p.AttributeType.DRILL_GROUNDS_SOLDIER_NUM_MAX then
            list.drillGroundsSoldierNumMax = info.base
        elseif attributeType == p.AttributeType.TRAP_ATTACK then
            list.trapAttackPct = info.pct/10000
            list.trapAttackExt = info.ext


        elseif attributeType == p.AttributeType.TRAP_HORSE_DAMAGE then
            list.trapHorseDamagePct = info.pct/10000
            list.trapHorseDamageExt = info.ext
        elseif attributeType == p.AttributeType.TRAP_STONE_DAMAGE then
            list.trapStoneDamagePct = info.pct/10000
            list.trapStoneDamageExt = info.ext
        elseif attributeType == p.AttributeType.TRAP_WOOD_DAMAGE then
            list.trapWoodDamagePct = info.pct/10000
            list.trapWoodDamageExt = info.ext
        elseif attributeType == p.AttributeType.TRAP_OIL_DAMAGE then
            list.trapOilDamagePct = info.pct/10000
            list.trapOilDamageExt = info.ext


        elseif attributeType == p.AttributeType.RIDER_RESTRAIN_INFANTRY then
            list.riderRestrainInfantryPct = info.pct/10000
        elseif attributeType == p.AttributeType.INFANTRY_RESTRAIN_ARCHER then
            list.infantryRestrainArcherPct = info.pct/10000
        elseif attributeType == p.AttributeType.ARCHER_RESTRAIN_MACHINE then
            list.archerRestrainMachinePct = info.pct/10000
        elseif attributeType == p.AttributeType.MACHINE_RESTRAIN_RIDER then
            list.machineRestrainRiderPct = info.pct/10000


        -- 武将一二级属性5001-5019
        elseif attributeType == p.AttributeType.HERO_POWER then
            list.heroPowerBase = info.base
            list.heroPowerPct = info.pct / 10000
            list.heroPowerExt = info.ext
        elseif attributeType == p.AttributeType.HERO_DEFENSE then
            list.heroDefenseBase = info.base
            list.heroDefensePct = info.pct / 10000
            list.heroDefenseExt = info.ext
        elseif attributeType == p.AttributeType.HERO_WISDOM then
            list.heroWisdomBase = info.base
            list.heroWisdomPct = info.pct / 10000
            list.heroWisdomExt = info.ext
        elseif attributeType == p.AttributeType.HERO_SKILL then
            list.heroSkillBase = info.base
            list.heroSkillPct = info.pct / 10000
            list.heroSkillExt = info.ext
        elseif attributeType == p.AttributeType.HERO_AGILE then
            list.heroAgileBase = info.base
            list.heroAgilePct = info.pct / 10000
            list.heroAgileExt = info.ext
        elseif attributeType == p.AttributeType.HERO_LUCKY then
            list.heroLuckyBase = info.base
            list.heroLuckyPct = info.pct / 10000
            list.heroLuckyExt = info.ext
        elseif attributeType == p.AttributeType.HERO_LIFE then
            list.heroLifeBase = info.base
            list.heroLifePct = info.pct / 10000
            list.heroLifeExt = info.ext
        elseif attributeType == p.AttributeType.HERO_PHYSICAL_ATTACK then            
            list.heroPhysicalAttackBase = info.base
            list.heroPhysicalAttackPct = info.pct / 10000
            list.heroPhysicalAttackExt = info.ext
        elseif attributeType == p.AttributeType.HERO_PHYSICAL_DEFENSE then
            list.heroPhysicalDefenseBase = info.base
            list.heroPhysicalDefensePct = info.pct / 10000
            list.heroPhysicalDefenseExt = info.ext
        elseif attributeType == p.AttributeType.HERO_WISDOM_ATTACK then
            list.herowisdomAttackBase = info.base
            list.herowisdomAttackPct = info.pct / 10000
            list.herowisdomAttackExt = info.ext
        elseif attributeType == p.AttributeType.HERO_WISDOM_DEFENSE then
            list.heroWisdomDefenseBase = info.base
            list.heroWisdomDefensePct = info.pct / 10000
            list.heroWisdomDefenseExt = info.ext
        elseif attributeType == p.AttributeType.HERO_HIT then
            list.heroHitBase = info.base
            list.heroHitPct = info.pct / 10000
            list.heroHitExt = info.ext
        elseif attributeType == p.AttributeType.HERO_AVOID then
            list.heroAvoidBase = info.base
            list.heroAvoidPct = info.pct / 10000
            list.heroAvoidExt = info.ext
        elseif attributeType == p.AttributeType.HERO_CRIT_HIT then
            list.heroCritHitBase = info.base
            list.heroCritHitPct = info.pct / 10000
            list.heroCritHitExt = info.ext
        elseif attributeType == p.AttributeType.HERO_CRIT_AVOID then
            list.heroCritAvoidBase = info.base
            list.heroCritAvoidPct = info.pct / 10000
            list.heroCritAvoidExt = info.ext
        elseif attributeType == p.AttributeType.HERO_SPEED then
            list.heroSpeedBase = info.base
            list.heroSpeedPct = info.pct / 10000
            list.heroSpeedExt = info.ext
        elseif attributeType == p.AttributeType.HERO_CITY_LIFE then
            list.heroCityLifeBase = info.base
            list.heroCityLifePct = info.pct / 10000
            list.heroCityLifeExt = info.ext
        elseif attributeType == p.AttributeType.HERO_FIGHT then
            list.heroFightBase = info.base
            list.heroFightPct = info.pct / 10000
            list.heroFightExt = info.ext
        elseif attributeType == p.AttributeType.HERO_SINGLE_ENERGY then
            list.heroSingleEnergyBase = info.base
            list.heroSingleEnergyPct = info.pct / 10000
            list.heroSingleEnergyExt = info.ext

        -- 6011-6018
        elseif attributeType == p.AttributeType.RIDER_POWER then
            list.riderPowerBase = info.base
            list.riderPowerPct = info.pct / 10000
            list.riderPowerExt = info.ext
        elseif attributeType == p.AttributeType.INFANTRY_POWER then
            list.infantryPowerBase = info.base
            list.infantryPowerPct = info.pct / 10000
            list.infantryPowerExt = info.ext
        elseif attributeType == p.AttributeType.ARCHER_POWER then
            list.archerPowerBase = info.base
            list.archerPowerPct = info.pct / 10000
            list.archerPowerExt = info.ext
        elseif attributeType == p.AttributeType.MECHANICAL_POWER then
            list.mechanicalPowerBase = info.base
            list.mechanicalPowerPct = info.pct / 10000
            list.mechanicalPowerExt = info.ext
        elseif attributeType == p.AttributeType.RIDER_DEFENSE then
            list.riderDefenseBase = info.base
            list.riderDefensePct = info.pct / 10000
            list.riderDefenseExt = info.ext
        elseif attributeType == p.AttributeType.INFANTRY_DEFENSE then
            list.infantry_defenseBase = info.base
            list.infantry_defensePct = info.pct / 10000
            list.infantry_defenseExt = info.ext
        elseif attributeType == p.AttributeType.ARCHER_DEFENSE then
            list.archerDefenseBase = info.base
            list.archerDefensePct = info.pct / 10000
            list.archerDefenseExt = info.ext
        elseif attributeType == p.AttributeType.MECHANICAL_DEFENSE then
            list.mechanicalDefenseBase = info.base
            list.mechanicalDefensePct = info.pct / 10000
            list.mechanicalDefenseExt = info.ext

        elseif attributeType == p.AttributeType.REINFORCE_LIMIT then
            list.reinforceLimit = info.base

        elseif attributeType == p.AttributeType.REDUCE_RIDER_TRAIN_CONSUME then
            list.reduceRiderTrainConsumePct = info.base
        elseif attributeType == p.AttributeType.REDUCE_INFANTRY_TRAIN_CONSUME then
            list.reduceInfantryTrainConsumePct = info.base
        elseif attributeType == p.AttributeType.REDUCE_ARCHER_TRAIN_CONSUME then
            list.reduceArcherTrainConsumePct = info.base
        elseif attributeType == p.AttributeType.REDUCE_MECHANICAL_TRAIN_CONSUME then
            list.reduceMechanicalTrainConsumePct = info.base
        elseif attributeType == p.AttributeType.CAN_ATTACK_MONSTER_LEVEL then
            list.canAttackMonsterLevel = info.base
        elseif attributeType == p.AttributeType.CAN_ATTACK_ELITE_MONSTER_LEVEL then
            list.canAttackEliteMonsterLevel = info.base

        elseif attributeType == p.AttributeType.ATTACK_MONSTER_DAMAGE then
            list.attackMonsterDamageBase = info.base
            list.attackMonsterDamagePct = info.pct / 10000
            list.attackMonsterDamageExt = info.ext
        elseif attributeType == p.AttributeType.ATTACK_ELITE_MONSTER_DAMAGE then
            list.attackEliteMonsterDamageBase = info.base
            list.attackEliteMonsterDamagePct = info.pct / 10000
            list.attackEliteMonsterDamageExt = info.ext
        elseif attributeType == p.AttributeType.ATTACK_WORLDBOSS_DAMAGE then
            list.attackWorldbossDamageBase = info.base
            list.attackWorldbossDamagePct = info.pct / 10000
            list.attackWorldbossDamageExt = info.ext
        end
    end

    --军营
    --list.barrackTrainNums = math.floor(list.barrackTrainNums)
    --list.barrackTrainQueues = math.floor(list.barrackTrainQueues)
    --步兵营  
    list.infantryBarrackTrainNums = math.floor(list.infantryBarrackTrainNums)
    list.infantryBarrackTrainQueues = math.floor(list.infantryBarrackTrainQueues)
    --骑兵营    
    list.riderBarrackTrainNums = math.floor(list.riderBarrackTrainNums)
    list.riderBarrackTrainQueues = math.floor(list.riderBarrackTrainQueues)
    --弓兵营    
    list.archerBarrackTrainNums = math.floor(list.archerBarrackTrainNums)
    list.archerBarrackTrainQueues = math.floor(list.archerBarrackTrainQueues)
    --器械营
    list.mechanicalBarrackTrainNums = math.floor(list.mechanicalBarrackTrainNums)
    list.mechanicalBarrackTrainQueues = math.floor(list.mechanicalBarrackTrainQueues)
    --城墙
    list.trapCapacity = math.floor(list.trapCapacity)
    list.trapMakeNums = math.floor(list.trapMakeNums)
    list.cityDefense = math.floor(list.cityDefense)
    list.turretAttackForce = math.floor(list.turretAttackForce)
    --仓库
    list.foodProtectedLimit = math.floor(list.foodProtectedLimit)
    list.woodProtectedLimit = math.floor(list.woodProtectedLimit)
    list.stoneProtectedLimit = math.floor(list.stoneProtectedLimit)
    list.ironProtectedLimit = math.floor(list.ironProtectedLimit)
    --伤兵营
    list.woundedCapacity = math.floor(list.woundedCapacity)
    --城内资源建筑
    list.foodOutPut = math.floor(list.foodOutPut)
    list.foodCapacity = math.floor(list.foodCapacity)
    list.woodOutPut = math.floor(list.woodOutPut)
    list.woodCapacity = math.floor(list.woodCapacity)
    list.stoneOutPut = math.floor(list.stoneOutPut)
    list.stoneCapacity = math.floor(list.stoneCapacity)
    list.ironOutPut = math.floor(list.ironOutPut)
    list.ironCapacity = math.floor(list.ironCapacity)
    --书院
    list.technologyQueues = math.floor(list.technologyQueues)
    --城主府
    list.castleTaxSilver = math.floor(list.castleTaxSilver)
    
    list.maxMarchingQueue = math.floor(list.maxMarchingQueue)
    list.maxHeroNum = math.floor(list.maxHeroNum)
    list.helpMax = math.floor(list.helpMax)
    list.helpReduceTime = math.floor(list.helpReduceTime)
    list.healNum = math.floor(list.healNum)
    list.healQueueNum = math.floor(list.healQueueNum)
    list.allianceReinforcementNum = math.floor(list.allianceReinforcementNum)
    --校场士兵上限
    list.drillGroundsSoldierNumMax = math.floor(list.drillGroundsSoldierNumMax)
    --市场
    list.transportNum = math.floor(list.transportNum)

end

function property.reset()
    for _, v in pairs(property.data) do
        v.base = 0
        v.pct = 0
        v.ext = 0
    end

    for _, v in pairs(property.list) do
        v = 0
    end
end

function property.setProperty(syncToCs)
    -- print(debug.traceback())
    --reset
    property.reset()

    --user
    local userTpl = t.lordLevel[user.info.level]
    if userTpl then
        for _,attr in pairs(userTpl.attrList) do
            -- print('user attr = ',utils.serialize(attr))
            property.addPropertyByConf(attr)
        end
    end
    --建筑属性
    for _, v in pairs(building.list) do
        --print('v.tplId..',v.tplId)
        local tplBuilding = t.building[v.tplId]
        --print('tplBuilding',utils.serialize(tplBuilding))
        if tplBuilding then
            local tpl = tplBuilding.levels[v.level]
            --print('tpl',utils.serialize(tpl))
            if tpl then
            --print('property.setProperty building...tplId, attrList', v.tplId, utils.serialize(tpl.attrList))
                for _, attr in pairs(tpl.attrList) do
                    property.addPropertyByConf(attr)
                end
            end
        end
    end
    -- print("")
    --科技属性 * 科技level
    for type,techTreeInfo in pairs(technology.techTreeList) do
        if techTreeInfo then
            for groupId,techGroupInfo in pairs(techTreeInfo.techGroupList) do
                if techGroupInfo and techGroupInfo.level > 0 then
                    local tplId = techGroupInfo.tplId  
                    local tpl = t.technology[tplId]
                    --print('tplId,tpl.attrList',tplId,utils.serialize(tpl.attrList))
                    if tpl and tpl.attrList then
                        for _, attr in pairs(tpl.attrList) do
                            property.addPropertyByConf(attr)
                        end
                    end
                end
            end
        end
    end
    --buff
    for _, v in pairs(buff.cityList) do
        for _, val in pairs(v.attr) do
            -- print('buff attr = ',utils.serialize(val))
            property.addPropertyByConf(val)
        end
    end
    --vip
    local vipAttrList = agent.vip.tpl.attrList
    for _, val in pairs(vipAttrList) do
        print('vip attr = ',utils.serialize(val))
        property.addPropertyByConf(val)
    end

    --palaceWar title
    local titleId = agent.palaceWar.titleId
    if titleId ~= 0 then
        print("title attr is .....", titleId)
        local attrList = t.title[titleId].attrList or {}
        for _, val in pairs(attrList) do
            print('title.property...', utils.serialize(val))
            property.addPropertyByConf(val)
        end
    end
    --alliance sciences
    local allianceSciences = alliance.scienceList()
    if allianceSciences then
        for _, v in pairs(allianceSciences) do
            if t.allianceScience[v.groupId] then
                local tpl = t.allianceScience[v.groupId][v.level]
                if tpl then
                    -- print('property.setProperty...alliance sciences, attrList=', utils.serialize(tpl.attrList))
                    property.addPropertyByConf(tpl.attrList)
                end
            end
        end
    end
    --alliance buff
    local allianceBuffs = alliance.allianceBuffList()
    if allianceBuffs then
        local now = timer.getTimestampCache()
        for _, v in pairs(allianceBuffs) do
            --myAllianceBuffs
            local myBuff = alliance.myAllianceBuffs[v.buffId]
            if myBuff and myBuff.open and v.endTimestamp > now then
                -- print('property.setProperty...alliance buff..buffId, myBuff, attrList', v.buffId, utils.serialize(myBuff), utils.serialize(v.tpl.attrList))
                for _, attr in pairs(v.tpl.attrList) do
                    property.addPropertyByConf(attr)
                end
                break
            end
        end
    end
    --中立城池
    local aid = alliance.allianceId()
    local cities = allianceStub.allAllianceCity[aid]
    local loseBuffCities = allianceStub.uselessBuffCity[aid]
    if cities then
        -- print('property.setProperty...cities cities cities', utils.serialize(cities))
        for _,v in pairs(cities) do
            local isAdd = true
            if loseBuffCities then
                for _,m in pairs(loseBuffCities) do
                    if m == v then
                        isAdd = false
                    end
                end
            end
            if isAdd and property.isInCityRange(v) then
                local cityTpl = t.mapCity[v]
                for _,attr in pairs(cityTpl.attrList) do
                    -- print('allAllianceCity attr = ',utils.serialize(attr))
                    property.addPropertyByConf(attr)
                end
                break
            end
        end
    end

    property.setDetailPropertyList()
    --configure
    local propertyBase = t.configure["propertyBase"]
    if list.maxMarchingQueue == 0 then
        list.maxMarchingQueue = propertyBase.initMarchingQueue
    end
    if list.maxHeroNum == 0 then
        print("list.maxHeroNum", list.maxHeroNum)
        list.maxHeroNum = propertyBase.initHeroNum
    end
    if list.helpMax == 0 then
        list.helpMax = t.configure["initHelpCount"]
    end
    if list.helpReduceTime == 0 then
        list.helpReduceTime = t.configure["initHelpReduceTime"]
    end
    if list.technologyQueues == 0 then
        list.technologyQueues = t.configure["technologyqueue"]
    end

    property.sendPropertyUpdate()
    property.sendHeroPropertyUpdate()
    property.syncProperty()

    --同步武将属性到cs 擂台
    if syncToCs == nil then
        syncToCs = true
    end
    if syncToCs then
        property.evtPropertyChange:trigger()
        property.syncHeroPropToCsArenaService()
    end
end

function property.sendPropertyUpdate()
    --TODO:test
    -- if agent.user.sdkType == 'test' then
    --     list.barrackTrainQueues = 5
    -- end
    -- print('=========list.infantryBarrackTrainNums, list.infantryBarrackTrainQueues', list.infantryBarrackTrainNums, list.trapCapacity)
    -- print("property.sendPropertyUpdate", utils.serialize(list))
    agent.sendPktout(p.SC_ATTR_PLUS_UPDATE,
        '@@1=i,2=f,3=i,4=i,5=f,6=i,7=i,8=f,9=i,10=i,11=f,12=i,13=i,14=f,15=i,16=i,17=i,18=i,19=i,20=i,21=i,22=i,23=i,24=i,25=i,26=i,27=f,28=f,29=f,30=f,31=f,32=f,33=f,34=f,35=f,36=f,37=f,38=f,39=f,40=f,41=f,42=f,43=f,44=f,45=f,46=f,47=f,48=f,49=f,50=f,51=f,52=f,53=i,54=i,55=i,56=i,57=i,58=i,59=f,60=i,61=i,62=i,63=i,64=i,65=f,66=f',
        --1~17
        list.infantryBarrackTrainNums,
        list.infantryBarrackTrainSpeedPct,
        list.infantryBarrackTrainQueues,
        list.riderBarrackTrainNums,
        list.riderBarrackTrainSpeedPct,
        list.riderBarrackTrainQueues,
        list.archerBarrackTrainNums,
        list.archerBarrackTrainSpeedPct,
        list.archerBarrackTrainQueues,
        list.mechanicalBarrackTrainNums,
        list.mechanicalBarrackTrainSpeedPct,
        list.mechanicalBarrackTrainQueues,
        list.trapCapacity,
        list.trapTrainSpeedPct,
        list.trapMakeNums,
        list.cityDefense,
        list.foodProtectedLimit,
        list.woodProtectedLimit,
        list.stoneProtectedLimit,
        list.ironProtectedLimit,
        list.woundedCapacity,
        list.foodOutPut,
        list.woodOutPut,
        list.stoneOutPut,
        list.ironOutPut,
        list.castleTaxSilver,
        --38~50
        list.troopAttackPct,
        list.troopAttackExt,
        list.troopDefensePct,
        list.troopDefenseExt,
        list.troopHpPct,
        list.troopHpExt,
        list.troopSpeedPct,
        list.troopSpeedExt,
        list.troopAttackCityPct,
        list.troopAttackCityExt,
        list.troopMorale,
        list.troopLoadPct,
        list.troopLoadExt,
        --51~63
        list.attackMonsterSpeedPct,
        list.marchSpeedPct,
        list.scoutSpeedPct,
        list.robResourcePct,

        list.foodGatherSpeedPct,
        list.woodGatherSpeedPct,
        list.stoneGatherSpeedPct,
        list.ironGatherSpeedPct,

        list.upkeepReducePct,
        list.reduceResearchPct,
        list.buildingSpeedPct,
        list.healSpeedPct,
        list.healReduceResourcePct,

        list.foodCapacity,
        list.woodCapacity,
        list.stoneCapacity,
        list.ironCapacity,

        list.maxMarchingQueue,
        list.maxHeroNum,
        list.turretAttackForce,
        list.healNum,
        list.healQueueNum,
        list.allianceReinforcementNum,
        list.drillGroundsSoldierNumMax,
        list.transportNum,
        list.transportTaxPct,
        list.transportSpeedUp
        )
end

function property.sendHeroPropertyUpdate()
   
    local updateList = {}
    table.insert(updateList, {attrType = p.AttributeType.HERO_POWER, attrBaseAdd = list.heroPowerBase, attrPctAdd = list.heroPowerPct, attrExtAdd = list.heroPowerExt})
    table.insert(updateList, {attrType = p.AttributeType.HERO_DEFENSE, attrBaseAdd = list.heroDefenseBase, attrPctAdd = list.heroDefensePct, attrExtAdd = list.heroDefenseExt})
    table.insert(updateList, {attrType = p.AttributeType.HERO_WISDOM, attrBaseAdd = list.heroWisdomBase, attrPctAdd = list.heroWisdomPct, attrExtAdd = list.heroWisdomExt})
    table.insert(updateList, {attrType = p.AttributeType.HERO_SKILL, attrBaseAdd = list.heroSkillBase, attrPctAdd = list.heroSkillPct, attrExtAdd = list.heroSkillExt})
    table.insert(updateList, {attrType = p.AttributeType.HERO_AGILE, attrBaseAdd = list.heroAgileBase, attrPctAdd = list.heroAgilePct, attrExtAdd = list.heroAgileExt})
    table.insert(updateList, {attrType = p.AttributeType.HERO_LUCKY, attrBaseAdd = list.heroLuckyBase, attrPctAdd = list.heroLuckyPct, attrExtAdd = list.heroLuckyExt})
    table.insert(updateList, {attrType = p.AttributeType.HERO_LIFE, attrBaseAdd = list.heroLifeBase, attrPctAdd = list.heroLifePct, attrExtAdd = list.heroLifeExt})
    table.insert(updateList, {attrType = p.AttributeType.HERO_PHYSICAL_ATTACK, attrBaseAdd = list.heroPhysicalAttackBase, 
        attrPctAdd = list.heroPhysicalAttackPct, attrExtAdd = list.heroPhysicalAttackExt})
    table.insert(updateList, {attrType = p.AttributeType.HERO_PHYSICAL_DEFENSE, attrBaseAdd = list.heroPhysicalDefenseBase, 
        attrPctAdd = list.heroPhysicalDefensePct, attrExtAdd = list.heroPhysicalDefenseExt})
    table.insert(updateList, {attrType = p.AttributeType.HERO_WISDOM_ATTACK, attrBaseAdd = list.heroWisdomAttackBase, 
        attrPctAdd = list.heroWisdomAttackPct, attrExtAdd = list.heroWisdomAttackExt})
    table.insert(updateList, {attrType = p.AttributeType.HERO_WISDOM_DEFENSE, attrBaseAdd = list.heroWisdomDefenseBase, 
        attrPctAdd = list.heroWisdomDefensePct, attrExtAdd = list.heroWisdomDefenseExt})
    table.insert(updateList, {attrType = p.AttributeType.HERO_HIT, attrBaseAdd = list.heroHitBase, attrPctAdd = list.heroHitPct, attrExtAdd = list.heroHitExt})
    table.insert(updateList, {attrType = p.AttributeType.HERO_AVOID, attrBaseAdd = list.heroAvoidBase, attrPctAdd = list.heroAvoidPct, attrExtAdd = list.heroAvoidExt})
    table.insert(updateList, {attrType = p.AttributeType.HERO_CRIT_HIT, attrBaseAdd = list.heroCritHitBase, attrPctAdd = list.heroCritHitPct, attrExtAdd = list.heroCritHitExt})
    table.insert(updateList, {attrType = p.AttributeType.HERO_CRIT_AVOID, attrBaseAdd = list.heroCritAvoidBase, attrPctAdd = list.heroCritAvoidPct, attrExtAdd = list.heroCritAvoidExt})
    table.insert(updateList, {attrType = p.AttributeType.HERO_SPEED, attrBaseAdd = list.heroSpeedBase, attrPctAdd = list.heroSpeedPct, attrExtAdd = list.heroSpeedExt})
    table.insert(updateList, {attrType = p.AttributeType.HERO_CITY_LIFE, attrBaseAdd = list.heroCityLifeBase, attrPctAdd = list.heroCityLifePct, attrExtAdd = list.heroCityLifeExt})
    table.insert(updateList, {attrType = p.AttributeType.HERO_FIGHT, attrBaseAdd = list.heroFightBase, attrPctAdd = list.heroFightPct, attrExtAdd = list.heroFightExt})
    table.insert(updateList, {attrType = p.AttributeType.HERO_SINGLE_ENERGY, attrBaseAdd = list.heroSingleEnergyBase, 
        attrPctAdd = list.heroSingleEnergyPct, attrExtAdd = list.heroSingleEnergyExt})

    -- print("###sendHeroPropertyUpdate()...", utils.serialize(updateList))
    agent.sendPktout(p.SC_HERO_ATTR_PLUS_UPDATE,'@@1=[attrType=i,attrBaseAdd=f,attrPctAdd=f,attrExtAdd=f]', updateList)
end

function property.syncHeroPropToCsArenaService()
    local attrList = property.getHeroProp()
    -- print('property.syncHeroPropToCsArenaService....attrList', utils.serialize(attrList))
    if next(attrList) then
        arenaStub.cast_arenaPropChange(user.uid, attrList)
    end
end

function property.getHeroProp()
    local propList = {}
    local attributeTypeList = {
        p.AttributeType.HERO_ATTACK,
        p.AttributeType.HERO_DEFENSE,
        p.AttributeType.HERO_HEALTH,
        p.AttributeType.HERO_STRATEGY,
        p.AttributeType.HERO_SPEED,
        p.AttributeType.HERO_CHALLENGE,
        p.AttributeType.HERO_RAGE,
        p.AttributeType.HERO_LEADERSHIP,
        p.AttributeType.HERO_INTELLECT,
    }
    for _, attributeType in pairs(attributeTypeList) do
        local prop = property.data[attributeType]
        if prop then
            -- print('property.getHeroProp...prop', utils.serialize(prop))
            propList[attributeType] = { [p.AttributeAdditionType.BASE] = prop.base, [p.AttributeAdditionType.PCT] = prop.pct, [p.AttributeAdditionType.EXT] = prop.ext }
        end
    end
    return propList
end

function property.getArmyProp()
    local propList = {}
    local attributeTypeList = {
        p.AttributeType.TROOPS_ATTACK,
        p.AttributeType.TROOPS_DEFENSE,
        p.AttributeType.TROOPS_HEALTH,
        p.AttributeType.TROOPS_SPEED,
    }
    for _, attributeType in pairs(attributeTypeList) do
        local prop = property.data[attributeType]
        if prop then
            -- print('property.getArmyProp...prop', utils.serialize(prop))
            propList[attributeType] = { [p.AttributeAdditionType.BASE] = prop.base, [p.AttributeAdditionType.PCT] = prop.pct, [p.AttributeAdditionType.EXT] = prop.ext }
        end
    end
    return propList
end

function property.syncProperty()
    local map = agent.map
    if map then
        map.cast("syncProperty", property.list)
    end
end

function property.onBuildingChange(tplid, level)
    property.setProperty()
end

function property.onBuffAdd()
    property.setProperty()
end

function property.onBuffRemove()
    property.setProperty()
end

function property.onVipUpgrade()
    property.setProperty()
end

function property.onAllianceBuffAdd()
    property.setProperty()
end

function property.onAllianceBuffRemove()
    property.setProperty()
end

function property.onScienceUpgrade()
    property.setProperty()
end

function property.onAllianceJoin()
    property.setProperty()
end

function property.onAllianceQuit()
    property.setProperty()
end

function property.onPalaceTitleUpdate()
    property.setProperty() 
end

function property.OnTechnologyUpdate()
    property.setProperty()
end

function property.isInRect(x,y,xStart,yStart,xEnd,yEnd)
    return x >= xStart and x <= xEnd and y >= yStart and y <= yEnd
end

function property.isInCityRange( cityId )
    local y = cityId % 1200
    local x = (cityId-y) / 1200

    local mapCityTpl = t.mapCity[cityId]
    local mapUnitTpl = t.mapUnit[mapCityTpl.unitId]
    local buffRange = mapUnitTpl.paramExt.buffRange
    if buffRange == nil then
        return false
    end

    local xStart = x - (buffRange[1]-mapUnitTpl.cellSizeX)/2
    local yStart = y - (buffRange[2]-mapUnitTpl.cellSizeY)/2
    -- local xEnd = x + mapUnitTpl.cellSizeX - 1 + (buffRange[1]-mapUnitTpl.cellSizeX)/2
    -- local yEnd = y + mapUnitTpl.cellSizeY - 1 + (buffRange[2]-mapUnitTpl.cellSizeY)/2
    local xEnd = x + mapUnitTpl.cellSizeX/2 + buffRange[1]
    local yEnd = y + mapUnitTpl.cellSizeY/2 + buffRange[2]

    local mapInfo = agent.map.info
    local castleTpl = mapInfo.castleTpl

    -- local isInRange = property.isInRect(mapInfo.x,mapInfo.y,xStart,yStart,xEnd,yEnd) or 
    --     property.isInRect(mapInfo.x + castleTpl.cellSizeX - 1, mapInfo.y, xStart,yStart,xEnd,yEnd) or
    --     property.isInRect(mapInfo.x, mapInfo.y + castleTpl.cellSizeY - 1, xStart,yStart,xEnd,yEnd) or
    --     property.isInRect(mapInfo.x + castleTpl.cellSizeX - 1, mapInfo.y + castleTpl.cellSizeY - 1, xStart,yStart,xEnd,yEnd)

    local isInRange = property.isInRect(mapInfo.x, mapInfo.y, xStart, yStart, xEnd, yEnd) 

    -- if isInRange then
    --     print('----isInCityRange xStart yStart xEnd yEnd, mapInfo.x mapInfo.y', isInRange, xStart, yStart, xEnd, yEnd, mapInfo.x, mapInfo.y)
    -- end

    return isInRange
end

function property.onAllianceOwnCity(cityId)
    if property.isInCityRange(cityId) then
        property.setProperty()
    end
end

function property.onAllianceLoseCity(cityId)
    if property.isInCityRange(cityId) then
        property.setProperty()
    end
end

function property.onUserLevelUp()
    property.setProperty()
end

function property.onCastleMove()
    --中立城池
    local aid = alliance.allianceId()
    local cities = allianceStub.allAllianceCity[aid]
    if cities then
        if #cities > 0 then
            property.setProperty()
        end
    end
end

return property