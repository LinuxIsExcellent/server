local cluster = require('cluster')
local misc = require('libs/misc')
local loginStub = require('stub/login')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()

local chatStub = {
    rawStub,

    kingdomChats = {},
    allianceChats = {},
    blockListMap = {},
}

local MAX_CHAT= 10

local subscriber = {
    kingdom_chat = function(chat)
        table.insert(chatStub.kingdomChats, 1, chat)
        chatStub.kingdomChats[MAX_CHAT + 1] = nil

        loginStub.broadKingdomChat(chat)
    end,

    alliance_chat = function(aid, chat)
        if chatStub.allianceChats[aid] == nil then
            chatStub.allianceChats[aid] = {}
        end
        table.insert(chatStub.allianceChats[aid], 1, chat)
        chatStub.allianceChats[aid][MAX_CHAT + 1] = nil

        loginStub.broadAllianceChat(aid, chat)
    end,

    block = function(uid, blockInfo)
        if not chatStub.blockListMap[uid] then
            chatStub.blockListMap[uid] = {}
        end
        local blockList = chatStub.blockListMap[uid]
        blockList[blockInfo.uid] = blockInfo
    end,

    unblock = function(uid, unblockUids)
        local blockList = chatStub.blockListMap[uid]
        if blockList then
            for _, v in pairs(unblockUids) do
                if blockList[v] then
                    blockList[v] = nil
                end
            end
        end
    end,
}

local rawStub

function chatStub.connectService()
    rawStub = cluster.connectService('chat@cs')
    chatStub.rawStub = rawStub
    rawStub:setSubscriber(subscriber)

    chatStub.fetchKingdomChats()
    chatStub.fetchAllianceChats()
    chatStub.fetchBlockListMap()
end


function chatStub.fetchKingdomChats()
    local ret, chats = rawStub:call_fetchKingdomChats()
    local list = {}
    if ret and chats then
        for i,v in ipairs(chats) do
            table.insert(list, v)
        end
    end
    chatStub.kingdomChats = list
end

function chatStub.fetchAllianceChats()
    local ret, chats = rawStub:call_fetchAllianceChats()
    local list = {}
    if ret and chats then
        for k,v in pairs(chats) do
            list[k] = v
        end
    end
    chatStub.allianceChats = list
end

function chatStub.fetchBlockListMap()
    local ret, blockListMap = rawStub:call_fetchBlockListMap()
    local list = {}
    if ret and blockListMap then
        for k,v in pairs(blockListMap) do
            list[k] = v
        end
    end
    chatStub.blockListMap = list
end

function chatStub.cast_kingdomChat(chat)
    rawStub:cast_kingdomChat(chat)
end

function chatStub.cast_allianceChat(aid, chat)
    rawStub:cast_allianceChat(aid, chat)
end

function chatStub.cast_chatBlock(uid, blockInfo)
    rawStub:cast_chatBlock(uid, blockInfo)
end

function chatStub.cast_chatUnblock(uid, unblockUids)
    rawStub:cast_chatUnblock(uid, unblockUids)
end

function chatStub.cast_privateMail(fromUid, fromUserLv, fromNickname, fronHeadId, fromLangType, fromAllianceNickname, toUid, toUserLv, toNickname, toHeadId, toAllianceNickname, content)
    rawStub:cast_privateMail(fromUid, fromUserLv, fromNickname, fronHeadId, fromLangType, fromAllianceNickname, toUid, toUserLv, toNickname, toHeadId, toAllianceNickname, content)
end

return chatStub
