local cluster = require('cluster')
local misc = require('libs/misc')
local utils = require('utils')
local pubHero = require('hero')
local loginStub = require('stub/login')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()

local arenaStub = {
    rawStub,
}

local subscriber = {
    arenaRankUpdate = function(uid, rank)
        -- print('arenaRankUpdate....', uid, rank)
        loginStub.arenaRankUpdate(uid, rank)
    end,

    arenaChangeOpponent = function(uid, list)
        -- print('arenaChangeOpponent....uid, list', uid, list)
        loginStub.arenaChangeOpponent(uid, list)
    end,
}

local rawStub

function arenaStub.connectService()
    rawStub = cluster.connectService('arena@cs')
    arenaStub.rawStub = rawStub
    rawStub:setSubscriber(subscriber)
end

--to cs
function arenaStub.cast_arenaChangeOpponent(uid)
    -- print('arenaStub.cast_arenaChangeOpponent...uid', uid)
    rawStub:cast_arenaChangeOpponent(uid)
end

function arenaStub.call_getArenaDefense(uid)
    -- print('arenaStub.call_getArenaDefense...uid', uid)
    local bRet, myRank, lastRank, heroList = rawStub:call_getArenaDefense(uid)
    if not bRet then
        myRank = 0
        lastRank = 0
        heroList = {}
    end
    return myRank, lastRank, heroList
end

function arenaStub.call_getRevengeDataByUid(uid)
    local bRet, data = rawStub:call_getRevengeDataByUid(uid)
    -- print("###arenaStub.call_getRevengeDataByUid...", utils.serialize(data))
    if data and bRet then
        return bRet, data
    end
end

function arenaStub.call_getRidingAloneDefenseData(uid, heroPower)
    local bRet, list = rawStub:call_getRidingAloneDefenseData(uid, heroPower)
    if not bRet then
        list = nil
    end
    return list
end

function arenaStub.cast_arenaSetDefense(uid, nickname, level, headId, aid, allianceName, allianceNickname, bannerId, heroList, attrList)
    -- print('arenaStub.cast_arenaSetDefense....uid, nickname, level, headId, aid, allianceName, allianceNickname, bannerId, heroList', uid, nickname, level, headId, aid, allianceName, allianceNickname, bannerId, utils.serialize(heroList), utils.serialize(attrList))
    rawStub:cast_arenaSetDefense(uid, nickname, level, headId, aid, allianceName, allianceNickname, bannerId, heroList, attrList)
end

function arenaStub.cast_arenaCombat(myUid, toUid, isWin, details)
    -- print('arenaStub.cast_arenaCombat...myUid, toUid, isWin, details', myUid, toUid, isWin, details)
    rawStub:cast_arenaCombat(myUid, toUid, isWin, details)
end

function arenaStub.cast_arenaHeroDataChange(uid, heroId, arenaDataType, data)
    -- print('arenaStub.cast_arenaHeroDataChange...uid, heroId, arenaDataType, data', uid, heroId, arenaDataType, data)
    rawStub:cast_arenaHeroDataChange(uid, heroId, arenaDataType, data)
end

function arenaStub.cast_arenaUserDataChange(uid, level, nickname, headId)
    -- print('arenaStub.cast_arenaUserDataChange...uid, level, nickname, headId', uid, level, nickname, headId)
    rawStub:cast_arenaUserDataChange(uid, level, nickname, headId)
end

function arenaStub.cast_arenaPropChange(uid, attrList)
    -- print('arenaStub.cast_arenaPropChange...uid, attrList', uid, attrList)
    rawStub:cast_cast_arenaPropChange(uid, attrList)
end

--to fs

return arenaStub