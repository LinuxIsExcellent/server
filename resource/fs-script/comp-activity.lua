local agent = ...
local p = require('protocol')
local t = require('tploader')
local timer = require('timer')
local utils = require('utils')
local activityStub = require('stub/activity')

local user = agent.user
local hero = agent.hero
local dict = agent.dict
local bag = agent.bag
local cdlist = agent.cdlist
local vip = agent.vip

local targetInfo = {
    activityId = 0,
    condList = {},      -- {[condType] = progress}
    drawnList = {},     -- {[targetId] = true}
    createTime = 0,

    sync = false,
}

local exchangeInfo = {
    activityId = 0,
    exTimesList = {},        -- 已兑换列表 {[exchangeId] = times}
    createTime = 0,

    sync = false,
}

local activity = {
    list = {},
    misc = {
        totalLoginTimes = 0,        -- 总共累计登录天数
        monthLoginTimes = 0,        -- 每月累计登录天数(每月重置)
        lastLoginTime = 0,
        accumulateDrawDays = {},    -- 累计登录领取天数列表(七天)
        everydayDrawDays = {},      -- 每日登录领取天数列表(每月重置)
        timeMealDrawIds = {},       -- 限时大餐领取id列表（每天重置）
    },
    targetList = {},               -- 目标活动信息 [activityId] = targetInfo
    exchangeList = {},             -- 兑换活动信息 [activityId] = exchangeInfo
}

function targetInfo:new(o)
    o = o or {}
    if o.condList == nil then o.condList = {} end
    if o.drawnList == nil then o.drawnList = {} end
    setmetatable(o, self)
    self.__index = self
    if o.createTime == 0 then
        o.createTime = timer.getTimestampCache()
    end
    return o
end

function exchangeInfo:new(o)
    o = o or {}
    if o.exTimesList == nil then o.exTimesList = {} end
    setmetatable(o, self)
    self.__index = self
    if o.createTime == 0 then
        o.createTime = timer.getTimestampCache()
    end
    return o
end

function activity.onInit()
    activity.dbLoad()

    activity.checkLogin()
    activity.sendActivityRecordUpdate()
    activity.sendActivityAccumulateLoginInfoUpdate()
    activity.sendActivityLoginEverydayInfoUpdate()
    activity.sendActivityTimeMealInfoUpdate()
    activity.sendActivityTargetInfoUpdate()
    activity.sendActivityExchangeInfoUpdate()

    -- attach evt
    cdlist.evtHour0Refresh:attachRaw(function()
        activity.sendActivityRecordUpdate()
        activity.checkLogin()
        activity.sendActivityAccumulateLoginInfoUpdate()
        activity.sendActivityLoginEverydayInfoUpdate()
        activity.sendActivityTimeMealInfoUpdate()
    end)
end

function activity.onAllCompInit()

end

function activity.onClose()
    activity.onSave()
end

function activity.onSave()
    activity.dbSave()
end

function activity.dbSave()
    local aMisc = activity.misc
    local miscData = {}
    miscData.totalLoginTimes = aMisc.totalLoginTimes
    miscData.monthLoginTimes = aMisc.monthLoginTimes
    miscData.lastLoginTime = aMisc.lastLoginTime
    miscData.accumulateDrawDays = aMisc.accumulateDrawDays
    miscData.everydayDrawDays = aMisc.everydayDrawDays
    miscData.timeMealDrawIds = aMisc.timeMealDrawIds
    -- miscData = aMisc
    dict.set('activity.misc', miscData)

    local aTarget = activity.targetList
    local targetData = {}
    for _, aInfo in pairs(aTarget) do
        local info = {}
        info.activityId = aInfo.activityId
        info.condList = aInfo.condList
        info.drawnList = aInfo.drawnList
        info.createTime = aInfo.createTime
        table.insert(targetData, info)
    end
    dict.set('activity.target', targetData)

    local aExchange = activity.exchangeList
    local exchangeData = {}
    for _, eInfo in pairs(aExchange) do
        local info = {}
        info.activityId = eInfo.activityId
        info.exTimesList = eInfo.exTimesList
        info.createTime = eInfo.createTime
        table.insert(exchangeData, info)
    end
    dict.set('activity.exchange', exchangeData)
