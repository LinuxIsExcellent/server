local agent = ...
local p = require('protocol')
local t = require('tploader')
local timer = require('timer')
local event = require('libs/event')
local utils = require('utils')
local pushStub = require('stub/push')
local logStub = require('stub/log')

local property = {}
local dict = agent.dict
local user = agent.user
local cdlist = agent.cdlist
local army = agent.army
local bag = agent.bag
local tech = agent.technology

local cancelratio = t.configure["cancelratio"] or 0.8

local building = {
    list = {},          --map<gridId, buildingInfo>  gridId:城内[1-99] 城外[100-n]
    removeList = {},    --建筑移除列表 map<id>
    builder1 = 0,
    builder2 = 0,
    builder2ExpireTime = timer.getTimestampCache(),
    maxId = 0,          --建筑Id
    forestList = {},    --<forestId,1>

    -- events
    evtBuildingLevelUp = event.new(), --(tplId, level)
    evtCastleLevelUp = event.new(),    --(level)
    evtMarketCreate = event.new(),    --(level)
    evtBuildingDemolish = event.new(), --(tplId, level)
    evtBuildingOpenForest = event.new(), --()
    evtBuildingStateClose = event.new(), -- (buildingId, cdId)
    evtBuildingCollectBoost = event.new(), --(buildingType)
    evtBuildingCollect = event.new(), --()
    evtBuildingTrain = event.new(),  --(type, count)
    eveTakeTax = event.new(), --(takeType)
    evtResourceTax = event.new(),     -- (resourceType, num)
    evtGoldResourceTax = event.new(),    -- ()
    evtBuildingHeal = event.new(),     -- (count)
    evtBuildingSpeedUp = event.new(),    -- ()
}

local buildingInfo = {
    id = 0,     --建筑Id(唯一)
    gridId = 0, --格子Id
    tplId = 0,  --建筑模板Id
    level = 0,  --建筑等级
    state = p.BuildingState.NORMAL,
    jobList = {},   --map<cdId, jobInfo >
    param1 = 0,
    param2 = 0,
    param3 = 0,
    sync = false,
    --[[
    param1 param2 param3说明:
    城外资源建筑: param1=上次采集时间 param2=双倍加成开始时间 param3=双倍加成结束时间
    ]]
}

local jobInfo = {
    cdId = 0,
    flagId = 0,     --标志 不同的建筑有不同的值
    result = 0,     --标志结果 1进行中 2已完成 3已处理
    p1 = 0,
    p2 = 0,
    p3 = 0,
    isSync = false,
    --[[说明
    cdId是必须有的，一些特殊的建筑会有自己的特有数据:
    if 军营 伤病营then
        flagId = 训练队列序号
        p1 = 兵种类型 ArmyType
        p2 = 数量
        p3 = 训练时兵种等级
        result = 1,2,3
    elseif 书院 then
        flagId = 科技ID
        p1 = 消耗 resourceConsume = {food=i, wood=i, stone = i, iron = i}
        result = 1,3
    elseif 伤兵营 then
        p1 = 伤兵列表healList[armyType] = {count=i,level=i}
        p2 = 伤兵治疗消耗 resourceConsume = {food=i, wood=i, stone = i, iron = i}
    end
    --]]
}

--主城征税
local buildingTax = {
    taxFreeCount = 0,   --free征税次数
    taxFreeExpireTime = timer.getTimestampCache(),  --free征税的CD到期时间
    taxFreeCD = 10,     --freeCD
    taxGoldCount = 0,   --gold征税次数
}

local pktHandlers = {}

function buildingInfo:new(o)
    o = o or {}
    if o.id == 0 or o.id == nil then
        building.maxId =  building.maxId + 1
        o.id = building.maxId
    end
    if o.jobList == nil then
        o.jobList = {}
    end
    setmetatable(o, self)
    self.__index = self
    return o
end

function jobInfo:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end

function buildingInfo:isCollectBuilding()
    local tpl = t.building[self.tplId]
    if tpl then
        local type = tpl.buildingType
        if type == p.BuildingType.FARM
            or type == p.BuildingType.SAWMILL
            or type == p.BuildingType.STONE_MINE
            or type == p.BuildingType.IRON_MINE then
            return true
        end
    end
    return false
end

function buildingInfo:isTrainBuilding()
    local tpl = t.building[self.tplId]
    if tpl then
        if tpl.buildingType == p.BuildingType.BARRACKS or tpl.buildingType == p.BuildingType.WALLS 
            or tpl.buildingType == p.BuildingType.INFANTRY_BARRACKS or tpl.buildingType == p.BuildingType.RIDER_BARRACKS 
            or tpl.buildingType == p.BuildingType.ARCHER_BARRACKS or tpl.buildingType == p.BuildingType.MECHANICAL_BARRACKS then
            return true
        end
    end
    return false
end

function buildingInfo:getBuildingType()
    local tpl = t.building[self.tplId]
    if tpl then
        return tpl.buildingType or 0
    end
    return 0
end

function buildingInfo:onBuildFinish()
    local tpl = t.building[self.tplId]
    if tpl then
        --城外资源,需要记录时间，以便计算资源产量
        if self:isCollectBuilding() then
            self.param1 = timer.getTimestampCache()
            self.sync = false
        end
    end
    -- print('buildingInfo:onBuildFinish...self.tplId, self.level', self.tplId, self.level)
    building.evtBuildingLevelUp:trigger(self.tplId, self.level)
    if tpl.buildingType == p.BuildingType.CASTLE then
        building.evtCastleLevelUp:trigger(self.level)
    -- 开启市场就dict就存一次，cs中接受入盟申请的时候需要读取当前市场的等级
    elseif tpl.buildingType == p.BuildingType.MARKET and self.level == 1 then
        building.dbSave()
        building.evtMarketCreate:trigger()
    end
end

function building.newBuildingInfo(o)
    return buildingInfo:new(o)
end

function building.onInit()
    agent.registerHandlers(pktHandlers)
    building.dbLoad()

    -- create default buildings
    for _, v in pairs(t.building) do
        if v.isInside and not building.list[v.buildingType] then
            local level = 1
            if v.initState == p.BuildingState.DAMAGED then
                level = 0
            end
            building.list[v.buildingType] = buildingInfo:new({ gridId = v.buildingType, tplId = v.id, level = level, state = v.initState })
        end
    end
    building.evtBuildingLevelUp:attachRaw(building.logBuilding)
    agent.combat.evtCombatOver:attachRaw(building.onCombatOver)

    if cdlist.isHour0Refresh then
        building.refreshTax()
    end
    cdlist.evtHour0Refresh:attachRaw(function()
        building.refreshTax()
    end)
end

function building.onAllCompInit()
    property = agent.property

    for _, v in pairs(building.list) do
        -- rebind callbacks
        for _, v2 in pairs(v.jobList) do
            if v2.cdId > 0 then
                local cd = cdlist.getCD(v2.cdId)
                if cd == nil then
                    -- print('finished gridId, cdId =', v.gridId, v2.cdId)
                    building.cdlistCallback(v2.cdId, 'self')
                elseif cd:isCooling() then
                    -- print('gridId, cdId, cdLeftSesonds =', v.gridId, v2.cdId, cd:getRemainTime())
                    cd:setCb(building.cdlistCallback)
                else
                    building.cdlistCallback(v2.cdId, 'self')
                end
            end
        end
    end

    building.sendBuildingUpdate()
    building.sendBuildingQueuesUpdate()
    building.sendTaxUpdate()
end

function building.onClose()
    building.dbSave()
end

function building.onTimerUpdate(timerIndex)

end

function building.dbLoad()
    local function cdIsValid(id)
        local cd = cdlist.getCD(id)
        if cd and cd:isCooling() then
            return true
        end
        return false
    end
    --building.list
    local blist = dict.get('building.list') or {}
    for _, v in pairs(blist) do
        v.sync = false
        if v.tplId == 0 then
            v.sync = true
        end
        local tempInfo = {
            id = v.id,
            gridId = v.gridId,
            tplId = v.tplId,
            level = v.level,
            state = v.state,
            param1 = v.param1,
            param2 = v.param2,
            param3 = v.param3,
            sync = v.sync,
        }
        local binfo = buildingInfo:new(tempInfo)
        for cdId, job in pairs(v.jobList) do
            local jinfo = jobInfo:new(job)
            binfo.jobList[cdId] = jinfo
        end
        -- print('building.dbLoad...buildingInfo..', utils.serialize(binfo))

        building.list[v.gridId] = binfo
    end
    --building.info
    local info = dict.get('building.info') or {}
    building.maxId = info.maxId or building.maxId
    building.forestList = info.forestList or building.forestList
    building.builder1 = info.builder1 or building.builder1
    if not cdIsValid(building.builder1) then
        building.builder1 = 0
    end
    building.builder2 = info.builder2 or building.builder2
    if not cdIsValid(building.builder2) then
        building.builder2 = 0
    end
    building.builder2ExpireTime = info.builder2ExpireTime or building.builder2ExpireTime

    local tax = dict.get('building.tax') or {}
    buildingTax.taxFreeCount = tax.taxFreeCount or 0
    buildingTax.taxFreeExpireTime = tax.taxFreeExpireTime or 0
    buildingTax.taxGoldCount = tax.taxGoldCount or 0
    buildingTax.freeTaxList = tax.freeTaxList or {}
end

function building.dbSave()
    local blist = {}
    for _, v in pairs(building.list) do
        local jobList = {}
        for cdId, job in pairs(v.jobList) do
            jobList[cdId] = { cdId = job.cdId, flagId = job.flagId, result = job.result, p1 = job.p1, p2 = job.p2, p3 = job.p3 }
        end
        table.insert(blist, {
            id = v.id,
            gridId = v.gridId,
            tplId = v.tplId,
            level = v.level,
            state = v.state,
            jobList = jobList,
            param1 = v.param1,
            param2 = v.param2,
            param3 = v.param3,
          })
        -- print('building.dbSave id, gridId, tplId, level, state, param1, param2, param3, jobList= ' , v.id, v.gridId, v.tplId, v.level, v.state, v.param1, v.param2, v.param3, utils.serialize(jobList))
    end
    dict.set('building.list', blist)

    dict.set('building.info', {
        maxId = building.maxId,
        forestList = building.forestList,
        builder1 = building.builder1,
        builder2 = building.builder2,
        builder2ExpireTime = building.builder2ExpireTime,
    })

    dict.set('building.tax',{
        taxFreeCount = buildingTax.taxFreeCount,
        taxFreeExpireTime = buildingTax.taxFreeExpireTime,
        taxGoldCount = buildingTax.taxGoldCount,
        freeTaxList = buildingTax.freeTaxList,
    })
    -- print(building.maxId,utils.serialize(building.forestList), building.builder1, building.builder2, building.builder2ExpireTime)
end

function building.onSave()
    building.dbSave()
end

function building.logBuilding(tplId, level)
    local tpl = t.building[tplId]
    if tpl and level > 1
        and tpl.buildingType ~= p.BuildingType.FARM
        and tpl.buildingType ~= p.BuildingType.SAWMILL
        and tpl.buildingType ~= p.BuildingType.STONE_MINE
        and tpl.buildingType ~= p.BuildingType.IRON_MINE then
        logStub.appendBuildingUpgrade(user.uid, tpl.buildingType, level - 1, level, timer.getTimestampCache())
    end
end

function building.hasBuilder(seconds)
    -- print('###building.hasBuilder..building.builder1, building.builder2', building.builder1, building.builder2)
    if building.builder1 == 0 then
        return true
    end
    if building.builder2 == 0 and building.builder2ExpireTime > timer.getTimestampCache() then
        local leftSeconds = building.builder2ExpireTime - timer.getTimestampCache()
        if leftSeconds >= seconds then
            return true
        end
    end
    return false
end

function building.useBuilder(cdId)
    if building.builder1 == 0 and cdId ~= nil then
        building.builder1 = cdId
        --print('useBuilder 1')
        return true
    end
    if building.builder2 == 0 and building.builder2ExpireTime > timer.getTimestampCache() and cdId ~= nil then
        building.builder2 = cdId
        --print('useBuilder 2')
        return true
    end
    return false
