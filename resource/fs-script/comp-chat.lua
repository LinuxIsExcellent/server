local agent = ...
local p = require('protocol')
local t = require('tploader')
local timer = require('timer')
local utils = require('utils')
local misc = require('libs/misc')
local trie = require('trie')
local chatStub = require('stub/chat')

local questInfo = require('quest/questInfo')
local questProducer = require('quest/quest-producer')

local user = agent.user
local alliance = agent.alliance
local dict = agent.dict
local quest = agent.quest
local army = agent.army

local chat = {
    blockList = {},
}

--聊天记录数量限制
local MAX_CHAT_KINGDOM = 10
local MAX_CHAT_ALLIANCE = 10
local CHAT_OPEN_LEVEL = t.configure['LimitChatLeve'] or 1

local badWords = { '充', '值', '返', '利', '折', '哲', '冲', '沖', 'Q', 'q', '扣', '群', '裙', '峮', '惠', '陶保', '淘宝' }

function chat.onInit()
    chat.dbLoad()

    chat.sendChatHistory()
    chat.sendBlockUpdate()
end

function chat.onClose()
    chat.dbSave()
end

function chat.onSave()
    chat.dbSave()
end

function chat.dbLoad()
    chat.blockList = dict.get('chat.blocklist') or {}
end

function chat.dbSave()
    dict.set('chat.blocklist', chat.blockList)
end

--
-- send
--
function chat.sendChatHistory()
    -- kingdom
    local kingdomChats = chatStub.kingdomChats
    if kingdomChats then
        local size = #kingdomChats
        if size > MAX_CHAT_KINGDOM then
            size = MAX_CHAT_KINGDOM
        end
        while size > 0 do
            local c = kingdomChats[size]
            if c ~= nil then
                chat.sendKingdomChat(c)
            end
            size = size - 1
        end
    end
    local allianceId = alliance.allianceId()
    if allianceId == nil or allianceId == 0 then
        return
    end
    local allianceChats = chatStub.allianceChats[allianceId]
    if allianceChats then
        local size = #allianceChats
        if size > MAX_CHAT_ALLIANCE then
            size = MAX_CHAT_ALLIANCE
        end
        while size > 0 do
            local c = allianceChats[size]
            if c ~= nil then
                chat.sendAllianceChat(c)
            end
            size = size - 1
        end
    end
end

function chat.sendKingdomChat(chatInfo)
    if chatInfo.type ~= p.ChatType.KINGDOM then
        return
    end
    -- check block
    if chat.blockList[chatInfo.from_uid] then
        return
    end
    chat.sendChat(chatInfo)
end

function chat.sendAllianceChat(chatInfo)
    if chatInfo.type ~= p.ChatType.ALLIANCE then
        return
    end
    -- check block
    if chat.blockList[chatInfo.from_uid] then
        return
    end
    chat.sendChat(chatInfo)
end

function chat.sendChat(chatInfo)
    -- print('chat.sendChat ...', chatInfo.type, chatInfo.subType, chatInfo.from_uid, chatInfo.from_nickname, chatInfo.vipLevel, chatInfo.headId, chatInfo.level, chatInfo.langType, chatInfo.allianceNickname, chatInfo.time, chatInfo.content, chatInfo.params)
    agent.sendPktout(p.SC_CHAT_RECV,  '@@1=i,2=i,3=i,4=s,5=i,6=i,7=i,8=i,9=s,10=i,11=s,12=s',
        chatInfo.type, chatInfo.subType, chatInfo.from_uid, chatInfo.from_nickname, chatInfo.vipLevel, chatInfo.headId, chatInfo.level, chatInfo.langType, chatInfo.allianceNickname, chatInfo.time, chatInfo.content, chatInfo.params)
end


function chat.sendBlockUpdate(updates, removes)
    local updateList = updates
    local removeList = removes or {}
    if not updateList then
        updateList = {}
        for _, v in pairs(chat.blockList) do
            --print('sendBlockUpdate ......', v.uid, v.headId)
            table.insert(updateList, v)
        end
    end
    agent.sendPktout(p.SC_CHAT_BLOCK_UPDATE, '@@1=[uid=i,nickname=s,headId=i,allianceNickname=s,blockTime=i],2=[uid=i]', updateList, removeList)
end

--
-- recv
--
function chat.cs_char_send(chatType, content)
    -- check ban
    if user.info.banChatTimestamp > timer.getTimestampCache() then
        return
    end

    -- check content
    if content == nil or content == '' then
        return
    end

    -- filter
    if chatType == p.ChatType.KINGDOM then
        if user.info.level < CHAT_OPEN_LEVEL then
            return
        end
        -- for k,v in pairs(badWords) do
        --     local pos = content:find(v)
        --     if pos then
        --         --print('filter content, word =', content, v)
        --         return
        --     end
        -- end
    end

    -- check bad word
    content = trie.filter(content)

    -- send
    local allianceId = alliance.allianceId()
    local allianceName = alliance.allianceName()
    local chatInfo = {}
    chatInfo.type = chatType
    chatInfo.subType = p.ChatSubType.NORMAL
    chatInfo.from_uid = user.uid
    chatInfo.from_nickname = user.info.nickname
    chatInfo.vipLevel = agent.vip.vipLevel()
    chatInfo.headId = user.info.headId
    chatInfo.level = user.info.level
    chatInfo.langType = user.info.langType
    chatInfo.allianceNickname = allianceName
    chatInfo.time = timer.getTimestampCache()
    chatInfo.content = content
    chatInfo.params = ''
    if chatType == p.ChatType.KINGDOM then
        -- 国家聊天
        local params = {}
        table.insert(params, { allianceId = allianceId, allianceNickname = allianceName })
        chatInfo.params = utils.serialize(params)
        chatStub.cast_kingdomChat(chatInfo)
    elseif chatType == p.ChatType.ALLIANCE then
        -- 联盟聊天
        chatStub.cast_allianceChat(allianceId, chatInfo)
    end
end

function chat.cs_chat_block(uid)
    agent.queueJob(function()
        local ret = 0
        repeat
            if chat.blockList[uid] then
                ret = 1
                break
            end
            -- find user info
            local info = agent.map.call_getPlayerInfo(uid)
            if not info then
                ret = 1
                print('###no this user...', uid)
                break
            else
                info.nickname = info.nickname or ''
                info.headId = info.headId or 0
                info.allianceNickname = info.allianceNickname or ''
            end
            local block = { uid = uid, nickname = info.nickname, headId = info.headId, allianceNickname = info.allianceNickname, blockTime = timer.getTimestampCache() }
            chat.blockList[uid] = block
            chatStub.cast_chatBlock(user.uid, block)

            chat.sendBlockUpdate({ block })
        until true
        agent.sendPktout(p.SC_CHAT_BLOCK_RESPONSE, ret)
    end)
end

function chat.cs_chat_unblock(uids, removeList)
    for k,v in pairs(uids) do
        if chat.blockList[v] then
            chat.blockList[v] = nil
        end
    end
    chat.sendBlockUpdate({}, removeList)
    
    agent.queueJob(function()
        chatStub.cast_chatUnblock(user.uid, uids)
        agent.sendPktout(p.SC_CHAT_UNBLOCK_RESPONSE, 0)
    end)
end

return chat