end

function activity.dbLoad()
    local aMisc = activity.misc
    local miscData = dict.get('activity.misc')
    if miscData then
        aMisc.totalLoginTimes = miscData.totalLoginTimes
        aMisc.monthLoginTimes = miscData.monthLoginTimes
        aMisc.lastLoginTime = miscData.lastLoginTime
        aMisc.accumulateDrawDays = miscData.accumulateDrawDays
        aMisc.everydayDrawDays = miscData.everydayDrawDays
        aMisc.timeMealDrawIds = miscData.timeMealDrawIds
    end

    local aTarget = activity.targetList
    local targetData = dict.get('activity.target')
    if type(targetData) == 'table' then
        for _, tData in pairs(targetData) do
            local info = targetInfo:new({
                activityId = tData.activityId,
                condList = tData.condList,
                drawnList = tData.drawnList,
                createTime = tData.createTime,
            })
            aTarget[tData.activityId] = info
        end
    end

    local aExchange = activity.exchangeList
    local exchangeData = dict.get('activity.exchange')
    if type(exchangeData) == 'table' then
        for _, eData in pairs(exchangeData) do
            local info = exchangeInfo:new({
                activityId = eData.activityId,
                exTimesList = eData.exTimesList,
                createTime = eData.createTime,
            })
            aExchange[eData.activityId] = info
        end
    end
end

-- orange
function activity.checkLogin()
    local aMisc = activity.misc
    if not timer.isTodayFromTimestamp(aMisc.lastLoginTime) then
        local now = timer.getTimestampCache()

        -- 每日登录
        local lastMonth = tonumber(os.date('%m', aMisc.lastLoginTime))
        local nowMonth = tonumber(os.date('%m', now))
        if lastMonth ~= nowMonth then
            aMisc.monthLoginTimes = 1
            aMisc.everydayDrawDays = {}
        else
            aMisc.monthLoginTimes = aMisc.monthLoginTimes + 1
        end
        -- 限时大餐
        aMisc.timeMealDrawIds = {}

        aMisc.totalLoginTimes = aMisc.totalLoginTimes + 1
        aMisc.lastLoginTime = now
    end
end

local function isVaildActivity(activity)
    if activity.active and user.info.level >= activity.minLordLevel and user.info.level <= activity.maxLordLevel then
        return true
    end
    return false
end

-- 检查是否要增加进度 value为增加的进度值
function activity.checkTarget(condType, value)
    value = math.floor(value)
    if value <= 0 then
        return
    end
    for _, v in pairs(activityStub.activities) do
        if v.type == p.ActivityType.TARGET and isVaildActivity(v) then
            local info = activity.targetList[v.id]
            if info == nil then
                info = targetInfo:new({activityId = v.id})
                activity.targetList[v.id] = info
            end
            if info.condList[condType] then
                info.condList[condType] = info.condList[condType] + value
            else
                info.condList[condType] = value
            end
            info.sync = false
        end
    end
    activity.sendActivityTargetInfoUpdate()
end

-- 根据活动类型获取一个有效活动
local function getVaildActivity(type)
    for _, v in pairs(activityStub.activities) do
        if v.type == type then
            if isVaildActivity(v) then
                return v
            end
        end
    end
    return nil
end
local function getVaildActivityById(id)
    for _, v in pairs(activityStub.activities) do
        if v.id == id then
            if isVaildActivity(v) then
                return v
            end
        end
    end
    return nil
end

-- 通用活动配置列表获取奖励（物品可能，道具可能）
-- list = {itemId, level, star, count}
local function drawRewards(list, gainType)
    local drops = {}
    for _, v in ipairs(list) do
        if v.itemId >= 1300001 and v.itemId < 1399999 then
            hero.addHeroByCond(v.itemId, v.level, v.star, gainType, true)
        else
            table.insert(drops, t.createDropItem(v.itemId, v.count))
        end
    end

    bag.pickDropItems(drops, gainType)
end

