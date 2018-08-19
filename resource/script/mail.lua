local dbo = require('dbo')
local t = require('tploader')

local mailInfo = {
    id = 0,     -- mail id
    parentId = 0,   -- mail parent id
    uid = 0,        -- user id
    otherUid = 0,   -- to or from user id
    type = 0,       -- mail type
    subType = 0,
    title = '',     -- 标题
    content = '',   -- 内容
    attachment = '',    -- 附件
    params = '',        -- 参数内容
    isLang = false,     --是否多语言
    isRead = false,     -- 是否已读
    isDraw = false,     -- 是否已领取
    deleteType = 0,     -- 删除类型 0存在 1用户删除 2过期删除
    createTime = 0,     -- 创建时间
    deleteTime = 0,     -- 删除时间
    reportId = 0,

    isSync = false,
    isDirty = false,
    -- global
}

function mailInfo:new(o)
    o = o or {}
    setmetatable(o,self)
    self.__index = self
    return o
end

local mail = {
}

function mail.newMailInfo(o)
    return mailInfo:new(o)
end

function mail.getTitleContentBySubType(mailSubType)
    local title, content = '', ''
    for _, v in pairs(t.mail) do
        if mailSubType == v.subType then
            title = v.title
            content = v.content
            break
        end
    end
    return title, content
end

function mail.getSummaryBySubType(mailSubType)
    local summary = ''
    for _, v in pairs(t.mail) do
        if mailSubType == v.subType then
            summary = v.summary
            break
        end
    end
    return summary
end

function mail.getIconBySubType(mailSubType)
    local imageIcon = ''
    for _, v in pairs(t.mail) do
        if mailSubType == v.subType then
            imageIcon = v.imageIcon
            break
        end
    end
    return imageIcon
end

function mail.getTitleBySubType(mailSubType)
    local title = ''
    for _, v in pairs(t.mail) do
        if mailSubType == v.subType then
            title = v.title
            break
        end
    end
    return title
end

return mail
