local agent = ...
local p = require('protocol')
local t = require('tploader')
local event = require('libs/event')
local timer = require('timer')
local utils = require('utils')
local achievementStub = require('stub/achievement')
local dict = agent.dict
local user = agent.user
local bag = agent.bag

local achievement = {
    draw = {},
    sync = {},

    evtAchievementFinish = event.new(), --(achievementId)
}

local pktHandlers = {}

function achievement.onInit()
    agent.registerHandlers(pktHandlers)
    achievement.dbLoad()
end

function achievement.onAllCompInit()
    achievement.sendAchievementUpdate()
end

function achievement.onClose()
    achievement.dbSave()
end

function achievement.onSave()
    achievement.dbSave()
end

function achievement.dbLoad()
    local draw = dict.get('achievement.draw') or {}
    for _, v in pairs(draw) do
        achievement.draw[v.tplId] = v.drawTime
    end
end


function achievement.dbSave()
    local  draw = {}
    for k, v in pairs(achievement.draw) do
        table.insert(draw, {
            tplId = k,
            drawTime = v,
          })
    end
    dict.set('achievement.draw', draw)
end

function achievement.isSync(achievementId)
    if achievement.sync[achievementId] == nil or achievement.sync[achievementId] == false then
        return false
    else
        return true
    end
end

function achievement.isFinish(achievementId)
    local ret = false
    local info = achievementStub.achieves[achievementId]
    if info then
        ret = info:isFinish()
    end
    return ret
end

function achievement.achievementUpdate(achievementId, isSync)
    achievement.setSync(achievementId, isSync)
    achievement.sendAchievementUpdate()
    --
    if achievement.isFinish(achievementId) then
        achievement.evtAchievementFinish:trigger(achievementId)
    end
end

function achievement.setSync(achievementId, isSync)
    achievement.sync[achievementId] = isSync
end

function achievement.sendAchievementUpdate()
    local  achieves = {}
    for _, info in pairs(achievementStub.achieves) do
        if not achievement.isSync(info.tpl.id) then
            local a = { tplId = info.tpl.id, progress = info.progress, finishTime = info.finishTime}
            if achievement.draw[info.tpl.id] == nil then
                a.draw = false
            else
                a.draw = true
            end
            achievement.setSync(info.tpl.id, true)

            table.insert(achieves, a)
            -- print('achievement.sendAchievementUpdate tplId, total, progress,draw = ' , info.tpl.id, info.tpl.count, info.progress, a.draw)
        end
    end

    agent.sendPktout(p.SC_ACHIEVEMENT_UPDATE,  '@@1=[tplId=i,progress=i,finishTime=i,draw=b]',  achieves)
end

function achievement.cs_achievement_draw(achievementId)
    local function drawResponse(id, result)
        --print('achievement drawResponse', id, result)
        agent.replyPktout(session, p.SC_ACHIEVEMENT_DRAW_RESPONSE, id, result)
    end

    local tpl = t.achievement[achievementId]
    if tpl == nil then
        print('achievement tpl not exist:', achievementId)
        drawResponse(achievementId, false)
        return
    end

    local a = achievementStub.achieves[tpl.id]
    if a == nil then
        print('achievement not accept tpl.type=', tpl.type)
        drawResponse(achievementId, false)
        return
    end

    if not a:isFinish() or achievement.draw[achievementId] ~= nil then
        print('achievement not finish or is draw:', achievementId)
        drawResponse(achievementId, false)
        return
    end

    achievement.setSync(achievementId, false)

    -- pickitem
    bag.pickDropItems(tpl.dropList, p.ResourceGainType.ACHIEVEMENT)


    --a:achievementInfo():draw(timer.getTimestampCache())
    achievement.draw[achievementId] = timer.getTimestampCache()

    achievement.sendAchievementUpdate()
    drawResponse(achievementId, true)
end

return achievement
