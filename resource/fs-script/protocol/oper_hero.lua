local agent = ...
local p = require('protocol')

local pktHandlers = {}

pktHandlers[p.CS_HERO_DRAW] = function(pktin, session)
    local drawType, isTen, bid, achievementId = pktin:read('ibii')
    agent.hero.cs_hero_draw(drawType, isTen, bid, achievementId, session)
end

pktHandlers[p.CS_HERO_STAR_UPGRADE] = function(pktin, session)
    local heroId, bid = pktin:read("ii")
    agent.hero.cs_hero_star_upgrade(heroId, bid, session)
end

pktHandlers[p.CS_HERO_SKILL_UPGRADE] = function(pktin, session)
    local heroId, skillId = pktin:read("ii")
    agent.hero.cs_hero_skill_upgrade(heroId, skillId, session)
    
end

pktHandlers[p.CS_HERO_DEBRIS_TURN] = function(pktin, session)
    local turnType, size = pktin:read('ii')
    local heroDebrisList = {}
    for i = 1, size do
        local debrisId, count = pktin:read('ii')
        heroDebrisList[debrisId] = count
    end
    agent.hero.cs_hero_debris_turn(turnType, heroDebrisList, session)
end

pktHandlers[p.CS_HERO_CHANGE_TECHNICAL_SKILL] = function(pktin, session)
    local heroId, slot, skillId = pktin:read('iii')
    agent.hero.cs_hero_change_technical_skill(heroId, slot, skillId, session)
end

pktHandlers[p.CS_HERO_QUICK_RECOVER] = function(pktin, session)
    local heroId, tplId, count = pktin:read('iii')
    agent.hero.cs_hero_quick_recover(heroId, tplId, count, session)
end

pktHandlers[p.CS_HERO_SOUL_UPGRADE] = function(pktin, session)
    local heroId, soulId = pktin:read('ii')
    agent.hero.cs_hero_soul_upgrade(heroId, soulId, session)
end

pktHandlers[p.CS_HERO_UPGRADE] = function(pktin, session)
    local heroId, bid = pktin:read('ii')
    agent.hero.cs_hero_upgrade(heroId, bid, session)
end

return pktHandlers