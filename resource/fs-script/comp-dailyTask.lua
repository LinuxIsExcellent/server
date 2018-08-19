local agent = ...
local p = require('protocol')
local t = require('tploader')
local timer = require('timer')
local utils = require('utils')
local miscUtil = require('libs/misc')
local logStub = require('stub/log')
local user = agent.user
local dict = agent.dict
local cdlist = agent.cdlist
local building = agent.building
local scenarioCopy = agent.scenarioCopy
local arena = agent.arena
local hero = agent.hero
local babel = agent.babel
local map = agent.map
local bronzeSparrowTower = agent.bronzeSparrowTower
local alliance = agent.alliance
local store = agent.store
local user = agent.user
local charge = agent.charge

--local pktHandlers = {}

local dailyTask = {
    doList = {},    --<key=id, dailyTaskInfo>
    drawList = {},  --<key=id, 1>
    curScore = 0,
}

local dailyTaskInfo = {
    id = 0,
    progress = 0,
    isDraw = false,
    -- sync
    isSync = false,
}

function dailyTaskInfo:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end

local function getRefreshTime()
    --刷新倒计时(秒)
    local refreshTime = timer.todayZeroTimestamp() + 24 * 3600
    local leftSeconds = refreshTime - timer.getTimestampCache()
    if leftSeconds < 0 then
        leftSeconds = 0
    end
    return leftSeconds
end

local function getNextTargetScore()
    local nextScore = 0
    local maxScore = 0
    for _, v in pairs(t.questDayPoints) do
        if maxScore <= v.points then
            maxScore = v.points
        end
        if v.points > dailyTask.curScore then 
            if nextScore == 0 then
                nextScore = v.points
            end
            if nextScore >= v.points then
                nextScore = v.points
            end
        end
    end
    if dailyTask.curScore >= maxScore then
        nextScore = maxScore
    end
    return nextScore
end

function dailyTask.onInit()
    --agent.registerHandlers(pktHandlers)
    dailyTask.dbLoad()

    if cdlist.isHour0Refresh then
        dailyTask.refresh(false)
    end

    cdlist.evtHour0Refresh:attachRaw(function()
        dailyTask.refresh(true)
    end)

    dailyTask.checkDailyTask()
    --等级条件
    -- user.evtLevelUp:attachRaw(function()
    --     dailyTask.checkDailyTask()
    --     dailyTask.sendDailyTaskUpdate()
    -- end)
    -- --BuildingLeve
    -- building.evtBuildingLevelUp:attachRaw(function()
    --     dailyTask.checkDailyTask()
    --     dailyTask.sendDailyTaskUpdate()
    -- end)

    -- 1001
    building.evtBuildingTrain:attachRaw(dailyTask.onTrainArmy)
    -- 1002 -- 1020
    cdlist.evtSpeed:attachRaw(dailyTask.TrainArmySpeed)
    -- 1020
    building.evtBuildingSpeedUp:attachRaw(dailyTask.onBuildingSpeedUp)
    -- 1053
    map.evtKillMonster:attachRaw(dailyTask.onKillMonster)
    -- 1004
    building.evtBuildingHeal:attachRaw(dailyTask.onBuildingHeal)
    -- 1005
    building.evtBuildingCollect:attachRaw(dailyTask.onBuildingCollect)
    -- 1006
    map.evtResourceGather:attachRaw(dailyTask.onResourceGather)
    -- 1007 
    hero.evtSkillUpgrade:attachRaw(dailyTask.onSkillUpgrade)
    -- 1008
    hero.evtHeroLevelUpOne:attachRaw(dailyTask.onHeroLevelUpOne)
    -- 1009
    alliance.evtAllianceDonate:attachRaw(dailyTask.onAllianceDonate)
    -- 1010
    alliance.evtAllianceHelp:attachRaw(dailyTask.onAllianceHelp)
    -- 1011
    store.evtBuyItem:attachRaw(dailyTask.onBuyItem)
    -- 1012
    babel.evtPassBabel:attachRaw(dailyTask.onPassBabel)
    -- 1013 1014
    scenarioCopy.evtFightScenarioCopy:attachRaw(dailyTask.onFightScenarioCopy)
    -- 1015 1071 
    arena.evtFightArena:attachRaw(dailyTask.onFightArena)
    -- 1016
    building.evtResourceTax:attachRaw(dailyTask.onResourceTax)
    -- 1017
    building.evtGoldResourceTax:attachRaw(dailyTask.onGoldResourceTax)
    -- 1018
    bronzeSparrowTower.evtAnswer:attachRaw(dailyTask.onAnswer)
    -- 1019
    building.evtBuildingLevelUp:attachRaw(dailyTask.onBuildingLevelUp)
    -- 1021
    map.evtRobPlayer:attachRaw(dailyTask.onRobPlayer)

    -- 1121 
    hero.evtDrawHeroByType:attachRaw(dailyTask.onDrawGetHero)
    -- 1152 
    map.evtAttackMonster:attachRaw(dailyTask.onAttackMonster)
 
    -- 1181 
    user.evtBuyStamina:attachRaw(dailyTask.onBuyStamina)

    --1182 
    charge.evtCharge:attachRaw(dailyTask.onRecharge)

