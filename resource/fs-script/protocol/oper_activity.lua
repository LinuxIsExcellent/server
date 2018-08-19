local agent = ...
local p = require('protocol')

local pktHandlers = {}

pktHandlers[p.CS_ACTIVITY_LOGIN_NEXT_DAY_DRAW] = function(pktin, session)
    -- TODO 暂时不做
    agent.activity.cs_activity_login_next_day_draw()
end

pktHandlers[p.CS_ACTIVITY_ACCUMULATE_LOGIN_DRAW] = function(pktin, session)
    local day = pktin:read('i')
    agent.activity.cs_activity_accumulate_login_draw(day, session)
end

pktHandlers[p.CS_ACTIVITY_LOGIN_EVERYDAY_DRAW] = function(pktin, session)
    local day = pktin:read('i')
    agent.activity.cs_activity_login_everyday_draw(day, session)
end

pktHandlers[p.CS_ACTIVITY_TIME_MEAL_DRAW] = function(pktin, session)
    local mealId = pktin:read('i')
    agent.activity.cs_activity_time_meal_draw(mealId, session)
end

pktHandlers[p.CS_ACTIVITY_TARGET_DRAW] = function(pktin, session)
    local activityId, targetId = pktin:read('ii')
    agent.activity.cs_activity_target_draw(mealId, session)
end

pktHandlers[p.CS_ACTIVITY_EXCHANGE_EXCHANGE] = function(pktin, session)
    local activityId, exchangeId = pktin:read('ii')
    agent.activity.cs_activity_exchange_exchange(activityId, exchangeId, session)
end

return pktHandlers