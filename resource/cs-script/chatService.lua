local M = {}
local cluster = require('cluster')
local p = require('protocol')
local t = require('tploader')
local timer = require('timer')
local dbo = require('dbo')
local pubMail = require('mail')
local utils = require('utils')
local misc = require('libs/misc')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()

local loginService = require('loginService')
local dataService = require('dataService')
local gmsService = nil

local rawService
local impl = {}

local kingdomChats = {}  -- array[max]  最新的记录排最前面
local allianceChats = {}  -- <key=aid,chatArray[max]>  最新的记录排最前面
local blockListMap = {}  -- <key=uid,table<uid,block>>
local MAX_CHAT = 10
local recentChatMap = {}  -- <key=uid,table<msg>>

function M.start()
    rawService = cluster.createService('chat', impl)
    M.loadBlockListMap()
end

function M.queueJob(threadFun, errorFun)
    jobExecutor:queue(threadFun, errorFun)
end

function M.loadBlockListMap()
    local db = dbo.open(0)
    local rs = db:executePrepare('SELECT * FROM s_dict WHERE k = "chat.blocklist" ')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            loginService.updateRegCount(row.uid)
            blockListMap[row.uid] = misc.deserialize(row.v)
        end
    end
end

function M.addChat(chatType, aid, chat)
    -- print('M.addChat...chatType, aid, chat', chatType, aid, utils.serialize(chat))
    if chat == nil then
        return
    end
    if aid == nil then
        aid = 0
    end
    -- add time
    chat.time = timer.getTimestampCache()
    if chatType == p.ChatType.KINGDOM then
        chat.type = p.ChatType.KINGDOM
        table.insert(kingdomChats, 1, chat)
        kingdomChats[MAX_CHAT + 1] = nil
        rawService:publishToAll('kingdom_chat', chat)
    elseif chatType == p.ChatType.ALLIANCE then
        if aid > 0 then
            if allianceChats[aid] == nil then
                allianceChats[aid] = {}
            end
            chat.type = p.ChatType.ALLIANCE
            local chats = allianceChats[aid]
            table.insert(chats, 1, chat)
            chats[MAX_CHAT + 1] = nil
            rawService:publishToAll('alliance_chat', aid, chat)
        end
    elseif chatType == p.ChatType.ALL then
        chat.type = p.ChatType.KINGDOM
        table.insert(kingdomChats, 1, chat)
        kingdomChats[MAX_CHAT + 1] = nil
        -- print('kingdom_chat...', utils.serialize(chat))
        rawService:publishToAll('kingdom_chat', chat)

        if aid > 0 then
            if allianceChats[aid] == nil then
                allianceChats[aid] = {}
            end
            chat.type = p.ChatType.ALLIANCE
            local chats = allianceChats[aid]
            table.insert(chats, 1, chat)
            chats[MAX_CHAT + 1] = nil
            -- print('alliance_chat...', utils.serialize(chat))
            rawService:publishToAll('alliance_chat', aid, chat)
        end
    end
end

--
-- chat service api implement
--
function impl.fetchKingdomChats()
    local list = {}
    for i,v in ipairs(kingdomChats) do
        table.insert(list, v)
    end
    return list
end

function impl.fetchAllianceChats()
    local list = {}
    for k,v in pairs(allianceChats) do
        list[k] = v
    end
    return list
end

function impl.fetchBlockListMap()
    local list = {}
    for k,v in pairs(blockListMap) do
        list[k] = v
    end
    return list
end

function impl.kingdomChat(chat)
    if chat == nil then
        return
    end
    if not recentChatMap[chat.from_uid] then
        recentChatMap[chat.from_uid] = {}
    end
    local recentList = recentChatMap[chat.from_uid]
    -- check limits
    local repeatCount = 0
    for k,v in pairs(recentList) do
        if chat.content == v then
            repeatCount = repeatCount + 1
        end
    end
    if repeatCount >= 4 then
        if not gmsService then
            gmsService = require('gmsService')
        end
        gmsService.banChat(function () end, chat.from_uid, 1800, 'BanSpeechReasons')
    end

    -- save to recent list
    table.insert(recentList, 1, chat.content)
    recentList[11] = nil

    -- add time
    chat.time = timer.getTimestampCache()
    -- save record
    table.insert(kingdomChats, 1, chat)
    kingdomChats[MAX_CHAT + 1] = nil
    -- publish it
    rawService:publishToAll('kingdom_chat', chat)
end

function impl.allianceChat(aid, chat)
    if chat == nil then
        return
    end
    -- add time
    chat.time = timer.getTimestampCache()
    -- save record
    if allianceChats[aid] == nil then
        allianceChats[aid] = {}
    end
    local chats = allianceChats[aid]
    table.insert(chats, 1, chat)
    chats[MAX_CHAT + 1] = nil
    -- publish it
    rawService:publishToAll('alliance_chat', aid, chat)
end


function impl.chatBlock(uid, blockInfo)
    if not blockListMap[uid] then
        blockListMap[uid] = {}
    end
    local blockList = blockListMap[uid]
    blockList[blockInfo.uid] = blockInfo
    -- publish it
    rawService:publishToAll('block', uid, blockInfo)
end

function impl.chatUnblock(uid, unblockUids)
    local blockList = blockListMap[uid]
    if blockList then
        for _, v in pairs (unblockUids) do
            if blockList[v] then
                blockList[v] = nil
            end
        end
    end
    -- publish it
    rawService:publishToAll('unblock', uid, unblockUids)
end

function impl.privateMail(fromUid, fromUserLv, fromNickname, fromHeadId, fromLangType, fromAllianceNickname, toUid, toUserLv, toNickname, toHeadId, toAllianceNickname, content)
    local params = {
        fromLanguage = fromLangType,
        fromUid = fromUid,
        fromUserLv = fromUserLv,
        fromNickname = fromNickname,
        fromHeadId = fromHeadId,
        fromAllianceNickname = fromAllianceNickname,
        toUid = toUid,
        toUserLv = toUserLv,
        toNickname = toNickname,
        toHeadId = toHeadId,
        toAllianceNickname = toAllianceNickname
    }

    -- from
    local mail = pubMail.newMailInfo()
    mail.type = p.MailType.PRIVATE
    mail.subType = p.MailSubType.PRIVATE_MAIL
    mail.createTime = timer.getTimestampCache()
    -- mail.title = pubMail.getTitleBySubType(mail.subType)
    --标题是对方的名字
    mail.title = toNickname
    mail.content = content
    mail.attachment = misc.serialize({})
    mail.params = misc.serialize(params)
    mail.battleData = ''
    mail.parentId = 0
    mail.uid = fromUid -- from
    mail.otherUid = toUid -- to
    mail.isLang = true
    mail.isRead = true
    mail.isDraw = true
    dataService.appendMailInfo(mail)

    -- to
    if fromUid ~= toUid then
        local toMail = pubMail.newMailInfo()
        toMail.type = p.MailType.PRIVATE
        toMail.subType = p.MailSubType.PRIVATE_MAIL
        toMail.createTime = timer.getTimestampCache()
        -- toMail.title = pubMail.getTitleBySubType(toMail.subType)
        --标题是对方的名字
        toMail.title = fromNickname
        toMail.content = content
        toMail.attachment = misc.serialize({})
        toMail.params = misc.serialize(params)
        toMail.battleData = ''
        toMail.parentId = 0
        toMail.uid = toUid -- to
        toMail.otherUid = fromUid -- from
        toMail.isLang = true
        toMail.isRead = false
        toMail.isDraw = true
        dataService.appendMailInfo(toMail)
    end
end

return M

