local M = {}
local p = require('protocol')
local cluster = require('cluster')
local dbo = require('dbo')
local timer = require('timer')
local framework = require('framework')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()

local rawService
local impl = {}

local saving = false
local closing = false
local logCount = 0
local logQueue = {}

for k,v in pairs(p.LogClassify) do
    logQueue[v] = {}
end

framework.beforeShutdown(function(cb)
    M.queueJob(function()
        print('framework.beforeShutdown log')
        if M.timer ~= nil then
            M.timer:cancel()
            M.timer = nil
        end
        closing = true
        M.saveInternal()
        cb()
    end, cb)
end)

local function getLogTable(classify)
    local tableName = nil
    if classify == p.LogClassify.LOGIN then
        tableName = 's_login'
    elseif classify == p.LogClassify.ITEM then
        tableName = 's_item'
    elseif classify == p.LogClassify.GOLD then
        tableName = 's_gold'
    elseif classify == p.LogClassify.SILVER then
        tableName = 's_silver'
    elseif classify == p.LogClassify.FOOD then
        tableName = 's_food'
    elseif classify == p.LogClassify.WOOD then
        tableName = 's_wood'
    elseif classify == p.LogClassify.IRON then
        tableName = 's_iron'
    elseif classify == p.LogClassify.STONE then
        tableName = 's_stone'
    elseif classify == p.LogClassify.STAMINA then
        tableName = 's_stamina'
    elseif classify == p.LogClassify.ROLE_UPGRADE then
        tableName = 's_role_upgrade'
    elseif classify == p.LogClassify.BUILDING_UPGRADE then
        tableName = 's_building_upgrade'
    elseif classify == p.LogClassify.VIP_UPGRADE then
        tableName = 's_vip_upgrade'
    elseif classify == p.LogClassify.SIGN then
        tableName = 's_sign'
    elseif classify == p.LogClassify.ONLINE_REWARD then
        tableName = 's_online_reward'
    elseif classify == p.LogClassify.MARCH then
        tableName = 's_march'
    elseif classify == p.LogClassify.MONSTER_SIEGE then
        tableName = 's_monster_siege'
    elseif classify == p.LogClassify.ALLIANCE_SCIENCE_DONATE then
        tableName = 's_alliance_science_donate'
    elseif classify == p.LogClassify.ALLIANCE_EXPLORE then
        tableName = 's_alliance_explore'
    elseif classify == p.LogClassify.TURNPLATE then
        tableName = 's_turnplate'
    elseif classify == p.LogClassify.ACTIVITY_TARGET_REWARD then
        tableName = 's_activity_target_reward'
    elseif classify == p.LogClassify.STORE then
        tableName = 's_store'
    elseif classify == p.LogClassify.TOWER then
        tableName = 's_tower'
    elseif classify == p.LogClassify.TECHNOLOGY then
        tableName = 's_technology'
    elseif classify == p.LogClassify.ARMY then
        tableName = 's_army'
    elseif classify == p.LogClassify.HERO then
        tableName = 's_hero'
    elseif classify == p.LogClassify.HERO_UPGRADE then
        tableName = 's_hero_upgrade'
    elseif classify == p.LogClassify.ARENA_WIN_POINT then
        tableName = 's_arena_win_point'
    elseif classify == p.LogClassify.ALLIANCE_SCORE then
        tableName = 's_alliance_score'
    elseif classify == p.LogClassify.BABEL_SCORE then
        tableName = 's_babel_score'
    elseif classify == p.LogClassify.BRONZE_SCORE then
        tableName = 's_bronze_score'
    elseif classify == p.LogClassify.SKILLPOINT then
        tableName = 's_skill_point'
    else
        print('wrong log classify = ', classify)
    end
    return tableName
end

function M.start()
    rawService = cluster.createService('log', impl)
    M.timer = timer.setInterval(M.checkSave, 500)
end

function M.queueJob(threadFun, errorFun)
    jobExecutor:queue(threadFun, errorFun)
end

function M.checkSave()
    if saving or closing then
        return
    end
    M.save()
end

function M.queueErrorFun()
    saving = false
end

function M.save()
    if logCount == 0 then
        return
    end
    M.queueJob(function()
        M.saveInternal()
    end, M.queueErrorFun)
end


function M.saveInternal()
    --print('logCount =', logCount)

    if logCount == 0 then
        return
    end

    saving = true

    local clasify = 0
    local toSaveList = {}

    for k,v in pairs(p.LogClassify) do
        local max = 1
        local queue = logQueue[v]
        local count = #queue
        if count > 200 then
            max = 200
        elseif count > 50 then
            max = 50
        end
        if count > 0 and closing then
            max = 200
        end

        if max > 1 then
            clasify = v
            repeat
                logCount = logCount - 1
                local log = table.remove(queue, 1)
                table.insert(toSaveList, log)
            until #toSaveList >= max or #queue == 0
            break
        end
    end

    if not next(toSaveList) then
        for k,v in pairs(p.LogClassify) do
            local queue = logQueue[v]
            if #queue > 0 then
                clasify = v
                logCount = logCount - 1
                local log = table.remove(queue, 1)
                table.insert(toSaveList, log)
                break
            end
        end
    end

    local saveCount = #toSaveList
    local saveBatch = saveCount > 1
    if saveCount > 0 then
        local tableName = getLogTable(clasify)
        if tableName then
            if saveBatch then
                print('save log  =', tableName, saveCount, logCount)
            end
            local db = dbo.open(3)
            local rs = db:insertBatch(tableName, toSaveList)
            if not rs.ok then
                utils.log('fail data: ' .. utils.toJson(toSaveList))
            end
        end
    end

    saving = saveBatch

    if closing then
        M.saveInternal()
    elseif saveBatch then
        M.save()
    end
end


--
-- log service api implement
--

function impl.appendLog(classify, logRow)
    --print('impl.appendLog ...')
    local list = logQueue[classify]
    if list then
        table.insert(list, logRow)
        logCount = logCount + 1
    end
end

return M

