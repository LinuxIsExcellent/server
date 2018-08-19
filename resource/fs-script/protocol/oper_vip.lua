local agent = ...
local p = require('protocol')

local pktHandlers = {}

pktHandlers[p.CS_VIP_TAKE_BOX] = function(pktin, session)
    local level = pktin:read("i")
    agent.vip.cs_vip_take_box(level)
end

return pktHandlers