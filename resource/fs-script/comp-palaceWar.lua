local agent = ...
local p = require('protocol')
local t = require('tploader')
local dbo = require('dbo')
local trie = require('trie')
local mapService
local user = agent.user
local alliance = agent.alliance
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()
local palaceWarStub = require('stub/palaceWar')
local allianceStub = require('stub/alliance')
local event = require('libs/event')

local pktHandlers = {}

local palaceWar = {
    titleId = 0,

    --event
    evtTitleUpdate = event.new(),   --(tplid)
}

-- -- framework node shutdown
-- framework.beforeShutdown(function(cb)
--     jobExecutor:queue(function()
--         utils.debug('framework.beforeShutdown palaceWar')
--         if timerChooseKing then
--             timerChooseKing:cancel()
--             timerChooseKing = nil
--         end
--         cb()
--     end, cb)
-- end)

function palaceWar.onInit()
    agent.registerHandlers(pktHandlers)
    --获取自己的称号
    palaceWar.titleId = palaceWarStub.findTitleByUid(user.uid)
    --print("palaceWar onInit titleId = ", palaceWar.titleId)
end

function palaceWar.onAllCompInit()
    agent.sendPktout(p.SC_TITLE_UPDATE, palaceWar.titleId)
end

function palaceWar.onReady()
    mapService = agent.map.mapService
end

function palaceWar.onClose()
end

function palaceWar.onUpdateTitle(tplid)
    -- print("comp-palaceWar.onUpdateTitle", tplid)
    palaceWar.titleId = tplid
    palaceWar.evtTitleUpdate:trigger(tplid)
    agent.sendPktout(p.SC_TITLE_UPDATE, tplid)
end

local function titlesDebug()
    --print('titlesDebug')
    local titles = palaceWarStub.titles
    for _, v in pairs(titles) do
        print("titlesDebug : tplid, uid, headId, nickname, lastCancelTimestamp", v.tplid, v.uid, v.headId, v.nickname, v.lastCancelTimestamp)
    end
end

function palaceWar.cs_kingdom_title_list(session)
    local titles = palaceWarStub.titles
    -- titlesDebug()
    local list = {}
    for _, v in pairs(titles) do
        local totalpower = 0
        jobExecutor:queue(function()
            local uInfo = agent.map.call_getPlayerInfo(v.uid)
            if not uInfo then
                print('###no this user...', v.uid)
                -- break
            else
                totalpower = uInfo.totalPower
            end
            -- var_dump(uInfo)
            --totalpower = uInfo.totalPower ~= nil and uInfo.totalPower or 1000
        end)
        v.totalpower = totalpower
        if v.uid ~= 0 then
            table.insert(list, v)
        end
    end

    agent.replyPktout(session, p.SC_KINGDOM_TITLE_LIST_RESPONSE, '@@1=[tplid=i,uid=i,headId=i,nickname=s,lastCancelTimestamp=i,totalpower=i]', list )
end

function palaceWar.cs_king_choosed_by_alliance_leader(uid, session)
    agent.queueJob(function()
        local ok, result = palaceWarStub.rawStub:call_chooseKing(user.uid, uid)
        if ok then
            -- print('choose king reply = ', result)
            agent.replyPktout(session, p.SC_KING_CHOOSED_BY_ALLIANCE_LEADER_RESPONSE, result)
            --result = 0 则客户端再次请求称号列表更新
        end
    end)
end

function palaceWar.cs_king_give_title(tplid, uid, session)
    agent.queueJob(function()
        local ok, result = palaceWarStub.rawStub:call_giveTitle(user.uid, tplid, uid)
        if ok then
            -- print("CS_KING_GIVE_TITLE***********", result)
            agent.replyPktout(session, p.SC_KING_GIVE_TITLE_RESPONSE, result)
            --result = 0 则客户端再次请求称号列表更新
        end
    end)
end

function palaceWar.cs_king_cancel_title(tplid, session)
    agent.queueJob(function()

        local ok, result = palaceWarStub.rawStub:call_cancelTitle(user.uid, tplid)
        if ok then
            agent.replyPktout(session, p.SC_KING_CANCEL_TITLE_RESPONSE, result)
            --result = 0 则客户端再次请求称号列表更新
        end
    end)
end

