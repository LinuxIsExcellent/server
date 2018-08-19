local agent = ...
local p = require('protocol')

local pktHandlers = {}

pktHandlers[p.CS_STORE_BUY] = function(pktin, session)
	local storeType, id = pktin:read("ii")
    agent.store.cs_store_buy(storeType, id)
end

pktHandlers[p.CS_STORE_REFRESH] = function(pktin, session)
    local storeType = pktin:read("i")
    -- print('p.CS_STORE_REFRESH...storeType', storeType)
    agent.store.cs_store_refresh(storeType)
end

return pktHandlers