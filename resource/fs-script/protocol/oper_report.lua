local agent = ...
local p = require('protocol')

local pktHandlers = {}

pktHandlers[p.CS_GET_REPORT] = function(pktin, session)
    local id = pktin:read('i')
    agent.report.cs_get_report(id)
end

return pktHandlers