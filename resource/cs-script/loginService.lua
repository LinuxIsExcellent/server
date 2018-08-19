local M = {}
local cluster = require('cluster')
local p = require('protocol')
local t = require('tploader')
local dbo = require('dbo')
local timer = require('timer')
local utils = require('utils')
local framework = require('framework')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()

local dataService = nil
local allianceService = nil
local activityService = nil

local rawService
local impl = {}


local userDict = {}
local sessionDict = {}
local onlineCount = 0
local fsIdDict = {}  -- <nodeid, fs_id>
local fsPayloadDict = {}  -- <fs_id, online_count>
local regDict = {}
local regCount = 0
local lastLogOnline = 0
local shutdown = false
local shutdownTimer = nil

-- framework node shutdown
framework.beforeShutdown(function(cb)
    M.queueJob(function()
        utils.debug('framework.beforeShutdown login')
        if M.timer then
            M.timer:cancel()
            M.timer = nil
        end
        cb()
    end, cb)
end)

function M.queueJob(threadFun, errorFun)
    jobExecutor:queue(threadFun, errorFun)
end

local nodeEventListener = { 
    onNodeUp = function(nodeid, name)
        local fs = string.match(name, '(%w+)-%d+')
        if fs == 'fs' then
            local fsId = tonumber(string.match(name, '%w+-(%d+)'))
            fsIdDict[nodeid] = fsId
            if fsPayloadDict[fsId] == nil then
                fsPayloadDict[fsId] = 0 -- be sure init
            end
        end
    end ,

    onNodeDown = function(nodeid, name)
        if name == 'ms' then
            utils.log('center_server will shutdown because the map_server shutdown!')
            M.kickAllPlayer()
            shutdownTimer = timer.setTimeout(function() framework.shutdown() end, 5 * 1000)
        else
            local fs = string.match(name, '(%w+)-%d+')
            if fs == 'fs' then
                -- print('frontServer is down', nodeid, fs, 'auto kick players in this node')
                local kickList = {}
                for uid, user in pairs(userDict) do
                    if user.mbid[1] == nodeid then
                        table.insert(kickList, uid)
                    end
                end
                for i, uid in ipairs(kickList) do
                    impl.logout(uid)
                end
                local fsId = tonumber(string.match(name, '%w+-(%d+)'))
                fsPayloadDict[fsId] = nil -- need to delete from payload
            end
        end
    end 
}

function M.start()
    cluster.addNodeEventListener(nodeEventListener)
    rawService = cluster.createService('login', impl)

    M.timer = timer.setInterval(M.timerUpdate, 1000)
end

function M.kickAllPlayer()
    shutdown = true
    rawService:publishToAll('cs_shutdown')

    for uid, user in pairs(userDict) do
        rawService:cast(user.mbid, 'kick', p.KickType.MAINTENANCE)
    end
end

function M.shutdownByGms(callback, afterSeconds)
    utils.log('call shutdownByGms afterSeconds = ' .. tostring(afterSeconds))
    if shutdown then
        callback(false)
        return
    end
    if shutdownTimer ~= nil then
        shutdown = false
        shutdownTimer:cancel()
        shutdownTimer = nil
    end
    if afterSeconds == -1 then
        -- do nothing
    elseif afterSeconds == 0 then
        framework.shutdown()
    else
        shutdownTimer = timer.setTimeout(function()
            framework.shutdown()
        end, afterSeconds * 1000)
    end
    callback(true)
end

function M.updateRegCount(uid)
    if not regDict[uid] then
        regDict[uid] = true
        regCount = regCount + 1
    end
end

function M.getOnlineCount()
    return onlineCount
end

function M.getFsPayload()
    local dict = {}
    for k, v in pairs(fsPayloadDict) do
        table.insert(dict, {
            fsId = k,
            count = v
        })
    end
    return dict
end

function M.getRegCount()
    return regCount
end

