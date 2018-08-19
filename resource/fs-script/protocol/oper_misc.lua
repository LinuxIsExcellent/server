local agent = ...
local p = require('protocol')

local pktHandlers = {}

pktHandlers[p.CS_MISC_DRAW_ONLINE] = function(pktin, session)
    agent.mail.cs_misc_draw_online(session)
end

pktHandlers[p.CS_PLAYER_SET] = function(pktin, session)
    local list, size = {}, pktin:read('i')
    for i = 1, size do
        local type, switch = pktin:read('ib')
        list[type] = switch
    end
    agent.misc.cs_player_set(list)
end

pktHandlers[p.CS_MISC_DRAW_CODE] = function(pktin, session)
    local vcode = pktin:read("s")
    agent.misc.cs_misc_draw_code(vcode, session)
end

return pktHandlers