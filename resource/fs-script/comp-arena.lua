local agent = ...
local p = require('protocol')
local t = require('tploader')
local dbo = require('dbo')
local timer = require('timer')
local utils = require('utils')
local misc = require('libs/misc')
local arenaStub = require('stub/arena')
local dataStub = require('stub/data')
local logStub = require('stub/log')
local event = require('libs/event')

local user = agent.user
local bag = agent.bag
local dict = agent.dict
local cdlist = agent.cdlist
local hero = agent.hero
local vip = agent.vip

local MIN_DEFENSE_HERO_NUMS = 5

--擂台战斗CD间隔时间(秒)
local arenaBattleCd = t.configure['arenaBattleCd']
--擂台清除战斗CD消耗(元宝)
local arenaClearCdCost = t.configure['arenaClearCdCost']
--擂台重置战斗次数基础消耗基数(元宝)  2的n次方 * arenaResetCost
local arenaResetCost = t.configure['arenaResetCost']
--擂台战斗次数上限
local arenaBattleMax = t.configure['arenaBattleMax']
--擂台战斗掉落
local arenaBattleReward = t.configure['arenaBattleReward']
--擂台胜点获得
local arenaWinPoint = t.configure['arenaWinPoint']

--擂台数据
local arena = {
    myRank = 0,         --我的当前排名(从cs取最新)
    lastRank = 0,       --上一次排行榜最终排名
    heroList = {},      --我的防守阵容 <locate,heroId>

    maxRank = 0,        --我的历史最高排名(每次战斗后比较设置一次)
    hadCount = 0,       --已挑战次数(需要次日刷新)
    expiredTime = 0,    --挑战CD到期时间
    winPoint = 0,       --擂台胜点(积分)
    isDrew = false,     --是否已领取昨天排行榜奖励(需要次日刷新)

    resetCount = 0,     --擂台重置次数

    evtFightArena = event.new(),  --
    evtArenaRank = event.new(), -- 擂台排名
}

--战斗记录
local recordInfo = {
    id = 0,                 --记录ID
    uid = 0,                --玩家UID
    nickname = '',          --玩家昵称
    level = 0,              --玩家等级
    headId = 0,             --玩家头像
    allianceName = '',      --玩家联盟全称
    allianceNickname = '',  --玩家联盟昵称
    bannerId = 0,           --玩家联盟旗帜ID

    toUid = 0,               --对手UID
    toNickname = '',          --对手昵称
    toLevel = 0,              --对手等级
    toHeadId = 0,             --对手头像
    toAllianceName = '',      --对手联盟全称
    toAllianceNickname = '',  --对手联盟昵称
    toBannerId = 0,           --对手联盟旗帜ID

    isWin = false,          --是否赢了
    isAttacker = false,     --是否攻击者
    changRank = 0,          --(输)下降或(赢)上升x名 0表示没有变化
    createTime = 0,         --记录时间
    battleData = '',        --战斗重播数据
    canRevenge = false,     --是否可以复仇
    toPower = 0,            --对方总战斗力
    isDirty = false,        
}
local RECORD_MAX = 20
local recordVsn = 200
local arenaRecord = {}  --map<i, recordInfo>

--对方防守阵容
local toDefenseList = {}

function recordInfo:new(o)
    o = o or {}
    o.isDirty = false
    setmetatable(o, self)
    self.__index = self
    return o
end

function arena.onInit()
    arena.dbLoad()

    if user.newPlayer then
        arena.isDrew = true
    end

    --0点刷新
    if cdlist.isHour0Refresh then
        if not user.newPlayer then
            arena.refresh(false)
        end
    end
    cdlist.evtHour0Refresh:attachRaw(function()
        arena.refresh(true)
    end)

    agent.combat.evtCombatOver:attachRaw(arena.onCombatOver)

    user.evtNicknameChange:attachRaw(arena.onUserDataChange)
    user.evtHeadIdChange:attachRaw(arena.onUserDataChange)
    user.evtLevelUp:attachRaw(arena.onUserDataChange)
end

function arena.onAllCompInit()
    agent.queueJob(function()
        if not cdlist.isHour0Refresh then
            local myRank, lastRank, heroList = arenaStub.call_getArenaDefense(user.uid)
            arena.myRank = myRank
            arena.heroList = heroList
        end

        arena.changeArenaUserData()
        arena.sendArenaDefenseUpdate()
    end)
end

function arena.onClose()
    arena.dbSave()
end

function arena.onSave()
    arena.dbSave()
end

