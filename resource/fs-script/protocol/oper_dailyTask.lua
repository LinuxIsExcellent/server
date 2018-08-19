local agent = ...
local p = require('protocol')

local pktHandlers = {}

pktHandlers[p.CS_DAILY_TASK_REWARD_DRAW] = function(pktin, session)
    local function dailyTaskRewardDrawResponse(result)
        -- print('###dailyTaskRewardDrawResponse..result', result)
        agent.replyPktout(session, p.SC_DAILY_TASK_REWARD_DRAW_RESPONSE, result)
    end
    local targetId = pktin:read('i')
    -- print('###p.CS_DAILY_TASK_REWARD_DRAW..targetId',targetId)
    agent.dailyTask.cs_daily_task_reward_draw(targetId)
end

return pktHandlers