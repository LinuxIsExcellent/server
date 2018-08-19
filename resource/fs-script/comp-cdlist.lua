local agent = ...
local p = require('protocol')
local timer = require('timer')
local t = require('tploader')
local utils = require('utils')
local event = require('libs/event')
local pushStub = require('stub/push')
local user = agent.user
local dict = agent.dict

local cdInfo = {
    id = 0,
    type = 0,
    beginTime = 0,
    endTime = 0,
    sync = false,
}

local cdList = {
    isHour0Refresh = false,
    isHour5Refresh = false,
    isHour9Refresh = false,
    isHour12Refresh = false,
    isHour14Refresh = false,
    isHour17Refresh = false,
    isHour21Refresh = false,

    lastHour0Timestamp = 0,
    lastHour5Timestamp = 0,
    lastHour9Timestamp = 0,
    lastHour12Timestamp = 0,
    lastHour14Timestamp = 0,
    lastHour17Timestamp = 0,
    lastHour21Timestamp = 0,

    evtHour0Refresh = event.new(), 
    evtHour5Refresh = event.new(),
    evtHour9Refresh = event.new(),
    evtHour12Refresh = event.new(),
    evtHour14Refresh = event.new(),
    evtHour17Refresh = event.new(),
    evtHour21Refresh = event.new(),
    evtSpeed = event.new(),   
}
local cds = {}
local maxid = 0
local nextHour5Timestamp = 0
local cdEndTime = 0

function cdInfo:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end

function cdInfo:isCooling()
    return self.endTime > timer.getTimestampCache()
end

function cdInfo:addEndTime(time)
    self.endTime = self.endTime + time
    self.sync = false
end

function cdInfo:subEndTime(time)
    self.endTime = self.endTime - time
    self.sync = false
end

function cdInfo:setBeginTime(beginTime)
    self.beginTime = beginTime
    self.sync = false
end

function cdInfo:setEndTime(endTime)
    self.endTime = endTime
    self.sync = false
end

function cdInfo:setCb(cb, obj)
    assert(type(cb) == "function")
    self.cb = cb
    self.obj = obj
end

function cdInfo:getSumTime()
    local time = self.endTime - self.beginTime
    if time < 0 then time = 0 end
    return time
end

function cdInfo:getRemainTime()
    local remainTime = self.endTime - timer.getTimestampCache()
    if remainTime < 0 then remainTime = 0 end
    return remainTime
end

function cdInfo:finish()
    self.endTime = 0
    self.sync = false
    if self.cb then 
        if self.obj then
            self.cb(self.obj, self.id)
        else
            print('111')
            self.cb(self.id) 
        end
    end
end

function cdInfo:isFinish()
    return self.endTime == 0
end

function cdList.resetTimer()
    cdEndTime = 0
    for _, cd in pairs(cds) do
        if not cd:isFinish() and (cdEndTime == 0 or cdEndTime > cd.endTime) then
            cdEndTime = cd.endTime
        end
    end
end

function cdList.getCD(id)
    return cds[id]
end

function cdList.addCD(type, time, cb, obj)
    maxid = maxid + 1
    cds[maxid] = cdInfo:new({
        id = maxid,
        type = type,
        beginTime = timer.getTimestampCache(),
        endTime = timer.getTimestampCache() + math.ceil(time),
        cb = cb,
        obj = obj,
    })
    cdList.resetTimer()
    cdList.sendCDListUpdate()
    return maxid
end

function cdList.cancelCD(id)
    local cd = cds[id]
    if cd then
        cd.cb = nil
        cd.obj = nil
        cd:finish()
    end
    cdList.resetTimer()
    cdList.sendCDListUpdate()
end

function cdList.addEndTime(id, time)
    local cd = cds[id]
    if cd then
        cd:addEndTime(time)
    end
    cdList.resetTimer()
    cdList.sendCDListUpdate()
end
function cdList.subEndTime(id, time)
    local cd = cds[id]
    if cd then
        cd:subEndTime(time)
        if not cd:isFinish() and not cd:isCooling() then
            cd:finish()
        end
    end
    cdList.resetTimer()
    cdList.sendCDListUpdate()
end

function cdList.sendCDListUpdate()
    local list = {}
    for _, cd in pairs(cds) do
        if not cd.sync then
            cd.remainTime = cd:getRemainTime()
            cd.sumTime = cd:getSumTime()
            table.insert(list, cd)
            cd.sync = true
            print('$$$sendCDListUpdate id, remainTime', cd.id, cd.remainTime)
        end
    end
    if next(list) ~= nil then
        agent.sendPktout(p.SC_CDLIST_UPDATE, '@@1=[id=i,type=i,remainTime=i,sumTime=i]', list)
    end
end

function cdList.checkCDList()
    for _, cd in pairs(cds) do
        if not cd:isFinish() and not cd:isCooling() then
            cd:finish()
        end
    end
    cdList.resetTimer()
    cdList.sendCDListUpdate()
    for id, cd in pairs(cds) do
        if cd:isFinish() then
            cds[id] = nil
        end
    end
