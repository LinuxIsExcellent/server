local M = {}
local cluster = require('cluster')
local p = require('protocol')
local t = require('tploader')
local timer = require('timer')
local dbo = require('dbo')
local utils = require('utils')
local framework = require('framework')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()

local rawService
local impl = {}

local vsn = 200
local RANGE_LIMIT_LEVEL = t.configure['rangeLimitCastleLevel'] or 5

local rangeMap = {}
local rangeTypes = {
    p.RangeType.ARENA_RANK,
    p.RangeType.BABEL_RANK,
--    p.RangeType.BRONZE_HISTORY_RANK,
--    p.RangeType.BRONZE_TODAY_RANK,
}
local userRank = {} --[uid][rangeTypes] = rank


-- framework node shutdown
framework.beforeShutdown(function(cb)
    M.queueJob(function()
        utils.debug('framework.beforeShutdown range')
        if M.onLoadRangesTimer then
            M.onLoadRangesTimer:cancel()
            M.onLoadRangesTimer = nil
        end
        cb()
    end, cb)
end)

function M.queueJob(threadFun, errorFun)
    jobExecutor:queue(threadFun, errorFun)
end

local function genVsn()
    if vsn == 9999999 then
        vsn = 0
    end
    vsn = vsn + 1
    return vsn
end

local function setUserRank(uid, rangeType, rank)
    if userRank[uid] == nil then
        userRank[uid] = {}
    end
    userRank[uid][rangeType] = rank
end

local function dbLoadLordPowerRange(db)
    local rs = db:executePrepare('SELECT uid, nickname, headId, totalPower, allianceId, allianceName, allianceNickname, allianceBannerId FROM s_user where castleLevel >= ? and isJoinRank = 1 order by totalPower desc', RANGE_LIMIT_LEVEL)
    if rs.ok then
        rangeMap[p.RangeType.LORD_POWER] = {}
        local list = rangeMap[p.RangeType.LORD_POWER]
        for k, row in ipairs(rs) do
            table.insert(list, {
                rank = k,
                uid = row.uid,
                nickname = row.nickname,
                headId = row.headId,
                totalPower = row.totalPower,
                aid = row.allianceId,
                allianceName = row.allianceName,
                allianceNickname = row.allianceNickname,
                allianceBannerId = row.allianceBannerId,
                })
            --
            setUserRank(row.uid, p.RangeType.LORD_POWER, k)
        end
    end
end

local function dbLoadLordKillsRange(db)
    local rs = db:executePrepare('SELECT uid, nickname, headId, kills, allianceId, allianceName, allianceNickname, allianceBannerId FROM s_user where castleLevel >= ? and isJoinRank = 1 order by kills desc', RANGE_LIMIT_LEVEL)
    if rs.ok then
        rangeMap[p.RangeType.LORD_KILLS] = {}
        local list = rangeMap[p.RangeType.LORD_KILLS]
        for k, row in ipairs(rs) do
            table.insert(list, {
                rank = k,
                uid = row.uid,
                nickname = row.nickname,
                headId = row.headId,
                kills = row.kills,
                aid = row.allianceId,
                allianceName = row.allianceName,
                allianceNickname = row.allianceNickname,
                allianceBannerId = row.allianceBannerId,
                })
            --
            setUserRank(row.uid, p.RangeType.LORD_KILLS, k)
        end
    end
end

local function dbLoadLordCastleRange(db)
    local rs = db:executePrepare('SELECT uid, nickname, headId, castleLevel, allianceId, allianceName, allianceNickname, allianceBannerId FROM s_user where castleLevel >= ? and isJoinRank = 1 order by castleLevel desc', RANGE_LIMIT_LEVEL)
    if rs.ok then
        rangeMap[p.RangeType.LORD_CASTLE] = {}
        local list = rangeMap[p.RangeType.LORD_CASTLE]
        for k, row in ipairs(rs) do
            table.insert(list, {
                rank = k,
                uid = row.uid,
                nickname = row.nickname,
                headId = row.headId,
                castleLevel = row.castleLevel,
                aid = row.allianceId,
                allianceName = row.allianceName,
                allianceNickname = row.allianceNickname,
                allianceBannerId = row.allianceBannerId,
                })
            --
            setUserRank(row.uid, p.RangeType.LORD_CASTLE, k)
        end
    end
end

