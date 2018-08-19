local cluster = require('cluster')
local misc = require('libs/misc')
local utils = require('utils')
local t = require('tploader')
local loginStub = require('stub/login')
local achievementInfo = require('achievementInfo')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()

local achievementStub = {
    rawStub,
    achieves = {},
}

local subscriber = {
    advance_achievement = function(ach)
        local tpl = t.achievement[ach.tplId]
        if tpl ~= nil then
            local info = achievementInfo:new(ach)
            info.tpl = tpl
            achievementStub.achieves[info.tpl.id] = info
            loginStub.broadAchievementUpdate(info.tpl.id)
        end
    end,
}

local rawStub

function achievementStub.connectService()
    -- print("111111111111111122222222222222222222")
    rawStub = cluster.connectService('achievement@cs')
    achievementStub.rawStub = rawStub
    rawStub:setSubscriber(subscriber)

    achievementStub.fetchAllAchieves()
end

--to cs
function achievementStub.fetchAllAchieves()
    local ret, achieves = rawStub:call_fetchAllAchieves()
    if ret and achieves then
        -- print("------------------fetchAllAchieves-------------")
        for _,v in pairs(achieves) do
            local tpl = t.achievement[v.tplId]
            if tpl ~= nil then
                print(v.tplId)
                local info = achievementInfo:new(v)
                info.tpl = tpl
                achievementStub.achieves[v.tplId] = info
            end
        end
    end
end


return achievementStub