-- send packet
function activity.sendActivityRecordUpdate()
    --print('activity.sendActivityRecordUpdate')
    local list = {}
    local now = timer.getTimestampCache()
    for _, v in pairs(activityStub.activities) do
        if v.display and v.active then
            local record = {
                id = v.id,
                remark = v.remark,
                priority = v.priority,
                type = v.type,
                tplId = v.tplId,
                minLordLevel = v.minLordLevel,
                maxLordLevel = v.maxLordLevel,
                openTime = v.openTime,
                closeTime = v.closeTime,
                openLeftSeconds = 0,
                closeLeftSeconds = 0,
                display = v.display,
                -- autoReward = v.autoReward,
                bestItemList = v.bestItemList,
                int1 = 0,
                int2 = 0,
                int3 = 0,
                accumulateLoginList = {},
                loginEverydayList = {},
                timeMealList = {},
                targetList = {},
                exchangeList = {},
            }
            if v.openTime > now then record.openLeftSeconds = v.openTime - now end
            if v.closeTime > now then record.closeLeftSeconds = v.closeTime - now end
            if v.type == p.ActivityType.NEXT_LOGIN_GIVE_HERO then
                record.int1 = v.config.heroTplId
                record.int2 = v.config.heroStar
                record.int3 = v.config.heroLevel
            elseif v.type == p.ActivityType.ACCUMULATE_LOGIN then
                record.accumulateLoginList = v.config.accumulateLoginList
            elseif v.type == p.ActivityType.LOGIN_EVERYDAY then
                -- 拿当前月份配置给前端
                local now = timer.getTimestampCache()
                local month = tonumber(os.date('%m', now))
                for _, v2 in pairs(v.config.loginEverydayList) do
                    if v2.month == month then
                        record.loginEverydayList = v2.rewards
                    end
                end
            elseif v.type == p.ActivityType.TIME_MEAL then
                record.timeMealList = v.config.timeMealList
            elseif v.type == p.ActivityType.TARGET then
                record.targetList = v.config.targetList
            elseif v.type == p.ActivityType.EXCHANGE then
                record.exchangeList = v.config.exchangeList
            else

            end
            table.insert(list, record)
            -- print('activity.sendActivityRecordUpdate id, remark, type, tplId = ', record.id, record.remark, record.type, record.tplId)
        end
    end
    local accumulateLoginList = '[day=i,items=[itemId=i,level=i,star=i,count=i]]'
    local loginEverydayList = '[day=i,itemId=i,level=i,star=i,count=i,vipLevel=i,mul=i]'
    local timeMealList = '[mealId=i,beginTime=i,endTime=i,itemId=i,count=i]'
    local targetList = '[targetId=i,condType=i,progress=i,rewards=[itemId=i,level=i,star=i,count=i],resType=i,resCount=i,p1=i]'
    local exchangeList = '[exchangeId=i,needItems=[itemId=i,count=i],getItems=[itemId=i,count=i],exTimesMax=i]'
    agent.sendPktout(p.SC_ACTIVITY_RECORD_UPDATE,  '@@1=[id=i,priority=i,type=i,tplId=i,minLordLevel=i,maxLordLevel=i,openTime=i,closeTime=i,openLeftSeconds=i,closeLeftSeconds=i,display=b,bestItemList=s,int1=i,int2=i,int3=i' .. ',accumulateLoginList=' .. accumulateLoginList .. ',loginEverydayList=' .. loginEverydayList .. ',timeMealList=' .. timeMealList .. ',targetList=' .. targetList .. ',exchangeList=' .. exchangeList .. ']', list)
end

function activity.sendActivityAccumulateLoginInfoUpdate()
    --print('activity.sendActivityAccumulateLoginInfoUpdate()')
    local drawDayList = {}
    for _, v in ipairs(activity.misc.accumulateDrawDays) do
        table.insert(drawDayList, {day = v})
    end
    agent.sendPktout(p.SC_ACTIVITY_ACCUMULATE_LOGIN_INFO_UPDATE, '@@1=i,2=[day=i]', activity.misc.totalLoginTimes, drawDayList)
end

function activity.sendActivityLoginEverydayInfoUpdate()
    --print('activity.sendActivityLoginEverydayInfoUpdate()')
    local drawDayList = {}
    for _, v in ipairs(activity.misc.everydayDrawDays) do
        table.insert(drawDayList, {day = v})
    end
    agent.sendPktout(p.SC_ACTIVITY_LOGIN_EVERYDAY_INFO_UPDATE, '@@1=i,2=[day=i]', activity.misc.monthLoginTimes, drawDayList)