end

function cdList.resetNextHour5RefreshTime()
    local t = os.date('*t', timer.getTimestampCache())
    if t.hour < 5 then
        nextHour5Timestamp = os.time({year = t.year, month = t.month, day = t.day, hour = 5})
    else
        nextHour5Timestamp = os.time({year = t.year, month = t.month, day = t.day, hour = 5}) + 24 * 3600
    end
end

function cdList.getHour5RefreshCountdown()
    local countdown = nextHour5Timestamp - timer.getTimestampCache()
    -- print(nextHour5Timestamp, timer.getTimestampCache(), countdown)
    return countdown > 0 and countdown or 0
end

function cdList.dbLoad()
    local cds = cds
    local data = dict.get("cdlist")
    if data then
        maxid = data.maxid
        for _, v in ipairs(data.cds) do
            --print(v[1], v[2], v[3])
            cds[v[1]] = cdInfo:new({
                id = v[1],
                type = v[2],
                beginTime = v[3],
                endTime = v[4],
            })
        end
        cdList.lastHour0Timestamp = data.hour0Refresh or 0
        cdList.lastHour5Timestamp = data.hour5Refresh or 0
        cdList.lastHour9Timestamp = data.hour9Refresh or 0
        cdList.lastHour12Timestamp = data.hour12Refresh or 0
        cdList.lastHour14Timestamp = data.hour14Refresh or 0
        cdList.lastHour17Timestamp = data.hour17Refresh or 0
        cdList.lastHour21Timestamp = data.hour21Refresh or 0
    end

    if cdList.lastHour0Timestamp == 0 or timer.isTsShouldRefreshAtHour(cdList.lastHour0Timestamp, 0) then
        cdList.isHour0Refresh = true
        cdList.lastHour0Timestamp = timer.getTimestampCache()
    end
    if cdList.lastHour5Timestamp == 0 or timer.isTsShouldRefreshAtHour(cdList.lastHour5Timestamp, 5) then
        cdList.isHour5Refresh = true
        cdList.lastHour5Timestamp = timer.getTimestampCache()
    end
    if cdList.lastHour9Timestamp == 0 or timer.isTsShouldRefreshAtHour(cdList.lastHour9Timestamp, 9) then
        cdList.isHour9Refresh = true
        cdList.lastHour9Timestamp = timer.getTimestampCache()
    end
    if cdList.lastHour12Timestamp == 0 or timer.isTsShouldRefreshAtHour(cdList.lastHour12Timestamp, 12) then
        cdList.lastHour12Timestamp = true
        cdList.lastHour12Timestamp = timer.getTimestampCache()
    end
    if cdList.lastHour14Timestamp == 0 or timer.isTsShouldRefreshAtHour(cdList.lastHour14Timestamp, 12) then
        cdList.lastHour14Timestamp = true
        cdList.lastHour14Timestamp = timer.getTimestampCache()
    end
    if cdList.lastHour17Timestamp == 0 or timer.isTsShouldRefreshAtHour(cdList.lastHour17Timestamp, 17) then
        cdList.lastHour17Timestamp = true
        cdList.lastHour17Timestamp = timer.getTimestampCache()
    end
    if cdList.lastHour21Timestamp == 0 or timer.isTsShouldRefreshAtHour(cdList.lastHour21Timestamp, 21) then
        cdList.lastHour21Timestamp = true
        cdList.lastHour21Timestamp = timer.getTimestampCache()
    end
end

function cdList.dbSave()
    local data = {}
    data.maxid = maxid
    data.cds = {}
    for _, cd in pairs(cds) do
        if cd:isCooling() then
            table.insert(data.cds, {cd.id, cd.type, cd.beginTime, cd.endTime})
        end
    end
    data.hour0Refresh = cdList.lastHour0Timestamp
    data.hour5Refresh = cdList.lastHour5Timestamp
    data.hour9Refresh = cdList.lastHour9Timestamp
    data.hour12Refresh = cdList.lastHour12Timestamp
    data.hour14Refresh = cdList.lastHour14Timestamp
    data.hour17Refresh = cdList.lastHour17Timestamp
    data.hour21Refresh = cdList.lastHour21Timestamp
    dict.set("cdlist", data)
end

function cdList.onSave()
    cdList.dbSave()
end

function cdList.onInit()
    cdList.dbLoad()
    cdList.resetNextHour5RefreshTime()
end

function cdList.onAllCompInit()
    cdList.checkCDList()
end

function cdList.onClose()
    cdList.dbSave()
end