end

function dailyTask.onAllCompInit()
    dailyTask.sendDailyTaskUpdate()
    dailyTask.sendDailyTaskRewardUpdate()
end

function dailyTask.onClose()
    dailyTask.dbSave()
end

function dailyTask.onSave()
    dailyTask.dbSave()
end

function dailyTask.dbLoad()
    local data = dict.get('dailyTask.info')
    if data then
        if not data.doList then
            data.doList = {}
        end
        for _, v in pairs(data.doList) do
            dailyTask.doList[v.id] = dailyTaskInfo:new(v)
        end
        dailyTask.curScore = data.curScore or 0
        dailyTask.drawList = data.drawList or {}
    end
end

function dailyTask.dbSave()
    local doList = {}
    for _, v in pairs(dailyTask.doList) do
        doList[v.id] = { id = v.id, progress = v.progress, isDraw = v.isDraw }
    end
    local data = {}
    data.curScore = dailyTask.curScore
    data.doList = doList
    data.drawList = dailyTask.drawList
    dict.set('dailyTask.info', data)
end

function dailyTask.checkDailyTask()
    -- print('###dailyTask.checkDailyTask...')

    --1.找可以接的任务
    local canAccetpTpls = {}
    for _, tpl in pairs(t.questDay) do
        if not dailyTask.doList[tpl.id] then
            -- print("------------------------questDay ", utils.serialize(tpl))
            if user.info.level >= tpl.minLevel and user.info.level <= tpl.maxLevel then
                local existTpl = canAccetpTpls[tpl.CondType]
                if existTpl then
                    if existTpl.id < tpl.id then
                        canAccetpTpls[tpl.CondType] = tpl
                    end
                else
                    canAccetpTpls[tpl.CondType] = tpl
                end
            end
        end
    end   


    for _, tpl in pairs(canAccetpTpls) do
        -- print("------------------------canAccetpTpls id = ", tpl.id)
        if not dailyTask.doList[tpl.id] then
            local info = dailyTaskInfo:new({})
            info.id = tpl.id
            info.progress = 0
            info.isSync = false
            dailyTask.doList[tpl.id] = info
        end
    end
end

function dailyTask.refresh(sendUpdate)
    --print('###dailyTask.refresh...', sendUpdate)
    dailyTask.doList = {}
    dailyTask.drawList = {}
    dailyTask.curScore = 0
    dailyTask.checkDailyTask()
    if sendUpdate then
        dailyTask.sendDailyTaskUpdate()
        dailyTask.sendDailyTaskRewardUpdate()
    end
end

function dailyTask.sendDailyTaskUpdate()
    -- print('###dailyTask.sendDailyTaskUpdate...')
    local list = {}
    for _, v in pairs(dailyTask.doList) do
        if not v.isSync and v.id > 0 then
            v.isSync = true
            table.insert(list, { id = v.id,  progress = v.progress, isDraw = v.isDraw })
        end
    end
    -- print("###sendDailyTaskUpdate,", utils.serialize(list))
    if next(list) then
        local nextScore = getNextTargetScore()
         for _, v in pairs(list) do
         end
        agent.sendPktout(p.SC_DAILY_TASK_UPDATE, '@@1=i,2=i,3=[id=i,progress=i,isDraw=b]', dailyTask.curScore, nextScore, list)
    end
