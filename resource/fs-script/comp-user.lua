local agent = ...
local p = require('protocol')
local t = require('tploader')
local utils = require('utils')
local misc = require('libs/misc')
local timer = require('timer')
local dbo = require('dbo')
local trie = require('trie')
local event = require('libs/event')
local idStub = require('stub/id')
local loginStub = require('stub/login')
local logStub = require('stub/log')
local dataStub = require('stub/data')
local sdk = require('sdk')

local building = {}
local army = {}
local technology = {}
local bag = {}
local hero = {}
local cdlist = {}
local novice = agent.novice

local RESOURCE_MAX  = 2100000000

local user = {
    uid = 0,
    info = {
        uid = 0,
        channel = 0,
        username = '',
        nickname = '',
        headId = 0,
        camp = 1,   --heroCampType阵营:1魏 2蜀 3吴 4群
        level = 1,
        exp = 0,
        gold = 0,   --元宝
        silver = 0, --银两
        food = 0,
        wood = 0,
        iron = 0,
        stone = 0,
        skillPoint = 0,     --技能点
        --stamina info
        stamina = 0,        --当前体力
        staminaPropUseCount = 0, -- 体力道具使用次数    使用道具恢复一定的体力
        staminaBuyCount = 0,    --体力已购买次数        直接购买多少点体力
        lastStaminaRecoverTime = 0,--最后一次恢复体力时间
        --energy  info
        energy = 0,         --当前行动力
        energyBuyCount = 0,  --行动力购买次数
        lastEnergyRecoverTime = 0, --最后一次恢复行动力时间

        goldCharged = 0,
        langType = 1,
        --combat info
        attackWins = 0,
        attackLosses = 0,
        defenseWins = 0,
        defenseLosses = 0,
        scoutCount = 0,
        kills = 0,
        losses = 0,
        heals = 0,
        --Power
        lordPower = 0,
        troopPower = 0,
        buildingPower = 0,
        sciencePower = 0,
        trapPower = 0,
        heroPower = 0,
        totalPower = 0,
        captives = 0,
        castleLevel = 0,
        lastLoginTimestamp = 0,
        registerTimestamp = 0,
        banChatTimestamp = 0,
        banChatReason = '',
        lockedTimestamp = 0,
        lockedReason = '',
        isJoinRank = true,
        isSetName = false,
        storyId = 0,
        vipLevel = 0,
    },

    newPlayer = false,
    newToken = '',
    sdkType = nil,
    version = nil,
    loginInfo = { exp = 0 },
    loginLogs = {},

    --events
    evtResourceAdd = event.new(), -- (type, count, gainType)
    evtResourceRemove = event.new(), -- (type, count, consumeType)
    evtNicknameChange = event.new(),
    evtHeadIdChange = event.new(),
    evtLevelUp = event.new(), --(level)
    evtTotalPowerChange = event.new(), --()
    evtKillsChange = event.new(), --()
    evtCaptivesChange = event.new(), --()
    evtSetName = event.new(),   --()
    evtGoldConsume = event.new(), 
    evtBuyStamina = event.new(), 
}

local configIntervalTime = t.configure["PhysicalRecovery"]
local configEnergyIntervalTime = t.configure["energyRecovery"]
local nameLimit = t.configure["nameLimit"]
local staminaRecoverTime = 0
local energyRecoverTime = 0
function user.isLevelFull()
    return user.info.level >= #t.lordLevel
end

function user.checkIsChineseCharacter(nickname)
    local s = ""
    local a = 0
    -- %c   控制字符
    s,a = string.gsub(nickname, "%c", "")
    if a > 0 then
        -- print("控制字符")
        return false
    end
    -- %p   标点字符
    s,a = string.gsub(nickname, "%p", "" )
    if a > 0 then
        -- print("标点字符")
        return false
    end
    -- %s   空白符
    s,a = string.gsub(nickname, "%s", "")
    if a > 0 then
        -- print("空白符")
        return false
    end
    local chineseCharacter = {"！","@","#","￥","…","&","……","*","《","》","？","：", "“","{","}","|","~","，", " 　"}
    for _,v in pairs(chineseCharacter) do
        s,a = string.gsub(nickname, v, "")
        if a > 0 then
            return false
        end
    end
    return true
end

function user.checkNicknameInvalid(nickname)
    local nicknameLength = string.len(nickname)
    -- print('user.checkNicknameInvalid.....nicknameLength', nicknameLength)
    local realLength = 0
    -- ^[a-zA-Z0-9\u4e00-\u9fa5]+$
    -- 非法字符判断
    if not user.checkIsChineseCharacter(nickname) then
        agent.sendNoticeMessage(p.ErrorCode.NAME_IS_ILLEGAL, '', 1)
        return true
    end
    
    local temp = {}
    -- 长度判断
    for i=1, nicknameLength do
        local byte = string.byte(nickname, i)
        -- 不管是数字还是字母 中文 都占一个单位（单位：3个字节）
        if (byte >= 48 and byte <= 57) or (byte >= 65 and byte <= 90) or (byte >= 97 and byte <= 122) then
            realLength = realLength + 3
        elseif byte > 127 then
            realLength = realLength + 1
            -- 这里处理一下  qq 输入法的 v1a
            table.insert(temp,byte)
            if utils.serialize(temp) == utils.serialize({227, 128, 128}) then
                agent.sendNoticeMessage(p.ErrorCode.NAME_IS_ILLEGAL, '', 1)
                return true
            end
            if #temp == 3 then
                temp = {}
            end
        end
    end

    -- 数字和字符都不能超过6个，中文也不能超过6个
    if realLength < (nameLimit.min * 3) or realLength > (nameLimit.max * 3) then
        print("p.ErrorCode.NAME_LENGTH_LIMIT,p.ErrorCode.NAME_LENGTH_LIMIT,")
        agent.sendNoticeMessage(p.ErrorCode.NAME_LENGTH_LIMIT, '', 1)
        return true
    end 

    if trie.isContain(nickname) then
        print("p.ErrorCode.NAME_IS_CONTAINED_BADWORDSp.ErrorCode.NAME_IS_CONTAINED_BADWORDS")
        agent.sendNoticeMessage(p.ErrorCode.NAME_IS_CONTAINED_BADWORDS, '', 1)
        return true
    end
    return false
end

--头像解锁
function user.checkUnlockCond(headId)
    -- print('user.checkUnlockCond...headId', headId)
    local ret = false
    local tpl = t.lordHead[headId]
    if tpl and headId ~= user.info.headId then
        ret = agent.systemUnlock.checkUnlockedByList(tpl.unlockCond)
    end
    return ret
end

function user.addExp(exp, sendUpdate)
    if sendUpdate == nil then sendUpdate = true end

    exp = math.ceil(exp)
    local total = user.info.exp + exp
    local levelup = false
    local tpl = t.lordLevel[user.info.level]
    local dropList = {}
    local oldStamina = user.info.stamina
    local oldLevel = user.info.level
    local oldEnergy = user.info.energy
    while tpl ~= nil and total >= tpl.maxExp and not user.isLevelFull() do
        total = total - tpl.maxExp
        user.info.level = user.info.level + 1
        user.info.exp = 0
        levelup = true
        local oldTpl = tpl
        tpl = t.lordLevel[user.info.level]
        user.info.lordPower = tpl.power
        user.reCalculateTotalPower()
        dropList = bag.pickDropItemsByDropId(tpl.dropId)
        --stamina
        user.addStamina(oldTpl.staminaAdd, p.StaminaGainType.USER_UPGRADE)
        --energy
        --user.addEnergy(oldTpl.energyAdd,p.EnergyGainType.USER_UPGRADE)
        -- log
        logStub.appendRoleUpgrade(user.uid, user.info.level - 1, user.info.level, timer.getTimestampCache())
    end
    if levelup then
        user.info.exp = total
        user.evtLevelUp:trigger(user.info.level)
    else
        if user.isLevelFull() then
            user.info.exp = 0
        else
            user.info.exp = user.info.exp + exp
        end
    end
    user.loginInfo.exp = user.loginInfo.exp + exp
    if sendUpdate then
        user.sendUpdate()
    end
    if levelup then
        user.sendUserUpgradeData(dropList, oldStamina, oldLevel, oldEnergy)
    end
    return levelup