function cdList.onTimerUpdate(timerIndex)
    local now = timer.getTimestampCache()
    if timer.isTsShouldRefreshAtHour(cdList.lastHour0Timestamp, 0) then
        cdList.evtHour0Refresh:trigger()
        cdList.lastHour0Timestamp = now
    end
    if timer.isTsShouldRefreshAtHour(cdList.lastHour5Timestamp, 5) then
        cdList.resetNextHour5RefreshTime()
        cdList.lastHour5Timestamp = now
        cdList.evtHour5Refresh:trigger()
    end
    if timer.isTsShouldRefreshAtHour(cdList.lastHour9Timestamp, 9) then
        cdList.evtHour9Refresh:trigger()
        cdList.lastHour9Timestamp = now
    end
    if timer.isTsShouldRefreshAtHour(cdList.lastHour12Timestamp, 12) then
        cdList.evtHour12Refresh:trigger()
        cdList.lastHour12Timestamp = now
    end
    if timer.isTsShouldRefreshAtHour(cdList.lastHour14Timestamp, 14) then
        cdList.evtHour14Refresh:trigger()
        cdList.lastHour14Timestamp = now
    end
    if timer.isTsShouldRefreshAtHour(cdList.lastHour17Timestamp, 17) then
        cdList.evtHour17Refresh:trigger()
        cdList.lastHour17Timestamp = now
    end
    if timer.isTsShouldRefreshAtHour(cdList.lastHour21Timestamp, 21) then
        cdList.evtHour21Refresh:trigger()
        cdList.lastHour21Timestamp = now
    end
    if cdEndTime ~= 0 and now >= cdEndTime then
        cdList.checkCDList()
    end
end

--speed up
function cdList.cs_cdlist_speed_up(id, speedUpType, tplId, count)
    --print("###speed up...id, speedUpType, tplId, count", id, speedUpType, tplId, count)
    local cd = cds[id]
    -- print("###speed up...cd:isCooling, cd", utils.serialize(cd))
    if cd and cd:isCooling() then
        local switch = {}

        switch[p.CDSpeedUpType.FREE] = function()
            if cd.type ~= p.ItemPropType.BUILDING_SPEEDUP then
                return
            end
            -- TODO local freeTime = t.configure["InitialFreeTime"]
            -- if agent.vip.isVip() then
                local freeTime = agent.vip.tpl.freeCd * 60
            -- end
            local remainTime = cd:getRemainTime()
            if remainTime <= freeTime then
                cd:subEndTime(freeTime)
            else
                agent.sendPktout(p.SC_CDLIST_SPEED_UP_RESPONSE, p.ErrorCode.PUBLIC_FREE_INVALID)
                return false
            end
            return true
        end

        switch[p.CDSpeedUpType.GOLD] = function()
            local remainTime = cd:getRemainTime()
            local gold = t.timeToGold(remainTime)
            if gold > 0 then
                if user.removeResource(p.ResourceType.GOLD, gold, p.ResourceConsumeType.CD_SPEED_UP) then
                    cd:setEndTime(timer.getTimestampCache())
                    return true
                else
                    agent.sendPktout(p.SC_CDLIST_SPEED_UP_RESPONSE, p.ErrorCode.PUBLIC_GOLD_NOT_ENOUGH)
                end
            end

            return false
        end

        switch[p.CDSpeedUpType.PROP] = function()
        print("###speed up...CDSpeedUpType.PROP")
            if count <= 0 or count > 10000 then
                -- print('count is invalid', count)
                return false
            end
            local speedUpTime = 3 * 60 --默认
            local tpl = t.item[tplId]
            if tpl and (tpl.subType == p.ItemPropType.SPEEDUP or cd.type == tpl.subType) then
                if agent.bag.removeItem(tplId, count, p.ResourceConsumeType.ITEM_USE) then
                    print("111")
                    speedUpTime = tpl.param1 * count
                    cd:subEndTime(speedUpTime)
                    return true
                else
                    print("222")
                    agent.sendPktout(p.SC_CDLIST_SPEED_UP_RESPONSE, p.ErrorCode.PUBLIC_PROP_NOT_ENOUGH)
                    -- print('prop not enough')
                end
            end
            return false
        end

        if switch[speedUpType] then
            --print("###speedUpType", speedUpType)
            if switch[speedUpType]() then
                if not cd:isFinish() and not cd:isCooling() then
                    cd:finish()
                    -- push
                    pushStub.cancelPush(user.uid, id)
                else
                    -- push
                    local sendTime = timer.getTimestampCache() + cd:getRemainTime()
                    pushStub.speedupPush(user.uid, id, sendTime)
                end
                cdList.resetTimer()
                cdList.sendCDListUpdate()

                local noticeCode = 0
                if speedUpType == p.CDSpeedUpType.PROP then
                    noticeCode = p.ErrorCode.PUBLIC_PROP_USE_SUCCESS
                end
                cdList.evtSpeed:trigger(cd.type, speedUpType) 
                agent.sendPktout(p.SC_CDLIST_SPEED_UP_RESPONSE, noticeCode)
            end
        end

    end
end

return cdList