end

function activity.sendActivityTimeMealInfoUpdate()
    -- print('activity.sendActivityTimeMealInfoUpdate()')
    local drawIdList = {}
    for _, v in ipairs(activity.misc.timeMealDrawIds) do
        table.insert(drawIdList, {id = v})
    end
    agent.sendPktout(p.SC_ACTIVITY_TIME_MEAL_INFO_UPDATE, '@@1=[id=i]', drawIdList)
end

function activity.sendActivityTargetInfoUpdate()
    -- print('activity.sendActivityTargetInfoUpdate()')
    local list = {}
    for _, tInfo in pairs(activity.targetList) do
        if tInfo.sync == false then
            local info = {condList = {}, drawnList = {}}
            info.activityId = tInfo.activityId
            for condType, progress in pairs(tInfo.condList) do
                table.insert(info.condList, {condType = condType, progress = progress})
            end
            for targetId, _ in pairs(tInfo.drawnList) do
                table.insert(info.drawnList, {targetId = targetId})
            end
            table.insert(list, info)
            tInfo.sync = true
        end
    end
    agent.sendPktout(p.SC_ACTIVITY_TARGET_INFO_UPDATE, '@@1=[activityId=i,condList=[condType=i,progress=i],drawnList=[targetId=i]]', list)
end

function activity.sendActivityExchangeInfoUpdate()
    -- print('activity.sendActivityExchangeInfoUpdate()')
    local list = {}
    for _, eInfo in pairs(activity.exchangeList) do
        if eInfo.sync == false then
            local info = {exTimesList = {}}
            info.activityId = eInfo.activityId
            for exchangeId, times in pairs(eInfo.exTimesList) do
                table.insert(info.exTimesList, {exchangeId = exchangeId, times = times})
            end
            table.insert(list, info)
            eInfo.sync = true
        end
    end
    agent.sendPktout(p.SC_ACTIVITY_EXCHANGE_INFO_UPDATE, '@@1=[activityId=i,exTimesList=[exchangeId=i,times=i]]', list)
end

-- recv client packet
function activity.cs_activity_login_next_day_draw()
    -- TODO 暂时不做
end

function activity.cs_activity_accumulate_login_draw(day, session)
    local function response(result, day)
        agent.replyPktout(session, p.SC_ACTIVITY_ACCUMULATE_LOGIN_DRAW_RESPONSE, result, day)
    end

    local aty = getVaildActivity(p.ActivityType.ACCUMULATE_LOGIN)
    if not aty then
        print('vaild activity not exist')
        response(p.ErrorCode.ACTIVITY_NOT_EXIST, day)
        return
    end

    local aMisc = activity.misc
    if day > aMisc.totalLoginTimes then
        print('day > totalLoginTimes', day, aMisc.totalLoginTimes)
        response(p.ErrorCode.ACTIVITY_TIME_INVALID, day)
        return
    end

    for _, v in ipairs(aMisc.accumulateDrawDays) do
        if v == day then
            print('this day has drawn', day)
            response(p.ErrorCode.ACTIVITY_HAS_DRAWN, day)
            return
        end
    end

    -- draw rewards
    for _, v in ipairs(aty.config.accumulateLoginList) do
        if v.day == day then
            table.insert(aMisc.accumulateDrawDays, day)
            drawRewards(v.items, p.ResourceGainType.ACTIVITY_ACCUMULATE_LOGIN)
            break
        end
    end

    -- send update
    response(p.ErrorCode.SUCCESS, day)
    activity.sendActivityAccumulateLoginInfoUpdate()
end