end

function user.isStaminaFull()
    local max = user.getStaminaLimit() or 0
    return user.info.stamina >= max
end

function user.isStaminaEnough(stamina)
    stamina = math.ceil(stamina)
    return stamina >= 0 and user.info.stamina >= stamina
end
function user.getStaminaLimit()
    local staminaLimit = t.configure['InitialPhysicalLimit'] or 0
    staminaLimit = math.ceil(staminaLimit)
    local tpl = t.vip[user.info.vipLevel] or {}
    if tpl then
        staminaLimit = staminaLimit + math.ceil(tpl.staminaLimit)
    end
    return staminaLimit
end

function user.getTotalStaminaRecoverTime()
    local staminaLimit = user.getStaminaLimit() or 0
    --print('staminaLimit,configIntervalTime',staminaLimit,configIntervalTime)
    local subCount = staminaLimit - user.info.stamina
    if subCount < 0 then
        subCount = 0
    end
    return subCount * configIntervalTime
end

function user.addStamina(stamina, gainType, sendUpdate)
    if not gainType then gainType = p.StaminaGainType.RECOVER end

    if gainType == p.StaminaGainType.STAMINA_WATER then 
        user.info.staminaPropUseCount = user.info.staminaPropUseCount + 1
    end
    stamina = math.ceil(stamina)
    if stamina <= 0 then
        return false
    end
    user.info.stamina = user.info.stamina + stamina
    if user.isStaminaFull() then
        staminaRecoverTime = 0
    end
    -- log
    logStub.appendStamina(user.uid, stamina, p.LogItemType.GAIN, gainType, timer.getTimestampCache())

    if sendUpdate then
        user.sendUpdate()
    end
    return true
end

function user.removeStamina(stamina, consumeType)
    if not consumeType then consumeType = p.StaminaConsumeType.SCENARIO_COPY end

    stamina = math.ceil(stamina)
    if stamina <= 0 then
        return false
    end
    if user.info.stamina < stamina then
        return false
    end
    user.info.stamina = user.info.stamina - stamina

    local now = timer.getTimestampCache()
    if not user.isStaminaFull() then
        if staminaRecoverTime == 0 then
            user.info.lastStaminaRecoverTime = now
            staminaRecoverTime = now + user.getTotalStaminaRecoverTime()
        end
    end
    -- log
    logStub.appendStamina(user.uid, stamina, p.LogItemType.CONSUME, consumeType, now)

    user.sendUpdate()
    return true
end

function user.staminaRecover(sendUpdate)
    local interval = math.ceil(configIntervalTime)
    local now = timer.getTimestampCache()
    local deffTime = now - user.info.lastStaminaRecoverTime

    local deff = deffTime / interval
    deff = math.floor(deff)
    local intervalMod = deffTime % interval
    local intervalTime = interval - intervalMod

    if deff > 0 then
        if not user.isStaminaFull() then
            user.addStamina(deff, p.StaminaGainType.RECOVER, sendUpdate)
            user.info.lastStaminaRecoverTime = now - intervalMod
        end
    end
    if user.isStaminaFull() then
        staminaRecoverTime = 0
    else
        staminaRecoverTime = now + intervalTime
    end
    --print("uid, now, interval, deffTime, deff, lastTime, intervalTime", user.info.uid, now, interval, deffTime, deff, user.info.lastStaminaRecoverTime, intervalTime)
end

function user.resetStaminaBuyCount(sendUpdate)
    user.info.staminaBuyCount = 0
    user.info.staminaPropUseCount = 0
    if sendUpdate then
        user.sendUpdate()
    end
end
--行动力
function user.getEnergyLimit()
    local energyLimit = t.configure['InitialEnergyLimit'] or 0
    energyLimit = math.ceil(energyLimit)
    local tpl = t.vip[user.info.vipLevel] or {}
    if tpl then
        energyLimit = energyLimit + math.ceil(tpl.energyLimit)
    end
    return energyLimit
end
function user.isEnergyFull()
    local max = user.getEnergyLimit() or 0
    return user.info.energy >= max
end

function user.isEnergyEnough(energy)
    energy = math.ceil(energy)
    return energy >= 0 and user.info.energy >= energy
end

function user.getTotalEnergyRecoverTime()
     local energyLimit = user.getEnergyLimit() or 0
    local subCount = energyLimit - user.info.energy
    if subCount < 0 then
        subCount = 0
    end
    return subCount * configEnergyIntervalTime
end

function user.addEnergy(energy, gainType, sendUpdate)
    if not gainType then gainType = p.EnergyGainType.RECOVER end

    if gainType == p.EnergyGainType.ENERGY_WATER then 
    end
    -- print("energy,   user.info.energy,   user.getEnergyLimit()...", energy, user.info.energy,  user.getEnergyLimit())
    energy = math.ceil(energy)
    if energy <= 0 then
        return false
    end
    user.info.energy = user.info.energy + energy
    if user.isEnergyFull() then
        energyRecoverTime = 0
    end
    -- log
    --logStub.appendStamina(user.uid, stamina, p.LogItemType.GAIN, gainType, timer.getTimestampCache())

    if sendUpdate then
        user.sendUpdate()
    end
    return true
end

function user.removeEnergy(energy, consumeType)
    if not consumeType then consumeType = p.EnergyConsumeType.ATTACK_MOSTER end
    energy = math.ceil(energy)
    if energy <= 0 then
        return false
    end
    if user.info.energy < energy then
        return false
    end
    user.info.energy = user.info.energy - energy

    local now = timer.getTimestampCache()
    if not user.isEnergyFull() then
        if energyRecoverTime == 0 then
            user.info.lastEnergyRecoverTime = now
            energyRecoverTime = now + user.getTotalEnergyRecoverTime()
        end
    end
    -- log
    --logStub.appendStamina(user.uid, stamina, p.LogItemType.CONSUME, consumeType, now)

    user.sendUpdate()
    return true
end

function user.energyRecover(sendUpdate)
    local interval = math.ceil(configEnergyIntervalTime)
    local now = timer.getTimestampCache()
    local deffTime = now - user.info.lastEnergyRecoverTime

    local deff = deffTime / interval
    deff = math.floor(deff)
    local intervalMod = deffTime % interval
    local intervalTime = interval - intervalMod

    if deff > 0 then
        if not user.isEnergyFull() then
            user.addEnergy(deff, p.EnergyGainType.RECOVER, sendUpdate)
            user.info.lastEnergyRecoverTime = now - intervalMod
        end
    end
    if user.isEnergyFull() then
        energyRecoverTime = 0
    else
        energyRecoverTime = now + intervalTime
    end
    --print("uid, now, interval, deffTime, deff, lastTime, intervalTime", user.info.uid, now, interval, deffTime, deff, user.info.lastStaminaRecoverTime, intervalTime)
