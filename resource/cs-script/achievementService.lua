local M = {}
local cluster = require('cluster')
local dbo = require('dbo')
local utils = require('utils')
local p = require('protocol')
local t = require('tploader')
local timer = require('timer')
local framework = require('framework')
local achievementInfo = require('achievementInfo')
local misc = require('libs/misc')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()

local allianceService
local mapStub

local achievement = {
    achieves = {},
    isDirty = true,
}

local rawService
local impl = {}

-- framework node shutdown
framework.beforeShutdown(function(cb)
    M.queueJob(function()
        utils.debug('framework.beforeShutdown achievement')
        if M.timer ~= nil then
            M.timer:cancel()
            M.timer = nil
        end
        M.dbSave()
        cb()
    end, cb)
end)


function M.start(mapStub_)
    rawService = cluster.createService('achievement', impl)
    allianceService = require('allianceService')
    mapStub = mapStub_

    M.dbLoad()
    M.checkNewAchievement()
    M.timer = timer.setInterval(M.checkSave, 300*1000)
end

function M.queueJob(threadFun)
    jobExecutor:queue(threadFun)
end

function M.checkSave()
    M.queueJob(function()
        M.dbSave()
    end)
end

function M.dbLoad()
    local db = dbo.open(0)
    local rs = db:executePrepare('SELECT * FROM s_dict WHERE uid = 0 and k = ?', 'cs_achievement.achieves')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            local data = misc.deserialize(row.v)
            for _,v in pairs(data) do
                local tpl = t.achievement[v.tplId]
                if tpl ~= nil then
                    local info = achievementInfo:new(v)
                    info.tpl = tpl
                    if info.progress >= info.tpl.count and info.finishTime == 0 then
                        info:setProgress(info.tpl.count)
                        info:finish(timer.getTimestampCache()) 
                    end
                    achievement.achieves[v.tplId] = info
                end
            end
            break
        end
    end
end

function M.dbSave()
    if achievement.isDirty then
        local list = {}
        for _, info in pairs(achievement.achieves) do
            table.insert(list, {
                tplId = info.tpl.id,
                createTime = info.createTime,
                finishTime = info.finishTime,
                drawTime = info.drawTime,
                progress = info.progress
              })
            --print('achievement.dbSave tplId, total, progress = ' , info.tpl.id, info.tpl.count, info.progress)
        end

        local db = dbo.open(0)
        local k = 'cs_achievement.achieves'
        local v = utils.serialize(list)
        local rs = db:executePrepare('insert into s_dict(uid, k, v) values (0, ?, ?) on duplicate key update v=values(v)', k, v)
        if rs.ok then
            achievement.isDirty = false
        else
            utils.log('achievement db save achieves error')
        end
    end
end


function M.checkNewAchievement()
    local newFinish = false

    repeat 
        newFinish = false
        for tplId,tpl in pairs(t.achievement) do
            local a = achievement.achieves[tplId]
            if a  == nil then
                local canAccept = true

                if tpl.preAchievementId > 0 then
                    local pre = achievement.achieves[tpl.preAchievementId]
                    if pre == nil or not pre:isFinish() then
                        canAccept = false
                    end
                end

                if canAccept then
                    local info = achievementInfo:new({ tpl = tpl })
                    M.initAchievement(info)
                    achievement.achieves[tplId] = info
                    achievement.isDirty = true

                    if info:isFinish() then
                        newFinish = true
                    end
                end
            end
        end
    until (not newFinish)
end

function M.initAchievement(info)
    local count = 0
    if info.tpl.type == p.AchievementType.MAP_LAND then
        count = mapStub.mapGlobalData.castleCellCnt or 0
    elseif info.tpl.type == p.AchievementType.KILL_NPC then
        count = mapStub.mapGlobalData.killNpcCnt or 0
    elseif info.tpl.type == p.AchievementType.OCCUPY_RES then
        count = 0
        if mapStub.mapGlobalData.occupyRes then
            count = mapStub.mapGlobalData.occupyRes[info.tpl.param1] or 0
        end
    elseif info.tpl.type == p.AchievementType.OCCUPY_COUNTY then
        count = mapStub.mapGlobalData.occupyCountyCnt or 0
    elseif info.tpl.type == p.AchievementType.OCCUPY_PREFECTURE then
        count = mapStub.mapGlobalData.occupyPrefectureCnt or 0
    elseif info.tpl.type == p.AchievementType.OCCUPY_CHOW then
        count = mapStub.mapGlobalData.occupyChowCnt or 0
    elseif info.tpl.type == p.AchievementType.OCCUPY_CAPITAL then
        count = mapStub.mapGlobalData.occupyCapitalCnt or 0
    elseif info.tpl.type == p.AchievementType.ALLIANCE_NUM then
        count = allianceService.getAllianceCount(info.tpl.param1)
    elseif info.tpl.type == p.AchievementType.TOTAL_POWER then
        count = mapStub.mapGlobalData.totalPower or 0
    elseif info.tpl.type == p.AchievementType.GATHER_RES then
        count = mapStub.mapGlobalData.gatherCnt or 0
    elseif info.tpl.type == p.AchievementType.HERO_COUNT then
        count = 0
        if mapStub.mapGlobalData.heroCnt then
            count = mapStub.mapGlobalData.heroCnt[info.tpl.param1] or 0
        end
    end

    info:setProgress(count)

    if info.progress >= info.tpl.count and info.finishTime == 0 then
        info:setProgress(info.tpl.count)
        info:finish(timer.getTimestampCache()) 
    end