function activity.cs_activity_login_everyday_draw(day, session)
    local function response(result, day)
        agent.replyPktout(session, p.SC_ACTIVITY_LOGIN_EVERYDAY_DRAW_RESPONSE, result, day)
    end

    local aty = getVaildActivity(p.ActivityType.LOGIN_EVERYDAY)
    if not aty then
        print('vaild activity not exist')
        response(p.ErrorCode.ACTIVITY_NOT_EXIST, day)
        return
    end

    local aMisc = activity.misc
    if day > aMisc.monthLoginTimes then
        print('day > monthLoginTimes', day, aMisc.monthLoginTimes)
        response(p.ErrorCode.ACTIVITY_TIME_INVALID, day)
        return
    end

    for _, v in ipairs(aMisc.everydayDrawDays) do
        if v == day then
            print('this day has drawn', day)
            response(p.ErrorCode.ACTIVITY_HAS_DRAWN, day)
            return
        end
    end

    -- draw rewards
    for _, v in ipairs(aty.config.loginEverydayList) do
        local now = timer.getTimestampCache()
        local month = tonumber(os.date('%m', now))
        if v.month == month then
            for _, v2 in ipairs(v.rewards) do
                if v2.day == day then
                    table.insert(aMisc.everydayDrawDays, day)
                    local items, count = {}, v2.count
                    if vip.vipLevel() > v2.vipLevel then
                        count = count * v2.mul
                    end
                    table.insert(items, {itemId = v2.itemId, level = v2.level, star = v2.star, count = count})
                    drawRewards(items, p.ResourceGainType.ACTIVITY_LOGIN_EVERYDAY)
                    break
                end
            end
        end
    end

    -- send update
    response(p.ErrorCode.SUCCESS, day)
    activity.sendActivityLoginEverydayInfoUpdate()
end

function activity.cs_activity_time_meal_draw(mealId, session)
    local function response(result, mealId)
        agent.replyPktout(session, p.SC_ACTIVITY_TIME_MEAL_DRAW_RESPONSE, result, mealId)
    end

    local aty = getVaildActivity(p.ActivityType.TIME_MEAL)
    if not aty then
        print('vaild activity not exist')
        response(p.ErrorCode.ACTIVITY_NOT_EXIST, mealId)
        return
    end

    local aMisc = activity.misc
 
    -- check draw
    for _, v in ipairs(aMisc.timeMealDrawIds) do
        if v == mealId then
            print('this meal id has drawn. mealId', mealId)
            response(p.ErrorCode.ACTIVITY_HAS_DRAWN, mealId)
            return
        end
    end

    -- check time and draw
    for _, v in ipairs(aty.config.timeMealList) do
        if mealId == v.mealId then
            local now = timer.getTimestampCache()
            local hour = tonumber(os.date('%H', now))
            local min = tonumber(os.date('%M', now))
            local passMin = hour * 60 + min
            if passMin >= v.beginTime and passMin <= v.endTime then
                table.insert(aMisc.timeMealDrawIds, mealId)
                local items = {}
                table.insert(items, {itemId = v.itemId, level = 0, star = 0, count = v.count})
                drawRewards(items, p.ResourceGainType.ACTIVITY_TIME_MEAL)
            else
                print('time is invalid. mealId, passMin, beginTime, endTime', mealId, passMin, v.beginTime, v.endTime)
                response(p.ErrorCode.ACTIVITY_TIME_INVALID, mealId)
                return
            end
            break
        end
    end

    -- send update
    response(p.ErrorCode.SUCCESS, mealId)
    activity.sendActivityTimeMealInfoUpdate()
end