end

function building.finishBuilder(cdId)
    if building.builder1 == cdId then building.builder1 = 0 end
    if building.builder2 == cdId then building.builder2 = 0 end
end

function building.castleLevel()
    local castle = building.list[p.BuildingType.CASTLE]
    return castle.level
end

function building.getBuildingNum(buildingType)
    local num = 0
    for _, v in pairs(building.list) do
        local tpl = t.building[v.tplId]
        if tpl ~= nil and tpl.buildingType == buildingType and v.state ~= p.BuildingState.DAMAGED then
            num  = num + 1
        end
    end
    return num
end

function building.getBuildingLevel(tplId)
    local level = 0
    for _, v in pairs(building.list) do
        if v.tplId == tplId and level < v.level then
            level  = v.level
        end
    end
    return level
end

function building.getBuildingLevelByType(buildingType)
    local level = 0
    for _, v in pairs(building.list) do
        local tpl = t.building[v.tplId]
        if tpl ~= nil and tpl.buildingType == buildingType and level < v.level then
            level  = v.level
        end
    end
    return level
end

function building.getBuildingMaxLevel(tplId)
    local tpl = t.building[tplId]
    if not tpl then
        -- print('###building.getBuildingMaxLevel..tpl_building no this tpl...tplId', tplId)
        return 0
    end
    return tpl.maxLevel or 0
end

function building.getBuildingById(buildingId)
    for _, v in pairs(building.list) do
        if v.id == buildingId then
            return v
        end
    end
    return nil
end

function building.setBuildingStateByGridId(gridId)
    local state = p.BuildingState.NORMAL
    local target = building.list[gridId]
    if target then
        local tpl = t.building[target.tplId]
        if tpl then
            if tpl.buildingType == p.BuildingType.BARRACKS
                or tpl.buildingType == p.BuildingType.WALLS
                or tpl.buildingType == p.BuildingType.INFANTRY_BARRACKS
                or tpl.buildingType == p.BuildingType.RIDER_BARRACKS
                or tpl.buildingType == p.BuildingType.ARCHER_BARRACKS 
                or tpl.buildingType == p.BuildingType.MECHANICAL_BARRACKS then
                for _, v in pairs(target.jobList) do
                    if v.result == 1 then
                        state = p.BuildingState.TRAINING
                    elseif v.result == 2 then
                        state = p.BuildingState.TRAIN_FINISH
                        break
                    end
                end
            elseif tpl.buildingType == p.BuildingType.COLLEGE then
                for _, v in pairs(target.jobList) do
                    if v.result == 1 then
                        state = p.BuildingState.RESEARCHING
                        break
                    end
                end
            elseif tpl.buildingType == p.BuildingType.HOSPITAL then
                for _, v in pairs(target.jobList) do
                    if v.result == 1 then
                        state = p.BuildingState.CARING
                    end
                end
            end
        end
    end
    -- print('###building.setBuildingStateByGridId...gridId, state', gridId, state)
    return state
end

function building.checkRemoveResource(time, food, wood, iron, stone, coin, consumeType, items)
    -- print('###building.checkRemoveResource..time, food, wood, iron, stone, san', time, food, wood, iron, stone, san)
    if type(items) ~= 'table' then items = {} end
    local food_, wood_, iron_, stone_, coin_, items_ =  food, wood, iron, stone, coin, {}
    for tplid, count in pairs(items) do
        items_[tplid] = count
    end
    local lackFood, lackWood, lackIron, lackStone =  0, 0, 0, 0
    if food_ > user.info.food then
        lackFood = food_ - user.info.food
        food_ = user.info.food
    end
    if wood_ > user.info.wood then
        lackWood = wood_ - user.info.wood
        wood_ = user.info.wood
    end
    if iron_ > user.info.iron then
        lackIron = iron_ - user.info.iron
        iron_ = user.info.iron
    end
    if stone_ > user.info.stone then
        lackStone = stone_ - user.info.stone
        stone_ = user.info.stone
    end
    if coin_ > 0 and coin_ > user.info.silver then
        return false, p.ErrorCode.PUBLIC_SILVER_NOT_ENOUGH
    end
    local needGold = t.timeToGold(time) + t.resourceToGold(lackFood, lackWood, lackIron, lackStone)
    for tplid, count in pairs(items_) do
        local isEnough, lackCount, lackGold = bag.checkItemEnough(tplid, count)
        if not isEnough and lackCount > 0 and lackGold > 0 then
            needGold = needGold + lackGold
            local removeCount = count - lackCount
            if removeCount > 0 then
                items_[tplid] = removeCount
            else
                items_[tplid] = nil
            end
        end
    end
    if needGold > 0 and not user.removeResource(p.ResourceType.GOLD, needGold, consumeType) then
        return false, p.ErrorCode.PUBLIC_GOLD_NOT_ENOUGH
    end
    user.removeResource(p.ResourceType.FOOD, food_, consumeType)
    user.removeResource(p.ResourceType.WOOD, wood_, consumeType)
    user.removeResource(p.ResourceType.IRON, iron_, consumeType)
    user.removeResource(p.ResourceType.STONE, stone_, consumeType)
    user.removeResource(p.ResourceType.SILVER, coin_, consumeType)
    bag.removeItems(items_, consumeType)
    return true, p.ErrorCode.SUCCESS
end

function building.checkBuildingCase(levelTpl)
    if levelTpl.case1 and levelTpl.case1[2] > building.getBuildingLevel(levelTpl.case1[1]) then
        -- print('need building ' .. levelTpl.case1[1] .. ' level to '  ..  levelTpl.case1[2])
        return false
    end
    if levelTpl.case2 and levelTpl.case2[2] > building.getBuildingLevel(levelTpl.case2[1]) then
        -- print('need building ' .. levelTpl.case2[1] .. ' level to '  ..  levelTpl.case2[2])
        return false
    end
    if levelTpl.case3 and levelTpl.case3[2] > building.getBuildingLevel(levelTpl.case3[1]) then
        -- print('need building ' .. levelTpl.case3[1] .. ' level to '  ..  levelTpl.case3[2])
        return false
    end
    if levelTpl.case4 and levelTpl.case4[2] > building.getBuildingLevel(levelTpl.case4[1]) then
        -- print('need building ' .. levelTpl.case4[1] .. ' level to '  ..  levelTpl.case4[2])
        return false
    end
    if levelTpl.case5 and levelTpl.case5[2] > building.getBuildingLevel(levelTpl.case5[1]) then
        -- print('need building ' .. levelTpl.case5[1] .. ' level to '  ..  levelTpl.case5[2])
        return false
    end
    if levelTpl.case6 and levelTpl.case6[2] > building.getBuildingLevel(levelTpl.case6[1]) then
        -- print('need building ' .. levelTpl.case6[1] .. ' level to '  ..  levelTpl.case6[2])
        return false
    end
    return true
end

function building.buildingStateClose(id, oldState, cdId)
    if oldState == p.BuildingState.BUILDING
        or oldState == p.BuildingState.UPGRADING
        or oldState == p.BuildingState.RESEARCHING
        or oldState == p.BuildingState.CARING then
        building.evtBuildingStateClose:trigger(id, cdId)
    end
end

function building.cdlistCallback(cdId, flag)
    --print('cdId,flag',cdId, flag)
    for _, v in pairs(building.list) do
        local job = v.jobList[cdId]
        -- if job and job.cdId == cdId  then
        --     print("###job.cdId, state, cdId", job.cdId, v.state, cdId)
        -- end
        if job and job.cdId == cdId and v.state ~= p.BuildingState.NORMAL then
            if v.state == p.BuildingState.BUILDING then -- 创建中
                v.level = 1
                building.finishBuilder(cdId)
                local tpl = t.building[v.tplId]
                if tpl and tpl.levels[v.level] then
                    user.addExp(tpl.levels[v.level].exp)
                end
                local buildingType = v:getBuildingType()
                if buildingType == p.BuildingType.COLLEGE then
                    tech.checkUnlockTechnologyTree(v.level)
                end
                v:onBuildFinish()
            elseif v.state == p.BuildingState.UPGRADING then -- 升级中
                v.level = v.level + 1
                building.finishBuilder(cdId)
                local tpl = t.building[v.tplId]
                if tpl and tpl.levels[v.level] then
                    user.addExp(tpl.levels[v.level].exp)
                end
                local buildingType = v:getBuildingType()
                if buildingType == p.BuildingType.COLLEGE then --书院升级时 检查科技树解锁
                    tech.checkUnlockTechnologyTree(v.level)
                end
                v:onBuildFinish()
            elseif v.state == p.BuildingState.DEMOLISHING then -- 拆除中
                table.insert(building.removeList, v.id)
                building.finishBuilder(cdId)
                building.list[v.gridId] = buildingInfo:new({ gridId = v.gridId, tplId = 0, sync = true })
                building.evtBuildingDemolish:trigger(v.tplId, v.level)
            elseif v.state == p.BuildingState.RESEARCHING then -- 研究中
                local techId = job.flagId
                local tpl = t.technology[techId]
                local groupId = tpl.group
                local treeType = tpl.type
                tech.addLevel(treeType,groupId)
                tech.evtResearchFinished:trigger(v.p1)
                v.jobList[cdId].cdId = 0
                v.jobList[cdId].result = 3
                v.jobList[cdId].isSync = false
            elseif v.state == p.BuildingState.TRAINING or v.state == p.BuildingState.TRAIN_FINISH then -- 训练中
                v.jobList[cdId].result = 2
                v.jobList[cdId].isSync = false
            elseif v.state == p.BuildingState.CARING then -- 治疗中
                -- 治疗时间一结束，把队列清除
                local totalCount = 0
                for k,v in pairs(v.jobList[cdId].p1) do
                    army.add(k, p.ArmyState.NORMAL, v, p.ArmyGainType.WOUNDED)
                    totalCount = totalCount + v
                end
                building.evtBuildingHeal:trigger(totalCount)
                v.jobList[cdId].cdId = 0
                v.jobList[cdId].result = 3
                v.jobList[cdId].isSync = false
                building.sendBuildingQueuesUpdate()
                -- newState = building.setBuildingStateByGridId(gridId)
            end
            building.buildingStateClose(v.id, v.state, cdId)
            if v.state ~= p.BuildingState.TRAINING
                and v.state ~= p.BuildingState.TRAIN_FINISH
                and v.state ~= p.BuildingState.RESEARCHING then
                v.jobList[cdId] = nil
            end
            v.state = building.setBuildingStateByGridId(v.gridId)
            v.sync = false
        end
    end
    if flag == nil or flag ~= 'self' then
        building.sendBuildingUpdate()
        building.sendBuildingQueuesUpdate()
    end
end

function building.onCombatOver(battleId, battleType, isWin, dataOnCombatOver,result)
    if battleType == p.BattleType.OPEN_FOREST then
         print('###building.onCombatOver...battleId, battleType, isWin, dataOnCombatOver', battleId, battleType, isWin, utils.serialize(dataOnCombatOver))
        local forestId = dataOnCombatOver.forestId
        if forestId then
            local tpl = t.forest[forestId]
            if tpl and not building.forestList[forestId] then
                if isWin then
                    user.removeResource(p.ResourceType.FOOD, tpl.food)
                    user.removeResource(p.ResourceType.WOOD, tpl.wood)

                    building.forestList[forestId] = 1
                    for i = tpl.startGridId, tpl.endGridId do
                        building.list[i] =  buildingInfo:new({ gridId = i, tplId = 0, sync = true })
                    end
                    -- print("building.list..", utils.serialize(building.list))
                    building.sendBuildingUpdate()
                    building.evtBuildingOpenForest:trigger()
                end
            end
        end
    end
end

--解锁条件
function building.checkUnlockCond(list)
    -- print('building.checkUnlockCond...list', utils.serialize(list))
    return agent.systemUnlock.checkUnlockedByList(list)
end