function arena.dbLoad()
    local info = dict.get("arena.info")
    if info then
        arena.lastRank = info.lastRank or arena.lastRank

        arena.maxRank = info.maxRank or arena.maxRank
        arena.hadCount = info.hadCount or arena.hadCount
        arena.expiredTime = info.expiredTime or arena.expiredTimes
        arena.winPoint = info.winPoint or arena.winPoint
        arena.isDrew = info.isDrew or arena.isDrew
        arena.resetCount = info.resetCount or arena.resetCount
    end

    local db = dbo.open(0)
    local rs = db:executePrepare('SELECT id, uid, nickname, level, headId, allianceName, allianceNickname, bannerId, toUid, toNickname, toLevel, toHeadId, toAllianceName, toAllianceNickname, toBannerId, isWin, isAttacker, changRank, createTime, battleData, canRevenge, toPower from s_arena_record where uid = ? order by id desc limit ?', user.uid, RECORD_MAX)
    if rs.ok then
        for _, row in ipairs(rs) do
            local info = recordInfo:new(row)
            -- print("###arena.dbLoad...", utils.serialize(info))
            table.insert(arenaRecord, info)
        end
    end
end

function arena.dbSave()
    dict.set("arena.info", {
        lastRank = arena.lastRank,
        maxRank = arena.maxRank,
        hadCount = arena.hadCount,
        expiredTime = arena.expiredTime,
        winPoint = arena.winPoint,
        isDrew = arena.isDrew,
        resetCount = arena.resetCount,
        })
    local arenaList = {}
    for _, v in pairs(arenaRecord) do
        -- print("arena.dbSave", utils.serialize(v))
        if v.isDirty then
            v.isDirty = false
            table.insert(arenaList, {id = v.id, uid = v.uid, nickname = v.nickname, level = v.level, headId = v.headId, allianceName = v.allianceName, allianceNickname = v.allianceNickname, bannerId = v.bannerId, toUid = v.uid, toNickname = v.nickname, toLevel = v.level, toHeadId = v.headId, toAllianceName = v.allianceName, toAllianceNickname = v.allianceNickname, toBannerId = v.bannerId, isAttacker = v.isAttacker, isWin = v.isWin, changRank = v.changRank, createTime = v.createTime, battleData = v.battleData, canRevenge = v.canRevenge, toPower = v.toPower})
        end
    end
    dataStub.appendDataList(p.DataClassify.ARENA_RECORD, arenaList)
end

function arena.reloadRecordById(recordId)
    agent.queueJob(function()
        -- print('arena.reloadRecordById ......uid, recordId', user.uid, recordId)
        local db = dbo.open(0)
        local rs = db:executePrepare('SELECT id, uid, nickname, level, headId, allianceName, allianceNickname, bannerId, toUid, toNickname, toLevel, toHeadId, toAllianceName, toAllianceNickname, toBannerId, isWin, isAttacker, changRank, createTime, battleData, canRevenge, toPower from s_arena_record where uid = ? and id = ?', user.uid, recordId)
        if rs.ok then
            for _, row in ipairs(rs) do
                recordVsn = recordVsn + 1
                local info = recordInfo:new(row)
                table.insert(arenaRecord, 1, info)
                while #arenaRecord > RECORD_MAX do
                    table.remove(arenaRecord)
                end
            end
        end
    end)
end

function arena.refresh(sendUpdate)
    agent.queueJob(function()
        local myRank, lastRank, heroList = arenaStub.call_getArenaDefense(user.uid)
        -- print('arena.refresh...myRank, lastRank, heroList', myRank, lastRank, utils.serialize(heroList))
        arena.myRank = myRank
        arena.lastRank = lastRank
        arena.heroList = heroList

        arena.hadCount = 0
        arena.resetCount = 0
        arena.isDrew = false

        if sendUpdate then
            arena.changeArenaUserData()
            arena.sendArenaDefenseUpdate()
        end
    end)
end

function arena.addWinPoint(value, gainType, sendUpdate)
    if gainType == nil then
        gainType = p.ResourceGainType.ARENA
    end
    if sendUpdate == nil then
        sendUpdate = true
    end
    if value > 0 then
        arena.winPoint = arena.winPoint + value
        --log
        logStub.appendArenaWinPoint(user.uid, value, p.LogItemType.GAIN, gainType, timer.getTimestampCache())

        if sendUpdate then
            arena.sendArenaUserDataUpdate()
        end
    end
end

