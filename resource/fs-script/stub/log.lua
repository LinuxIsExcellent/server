local p = require('protocol')
local cluster = require('cluster')

local logStub = {
    rawStub,
}

local subscriber = {
}

local rawStub

function logStub.connectService()
    rawStub = cluster.connectService('log@cs')
    logStub.rawStub = rawStub
    rawStub:setSubscriber(subscriber)
end

function logStub.appendLogin(uid, exp, login_level, logout_level, login_time, logout_time, isReconnect)
    rawStub:cast_appendLog(p.LogClassify.LOGIN, {
        uid = uid, exp = exp, login_level = login_level, logout_level = logout_level, login_time = login_time, logout_time = logout_time, isReconnect = isReconnect
        })
end

function logStub.appendItem(uid, tplid, count, log_type, opt_type, log_time)
    rawStub:cast_appendLog(p.LogClassify.ITEM, {
        uid = uid, tplid = tplid, count = count, log_type = log_type, opt_type = opt_type, log_time = log_time
        })
end

function logStub.appendGold(uid, count, param1, log_type, opt_type, log_time)
    rawStub:cast_appendLog(p.LogClassify.GOLD, {
        uid = uid, count = count, param1 = param1, log_type = log_type, opt_type = opt_type, log_time = log_time
        })
end

function logStub.appendSilver(uid, count, log_type, opt_type, log_time)
    rawStub:cast_appendLog(p.LogClassify.SILVER, {
        uid = uid, count = count, log_type = log_type, opt_type = opt_type, log_time = log_time
        })
end

function logStub.appendFood(uid, count, log_type, opt_type, log_time)
    rawStub:cast_appendLog(p.LogClassify.FOOD, {
        uid = uid, count = count, log_type = log_type, opt_type = opt_type, log_time = log_time
        })
end

function logStub.appendWood(uid, count, log_type, opt_type, log_time)
    rawStub:cast_appendLog(p.LogClassify.WOOD, {
        uid = uid, count = count, log_type = log_type, opt_type = opt_type, log_time = log_time
        })
end

function logStub.appendIron(uid, count, log_type, opt_type, log_time)
    rawStub:cast_appendLog(p.LogClassify.IRON, {
        uid = uid, count = count, log_type = log_type, opt_type = opt_type, log_time = log_time
        })
end

function logStub.appendStone(uid, count, log_type, opt_type, log_time)
    rawStub:cast_appendLog(p.LogClassify.STONE, {
        uid = uid, count = count, log_type = log_type, opt_type = opt_type, log_time = log_time
        })
end

function logStub.appendStamina(uid, count, log_type, opt_type, log_time)
    rawStub:cast_appendLog(p.LogClassify.STAMINA, {
        uid = uid, count = count, log_type = log_type, opt_type = opt_type, log_time = log_time
        })
end

function logStub.appendRoleUpgrade(uid, from_level, to_level, log_time)
    rawStub:cast_appendLog(p.LogClassify.ROLE_UPGRADE, {
        uid = uid, from_level = from_level, to_level = to_level, log_time = log_time
        })
end

function logStub.appendBuildingUpgrade(uid, building_type, from_level, to_level, log_time)
    rawStub:cast_appendLog(p.LogClassify.BUILDING_UPGRADE, {
        uid = uid, building_type = building_type, from_level = from_level, to_level = to_level, log_time = log_time
        })
end

function logStub.appendVipUpgrade(uid, from_level, to_level, log_time)
    rawStub:cast_appendLog(p.LogClassify.VIP_UPGRADE, {
        uid = uid, from_level = from_level, to_level = to_level, log_time = log_time
        })
end

function logStub.appendSign(uid, days, log_time)
    rawStub:cast_appendLog(p.LogClassify.SIGN, {
        uid = uid, days = days, log_time = log_time
        })
end

function logStub.appendOnlineReward(uid, seconds, log_time)
    rawStub:cast_appendLog(p.LogClassify.ONLINE_REWARD, {
        uid = uid, seconds = seconds, log_time = log_time
        })
end

--作为攻击方出征时战斗胜负记录
function logStub.appendMarch(uid, type, is_win, log_time)
    -- print('-------logStub.appendMarch...uid, type, is_win, log_time', uid, type, is_win, log_time)
    rawStub:cast_appendLog(p.LogClassify.MARCH, {
        uid = uid, type = type, is_win = is_win, log_time = log_time
        })
end

function logStub.appendMonsterSiege(uid, is_get_back, level, food, wood, iron, stone, log_time)
    rawStub:cast_appendLog(p.LogClassify.MONSTER_SIEGE, {
        uid = uid, is_get_back = is_get_back, level = level, food = food, wood = wood, iron = iron, stone = stone, log_time = log_time
        })
end

function logStub.appendAllianceScienceDonate(aid, uid, science_id, group_id, science_level, item_id, count, exp, active, score, log_time)
    rawStub:cast_appendLog(p.LogClassify.ALLIANCE_SCIENCE_DONATE, {
        aid = aid, uid = uid, science_id = science_id, group_id = group_id, science_level = science_level, item_id = item_id, count = count, exp = exp, active = active, score = score, log_time = log_time
        })