end

function M.advanceAchievement(info, step)
    if (info:isFinish()) then
        return
    end
    local old = info.progress
    info:setProgress(info.progress + step)
    if info.progress >= info.tpl.count then
        info:setProgress(info.tpl.count)
        info:finish(timer.getTimestampCache())
    end

    if old ~= info.progress then
        if info:isFinish() then
            M.checkNewAchievement()
        end

        achievement.isDirty = true
        -- publish it
        M.sendAchieveUpdate(info)
    end
end

function M.onMapGlobalDataUpdate(oldData, newData)
    repeat
        local achieves = {}
        local oldCount = 0
        for _, info in pairs(achievement.achieves) do
            oldCount = oldCount + 1
            table.insert(achieves,info)
        end

        for _, info in pairs(achieves) do
            if not info:isFinish() then
                local step = 0
                if info.tpl.type == p.AchievementType.MAP_LAND then
                    if newData.castleCellCnt - (oldData.castleCellCnt or 0) > 0 then
                        step = newData.castleCellCnt - info.progress
                    end
                elseif info.tpl.type == p.AchievementType.KILL_NPC then
                    if newData.killNpcCnt - (oldData.killNpcCnt or 0) > 0 then
                        step = newData.killNpcCnt - info.progress
                    end
                elseif info.tpl.type == p.AchievementType.OCCUPY_RES then
                    local newResCnt = 0
                    if newData.occupyRes then
                        newResCnt = newData.occupyRes[info.tpl.param1] or 0
                    end

                    local oldResCnt = 0
                    if oldData.occupyRes then
                        oldResCnt = oldData.occupyRes[info.tpl.param1] or 0
                    end

                    if newResCnt - oldResCnt > 0 then
                        step = newResCnt - info.progress
                    end
                elseif info.tpl.type == p.AchievementType.OCCUPY_COUNTY then
                    if newData.occupyCountyCnt - (oldData.occupyCountyCnt or 0) > 0 then
                        step = newData.occupyCountyCnt - info.progress
                    end
                elseif info.tpl.type == p.AchievementType.OCCUPY_PREFECTURE then
                    if newData.occupyPrefectureCnt - (oldData.occupyPrefectureCnt or 0) > 0 then
                        step = newData.occupyPrefectureCnt - info.progress
                    end
                elseif info.tpl.type == p.AchievementType.OCCUPY_CHOW then
                    if newData.occupyChowCnt - (oldData.occupyChowCnt or 0) > 0 then
                        step = newData.occupyChowCnt - info.progress
                    end
                elseif info.tpl.type == p.AchievementType.OCCUPY_CAPITAL then
                    if newData.occupyCapitalCnt - (oldData.occupyCapitalCnt or 0) > 0 then
                        step = newData.occupyCapitalCnt - info.progress
                    end
                elseif info.tpl.type == p.AchievementType.TOTAL_POWER then
                    if newData.totalPower - (oldData.totalPower or 0) > 0 then
                        step = newData.totalPower - info.progress
                    end
                elseif info.tpl.type == p.AchievementType.GATHER_RES then
                    if newData.gatherCnt - (oldData.gatherCnt or 0) > 0 then
                        step = newData.gatherCnt - info.progress
                    end
                elseif info.tpl.type == p.AchievementType.HERO_COUNT then
                    local newHeroCnt = 0
                    if newData.heroCnt then
                        newHeroCnt = newData.heroCnt[info.tpl.param1] or 0
                    end

                    local oldHeroCnt = 0
                    if oldData.heroCnt then
                        oldHeroCnt = oldData.heroCnt[info.tpl.param1] or 0
                    end

                    if newHeroCnt - oldHeroCnt > 0 then
                        step = newHeroCnt - info.progress
                    end
                end

                -- print('step+++++++++++++',step)
                if step > 0 then
                    M.advanceAchievement(info, step)
                end
            end
        end

        local newCount = 0
        for _, info in pairs(achievement.achieves) do
            newCount = newCount + 1
        end

    until (oldCount == newCount)
end

function M.onAllianceUpdate(oldLevel,newLevel)
    for _, info in pairs(achievement.achieves) do
        if info.tpl.type == p.AchievementType.ALLIANCE_NUM then
            if info.tpl.param1 > oldLevel and info.tpl.param1 <= newLevel then
                M.advanceAchievement(info, 1)
            end
        end
    end
end

function M.sendAchieveUpdate(info)
    for _, info in pairs(achievement.achieves) do
        if not info:isSync() then
            local data = {
                tplId = info.tpl.id,
                createTime = info.createTime,
                finishTime = info.finishTime,
                drawTime = info.drawTime,
                progress = info.progress,
            }
            rawService:publishToAll('advance_achievement', data)
            info:setSync(true)
            -- print('=======sendAchieveUpdate tplId, type, total, progress = ' , info.tpl.id, info.tpl.type, info.tpl.count, info.progress)
        end
    end
end

function impl.fetchAllAchieves()
    local list = {}
    for _, info in pairs(achievement.achieves) do
        local data = {
            tplId = info.tpl.id,
            createTime = info.createTime,
            finishTime = info.finishTime,
            drawTime = info.drawTime,
            progress = info.progress,
        }

        info:setSync(true)
        table.insert(list,data)
        -- print('=======fetchAllAchieves tplId, type, total, progress = ' , info.tpl.id, info.tpl.type, info.tpl.count, info.progress)
    end

    return list
end

return M