function arena.removeWinPoint(value, consumeType, sendUpdate)
    -- print('arena.removeWinPoint...value, consumeType, sendUpdate', value, consumeType, sendUpdate)
    if consumeType == nil then
        consumeType = p.ResourceConsumeType.STORE_BUY
    end
    if sendUpdate == nil then
        sendUpdate = true
    end
    if value > 0 then
        arena.winPoint = arena.winPoint - value
        if arena.winPoint < 0 then
            arena.winPoint = 0
        end
        --log
        logStub.appendArenaWinPoint(user.uid, value, p.LogItemType.CONSUME, consumeType, timer.getTimestampCache())

        if sendUpdate then
            arena.sendArenaUserDataUpdate()
        end
    end
    return true
end

function arena.isWinPointEnough(value)
    value = math.ceil(value)
    if value < 0 then
        return false
    end
    return arena.winPoint >= value
end

function arena.isArenaDefenseHero(heroId)
    local bRet = false
    for _, v in pairs(arena.heroList) do
        if v.heroId == heroId then
            bRet = true
            break
        end
    end
    return bRet
end

function arena.isArenaSetDefense()
    return  #arena.heroList == MIN_DEFENSE_HERO_NUMS
end

function arena.getArenaDefenseRankData()
    local heroPower = 0
    for _, v in pairs(arena.heroList) do
        local heroInfo = hero.info[v.heroId]
        if heroInfo then
            local power = heroInfo:getSingleHeroPower()
            heroPower = heroPower + math.floor(power)
        end
    end

    return arena.myRank, heroPower
end

--设置擂台防守阵容
function arena.setArenaDefense(list)
    -- print('arena.setArenaDefense...', utils.serialize(list))
    if list then
        local heroList = {}
        arena.heroList = {}
        for locate, heroId in pairs(list) do
            table.insert(arena.heroList, { locate = locate, heroId = heroId })

            local heroInfo = hero.info[heroId]
            local skill = heroInfo:getHeroSkill(true)
            local equip = {}
            local treasure = {}
            heroList[locate] = { id = heroId, level = heroInfo.level, star = heroInfo.star, skill = skill, equip = equip, treasure = treasure }
        end
        arena.sendArenaDefenseUpdate()

        --sync to cs
        local uid = user.uid
        local nickname = user.info.nickname
        local level = user.info.level
        local headId = user.info.headId
        local aid = 0
        local allianceName = ''
        local allianceNickname = ''
        local bannerId = 0
        local attrList = agent.property.getHeroProp() or {}
        -- print('arena.setArenaDefense...', utils.serialize(heroList))
        arenaStub.cast_arenaSetDefense(uid, nickname, level, headId, aid, allianceName, allianceNickname, bannerId, heroList, attrList)
    end
end

function arena.pickArenaBattleDrops(drops, gainType, heroList)
    for _, drop in pairs(drops) do
        if drop.tplId == p.SpecialPropIdType.HERO_EXP then
            if heroList then
                for _, v in pairs(heroList) do
                     hero.addHeroExp(v.heroId, drop.count)
                end
            end
        else
            --item
            if t.item[drop.tplId] == nil then
                print('arena.pickArenaBattleDrops, tpl_item not exist tplId=', drop.tplId)
            else
                bag.addItem(drop.tplId, drop.count, gainType, false)
            end
        end
    end
    hero.sendHeroUpdate()
    bag.sendBagUpdate()
end

function arena.onCombatOver(battleId, battleType, isWin, battleData, dataOnCombatOver)
    -- print('arena.onCombatOver...battleId, battleType, isWin, result, dataOnCombatOver', battleId, battleType, isWin, result, dataOnCombatOver)
    if battleType == p.BattleType.ARENA_COMBAT then
        --同步到cs
        local toUid = dataOnCombatOver.toUid
        arenaStub.cast_arenaCombat(user.uid, toUid, isWin, battleData)

        arena.evtFightArena:trigger(isWin)
    end
end

function arena.onUserDataChange()
    -- print('arena.onUserDataChange....')
    if arena.isArenaSetDefense() then
        arenaStub.cast_arenaUserDataChange(user.uid, user.info.level, user.info.nickname, user.info.headId)
    end
end

function arena.arenaChangeOpponent(list)
    local function rankSort(data1, data2)
        if data1.rank > data2.rank then
            return false
        else
            return true
        end
    end
    -- print("###arena.arenaChangeOpponent   111", utils.serialize(list))
    toDefenseList = {}
    for _, v in pairs(list) do
        toDefenseList[v.uid] = v
    end
    for _, v in pairs(list) do
        table.sort(list, rankSort)
    end
    -- 加上武将战斗力 TODO by zds
    if next(list) then
        -- print('arena.arenaChangeOpponent....', list)
        agent.sendPktout(p.SC_ARENA_CHANGE_OPPONENT_RESPONSE, '@@1=b,2=[uid=i,nickname=s,allianceName=s,level=i,headId=i,rank=i,winCount=i,loseCount=i,power=i,heroList=[locate=i,heroId=i,level=i,star=i],isRobot=b]', true, list)
    end
