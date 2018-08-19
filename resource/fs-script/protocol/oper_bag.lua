local agent = ...
local p = require('protocol')

local pktHandlers = {}

pktHandlers[p.CS_BAG_USE] = function(pktin, session)
	local bid, tplId, count, useGold, p1 = pktin:read("iiibs")
	agent.bag.cs_bag_use(bid, tplId, count, useGold, p1, session)
end

pktHandlers[p.CS_BAG_BUY] = function(pktin, session)
    local stroeId, count = pktin:read("ii")
    agent.bag.cs_bag_buy(stroeId, count, session)
end

pktHandlers[p.CS_BAG_SELL] = function(pktin, session)
    local bid, count = pktin:read("ii")
    agent.bag.cs_bag_sell(bid, count, session)
end

pktHandlers[p.CS_BAG_SYNTHESIZE] = function(pktin, session)
    local bid = pktin:read("i")
    agent.bag.cs_bag_synthesize(bid, session)
end

pktHandlers[p.CS_BAG_MELT] = function(pktin, session)
    local size = pktin:read('i')
    local list = {}
    for i=1, size do
        local bid, count = pktin:read('ii')
        table.insert(list, {bid = bid, count = count})
    end
    agent.bag.cs_bag_melt(list, session)
end

return pktHandlers