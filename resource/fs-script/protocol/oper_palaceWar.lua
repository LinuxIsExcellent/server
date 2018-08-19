local agent = ...
local p = require('protocol')

local pktHandlers = {}

pktHandlers[p.CS_GET_SERVER_INFOS] = function(pktin)
    agent.map.mapService:forward(pktin)
end

pktHandlers[p.CS_PALACE_WAR_SUCCESSIVE_KINGS] = function(pktin)
    agent.map.mapService:forward(pktin)
end

pktHandlers[p.CS_PALACE_WAR_RECORDS] = function(pktin)
    agent.map.mapService:forward(pktin)
end

pktHandlers[p.CS_KING_GET_KINGDOM_SET_INFOS] = function(pktin)
    agent.map.mapService:forward(pktin)
end

pktHandlers[p.CS_KINGDOM_TITLE_LIST] = function(pktin, session)
	agent.palaceWar.cs_kingdom_title_list(session)
end

pktHandlers[p.CS_KING_CHOOSED_BY_ALLIANCE_LEADER] = function(pktin, session)
    local uid = pktin:read('i')
    agent.palaceWar.cs_king_choosed_by_alliance_leader(uid,session)
end

pktHandlers[p.CS_KING_GIVE_TITLE] = function(pktin, session)
    local tplid, uid = pktin:read('ii')
    agent.palaceWar.cs_king_give_title(tplid, uid, session)
end

pktHandlers[p.CS_KING_CANCEL_TITLE] = function(pktin, session)
    local tplid = pktin:read('i')
    agent.palaceWar.cs_king_cancel_title(tplid, session)
end

pktHandlers[p.CS_KING_CHANGE_SERVER_NAME] = function(pktin, session)
    local name = pktin:read('s')
    agent.palaceWar.cs_king_change_server_name(name, session)
end

pktHandlers[p.CS_KING_CHANGE_RESOURCE_NODE_RATE_ID] = function(pktin, session)
    local id = pktin:read('i')
    agent.palaceWar.cs_king_change_resource_node_rate_id(id, session)
end

pktHandlers[p.CS_KING_GET_GIFT_LEFT_COUNT] = function(pktin, session)
    agent.palaceWar.cs_king_get_gift_left_count(session)
end

pktHandlers[p.CS_KING_GET_GIFT_GIVEN_LIST] = function(pktin, session)
    agent.palaceWar.cs_king_get_gift_given_list(session)
end

pktHandlers[p.CS_KING_GIVE_GIFT] = function(pktin, session)
    local tplid, size = pktin:read('ii')
    local uids = {}
    for i = 1, size, 1 do
        local uid = pktin:read('i')
        table.insert(uids, uid)
    end
    agent.palaceWar.cs_king_give_gift(tplid, uids)
end

pktHandlers[p.CS_KING_GET_PLAYER_LIST_WHEN_GIVE] = function(pktin, session)
    local type, name = pktin:read('is')
    agent.palaceWar.cs_king_get_player_list_when_give(type, name, session)
end

return pktHandlers