local function dbLoadLordLevelRange(db)
    local rs = db:executePrepare('SELECT uid, nickname, headId, level, allianceId, allianceName, allianceNickname, allianceBannerId FROM s_user where castleLevel >= ? and isJoinRank = 1 order by level desc', RANGE_LIMIT_LEVEL)
    if rs.ok then
        rangeMap[p.RangeType.LORD_LEVEL] = {}
        local list = rangeMap[p.RangeType.LORD_LEVEL]
        for k, row in ipairs(rs) do
            table.insert(list, {
                rank = k,
                uid = row.uid,
                nickname = row.nickname,
                headId = row.headId,
                level = row.level,
                aid = row.allianceId,
                allianceName = row.allianceName,
                allianceNickname = row.allianceNickname,
                allianceBannerId = row.allianceBannerId,
                })
            --
            setUserRank(row.uid, p.RangeType.LORD_LEVEL, k)
        end
    end
end

local function dbLoadAlliancePowerRange(db)
    local rs = db:executePrepare('SELECT id, name, nickname, bannerId, totalPower, leaderId, leaderName FROM s_alliance where disbandTime=0 order by totalPower desc')
    if rs.ok then
        rangeMap[p.RangeType.ALLIANCE_POWER] = {}
        local list = rangeMap[p.RangeType.ALLIANCE_POWER]
        for k, row in ipairs(rs) do
            table.insert(list, {
                rank = k,
                aid = row.id,
                allianceName = row.name,
                allianceNickname = row.nickname,
                allianceBannerId = row.bannerId,
                totalPower = row.totalPower,
                leaderId = row.leaderId,
                leaderName = row.leaderName,
                })
            --
            setUserRank(row.uid, p.RangeType.ALLIANCE_POWER, k)
        end
    end
end

local function dbLoadAllianceKillsRange(db)
    local rs = db:executePrepare('SELECT id, name, nickname, bannerId, totalKills, leaderId, leaderName FROM s_alliance where disbandTime=0 order by totalKills desc')
    if rs.ok then
        rangeMap[p.RangeType.ALLIANCE_KILLS] = {}
        local list = rangeMap[p.RangeType.ALLIANCE_KILLS]
        for k, row in ipairs(rs) do
            table.insert(list, {
                rank = k,
                aid = row.id,
                allianceName = row.name,
                allianceNickname = row.nickname,
                allianceBannerId = row.bannerId,
                totalKills = row.totalKills,
                leaderId = row.leaderId,
                leaderName = row.leaderName,
                })
            --
            setUserRank(row.uid, p.RangeType.ALLIANCE_KILLS, k)
        end
    end
end

local function dbLoadBabelScoreRange(db)
    local rs = db:executePrepare('SELECT uid, nickname, level, headId, allianceId, allianceName, allianceNickname, allianceBannerId, babelMaxLayer FROM s_user where isJoinRank = 1 and babelMaxLayer > 0 order by babelMaxLayer desc')
    if rs.ok then
        rangeMap[p.RangeType.BABEL_RANK] = {}
        local list = rangeMap[p.RangeType.BABEL_RANK]
        for k, row in ipairs(rs) do
            table.insert(list, {
                rank = k,
                uid = row.uid,
                nickname = row.nickname,
                level = row.level,
                headId = row.headId,
                aid = row.allianceId,
                allianceName = row.allianceName,
                allianceNickname = row.allianceNickname,
                bannerId = row.allianceBannerId,
                layer = row.babelMaxLayer,
                })
            --
            setUserRank(row.uid, p.RangeType.BABEL_RANK, k)
        end
    end
end

--local function dbLoadBronzeHistoryScoreRange(db)
--    local rs = db:executePrepare('SELECT uid, nickname, level, headId, allianceId, allianceName, allianceNickname, allianceBannerId, bronzeMaxScore FROM s_user where isJoinRank = 1 and bronzeMaxScore > 0 order by bronzeMaxScore desc')
--    if rs.ok then
--        rangeMap[p.RangeType.BRONZE_HISTORY_RANK] = {}
--        local list = rangeMap[p.RangeType.BRONZE_HISTORY_RANK]
--        for k, row in ipairs(rs) do
--            table.insert(list, {
--                rank = k,
--                uid = row.uid,
--                nickname = row.nickname,
--                level = row.level,
--               headId = row.headId,
--                aid = row.allianceId,
--                allianceName = row.allianceName,
--                allianceNickname = row.allianceNickname,
--                bannerId = row.allianceBannerId,
--                score = row.bronzeMaxScore,
--                })
--            --
--           setUserRank(row.uid, p.RangeType.BRONZE_HISTORY_RANK, k)
--        end
--    end
--end