end

function user.resetEnergyBuyCount(sendUpdate)
    user.info.energyBuyCount = 0
    if sendUpdate then
        user.sendUpdate()
    end
end

function user.isSkillPointFull()
    local max = t.configure['skillPointLimit'] or 0
    return user.info.skillPoint >= max
end

function user.isSkillPointEnough(skillPoint)
    skillPoint = math.ceil(skillPoint)
    return skillPoint >= 0 and user.info.skillPoint >= skillPoint
end

function user.addSkillPoint(skillPoint, gainType, sendUpdate)
    if skillPoint <= 0 then
        return false
    end
    skillPoint = math.ceil(skillPoint)
    user.info.skillPoint = user.info.skillPoint + skillPoint
    if user.isSkillPointFull() then
        user.info.skillPoint = t.configure['skillPointLimit'] 
    end
    -- log
    logStub.appendSkillPoint(user.uid, skillPoint, p.LogItemType.GAIN, gainType, timer.getTimestampCache())

    if sendUpdate then
        user.sendUpdate()
    end
    return true
end

function user.removeSkillPoint(skillPoint, consumeType)

    if not consumeType then 
        consumeType = p.ResourceConsumeType.HERO_DEBRIS_TURN 
    end
    skillPoint = math.ceil(skillPoint)
    if skillPoint <= 0 then
        return false
    end
    if user.info.skillPoint < skillPoint then
        return false
    end
    user.info.skillPoint = user.info.skillPoint - skillPoint
    -- log
    logStub.appendSkillPoint(user.uid, skillPoint, p.LogItemType.CONSUME, consumeType, now)

    user.sendUpdate()
    return true
end
function user.isResourceEnough(type, count)
    count = math.ceil(count)
    if count < 0 then
        return false
    end
    if type == p.ResourceType.FOOD then
        return user.info.food >= count
    elseif type == p.ResourceType.WOOD then
        return user.info.wood >= count
    elseif type == p.ResourceType.IRON then
        return user.info.iron >= count
    elseif type == p.ResourceType.STONE then
        return user.info.stone >= count
    elseif type == p.ResourceType.GOLD then
        return user.info.gold >= count
    elseif type == p.ResourceType.SILVER then
        return user.info.silver >= count
    end
    return false
end

function user.addResource(type, count, gainType, param1, sendUpdate)
    -- print('user.addResource...type, count, gainType, param1, sendUpdate', type, count, gainType, param1, sendUpdate)
    if not gainType then
        gainType = p.ResourceGainType.DEFAULT
    end
    if not param1 then
        param1 = 0
    end
    if sendUpdate == nil then
        sendUpdate = true
    end

    count = math.ceil(count)
    if count <= 0 then
        return false
    end
    local ok =  false
    if type == p.ResourceType.FOOD then
        user.info.food = user.info.food + count
        if user.info.food >= RESOURCE_MAX then
            user.info.food = RESOURCE_MAX
        end
        -- log
        logStub.appendFood(user.uid, count, p.LogItemType.GAIN, gainType, timer.getTimestampCache())
        ok = true
    elseif type == p.ResourceType.WOOD then
        user.info.wood = user.info.wood + count
        if user.info.wood >= RESOURCE_MAX then
            user.info.wood = RESOURCE_MAX
        end
        -- log
        logStub.appendWood(user.uid, count, p.LogItemType.GAIN, gainType, timer.getTimestampCache())
        ok = true
    elseif type == p.ResourceType.IRON then
        user.info.iron = user.info.iron + count
        if user.info.iron >= RESOURCE_MAX then
            user.info.iron = RESOURCE_MAX
        end
        -- log
        logStub.appendIron(user.uid, count, p.LogItemType.GAIN, gainType, timer.getTimestampCache())
        ok = true
    elseif type == p.ResourceType.STONE then
        user.info.stone = user.info.stone + count
        if user.info.stone >= RESOURCE_MAX then
            user.info.stone = RESOURCE_MAX
        end
        -- log
        logStub.appendStone(user.uid, count, p.LogItemType.GAIN, gainType, timer.getTimestampCache())
        ok = true
    elseif type == p.ResourceType.GOLD then
        user.info.gold = user.info.gold + count
        if user.info.gold >= RESOURCE_MAX then
            user.info.gold = RESOURCE_MAX
        end
        -- log
        logStub.appendGold(user.uid, count, param1, p.LogItemType.GAIN, gainType, timer.getTimestampCache())
        ok = true
    elseif type == p.ResourceType.SILVER then
        user.info.silver = user.info.silver + count
        if user.info.silver >= RESOURCE_MAX then
            user.info.silver = RESOURCE_MAX
        end
        -- log
        logStub.appendSilver(user.uid, count, p.LogItemType.GAIN, gainType, timer.getTimestampCache())
        ok = true
    elseif type == 17 then
        user.addSkillPoint(count,gainType,true)
        logStub.appendSkillPoint(user.uid, count, p.LogItemType.GAIN, gainType, timer.getTimestampCache())
        ok = true
    elseif type == p.SpecialPropIdType.COIN then
        --user.addCoin(count,p.StaminaGainType.STAMINA_WATER,true)
        ok = true
    elseif type == p.SpecialPropIdType.STAMINA then
        user.addStamina(count,p.StaminaGainType.STAMINA_WATER,true)
        ok = true
    elseif type == p.SpecialPropIdType.ENERGY then
        user.addEnergy(count,p.EnergyGainType.ENERGY_WATER,true)
        ok = true
    end
    if ok then
        if sendUpdate then
            user.sendUpdate()
        end
        user.evtResourceAdd:trigger(type, count, gainType)
    end
    return ok
end

function user.addResources(list, gainType)
    if not gainType then
        gainType = p.ResourceGainType.DEFAULT
    end
    for _, v in pairs(list) do
        if v.tplId == p.SpecialPropIdType.FOOD then
            user.addResource(p.ResourceType.FOOD, v.count, gainType, 0, false)
        elseif v.tplId == p.SpecialPropIdType.WOOD then
            user.addResource(p.ResourceType.WOOD, v.count, gainType, 0, false)
        elseif v.tplId == p.SpecialPropIdType.IRON then
            user.addResource(p.ResourceType.IRON, v.count, gainType, 0, false)
        elseif v.tplId == p.SpecialPropIdType.STONE then
            user.addResource(p.ResourceType.STONE, v.count, gainType, 0, false)
        elseif v.tplId == p.SpecialPropIdType.GOLD then
            user.addResource(p.ResourceType.GOLD, v.count, gainType, 0, false)
        elseif v.tplId == p.SpecialPropIdType.SILVER then
            user.addResource(p.ResourceType.SILVER, v.count, gainType, 0, false)
        end
    end
    user.sendUpdate()
end

