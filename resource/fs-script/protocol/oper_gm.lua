local agent = ...
local p = require('protocol')

local pktHandlers = {}
pktHandlers[p.CS_CHAT_GM] = function(pktin, session)
    local cmd = pktin:read('s')
    agent.gm.command(cmd, session)
end

return pktHandlers