function building.sendBuildingUpdate()
    -- for _, v in pairs(building.list) do
    --     print('###building.sendBuildingUpdate..', utils.serialize(v))
    -- end
    local updateList = {}
    for _, v in pairs(building.list) do
        if not v.sync then
            v.sync = true
            --找时间最短的CD
            local cdList = {}
            local cdId = 0
            local remainTime = 0
            for _, v2 in pairs(v.jobList) do
                local cd = cdlist.getCD(v2.cdId)
                if cd then
                    local leftTime = cd:getRemainTime()
                    if remainTime == 0 then
                        remainTime = leftTime
                    end
                    if leftTime > 0 and leftTime <= remainTime then
                        cdId = v2.cdId
                        remainTime = leftTime
                    end
                end
            end
            if cdId > 0 then
                table.insert(cdList, { cdId = cdId })
            end
            --资源建筑
            --p1已采集的时间(秒), p2双倍加成已采集时间(秒), p3双倍加成剩余时间(秒), p4双倍加成的总时长
            local p1, p2, p3, p4 = 0, 0, 0, 0
            if v:isCollectBuilding() then
                local now = timer.getTimestampCache()
                p1 = now - v.param1
                if v.param1 < v.param2 then
                    if now >= v.param3 then
                        p2 = v.param3 - v.param2
                        p3 = 0
                    else
                        p2 = now - v.param2
                        p3 = v.param3 - now
                    end
                elseif v.param1 > v.param3 then
                    p2 = 0
                    p3 = 0
                else
                    if now >= v.param3 then
                        p2 = v.param3 - v.param1
                        p3 = 0
                    else
                        p2 = now - v.param1
                        p3 = v.param3 - now
                    end
                end
                if p3 > 0 then
                    p4 = v.param3 - v.param2
                end
            end

            local maxLevel = building.getBuildingMaxLevel(v.tplId)
            table.insert(updateList, { id = v.id, gridId = v.gridId, tplId = v.tplId, level = v.level, maxLevel = maxLevel, state = v.state, cdList = cdList, param1 = p1, param2 = p2, param3 = p3, param4 = p4 })
             -- print('sendBuildingUpdate id, gridId, tplId, level, maxLevel, state, cdList, p1, p2, p3, p4 = ' , v.id, v.gridId, v.tplId, v.level, maxLevel, v.state, utils.serialize(cdList), p1, p2, p3, p4)
        end
    end

    local removeList = {}
    for _, v in pairs(building.removeList) do
        table.insert(removeList, { id = v })
         --print('sendBuildingUpdate remove building id=', v)
    end
    building.removeList = {}

    local forestList = {}
    for k, v in pairs(building.forestList) do
        table.insert(forestList, { forestId = k })
    end
    -- print('sendBuildingUpdate..forestList..', utils.serialize(forestList))

    local builder2LeftSeconds = building.builder2ExpireTime - timer.getTimestampCache()
    if builder2LeftSeconds < 0 then builder2LeftSeconds = 0 end

    agent.sendPktout(p.SC_BUILDING_UPDATE,
    '@@1=[id=i,gridId=i,tplId=i,level=i,maxLevel=i,state=i,cdList=[cdId=i],param1=i,param2=i,param3=i,param4=i],2=[id=i],3=[forestId=i],4=i,5=i,6=i',
    updateList, removeList, forestList, building.builder1, building.builder2, builder2LeftSeconds)
end


function building.sendBuildingQueuesUpdate()
    local temp = {
        [p.BuildingType.COLLEGE] = property.list.technologyQueues,
        [p.BuildingType.BARRACKS] = property.list.barrackTrainQueues,
        [p.BuildingType.INFANTRY_BARRACKS] = property.list.infantryBarrackTrainQueues,
        [p.BuildingType.RIDER_BARRACKS] = property.list.riderBarrackTrainQueues,
        [p.BuildingType.ARCHER_BARRACKS] = property.list.archerBarrackTrainQueues,
        [p.BuildingType.MECHANICAL_BARRACKS] = property.list.mechanicalBarrackTrainQueues,
        [p.BuildingType.HOSPITAL] = property.list.healQueueNum,
        [p.BuildingType.WALLS] = 1,
    }

    local list = {}
    for buildingType, count in pairs(temp) do
        local info = building.list[buildingType]
        -- print('----------------------------------',utils.serialize(info))
        if info and info.jobList then
            local updateList = {}
            local removeList = {}
            for k, v in pairs(info.jobList) do
                if not v.isSync then
                    v.isSync = true
                    if v.result == 3 then
                        table.insert(removeList, { flagId = v.flagId })
                        info.jobList[k] = nil
                    elseif v.result > 0 then
                        local cdId = v.cdId
                        if v.result == 2 then
                            cdId = 0
                        end
                        local param1 = 0
                        if type(v.p1) == "number" then
                            param1 = v.p1
                        end
                        local param2 = 0
                        if type(v.p2) == "number" then
                            param2 = v.p2
                        end
                        table.insert(updateList, { cdId = cdId, flagId = v.flagId, param1 = param1, param2 = param2 })
                    end
                end
            end
            table.insert(list, { gridId = info.gridId, count = count, updateList = updateList, removeList = removeList })
        end
    end
    print('###building.sendBuildingQueuesUpdate...list', utils.serialize(list))
    agent.sendPktout(p.SC_BUILDING_QUEUES_UPDATE, '@@1=[gridId=i,count=i,updateList=[cdId=i,flagId=i,param1=i,param2=i],removeList=[flagId=i]]', list)
end

function building.sendTaxUpdate()
    local now = timer.getTimestampCache()
    local leftTime = buildingTax.taxFreeExpireTime - now
    if leftTime < 0 then
        leftTime = 0
    end

    agent.sendPktout(p.SC_TAX_UPDATE, '@@1=i,2=i,3=i,4=[taxSilver=i,timestamp=i]',buildingTax.taxFreeCount,leftTime,buildingTax.taxGoldCount,buildingTax.freeTaxList)
end

function building.refreshTax()
    buildingTax.taxFreeCount = 0
    buildingTax.taxFreeExpireTime = 0
    buildingTax.taxGoldCount = 0
    buildingTax.freeTaxList = {}
    building.sendTaxUpdate()
end