function user.removeResource(type, count, consumeType, param1, sendUpdate)
    if not consumeType then
        consumeType = p.ResourceConsumeType.DEFAULT
    end
    if not param1 then
        param1 = 0
    end
    if sendUpdate == nil then
        sendUpdate = true
    end

    count = math.ceil(count)
    if count <= 0 then
        return false
    end

    local ok =  false
    if type == p.ResourceType.FOOD then
        if user.info.food >= count then
            user.info.food = user.info.food - count
            -- log
            logStub.appendFood(user.uid, count, p.LogItemType.CONSUME, consumeType, timer.getTimestampCache())
            ok = true
        end
    elseif type == p.ResourceType.WOOD then
        if user.info.wood >= count then
            user.info.wood = user.info.wood - count
            -- log
            logStub.appendWood(user.uid, count, p.LogItemType.CONSUME, consumeType, timer.getTimestampCache())
            ok = true
        end
    elseif type == p.ResourceType.IRON then
        if user.info.iron >= count then
            user.info.iron = user.info.iron - count
            -- log
            logStub.appendIron(user.uid, count, p.LogItemType.CONSUME, consumeType, timer.getTimestampCache())
            ok = true
        end
    elseif type == p.ResourceType.STONE then
        if user.info.stone >= count then
            user.info.stone = user.info.stone - count
            -- log
            logStub.appendStone(user.uid, count, p.LogItemType.CONSUME, consumeType, timer.getTimestampCache())
            ok = true
        end
    elseif type == p.ResourceType.GOLD then
        if user.info.gold >= count then
            user.info.gold = user.info.gold - count
            -- log
            logStub.appendGold(user.uid, count, param1, p.LogItemType.CONSUME, consumeType, timer.getTimestampCache())
            ok = true

            user.evtGoldConsume:trigger(count)
        end
    elseif type == p.ResourceType.SILVER then
        if user.info.silver >= count then
            user.info.silver = user.info.silver - count
            -- log
            logStub.appendSilver(user.uid, count, p.LogItemType.CONSUME, consumeType, timer.getTimestampCache())
            ok = true
        end
    end
    if ok then
        if sendUpdate then
            user.sendUpdate()
        end
        user.evtResourceRemove:trigger(type, count, consumeType)
    end
    return ok
end

function user.addKillsAndLosses(kills, losses)
    local update = false
    if kills > 0 then
        user.info.kills = user.info.kills + kills
        user.evtKillsChange:trigger()
        update = true
    end
    if losses > 0 then
        user.info.losses = user.info.losses + losses
        update = true
    end
    -- update to client
    if update then



        user.sendUpdate()
    end
end

function user.getLangKey(prefix)
    for key, val in pairs(p.LangType) do
        if user.info.langType == val then
            return prefix .. key
        end
    end
    return prefix .. 'CN'
end
function user.setVipLevel(level)
    local tpl = t.vip[level]
    if tpl then
        user.info.vipLevel = level
    end
end
function user.setHead(headId)
    local tpl = t.lordHead[headId]
    if tpl and headId ~= user.info.headId then
        user.info.headId = headId
        user.evtHeadIdChange:trigger()
        user.sendUpdate()
        return true
    end
    return false
end

function user.setNickname(nickname, session)
    nickname = tostring(nickname)
    local function replySetOrCheckNicknameResponse(result)
        agent.replyPktout(session, p.SC_SET_OR_CHECK_NICKNAME_RESPONSE, result)
    end

    print("###user.setNickname",nickname)

    if user.checkNicknameInvalid(nickname) then
        replySetOrCheckNicknameResponse(p.SetNicknameResultType.BAD_FORMAT)
        return false
    end

    local db = dbo.open(0)
    local rs = db:executePrepare('UPDATE s_user SET nickname = ? where uid = ?', nickname, user.uid)
    if rs.ok and rs.affectedRows > 0 then
        user.info.nickname = nickname
        user.evtNicknameChange:trigger()
        replySetOrCheckNicknameResponse(p.SetNicknameResultType.OK)
        user.sendUpdate()
        return true
    else
        replySetOrCheckNicknameResponse(p.SetNicknameResultType.EXIST)
        return false
    end
end

function user.firstSetNickName(nickname)
    nickname = tostring(nickname)

    local db = dbo.open(0)
    local rs = db:executePrepare('UPDATE s_user SET nickname = ? where uid = ?', nickname, user.uid)
    if rs.ok and rs.affectedRows > 0 then
        user.info.nickname = nickname
        user.evtNicknameChange:trigger()
        user.info.isSetName = true
        user.sendUpdate()
        return p.SetNicknameResultType.OK
    else
        agent.sendNoticeMessage(p.ErrorCode.NAME_WAS_USED, '', 1)
        return p.SetNicknameResultType.EXIST
    end
end

function user.getCrossTeleportInfo(conf)
    local uinfo = user.info
    local t = {}
    t.uid = uinfo.uid
    t.channel = uinfo.channel
    t.username = uinfo.username
    t.nickname = uinfo.name
    t.headId = uinfo.headId
    t.camp = uinfo.camp
    t.level = uinfo.level
    t.exp = uinfo.exp
    t.gold = uinfo.gold
    t.silver = uinfo.silver
    t.food = uinfo.food
    t.wood = uinfo.wood
    t.iron = uinfo.iron
    t.stone = uinfo.stone
    if t.food > conf.food then t.food = conf.food end
    if t.wood > conf.wood then t.wood = conf.wood end
    if t.iron > conf.iron then t.iron = conf.iron end
    if t.stone > conf.stone then t.stone = conf.stone end
    t.stamina = uinfo.stamina
    t.staminaBuyCount = uinfo.staminaBuyCount
    t.staminaPropUseCount = uinfo.staminaPropUseCount
    t.lastStaminaRecoverTime = uinfo.lastStaminaRecoverTime
    t.energy = uinfo.energy
    t.energyBuyCount = uinfo.energyBuyCount
    t.lastEnergyRecoverTime = uinfo.lastEnergyRecoverTime
    t.goldCharged = uinfo.goldCharged
    t.langType = uinfo.langType
    --combat info
    t.attackWins = uinfo.attackWins
    t.attackLosses = uinfo.attackLosses
    t.defenseWins = uinfo.defenseWins
    t.defenseLosses = uinfo.defenseLosses
    t.scoutCount = uinfo.scoutCount
    t.kills = uinfo.kills
    t.losses = uinfo.losses
    t.heals = uinfo.heals
    --Power
    t.lordPower = uinfo.lordPower
    t.troopPower = uinfo.troopPower
    t.buildingPower = uinfo.buildingPower
    t.sciencePower = uinfo.sciencePower
    t.trapPower = uinfo.trapPower
    t.heroPower = uinfo.heroPower
    t.totalPower = uinfo.totalPower
    t.captives = uinfo.captives
    t.lastLoginTimestamp = uinfo.lastLoginTimestamp
    t.registerTimestamp = uinfo.registerTimestamp
    t.banChatTimestamp = uinfo.banChatTimestamp
    t.banChatReason = uinfo.banChatReason
    t.lockedTimestamp = uinfo.lockedTimestamp
    t.lockedReason = uinfo.lockedReason
    t.isSetName = uinfo.isSetName
    t.storyId = uinfo.storyId
	t.vipLevel = uinfo.vipLevel
    return t
end



--  ======================   Login & Register   START    ============================