end

function logStub.appendAllianceExplore(aid, uid, explore_id, log_time)
    rawStub:cast_appendLog(p.LogClassify.ALLIANCE_EXPLORE, {
        aid = aid, uid = uid, explore_id = explore_id, log_time = log_time
        })
end

function logStub.appendTurnplate(uid, is_free, type, mul, drops, log_time)
    rawStub:cast_appendLog(p.LogClassify.TURNPLATE, {
        uid = uid, is_free = is_free, type = type, mul = mul, drops = drops, log_time = log_time
        })
end

function logStub.appendActivityTargetReward(uid, activity_id, activity_tplid, target, log_time)
    rawStub:cast_appendLog(p.LogClassify.ACTIVITY_TARGET_REWARD, {
        uid = uid, activity_id = activity_id, activity_tplid = activity_tplid, target = target, log_time = log_time
        })
end

function logStub.appendAllianceStore(aid, uid, honor, item_id, item_count, log_time)
    rawStub:cast_appendLog(p.LogClassify.ALLIANCE_STORE, {
        aid = aid, uid = uid, honor = honor, item_id = item_id, item_count = item_count, log_time = log_time
        })
end

function logStub.appendTower(uid, layer, is_win, log_time)
    rawStub:cast_appendLog(p.LogClassify.TOWER, {
        uid = uid, layer = layer, is_win = is_win, log_time = log_time
        })
end

function logStub.appendTechnology(uid, technology_id, technology_level, log_time)
    rawStub:cast_appendLog(p.LogClassify.TECHNOLOGY, {
        uid = uid, technology_id = technology_id, technology_level = technology_level, log_time = log_time
        })
end

function logStub.appendArmy(uid, armyType, level, count, log_type, opt_type, log_time)
    rawStub:cast_appendLog(p.LogClassify.ARMY, {
        uid = uid, armyType = armyType, level = level, count = count, log_type = log_type, opt_type = opt_type, log_time = log_time
        })
end

function logStub.appendHero(uid, hero_id, hero_level, hero_star, log_type, opt_type, log_time)
    -- print('logStub.appendHero...uid, hero_id, hero_level, hero_star, log_type, opt_type, log_time', uid, hero_id, hero_level, hero_star, log_type, opt_type, log_time)
    rawStub:cast_appendLog(p.LogClassify.HERO, {
        uid = uid, hero_id = hero_id, hero_level = hero_level, hero_star = hero_star, log_type = log_type, opt_type = opt_type, log_time = log_time
        })
end

function logStub.appendHeroUpgrade(uid, heroId, from_level, to_level, from_star, to_star, log_time)
    rawStub:cast_appendLog(p.LogClassify.HERO_UPGRADE, {
        uid = uid, heroId = heroId, from_level = from_level, to_level = to_level, from_star = from_star, to_star = to_star, log_time = log_time
        })
end

function logStub.appendArenaWinPoint(uid, score, log_type, opt_type, log_time)
    rawStub:cast_appendLog(p.LogClassify.ARENA_WIN_POINT, {
        uid = uid, score = score, log_type = log_type, opt_type = opt_type, log_time = log_time
        })
end

function logStub.appendAllianceScore(uid, score, log_type, opt_type, log_time)
    rawStub:cast_appendLog(p.LogClassify.ALLIANCE_SCORE, {
        uid = uid, score = score, log_type = log_type, opt_type = opt_type, log_time = log_time
        })
end

function logStub.appendBabelScore(uid, score, log_type, opt_type, log_time)
    rawStub:cast_appendLog(p.LogClassify.BABEL_SCORE, {
        uid = uid, score = score, log_type = log_type, opt_type = opt_type, log_time = log_time
        })
end

function logStub.appendBronzeScore(uid, score, log_type, opt_type, log_time)
    rawStub:cast_appendLog(p.LogClassify.BRONZE_SCORE, {
        uid = uid, score = score, log_type = log_type, opt_type = opt_type, log_time = log_time
        })
end

function logStub.appendBronzeScore(uid, score, log_type, opt_type, log_time)
    rawStub:cast_appendLog(p.LogClassify.BRONZE_SCORE, {
        uid = uid, score = score, log_type = log_type, opt_type = opt_type, log_time = log_time
        })
end

function logStub.appendStore(uid, store_type, coin_type, consume, item_id, item_count, log_time)
    rawStub:cast_appendLog(p.LogClassify.STORE, {
        uid = uid, store_type = store_type, coin_type = coin_type, consume = consume, item_id = item_id, item_count = item_count, log_time = log_time
        })
end

function logStub.appendSkillPoint(uid, count, log_type, opt_type, log_time)
    rawStub:cast_appendLog(p.LogClassify.SKILLPOINT, {
        uid = uid, count = count, log_type = log_type, opt_type = opt_type, log_time = log_time
        })
end

return logStub
