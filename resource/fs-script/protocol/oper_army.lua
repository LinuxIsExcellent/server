local agent = ...
local p = require('protocol')

local pktHandlers = {}

pktHandlers[p.CS_SAVE_TEAM_INFO] = function(pktin, session)
    local teamLists = {}
    local size = pktin:read('i')
    for i=1, size do
        local team, len = pktin:read('ii')
        local confList = {}
        for j=1, len do
        	local heroId, armyType, count, position = pktin:read('iiii')
        	table.insert(confList, {heroId = heroId, armyType = armyType, count = count, position = position})
        end
        if next(confList) then
        	table.insert(teamLists, {team = team, confList = confList})
        end
    end
   	agent.army.cs_save_team_info(teamLists, session) 
end

pktHandlers[p.CS_ARMY_FILL] = function(pktin, session)
    local team = pktin:read('i')
    agent.army.cs_army_fill(team, session)
end

pktHandlers[p.CS_SET_CASTLE_DEFENDER] = function(pktin, session)
    local isUp, team = pktin:read('bi')
    agent.army.cs_set_castle_defender(isUp, team, session)
end

pktHandlers[p.CS_DESTROY_TRAP] = function(pktin, session)
    local type, count = pktin:read('ii')
    agent.army.cs_destroy_trap(type, count, session)
end

return pktHandlers