local function loadUserInfo(db, username)
    local rs = db:executePrepare('SELECT uid, channel, username, nickname, headId, camp, level, exp, silver, gold, food, wood, iron, stone, stamina, skillPoint, staminaBuyCount, lastStaminaRecoverTime, goldCharged, langType, attackWins, attackLosses, defenseWins, defenseLosses, scoutCount, kills, losses, heals, lordPower, troopPower, buildingPower, sciencePower, trapPower, heroPower, totalPower, captives, castleLevel, lastLoginTimestamp, registerTimestamp, banChatTimestamp, banChatReason, lockedTimestamp, lockedReason, isJoinRank, isSetName , storyId, staminaPropUseCount, energy, energyBuyCount, lastEnergyRecoverTime, vipLevel FROM s_user where username = ?', username)
    if rs.ok then
        if rs.rowsCount > 0 then
            for _, row in ipairs(rs) do
                local info = user.info
                user.uid = row.uid
                info.uid = row.uid
                info.channel = row.channel
                info.username = row.username
                info.nickname = row.nickname
                info.headId = row.headId
                info.camp = row.camp
                info.level = row.level
                info.exp = row.exp
                info.silver = row.silver
                info.gold = row.gold
                info.food = row.food
                info.wood = row.wood
                info.iron = row.iron
                info.stone = row.stone
                info.stamina = row.stamina
                info.skillPoint = row.skillPoint
                info.staminaBuyCount = row.staminaBuyCount
                info.lastStaminaRecoverTime = row.lastStaminaRecoverTime
                info.goldCharged = row.goldCharged
                info.langType = row.langType
                info.attackWins = row.attackWins
                info.attackLosses = row.attackLosses
                info.defenseWins = row.defenseWins
                info.defenseLosses = row.defenseLosses
                info.scoutCount = row.scoutCount
                info.kills = row.kills
                info.losses = row.losses
                info.heals = row.heals
                info.lordPower = row.lordPower
                info.troopPower = row.troopPower
                info.buildingPower = row.buildingPower
                info.sciencePower = row.sciencePower
                info.trapPower = row.trapPower
                info.heroPower = row.heroPower
                info.totalPower = row.totalPower
                info.captives = row.captives
                info.castleLevel = row.castleLevel
                info.lastLoginTimestamp = row.lastLoginTimestamp
                info.registerTimestamp = row.registerTimestamp
                info.banChatTimestamp = row.banChatTimestamp
                info.banChatReason = row.banChatReason
                info.lockedTimestamp = row.lockedTimestamp
                info.lockedReason = row.lockedReason   
                info.isSetName = row.isSetName
                info.storyId = row.storyId
                info.staminaPropUseCount = row.staminaPropUseCount
                info.energy = row.energy
                info.energyBuyCount = row.energyBuyCount
                info.lastEnergyRecoverTime = row.lastEnergyRecoverTime
                info.vipLevel = row.vipLevel
                -- info.isJoinRank = row.isJoinRank
                break
            end
            return 1
        else
            return 0
        end
    else
        return -1
    end
end

local function registerUser(db, username, idfa, channel, langType)
    local uid = idStub.createUid()
    print('========registerUser=======', channel, uid)
    if uid == 0 then
        return -1
    end
    -- 注册操作
    local validLangType = false
    if langType > 0 then
        for k,v in pairs(p.LangType) do
            if v == langType then
                validLangType = true
                break
            end
        end
    end
    if not validLangType then
        langType = p.LangType.CN
    end

    local now = timer.getTimestampCache()
    local initialResource = t.configure['initialResource']
    local food = initialResource.food
    local wood = initialResource.wood
    local iron = initialResource.iron
    local stone = initialResource.stone
    local gold = initialResource.gold
    local silver = initialResource.silver
    local stamina = t.configure['InitialPhysical']
    local skillPoint = 0
    -- 到时候去掉这个 TODO by zds
    local headId = t.configure["DefaultImage"] or 3000001
    local lastStaminaRecoverTime = now
    local staminaBuyCount = 0
    local staminaPropUseCount = 0
    local energy = t.configure['InitialEnergy']
    local energyBuyCount = 0
    local lastEnergyRecoverTime = now
    local lastLoginTimestamp = now
    local registerTimestamp = now
    local vipLevel = 0
    local sql = 'INSERT INTO s_user (uid, idfa, channel, username, nickname, headId, level, food, wood, iron, stone, gold, silver, stamina, skillPoint, langType, staminaBuyCount, lastStaminaRecoverTime, lastLoginTimestamp, registerTimestamp ,energy,energyBuyCount,lastEnergyRecoverTime,vipLevel) VALUES (?, ?, ?, ?, ?, ?, 1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)'
    local rs = db:executePrepare(sql, uid, idfa, channel, username, 'K' .. uid, headId, food, wood, iron, stone, gold, silver, stamina, skillPoint, langType, staminaBuyCount, lastStaminaRecoverTime, lastLoginTimestamp, registerTimestamp,energy,energyBuyCount,lastEnergyRecoverTime,vipLevel);
    --print('===============rs', utils.toJson(rs))
    if rs.ok and rs.affectedRows > 0 then
        return 1
    else
        return -1
    end
end

-- 登录到中心服务器
local function loginToCenterServer(replyLoginResponse, uid, token, isReconnect)
    local rawStub = loginStub.rawStub
    local rpcOk, ok = false, false
    local maxTry = 20
    while maxTry > 0 and not ok do
        rpcOk, ok = rawStub:call_login(uid, agent.mailbox:getMailboxId(), token, isReconnect)
        timer.sleep(800)
        maxTry = maxTry - 1
    end

    if ok then
        return true
    else
        return false
    end
end

-- 处理登录协议
local function handleCSLogin(pktin, session)
    local sdkType, username, token, idfa, channel, version, langType, isReconnect = pktin:read('ssssssib')

    print('request login', sdkType, username, token, idfa, channel, version, langType, isReconnect)
    -- utils.log(string.format('request login %s %s %s %s %s %s %s %s', sdkType, username, token, idfa, channel, version, tostring(langType), tostring(isReconnect)))

    local function replyLoginResponse(result, reason)
        print('replyLoginResponse', result, reason)
        if not reason then
            reason = ''
        end
        agent.replyPktout(session, p.SC_LOGIN_RESPONSE, result, reason)
    end

    if loginStub.csShutdown then
        replyLoginResponse(p.LoginResultType.UNKNOWN_ERROR)
        agent.exit()
        return
    end

    -- check sdkType
    if not sdk.isValid(sdkType) then
        utils.log('handleCSLogin fail : sdk.isValid false username=' .. username)
        replyLoginResponse(p.LoginResultType.UNKNOWN_ERROR)
        agent.exit()
        return
    end

    -- check openId
    local openId = username
    local pos = username:find('_')
    if pos then
        openId = misc.strTrim(username:sub(pos + 1))
        if openId == '' then
            -- username must be format:  serverId_openId
            utils.log('handleCSLogin fail : username wrong format username=' .. username)
            replyLoginResponse(p.LoginResultType.UNKNOWN_ERROR)
            agent.exit()
            return
        end
    elseif sdkType ~= 'client_test' then
        -- username must be format:  serverId_openId
        utils.log('handleCSLogin fail : username wrong format username=' .. username)
        replyLoginResponse(p.LoginResultType.UNKNOWN_ERROR)
        agent.exit()
        return
    end

    local db = dbo.open(0)
    local r = loadUserInfo(db, username)
    --print('loadUserInfo .. username,stamina',username,user.info.stamina)
    -- check locked or not
    if r == 1 and user.info.lockedTimestamp > timer.getTimestampCache() then
        utils.log('handleCSLogin fail : user locked username=' .. username)
        replyLoginResponse(p.LoginResultType.LOCKED, user.info.lockedReason)
        agent.exit()
        return
    end

    local function sdkVerifyCallback(result)
        -- print("###sdkVerifyCallback",debug.traceback())
        if not result then
            utils.log('handleCSLogin fail : sdkVerifyCallback false username=' .. username)
            replyLoginResponse(p.LoginResultType.BAD_SESSION)
            agent.exit()
            return
        end
        -- login to center server
        local ok = false
        if not isReconnect then -- the first time to login, create new token to cs for verifycation while reconnecting
            user.newToken = tostring(utils.createItemId())
        else
            user.newToken = token
        end
        do
            local db = dbo.open(0)
            if r == 1 then
                ok = loginToCenterServer(replyLoginResponse, user.uid, user.newToken, isReconnect)
                r = loadUserInfo(db, username)
                assert(r == 1)
            elseif r == 0 then
                if registerUser(db, username, idfa, channel, langType) == 1 then
                    user.newPlayer = true
                    r = loadUserInfo(db, username)
                    assert(r == 1)
                    ok = loginToCenterServer(replyLoginResponse, user.uid, user.newToken, isReconnect)
                end
            end
        end
        if ok then
            user.sdkType = sdkType
            user.version = version
            -- log infos
            user.loginInfo.uid = user.info.uid
            user.loginInfo.exp = 0
            user.loginInfo.login_level = user.info.level
            user.loginInfo.login_time = timer.getTimestampCache()
            user.loginInfo.isReconnect = isReconnect
            -- report player infos
            local params = {
                openId = openId,
                session = token,
                gameUid = user.uid,
                gameNickname = user.info.nickname,
                gameLevel = user.info.level,
                gameServerId = utils.getMapServiceId(),
                timestamp = timer.getTimestampCache()
            }
            if not isReconnect then
                sdk.report(sdkType, params)
            end

            -- set trust and go on
            agent.setTrust()
            replyLoginResponse(p.LoginResultType.OK, user.newToken)
            -- print("*****after register ", utils.serialize(user))
            user.sendUpdate()
            user.sendIsSetNameUpdate()
        else
            utils.log('handleCSLogin fail : loginToCenterServer false username=' .. username)
            replyLoginResponse(p.LoginResultType.BAD_SESSION)
            agent.exit()
        end
    end

    -- vefiry sdk login
    local params = {
        openId = openId,
        session = token,
        gameServerId = utils.getMapServiceId(),
        timestamp = timer.getTimestampCache()
    }
    if isReconnect then
        sdkVerifyCallback(true)
    else
        sdk.verify(sdkVerifyCallback, sdkType, params)
    end
