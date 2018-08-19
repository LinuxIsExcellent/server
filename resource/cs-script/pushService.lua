local M = {}
local t = require('tploader')
local p = require('protocol')
local cluster = require('cluster')
local dbo = require('dbo')
local http = require('http')
local timer = require('timer')
local utils = require('utils')
local misc = require('libs/misc')
local framework = require('framework')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()

local dataService = require('dataService')

local rawService
local impl = {}

local sending = false
local closing = false
local pushCount = 0
local pushQueue = {}
local pushPendingList = {}  -- <key=uid,table<index,pushInfo>>
local pushPendingIndex = {}  -- <key=uid, index>
local pushDataMap = {}  -- <key=uid,table<device,deviceToken, table<group,isPush>>>
local pushSwitchMap = {}  -- <key=uid,switch>
local delaySeconds = 5
local pushApi = ''

-- callback
local daySeconds = 24 * 60 * 60
local callbackDataMap = {}  -- <key=uid,table<dayIndex,lastPushTime>>
local callback = t.configure['callback'] or { 1, 3, 5}
local callbackMax = 3
for k,v in pairs(callback) do
    if v > callbackMax then
        callbackMax = v
    end
end

-- framework node shutdown
framework.beforeShutdown(function(cb)
    M.queueJob(function()
        utils.debug('framework.beforeShutdown push')
        if M.timer ~= nil then
            M.timer:cancel()
            M.timer = nil
        end
        closing = true
        utils.debug('framework.beforeShutdown push end')
        cb()
    end, cb)
end)

function M.start()
    rawService = cluster.createService('push', impl)
    M.timer = timer.setInterval(M.checkSend, 1000)
    M.loadDataMap()
    pushApi = 'http://' .. t.miscConf.hubSite.ip .. ':' .. tostring(t.miscConf.hubSite.port) .. '/push_api.php'
end

function M.queueJob(threadFun, errorFun)
    jobExecutor:queue(threadFun, errorFun)
end

function M.loadDataMap()
    local db = dbo.open(0)
    local rs = db:executePrepare('SELECT * FROM s_dict WHERE k = "push.data" or k = "callback.data"')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if row.k == 'push.data' then
                local data = misc.deserialize(row.v)
                if data.langType == p.LangType.ALL then
                    data.langType = p.LangType.CN
                end
                pushDataMap[row.uid] = data
            elseif row.k == 'callback.data' then
                callbackDataMap[row.uid] = misc.deserialize(row.v)
            end
        end
    end
end

function M.checkSend()
    local now = timer.getTimestampCache()

    -- check callback
    if now % 300 == 0 then
        M.queueJob(function()
            --print('check p.PushClassify.CALLBACK ...')
            local db = dbo.open(0)
            local ts1 = now - daySeconds
            local ts2 = now - daySeconds * 60
            local rs = db:executePrepare('SELECT uid, lastLoginTimestamp FROM s_user WHERE lastLoginTimestamp < ? and lastLoginTimestamp > ?', ts1, ts2)
            if rs.ok and rs.rowsCount > 0 then
                for _, row in ipairs(rs) do
                    local data = callbackDataMap[row.uid]
                    if not data then
                        data = { dayIndex = 0,  lastPushTime = 0 }
                        callbackDataMap[row.uid] = data
                    end
                    local days = math.floor((now - row.lastLoginTimestamp) / daySeconds)
                    if days > 0 and days < callbackMax  and days > data.dayIndex then
                        data.dayIndex = days
                        data.lastPushTime = now
                        local pushInfo = {  uid = row.uid,  classify = p.PushClassify.CALLBACK, tag = '',  cdId = 0, sendTime = now }
                        impl.appendPush(pushInfo)
                        --print('callback uid=', row.uid)
                    end
                end
            end
        end)
    end

    -- check pending
    if not closing then
        for uid, plist in pairs(pushPendingList) do
            for index,pushInfo in pairs(plist) do
                if now >= pushInfo.sendTime then
                    table.insert(pushQueue, pushInfo)
                    pushCount = pushCount + 1
                    pushPendingList[uid][index] = nil
                end
            end
        end
    end

    -- check sending
    if sending or closing then
        return
    end
    M.send()