function palaceWar.cs_king_change_server_name(name, session)
    -- 0：未知错误 1：成功 2：不是国王 3：名字非法
    local function response(result)
        -- print('response', result)
        agent.replyPktout(session, p.SC_KING_CHANGE_SERVER_NAME_RESOPNSE, result)
    end
    agent.queueJob(function()
        if not palaceWarStub.isKing(user.uid) then
            response(2)
            return
        end
        local length = utf8.len(name)
        if length < 2 or length > 8  or trie.isContain(name) then
            response(3)
            return
        end
        local result = mapService:call('kingChangeServerName', name)
        if result then
            response(1)
        else
            response(0)
        end
    end)
end

function palaceWar.cs_king_change_resource_node_rate_id(id, session)
    agent.queueJob(function()
        local result = mapService:call('kingChangeResNodeRate', id)
        agent.replyPktout(session, p.SC_KING_CHANGE_RESOURCE_NODE_RATE_ID_RESPONSE, result)
    end)
end

function palaceWar.cs_king_get_gift_left_count(session)
    local gifts = palaceWarStub.gifts
    local list = {}
    local tpl = t.configure["palaceWarGift"]
    for tplid, v in pairs(tpl) do
        local temp = {}
        temp.tplid = tplid
        --礼包最大数量
        local giftcount = 0
        if gifts[tplid] then
            giftcount = #gifts[tplid]
        end
        temp.leftcount = v - giftcount
        table.insert(list, temp)
    end
    agent.replyPktout(session, p.SC_KING_GET_GIFT_LEFT_COUNT_RESPONSE, '@@1=[tplid=i,leftcount=i', list)
end

function palaceWar.cs_king_get_gift_given_list(session)
    local gifts = palaceWarStub.gifts
    local list = {}
    for tplid, v in pairs(gifts) do
        local temp = {}
        temp.tplid = tplid
        local uids = {}
        for _, uid in pairs(v) do
            table.insert(uids, {uid = uid})
        end
        temp.uids = uids
        table.insert(list, temp)
    end
    agent.replyPktout(session, p.SC_KING_GET_GIFT_GIVEN_LIST_RESPONSE, '@@1=[tplid=i,uids=[uid=i]]', list)
end

function palaceWar.cs_king_give_gift(tplid, uids, session)
    agent.queueJob(function()
        local ok, result = palaceWarStub.rawStub:call_giveGift(user.uid, tplid, uids)
        if ok then
            -- print("SC_KING_GIVE_GIFT_RESPONSE***********", result)
            agent.replyPktout(session, p.SC_KING_GIVE_GIFT_RESPONSE, result)
            --result = 0 则客户端再次请求称号列表更新-
        end
    end)
end

function palaceWar.cs_king_get_player_list_when_give(type, name ,session)
    agent.queueJob(function()
        local list = {}
        local function dbGetPlayerList(sql)
            local db = dbo.open(0)
            local rs = db:execute(sql)
            if rs.ok then
                for _, row in ipairs(rs) do
                    local isofficer = false;
                    if palaceWarStub.findTitleByUid(row.uid) ~= 0 then
                        isofficer = true;
                    end
                    table.insert(list, {
                        uid = row.uid,
                        headId = row.headId,
                        allianceNickname = row.allianceName,
                        nickname = row.nickname,
                        totalPower = row.totalPower,
                        langType = row.langType,
                        officer = isofficer,
                    }) 
                end
            end
        end
        -- if palaceWarStub.isKing(user.uid) then
            if type == 1 then
                local length = string.len(name) 
                if length > 2 and length < 16 then
                    local sql = 'SELECT uid, headId, allianceName, nickname, totalPower, langType FROM s_user WHERE nickname LIKE ' .. '"%' .. name .. '%"' .. ' LIMIT 100'
                    dbGetPlayerList(sql)
                end
            elseif type == 2 then
                local sql = 'SELECT uid, headId, allianceName, nickname, totalPower, langType FROM s_user ORDER BY uid ASC LIMIT 20'
                dbGetPlayerList(sql)
            end
        -- end
        agent.replyPktout(session, p.SC_KING_GET_PLAYER_LIST_WHEN_GIVE_RESPONSE, '@@1=[uid=i,headId=i,allianceNickname=s,nickname=s,totalPower=i,langType=i,officer=b]', list)
    end)
end


return palaceWar