end

--  ======================   Login & Register   END    ============================


function user.onInit()
    local pktin, session = agent.waitPktin(p.CS_LOGIN)
    handleCSLogin(pktin, session)

    user.info.lastLoginTimestamp = timer.getTimestampCache()
end

function user.onClose()
    if user.timer then
        user.timer:cancel()
        user.timer = nil
    end
    user.onSave()
end

function user.onAllCompInit()
    building = agent.building
    army = agent.army
    technology = agent.technology
    bag = agent.bag
    hero = agent.hero
    cdlist = agent.cdlist
    user.info.castleLevel = building.castleLevel()
end

function user.onSave()
    -- login infos

    local db = dbo.open(0)
    local u = user.info

    --联盟数据
    local allianceId = 0
    local allianceName = ''
    local allianceNickname = ''
    local allianceBannerId = 0
    local aInfo = agent.alliance.info
    if aInfo ~= nil then
        allianceId = aInfo.id
        allianceName = aInfo.name
        allianceNickname = aInfo.nickname
        allianceBannerId = aInfo.bannerId
    end
    --千层楼 排行榜数据
    local babelMaxLayer = agent.babel.historylayer or 0
    local babelTimestamp = agent.babel.historyTime or 0
    --铜雀台 排行榜数据
    --local bronzeMaxScore = agent.bronzeSparrowTower.historyScore or 0
    local bronzeTodayScore = agent.bronzeSparrowTower.todayScore or 0
    local bronzeTodayTimestamp = agent.bronzeSparrowTower.answerTime or 0
    local mapInfo = agent.map.info
    local dataRow = { uid = u.uid,  username = u.username,  nickname = u.nickname, camp = u.camp, headId = u.headId, level = u.level, exp = u.exp, silver = u.silver, gold = u.gold, food = u.food, wood = u.wood, iron = u.iron, stone = u.stone, stamina = u.stamina, skillPoint = u.skillPoint, staminaBuyCount = u.staminaBuyCount,
    lastStaminaRecoverTime = u.lastStaminaRecoverTime, goldCharged = u.goldCharged, langType = u.langType, attackWins = u.attackWins, attackLosses = u.attackLosses, defenseWins = u.defenseWins, defenseLosses = u.defenseLosses, 
    scoutCount = u.scoutCount, kills = u.kills, losses = u.losses, heals = u.heals, lordPower = u.lordPower, troopPower = u.troopPower, buildingPower = u.buildingPower, sciencePower = u.sciencePower, trapPower = u.trapPower, heroPower = u.heroPower, 
    totalPower = u.totalPower, captives = u.captives, lastLoginTimestamp = u.lastLoginTimestamp, castleLevel = u.castleLevel, allianceId = allianceId, allianceName = allianceName, allianceNickname = allianceNickname, allianceBannerId = allianceBannerId, 
    x = mapInfo.x, y = mapInfo.y, isJoinRank = u.isJoinRank, babelMaxLayer = babelMaxLayer, babelTimestamp = babelTimestamp, bronzeMaxScore = bronzeMaxScore, bronzeTodayScore = bronzeTodayScore, bronzeTodayTimestamp = bronzeTodayTimestamp , isSetName = u.isSetName ,
    storyId = u.storyId,staminaPropUseCount = u.staminaPropUseCount ,energy = u.energy ,energyBuyCount = u.energyBuyCount ,lastEnergyRecoverTime = u.lastEnergyRecoverTime,vipLevel = u.vipLevel}
    dataStub.appendData(p.DataClassify.USER_UPDATE, dataRow)
end

function user.onReady()
    if cdlist.isHour5Refresh then
        user.resetStaminaBuyCount()
        user.resetEnergyBuyCount()
    end
    cdlist.evtHour5Refresh:attachRaw(function()
        user.resetStaminaBuyCount()
        user.resetEnergyBuyCount()
    end)

    user.reCalculateLordPower(false)
    user.reCalculateBuildingPower(false)
    user.reCalculateTroopPower(false)
    user.reCalculateTrapPower(false)
    user.reCalculateSciencePower(false)
    user.reCalculateHeroPower(false)

    building.evtBuildingLevelUp:attachRaw(user.reCalculateBuildingPower)
    building.evtBuildingDemolish:attachRaw(user.reCalculateBuildingPower)
    building.evtCastleLevelUp:attachRaw(user.updateCastleLevel)
    army.evtArmyAdd:attachRaw(user.reCalculateTroopPower)
    army.evtArmyAdd:attachRaw(user.reCalculateTrapPower)
    army.evtArmySub:attachRaw(user.reCalculateTroopPower)
    army.evtArmySub:attachRaw(user.reCalculateTrapPower)
    army.evtTeamChange:attachRaw(user.reCalculateTroopPower)
    technology.evtAddLevel:attachRaw(user.reCalculateSciencePower)
    --hero
    hero.evtAddHero:attachRaw(user.reCalculateHeroPower)
    hero.evtHeroLevelUp:attachRaw(user.reCalculateHeroPower)
    hero.evtHeroStarUp:attachRaw(user.reCalculateHeroPower)

    agent.property.evtPropertyChange:attachRaw(function()
            user.reCalculateHeroPower(true)
        end)

    --
    user.staminaRecover(false)
    user.energyRecover(false)
    user.sendUpdate()
    agent.sendPktout(p.SC_READY)
end