end

function M.queueErrorFun()
    sending = false
end

function M.send()
    if pushCount == 0 then
        return
    end
    M.queueJob(function()
        M.sendInternal()
    end, M.queueErrorFun)
end

function M.sendInternal()
    --print('pushCount =', pushCount)
    if pushCount == 0 or closing then
        return
    end

    sending = true

    pushCount = pushCount - 1
    local pushInfo = table.remove(pushQueue, 1)
    local data = pushDataMap[pushInfo.uid]
    local switch = pushSwitchMap[pushInfo.uid]
    if switch == nil then
        switch = true
        pushSwitchMap[pushInfo.uid] = switch
    end
    --var_dump(data)
    if data and data.deviceToken ~= '' and switch then
        local shouldSend = true
        pushInfo.device = data.device
        pushInfo.deviceToken = data.deviceToken
        local title = 'Notification_Title'
        if data.channel == 'xy_yinhulwzt' then
            title = 'Notification_Title_001'
        elseif data.channel == 'xy_yinhuzssd' then
            title = 'Notification_Title_002'
        end
        pushInfo.title = t.getLangString(title, data.langType)
        pushInfo.message = t.getLangString('Notification_' .. tostring(pushInfo.classify), data.langType)
        -- alter message if nessi
        if pushInfo.classify == p.PushClassify.CASTLE_ATTACTED then
            -- 您的城市被X攻击了
            shouldSend = data.settings[p.PushGroup.FIGHT]
            if pushInfo.tag == 'Kingdom_Map_Neutral_city' then
                pushInfo.tag = t.getLangString(pushInfo.tag, data.langType)
            end
            pushInfo.message = string.gsub(pushInfo.message, '{0}', pushInfo.tag)
        elseif pushInfo.classify == p.PushClassify.CAMP_ATTACTED then
            -- 您的驻军被X攻击了
            shouldSend = data.settings[p.PushGroup.FIGHT]
            pushInfo.message = string.gsub(pushInfo.message, '{0}', pushInfo.tag)
        elseif pushInfo.classify == p.PushClassify.SCOUTED then
            -- 您被X侦查了
            shouldSend = data.settings[p.PushGroup.FIGHT]
            pushInfo.message = string.gsub(pushInfo.message, '{0}', pushInfo.tag)
        elseif pushInfo.classify == p.PushClassify.TROOP_BACK then
            -- 军队已返回城市
            shouldSend = data.settings[p.PushGroup.FIGHT]

        elseif pushInfo.classify == p.PushClassify.BUILDING_UP then
            -- X建筑升级完成
            shouldSend = data.settings[p.PushGroup.EVENT]
            local tpl = t.building[pushInfo.tag]
            local name = t.getLangString(tpl.name, data.langType)
            pushInfo.message = string.gsub(pushInfo.message, '{0}', name)
        elseif pushInfo.classify == p.PushClassify.TECHNOLOGY_UP then
            -- X科技已研究完成
            shouldSend = data.settings[p.PushGroup.EVENT]
            local tpl = t.technology[pushInfo.tag]
            local name = t.getLangString(tpl.name, data.langType)
            pushInfo.message = string.gsub(pushInfo.message, '{0}', name)
        elseif pushInfo.classify == p.PushClassify.ARMY_TRAINED then
            -- X部队已经完成训练
            shouldSend = data.settings[p.PushGroup.EVENT]
            local tpl = t.army[pushInfo.tag]
            local name = t.getLangString(tpl.name, data.langType)
            pushInfo.message = string.gsub(pushInfo.message, '{0}', name)
        elseif pushInfo.classify == p.PushClassify.TRAP_BUILDED then
            -- X陷阱已经建造完成
            shouldSend = data.settings[p.PushGroup.EVENT]
            local tpl = t.army[pushInfo.tag]
            local name = t.getLangString(tpl.name, data.langType)
            pushInfo.message = string.gsub(pushInfo.message, '{0}', name)
        elseif pushInfo.classify == p.PushClassify.ARMY_HEALED then
            -- 伤兵治疗已完成
            shouldSend = data.settings[p.PushGroup.EVENT]
        elseif pushInfo.classify == p.PushClassify.EQUIP_FORGED then
            -- X装备锻造完成
            shouldSend = data.settings[p.PushGroup.EVENT]
            local tpl = t.item[pushInfo.tag]
            local name = t.getLangString(tpl.name, data.langType)
            pushInfo.message = string.gsub(pushInfo.message, '{0}', name)
        elseif pushInfo.classify == p.PushClassify.ALLIANCE_EXPLORE_DONE then
            -- 联盟探险完成了
            shouldSend = data.settings[p.PushGroup.EVENT]

        elseif pushInfo.classify == p.PushClassify.ONLINE_REWARD then
            -- 货船入港，可以收取物资了
            shouldSend = data.settings[p.PushGroup.REWARD]
        elseif pushInfo.classify == p.PushClassify.ALLIANCE_GIFT then
            -- 有新的联盟礼物可以领取
            shouldSend = data.settings[p.PushGroup.REWARD]

        elseif pushInfo.classify == p.PushClassify.CALLBACK then
            -- 领主大人，你好久没来城堡看看了
            -- save to dict
            local v = utils.serialize(callbackDataMap[pushInfo.uid])
            dataService.appendData(p.DataClassify.DICT, { uid = pushInfo.uid, k = 'callback.data', v = v })
        end

        -- send push
        if shouldSend then
            --var_dump(pushInfo)
            local code, obj = http.postForString(pushApi, pushInfo)
            -- print('push return: ', code, obj, pushCount)
            if code ~= 200 then
                utils.log(string.format('sendInternal code=%i, obj=%s, pushCount=%s', code, obj, tostring(pushCount)))
            end
        end
    end

    sending = pushCount > 0

    if sending and not closing then
        M.sendInternal()
    end
