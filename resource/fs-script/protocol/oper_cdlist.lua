local agent = ...
local p = require('protocol')

local pktHandlers = {}

pktHandlers[p.CS_CDLIST_SPEED_UP] = function(pktin)
    local id, speedUpType, tplId, count = pktin:read('iiii')
    agent.cdList.cs_cdlist_speed_up(id, speedUpType, tplId, count)
end

return pktHandlers