function user.onTimerUpdate(timerIndex)
    local now = timer.getTimestampCache()
    if staminaRecoverTime ~= 0 and now >= staminaRecoverTime then
        user.staminaRecover(true)
    end
    if energyRecoverTime ~= 0 and now >= energyRecoverTime then
        user.energyRecover(true)
    end
end

function user.reCalculateTotalPower()
    local u = user.info
    local oldTotalPower = u.totalPower
    u.totalPower = u.lordPower + u.troopPower + u.buildingPower + u.sciencePower + u.trapPower + u.heroPower
    u.totalPower = math.floor(u.totalPower)
    if oldTotalPower ~= u.totalPower then
        -- power change event
        user.evtTotalPowerChange:trigger(u.totalPower)
    end
end

function user.reCalculateLordPower(sendUpdate)
    local tpl = t.lordLevel[user.info.level]
    if tpl then
        user.info.lordPower = tpl.power
        user.reCalculateTotalPower()
        if sendUpdate then
            user.sendUpdate()
        end
    end
end

function user.reCalculateTroopPower(sendUpdate)
    local u = user.info
    local tempPower = u.troopPower
    u.troopPower = 0

    for _, v in pairs(army.armies) do
        local level = army.getArmyLevelByArmyType(v.armyType)
        local tpl = t.findArmysTpl(v.armyType, level)
        if tpl and not army.isTrap(tpl.type) then
            u.troopPower = u.troopPower + tpl.power * (v.states[p.ArmyState.NORMAL] or 0)
        end
    end
    for _, team in pairs(army.teams) do
        for _, v in pairs(team.confList) do
            local level = army.getArmyLevelByArmyType(v.armyType)
            local tpl = t.findArmysTpl(v.armyType, level)
            if tpl and not army.isTrap(tpl.type) then
                u.troopPower = u.troopPower + tpl.power * v.count
            end
        end
    end
    u.troopPower = math.floor(u.troopPower)

    user.reCalculateTotalPower()
    if sendUpdate or tempPower ~= u.troopPower then
        user.sendUpdate()
    end
end

function user.reCalculateTrapPower(sendUpdate)
    local u = user.info
    u.trapPower = 0

    for _, v in pairs(army.armies) do
        local level = army.getArmyLevelByArmyType(v.armyType)
        local tpl = t.findArmysTpl(v.armyType, level)
        if tpl and army.isTrap(tpl.subType) then
            u.trapPower = u.trapPower + tpl.power * (v.states[p.ArmyState.NORMAL] or 0)
        end
    end

    u.trapPower = math.floor(u.trapPower)
    user.reCalculateTotalPower()
    if sendUpdate then
        user.sendUpdate()
    end
end

function user.reCalculateBuildingPower(sendUpdate)
    local u = user.info
    u.buildingPower = 0
    for _, v in pairs(building.list) do
        local tpl = t.building[v.tplId]
        if tpl then
            local levelTpl = tpl.levels[v.level]
            if levelTpl then
                u.buildingPower = u.buildingPower + levelTpl.power
            end
        end
    end

    user.reCalculateTotalPower()
    if sendUpdate then
        user.sendUpdate()
    end
end

function user.reCalculateSciencePower(sendUpdate)
    local u = user.info
    u.sciencePower = 0
    for type,techTreeInfo in pairs(technology.techTreeList) do
        if techTreeInfo then
            for groupId,techGroupInfo in pairs(techTreeInfo.techGroupList) do
                if techGroupInfo then
                    local tplId = techGroupInfo.tplId  
                    local tpl = t.technology[tplId]
                    if tpl then
                       u.sciencePower = u.sciencePower + tpl.power
                    end
                end
            end
        end
    end
    user.reCalculateTotalPower()
    if sendUpdate then
        user.sendUpdate()
    end
end

function user.reCalculateHeroPower(sendUpdate)
    local oldHeroPower = user.info.heroPower or 0
    local heroPower = 0
    for _, v in pairs(hero.info) do
        heroPower = heroPower + v:getSingleHeroPower(agent.property.getHeroProp())
    end
    
    if heroPower > 0  and heroPower ~= oldHeroPower then
        user.info.heroPower = math.floor(heroPower)
        user.reCalculateTotalPower()
        if sendUpdate then
            user.sendUpdate()
        end
    end
end

function user.updateCastleLevel(level)
    user.info.castleLevel = level
    agent.alliance.onMemberUpdate()
end

function user.sendUserUpgradeData(dropList, oldStamina, oldLevel, oldEnergy)
    if dropList then
        -- print('user.sendUserUpgradeData...dropList,oldStamina, user.info.stamina, oldLevel, user.info.level,oldEnergy,user.info.energy', utils.serialize(dropList),oldStamina, user.info.stamina, oldLevel, user.info.level,oldEnergy,user.info.energy)
        agent.sendPktout(p.SC_USER_UPGRADE_DATA, '@@1=[tplId=i,count=i],2=i,3=i,4=i,5=i,6=i,7=i', dropList, oldStamina, user.info.stamina, oldLevel, user.info.level,oldEnergy,user.info.energy)
    end
end

function user.sendUpdate()
    local u = user.info
    local banChatLeftSeconds = u.banChatTimestamp - timer.getTimestampCache()
    if banChatLeftSeconds < 0 then
        banChatLeftSeconds = 0
    end
    -- print("###user.sendupdate", debug.traceback())
    --print("user.sendUpdate", utils.serialize(u))
    local staminaTime = user.getTotalStaminaRecoverTime()

    local energyTime = user.getTotalEnergyRecoverTime()
    local staminaLimit = user.getStaminaLimit()
    local energyLimit = user.getEnergyLimit()
    --print('user.sendUpdate-------------u.storyId,u.stamina, u.staminaBuyCount, staminaTime,u.staminaBuyCount,u.energy,u.energyRecoverTime,u.energyBuyCount', u.storyId, u.stamina, u.staminaBuyCount, staminaTime,u.staminaBuyCount,u.energy,energyTime,u.energyBuyCount)
    agent.sendPktout(p.SC_USER_UPDATE, u.uid, u.headId, u.nickname, u.level, u.exp, u.silver, u.gold, u.food, u.wood, u.iron, u.stone, u.stamina, u.skillPoint, u.staminaBuyCount, staminaTime, banChatLeftSeconds, u.banChatReason, u.attackWins, u.attackLosses, u.defenseWins, u.defenseLosses, u.scoutCount, u.lordPower, u.buildingPower, u.sciencePower, u.troopPower, u.trapPower, u.heroPower, u.totalPower , u.storyId, u.staminaPropUseCount,u.energy, u.energyBuyCount, energyTime , staminaLimit , energyLimit )
    -- print("###user.sendUpdate,u.uid, u.headId, u.nickname, u.level, u.exp, u.silver, u.gold, u.food, u.wood, u.iron, u.stone, u.stamina, u.skillPoint, u.staminaBuyCount, staminaTime, banChatLeftSeconds, u.banChatReason, u.attackWins, u.attackLosses, u.defenseWins, u.defenseLosses, u.scoutCount, u.lordPower, u.buildingPower, u.sciencePower, u.troopPower, u.trapPower, u.heroPower, u.totalPower, u.storyId = ", u.uid, u.headId, u.nickname, u.level, u.exp, u.silver, u.gold, u.food, u.wood, u.iron, u.stone, u.stamina, u.skillPoint, u.staminaBuyCount, staminaTime, banChatLeftSeconds, u.banChatReason, u.attackWins, u.attackLosses, u.defenseWins, u.defenseLosses, u.scoutCount, u.lordPower, u.buildingPower, u.sciencePower, u.troopPower, u.trapPower, u.heroPower, u.totalPower, u.storyId)
    -- print('###user.sendUpdate..',u.uid, u.nickname, u.silver, u.gold, u.food, u.wood, u.iron, u.stone, u.stamina, u.skillPoint, u.headId, u.level, u.exp)