end

function arena.arenaRankUpdate(myRank)
    -- print('arena.arenaRankUpdate....myRank', myRank)
    if myRank > 0 then
        arena.myRank = myRank
        if arena.maxRank == 0 then
            arena.maxRank = myRank
        end
        arena.changeArenaUserData()

        --任务 擂台排名
        arena.evtArenaRank:trigger(myRank)
    end
end

function arena.changeArenaUserData()
    if arena.myRank == 0 and arena.maxRank > 0 then
        arena.maxRank = 0
    end
    -- print('arena.changeArenaUserData...arena.myRank, arena.maxRank', arena.myRank, arena.maxRank)
    --历史最高排名及奖励邮件发送
    if arena.myRank < arena.maxRank then
        print('arena.changeArenaUserData....myRank, maxRank',arena.myRank, arena.maxRank)
        local oldMax = arena.maxRank
        local newMax = arena.myRank
        arena.maxRank = arena.myRank

        local rewards = t.arenaRankReward[p.ArenaRewardType.ARENA_MAX_RECORD]
        if rewards == nil then
            utils.log("tpl_arena_rank_reward no max record reward...")
            return
        end

        local tempList = {}
        local bSend = false
        for k, v in ipairs(rewards) do
            -- print('xxx',k, v.rankMin, v.rankMax)
            if v.rankMin <= oldMax and v.rankMax >= newMax then
                if newMax >= v.rankMin and newMax <= v.rankMax then
                    local muil = 0
                    if oldMax > v.rankMax then
                        muil = v.rankMax - newMax + 1
                        newMax = v.rankMax + 1
                    elseif oldMax <= v.rankMax then
                        muil = oldMax - newMax
                    end
                    if muil > 0 then
                        for k, count in pairs(v.rewardList) do
                            if tempList[k] == nil then
                                tempList[k] = 0
                            end
                            tempList[k] = tempList[k] + muil * count
                            bSend = true
                        end
                    end
                end
            end
        end

        if bSend then
            local drops = {}
            for tplId, count in pairs(tempList) do
                table.insert(drops, { tplId = tplId, count = count })
            end
            --邮件发送
            local mailType = p.MailType.SYSTEM
            local mailSubType = p.MailSubType.SYSTEM_ARENA_MAX_RECORD
            local title, content = agent.mail.getTitleContentBySubType(mailSubType)
            local attachments = utils.serialize(drops)
            local timestamp = timer.getTimestampCache()
            local params = tostring(newMax)
            agent.mail.saveMailRaw(user.uid, 0, mailType, mailSubType, title, content, timestamp, true, false, attachments, params, 0)
        else
            utils.log("tpl_arena_rank_reward no max record reward...uid=%i, oldMaxRank=%i, newMaxRank=%i, drops=%s", user.uid, oldMax, arena.maxRank, utils.serialize(drops))
        end
    end

    arena.sendArenaUserDataUpdate()
end

function arena.sendArenaUserDataUpdate()
    local leftTime = arena.expiredTime - timer.getTimestampCache()
    if leftTime < 0 then
        leftTime = 0
    end
    -- print('arena.sendArenaUserDataUpdate...myRank, maxRank, hadCount, arenaBattleMax, leftTime, isDrew, winPoint, lastRank, resetCount', arena.myRank, arena.maxRank, arena.hadCount, arenaBattleMax, leftTime, arena.isDrew, arena.winPoint, arena.lastRank, arena.resetCount)
    agent.sendPktout(p.SC_ARENA_USER_DATA_UPDATE, arena.myRank, arena.maxRank, arena.hadCount, leftTime, arena.isDrew, arena.winPoint, arena.lastRank, arena.resetCount)
end

function arena.sendArenaDefenseUpdate()
    -- print('arena.sendArenaDefenseUpdate...heroList', utils.serialize(arena.heroList))
    if #arena.heroList == MIN_DEFENSE_HERO_NUMS then
        agent.sendPktout(p.SC_ARENA_DEFENSE_UPDATE, '@@1=[locate=i,heroId=i]', arena.heroList)
    end
end

