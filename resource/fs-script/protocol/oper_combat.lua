local agent = ...
local p = require('protocol')

local pktHandlers = {}

pktHandlers[p.CS_BATTLE_START] = function(pktin)
    local battleId, teamId = pktin:read('ii')
    agent.combat.cs_battle_start(battleId, teamId)
end

pktHandlers[p.CS_BATTLE_OVER] = function(pktin)
    local battleId,win = pktin:read('ii')
    local singleReleaseSpells = {}
    local singleSpellsLen = pktin:readInteger()
    for i = 1, singleSpellsLen do
        local round = pktin:readInteger()
        local spellBaseId = pktin:readInteger()
        table.insert(singleReleaseSpells, {round = round, spellBaseId = spellBaseId})
    end
    local mixedReleaseSpells = {}
    local mixedSpellsLen = pktin:readInteger()
    for i = 1, mixedSpellsLen do
        local bigRound = pktin:readInteger()
        local smallRound = pktin:readInteger()
        local spellRound = pktin:readInteger()
        local position = pktin:readInteger()
        local spellBaseId = pktin:readInteger()
        table.insert(mixedReleaseSpells, {bigRound = bigRound, smallRound = smallRound, spellRound = spellRound, position = position, spellBaseId = spellBaseId})
    end
    agent.combat.cs_battle_over(battleId, win, singleReleaseSpells,mixedReleaseSpells)
end

return pktHandlers