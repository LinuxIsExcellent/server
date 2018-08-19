local agent = ...
local p = require('protocol')
local timer = require('timer')
local utils = require('utils')
local misc = require('libs/misc')
local rangeStub = require('stub/range')

local user = agent.user
local arena = agent.arena
local dict = agent.dict
local alliance = agent.alliance

local pktHandlers = {}

local range = {
    data = {},  --map<rangeType, lastRank>
}

function range.onInit()
    agent.registerHandlers(pktHandlers)
    range.dbLoad()
end

function range.onAllCompInit()
end

function range.onSave()
    range.dbSave()
end

function range.onClose()
    range.dbSave()
end

function range.dbLoad()
    range.data = dict.get("range.data") or {}
end

function range.dbSave()
    dict.set("range.data", range.data)
end

-- recv client packet
pktHandlers[p.CS_RANGE_FETCH] = function(pktin)
    local vsn, rangeType  = pktin:read("ii")
    local r = rangeStub.fetchRangeByType(rangeType)
    -- print('p.CS_RANGE_FETCH ... vsn, otherVsn, type', vsn, r.vsn, rangeType)
    if r ~= nil and vsn ~= r.vsn then
        -- print('p.CS_RANGE_FETCH ... send vsn, data', r.vsn, utils.serialize(r.data))
        local selfRank = 0
        local changeRank = 0
        local param1 = 0
        local lastRank = range.data[rangeType] or 0
        local allRange = r.data or {}
        local rangeList = {}
       if rangeType == p.RangeType.ARENA_RANK then
            for i, v in pairs(allRange) do
                if v.uid == user.uid then
                    selfRank = v.rank
                    param1 = v.heroPower
                end
                if i > 20 then
                    break
                end
                table.insert(rangeList, v)
            end
            if selfRank == 0 then
                local newRank, heroPower = arena.getArenaDefenseRankData()
                selfRank = newRank
                --战力
                param1 = heroPower
            end
        elseif rangeType == p.RangeType.BABEL_RANK then
            for i, v in pairs(allRange) do
                if v.uid == user.uid then
                    selfRank = v.rank
                    --层数
                    param1 = v.layer
                end
                if i > 100 and selfRank > 0 then
                    break
                end
                table.insert(rangeList, v)
            end
        --elseif rangeType == p.RangeType.BRONZE_HISTORY_RANK then
        --   for i, v in pairs(allRange) do
        --        if v.uid == user.uid then
        --            selfRank = v.rank
        --            --历史积分
        --            param1 = v.score
        --        end
        --        if i > 100 and selfRank > 0 then
        --            break
        --        end
        --        table.insert(rangeList, v)
        --    end
        --elseif rangeType == p.RangeType.BRONZE_TODAY_RANK then
        --    for i, v in pairs(allRange) do
        --        if v.uid == user.uid then
        --            selfRank = v.rank
        --            --今天积分
        --           param1 = v.score
        --        end
        --        if i > 50 and selfRank > 0 then
        --            break
        --        end
        --        table.insert(rangeList, v)
        --    end
        end
        if lastRank > 0 then
            changeRank = lastRank - selfRank
        -- elseif lastRank == 0 then
        --     changeRank = selfRank + lastRank
        end
        range.data[rangeType] = selfRank
--TODO:铜雀台排行榜需要去掉，排行榜协议需要修改
        local tempRange = {}
        for _, v in ipairs(rangeList) do
            if rangeType == p.RangeType.ARENA_RANK then
                table.insert(tempRange, { v.rank, v.uid, v.nickname, v.level, v.headId, v.aid, v.allianceName, v.allianceNickname, v.bannerId, v.heroPower })
            elseif rangeType == p.RangeType.BABEL_RANK then
                table.insert(tempRange, { v.rank, v.uid, v.nickname, v.level, v.headId, v.aid, v.allianceName, v.allianceNickname, v.bannerId, v.layer })
            --elseif rangeType == p.RangeType.BRONZE_HISTORY_RANK then
            --   table.insert(tempRange, { v.rank, v.uid, v.nickname, v.level, v.headId, v.aid, v.allianceName, v.allianceNickname, v.bannerId, v.score })
            --elseif rangeType == p.RangeType.BRONZE_TODAY_RANK then
            --    table.insert(tempRange, { v.rank, v.uid, v.nickname, v.level, v.headId, v.aid, v.allianceName, v.allianceNickname, v.bannerId, v.score })
            end
        end
        local data = utils.toJson(tempRange)
        -- print('p.CS_RANGE_FETCH...tempRange', utils.serialize(tempRange))
        -- print('p.CS_RANGE_FETCH...data', data)
        -- print('p.CS_RANGE_FETCH..rangeType, r.vsn, selfRank, changeRank, param1', rangeType, r.vsn, selfRank, changeRank, param1)
        agent.sendPktout(p.SC_RANGE_FETCH_RESPONSE, rangeType, r.vsn, selfRank, changeRank, param1, data)
    end
end

return range
