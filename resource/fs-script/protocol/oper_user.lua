local agent = ...
local p = require('protocol')

local pktHandlers = {}

pktHandlers[p.CS_SET_LANG_TYPE] = function(pktin, session)
    local lang = pktin:read('i') 
    agent.user.cs_set_lang_type(lang, session)
end

-- 设置或检查昵称
pktHandlers[p.CS_SET_OR_CHECK_NICKNAME] = function(pktin, session)
    local onlyCheck, nickname = pktin:read('bs')
    agent.user.cs_set_or_check_nickname(onlyCheck, nickname, session)
end

pktHandlers[p.CS_STAMINA_BUY] = function(pktin, session)
    agent.user.cs_stamina_buy(session)
end

pktHandlers[p.CS_ENERGY_BUY] = function(pktin, session)
    agent.user.cs_energy_buy(session)
end

pktHandlers[p.CS_GET_RANDOM_NAME] = function(pktin, session)
    local sex = pktin:read('i')
    agent.user.cs_get_random_name(sex, session)
end

pktHandlers[p.CS_SET_NAME] = function(pktin, session)
    local nickname = pktin:read('s')
    agent.user.cs_set_name(nickname, session)
end

pktHandlers[p.CS_STORY_ID_UPDATE] = function(pktin, session)
    local storyId = pktin:read('i')
    agent.user.cs_story_id_update(storyId, session)
end

pktHandlers[p.CS_FILL_RESOURCE] = function(pktin, session)
    agent.user.cs_fill_resource(session)
end

return pktHandlers