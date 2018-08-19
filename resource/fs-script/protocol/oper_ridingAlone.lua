local agent = ...
local p = require('protocol')

local pktHandlers = {}

pktHandlers[p.CS_RIDING_ALONE_FIGHT] = function(pktin, session)
    local sectionId , size = pktin:read('ii')
    -- print('p.CS_RIDING_ALONE_FIGHT...sectionId, size', sectionId, size)
    agent.ridingAlone.cs_riding_alone_fight(sectionId, size)
end

pktHandlers[p.CS_RIDING_ALONE_RESET] = function(pktin, session)
end

return pktHandlers