function M.timerUpdate()
    local now = timer.getTimestampCache()

    local shouldLog = false
    if now % 300 == 0 then
        shouldLog = true
        lastLogOnline = now
    end
    -- it may miss to record log
    if lastLogOnline > 0 and now - lastLogOnline > 300 then
        shouldLog = true
        lastLogOnline = lastLogOnline + 300
    end
    if shouldLog then
        M.queueJob(function()
            local db = dbo.open(3)
            local rs = db:executePrepare('INSERT INTO s_online(count, log_time) VALUES (?, ?) ', onlineCount, lastLogOnline)
        end)
    end
    --TEMP sql counts
    if now % 10 == 0 then
        M.queueJob(function()
            local db = dbo.open(0)
            local json = utils.getSqlCountsJson()
            local rs = db:executePrepare('INSERT INTO s_stat(k, v) VALUES ("sql_counts_cs", ?) ON DUPLICATE KEY UPDATE v=VALUES(v)', json)
        end)
    end
end


function M.findUser(uid)
    return userDict[uid]
end

function M.setSessionDict(uid, token)
    sessionDict[uid] = {
        token = token,
        expired = timer.getTimestampCache() + 86400
    }
end

function M.cast(uid, method, ...)
    --print("loginService.cast", uid, method)
    local user = userDict[uid]
    if user then
        rawService:cast(user.mbid, method, ...)
    end
end

--cast
function M.castMailReload(uid, mid)
    M.cast(uid, 'mail_reload', mid)
end

function M.castArenaRecordReload(uid, recordId)
    M.cast(uid, 'arena_record_reload', recordId)
end

function M.castTransportRecordReload(uid, recordId)
    M.cast(uid, 'transport_record_reload', recordId)
end

function M.castLock(uid, lockedTimestamp, lockedReason)
    M.cast(uid, 'lock', lockedTimestamp, lockedReason)
end

function M.castBanChat(uid, banChatTimestamp, banChatReason)
    M.cast(uid, 'ban_chat', banChatTimestamp, banChatReason)
end

function M.castCharge(uid, orderId)
    M.cast(uid, 'charge', orderId)
end

function M.castAllianceMultiExploreUpdate(uid, updateList, nextRefreshTime)
    M.cast(uid, 'alliance_multi_explore_update', updateList, nextRefreshTime)
end

--broad
function M.broadNoticeMessage(id, type, param)
    for _, user in pairs(userDict) do
        rawService:cast(user.mbid, 'notice_message', id, type, param)
    end
end


--
-- login service api implement
--
function impl.login(uid, mbid, token, isReconnect)
    if shutdown then
        --utils.debug('login fail :  shutdown=true')
        return false
    end

    M.updateRegCount(uid)

    if not dataService then
        dataService = require('dataService')
    end
    if dataService.isDataSaving(uid) then
        utils.debug('login fail :  isDataSaving uid=' .. tostring(uid))
        return false
    end

    local user = userDict[uid]
    if user ~= nil then
        -- kick
        local kickType = p.KickType.LOGIN_DUPLICATE
        if isReconnect == true then
            kickType = p.KickType.RECONNECT
        end
        rawService:cast(user.mbid, 'kick', kickType)
        utils.debug('login fail :  uid, kickType=' .. tostring(uid) .. ', ' .. tostring(kickType))
        return false
    else
        local now = timer.getTimestampCache()
        if isReconnect then
            local session = sessionDict[uid]
            -- bad session
            if session == nil or token ~= session.token or now > session.expired then
                --utils.debug('login fail :  bad session uid=' .. tostring(uid))
                return false
            end
        else
            M.setSessionDict(uid, token)
        end

        userDict[uid] = {
            uid = uid,
            mbid = mbid
        }
        onlineCount = onlineCount + 1
        local fsId = fsIdDict[mbid[1]]
        if fsId and fsPayloadDict[fsId] then
            fsPayloadDict[fsId] = fsPayloadDict[fsId] + 1
        end
        print('***** user_online', uid, onlineCount)
        rawService:publishToAll('user_online', uid, mbid, onlineCount)
        if not allianceService then
            allianceService = require('allianceService')
        end
        allianceService.onLogin(uid)
        return true
    end
end

function impl.logout(uid)
    local user = userDict[uid]
    if user ~= nil then
        userDict[uid] = nil
        onlineCount = onlineCount - 1
        local fsId = fsIdDict[user.mbid[1]]
        if fsId and fsPayloadDict[fsId] then
            fsPayloadDict[fsId] = fsPayloadDict[fsId] - 1
        end
        print('***** user_offline', uid, onlineCount)
        rawService:publishToAll('user_offline', uid, onlineCount)
        if not allianceService then
            allianceService = require('allianceService')
        end
        allianceService.onLogout(uid)
    end
end


return M