end

function user.sendIsSetNameUpdate()
    local u = user.info
    agent.sendPktout(p.SC_IS_SET_NAME, u.isSetName)
end

function user.cs_set_lang_type(lang, session)
    local function setLangResponse(result)
        --print('setLangResponse', result)
        agent.replyPktout(session, p.SC_SET_LANG_TYPE_RESPONSE, result)
    end

    local ok = false
    for _, v in pairs(p.LangType) do
        if v == lang then
            ok = true
            user.info.langType = lang
            user.sendUpdate()
            break
        end
    end
    setLangResponse(ok)
end

-- 设置或检查昵称
function user.cs_set_or_check_nickname(onlyCheck, nickname, session)
    local function replySetOrCheckNicknameResponse(result)
        agent.replyPktout(session, p.SC_SET_OR_CHECK_NICKNAME_RESPONSE, result)
    end
    -- print('p.CS_SET_OR_CHECK_NICKNAME...onlyCheck, nickname', onlyCheck, nickname)
    if user.checkNicknameInvalid(nickname) then
        replySetOrCheckNicknameResponse(p.SetNicknameResultType.BAD_FORMAT)
        return
    end

    agent.queueJob(function()
        local db = dbo.open(0)
        if onlyCheck then
            local rs = db:executePrepare('SELECT uid FROM s_user WHERE nickname=?', nickname)
            if rs.ok and rs.rowsCount == 0 then
                replySetOrCheckNicknameResponse(p.SetNicknameResultType.OK)
            else
                replySetOrCheckNicknameResponse(p.SetNicknameResultType.EXIST)
            end
        end
    end)
end

function user.cs_stamina_buy(session)
    local function staminaBuyResponse(result)
        print('p.CS_STAMINA_BUY..result', result)
        agent.replyPktout(session, p.SC_STAMINA_BUY_RESPONSE, result)
    end
    --1.判断购买次数是否已达到上限
    local max = agent.vip.tpl.staminaBuyCount
    if user.info.staminaBuyCount >= max then
        print('p.CS_STAMINA_BUY...staminaBuyCount >= max', user.info.staminaBuyCount, max)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local tpl = t.staminaBuy[user.info.staminaBuyCount + 1]
    if tpl == nil then
        print('p.CS_STAMINA_BUY... tpl_stamina_buy no datas....staminaBuyCount', user.info.staminaBuyCount)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --2.检查金币是否足够
    local gold = tpl.gold
    if not user.isResourceEnough(p.ResourceType.GOLD, gold) then
        agent.sendNoticeMessage(p.ErrorCode.PUBLIC_GOLD_NOT_ENOUGH, '', 1)
        return
    end
    --
    user.removeResource(p.ResourceType.GOLD, gold, p.ResourceConsumeType.STAMINA_BUY, 0, false)
    user.addStamina(tpl.stamina, p.StaminaGainType.BUY, false)

    user.info.staminaBuyCount = user.info.staminaBuyCount + 1

    user.evtBuyStamina:trigger()

    user.sendUpdate()
    staminaBuyResponse(true)
end
--TODO:需要增加tpl_energy_buy表 tpl_vip中增加 energyBuyCount
function user.cs_energy_buy(session)
    local function energyBuyResponse(result)
        print('p.CS_ENERGY_BUY..result', result)
        agent.replyPktout(session, p.SC_ENERGY_BUY_RESPONSE, result)
    end
    --1.判断购买次数是否已达到上限
    local max = agent.vip.tpl.energyBuyCount
    if user.info.energyBuyCount >= max then
        print('p.CS_ENERGY_BUY...energyBuyCount >= max', user.info.energyBuyCount, max)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local tpl = t.energyBuy[user.info.energyBuyCount + 1]
    if tpl == nil then
        print('p.CS_ENERGY_BUY... tpl_ENERGY_BUY no datas....energyBuyCount', user.info.energyBuyCount)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --2.检查金币是否足够
    local gold = tpl.gold
    if not user.isResourceEnough(p.ResourceType.GOLD, gold) then
        agent.sendNoticeMessage(p.ErrorCode.PUBLIC_GOLD_NOT_ENOUGH, '', 1)
        return
    end
    --
    user.removeResource(p.ResourceType.GOLD, gold, p.ResourceConsumeType.ENERGY_BUY, 0, false)
    user.addEnergy(tpl.energy, p.EnergyGainType.BUY, false)

    user.info.energyBuyCount = user.info.energyBuyCount + 1

    user.sendUpdate()
    energyBuyResponse(true)
end
function user.cs_get_random_name(sex, session)
    local function GetRandomResponse(name)
        -- print('p.SC_GET_RANDOM_NAME_RESPONSE..name', name)
        agent.replyPktout(session, p.SC_GET_RANDOM_NAME_RESPONSE, name)
    end
    local function GetRandomStr(strList)
        local count = #strList
        local randNum = utils.getRandomNum(1, count) 
        return strList[randNum]
    end
    -- 如果已经设置过名字，则不走随机
    if user.info.isSetName then
        print("###p.CS_GET_RANDOM_NAME... user was setName!!!")
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
    end
    local xingCount = #t.xing
    local randXing = GetRandomStr(t.xing)
    local randName
    if sex == 0 then
        randName = GetRandomStr(t.manName)
    else 
        randName = GetRandomStr(t.womanName)
    end
    GetRandomResponse(randXing..randName)
end

function user.cs_set_name(nickname, session)
    local function SetNameReponse(result)
        -- print("p.CS_SET_NAME...result", result)
        agent.replyPktout(session, p.SC_SET_NAME_RESPONSE, result)
    end
    -- print("###p.CS_SET_NAME", nickname)
    if user.checkNicknameInvalid(nickname) then
        SetNameReponse(p.SetNicknameResultType.BAD_FORMAT)
        return false
    end
    agent.queueJob(function()
        local result = user.firstSetNickName(nickname)
        SetNameReponse(result)
        if result == p.SetNicknameResultType.OK then
            user.evtSetName:trigger()
        end
    end)
end
function user.cs_story_id_update(storyId, session)
    print('p.CS_STORY_ID_UPDATE..storyId=',storyId)
    user.info.storyId = storyId
    agent.replyPktout(session, p.SC_STORY_ID_UPDATE_RESPONSE, user.info.storyId)
end

function user.cs_fill_resource(session)
    local function FillResourceReponse(result)
        print("p.CS_SET_NAME...result", result)
        agent.replyPktout(session, p.SC_FILL_RESOURCE_RESPONSE, result)
    end
    -- local nickname = pktin:read('s')
    -- if user.checkNicknameInvalid(nickname) then
    --     SetNameReponse(p.SetNicknameResultType.BAD_FORMAT)
    --     return false
    -- end
    -- local db = dbo.open(0)
    -- local rs = db:executePrepare('UPDATE s_user SET nickname = ? where uid = ?', nickname, user.uid)
    -- if rs.ok and rs.affectedRows > 0 then
    --     user.info.nickname = nickname
    --     user.info.isSetName = true
    --     SetNameReponse(p.SetNicknameResultType.OK)
    --     user.sendUpdate()
    -- else
    --     SetNameReponse(p.SetNicknameResultType.EXIST)
    -- end
end

return user