end

function M.stopPush()
    closing = true
    pushCount = 0
    pushQueue = {}
    print('stopPush')
end

function M.appendPush(pushInfo)
    impl.appendPush(pushInfo)
end

--
-- push service api implement
--

-- pushInfo = {  uid = 1,  classify = 1, tag = 'some data ...',  cdId = 0, sendTime = 0 }

function impl.appendPush(pushInfo)
    --print('impl.appendPush ...')
    --var_dump(pushInfo)
    local now = timer.getTimestampCache()
    if now >= pushInfo.sendTime then
        table.insert(pushQueue, pushInfo)
        pushCount = pushCount + 1
    else
        pushInfo.sendTime = pushInfo.sendTime + delaySeconds
        -- add to pending
        local list = pushPendingList[pushInfo.uid]
        if not list then
            list = {}
            pushPendingList[pushInfo.uid] = list
            pushPendingIndex[pushInfo.uid] = 0
        end
        local index = pushPendingIndex[pushInfo.uid] + 1
        list[index] = pushInfo
        pushPendingIndex[pushInfo.uid] = index
    end
end

function impl.appendPushList(pushInfoList)
    for k,pushInfo in pairs(pushInfoList) do
        impl.appendPush(pushInfo)
    end
end

function impl.cancelPush(uid, cdId)
    --print('impl.cancelPush ...', uid, cdId)
    local list = pushPendingList[uid]
    if list then
        for k,v in pairs(list) do
            if v.uid == uid and v.cdId == cdId then
                list[k] = nil
                break
            end
        end
    end
end

function impl.speedupPush(uid, cdId, sendTime)
    --print('impl.speedupPush ...', uid, cdId, sendTime)
    local list = pushPendingList[uid]
    if list then
        for k,v in pairs(list) do
            if v.uid == uid and v.cdId == cdId then
                list[k].sendTime = sendTime + delaySeconds
                break
            end
        end
    end
end

function impl.syncData(uid, data)
    --print('impl.syncData ...')
    pushDataMap[uid] = data
    --pushPendingList[uid] = {}
    --pushPendingIndex[uid] = 0
end

function impl.syncSwitch(uid, switch)
    --print('impl.syncSwitch uid, switch=', uid, switch)
    pushSwitchMap[uid] = switch
end


return M

