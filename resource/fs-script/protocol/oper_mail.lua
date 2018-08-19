local agent = ...
local p = require('protocol')

local pktHandlers = {}

pktHandlers[p.CS_MAIL_VIEW] = function(pktin)
    local mailType, mailId = pktin:read("ii")
    agent.mail.cs_mail_view(mailType, mailId)
end

pktHandlers[p.CS_MAIL_DELETE] = function(pktin)
    local mailType, size = pktin:read('ii')
    local mailIdList = {}
    for i=1, size do
        table.insert(mailIdList, pktin:read('i'))
    end
    agent.mail.cs_mail_delete(mailType, mailIdList)
end

pktHandlers[p.CS_MAIL_DRAW_ATTACHMENT] = function(pktin)
    local mailType, size = pktin:read('ii')
    local mailIdList = {}
    for i=1, size do
        table.insert(mailIdList, pktin:read('i'))
    end
    agent.mail.cs_mail_draw_attachment(mailType, mailIdList)
end

pktHandlers[p.CS_MAIL_ALLIANCE_SEND] = function(pktin)
    local content = pktin:read('s')
    agent.mail.cs_mail_alliance_send(content)
end

pktHandlers[p.CS_MAIL_PRIVATE_SEND] = function(pktin)
    local uid, content = pktin:read('is')
    agent.mail.cs_mail_private_send(uid, content)
end

pktHandlers[p.CS_MAIL_ALLIANCE_SHARE] = function(pktin)
    local mailType, mailId = pktin:read('ii')
    agent.mail.cs_mail_alliance_share(mailType, mailId)
end

pktHandlers[p.CS_MAIL_ALLIANCE_SHARE_VIEW] = function(pktin)
    local uid, mailId = pktin:read('ii')
    agent.mail.cs_mail_alliance_share_view(uid, mailId)
end

return pktHandlers