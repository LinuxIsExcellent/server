local agent = ...
local p = require('protocol')

local pktHandlers = {}


pktHandlers[p.CS_ACHIEVEMENT_DRAW] = function(pktin, session)
    local achievementId = pktin:read('i')
    agent.achievement.cs_achievement_draw(achievementId)
end

return pktHandlers