end

function dailyTask.sendDailyTaskRewardUpdate()
    local list = {}
    for k, v in pairs(dailyTask.drawList) do
        if v > 0 then
            table.insert(list, { targetId = k })
        end
    end
    local leftSeconds = getRefreshTime()
    --print('###sendDailyTaskRewardUpdate..leftSeconds, list', leftSeconds, utils.serialize(list))
    agent.sendPktout(p.SC_DAILY_TASK_REWARD_UPDATE, '@@1=i,2=[targetId=i]', leftSeconds, list)
end

function dailyTask.handleCondData(cond, val, param1, param2, param3)
    local p1 = param1 or 0
    local p2 = param2 or 0
    local p3 = param3 or 0
    --print('###dailyTask.handleCondData...cond, val....',cond, val)
    if cond == nil or val == nil or val <=0 then
        return
    end

    local bSend = false
    for _, v in pairs(t.questDay) do
        local bUpdate = false

        local task = dailyTask.doList[v.id]
        if task and v.CondType == cond and v.param1 == p1 and v.param2 == p2 and v.param3 == p3 then
            if task.progress < v.count then
                task.progress = task.progress + val
                bUpdate = true
            end
        end

        --dailyTask.curScore
        if bUpdate then
            bSend = true
            if task.progress >= v.count then
                task.progress = v.count
                -- print("dailyTask.handleCondData, task, count", utils.serialize(task), v.count)
                -- print(debug.traceback())
                dailyTask.curScore = dailyTask.curScore + v.point
            end
            task.isSync = false
        end
    end

    if bSend then
        dailyTask.sendDailyTaskUpdate()
    end
end

function dailyTask.onTrainArmy(armyType, count)
    for _, v in pairs(t.questDay) do
        local task = dailyTask.doList[v.id]
        if task and v.CondType == p.DailyTaskCondType.TRAIN_ARMY then
            if  armyType >= v.param1 then
                dailyTask.handleCondData(p.DailyTaskCondType.TRAIN_ARMY, count, v.param1)
            end
        end
    end

    --dailyTask.handleCondData(p.DailyTaskCondType.TRAIN_ARMY, count)
end

-- 这个是建筑升级的过程中用金币加速
function dailyTask.TrainArmySpeed(cdType, speedType)
    if cdType == p.ItemPropType.TROOP_TRAINING_SPEEDUP then
        -- print(debug.traceback())
        dailyTask.handleCondData(p.DailyTaskCondType.SPEED_TRAIN, 1)
    elseif cdType == p.ItemPropType.BUILDING_SPEEDUP and speedType == p.CDSpeedUpType.GOLD then
        dailyTask.handleCondData(p.DailyTaskCondType.BUILDING_SPEED, 1)
    end
end

-- 这个是直接用金币完成的
function dailyTask.onBuildingSpeedUp()
    dailyTask.handleCondData(p.DailyTaskCondType.BUILDING_SPEED, 1)
end

function dailyTask.onKillMonster(id)
    local tpl = t.mapUnit[id]
    if tpl then
        for _, v in pairs(t.questDay) do
            local task = dailyTask.doList[v.id]
            if task and v.CondType == p.DailyTaskCondType.KILL_MONSTER_NUM then
                if  tpl.level >= v.param1 then
                    dailyTask.handleCondData(p.DailyTaskCondType.KILL_MONSTER_NUM, 1, v.param1)
                end
            end
        end
    end
end

function dailyTask.onAttackMonster(monsterTpl, isWin)
    for _, v in pairs(t.questDay) do
        local task = dailyTask.doList[v.id]
        if task and v.CondType == p.DailyTaskCondType.ATTACK_MONSTER_NUM then
            if  tpl.level >= v.param1 then
                dailyTask.handleCondData(p.DailyTaskCondType.ATTACK_MONSTER_NUM, 1, v.param1)
            end
        end
    end