-- create
pktHandlers[p.CS_BUILDING_CREATE] = function(pktin, session)
    local function createResponse(result)
        -- print('createResponse', result)
        agent.replyPktout(session, p.SC_BUILDING_CREATE_RESPONSE, result)
    end

    local gridId, tplId, buildNow = pktin:read('iib')
    -- print('###p.CS_BUILDING_CREATE..gridId, tplId, buildNow', gridId, tplId, buildNow)

    -- 1.检查模板表是否有该建筑
    local tpl = t.building[tplId]
    if not tpl then
        print('not exist tplId = ', tplId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local levelTpl = tpl.levels[1]   -- level 1 tpl
    if not levelTpl then
        print('###p.CS_BUILDING_CREATE, no one level building, check tpl_building_level..tplId ', tplId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    -- 2.检查拥有的建筑格子 看数据是否对上
    if not building.list[gridId]
        or (tpl.isInside == true and building.list[gridId].tplId ~= tplId)
        or (tpl.isInside == false and building.list[gridId].tplId ~= 0) then
        print('###p.CS_BUILDING_CREATE, need empty gridId= ', gridId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    -- 3.城内，城外 格子的范围
    if tpl.isInside == true and (gridId < 1 or gridId > 99) then
        print('###p.CS_BUILDING_CREATE, invalid gridId = ', gridId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    if tpl.isInside == false and (gridId < 100 or gridId > 124) then
        print('###p.CS_BUILDING_CREATE, invalid gridId = ', gridId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    -- 4.检查该建筑可以建的最大数量
    if building.getBuildingNum(tpl.buildingType) >= tpl.maxNum then
        print('###p.CS_BUILDING_CREATE, max building num = ', tpl.maxNum)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    -- 5.检查建造或修补该建筑 需要的主城等级
    local castle = building.list[p.BuildingType.CASTLE]
    if tpl.unlockLevel > castle.level then
        print('###p.CS_BUILDING_CREATE, need castle.level >= ',tpl.unlockLevel)
        agent.sendNoticeMessage(p.ErrorCode.CASTLE_LEVEL_NOT_ENOUGH, '', 1)
        return
    end
    -- 7.检查升级建造队列
    local time = math.ceil(levelTpl.buildTime / (1 + property.list.buildingSpeedPct))
    -- print('###p.CS_BUILDING_CREATE...buildTime, buildingSpeedPct, needTime', levelTpl.buildTime, property.list.buildingSpeedPct, time)
    if not buildNow and not building.hasBuilder(time) then
        print('###p.CS_BUILDING_CREATE, no builder ' )
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --8.检查建造或修补该建筑是否满足条件需求 TODO:可能整合到系统解锁
    if not building.checkBuildingCase(levelTpl) then
         print('###p.CS_BUILDING_CREATE, checkBuildingCase  is not satisfied ' )
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    if buildNow == true then
        --9.1检查资源是否足够
        -- print("###p.CS_BUILDING_CREATE, time, levelTpl.food, levelTpl.wood, levelTpl.iron, levelTpl.stone", time, levelTpl.food, levelTpl.wood, levelTpl.iron, levelTpl.stone)
        local isEnough, ret = building.checkRemoveResource(time, levelTpl.food, levelTpl.wood, levelTpl.iron, levelTpl.stone, 0, p.ResourceConsumeType.BUILDING_CREATE, levelTpl.case7)
        if not isEnough then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        building.list[gridId] = buildingInfo:new({ gridId = gridId, tplId = tplId, level = 1, jobList = {},  state = p.BuildingState.NORMAL })
        building.list[gridId]:onBuildFinish()
        user.addExp(levelTpl.exp)

        if tpl.buildingType == p.BuildingType.COLLEGE then
            tech.checkUnlockTechnologyTree(1)
        end
        building.sendBuildingQueuesUpdate()
    else
        --9.2检查资源是否足够
        local isEnough, ret = building.checkRemoveResource(0, levelTpl.food, levelTpl.wood, levelTpl.iron, levelTpl.stone, 0, p.ResourceConsumeType.BUILDING_CREATE, levelTpl.case7)
        if not isEnough then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        local cdId = cdlist.addCD(p.ItemPropType.BUILDING_SPEEDUP, time, building.cdlistCallback)
        building.useBuilder(cdId)

        local jinfo = jobInfo:new({ cdId = cdId })
        if building.list[gridId] then
            building.list[gridId].level = 1
            building.list[gridId].state = p.BuildingState.BUILDING
            if not building.list[gridId].jobList then
                building.list[gridId].jobList = {}
            end
            building.list[gridId].jobList[cdId] = jinfo
            building.list[gridId].sync = false
        else
            local binfo = buildingInfo:new({ gridId = gridId, tplId = tplId, level = 1, state = p.BuildingState.BUILDING })
            binfo.jobList[cdId] = jinfo
            building.list[gridId] = binfo
        end
    end

    building.sendBuildingUpdate()
    createResponse(true)
end

-- upgrade
pktHandlers[p.CS_BUILDING_UPGRADE] = function(pktin, session)
    local function upgradeResponse(result)
        -- print('upgradeResponse', result)
        agent.replyPktout(session, p.SC_BUILDING_UPGRADE_RESPONSE, result)
    end
    local gridId, upgradeNow = pktin:read('ib')
    -- print('###p.CS_BUILDING_UPGRADE...gridId, upgradeNow', gridId, upgradeNow)

    --1.检查是否拥有该建筑
    local target = building.list[gridId]
    if not target then
        print('###p.CS_BUILDING_UPGRADE, invalid gridId = ', gridId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --2.检查该建筑当前状态 是否符合升级
    if target.state ~= p.BuildingState.NORMAL then
        print('###p.CS_BUILDING_UPGRADE, wrong building state = ', target.state)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --3.检查模板表是否有该建筑
    local tpl = t.building[target.tplId]
    if not tpl then
        print('###p.CS_BUILDING_UPGRADE, tpl not exists  tplId= ', target.tplId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    if target.level >= tpl.maxLevel then
        print('###p.CS_BUILDING_UPGRADE, max building level = ', tpl.maxLevel)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --4.检查升级该建筑 需要的主城等级
    local castle = building.list[p.BuildingType.CASTLE]
    if tpl.unlockLevel > castle.level then
        print('###p.CS_BUILDING_UPGRADE, need castle.level >= ', tpl.unlockLevel)
        agent.sendNoticeMessage(p.ErrorCode.CASTLE_LEVEL_NOT_ENOUGH, '', 1)
        return
    end
    --5.检查模板表是否有当前等级建筑
    local currentTpl = tpl.levels[target.level]
    if not currentTpl then
        print('###p.CS_BUILDING_UPGRADE, not currentTpl' )
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --6.检查模板表是否有下一等级建筑
    local nextTpl = tpl.levels[target.level + 1]
    if not nextTpl then
        print('###p.CS_BUILDING_UPGRADE, tpl_building_level not this tpl....tplId,level = ', tplId, target.level+1)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --7.检查升级建造队列
    local time = math.ceil(nextTpl.buildTime / (1 + property.list.buildingSpeedPct))
    -- print('###p.CS_BUILDING_UPGRADE...buildTime, buildingSpeedPct, needTime', nextTpl.buildTime, property.list.buildingSpeedPct, time)
    if not upgradeNow and not building.hasBuilder(time) then
        print('###p.CS_BUILDING_UPGRADE, no builder ' )
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --8.检查升级该建筑是否满足条件需求 TODO:可能整合到系统解锁
    if not building.checkBuildingCase(nextTpl) then
        print('###p.CS_BUILDING_UPGRADE, upgade condition is is not satisfied')
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local tplBuilding = t.building[target.tplId]
    if upgradeNow == true then
        --9.1检查资源是否足够
        local isEnough, ret = building.checkRemoveResource(time, nextTpl.food, nextTpl.wood, nextTpl.iron, nextTpl.stone, 0, p.ResourceConsumeType.BUILDING_UPGRADE, nextTpl.case7)
        if not isEnough then
            -- print('###p.CS_BUILDING_UPGRADE, upgade resource  is not enough..upgradeNow', upgradeNow)
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        target.level = target.level + 1
        if tplBuilding.buildingType == p.BuildingType.COLLEGE then
            tech.checkUnlockTechnologyTree(target.level)
        end
        target.sync = false
        target:onBuildFinish()
        user.addExp(nextTpl.exp)
        building.sendBuildingQueuesUpdate()
        building.evtBuildingSpeedUp:trigger()
    else
        --9.2检查资源是否足够
        local isEnough, ret = building.checkRemoveResource(0, nextTpl.food, nextTpl.wood, nextTpl.iron, nextTpl.stone, 0, p.ResourceConsumeType.BUILDING_UPGRADE, nextTpl.case7)
        if not isEnough then
            -- print('###p.CS_BUILDING_UPGRADE, upgade resource  is not enough..upgradeNow',upgradeNow)
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        local cdId = cdlist.addCD(p.ItemPropType.BUILDING_SPEEDUP, time, building.cdlistCallback)
        local jinfo = jobInfo:new({ cdId = cdId })
        target.jobList[cdId] = jinfo
        building.useBuilder(cdId)
        target.state = p.BuildingState.UPGRADING
        target.sync = false
        -- push
        local sendTime = timer.getTimestampCache() + time
        pushStub.appendPush(user.uid, p.PushClassify.BUILDING_UP, target.tplId, cdId, sendTime)
    end

    building.sendBuildingUpdate()
    upgradeResponse(true)
end

-- cancel upgrade
pktHandlers[p.CS_BUILDING_UPGRADE_CANCEL] = function(pktin, session)
    local function upgradeCancelResponse(result)
        print('upgradeCancelResponse', result)
        agent.replyPktout(session, p.SC_BUILDING_UPGRADE_CANCEL_RESPONSE, result)
    end

    local gridId = pktin:read('i')
    local target = building.list[gridId]
    if target == nil then
        print('###p.CS_BUILDING_UPGRADE_CANCEL, building not exist.. gridId = ', gridId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    if target.state ~= p.BuildingState.UPGRADING then
        print('###p.CS_BUILDING_UPGRADE_CANCEL, wrong building state = ', target.state)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    local tpl = t.building[target.tplId]
    if not tpl then
        print('###p.CS_BUILDING_UPGRADE_CANCEL, tpl_building not this tpl..tplId=', target.tplId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local nextTpl = tpl.levels[target.level + 1]
    if not nextTpl then
        print('###p.CS_BUILDING_UPGRADE_CANCEL, tpl_building_level not this tpl..level=', target.level+1)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local job = {}
    for _, v in pairs(target.jobList) do
        job = v
        break
    end
    -- print('###p.CS_BUILDING_UPGRADE_CANCEL, job=', utils.serialize(job))
    if not next(job) then
        print('###p.CS_BUILDING_UPGRADE_CANCEL, building jobList is empty....gridId', gridId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    -- return half resources
    local list = {}
    table.insert(list, { tplId = p.ResourceType.FOOD, count = nextTpl.food * cancelratio })
    table.insert(list, { tplId = p.ResourceType.WOOD, count = nextTpl.wood * cancelratio })
    table.insert(list, { tplId = p.ResourceType.STONE, count = nextTpl.stone * cancelratio })
    table.insert(list, { tplId = p.ResourceType.IRON, count = nextTpl.iron * cancelratio })
    user.addResources(list)
    -- push
    pushStub.cancelPush(user.uid, job.cdId)

    cdlist.cancelCD(job.cdId)
    building.buildingStateClose(target.id, target.state, job.cdId)
    building.finishBuilder(job.cdId)
    target.jobList[job.cdId] = nil
    target.state = p.BuildingState.NORMAL
    target.sync = false

    building.sendBuildingUpdate()
    upgradeCancelResponse(true)
end

-- train
pktHandlers[p.CS_BUILDING_TRAIN] = function(pktin, session)
    local function trainResponse(result)
        -- print('trainResponse', result)
        agent.replyPktout(session, p.SC_BUILDING_TRAIN_RESPONSE, result)
    end

    local gridId, queueIndex, armyType, count, trainNow = pktin:read("iiiib")
    --print('###p.CS_BUILDING_TRAIN...gridId, queueIndex, armyType, count, trainNow', gridId, queueIndex, armyType, count, trainNow)

    --0.检查训练兵种是否为配置表中的四个兵种
    if armyType ~= p.ArmysType.ROLLING_LOGS and armyType ~= p.ArmysType.GRIND_STONE and armyType ~= p.ArmysType.CHEVAL_DE_FRISE and armyType ~= p.ArmysType.KEROSENE 
        and armyType ~= p.ArmysType.INFANTRY and armyType ~= p.ArmysType.RIDER and armyType ~= p.ArmysType.ARCHER and armyType ~= p.ArmysType.MECHANICAL then
        print("###p.CS_BUILDING_TRAIN, armyType = ", armyType)
        --agent.sendNoticeMessage(p.ErrorCode.JUST_TRAIN_REDIF, '', 1)
        return
    end
    --1.检查训练士兵数量
    if count <= 0 then
        print("###p.CS_BUILDING_TRAIN, building train armies are empty, armyType, count = ", armyType, count)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --2.检查是否拥有这个建筑
    local target = building.list[gridId]
    if not target then
        print("###p.CS_BUILDING_TRAIN, building not exist, gridId = ", gridId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --3.检查模板表 是否有该建筑
    local tplBuilding = t.building[target.tplId]
    if tplBuilding == nil then
        print('###p.CS_BUILDING_TRAIN, building tpl not exists  tplId= ', target.tplId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local tpl = tplBuilding.levels[target.level]
    if not tpl then
        print("###p.CS_BUILDING_TRAIN, building level tpl not found")
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --4.检查tpl_army模板表 是否有该兵种.  消耗以后台兵种当前等级为主
    local level = army.getArmyLevelByArmyType(armyType)
    local tplArmy = t.findArmysTpl(armyType, level)
    if tplArmy == nil then
        print('###p.CS_BUILDING_TRAIN, army tpl not exists  armyType, level= ', armyType, level)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    -- 5.是否有该建筑
    if tplBuilding.buildingType == p.BuildingType.HOSPITAL then
    else
        if target.tplId ~= tplArmy.source then
            print('###p.CS_BUILDING_TRAIN, army tplId ~= tpl.source ', target.tplId, tplArmy.source)
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            return
        end
    end

    -- 6.检查训练军队数量上限
    if tplBuilding.buildingType == p.BuildingType.WALLS then
        local trainLimit = property.list.trapMakeNums
        if count > trainLimit then
            print("###p.CS_BUILDING_TRAIN, count > trainLimit", count, trainLimit)
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            return
        end
        local maxTrap = property.list.trapCapacity
        if count + army.getTrapsTotal() > maxTrap then
            print("count + army.getTrapsTotal > wall.maxTrap", count, army.getTrapsTotal(), maxTrap)
            agent.sendNoticeMessage(p.ErrorCode.BUILDING_TRAP_CAPACITY_NOT_ENOUGH, '', 1)
            return
        end
    elseif tplBuilding.buildingType == p.BuildingType.INFANTRY_BARRACKS then
        local trainLimit = property.list.infantryBarrackTrainNums
        if count > trainLimit then
            print('###p.CS_BUILDING_TRAIN, count > trainLimit', count, trainLimit)
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            return
        end
    elseif tplBuilding.buildingType == p.BuildingType.RIDER_BARRACKS then
        local trainLimit = property.list.riderBarrackTrainNums
        if count > trainLimit then
            print('###p.CS_BUILDING_TRAIN, count > trainLimit', count, trainLimit)
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            return
        end
    elseif tplBuilding.buildingType == p.BuildingType.ARCHER_BARRACKS then
        local trainLimit = property.list.archerBarrackTrainNums
        if count > trainLimit then
            print('###p.CS_BUILDING_TRAIN, count > trainLimit', count, trainLimit)
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            return
        end
    elseif tplBuilding.buildingType == p.BuildingType.MECHANICAL_BARRACKS then
        local trainLimit = property.list.mechanicalBarrackTrainNums
        if count > trainLimit then
            print('###p.CS_BUILDING_TRAIN, count > trainLimit', count, trainLimit)
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            return
        end                
    elseif tplBuilding.buildingType == p.BuildingType.HOSPITAL then
        local healNumLimit = property.list.healNum
        if count > healNumLimit then
            print("###p.CS_BUILDING_TRAIN, count > healNumLimit", count, healNumLimit)
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            return
        end
        -- 如果治疗数大于伤兵数，则直接返回
        local woundCount = army.getArmyCount(armyType, p.ArmyState.WOUNDED)
        if count > woundCount then
            print("###p.CS_BUILDING_HEAL, count > woundCount", count, woundCount)
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            return
        end
    end
    --7.消耗
    local wood, food, iron, stone = tplArmy.training_wood * count, tplArmy.training_food * count, tplArmy.training_iron * count, tplArmy.training_stone * count
    --8.训练时间
    local time = 0
    local trainMax, trainSpeed, cdType = 0, 0, 0
    if tplBuilding.buildingType == p.BuildingType.WALLS then
        trainMax = 1
        trainSpeed = property.list.trapTrainSpeedPct
        cdType = p.ItemPropType.TRAPS_BUILDING_SPEEDUP
        time = math.ceil(tplArmy.training_time * count / (1 + trainSpeed))

    elseif tplBuilding.buildingType == p.BuildingType.INFANTRY_BARRACKS then
        trainMax = property.list.infantryBarrackTrainQueues
        trainSpeed = property.list.infantryBarrackTrainSpeedPct
        cdType = p.ItemPropType.TROOP_TRAINING_SPEEDUP
        time = math.ceil(tplArmy.training_time * count / (1 + trainSpeed))
        wood = wood / (1 + property.list.reduceInfantryTrainConsumePct)
        food = food / (1 + property.list.reduceInfantryTrainConsumePct)
        iron = iron / (1 + property.list.reduceInfantryTrainConsumePct)
        stone = stone / (1 + property.list.reduceInfantryTrainConsumePct)

    elseif tplBuilding.buildingType == p.BuildingType.RIDER_BARRACKS then
        trainMax = property.list.riderBarrackTrainQueues
        trainSpeed = property.list.riderBarrackTrainSpeedPct
        cdType = p.ItemPropType.TROOP_TRAINING_SPEEDUP
        time = math.ceil(tplArmy.training_time * count / (1 + trainSpeed))
        wood = wood / (1 + property.list.reduceRiderTrainConsumePct)
        food = food / (1 + property.list.reduceRiderTrainConsumePct)
        iron = iron / (1 + property.list.reduceRiderTrainConsumePct)
        stone = stone / (1 + property.list.reduceRiderTrainConsumePct)

    elseif tplBuilding.buildingType == p.BuildingType.ARCHER_BARRACKS then
        trainMax = property.list.archerBarrackTrainQueues
        trainSpeed = property.list.archerBarrackTrainSpeedPct
        cdType = p.ItemPropType.TROOP_TRAINING_SPEEDUP
        time = math.ceil(tplArmy.training_time * count / (1 + trainSpeed))
        wood = wood / (1 + property.list.reduceArcherTrainConsumePct)
        food = food / (1 + property.list.reduceArcherTrainConsumePct)
        iron = iron / (1 + property.list.reduceArcherTrainConsumePct)
        stone = stone / (1 + property.list.reduceArcherTrainConsumePct)

    elseif tplBuilding.buildingType == p.BuildingType.MECHANICAL_BARRACKS then
        trainMax = property.list.mechanicalBarrackTrainQueues
        trainSpeed = property.list.mechanicalBarrackTrainSpeedPct
        cdType = p.ItemPropType.TROOP_TRAINING_SPEEDUP
        time = math.ceil(tplArmy.training_time * count / (1 + trainSpeed))
        wood = wood / (1 + property.list.reduceMechanicalTrainConsumePct)
        food = food / (1 + property.list.reduceMechanicalTrainConsumePct)
        iron = iron / (1 + property.list.reduceMechanicalTrainConsumePct)
        stone = stone / (1 + property.list.reduceMechanicalTrainConsumePct)
    
    elseif tplBuilding.buildingType == p.BuildingType.HOSPITAL then
        trainMax = property.list.healQueueNum
        trainSpeed = property.list.healSpeedPct
        cdType = p.ItemPropType.WOUNDED_RECOVERY_SPEEDUP
        time = math.ceil(tplArmy.heal_time * count / (1 + trainSpeed))
        local healratio = t.configure["healratio"] or 0.5
        wood = (wood * healratio)/(1 + property.list.healReduceResourcePct)
        food = (food * healratio)/(1 + property.list.healReduceResourcePct)
        iron = (iron * healratio)/(1 + property.list.healReduceResourcePct)
        stone = (stone * healratio)/(1 + property.list.healReduceResourcePct)
    end
    -- print('###p.CS_BUILDING_TRAIN, wood, food, iron, stone, time, trainSpeed',wood, food, iron, stone, time, trainSpeed)
    if trainNow then
        --9.1检查资源是否足够
        local isEnough, ret = building.checkRemoveResource(time, food, wood, iron, stone, 0, p.ResourceConsumeType.TRAINNING)
        if not isEnough then
            agent.sendNoticeMessage(ACTIVITY_RESOURCE_NOT_ENOUGH,'', 1)
            return
        end
        if tplBuilding.buildingType == p.BuildingType.HOSPITAL then
            local healList = {}
            healList[armyType] = count
            army.heal(healList)    
            -- 部队治疗
            building.evtBuildingHeal:trigger(count)
        else
            army.add(armyType, p.ArmyState.NORMAL, count, p.ArmyGainType.TRAIN)
        end
        --通知前端
        local list = {}
        table.insert(list, { param1 = armyType, param2 = count })
        agent.replyPktout(session, p.SC_BUILDING_COLLECT_RESPONSE, '@@1=i,2=i,3=[param1=i,param2=i]', 1, gridId, list)
        army.evtTrainFinished:trigger(armyType, count)
        print("armyType --------##1  ", armyType, " ---- count --------- ", count)
        building.evtBuildingTrain:trigger(armyType, count)
    else
        --9.2检查训练队列数量
        local trainCount = 0
        local isUse = false
        for _, v in pairs(target.jobList) do
            trainCount = trainCount + 1
            if queueIndex == v.flagId then
                isUse = true
                break
            end
        end
        -- print('###p.CS_BUILDING_TRAIN...trainCount, trainMax', trainCount, trainMax)
        if trainCount > trainMax or isUse then
            print("###p.CS_BUILDING_TRAIN, building train queues have no empty queue or is used... trainCount, trainMax, queueIndex, isUse", trainCount, trainMax, queueIndex, isUse)
            agent.sendNoticeMessage(p.ErrorCode.TRAIN_QUEUES_NOT_UNLOCKED, '', 1)
            return
        end
        --9.3检查资源是否足够
        local isEnough, ret = building.checkRemoveResource(0, food, wood, iron, stone, 0, p.ResourceConsumeType.TRAINNING)
        if not isEnough then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        if target.state ~= p.BuildingState.TRAIN_FINISH then
            target.state = p.BuildingState.TRAINING
        end
        if tplBuilding.buildingType == p.BuildingType.HOSPITAL then
            target.state = p.BuildingState.CARING
        end
        local cdId = cdlist.addCD(cdType, time, building.cdlistCallback)
        local jinfo = jobInfo:new({ cdId = cdId, flagId = queueIndex, p1 = armyType, p2 = count, p3 = level, result = 1, isSync = false })
        target.jobList[cdId] = jinfo
        target.sync = false

        --push
        local sendTime = timer.getTimestampCache() + time
        if tplBuilding.buildingType == p.BuildingType.WALLS then
            pushStub.appendPush(user.uid, p.PushClassify.TRAP_BUILDED, armyType, cdId, sendTime)
            building.evtBuildingTrain:trigger(armyType, count)
        elseif tplBuilding.buildingType == tplBuilding.INFANTRY_BARRACKS or tplBuilding.buildingType == p.BuildingType.RIDER_BARRACKS 
            or tplBuilding.buildingType == p.BuildingType.ARCHER_BARRACKS or tplBuilding.buildingType == p.BuildingType.MECHANICAL_BARRACKS then
            pushStub.appendPush(user.uid, p.PushClassify.ARMY_TRAINED, armyType, cdId, sendTime)
            print("armyType --------##2  ", armyType, " ---- count --------- ", count)
            building.evtBuildingTrain:trigger(armyType, count)
        elseif tplBuilding.buildingType == p.BuildingType.HOSPITAL then
            army.sub(armyType, state, count, armyConsumeType)
            pushStub.appendPush(user.uid, p.PushClassify.ARMY_HEALED, armyType, cdId, sendTime)
            building.evtBuildingHeal:trigger(count)
        end
        if tplBuilding.buildingType == p.BuildingType.HOSPITAL then
            local list = {}
            list[armyType] = count
            building.evtBuildingHeal:trigger(count)
            army.SubWounded(list)
        end
        building.sendBuildingUpdate()
        building.sendBuildingQueuesUpdate()
    end
    army.evtTraining:trigger(armyType, count)
    -- print('###p.CS_BUILDING_TRAIN....',utils.serialize(building.list))
    trainResponse(true)
end

-- cancel train
pktHandlers[p.CS_BUILDING_TRAIN_CANCEL] = function(pktin, session)
    local function trainCancelResponse(result)
        -- print('trainCancelResponse', result)
        agent.replyPktout(session, p.SC_BUILDING_TRAIN_CANCEL_RESPONSE, result)
    end

    local gridId, queueIndex = pktin:read('ii')

    local target = building.list[gridId]
    if not target then
        print('p.CS_BUILDING_TRAIN_CANCEL...building not exist, gridId = ', gridId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local job = {}
    for _, v in pairs(target.jobList) do
        if v.flagId == queueIndex and v.result == 1 then
            job = v
            break
        end
    end
    if not next(job) then
        print('###p.CS_BUILDING_TRAIN_CANCEL, no training queue...gricdId, queueIndex', gridId, queueIndex)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    local tplArmy = t.findArmysTpl(job.p1, job.p3)
    if tplArmy == nil then
        print('###p.CS_BUILDING_TRAIN_CANCEL, army tpl not exists  subType, level= ',job.p1, job.p3)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    local count = job.p2 or 0
    local wood = tplArmy.training_wood * count * cancelratio
    local food = tplArmy.training_food * count * cancelratio
    local iron = tplArmy.training_iron * count * cancelratio
    local stone = tplArmy.training_stone * count * cancelratio
    local tplBuilding = t.building[target.tplId]
    if tplBuilding.buildingType == p.BuildingType.HOSPITAL then
        local healratio = t.configure["healratio"] or 0.5
        wood = (wood * healratio)/(1 + property.list.healReduceResourcePct)
        food = (food * healratio)/(1 + property.list.healReduceResourcePct)
        iron = (iron * healratio)/(1 + property.list.healReduceResourcePct)
        stone = (stone * healratio)/(1 + property.list.healReduceResourcePct)
    end
    local list = {}
    table.insert(list, { tplId = p.ResourceType.FOOD, count = food })
    table.insert(list, { tplId = p.ResourceType.WOOD, count = wood })
    table.insert(list, { tplId = p.ResourceType.STONE, count = stone })
    table.insert(list, { tplId = p.ResourceType.IRON, count = iron })
    user.addResources(list)
    army.add(job.p1,p.ArmyState.WOUNDED,job.p2,p.ArmyGainType.WOUNDED)
    -- push
    pushStub.cancelPush(user.uid, job.cdId)

    cdlist.cancelCD(job.cdId)
    building.buildingStateClose(target.id, target.state, job.cdId)
    target.jobList[job.cdId].result = 3
    target.jobList[job.cdId].isSync = false
    building.sendBuildingQueuesUpdate()

    local newState = building.setBuildingStateByGridId(gridId)
    -- print('###p.CS_BUILDING_TRAIN_CANCEL,  newState=', newState)
    target.state = newState
    target.sync = false
    building.sendBuildingUpdate()
    trainCancelResponse(true)
end

-- collect
pktHandlers[p.CS_BUILDING_COLLECT] = function(pktin, session)
    local function buildingCollectResponse(result, gridId, list)
        -- print('buildingCollectResponse', result, gridId, list)
        if not list then
            list = {}
        end
        agent.replyPktout(session, p.SC_BUILDING_COLLECT_RESPONSE, '@@1=i,2=i,3=[param1=i,param2=i]', result, gridId, list)
    end
    --获取两段时间段的相交时间(秒)
    local function getCrossTime(a1, a2, b1, b2)
        local t1, t2 = 0, 0
        if a1 > b1 then t1 = a1 else t1 = b1 end
        if a2 < b2 then t2 = a2 else t2 = b2 end
        local t = t2 - t1
        return t > 0 and t or 0
    end

    local gridId,index = pktin:read('ii')
    -- print('###p.CS_BUILDING_COLLECT...', gridId,index)

    --1.检查是否拥有该建筑
    local target = building.list[gridId]
    if not target then
        print("###p.CS_BUILDING_COLLECT, building not exist, gridId = ", gridId)
        buildingCollectResponse(2, gridId)
        return
    end
    --2.检查模板表是否有该建筑
    local tpl = t.building[target.tplId]
    if tpl then
        --军营训练
        if target:isTrainBuilding() then
            if next(target.jobList) and target.state == p.BuildingState.TRAIN_FINISH then
                local newState = p.BuildingState.NORMAL
                local list = {}
                if index == 0 then
                    -- 一键收取
                    for _, v in pairs(target.jobList) do
                        if v.result == 2 and v.p1 ~= 0 and v.p2 ~= 0 then
                            army.add(v.p1, p.ArmyState.NORMAL, v.p2, p.ArmyGainType.TRAIN)
                            table.insert(list, { param1 = v.p1, param2 = v.p2 })
                            v.result = 3
                            v.cdId = 0
                            v.isSync = false
                            army.evtTrainFinished:trigger(v.p1, v.p2)
                        end
                    end
                else
                    -- 单个收取
                    local job = {}
                    local bHas = false
                    for _, v in pairs(target.jobList) do
                        if v.flagId == index then
                            if v.result == 2 and v.p1 ~= 0 and v.p2 ~= 0 then
                                army.add(v.p1, p.ArmyState.NORMAL, v.p2, p.ArmyGainType.TRAIN)
                                table.insert(list, { param1 = v.p1, param2 = v.p2 })
                                v.result = 3
                                v.cdId = 0
                                v.isSync = false
                                bHas = true
                                army.evtTrainFinished:trigger(v.p1, v.p2)
                                break
                            end
                        end
                    end
                    if not bHas then
                        buildingCollectResponse(2, gridId)
                        return
                    end
                end
                --必须在building.setBuildingStateByGridId(gridId)前面
                building.sendBuildingQueuesUpdate()

                newState = building.setBuildingStateByGridId(gridId)
                -- print('p.CS_BUILDING_COLLECT.......newState',newState)
                target.state = newState
                target.sync = false
                building.sendBuildingUpdate()
                buildingCollectResponse(1, gridId, list)
                return
            end
        elseif target:isCollectBuilding() then
        --资源收集
            local type = tpl.buildingType
            tpl = tpl.levels[target.level] -- tpl = t.buildingLevel[type][target.level]
            if tpl then
                local now = timer.getTimestampCache()
                local timeSpan = now - target.param1
                if timeSpan > 0 then
                    -- print("###p.CS_BUILDING_COLLECT, timeSpan = ", timeSpan)

                    local incomePerSecond, resourceType, capacity, incomePerHour = 0, 0, 0, 0
                    if type == p.BuildingType.FARM then
                        resourceType = p.ResourceType.FOOD
                        incomePerHour = property.list.foodOutPut
                        capacity = property.list.foodCapacity
                    elseif type == p.BuildingType.SAWMILL then
                        resourceType = p.ResourceType.WOOD
                        incomePerHour = property.list.woodOutPut
                        capacity = property.list.woodCapacity
                    elseif type == p.BuildingType.IRON_MINE then
                        resourceType = p.ResourceType.IRON
                        incomePerHour = property.list.ironOutPut
                        capacity = property.list.ironCapacity
                    elseif type == p.BuildingType.STONE_MINE then
                        resourceType = p.ResourceType.STONE
                        incomePerHour = property.list.stoneOutPut
                        capacity = property.list.stoneCapacity
                    else
                        print('###p.CS_BUILDING_COLLECT, wrong BuildingType = ', type)
                        buildingCollectResponse(2, gridId)
                        return
                    end
                    incomePerSecond = incomePerHour / 3600
                    -- print('###p.CS_BUILDING_COLLECT, buildingType, incomePerHour, incomePerSecond', type, incomePerHour, incomePerSecond)
                    --base
                    local produce = incomePerSecond * timeSpan
                    --boost
                    local boostTime = getCrossTime(target.param1, now, target.param2, target.param3)
                    if boostTime > 0 then
                        produce = produce + incomePerSecond * boostTime
                    end
                    capacity = math.floor(capacity)
                    produce = math.floor(produce)
                    -- print("###p.CS_BUILDING_COLLECT, capacity =", capacity, "produce =", produce)
                    if produce > 0 then
                        if produce > capacity then
                            produce = capacity
                        end
                        local list = {}
                        table.insert(list, { param1 = produce, param2 = 0 })

                        user.addResource(resourceType, produce, p.ResourceGainType.COLLECT)
                        target.param1 = now
                        target.sync = false
                        building.evtBuildingCollect:trigger()
                        building.evtResourceTax:trigger(resourceType, produce)

                        building.sendBuildingUpdate()
                        buildingCollectResponse(1, gridId, list)
                        return
                    end
                end
            end
        end
    end
    -- print('####p.CS_BUILDING_COLLECT...', utils.serialize(building.list[gridId]))
    buildingCollectResponse(2, gridId)
end

-- collect boost
pktHandlers[p.CS_BUILDING_COLLECT_BOOST] = function(pktin, session)
    local gridId, consumeType, bid, needCount = pktin:read('iiii')
    -- print('p.CS_BUILDING_COLLECT_BOOST..gridId, consumeType, bid, needCount', gridId, consumeType, bid, needCount)

    local result = false
    local info = building.list[gridId]
    if not info then
        print('###p.CS_BUILDING_COLLECT_BOOST, building not exist, gridId = ', gridId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local tpl = t.building[info.tplId]
    if not tpl then
        print('###p.CS_BUILDING_COLLECT_BOOST, tpl not found', info.tplId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    local base = 0
    local conf = t.configure["BasicGold"]
    if conf then
        local buildingType = tpl.buildingType
        if buildingType == p.BuildingType.FARM then
            base = conf.food
        elseif buildingType == p.BuildingType.SAWMILL then
            base = conf.wood
        elseif buildingType == p.BuildingType.IRON_MINE then
            base = conf.iron
        elseif buildingType == p.BuildingType.STONE_MINE then
            base = conf.stone
        end
    end
    if base == 0 then
        print('###p.CS_BUILDING_COLLECT_BOOST, base == 0')
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    local isAdd = false
    local now = timer.getTimestampCache()
    if info.param3 > now then
        isAdd = true
    end
    --0:道具提速 1:金币提速
    if consumeType == 0 then
        local info = bag.bag_items[bid]
        if info and needCount > 0 and info.count >= needCount then
            local itemId = info.tpl.id
            -- print('###p.CS_BUILDING_COLLECT_BOOST...itemId', itemId)
            local itemTpl = t.item[itemId]
            if itemTpl then
                local subType = itemTpl.subType
                local buildingType = tpl.buildingType
                if buildingType == p.BuildingType.FARM and subType == p.ItemPropType.FRAM_BOOST then
                    result = bag.removeItemByBid(bid, itemId, needCount, p.ResourceConsumeType.COLLECT_BOOST)
                elseif buildingType == p.BuildingType.SAWMILL and subType == p.ItemPropType.SAWMILL_BOOST then
                    result = bag.removeItemByBid(bid, itemId, needCount, p.ResourceConsumeType.COLLECT_BOOST)
                elseif buildingType == p.BuildingType.IRON_MINE and subType == p.ItemPropType.IRON_MINE_BOOST then
                    result = bag.removeItemByBid(bid, itemId, needCount, p.ResourceConsumeType.COLLECT_BOOST)
                elseif buildingType == p.BuildingType.STONE_MINE and subType == p.ItemPropType.STONE_MINE_BOOST then
                    result = bag.removeItemByBid(bid, itemId, needCount, p.ResourceConsumeType.COLLECT_BOOST)
                else
                    print('###p.CS_BUILDING_COLLECT_BOOST, buildingType or itemType wrong : buildingType, bid, itemId, subType', buildingType, bid, itemId, subType)
                end
            end
        end
        if not result then
            agent.sendNoticeMessage(p.ErrorCode.PUBLIC_PROP_NOT_ENOUGH, '', 1)
        end
    elseif consumeType == 1 then
        -- --有加速则不能用元宝加速
        -- if isAdd then
        --     print('###p.CS_BUILDING_COLLECT_BOOST, can not use gold to boost')
        --     agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        --     return
        -- end
        local gold = info.level * base
        if not user.removeResource(p.ResourceType.GOLD, gold, p.ResourceConsumeType.COLLECT_BOOST) then
            print('###p.CS_BUILDING_COLLECT_BOOST, gold is not enough', gold)
            agent.sendNoticeMessage(p.ErrorCode.PUBLIC_GOLD_NOT_ENOUGH, '', 1)
            return
        end
        needCount = 1
        result = true
    end
    if result then
        building.evtBuildingCollectBoost:trigger(tpl.buildingType)
        if not isAdd then
            info.param2 = now
            info.param3 = now + conf.time * needCount
        else
            info.param3 = info.param3 + conf.time * needCount
        end
        info.sync = false

        building.sendBuildingUpdate()
        agent.sendPktout(p.SC_BUILDING_COLLECT_BOOST_RESPONSE, true)
    end
end

-- demolish
pktHandlers[p.CS_BUILDING_DEMOLISH] = function(pktin, session)
    local function demolishResponse(result)
        -- print('demolishResponse', result)
        agent.replyPktout(session, p.SC_BUILDING_DEMOLISH_RESPONSE, result)
    end

    local gridId = pktin:read('i')
    -- print('###p.CS_BUILDING_DEMOLISH...gridId', gridId)
    if gridId > 124 or gridId < 100 then
        print('###p.CS_BUILDING_DEMOLISH, invalid gridId=',  gridId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local target = building.list[gridId]
    if target == nil or target.tplId == 0 then
        print('###p.CS_BUILDING_DEMOLISH, empty building gridId=', gridId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    if target.state ~= p.BuildingState.NORMAL then
        print('###p.CS_BUILDING_DEMOLISH, invalid state=',  target.state)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local tpl = t.building[target.tplId]
    if not tpl then
        print('###p.CS_BUILDING_DEMOLISH, tpl_building not this tpl..tplId=', target.tplId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local currentTpl = tpl.levels[target.level]
    if not currentTpl then
        print('###p.CS_BUILDING_DEMOLISH, tpl_building_level not this tpl..level=', target.level)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local needSeconds = currentTpl.buildTime / 2
    if not building.hasBuilder(needSeconds) then
        print('###p.CS_BUILDING_DEMOLISH, no builder')
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    local cdId = cdlist.addCD(p.ItemPropType.BUILDING_SPEEDUP, needSeconds, building.cdlistCallback)
    local jinfo = jobInfo:new({ cdId = cdId })
    target.jobList[cdId] = jinfo
    building.useBuilder(cdId)
    target.state = p.BuildingState.DEMOLISHING
    target.sync = false
    building.sendBuildingUpdate()

    -- push
    local sendTime = timer.getTimestampCache() + needSeconds
    pushStub.appendPush(user.uid, p.PushClassify.BUILDING_UP, target.tplId, cdId, sendTime)

    demolishResponse(true)
end

-- cancel demolish
pktHandlers[p.CS_BUILDING_DEMOLISH_CANCEL] = function(pktin, session)
    local function demolishCancelResponse(result)
        -- print('demolishCancelResponse', result)
        agent.replyPktout(session, p.SC_BUILDING_DEMOLISH_CANCEL_RESPONSE, result)
    end

    local gridId = pktin:read('i')
    -- print('###p.CS_BUILDING_DEMOLISH_CANCEL...gridId', gridId)
    local target = building.list[gridId]
    if target == nil or target.state ~= p.BuildingState.DEMOLISHING then
        print('###p.CS_BUILDING_DEMOLISH_CANCEL, wrong building state = ', target.state)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local job = {}
    for _, v in pairs(target.jobList) do
        job = v
        break
    end
    -- print('###p.CS_BUILDING_DEMOLISH_CANCEL, job=', utils.serialize(job))
    if not next(job) then
        print('###p.CS_BUILDING_DEMOLISH_CANCEL, building jobList is empty....gridId', gridId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    -- push
    pushStub.cancelPush(user.uid, job.cdId)

    cdlist.cancelCD(job.cdId)
    building.buildingStateClose(target.id, target.state, job.cdId)
    building.finishBuilder(job.cdId)
    target.jobList[job.cdId] = nil
    target.state = p.BuildingState.NORMAL
    target.sync = false
    target:onBuildFinish()

    building.sendBuildingUpdate()
    demolishCancelResponse(true)
end

-- heal
pktHandlers[p.CS_BUILDING_HEAL] = function(pktin, session)
    local function healResponse(result)
        print('healResponse', result)
        agent.replyPktout(session, p.SC_BUILDING_HEAL_RESPONSE, result)
    end

    local gridId, queueIndex ,size = pktin:read('iii')
    --print('gridId, queueIndex ,size =', gridId, queueIndex ,size)
    local healList = {}
    local healNumLimit = property.list.healNum
    local total = 0
    for i = 1, size, 1 do
        local armyType = pktin:readInteger()
        local count = pktin:readInteger()
        if count > 0 then
            healList[armyType] = count
            total = total + count
            print('armyType, count', armyType, count)
        end
    end
    if total > healNumLimit then
        print("###p.CS_BUILDING_TRAIN, total > healNumLimit", total, healNumLimit)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    local healNow = pktin:readBool()
    --print('p.CS_BUILDING_HEAL..healNow', healNow)

    local target = building.list[gridId]
    if not target then
        print("###p.CS_BUILDING_HEAL, building not exist, gridId = ", gridId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    if target.state ~= p.BuildingState.NORMAL then
        print('###p.CS_BUILDING_HEAL, wrong building state = ', target.state)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    local tplBuilding = t.building[target.tplId]
    if tplBuilding == nil then
        print('###p.CS_BUILDING_HEAL, building tpl not exists  tplId= ', target.tplId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    local wood, food, iron, stone = 0, 0, 0, 0
    local time = 0
    local totalCount = 0
    for armyType, count in pairs(healList) do
        local level = army.getArmyLevelByArmyType(armyType)
        local tplArmy = t.findArmysTpl(armyType, level)
        if tplArmy == nil then
            print('###p.CS_BUILDING_HEAL, army tpl not exists  armyType, level= ', armyType, level)
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            return
        end
        local woundCount = army.getArmyCount(armyType, p.ArmyState.WOUNDED)
        if count > woundCount then
            print("###p.CS_BUILDING_HEAL, count > woundCount", count, woundCount)
            agent.sendNoticeMessage(p.ErrorCode.TRAIN_EXCEED_WOUND, '', 1)
            return
        else
            local healratio = t.configure["healratio"] or 0.5
            wood = wood + tplArmy.training_wood * count * healratio
            food = food + tplArmy.training_food * count * healratio
            iron = iron + tplArmy.training_iron * count * healratio
            stone = stone + tplArmy.training_stone * count * healratio
            time = time + tplArmy.heal_time * count
            totalCount = totalCount + count
        end
    end

    time = math.ceil(time/(1 + property.list.healSpeedPct))
    wood = math.ceil(wood/(1 + property.list.healReduceResourcePct))
    food = math.ceil(food/(1 + property.list.healReduceResourcePct))
    iron = math.ceil(iron/(1 + property.list.healReduceResourcePct))
    stone = math.ceil(stone/(1 + property.list.healReduceResourcePct))
    --print("heal...", time, wood, food, iron, stone)

    local trainMax, cdType = 0, 0, 0
    trainMax = property.list.healQueueNum
    cdType = p.ItemPropType.WOUNDED_RECOVERY_SPEEDUP
   
    if healNow then
        local isEnough, ret = building.checkRemoveResource(time, food, wood, iron, stone, 0, p.ResourceConsumeType.HEAL)
        if not isEnough then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        army.heal(healList)
        building.evtBuildingHeal:trigger(totalCount)
    else
        --9.2检查训练队列数量
        local trainCount = 0
        local isUse = false
        for _, v in pairs(target.jobList) do
            trainCount = trainCount + 1
            if queueIndex == v.flagId then
                isUse = true
                break
            end
        end
        if trainCount > trainMax or isUse then
            print("###p.CS_BUILDING_TRAIN, building train queues have no empty queue or is used... trainCount, trainMax, queueIndex, isUse", trainCount, trainMax, queueIndex, isUse)
            agent.sendNoticeMessage(p.ErrorCode.TRAIN_QUEUES_NOT_UNLOCKED, '', 1)
            return
        end
        --9.3检查资源是否足够
        local isEnough, ret = building.checkRemoveResource(0, food, wood, iron, stone, 0, p.ResourceConsumeType.HEAL)
        if not isEnough then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        target.state = p.BuildingState.CARING
        local cdId = cdlist.addCD(p.ItemPropType.WOUNDED_RECOVERY_SPEEDUP, time, building.cdlistCallback)
        local resourceConsume = { food = food, wood = wood, stone = stone, iron = iron }
        local jinfo = jobInfo:new({ cdId = cdId, flagId = queueIndex, p1 = healList, p2 = resourceConsume, result = 1, isSync = false })
        target.jobList[cdId] = jinfo
        target.sync = false
        pushStub.appendPush(user.uid, p.PushClassify.ARMY_HEALED, 0, cdId, sendTime)
        -- push
        local sendTime = timer.getTimestampCache() + time
        for armyType, count in pairs(healList) do
            local list = {}
            list[armyType] = count
           -- building.evtBuildingHeal:trigger(count)
            army.SubWounded(list)
            --army.evtTraining:trigger(armyType, count)
        end
        building.sendBuildingUpdate()    
        building.sendBuildingQueuesUpdate()
    end
    healResponse(true)
end

-- cancel heal
pktHandlers[p.CS_BUILDING_HEAL_CANCEL] = function(pktin, session)
    local function healCancelResponse(result)
        agent.replyPktout(session, p.SC_BUILDING_HEAL_CANCEL_RESPONSE, result)
    end

    local gridId, queueIndex = pktin:read('ii')
    local target = building.list[gridId]
    if target == nil then
        print("###p.CS_BUILDING_HEAL_CANCEL, building not exist, gridId = ", gridId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local job = {}
    for _, v in pairs(target.jobList) do
        if v.flagId == queueIndex and v.result == 1 then
            job = v
            break
        end
    end
    --print('###p.CS_BUILDING_HEAL_CANCEL, job=', utils.serialize(job))
    if not next(job) then
        print('###p.CS_BUILDING_HEAL_CANCEL, no training queue...gricdId, queueIndex', gridId, queueIndex)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end  
      
    if target.state ~= p.BuildingState.CARING or target.p1 == 0 then
        print("###p.CS_BUILDING_HEAL_CANCEL, is not caring", target.state, target.p1)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    local resourceConsume = job.p2 or {}
    local wood = resourceConsume.wood or 0
    local food = resourceConsume.food or 0
    local iron = resourceConsume.iron or 0
    local stone = resourceConsume.stone or 0
    wood = math.floor(wood * cancelratio)
    food = math.floor(food * cancelratio)
    iron = math.floor(iron * cancelratio)
    stone = math.floor(stone * cancelratio)

    local list = {}
    table.insert(list, { tplId = p.ResourceType.FOOD, count = food })
    table.insert(list, { tplId = p.ResourceType.WOOD, count = wood })
    table.insert(list, { tplId = p.ResourceType.STONE, count = stone })
    table.insert(list, { tplId = p.ResourceType.IRON, count = iron })
    user.addResources(list)
    local woundList = {}
    for k,v in pairs(job) do
        if k == 'p1' then
            woundList = v
        end
    end
    --print('###p.CS_BUILDING_HEAL_CANCEL, woundList=', utils.serialize(woundList))
    for k,v in pairs(woundList) do
        army.add(k,p.ArmyState.WOUNDED,v,p.ArmyGainType.WOUNDED)
    end
    -- push
    pushStub.cancelPush(user.uid, job.cdId)

    cdlist.cancelCD(job.cdId)
    building.buildingStateClose(target.id, target.state, job.cdId)
    target.state = p.BuildingState.NORMAL
    target.jobList[job.cdId] = nil
    target.sync = false

    building.sendBuildingUpdate()
    building.sendBuildingQueuesUpdate()
    healCancelResponse(true)
end

-- open buidler2
pktHandlers[p.CS_BUILDING_OPEN_BUILDER2] = function(pktin, session)
    local function openBuilder2Response(result)
        -- print('opebBuilder2Response', result)
        agent.replyPktout(session, p.SC_BUILDING_OPEN_BUILDER2_RESPONSE, result)
    end

    local allTime = t.configure['queueTime'] or 3600 * 48
    local cost = t.configure['queueCost'] or 250
    if not user.removeResource(p.ResourceType.GOLD, cost, p.ResourceConsumeType.OPEN_BUILDER2) then
        agent.sendNoticeMessage(p.ErrorCode.PUBLIC_GOLD_NOT_ENOUGH, '', 1)
        return
    end

    if building.builder2ExpireTime < timer.getTimestampCache() then
        building.builder2ExpireTime = timer.getTimestampCache()
    end
    building.builder2ExpireTime = building.builder2ExpireTime + allTime

    building.sendBuildingUpdate()
    openBuilder2Response(true)
end

-- open forest
pktHandlers[p.CS_BUILDING_OPEN_FOREST] = function(pktin, session)
    local hero = agent.hero
    local function openForestResponse(result, battleId, warReport)
        -- print('openForestResponse', result, battleId)
        agent.replyPktout(session, p.SC_BUILDING_OPEN_FOREST_RESPONSE, result, battleId, warReport)
    end

    local forestId, size = pktin:read('ii')
    -- print('p.CS_BUILDING_OPEN_FOREST...forestId, size', forestId, size)

    local battleId = 0
    local warReport 
    local tpl = t.forest[forestId]
    if building.forestList[forestId] or tpl == nil then
        print('###p.CS_BUILDING_OPEN_FOREST, invalid forestId =', forestId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    local armyList = {}
    for i=1, size do
        local heroId, armyType, count, position = pktin:read('iiii')
        -- print('p.CS_BUILDING_OPEN_FOREST...heroId, armyType, count, position', heroId, armyType, count, position)
        if heroId > 0 and hero.info[heroId] then
            --TODO:
            local level = agent.army.getArmyLevelByArmyType(armyType)
            armyList[heroId] = { heroId = heroId, armyType = armyType, level = level, count = count, position = position }
        end
    end
    if not next(armyList) then
        print('p.CS_BUILDING_OPEN_FOREST...armyList is empty')
        agent.sendNoticeMessage(p.ErrorCode.HERO_ARMYCOUNT_CANNOT_ZERO, '', 1)
        return
    end

    local castle = building.list[p.BuildingType.CASTLE]
    if tpl.openLevel > castle.level then
        -- print('###p.CS_BUILDING_OPEN_FOREST, need castle level > tpl.openLevel', tpl.openLevel, castle.level)
        agent.sendNoticeMessage(p.ErrorCode.CASTLE_LEVEL_NOT_ENOUGH, '', 1)
        return
    end
    if not user.isResourceEnough(p.ResourceType.FOOD, tpl.food) then
        -- print('###p.CS_BUILDING_OPEN_FOREST, not  enough food')
        agent.sendNoticeMessage(p.ErrorCode.PUBLIC_RESOURCE_NOT_ENOUGH, '', 1)
        return
    end
    if not user.isResourceEnough(p.ResourceType.WOOD, tpl.wood) then
        -- print('###p.CS_BUILDING_OPEN_FOREST, not  enough wood')
        agent.sendNoticeMessage(p.ErrorCode.PUBLIC_RESOURCE_NOT_ENOUGH, '', 1)
        return
    end

    --调用战斗接口  事件处理结果
    local dataOnCombatOver = { forestId = forestId }
    --Todo: modify temp 
    -- 如果必然触发单挑则 levelType == 2
    local _level = 0
    if tpl.singleCombat == 1 then
        _level  = 2
    else
        _level  = 1
    end
    local extParam = {singleCombat = tpl.singleCombat, levelType = _level}
    battleId, warReport = agent.combat.createBattleByArmyList(p.BattleType.OPEN_FOREST, armyList, tpl.armyGroup, tpl.armyCount, dataOnCombatOver, extParam)

    openForestResponse(p.ErrorCode.SUCCESS, battleId, warReport)
end

-- move
pktHandlers[p.CS_BUILDING_MOVE] = function(pktin, session)
    local function moveResponse(result)
        --print('moveResponse', result)
        agent.replyPktout(session, p.SC_BUILDING_MOVE_RESPONSE, result)
    end

    local function checkMoveGrids(a,  b)
        if a ~= b and a >= 100 and a <= 1024 and b >= 100 and b <=1024 then
            return true
        end
        return false
    end

    local fromId, toId = pktin:read('ii')
    if not checkMoveGrids(fromId, toId) then
        print('invalid moving gridId',  fromId, toId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --移动源不能为空地
    local fromBuilding = building.list[fromId]
    if not fromBuilding or fromBuilding.tplId == 0 then
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --目标为空地
    local toBuilding = building.list[toId]
    if not toBuilding or toBuilding.tplId ~= 0 then
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    -- local cost = t.configure['MoveCost']
    -- if not user.removeResource(p.ResourceType.GOLD, cost) then
    --     moveResponse(false)
    --     return
    -- end

    -- swap position
    fromBuilding.gridId = toId
    fromBuilding.sync = false
    toBuilding.gridId = fromId
    toBuilding.sync = false
    building.list[fromId] = toBuilding
    building.list[toId] =  fromBuilding

    building.sendBuildingUpdate()
    moveResponse(true)
end

-- technology research
pktHandlers[p.CS_BUILDING_TECHNOLOGY_RESEARCH] = function(pktin, session)
    local function techResearchResponse(result)
        -- print("====>techResearchResponse", result)
        agent.replyPktout(session, p.SC_BUILDING_TECHNOLOGY_RESEARCH_RESPONSE, result)
    end

    local gridId, tplId, researchNow, groupId = pktin:read("iibi")
    --1.已有建筑列表g
    local target = building.list[gridId]
    if not target then
        print('p.CS_BUILDING_TECHNOLOGY_RESEARCH...building not exist, gridId = ', gridId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --2.建筑当前状态
    if target.state ~= p.BuildingState.NORMAL and target.state ~= p.BuildingState.RESEARCHING then
        print('p.CS_BUILDING_TECHNOLOGY_RESEARCH...wrong building state = ', target.state)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --3.建筑模板表
    local tplBuilding = t.building[target.tplId]
    if tplBuilding == nil then
        print('p.CS_BUILDING_TECHNOLOGY_RESEARCH...building tpl not exists  tplId= ', target.tplId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    elseif tplBuilding.buildingType ~= p.BuildingType.COLLEGE then
        print('p.CS_BUILDING_TECHNOLOGY_RESEARCH...building type wrong  type= ',tplBuilding.buildingType)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --4.科技模板表
    local tplTech = t.technology[tplId]
    if not tplTech then
        print('p.CS_BUILDING_TECHNOLOGY_RESEARCH...technology tpl not exist tplId = ', tplId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    -- 判断科技组是否正确
    if tplTech.group ~= groupId then
        print('p.CS_BUILDING_TECHNOLOGY_RESEARCH...technology tpl not exist tplId = ', tplTech.group)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --5.判断当前科技是否满级
    local techLevel = tech.getLevelByGroup(tplTech.type,tplTech.group)
    if techLevel >= tplTech.maxLevel then
        print('p.CS_BUILDING_TECHNOLOGY_RESEARCH...technology level >= maxLevel = ', techLevel, tplTech.maxLevel)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --检查研究队列数量
    local techCount = 0
    local isTech = false
    for _, v in pairs(target.jobList) do
        techCount = techCount + 1
        if v.flagId == tplId then
            isTech = true
            break
        end
    end
    if isTech then
        print('p.CS_BUILDING_TECHNOLOGY_RESEARCH...technology is researching.. isTech= ', isTech)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    if not researchNow and techCount >= property.list.technologyQueues then
        print('p.CS_BUILDING_TECHNOLOGY_RESEARCH...technology counts are full, techCount, maxCount = ', techCount, property.list.technologyQueues)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --6.科技解锁条件
    if not building.checkUnlockCond(tplTech.unlockCond) then
        print('p.CS_BUILDING_TECHNOLOGY_RESEARCH...technology unlock can not satisfy')
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local nextTechId = tech.getNextLevelTechIdByGroup(tplTech.type,tplTech.group)
    local nextTech = t.technology[nextTechId]
    local time = math.ceil(nextTech.time)
    local wood, food, iron, stone , coin = nextTech.wood , nextTech.food , nextTech.iron , nextTech.stone,nextTech.coin
    --属性
    local reduceResearchConsume = property.list.reduceResearchPct
    food = math.ceil(food * (1 - reduceResearchConsume))
    wood = math.ceil(wood * (1 - reduceResearchConsume))
    iron = math.ceil(iron * (1 - reduceResearchConsume))
    stone = math.ceil(stone * (1 - reduceResearchConsume))
    coin = math.ceil(coin)
    if researchNow then
        local isEnough, ret = building.checkRemoveResource(time, food, wood, iron, stone, coin, p.ResourceConsumeType.TECHNOLOGY_RESEARCH)
        if not isEnough then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        tech.addLevel(tplTech.type,tplTech.group)
    else
        local isEnough, ret = building.checkRemoveResource(0, food, wood, iron, stone, coin, p.ResourceConsumeType.TECHNOLOGY_RESEARCH)
        if not isEnough then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        target.state = p.BuildingState.RESEARCHING
        local cdId= cdlist.addCD(p.ItemPropType.SCIENCE_RESEARCH_SPEEDUP, time, building.cdlistCallback)
        local resourceConsume = { food = food, wood = wood, stone = stone, iron = iron }
        local jinfo = jobInfo:new({ cdId = cdId, flagId = tplId, p1 = resourceConsume, result = 1 })
        target.jobList[cdId] = jinfo
        target.sync = false
        -- push
        local sendTime = timer.getTimestampCache() + time
        pushStub.appendPush(user.uid, p.PushClassify.TECHNOLOGY_UP, tplId, cdId, sendTime)

        building.sendBuildingQueuesUpdate()
        building.sendBuildingUpdate()
    end
    --tech.evtDoResearch:trigger(tplId)

    techResearchResponse(true)
    -- log
    logStub.appendTechnology(user.uid, tplId, techLevel, timer.getTimestampCache())
end

-- cancel technology research
pktHandlers[p.CS_BUILDING_TECHNOLOGY_RESEARCH_CANCEL] = function(pktin, session)
    local function techResearchCancelResponse(isOk)
        print("techResearchCancelResponse", isOk)
        agent.replyPktout(session, p.SC_BUILDING_TECHNOLOGY_RESEARCH_CANCEL_RESPONSE, isOk)
    end

    local gridId,tplId,groupId = pktin:read('iii')
    --1.已有建筑列表
    local target = building.list[gridId]
    if not target then
        print('p.CS_BUILDING_TECHNOLOGY_RESEARCH_CANCEL...building not exist, gridId = ', gridId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --2.建筑当前状态 取消的科技
    local job = {}
    for _, v in pairs(target.jobList) do
        if v.flagId == tplId and v.result == 1 then
            job = v
            break
        end
    end
    if not next(job) then
        print('p.CS_BUILDING_TECHNOLOGY_RESEARCH_CANCEL, no technology queue...gridId, tplId', gridId, tplId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --3.建筑模板表
    local tplBuilding = t.building[target.tplId]
    if not tplBuilding then
        print('p.CS_BUILDING_TECHNOLOGY_RESEARCH_CANCEL...building tpl not exists  tplId= ', target.tplId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    elseif tplBuilding.buildingType ~= p.BuildingType.COLLEGE then
        print('p.CS_BUILDING_TECHNOLOGY_RESEARCH_CANCEL...building type wrong  type= ', tplBuilding.buildingType)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --4.科技模板表
    local tplTech = t.technology[tplId]
    if not tplTech then
        print('p.CS_BUILDING_TECHNOLOGY_RESEARCH_CANCEL...technology tpl not exist tplId = ', tplId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    --local techLevel = tplTech.level
    --return san
    --return resource
    local resourceConsume = job.p1 or {}
    local wood = resourceConsume.wood or 0
    local food = resourceConsume.food or 0
    local iron = resourceConsume.iron or 0
    local stone = resourceConsume.stone or 0
    local coin = resourceConsume.coin or 0
    wood = math.floor(wood * cancelratio)
    food = math.floor(food * cancelratio)
    iron = math.floor(iron * cancelratio)
    stone = math.floor(stone * cancelratio)
    coin = math.floor(coin * cancelratio)
    local list = {}
    table.insert(list, { tplId = p.ResourceType.FOOD, count = food })
    table.insert(list, { tplId = p.ResourceType.WOOD, count = wood })
    table.insert(list, { tplId = p.ResourceType.STONE, count = stone })
    table.insert(list, { tplId = p.ResourceType.IRON, count = iron })
    --table.insert(list, { tplId = p.ResourceType.SILVER, count = coin })
    --todo: cg 添加铜币
    user.addResources(list)
    -- push
    pushStub.cancelPush(user.uid, job.cdId)

    cdlist.cancelCD(job.cdId)
    building.buildingStateClose(target.id, target.state, job.cdId)
    job.cdId = 0
    job.result = 3
    job.isSync = false

    building.sendBuildingQueuesUpdate()
    local newState = building.setBuildingStateByGridId(gridId)
    -- print('###p.CS_BUILDING_TECHNOLOGY_RESEARCH_CANCEL,  newState=', newState)
    target.state = newState
    target.sync = false

    building.sendBuildingUpdate()
    techResearchCancelResponse(0)
end

-- TAKE_TAX
pktHandlers[p.CS_TAKE_TAX] = function(pktin, session)
    -- taxList = taxSilver,taxGoldCrit
    local function takeTaxResponse(ok,taxList)
        agent.replyPktout(session, p.SC_TAKE_TAX_RESPONSE, '@@1=b,2=[id=i,taxSilver=i,taxGoldCrit=i]', ok, taxList)
    end

    local isGoldTax =  pktin:read('b')
    local count = pktin:read('i')
    --{freeTime=10,freeCD=300,goldBase=2}
    local taxList = {}

    local taxConf = t.configure["taxation"]
    if not isGoldTax then
        if buildingTax.taxFreeCount < taxConf.freeTime then
            local now = timer.getTimestampCache()
            if buildingTax.taxFreeExpireTime <= now then
                local taxSilver = property.list.castleTaxSilver or 0
                user.addResource(p.ResourceType.SILVER, taxSilver, p.ResourceGainType.TAX)
                buildingTax.taxFreeCount = buildingTax.taxFreeCount + 1
                buildingTax.taxFreeExpireTime = now + taxConf.freeCD

                table.insert(buildingTax.freeTaxList, {taxSilver=taxSilver,timestamp = now})

                local taxResult = {id=0,taxSilver=taxSilver,taxGoldCrit=0}
                table.insert(taxList,taxResult)
                takeTaxResponse(true,taxList)
                building.sendTaxUpdate()

                -- building.eveTakeTax:trigger(1)
                building.evtResourceTax:trigger(p.ResourceType.SILVER, taxSilver)
            else
                takeTaxResponse(false,taxList)
            end
        else
            takeTaxResponse(false,taxList)
        end
    else
        if buildingTax.taxGoldCount < agent.vip.tpl.taxCount then
            local takeCount = count
            local remainCount = agent.vip.tpl.taxCount - buildingTax.taxGoldCount
            if takeCount > remainCount then
                takeCount = remainCount
            end
            local totalSilver = 0
            for i=1, takeCount do
                local currentCount = buildingTax.taxGoldCount + 1
                local taxTpl = t.takeTax[currentCount]
                if taxTpl ~= nil then
                    local gold = taxTpl.gold
                    if user.removeResource(p.ResourceType.GOLD, gold, p.ResourceConsumeType.DEFAULT, 0, false) then
                        local taxCritConf = t.configure["taxationCrit"]
                        local totalWeight = 0
                        for k,v in pairs(taxCritConf) do
                            totalWeight = totalWeight + v.weight
                        end

                        local crit = 0
                        local rand = utils.getRandomNum(1, totalWeight + 1)
                        local weight = 0
                        for k,v in pairs(taxCritConf) do
                            weight = weight + v.weight
                            if weight >= rand then
                                crit = v.times
                                break
                            end
                        end

                        local taxSilver = taxTpl.silver
                        taxSilver = math.floor(taxSilver * (1 + crit/100))

                        totalSilver = totalSilver + taxSilver
                        buildingTax.taxGoldCount = currentCount

                        local taxResult = {id=currentCount,taxSilver=taxSilver,taxGoldCrit=crit}
                        table.insert(taxList,taxResult)

                        -- building.eveTakeTax:trigger(2)
                    else
                        print("gold is not enough", gold)
                        -- takeTaxResponse(false,taxList)
                        break
                    end
                end
            end
            if totalSilver > 0 then
                user.addResource(p.ResourceType.SILVER, totalSilver, p.ResourceGainType.TAX, 0, true)
                building.evtResourceTax:trigger(p.ResourceType.SILVER, totalSilver)
                building.evtGoldResourceTax:trigger()
            end

            takeTaxResponse(true,taxList)
            building.sendTaxUpdate()
        else
            takeTaxResponse(false,taxList)
        end
    end
end

return building
