local agent = ...
local p = require('protocol')

local pktHandlers = {}

pktHandlers[p.CS_MONTH_CARD_DRAW] = function(pktin, session)
    agent.charge.cs_month_card_draw(session)
end

return pktHandlers