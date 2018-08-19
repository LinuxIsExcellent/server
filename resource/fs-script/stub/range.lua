local cluster = require('cluster')
local p = require('protocol')

local rangeTypes = {
    p.RangeType.ARENA_RANK,
    p.RangeType.BABEL_RANK,
    p.RangeType.BRONZE_HISTORY_RANK,
    p.RangeType.BRONZE_TODAY_RANK,
}

local rangeStub = {
    rawStub,

    rangeMap = {},
    userRank = {},  --[uid][rangeTypes] = rank
}

local subscriber = {
    ranges_update = function(ranges)
        if ranges == nil then
            return
        end
        rangeStub.rangeMap[ranges.type] = ranges
    end,

    user_rank_update = function(ranges)
        if ranges == nil then
            return
        end
        rangeStub.userRank = ranges
    end,
}

local rawStub

function rangeStub.connectService()
    rawStub = cluster.connectService('range@cs')
    rangeStub.rawStub = rawStub
    rawStub:setSubscriber(subscriber)
    -- fetch cs all ranges
    rangeStub.fetchAllRanges()
    rangeStub.fetchUserRank()
end

function rangeStub.fetchAllRanges()
    --print('rangeStub.fetchAllRanges ...')
    for i,v in ipairs(rangeTypes) do
        local ret, ranges = rawStub:call_fetch_all_ranges(v)
        if ret and ranges ~= nil then
            rangeStub.rangeMap[ranges.type] = ranges
        end
    end
end

function rangeStub.fetchUserRank()
    -- print('rangeStub.fetchUserRank ...')
    local ret, ranges = rawStub:call_fetch_user_rank()
    if ret and ranges ~= nil then
        rangeStub.userRank = ranges
    end
end

--
-- local
--

--return newVsn, range
function rangeStub.fetchRangeByType(type)
    return rangeStub.rangeMap[type]
end

function rangeStub.fetchUserRankByUid(uid)
    return rangeStub.userRank[uid] or {}
end

return rangeStub