end

function dailyTask.onBuildingHeal()
    dailyTask.handleCondData(p.DailyTaskCondType.HEAL_ARMY, 1)
end

function dailyTask.onBuildingCollect()
    dailyTask.handleCondData(p.DailyTaskCondType.COLLECT_RESOURCE, 1)
end

function dailyTask.onResourceGather(resourceType, count)
    if p.ResourceType.FOOD == resourceType then
        dailyTask.handleCondData(p.DailyTaskCondType.GATHER_FOOD, 1)
    end
end

function dailyTask.onSkillUpgrade()
   dailyTask.handleCondData(p.DailyTaskCondType.UPGRADE_HERO_SKILL, 1) 
end

function dailyTask.onHeroLevelUpOne(id ,level)
    dailyTask.handleCondData(p.DailyTaskCondType.UPGRADE_HERO, 1) 
end

function dailyTask.onAllianceDonate()
   dailyTask.handleCondData(p.DailyTaskCondType.DONATE_ALLIANCE_SCIENCE, 1)  
end

function dailyTask.onAllianceHelp()
   dailyTask.handleCondData(p.DailyTaskCondType.HELP_ALLIANCE, 1)   
end

function dailyTask.onBuyItem(type, count)
    if type == p.StoreType.STORE_ALLIANCE then
        dailyTask.handleCondData(p.DailyTaskCondType.ALLIANCE_STORE_BUY, 1)   
    end
end

function dailyTask.onPassBabel(layer)
    for _, v in pairs(t.questDay) do
        local task = dailyTask.doList[v.id]
        if task and v.CondType == p.DailyTaskCondType.BABEL_WIN_NUM then
            if  layer >= v.param1 then
                dailyTask.handleCondData(p.DailyTaskCondType.BABEL_WIN_NUM, 1, v.param1)
            end
        end
    end
end

function dailyTask.onFightScenarioCopy(tplSec, isWin, count, tpl)
    if tpl.type == p.ChapterType.NORMAL then
        dailyTask.handleCondData(p.DailyTaskCondType.PASS_NORMAL_SCENARIO, count)
    elseif tpl.type == p.ChapterType.ELITE then
        dailyTask.handleCondData(p.DailyTaskCondType.PASS_ELITE_SCENARIO, count)    
    end
end

function dailyTask.onFightArena(isWin)
    if isWin then
       dailyTask.handleCondData(p.DailyTaskCondType.ARENA_WIN_NUM, 1)     
    end
    dailyTask.handleCondData(p.DailyTaskCondType.ARENA_NUM, 1)
end

function dailyTask.onResourceTax(resourceType, num)
    if resourceType == p.ResourceType.SILVER then
       dailyTask.handleCondData(p.DailyTaskCondType.TAKE_TAX, 1)      
    end
end

function dailyTask.onGoldResourceTax()
    dailyTask.handleCondData(p.DailyTaskCondType.GOLD_TAKE_TAX, 1)
end

function dailyTask.onAnswer(score)
    dailyTask.handleCondData(p.DailyTaskCondType.BRONZE_NUM, 1)
end

function dailyTask.onBuildingLevelUp(tplId, level)
    dailyTask.handleCondData(p.DailyTaskCondType.UPGRADE_BUILDING, 1)
end

function dailyTask.onRobPlayer(isWin)
    -- print(debug.traceback())
    dailyTask.handleCondData(p.DailyTaskCondType.ROB_PLAYER, 1)
end

function dailyTask.onDrawGetHero(drawType, count)
    for _, v in pairs(t.questDay) do
        local task = dailyTask.doList[v.id]
        if task and v.CondType == p.DailyTaskCondType.DRAW_GET_HERO then
            if  drawType == v.param1 then
                dailyTask.handleCondData(p.DailyTaskCondType.DRAW_GET_HERO, 1, v.param1)
            end
        end
    end
end

-- 1181
function dailyTask.onBuyStamina()
    dailyTask.handleCondData(p.DailyTaskCondType.BUY_STAMINA, 1)
