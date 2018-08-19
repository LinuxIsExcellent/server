local M = {}
local cluster = require('cluster')
local p = require('protocol')
local t = require('tploader')
local pubMail = require('mail')
local timer = require('timer')
local dbo = require('dbo')
local utils = require('utils')
local framework = require('framework')
local misc = require('libs/misc')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()

local loginService = require('loginService')
local dataService = require('dataService')
local httpService = require('httpService')
local mapStub

local rawService
local impl = {}

local activities = {}  -- all activities
local timerIndex = 0

-- framework node shutdown
framework.beforeShutdown(function(cb)
    M.queueJob(function()
        utils.debug('framework.beforeShutdown activitiy')
        M.timer:cancel()
        M.timer = nil
        M.save()
        cb()
    end, cb)
end)

local function publistAllActivitiesToClient()
    local list = {}
    for _, v in pairs(activities) do
        table.insert(list, v)
    end
    rawService:publishToAll('activities_update', list)
end

function M.start(mapStub_)
    rawService = cluster.createService('activity', impl)
    mapStub = mapStub_
    M.loadActivities()
    M.timer = timer.setInterval(M.update, 10 * 1000)
end

function M.queueJob(threadFun, errorFun)
    jobExecutor:queue(threadFun, errorFun)
end

function M.save()

end

function M.loadActivities()
    local listToMs = {}
    local function addListToMs(row)
        table.insert(listToMs, {
            id = row.id,
            type = row.type,
            openTime = row.openTime,
            closeTime = row.closeTime,
            bestItemList = row.bestItemList,
            config = row.config,
        })
    end
    local db = dbo.open(0)
    local now = timer.getTimestampCache()
    local rs = db:execute('SELECT * FROM s_activity where active=1 order by openTime desc')
    if rs.ok then
        local list = {}
        for k, row in ipairs(rs) do
            -- trim
            row.config = misc.strTrim(row.config)
            if row.timeType == p.ActivityTimeType.OPEN_SERVER then
                row.openTime = httpService.getOpenServerTime()
                row.closeTime = row.openTime + row.closeTime
            end
            row.openTimeP = row.openTime - row.preOpenSeconds
            row.closeTimeP = row.closeTime + row.afterCloseSeconds

            row.nextLoginGiveHeroConf = {}
            row.accumulateLoginConf = {}
            row.loginEverydayConf = {}

            if row.config == '' then
                row.config = '{}'
            end
            --print('row.config = ', row.config)
            row.config = utils.fromJson(row.config)
            -- check init and errors
            repeat
                -- wrong datas , skip it
                if row.config == nil then
                    utils.log('#bad activity (config) id=' .. tostring(row.id) .. ', remark=' .. row.remark)
                    break
                end
                if row.type == p.ActivityType.NEXT_LOGIN_GIVE_HERO then
                    -- 次登送将
                elseif row.type == p.ActivityType.ACCUMULATE_LOGIN then
                    -- 累计登录
                elseif row.type == p.ActivityType.LOGIN_EVERYDAY then
                    -- 每日登录
                elseif row.type == p.ActivityType.TIME_MEAL then
                    -- 限时大餐
                elseif row.type == p.ActivityType.TARGET then
                    -- 目标活动
                elseif row.type == p.ActivityType.EXCHANGE then
                    -- 兑换活动
                elseif row.type == p.ActivityType.WORLD_BOSS then
                    -- 世界BOSS
                end
                -- check active & accept
                if row.timeType == p.ActivityTimeType.ALL_TIME then
                    row.active = true
                    row.accept = true
                elseif now >= row.openTimeP and row.closeTimeP > now then
                    row.active = true
                    if now >= row.openTime and row.closeTime > now then
                        row.accept = true
                    else
                        row.accept = false
                    end
                else
                    row.active = false
                end

                addListToMs(row)
                -- print('#load activity id, remark, active, list.size=', row.id, row.remark, row.active, #list)
                -- print('###list.serialize', utils.serialize(list))
            until true

        end
        -- activities = list
    end

    -- publish
    publistAllActivitiesToClient()
    -- to ms
    mapStub.cast_activitiesUpdate(listToMs)
end

function M.reLoadActivities()
    M.queueJob(function()
        M.loadActivities()
    end)
end


function M.update()
    timerIndex = timerIndex + 1
    M.updateActivities()

    if timerIndex % 5 == 0 then
        M.queueJob(function()
            M.save()
        end)
    end
end

-- check if any activities to be sync to client
function M.updateActivities()
    local needPublish = false
    local now = timer.getTimestampCache()
    --print('updateActivities now=', now)
    for _, v in pairs(activities) do
        if v.timeType ~= p.ActivityTimeType.ALL_TIME then
            if v.active then
                if  now >=  v.closeTimeP then
                    v.active = false
                    needPublish = true
                    --print('change id, active=', v.id, v.active)
                end
            else
                if  now >= v.openTimeP and v.closeTimeP > now then
                    v.active = true
                    needPublish = true
                    --print('change id, active=', v.id, v.active)
                end
            end
            if not v.accept then
                if  now >= v.openTime and v.closeTime > now then
                    v.accept = true
                    needPublish = true
                    --print('change id, accept=', v.id, v.accept)
                end
            end
        end
    end
    if needPublish then
        publistAllActivitiesToClient()
    end
end

--
-- activity service api implement
--
function impl.fetch_all_activities()
    --print('impl.fetch_all_activities ...')
    local list = {}
    for _, v in pairs(activities) do
        table.insert(list, v)
    end
    return list
end


return M

