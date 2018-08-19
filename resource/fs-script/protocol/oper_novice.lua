local agent = ...
local p = require('protocol')

local pktHandlers = {}

pktHandlers[p.CS_NOVICE_SET] = function(pktin)
    local flagId = pktin:read('s')
    agent.novice.cs_novice_set(flagId)
end

return pktHandlers