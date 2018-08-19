local agent = ...
local p = require('protocol')

local pktHandlers = {}

pktHandlers[p.CS_QUEST_DRAW] = function(pktin, session)
    local questId = pktin:read('i')
    agent.quest.cs_quest_draw(questId)
end

pktHandlers[p.CS_STORY_QUEST_DRAW] = function(pktin, session)
    local questId = pktin:read('i')
    agent.quest.cs_story_quest_draw(questId)
end

return pktHandlers