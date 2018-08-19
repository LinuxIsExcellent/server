local agent = ...
local hero = agent.hero
local p = require('protocol')

local pktHandlers = {}

pktHandlers[p.CS_BABEL_FIGHT] = function(pktin, session)
    local layer, size = pktin:read('ii')
    if size <= 0 then
        print('p.CS_BABEL_FIGHT...size=', size)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local armyList = {}
    for i=1, size do
        local heroId, armyType, count, position = pktin:read('iiii')
        print('p.CS_BABEL_FIGHT...heroId, armyType, count, position', heroId, armyType, count, position)
        if heroId > 0 and t.heros[heroId] then
            local level = agent.army.getArmyLevelByArmyType(armyType)
            armyList[heroId] = { heroId = heroId, armyType = armyType, level = level, count = count, position = position }
        end
    end
    agent.babel.cs_babel_fight(armyList, session)
end

pktHandlers[p.CS_BABEL_MOPUP] = function(pktin, session)
    agent.babel.cs_babel_mopup(session)
end

pktHandlers[p.CS_BABEL_RESET] = function(pktin, session)
    agent.babel.cs_babel_reset()
end

return pktHandlers