function arena.cs_arena_fight(toUid, arenaList, session)
    local function arenaFightResponse(result, battleId, isWin, beginRank, endRank, winPoint, warReport)
        -- print("###p.CS_ARENA_FIGHT...result, battleId, isWin, myRank, arena.myRank, winPoint, warReport",result, battleId, isWin, beginRank, endRank, winPoint, warReport)
        agent.replyPktout(session, p.SC_ARENA_FIGHT_RESPONSE, result, battleId, isWin, beginRank, endRank, winPoint, warReport)
    end
    -- print('p.CS_ARENA_FIGHT...toUid, size ', toUid, size )

    local battleId = 0
    local isWin = false
    local warReport 
    if #arenaList ~= MIN_DEFENSE_HERO_NUMS then
        agent.sendNoticeMessage(p.ErrorCode.ARENA_HERO_NOT_ENOUGH, '', 1)
        return
    end

    local leftCount = arenaBattleMax - arena.hadCount
    if leftCount <= 0 then
        print('p.CS_ARENA_FIGHT...hadCount >= arenaBattleMax', arena.hadCount, arenaBattleMax)
        agent.sendNoticeMessage(p.ErrorCode.ARENA_RESET_BATTLE_COUNT, '', 1)
        return
    end

    local now = timer.getTimestampCache()
    local leftTime = arena.expiredTime - now
    if leftTime > 0 then
        print('p.CS_ARENA_FIGHT... cd is cooling...leftTime=', leftTime)
        agent.sendNoticeMessage(p.ErrorCode.ARENA_CD_COOLING, '', 1)
        return
    end

    --对方数据
    local toDefense = toDefenseList[toUid]
    if not toDefense then
        print('p.CS_ARENA_FIGHT...arena no this user, toUid=', toUid)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    local toHeroList = {}
    for _, heroInfo in pairs(toDefense.heroList) do
        toHeroList[heroInfo.locate] = {
            id = heroInfo.heroId,
            level = heroInfo.level,
            star = heroInfo.star,
            hp = heroInfo.hp,
            attack = heroInfo.attack,
            defense = heroInfo.defense,
            strategy = heroInfo.strategy,
            speed = heroInfo.speed,
            leadership = heroInfo.leadership,
            rage = heroInfo.rage,
            challenge = heroInfo.challenge,
            intellect = heroInfo.intellect,
            skill = heroInfo.skill,
            equip = heroInfo.equip,
            treasure = heroInfo.treasure,
            additionList = heroInfo.additionList,
            secondAdditionList = heroInfo.secondAdditionList,
        }
    end
    local defenseInfo = {
        uid = toDefense.uid,
        nickname = toDefense.nickname,
        level = toDefense.level,
        headId = toDefense.headId,
        heroList = toHeroList,
        attrList = toDefense.attrList,
    }
    -- 我方战斗前排名
    local myRank = arena.myRank
    -- 防守方战斗前排名
    local defenseRank = toDefense.rank

    --我方数据
    local myHeroList = {}
    local oldHeroList = {}
    local tempLocateList = {}
    local tempHeroList = {}
    for _,v in pairs(arenaList) do
        local locate, heroId = v.locate, v.heroId
        -- print('p.CS_ARENA_FIGHT...locate, heroId', locate, heroId)
        local heroInfo = hero.info[heroId]
        if heroInfo == nil
            or locate <= 0 or locate > MIN_DEFENSE_HERO_NUMS
            or heroId <= 0 or tempHeroList[heroId]
            or myHeroList[locate] then
            print('p.CS_ARENA_FIGHT...data is wrong or hero repeat...locate, heroId', locate, heroId)
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            return
        end
        local skill = heroInfo:getHeroSkill(false)
        local equip = {}
        local treasure = {}
        local additionList = heroInfo:getHeroAdditionList()
        myHeroList[locate] = {
            id = heroId,
            level = heroInfo.level,
            star = heroInfo.star,
            hp = heroInfo.hp,
            attack = heroInfo.attack,
            defense = heroInfo.defense,
            strategy = heroInfo.strategy,
            speed = heroInfo.speed,
            leadership = heroInfo.leadership,
            rage = heroInfo.rage,
            challenge = heroInfo.challenge,
            intellect = heroInfo.intellect,
            skill = skill,
            equip = equip,
            treasure = treasure,
            additionList = additionList,
        }
        --武将旧数据
        table.insert(oldHeroList, { heroId = heroId, oldLevel = heroInfo.level, oldExp = heroInfo.exp })
        tempHeroList[heroId] = locate
        tempLocateList[locate] = heroId
    end
    local attackInfo = { uid = user.uid, nickname = user.info.nickname, level = user.info.level, headId = user.info.headId, heroList = myHeroList }

    --己方是否有防守阵容,没有则设置
    -- print("#arena.heroList, MIN_DEFENSE_HERO_NUMS", #arena.heroList, MIN_DEFENSE_HERO_NUMS)
    -- print("#arena.heroList, MIN_DEFENSE_HERO_NUMS", utils.serialize(arena.heroList))
    if #arena.heroList ~= MIN_DEFENSE_HERO_NUMS then
        arena.setArenaDefense(tempLocateList)
    end

    --战斗
    local dataOnCombatOver = { heroList = oldHeroList, toUid = toUid }
    isWin, battleId, warReport = agent.combat.createHeroBattle(attackInfo, defenseInfo, p.BattleType.ARENA_COMBAT, dataOnCombatOver)
    print("###arenaFight...isWin", type(isWin))
    if battleId > 0 then
        local winPoint = 0
        local beginRank = myRank
        local endRank = myRank
        print("###arenaFight...isWin, myRank, defenseRank", isWin, myRank, defenseRank)
        if isWin == 1 then
            if myRank > defenseRank then
                winPoint = arenaWinPoint.low[1]
                endRank = defenseRank
            else
                winPoint = arenaWinPoint.high[1]
            end
        else
            if myRank > defenseRank then
                winPoint = arenaWinPoint.low[2]
            else
                winPoint = arenaWinPoint.high[2]
            end
        end
        arena.winPoint = arena.winPoint + winPoint
        arena.hadCount = arena.hadCount + 1
        arena.expiredTime = now + arenaBattleCd
        arenaFightResponse(true, battleId, isWin, beginRank, endRank, winPoint, warReport)
        arena.sendArenaUserDataUpdate()
    else
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
    end
end

function arena.cs_arena_revenge(recordId, arenaList, session)
    local function revengeFightResponse(isWin, battleId, warReport)
        -- print('revengeFightResponse..result, battleId', battleId, warReport)
        agent.replyPktout(session, p.SC_ARENA_REVENGE_RESPONSE, isWin, battleId, warReport)
    end
    -- print("###p.CS_ARENA_REVENGE", recordId)
    local myHeroList = {}
    local oldHeroList = {}
    local tempLocateList = {}
    local tempHeroList = {}
    local now = timer.getTimestampCache()
    -- local leftTime = arena.expiredTime - now
    -- if leftTime > 0 then
    --     print('p.CS_ARENA_FIGHT... cd is cooling...leftTime=', leftTime)
    --     agent.sendNoticeMessage(p.ErrorCode.ARENA_CD_COOLING, '', 1)
    --     return
    -- end
    --我方数据
    local attackInfo = { uid = user.uid, nickname = user.info.nickname, level = user.info.level, headId = user.info.headId, heroList = myHeroList }
    for _,v in pairs(arenaList) do
        local locate, heroId = v.locate, v.heroId
        print('p.CS_ARENA_FIGHT...locate, heroId', locate, heroId)
        local heroInfo = hero.info[heroId]
        if heroInfo == nil
            or locate <= 0 or locate > MIN_DEFENSE_HERO_NUMS
            or heroId <= 0 or tempHeroList[heroId]
            or myHeroList[locate] then
            print('p.CS_ARENA_FIGHT...data is wrong or hero repeat...locate, heroId', locate, heroId)
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            return
        end
        local skill = heroInfo:getHeroSkill(false)
        local equip = {}
        local treasure = {}
        local additionList = heroInfo:getHeroAdditionList()
        myHeroList[locate] = {
            id = heroId,
            level = heroInfo.level,
            star = heroInfo.star,
            hp = heroInfo.hp,
            attack = heroInfo.attack,
            defense = heroInfo.defense,
            strategy = heroInfo.strategy,
            speed = heroInfo.speed,
            leadership = heroInfo.leadership,
            rage = heroInfo.rage,
            challenge = heroInfo.challenge,
            intellect = heroInfo.intellect,
            skill = skill,
            equip = equip,
            treasure = treasure,
            additionList = additionList,
        }
        --武将旧数据
        table.insert(oldHeroList, { heroId = heroId, oldLevel = heroInfo.level, oldExp = heroInfo.exp })
        tempHeroList[heroId] = locate
        tempLocateList[locate] = heroId
    end
    local info = nil
    for _, v in pairs(arenaRecord) do
        if v.id == recordId then
            info = v
            break
        end
    end
    if info == nil then
        print('p.CS_ARENA_REVENGE... no this record, recordId=', recordId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    if not info.canRevenge then
        print("p.CS_ARENA_REVENGE... is not revenge")
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    agent.queueJob(function()
        -- print('###p.CS_ARENA_REVENGE... data', info.toUid)
        local bRet, data = arenaStub.call_getRevengeDataByUid(info.toUid)
        -- local data = arenaStub.call_getArenaDefense(info.toUid)
        if bRet then
            -- print('###p.CS_ARENA_REVENGE... data', data)
            local toHeroList = {}
            for _, heroInfo in pairs(data.heroList) do
                toHeroList[heroInfo.locate] = {
                    id = heroInfo.heroId,
                    level = heroInfo.level,
                    star = heroInfo.star,
                    hp = heroInfo.hp,
                    attack = heroInfo.attack,
                    defense = heroInfo.defense,
                    strategy = heroInfo.strategy,
                    speed = heroInfo.speed,
                    leadership = heroInfo.leadership,
                    rage = heroInfo.rage,
                    challenge = heroInfo.challenge,
                    intellect = heroInfo.intellect,
                    skill = heroInfo.skill,
                    equip = heroInfo.equip,
                    treasure = heroInfo.treasure,
                    additionList = heroInfo.additionList,
                }
            end
            local defenseInfo = {
                uid = data.uid,
                nickname = data.nickname,
                level = data.level,
                headId = data.headId,
                heroList = toHeroList,
                attrList = data.attrList,
            }
            -- 我方战斗前排名
            local myRank = arena.myRank
            -- 防守方战斗前排名
            local defenseRank = data.rank
            local dataOnCombatOver = { heroList = oldHeroList, toUid = info.toUid }
            local isWin = false
            local battleId = 0
            local warReport = nil
            isWin, battleId, warReport = agent.combat.createHeroBattle(attackInfo, defenseInfo, p.BattleType.ARENA_COMBAT, dataOnCombatOver)
            -- print("###ARENA_REVENGE...  isWin, battleId, warReport", isWin, battleId, utils.serialize(warReport))
            if battleId > 0 then
                revengeFightResponse(true, battleId, warReport)
                if isWin then
                    if myRank > defenseRank then
                        arena.winPoint = arena.winPoint + arenaWinPoint.high[1]
                    else
                        arena.winPoint = arena.winPoint + arenaWinPoint.low[1]
                    end
                else
                    if myRank > defenseRank then
                        arena.winPoint = arena.winPoint + arenaWinPoint.high[2]
                    else
                        arena.winPoint = arena.winPoint + arenaWinPoint.low[2]
                    end
                end
                info.canRevenge = false
                info.isDirty = true
                arena.hadCount = arena.hadCount + 1
                arena.expiredTime = now + arenaBattleCd
                arena.sendArenaUserDataUpdate()
            else
                agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            end
        end
    end)
end

function arena.cs_arena_set_defense(arenaList, session)
    local function arenaSetDefenseResponse(result)
        -- print('arenaSetDefenseResponse..result', result)
        agent.replyPktout(session, p.SC_ARENA_SET_DEFENSE_RESPONSE, result)
    end
    if #arenaList ~= MIN_DEFENSE_HERO_NUMS then
        agent.sendNoticeMessage(p.ErrorCode.ARENA_HERO_NOT_ENOUGH, '', 1)
        return
    end
    local tempLocateList = {}
    local tempHeroList = {}
    for _,v in pairs(arenaList) do
        local locate, heroId = v.locate, v.heroId
        if locate <= 0 or locate > MIN_DEFENSE_HERO_NUMS or heroId <= 0 then
            print('p.CS_ARENA_SET_DEFENSE...data is wrong....locate, heroId', locate, heroId)
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            return
        end
        if not hero.info[heroId] or tempLocateList[locate] or tempHeroList[heroId] then
            print('p.CS_ARENA_SET_DEFENSE...no this hero or hero is repeat..')
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            return
        end
        tempLocateList[locate] = heroId
        tempHeroList[heroId] = locate
    end
    -- print('p.CS_ARENA_SET_DEFENSE...len, tempLocateList', #tempLocateList, utils.serialize(tempLocateList))
    arena.setArenaDefense(tempLocateList)
    arenaSetDefenseResponse(true)
end

function arena.cs_arena_drew_reward(session)
    local function arenaDrewRewardResponse(result, dropList)
        if dropList == nil then
            dropList = {}
        end
        -- print('arenaDrewRewardResponse...result, dropList', result, utils.serialize(dropList))
        agent.replyPktout(session, p.SC_ARENA_DREW_REWARD_RESPONSE, '@@1=b,2=[tplId=i,count=i]', result, dropList)
    end

    --1.判断上一期排名
    if arena.lastRank <= 0 then
        print('p.CS_ARENA_DREW_REWARD...not into the ranking list, lastRank=', arena.lastRank)
        agent.sendNoticeMessage(p.ErrorCode.ARENA_NO_REWARD, '', 1)
        return
    end
    --2.是否已领取
    if arena.isDrew then
        print('p.CS_ARENA_DREW_REWARD...the reward was drew...isDrew=', arena.isDrew)
        agent.sendNoticeMessage(p.ErrorCode.ARENA_NO_REWARD, '', 1)
        return
    end
    --reward
    local list = {}
    local rewards = t.arenaRankReward[p.ArenaRewardType.ARENA_RANK]
    if rewards == nil then
        print('p.CS_ARENA_DREW_REWARD...no rewards ...arenaRewardType=', p.ArenaRewardType.ARENA_RANK)
        agent.sendNoticeMessage(p.ErrorCode.ARENA_NO_REWARD, '', 1)
        return
    end
    for _, v in pairs(rewards) do
        if arena.lastRank >= v.rankMin and arena.lastRank <= v.rankMax then
            for tplId, count in pairs(v.rewardList) do
                table.insert(list, { tplId = tplId, count = count })
            end
            break
        end
    end
    --pick
    arena.isDrew = true
    agent.bag.pickDropItems(list, p.ResourceGainType.ARENA)

    arenaDrewRewardResponse(true, list)
end

function arena.cs_arena_battle_record(vsn, session)
    if vsn ~= recordVsn and #arenaRecord > 0 then
        local tempList = {}
        for _, v in ipairs(arenaRecord) do
            table.insert(tempList, { v.id, v.isWin, v.changRank, v.toNickname, v.allianceName, v.toHeadId, v.toLevel, v.toPower, v.canRevenge, v.createTime })
        end

        local data = utils.toJson(tempList)
        -- print('p.CS_ARENA_BATTLE_RECORD...data', data)
        agent.sendPktout(p.SC_ARENA_BATTLE_RECORD_RESPONSE, recordVsn, data)
    end
end

function arena.cs_arena_battle_details(recordId, session)
    local function arenaBattleDetailsResponse(battleData)
        -- print('arenaBattleDetailsResponse...battleData', battleData)
        if battleData == nil then
            battleData = ''
        end
        agent.replyPktout(session, p.SC_ARENA_BATTLE_DETAILS_RESPONSE, '@@1=s', battleData)
    end
    -- print('p.CS_ARENA_BATTLE_DETAILS...uid, recordId', user.uid, recordId)

    --1.判断是否有这条记录
    local info = nil
    for _, v in pairs(arenaRecord) do
        if v.id == recordId then
            info = v
            break
        end
    end
    if info == nil then
        print('p.CS_ARENA_BATTLE_DETAILS... no this record, recordId=', recordId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    arenaBattleDetailsResponse(info.battleData)
end

function arena.cs_arena_clear_battle_cd(session)
    if not user.isResourceEnough(p.ResourceType.GOLD, arenaClearCdCost) then
        -- print('p.CS_ARENA_CLEAR_BATTLE_CD...gold is not enough')
        agent.sendNoticeMessage(p.ErrorCode.PUBLIC_GOLD_NOT_ENOUGH, '', 1)
        return
    end
    user.removeResource(p.ResourceType.GOLD, arenaClearCdCost, p.ResourceConsumeType.ARENA_CLEAR_BATTLE_CD)

    arena.expiredTime = 0
    arena.sendArenaUserDataUpdate()
    agent.sendPktout(p.SC_ARENA_CLEAR_BATTLE_CD_RESPONSE, p.ErrorCode.SUCCESS)
end

function arena.cs_arena_reset_battle_count(session)
    -- print('p.CS_ARENA_RESET_BATTLE_COUNT...resetCount, resetMax', arena.resetCount, vip.tpl.arenaResetCouunt)
    if vip.tpl.arenaResetCouunt < arena.resetCount then
        agent.sendNoticeMessage(p.ErrorCode.PUBLIC_VIP_RESET_MAX, '', 1)
        return
    end
    -- x * 2.0^exp
    local needGold = arenaResetCost * 2^arena.resetCount
    -- print('p.CS_ARENA_RESET_BATTLE_COUNT...needGold', needGold)
    if not user.isResourceEnough(p.ResourceType.GOLD, needGold) then
        -- print('p.CS_ARENA_RESET_BATTLE_COUNT...gold is not enough')
        agent.sendNoticeMessage(p.ErrorCode.PUBLIC_GOLD_NOT_ENOUGH, '', 1)
        return
    end
    user.removeResource(p.ResourceType.GOLD, needGold, p.ResourceConsumeType.ARENA_RESET_BATTLE_COUNT)

    arena.hadCount = 0
    arena.resetCount = arena.resetCount + 1
    arena.sendArenaUserDataUpdate()
    agent.sendPktout(p.SC_ARENA_RESET_BATTLE_COUNT_RESPONSE, p.ErrorCode.SUCCESS)
end

return arena