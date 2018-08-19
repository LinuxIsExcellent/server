local agent = ...
local p = require('protocol')

local pktHandlers = {}

pktHandlers[p.CS_CHAT_SEND] = function(pktin)
    local chatType, content = pktin:read('is')
    agent.chat.cs_char_send(chatType, content)
end

pktHandlers[p.CS_CHAT_BLOCK] = function(pktin)
    local uid = pktin:read('i')
    agent.chat.cs_chat_block(uid)
end

pktHandlers[p.CS_CHAT_UNBLOCK] = function(pktin)
    local uids = {}
    local removeList = {}
    local len = pktin:read('i')
    for i =1, len do
        local uid = pktin:read('i')
        table.insert(uids, uid)
        table.insert(removeList, { uid = uid })
    end
    agent.chat.cs_chat_unblock(uids, removeList)
end

return pktHandlers