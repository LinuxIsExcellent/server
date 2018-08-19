local agent = ...
local p = require('protocol')
local arenaStub = require('stub/arena')

local pktHandlers = {}

pktHandlers[p.CS_ARENA_CHANGE_OPPONENT] = function(pktin)
    -- print('p.CS_ARENA_CHANGE_OPPONENT.....')
    arenaStub.cast_arenaChangeOpponent(agent.user.uid)
end

pktHandlers[p.CS_ARENA_FIGHT] = function(pktin, session)
	local toUid, size = pktin:read('ii')
	local arenaList = {}
	for i=1, size do
        local locate, heroId = pktin:read('ii')
        table.insert(arenaList, {locate = locate, heroId = heroId})
    end
    agent.arena.cs_arena_fight(toUid, arenaList, session)
end

pktHandlers[p.CS_ARENA_REVENGE] = function(pktin, session)
	local recordId, size = pktin:read('ii')
	local arenaList = {}
	for i=1, size do
        local locate, heroId = pktin:read('ii')
        table.insert(arenaList, {locate = locate, heroId = heroId})
    end
    agent.arena.cs_arena_revenge(recordId, arenaList, session)
end

pktHandlers[p.CS_ARENA_SET_DEFENSE] = function(pktin, session)
	local size = pktin:read('i')
	local arenaList = {}
	for i=1, size do
        local locate, heroId = pktin:read('ii')
        table.insert(arenaList, {locate = locate, heroId = heroId})
    end
    agent.arena.cs_arena_set_defense(arenaList, session)
end

pktHandlers[p.CS_ARENA_DREW_REWARD] = function(pktin, session)
	agent.arena.cs_arena_drew_reward(session)
end

pktHandlers[p.CS_ARENA_BATTLE_RECORD] = function(pktin, session)
    local vsn = pktin:read("i")
    agent.arena.cs_arena_battle_record(vsn, session)
end

pktHandlers[p.CS_ARENA_BATTLE_DETAILS] = function(pktin, session)
	local recordId = pktin:read('i')
	agent.arena.cs_arena_battle_details(recordId, session)
end

pktHandlers[p.CS_ARENA_CLEAR_BATTLE_CD] = function(pktin, session)
	agent.arena.cs_arena_clear_battle_cd(session)
end

pktHandlers[p.CS_ARENA_RESET_BATTLE_COUNT] = function(pktin, session)
	agent.arena.cs_arena_reset_battle_count(session)
end

return pktHandlers