end

function dailyTask.onRecharge(tpl)
    if tpl then
        dailyTask.handleCondData(p.DailyTaskCondType.RECHARGE, tpl.price)
    end
end

function dailyTask.onConsumeGold(count)
    dailyTask.handleCondData(p.DailyTaskCondType.CONSUME_GOLD, count)
end

function dailyTask.onStoreBuyShop(count)
    dailyTask.handleCondData(p.DailyTaskCondType.STORE_BUY_SHOP, count)
end

function dailyTask.onUseBoostItem()
    local tpl = t.item[tplId]
    if not tpl then 
        return
    end

    local propType = tpl.subType 
    if propType == p.ItemPropType.FRAM_BOOST or propType == p.ItemPropType.SAWMILL_BOOST 
        or propType == p.ItemPropType.STONE_MINE_BOOST or propType == p.ItemPropType.IRON_MINE_BOOST then
        dailyTask.handleCondData(p.DailyTaskCondType.USE_BOOST_ITEM, 1)
    end     
end

function dailyTask.onUseResourceItem()
    local tpl = t.item[tplId]
    if not tpl then 
        return
    end

    local propType = tpl.subType 
    if propType == p.ItemPropType.FOOD or propType == p.ItemPropType.WOOD 
        or propType == p.ItemPropType.STONE or propType == p.ItemPropType.IRON then
        dailyTask.handleCondData(p.DailyTaskCondType.USE_RESOURCE_ITEM, 1)
    end   

    --dailyTask.handleCondData(p.DailyTaskCondType.USE_RESOURCE_ITEM, 1)
end

function dailyTask.onUseSpeedItem()
    local tpl = t.item[tplId]
    if not tpl then 
        return
    end

    local propType = tpl.subType 
    if propType >= p.ItemPropType.BUILDING_SPEEDUP  and propType <= p.ItemPropType.SPEEDUP then
        dailyTask.handleCondData(p.DailyTaskCondType.USE_SPEED_ITEM, 1)
    end   

    --dailyTask.handleCondData(p.DailyTaskCondType.USE_SPEED_ITEM, 1)
end

--pktHandlers[p.CS_DAILY_TASK_REWARD_DRAW] = function(pktin, session)
function dailyTask.cs_daily_task_reward_draw(targetId)
    local function dailyTaskRewardDrawResponse(result)
        -- print('###dailyTaskRewardDrawResponse..result', result)
        agent.replyPktout(session, p.SC_DAILY_TASK_REWARD_DRAW_RESPONSE, result)
    end
    -- print('###p.CS_DAILY_TASK_REWARD_DRAW..targetId',targetId)

    local tpl = t.questDayPoints[targetId]
    if not tpl then
        print('p.CS_DAILY_TASK_REWARD_DRAW: tpl_daily_task_reward, no this tpl..id = ',targetId)
        dailyTaskRewardDrawResponse(false)
        return
    end
    if dailyTask.curScore < tpl.points then
        print('###p.CS_DAILY_TASK_REWARD_DRAW, user score is not enough..dailyTask.curScore, tpl.points', dailyTask.curScore, tpl.points)
        dailyTaskRewardDrawResponse(false)
        return
    end
    if dailyTask.drawList[targetId] and dailyTask.drawList[targetId] > 0 then
        print('###p.CS_DAILY_TASK_REWARD_DRAW, target is drew..id', targetId)
        dailyTaskRewardDrawResponse(false)
        return
    end

    --pick drop
    local dropTpl = t.drop[tpl.dropId]
    if not dropTpl then
        print('dropId not exist', dropId)
        return
    end
    local dropList = dropTpl:DoDrop() or {}
    agent.bag.pickDropItems(dropList, p.ResourceGainType.DAILY_TASK_REWARD)

    -- log
    -- logStub.appendDailyTaskReward(user.uid, tpl.targetScore, timer.getTimestampCache())

    dailyTask.drawList[targetId] = 1
    dailyTask.sendDailyTaskRewardUpdate()
    dailyTaskRewardDrawResponse(true)
end

return dailyTask