--local function dbLoadBronzeTodayScoreRange(db)
--   local now = timer.getTimestampCache()
--    local t = os.date('*t', now)
--    local compTimestamp = os.time({year = t.year, month = t.month, day = t.day, hour = 0, min = 0, sec = 0})
--    -- local compTimestamp = timer.todayZeroTimestamp()
--    local rs = db:executePrepare('SELECT uid, nickname, level, headId, allianceId, allianceName, allianceNickname, allianceBannerId, bronzeTodayScore FROM s_user where isJoinRank = 1 and bronzeTodayScore > 0 and bronzeTodayTimestamp >= ? order by bronzeTodayScore desc', compTimestamp)
--    if rs.ok then
--        rangeMap[p.RangeType.BRONZE_TODAY_RANK] = {}
--        local list = rangeMap[p.RangeType.BRONZE_TODAY_RANK]
--        for k, row in ipairs(rs) do
 --           table.insert(list, {
--                rank = k,
--                uid = row.uid,
--                nickname = row.nickname,
--                level = row.level,
--                headId = row.headId,
--                aid = row.allianceId,
--                allianceName = row.allianceName,
--                allianceNickname = row.allianceNickname,
--                bannerId = row.allianceBannerId,
--                score = row.bronzeTodayScore,
--                })
 --           -- --
--            -- setUserRank(row.uid, p.RangeType.BRONZE_TODAY_RANK, k)
--        end
--    end
--end

local function dbLoadArenaRankRange(db)
    local rs = db:executePrepare('SELECT uid, nickname, level, headId, aid, allianceName, allianceNickname, bannerId, heroPower FROM s_arena where rank > 0 order by rank asc')
    if rs.ok then
        rangeMap[p.RangeType.ARENA_RANK] = {}
        local list = rangeMap[p.RangeType.ARENA_RANK]
        for k, row in ipairs(rs) do
            table.insert(list, {
                rank = k,
                uid = row.uid,
                nickname = row.nickname,
                level = row.level,
                headId = row.headId,
                aid = row.aid,
                allianceName = row.allianceName,
                allianceNickname = row.allianceNickname,
                bannerId = row.bannerId,
                heroPower = row.heroPower,
                })
            --
            setUserRank(row.uid, p.RangeType.ARENA_RANK, k)
        end
    end
end

local function publistAllRangeToClient()
    for k,v in pairs(rangeMap) do
        rawService:publishToAll('ranges_update', { vsn = vsn, type = k, data = v })
    end
end

local function publistUserRankToClient()
    rawService:publishToAll('user_rank_update', userRank)
end


function M.start()
    rawService = cluster.createService('range', impl)
    M.onLoadRangesTimer = timer.setInterval(M.onLoadRanges, 60 * 60 * 1000)

    M.loadRanges()
end

function M.loadRanges()
    -- new vsn
    genVsn()
    local db = dbo.open(0)

    dbLoadBabelScoreRange(db)
    --dbLoadBronzeHistoryScoreRange(db)
    --dbLoadBronzeTodayScoreRange(db)
    dbLoadArenaRankRange(db)
    -- publish
    publistAllRangeToClient()
    publistUserRankToClient()
end

function M.onLoadRanges()
    M.queueJob(function()
        M.loadRanges()
    end)
end

function M.getTopRange(type)
    local list = rangeMap[type]
    local topRange = {}
    for i, v in pairs(list) do
        if i > 100 then
            break
        else
            table.insert(topRange, v)
        end
    end
    return topRange
end

function M.onAllianceDisband(aid)
    -- new vsn
    genVsn()

    local index = 0

    do
        local range = rangeMap[p.RangeType.ALLIANCE_POWER]
        for i,v in ipairs(range) do
            if v.aid == aid then
                index = i
                break
            end
        end
        if index > 0 then
            range[index] = nil
            index = 0
        end
    end

    do
        local range = rangeMap[p.RangeType.ALLIANCE_KILLS]
        for i,v in ipairs(range) do
            if v.aid == aid then
                index = i
                break
            end
        end
        if index > 0 then
            allianceKillsRange[index] = nil
            index = 0
        end
    end

    publistAllRangeToClient()
end

--
--
--
function impl.fetch_all_ranges(type)
    --print('impl.fetch_all_ranges ...')
    local data = rangeMap[type]
    if data then
        return { vsn = vsn, type = type, data = data }
    else
        return nil
    end
end

function impl.fetch_user_rank()
    return userRank
end

return M

