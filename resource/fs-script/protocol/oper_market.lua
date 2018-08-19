local agent = ...
local p = require('protocol')

local pktHandlers = {}

pktHandlers[p.CS_TRANSPORT] = function(pktin, session)
    local toUid, size = pktin:read("ii")
    local list = {}
    for i=1, size do
        local tplId, count = pktin:read('ii')
        table.insert(list, {tplId = tplId, count = count})
    end
    agent.market.cs_transport(toUid, list, session)
end

return pktHandlers