function activity.cs_activity_target_draw(mealId, session)
    local function response(result, activityId, targetId)
        agent.replyPktout(session, p.SC_ACTIVITY_TARGET_DRAW_RESPONSE, result, activityId, targetId)
    end

    local aty = getVaildActivityById(activityId)
    if not aty and aty.type == p.ActivityType.TARGET then
        print('vaild activity not exist', activityId, targetId)
        response(p.ErrorCode.ACTIVITY_NOT_EXIST, activityId, targetId)
        return
    end
    local targetConf = nil
    for _, v in ipairs(aty.config.targetList) do
        if v.targetId == targetId then
            targetConf = v
            break
        end
    end
    if not targetConf then
        print('target conf not exist', activityId, targetId)
        response(p.ErrorCode.ACTIVITY_NOT_EXIST, activityId, targetId)
        return
    end

    local aTarget = activity.targetList
    local tInfo = aTarget[activityId]
    if tInfo == nil and targetConf.condType == p.ActivityCondType.VIP then
        tInfo = targetInfo:new({activityId = activityId})
        activity.targetList[activityId] = tInfo
    end
    if not tInfo then
        print('target info not exist', activityId, targetId)
        response(p.ErrorCode.ACTIVITY_INFO_NOT_EXIST, activityId, targetId)
        return
    end
    
    -- check progress
    local progress = 0
    if targetConf.condType == p.ActivityCondType.VIP then
        progress = vip.vipLevel()
    else
        progress = tInfo.condList[targetConf.condType] or 0
    end
    if progress < targetConf.progress then
        print('progress not enough', activityId, targetId)
        response(p.ErrorCode.ACTIVITY_TARGET_PORGRESS_NOT_ENOUGH, activityId, targetId)
        return
    end

    -- check draw
    if tInfo.drawnList[targetId] == true then
        print('activity target has drawn', activityId, targetId)
        response(p.ErrorCode.ACTIVITY_HAS_DRAWN, activityId, targetId)
        return
    end

    -- check need
    if targetConf.resCount > 0 and not user.isResourceEnough(targetConf.resType, targetConf.resCount) then
        print('activity target draw need resource not enough', activityId, targetId)
        response(p.ErrorCode.ACTIVITY_RESOURCE_NOT_ENOUGH, activityId, targetId)
        return
    end

    -- remove resource 
    user.removeResource(targetConf.resType, targetConf.resCount, p.ResourceConsumeType.ACTIVITY_TARGET)

    -- draw
    tInfo.drawnList[targetId] = true
    tInfo.sync = false
    drawRewards(targetConf.rewards)

    response(p.ErrorCode.SUCCESS, activityId, targetId)
    activity.sendActivityTargetInfoUpdate()
end

function activity.cs_activity_exchange_exchange(activityId, exchangeId, session)
    local function response(result, activityId, exchangeId)
        agent.replyPktout(session, p.SC_ACTIVITY_EXCHANGE_EXCHANGE_RESPONSE, result, activityId, exchangeId)
    end

    local aty = getVaildActivityById(activityId)
    if not aty and aty.type == p.ActivityType.EXCHANGE then
        print('vaild activity not exist', activityId, exchangeId)
        response(p.ErrorCode.ACTIVITY_NOT_EXIST, activityId, exchangeId)
        return
    end
    local exchangeConf = nil
    for _, v in ipairs(aty.config.exchangeList) do
        if v.exchangeId == exchangeId then
            exchangeConf = v
            break
        end
    end
    if not exchangeConf then
        print('exchange conf not exist', activityId, exchangeId)
        response(p.ErrorCode.ACTIVITY_NOT_EXIST, activityId, exchangeId)
        return
    end

    local aExchange = activity.exchangeList
    local eInfo = aExchange[activityId]
    if not eInfo then
        eInfo = exchangeInfo:new({activityId = activityId})
        activity.exchangeList[activityId] = eInfo
    end
    
    -- check times
    local times = eInfo.exTimesList[exchangeId] or 0
    if exchangeConf.exTimesMax > 0 and times >= exchangeConf.exTimesMax then
        print('times >= max', activityId, exchangeId)
        response(p.ErrorCode.ACTIVITY_EXCHANGE_TIMES_FULL, activityId, exchangeId)
        return
    end

    -- check exchange need
    local needItems = {}
    for _, v in ipairs(exchangeConf.needItems) do
        needItems[v.itemId] = v.count
    end
    if not bag.checkItemsEnough(needItems) then
        print('activity exchange needItems not enough', activityId, exchangeId)
        response(p.ErrorCode.ACTIVITY_RESOURCE_NOT_ENOUGH, activityId, exchangeId)
        return
    end

    -- draw
    bag.removeItems(needItems, p.ResourceConsumeType.ACTIVITY_EXCHANGE)
    eInfo.exTimesList[exchangeId] = times + 1
    eInfo.sync = false

    local drops = {}
    for _, v in ipairs(exchangeConf.getItems) do
        table.insert(drops, t.createDropItem(v.itemId, v.count))
    end
    bag.pickDropItems(drops, p.ResourceGainType.ACTIVITY_EXCHANGE)

    response(p.ErrorCode.SUCCESS, activityId, exchangeId)
    activity.sendActivityExchangeInfoUpdate()
end


return activity
