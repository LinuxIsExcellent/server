local cluster = require('cluster')
local p = require('protocol')
local t = require('tploader')
local loginStub = require('stub/login')

local activityStub = {
    rawStub,

    activities = {},
}

-- 若活动需特殊处理在这
local function alterActivity(activity)
    -- if activity.type  == p.ActivityType. then
    -- end
    print('stub alterActivity ' .. activity.id)
    return activity
end

local subscriber = {
    activities_update = function(allActivities)
        if allActivities == nil then
            return
        end
        local list = {}
        for _, v in pairs(allActivities) do
            list[v.id] = alterActivity(v)
        end
        activityStub.activities = list
        -- notify to client
        loginStub.broadActivityUpdate()
    end,
}

local rawStub

function activityStub.connectService()
    rawStub = cluster.connectService('activity@cs')
    activityStub.rawStub = rawStub
    rawStub:setSubscriber(subscriber)
    -- fetch cs all activities
    activityStub.fetchAllActivities()
end

function activityStub.isServiceAvaiable()
    if not rawStub or not rawStub:isServiceAvaiable() then
        return false
    end
    return true
end

function activityStub.fetchAllActivities()
    local ret, allActivities = rawStub:call_fetch_all_activities()
    if ret and allActivities then
        local list = {}
        for _, v in pairs(allActivities) do
            list[v.id] = alterActivity(v)
        end
        activityStub.activities = list
    end
end

return activityStub
