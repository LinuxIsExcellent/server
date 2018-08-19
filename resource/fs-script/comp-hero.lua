local agent = ...
local p = require('protocol')
local t = require('tploader')
local dbo = require('dbo')
local timer = require('timer')
local event = require('libs/event')
local utils = require('utils')
local misc = require('libs/misc')
local pubHero = require('hero')
local logStub = require('stub/log')
local dataStub = require('stub/data')
local arenaStub = require('stub/arena')
local achievementStub = require('stub/achievement')

local dict = agent.dict
local user = agent.user
local bag = agent.bag
local army = agent.army
local property = agent.property

local initHeroHave = t.configure['initHeroHave'] or {}
local initHeroDraw = t.configure['initHeroDraw']
local firstSingleDraw = t.configure['firstSingleDraw'] or {}
local heroSkillUpgradeConsume = t.configure['heroSkillUpgradeConsume'] or 10
local heroEquipUpgradeConsume = t.configure['heroEquipUpgradeConsume'] or 5
local heroPhysicalRecoverTime = t.configure['heroPhysical'].interval or 5    --武将体力恢复时间间隔
local heroPhysicalRecovery = t.configure['heroPhysical'].recovery or 1      --武将体力恢复百分比

-- 免费抽卡间隔时间
local mixExpireTime = 0             --免费到期时间

local MAX_LEFT_COUNT = 10           --抽到紫橙品质武将的上限
local MAX_ACTIVE_SKILL_COUNT = 2    --主动技能数量上限
local MAX_SKILL_BREAK = 5           --技能突破系数

local hero = {
    info = {},      --武将 map<id, heroInfo>
    datas = {},     --抽卡数据 map<heroDrawType, dataInfo>
    removeList = {},-- map< table=<type, slot> >

    --events
    --武将新增
    evtAddHero = event.new(),   --(heroId)
    evtDrawHero = event.new(),   --(count)
    evtDrawHeroById = event.new(),    --(heroId)
    evtDrawHeroByType = event.new(),
    --武将等级星级
    evtHeroLevelUp = event.new(),    --(heroId, oldLevel)
    evtHeroLevelUpOne = event.new(),    --(heroId, newLevel)
    evtHeroStarUp = event.new(),    --(heroId, star)
    evtSkillUpgrade = event.new(),  --(level)

    --属性变化
    evtChangeHeroBaseProperty = event.new(),  --(heroId, arenaDataType, slot)
}

--抽卡数据信息
local dataInfo = {
    drawType = 0,       --抽取类型 heroDrawType
    expireFreeTime = 0, --免费抽取时间
    blueRate = 0,       --抽蓝抽紫概率
    otherRate = 0,      --抽紫抽橙概率
    leftCount = 0,      --还差几次可以抽到高品质武将:对名将，热门有效
    nextQuality = 0,    --品质 ItemQualityType
    isFirst = false,    --是否第一次单抽

    isSync = false,
}


function dataInfo:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end

function hero.onInit()
    print('hero init---------------------')
    hero.dbLoad()

    if user.newPlayer then
        if initHeroHave then
            for _, id in pairs(initHeroHave) do
                hero.addHero(id, p.HeroGainType.INIT, false)
            end
        end
    end

    for _,v in pairs(t.heroDrawType) do 
        -- 如果当前的抽卡类型不存在玩家的数据里面就生成一个抽卡类型数据给玩家
        if hero.datas[v.id] == nil then
            local isFirst = false       --  首抽：类型为 1   2   的需要
            if v.id <= 2 then
                isFirst = true
            end
            hero.datas[v.id] = dataInfo:new({
                drawType = v.id,
                expireFreeTime = 0,
                blueRate = v.blue,
                otherRate = v.other,
                leftCount = 10,                          -- 有的要用，有的不用，先都设置成10吧，如果数字不相同，就需要加在配置表里面加字段了
                nextQuality = v.initQ,
                isFirst = isFirst,
                })
        end
    end
    -- print("t.heroDrawType[1]", t.heroDrawType[1].initQ, t.heroDrawType[1].blue, t.heroDrawType[1].other, t.heroDrawType[1].change)
    --免费倒计时
    hero.resetExpireFreeTime()

    --武将属性变更
    hero.evtChangeHeroBaseProperty:attachRaw(hero.onChangeHeroBaseProperty)
    agent.achievement.evtAchievementFinish:attachRaw(hero.onAchievementFinish)
end

function hero.onAllCompInit()
    print("hero --------- onAllCompInit -----------")
    hero.sendHeroUpdate()
    hero.sendHeroSkillUpdate()
    hero.sendHeroSoulUpdate()
    hero.sendHeroDrawDataUpdate()
end

function hero.onSave()
    hero.dbSave()
end

function hero.onClose()
    hero.dbSave()
end

function hero.onTimerUpdate(timerIndex)
    local now = timer.getTimestampCache()
    if mixExpireTime ~= 0 and mixExpireTime <= now then
        -- print('hero.onTimerUpdate...timerIndex, mixExpireTime', timerIndex, mixExpireTime)
        hero.checkFreeTime()
    end
    hero.checkHeroPhysicalRecovery()
end

function hero.dbLoad()
    --heros
    local db = dbo.open(0)
    local rs = db:executePrepare('SELECT heroId, exp, level, star, soulLevel, physical, physicalRecoveryTimeStamp, slotNum, isLock, skill, soul from s_hero where uid = ?', user.uid)
    local now = timer.getTimestampCache()
    if rs.ok then
        for _, row in ipairs(rs) do
            local tpl = t.heros[row.heroId]
            if tpl then
                if row.skill == '' then
                    row.skill = '{}'
                end
                -- info.isActive
                row.skill = misc.deserialize(row.skill)
                row.soul = misc.deserialize(row.soul)
                hero.info[row.heroId] = pubHero.newHeroInfo({
                    id = row.heroId,
                    exp = row.exp,
                    level = row.level,
                    star = row.star,
                    soulLevel = row.soulLevel,
                    physical = row.physical,
                    physicalRecoveryTimeStamp = row.physicalRecoveryTimeStamp,
                    slotNum = row.slotNum,
                    isLock = row.isLock,
                    skill = row.skill,
                    soul = row.soul,
                    })
                local maxPhysical = hero.info[row.heroId].heroSolohp
                -- print("row.heroId, hero.info[row.heroId].physicalRecoveryTimeStamp, hero.info[row.heroId].physical, now", row.heroId, row.physicalRecoveryTimeStamp, hero.info[row.heroId].physical, now)
                if row.physicalRecoveryTimeStamp < now and hero.info[row.heroId].physical < maxPhysical then
                    local addCount = math.ceil((now - row.physicalRecoveryTimeStamp) / heroPhysicalRecoverTime)
                    local addTimeStamp = now - row.physicalRecoveryTimeStamp - (addCount * heroPhysicalRecoverTime)
                    local newPhysical = hero.info[row.heroId].physical + math.floor(maxPhysical * heroPhysicalRecovery / 100) * addCount
                    if newPhysical > maxPhysical then
                        newPhysical = maxPhysical
                    end
                    hero.info[row.heroId].physical = math.floor(newPhysical)
                    hero.info[row.heroId].physicalRecoveryTimeStamp = hero.info[row.heroId].physicalRecoveryTimeStamp + heroPhysicalRecoverTime
                end
            end
        end
    end

    --datas
    local dts = dict.get('hero.datas') or {}
    local datas = dts.datas or {}
    for _, v in pairs(datas) do
        hero.datas[v.drawType] = dataInfo:new(v)
    end

    --print
    -- for _, v in pairs(hero.info) do
    --     -- print('###hero.dbLoad..hero.info', utils.serialize(v))
    --     v:dump()
    -- end
    -- for _, v in pairs(hero.datas) do
    --     print('###hero.dbLoad..hero.datas', utils.serialize(v))
    -- end
end

function hero.dbSave()
    --heros
    local db = dbo.open(0)
    local list = {}
    for _, v in pairs(hero.info) do
        -- print('----------------v.isDirty', v.isDirty)
        if v.isDirty then
            v.isDirty = false
            local skillTemp = {}
            for flag, info in pairs(v.skill) do
                skillTemp[flag] = { tplId = info.tplId, type = info.type, level = info.level, slot = info.slot, isActive = info.isActive }
            end

            local skill = utils.serialize(skillTemp)
            local soulTemp = {}
            for soulId, info in pairs(v.soul) do
                soulTemp[soulId] = {tplId = info.tplId, soulLevel = info.soulLevel, level = info.level}
            end
            local soul = utils.serialize(soulTemp)
            -- print("###dbSave, uid, heroId, exp, level, physical, physicalRecoveryTimeStamp", user.uid, v.id, v.exp, v.level, v.physical, v.physicalRecoveryTimeStamp)
            table.insert(list, {
                uid = user.uid,
                heroId = v.id,
                exp = v.exp,
                level = v.level,
                star = v.star,
                soulLevel = v.soulLevel,
                physical = v.physical,
                physicalRecoveryTimeStamp = v.physicalRecoveryTimeStamp,
                slotNum = v.slotNum,
                isLock = v.isLock,
                skill = skill,
                soul = soul,
                })
        end
    end
    dataStub.appendDataList(p.DataClassify.HERO, list)
    -- print('###hero.dbSave..hero.info', utils.serialize(list))

    --datas
    local dts = {}
    dts.datas = {}
    for _, v in pairs(hero.datas) do
        table.insert(dts.datas, {
            drawType = v.drawType,
            expireFreeTime = v.expireFreeTime,
            blueRate = v.blueRate,
            otherRate = v.otherRate,
            leftCount = v.leftCount,
            nextQuality = v.nextQuality,
            isFirst = v.isFirst,
            })
    end
    dict.set('hero.datas', dts)

    -- print('###hero.dbSave..hero.datas', utils.serialize(hero.datas))
end

function hero.onChangeHeroBaseProperty(id, arenaDataType, skillId)
    local heroInfo = hero.info[id]
    -- print("hero.onChangeHeroBaseProperty", utils.serialize(heroInfo))
    if heroInfo then
        heroInfo:setHeroBaseProperty()
        hero.sendHeroUpdate()

        --sync to cs, arena
        hero.heroSyncToCs(id, arenaDataType)

        --sync to ms
        agent.map.syncHero()
    end
end

function hero.heroSyncToMs()
    local heroList = {}
    for k, v in pairs(hero.info) do
        local skill = {}
        for slot, info in pairs(v.skill) do
            -- print('hero.heroSyncToMs...heroId, slot, isOpen', v.id, slot, info.isOpen)
            -- 如果技能是活跃的，并且已经上了槽
            if info.type == p.HeroSkillType.HERO_SKILL_TALENT or (info.type == p.HeroSkillType.HERO_SKILL_STAR and info.slot ~= 0) then
                table.insert(skill, { tplId = info.tplId, level = info.level })
            end
        end

        heroList[k] = {
            id = v.id,
            level = v.level,
            star = v.star,
            physical = v.physical,
            skill = skill,
            additionList = v.additionList,
        }
        -- print('###hero.heroSyncToMs..heroId, type(skill), skill', v.id, type(v.skill), v.skill)
    end
    return heroList
end

function hero.heroSyncToCs(id, arenaDataType, slot)
    --判段是否是擂台防守武将
    local bArena = agent.arena.isArenaDefenseHero(id)
    -- todo by zds 
    if bArena then
        local heroInfo = hero.info[id]
        if heroInfo then
            if slot == nil then
                slot = 0
            end
            local data = {}
            
            -- 武将等级星级
            if p.ArenaDataType.HERO_LEVEL_STAR == arenaDataType then
                data.level = heroInfo.level
                data.star = heroInfo.star
            elseif p.ArenaDataType.HERO_SOUL == arenaDataType then
                local soulInfo = heroInfo.soul[tplId]
                if soulInfo then
                    data.tplId = tplId
                    data.level = soulInfo.level
                end
            --技能
            elseif p.ArenaDataType.HERO_SKILL == arenaDataType then
                local skillInfo = heroInfo.skill[tplId]
                if skillInfo then
                    data.tplId = tplId
                    data.level = skillInfo.level
                    data.slot = skillInfo.slot
                    data.type = skillInfo.type
                end
            elseif p.ArenaDataType.USER_DATA_CHANGE == arenaDataType then
                -- data.slot = slot
            else
                return
            end
            print('-----hero.heroSyncToCs------uid, heroId, arenaDataType, data', user.uid, id, arenaDataType, utils.serialize(data))
            arenaStub.cast_arenaHeroDataChange(user.uid, id, arenaDataType, data)
        end
    end
end

function hero.getHeroLevelAndStar(id)
    local info = hero.info[id]
    local level = 0
    local star = 0
    if info then
        level = info.level or 0
        star = info.star or 0
    end
    return level, star
end

function hero.getHeroDrawProp(bid)
    local prop = bag.getItemBybid(bid)
    if not prop then
        return nil
    end
    if prop.subType ~= p.ItemPropType.HERO_DRAW_PROP then
        return nil
    end
    return prop
end

function hero.getHeroCountByQuality(quality)
    if quality == nil then
        quality = 0
    end

    local count = 0
    if quality == 0 then
        for _, v in pairs(hero.info) do
            count = count + 1
        end
    else
        for _, v in pairs(hero.info) do
            if v.tpl.quality == quality then
                count = count + 1
            end
        end
    end
    return count
end

function hero.getHeroCountByLevel(level)
    local count = 0
    for _, v in pairs(hero.info) do
        if v.level >= level then
            count = count + 1
        end
    end
    return count
end

function hero.getHeroCountByStar(star)
    local count = 0
    for _, v in pairs(hero.info) do
        if v.star >= star then
            count = count + 1
        end
    end
    return count
end

function hero.addHero(id, gainType, bSendUpdate)
    -- print('###addHero..', id)
    if gainType == nil then
        gainType = p.HeroGainType.DEFAULT
    end
    if bSendUpdate == nil then
        bSendUpdate = true
    end
    if not hero.info[id] then
        local info = pubHero.newHeroInfo({ id = id })
        if info then
            hero.info[id]= info
            if bSendUpdate then
                hero.sendHeroUpdate()
                hero.sendHeroSkillUpdate()
            end
            --log
            logStub.appendHero(user.uid, id, info.level, info.star, p.LogItemType.GAIN, gainType, timer.getTimestampCache())

            hero.evtAddHero:trigger(id)
            hero.detectSupportSkill(id)
        else
            utils.log(string.format('tpl_hero no this hero...heroId=%i', id))
        end
    else
        local tpl = t.heros[id]
        if tpl then
            local debrisCount = hero.heroChangeDebris(id)
            hero.addDebris(tpl.heroDebris, debrisCount)
        end
    end
end

function hero.detectSupportSkill(id)
    for _, v in pairs(hero.info) do
        if v.tpl.support[id] then
            -- 增加羁绊技能
            if not v.skill[v.tpl.support[id]] then
                v.skill[v.tpl.support[id]] = pubHero.skillInfo:new({tplId = v.tpl.support[id], type = p.HeroSkillType.HERO_SKILL_FETTER, level = 1, slot = 0})
            end
        end
    end
end

function hero.addHeroByCond(id, level, star, gainType, bSendUpdate)
    -- print('hero.addHeroByCond...id, level, star, gainType, bSendUpdate', id, level, star, gainType, bSendUpdate)
    if gainType == nil then
        gainType = p.HeroGainType.DEFAULT
    end
    if bSendUpdate == nil then
        bSendUpdate = true
    end
    if not hero.info[id] then
        local info = pubHero.newHeroInfo({ id = id, level = level, star = star })
        if info then
            hero.info[id]= info
            if bSendUpdate then
                hero.sendHeroUpdate()
                hero.sendHeroSkillUpdate()
            end
            --log
            logStub.appendHero(user.uid, id, info.level, info.star, p.LogItemType.GAIN, gainType, timer.getTimestampCache())

            hero.evtAddHero:trigger(id)
        else
             utils.log(string.format('tpl_hero no this hero...heroId=%i', id))
        end
    else
        local debrisCount = hero.heroChangeDebris(id, star)
        local tpl = t.heros[id]
        if tpl then
            hero.addDebris(tpl.heroDebris, debrisCount)
        end
    end
end

function hero.addDebris(itemId, debrisCount)
    -- print('hero.addDebris..itemId, debrisCount', itemId, debrisCount)
    if itemId > 0 and debrisCount > 0 then
        bag.addItem(itemId, debrisCount, p.ResourceGainType.HERO_DRAW, true)
    end
end

function hero.heroChangeDebris(id, star)
    --print('hero.heroChangeDebris...', id, star)
    local debrisCount = 0
    if t.heros[id] then
        star = star or t.heros[id].star
        local tplStar = t.heroStarlevel[id]
        if tplStar and tplStar[star] then
            debrisCount = tplStar[star].convertDebris
        end
    end
    if debrisCount == 0 then
        debrisCount = 10
    end
    --print('hero.heroChangeDebris...id, star, debrisCount', id, star, debrisCount)
    return debrisCount
end

function hero.getOpenHeroNeedDebris(id)
    local needDebris = 0
    if t.heros[id] then
        local initStar = t.heros[id].star
        needDebris = t.heroStarlevel[id][initStar].debris
    end
    return needDebris
end

function hero.getHeroExpItemListToFullHeroLevel(id)
    local itemList = {}
    local info = hero.info[id]
    local lordTpl = t.lordLevel[user.info.level]
    local expCount = 0
    for i = info.level, lordTpl.maxHeroLevel do
        local tpl = t.heroLevel[i]
        if i == lordTpl.maxHeroLevel then
            expCount = expCount + tpl.exp - info.exp - 1
        else
            expCount = expCount + tpl.exp
        end 
    end
    if expCount <= 0 then
        return itemList
    end
    local items = bag.getItemsBySubType(p.ItemPropType.HERO_EXP)
    local function itemSortFun(item1, item2)
        return item1.quality < item2.quality
    end
    if not items then
        return itemList
    end
    print("utils.serialize...items,begin ", utils.serialize(items))
    table.sort(items, itemSortFun)
    print("utils.serialize...items,end ", utils.serialize(items))
    for _, v in pairs(items) do
        local itemTpl = t.item[v.id]
        if not itemTpl then
            utils.log('hero.getHeroExpItemListToFullHeroLevel not exist item tpl in bag_items, tplId=' .. v.id)
        end
        if expCount < itemTpl.param1 * v.count then
            itemList[v.id] = math.ceil(expCount / itemTpl.param1)
            return itemList
        elseif expCount == itemTpl.param1 * v.count then
            itemList[v.id] = expCount / itemTpl.param1
            return itemList
        elseif expCount > itemTpl.param1 * v.count then
            itemList[v.id] = v.count
            expCount = expCount - itemTpl.param1 * v.count
        end
    end
    print("utils.serialize...itemList", utils.serialize(itemList))
    return itemList
end

function hero.canAddHeroExp(id)
    local info = hero.info[id]
    local lordTpl = t.lordLevel[user.info.level]
    if info and lordTpl then
        local level = info.level or 0
        local exp = info.exp or 0
        if level < lordTpl.maxHeroLevel then
            return true
        elseif level == lordTpl.maxHeroLevel then
            local tpl = t.heroLevel[level]
            if tpl then
                if exp < tpl.exp - 1 then
                    return true
                end
            end
        end
    end
    return false
end

function hero.addHeroExp(id, exp, bSendUpdate)
    -- print(debug.traceback())
    -- print('hero.addHeroExp...id, exp', id, exp, bSendUpdate)
    exp = math.ceil(exp)
    local info = hero.info[id]
    local lordTpl = t.lordLevel[user.info.level]
    if lordTpl and info and exp > 0 then
        local isLevelUp = false
        local total = info.exp + exp
        local tpl = t.heroLevel[info.level]
        local oldLevel = info.level
        local maxLevel = #t.heroLevel
        while tpl
            and total >= tpl.exp
            and info.level < maxLevel
            and info.level < lordTpl.maxHeroLevel do
            total = total - tpl.exp
            info.level = info.level + 1
            tpl = t.heroLevel[info.level]
            info.exp = 0
            isLevelUp = true
            hero.evtHeroLevelUpOne:trigger(id, info.level)
            -- print('hero.addHeroExp..heroLevel', info.level)
            --log
            logStub.appendHeroUpgrade(user.uid, id, info.level - 1, info.level, info.star, info.star, timer.getTimestampCache())
        end
        if total >= tpl.exp then
            info.exp = tpl.exp - 1
        else
            info.exp = total
        end
        if isLevelUp then
            hero.evtHeroLevelUp:trigger(id, oldLevel)
            hero.evtChangeHeroBaseProperty:trigger(id, p.ArenaDataType.HERO_LEVEL_STAR)
        end
        -- print('hero.addHeroExp..heroExp', info.exp)

        info.isDirty = true
        info.isSync = false
    end
    if bSendUpdate then
        hero.sendHeroUpdate()
    end
end

function hero.recoveryPhysical(heroId, tempCount)
    local heroInfo = hero.info[heroId]
    local addPhysical = math.floor((tempCount / 10000) * heroInfo.heroSolohp)
    if addPhysical + heroInfo.physical > heroInfo.heroSolohp then
        heroInfo.physical = heroInfo.heroSolohp
    else
        heroInfo.physical = heroInfo.physical + addPhysical
    end
    heroInfo.isSync = false
    heroInfo.isDirty = true
    hero.sendHeroUpdate()
end

function hero.checkFreeTime()
    -- print('###hero.checkFreeTime....')
    local bSend = false
    for _, data in pairs(hero.datas) do
        local leftTime = hero.getHeroLeftFreeTime(data.drawType, data.expireFreeTime)
        if leftTime <= 0 then
            bSend = true
            data.isSync = false
        end
    end
    if bSend then
        hero.sendHeroDrawDataUpdate()
        hero.resetExpireFreeTime()
    end
end

function hero.checkHeroPhysicalRecovery()
    -- todo by zds, testin battle
    local now = timer.getTimestampCache()
    local isUpdate = false
    for _, v in pairs(hero.info) do
        -- 不能大于当前最大值
        local maxPhysical = v.heroSolohp
        -- print("v.physicalRecoveryTimeStamp", v.physicalRecoveryTimeStamp)
        -- print("v.physical", v.physical)
        if v.physicalRecoveryTimeStamp <= now and v.physical < maxPhysical then
            if not agent.army.heroIsMarch(v.id) then
                local newPhysical = v.physical + math.floor(maxPhysical * heroPhysicalRecovery / 100)
                if newPhysical > maxPhysical then
                    newPhysical = maxPhysical
                end
                v.physical = math.floor(newPhysical)
                -- print("newPhysical, v.physical", newPhysical, v.physical)
            end
            v.isDirty = true
            -- v.isSync = false
            -- isUpdate = true
            -- 不一直发送，1.上线的时候发送  2.武将数据变化的时候发送（不包括随时间恢复）
            v.physicalRecoveryTimeStamp = now + heroPhysicalRecoverTime
        end
    end
    if isUpdate then
        hero.sendHeroUpdate()
    end
end

function hero.resetExpireFreeTime()
    mixExpireTime = 0
    local now = timer.getTimestampCache()
    for _, data in pairs(hero.datas) do
        if now < data.expireFreeTime and
            (mixExpireTime == 0 or mixExpireTime > data.expireFreeTime) then
            mixExpireTime = data.expireFreeTime
        end
    end
    -- print('hero.resetExpireFreeTime...mixExpireTime', mixExpireTime)
end

function hero.getExpireFreeTimeByType(drawType)
    -- print("###hero.getExpireFreeTimeByType", drawType)
    for _,v in pairs(t.heroDrawType) do 
        if v.id == drawType then
            return v.free
        end
    end
    return 0
end

function hero.getHeroLeftFreeTime(drawType, expireFreeTime)
    -- print('###hero.getHeroLeftFreeTime...drawType, expireFreeTime', drawType, expireFreeTime)
    local now = timer.getTimestampCache()
    local leftTime = expireFreeTime - now
    if leftTime < 0 then
        leftTime = 0
    end
    -- print('###hero.getHeroLeftFreeTime..drawType, leftTime', drawType, leftTime)
    return leftTime
end

function hero.getTotalTop5HeroPower()
    local list = {}
    for _, v in pairs(hero.info) do
        local power = v:getHeroPower(agent.property.getHeroProp())
        table.insert(list, power)
    end
    table.sort(list, function(lValue, rValue) return lValue > rValue end)
    local total = 0
    for i=1, 5 do
        if list[i] then
            total = total + list[i]
        end
    end
    return total
end

--TODO:成就事件类型，确定牌库
function hero.getEventType(drawType)
    local et = 0
    if drawType == p.heroDrawType.HOT then
        et = 1
    end
    -- print('hero.getEventType...drawType, et', drawType, et)
    return et
end
--成就事件到期剩余时间
function hero.achievementLeftTime(achievementId)
    local leftTime = 0
    local now = timer.getTimestampCache()
    local ach = achievementStub.achieves[achievementId]
    if ach then
        leftTime = ach.finishTime + ach.tpl.eventSeconds - now
    end
    if leftTime < 0 then
        leftTime = 0
    end

    --print('hero.achievementLeftTime...achievementId, nowTime, expiredTime, leftTime', achievementId, now, ach.finishTime + ach.tpl.eventSeconds, leftTime)
    return leftTime
end

function hero.onAchievementFinish(achievementId)
    local bSend = false
    local updates = {}
    local data = hero.datas[p.heroDrawType.HOT]
    local ach = achievementStub.achieves[achievementId]
    if data and ach then
        local achList = {}
        local leftTimeExpired = hero.achievementLeftTime(achievementId)
        if leftTimeExpired > 0 then
            bSend = true
            table.insert(achList, { achievementId = achievementId, leftTimeExpired = leftTimeExpired })
        end
        table.insert(updates, {
            type = p.heroDrawType.HOT,
            leftTime = 0,
            leftCount = 0,
            quality = 0,
            consumeOne = data.consumeOne,
            consumeTen = data.consumeTen,
            achList = achList,
            })
    end
    if bSend then
        hero.sendHeroDrawDataUpdate(updates)
    end
end

--解锁条件
function hero.checkUnlockCond(list, param1)
    -- print('hero.checkUnlockCond...list, param1', utils.serialize(list), param1)
    return agent.systemUnlock.checkUnlockedByList(list, param1)
end

function hero.sendHeroUpdate()
    -- print('--------hero.sendHeroUpdate......')
    -- print('hero.info.skillInfo.....', utils.serialize(hero.info))
    local updateList = {}
    for _, v in pairs(hero.info) do
        -- print("------------- ", v.isSync)
        if not v.isSync then
            v.isSync = true
            local additionList = {}
            for attrType, attrInfo in pairs(v.additionList) do
                table.insert(additionList, {attrType = attrType, attrBaseAdd = attrInfo[p.AttributeAdditionType.BASE], 
                    attrPctAdd = attrInfo[p.AttributeAdditionType.PCT], attrExtAdd = attrInfo[p.AttributeAdditionType.EXT]})
            end
            table.insert(updateList, { id = v.id, exp = v.exp, level = v.level, star = v.star , 
                soulLevel = v.soulLevel , physical = v.physical, slotNum = v.slotNum, additionList = additionList})
        end
    end
    -- for _, v in pairs(updateList) do
    --     print('###hero.sendHeroUpdate...updateList', utils.serialize(v))
    -- end
    -- print('###hero.sendHeroUpdate...updateList', utils.serialize(updateList))
    if next(updateList) then
        agent.sendPktout(p.SC_HERO_UPDATE, '@@1=[id=i,exp=i,level=i,star=i,soulLevel=i,physical=i,slotNum=i,additionList=[attrType=i,attrBaseAdd=f,attrPctAdd=f,attrExtAdd=f]]', updateList)
    end
end

function hero.sendHeroSkillUpdate()
    local updateList = {}
    -- print('hero.sendHeroSkillUpdate...heroId, star, skill, isActive', utils.serialize(hero.info))
    for _, v in pairs(hero.info) do
        local skillList = {}
        -- print('hero.sendHeroSkillUpdate...heroId, star, skill, isActive', utils.serialize(v))
        for k, info in pairs(v.skill) do
            -- print('hero.sendHeroSkillUpdate...heroId, star, skill, isActive', v.id, v.star, info.isActive, info.isOpen, utils.serialize(info))
            if not info.isSync then
                info.isSync = true
                table.insert(skillList, {tplId = info.tplId, skillType = info.type, level = info.level, slot = info.slot})
            end
        end
        table.insert(updateList, { id = v.id, skillList = skillList })
    end
    -- print('hero.sendHeroSkillUpdate updateList =', utils.serialize(updateList))
    if next(updateList) then
        agent.sendPktout(p.SC_HERO_SKILL_UPDATE, '@@1=[id=i,skillList=[tplId=i,skillType=i,level=i,slot=i]]', updateList)
    end
end

function hero.sendHeroSoulUpdate()
    local updateList = {}
    -- print('hero.sendHeroSoulUpdate...h', utils.serialize(hero.info))
    for _, v in pairs(hero.info) do
        local soulList = {}
        for k, info in pairs(v.soul) do
            -- 只把当前阶级的命魂发过去
            if info.soulLevel == v.soulLevel then
                if not info.isSync then
                    info.isSync = true
                    table.insert(soulList, {tplId = info.tplId, level = info.level})
                end
            end
        end
        -- print('hero.sendHeroSoulUpdate soulSeri =', utils.serialize(v.soul))
        table.insert(updateList, { id = v.id, soulList = soulList })
    end
    -- print('hero.sendHeroSoulUpdate updateList =', utils.serialize(updateList))
    if next(updateList) then
        agent.sendPktout(p.SC_HERO_SOUL_UPDATE, '@@1=[id=i,soulList=[tplId=i,level=i]]', updateList)
    end
end

function hero.sendHeroDrawDataUpdate(updates)
    if updates == nil then
        updates = {}
        for _, v in pairs(hero.datas) do
            if not v.isSync then
                v.isSync = true
                local leftTimeFree = hero.getHeroLeftFreeTime(v.drawType, v.expireFreeTime)
                -- print('###hero.sendHeroDrawDataUpdate...drawType, leftTime, sub', v.drawType, leftTime, v.expireFreeTime - timer.getTimestampCache())
                local consumeOne = t.heroDrawType[v.drawType].conOne or 0
                local consumeTen = t.heroDrawType[v.drawType].conTen or 0
                if v.drawType == p.heroDrawType.STAR or v.drawType == p.heroDrawType.HOT then
                    if v.nextQuality < p.ItemQualityType.PURPLE then
                        v.nextQuality = p.ItemQualityType.PURPLE
                    end
                end
                --成就事件
                local newAchievement = 0
                local achList = {}
                if v.drawType == p.heroDrawType.HOT then
                    for achievementId, ach in pairs(achievementStub.achieves) do
                        if ach.finishTime > 0 and achievementId > newAchievement then
                            newAchievement = achievementId
                        end
                    end
                end
                if newAchievement > 0 then
                    local leftTimeExpired = hero.achievementLeftTime(newAchievement)
                    if leftTimeExpired > 0 then
                        table.insert(achList, { achievementId = newAchievement, leftTimeExpired = leftTimeExpired })
                    end
                end

                --print('###hero.sendHeroDrawDataUpdate... achList', utils.serialize(achList))

                table.insert(updates, {
                    type = v.drawType,
                    leftTime = leftTimeFree,
                    leftCount = v.leftCount,
                    quality = v.nextQuality,
                    consumeOne = consumeOne,
                    consumeTen = consumeTen,
                    achList = achList,
                    })
            end
        end
    end
    -- print('###hero.sendHeroDrawDataUpdate... updates', utils.serialize(updates))
    -- agent.sendPktout(p.SC_HERO_DRAW_DATA_UPDATE, '@@1=[type=i,leftTime=i,leftCount=i,quality=i,consumeOne=i,consumeTen=i]', updates)
    agent.sendPktout(p.SC_HERO_DRAW_DATA_UPDATE, '@@1=[type=i,leftTime=i,leftCount=i,quality=i,consumeOne=i,consumeTen=i,achList=[achievementId=i,leftTimeExpired=i]]', updates)
end

function hero.cs_hero_draw(drawType, isTen, bid, achievementId, session)
    local function heroDrawResponse(result, list)
        if not list then
            list = {}
        end
        print('heroDrawResponse', result, utils.serialize(list))
        agent.replyPktout(session, p.SC_HERO_DRAW_RESPONSE, '@@1=b,2=[id=i,debrisCount=i,heroId=i]', result, list)
    end
    --print('###p.CS_HERO_DRAW..drawType, isTen, bid, achievementId', drawType, isTen, bid, achievementId)

    local data = {}
    for k, v in pairs(hero.datas) do
        if drawType == v.drawType then
            data = hero.datas[k]
        end
    end
    if not data or not next(data) then
        print('hero.datas is empty...drawType', drawType)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    -- print('###p.CS_HERO_DRAW..data1111', utils.serialize(data))

    local init = t.heroDrawType[drawType]
    if not init or not next(init) then
        print('initHeroDraw is empty...drawType', drawType)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    local list = {}
    local function draw(id, val)
        -- print('draw...id, val', id, val)
        --根据权重抽卡
        local count = val
        if not t.heroDraw[id] or not t.heroDraw[id].heroPool then
            print('tpl_hero_draw data is empty ..id', id)
            return
        end
        local heroPool = misc.deserialize(t.heroDraw[id].heroPool) or {}

        local containOtherDraw = misc.deserialize(t.heroDraw[id].containOtherDraw) or {}
        for _,v in pairs(containOtherDraw) do
            local otherPool = misc.deserialize(t.heroDraw[v].heroPool)
            --print('containOtherDraw otherPool', utils.serialize(otherPool))
            for k,value in pairs(otherPool) do
                heroPool[k] = value
            end
        end

        --print('draw heroPool....', utils.serialize(heroPool))
        if not next(heroPool) then
            print('tpl_hero_draw heroPool is empty..id', id)
            return
        end
        local rateList = {}
        local oldRate = 1
        local sum = 0
        for k,value in pairs(heroPool) do
            local weight = value[1] or 0
            local count = value[2] or 0
            local canDraw = false
            if count == 0 then
                if t.heros[k] and t.heros[k].isCanUse == 1 then
                    canDraw = true
                end
            else
                canDraw = true
            end

            if canDraw then
                table.insert(rateList, { id = k, min = oldRate, max = oldRate + weight - 1, count = count})
                oldRate = oldRate + weight
                sum = sum + weight    
            end
        end
        if not next(rateList) then
            print('draw heroPool is wrong..', t.heroDraw[id].heroPool)
            return
        end
        while count > 0 do
            local randNum = utils.getRandomNum(1, sum)
            for _, v in pairs(rateList) do
                if v.min <= randNum and randNum <= v.max then
                    if v.count == 0 then
                        local debrisCount = 0
                        local tpl = t.heros[v.id]
                        if tpl then
                            if hero.info[v.id] then
                                debrisCount = hero.heroChangeDebris(v.id)
                                hero.addDebris(tpl.heroDebris, debrisCount)
                                table.insert(list, { id = tpl.heroDebris, debrisCount = debrisCount, heroId = v.id})
                            else
                                -- hero.info[v.id] = pubHero.newHeroInfo({ id = v.id })
                                hero.addHero(v.id, p.HeroGainType.HERO_DRAW, false)
                                hero.evtDrawHeroById:trigger(v.id)
                                table.insert(list, { id = v.id, debrisCount = debrisCount, heroId = v.id})
                            end
                            break
                        end
                    else
                        local tpl = t.item[v.id]
                        if tpl then
                            hero.addDebris(v.id, v.count)
                            table.insert(list, { id = v.id, debrisCount = v.count, heroId = 0})
                            break
                        end
                    end
                end
            end
            count = count - 1
        end
    end
    local function firstDraw(drawType)
        local size = #firstSingleDraw[drawType]
        local randIndex = utils.getRandomNum(1,size+1)
        -- print('p.CS_HERO_DRAW...size, randIndex, heroId', size, randIndex, utils.serialize(firstSingleDraw[drawType][randIndex]))
        local drawData = firstSingleDraw[drawType][randIndex]
        local id = drawData[1]
        local count = drawData[2]
        if count == 0 then
            local heroId = id
            local debrisCount = 0
            local tpl = t.heros[heroId]
            if tpl then
                if hero.info[heroId] then
                    debrisCount = hero.heroChangeDebris(heroId)
                    hero.addDebris(tpl.heroDebris, debrisCount)
                    table.insert(list, { id = tpl.heroDebris, debrisCount = debrisCount, heroId = heroId})
                else
                    -- hero.info[heroId] = pubHero.newHeroInfo({ id = heroId })
                    hero.addHero(heroId, p.HeroGainType.HERO_DRAW, false)
                    hero.evtDrawHeroById:trigger(heroId)
                    table.insert(list, { id = heroId, debrisCount = debrisCount, heroId = heroId})
                end
            end
        else
            local tpl = t.item[id]
            if tpl then
                hero.addDebris(id, count)
                table.insert(list, { id = id, debrisCount = count, heroId = 0})
            end
        end
    end

    local initConOne, initConTen, initBlue, initOther, initChange, initConOneItemId, initConTenItemId, initFreeTime = init.conOne, init.conTen, init.blue, init.other, init.change, init.conOneItemId, init.conTenItemId, init.free 
    -- print('###p.CS_HERO_DRAW..drawType, isFirst, initConOne, initConTen, initBlue, initOther, initChange', drawType, data.isFirst, initConOne, initConTen, initBlue, initOther, initChange)
    -- 类型 1，普通
    if drawType == p.heroDrawType.GENERAL then
        if not isTen then
           --1 消耗判断
            local leftTime = hero.getHeroLeftFreeTime(drawType, data.expireFreeTime)
            --2.确定卡库
            local tempRate = 0
            local tempOther = 0
            local drawQuality = p.ItemQualityType.BLUE
            local sumRate = data.blueRate + data.otherRate
            local randNum = utils.getRandomNum(1, sumRate)
            if randNum > data.blueRate then
                drawQuality  = data.nextQuality
                tempRate = initBlue + initOther
                tempOther = 0
            else
                tempRate = data.blueRate - initChange
                tempOther = data.otherRate + initChange
            end
            --id
            local drawId = 0
            if not data.isFirst then
                for _, v in pairs(t.heroDraw) do
                    if v.drawType == drawType and v.drawQuality == drawQuality then
                        drawId = v.id
                        break
                    end
                end
                 -- print('###p.heroDrawType.GENERAL ..randNum, data.blueRate, data.otherRate, sumRate, drawQuality, drawId', randNum, data.blueRate, data.otherRate, sumRate, drawQuality, drawId)
                --3.根据权重抽卡
                 if drawId > 0 then
                    draw(drawId, 1)
                else
                    print('pl_hero_draw id is wrong..id, drawType, drawQuality', drawId, drawType, drawQuality)
                    agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
                    return
                end
            else
                firstDraw(drawType)
            end
            --4.消耗
            if not next(list) then
                print('pl_hero_draw.heroPool no data..id, drawType', drawId, drawType)
                agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
                return
            end
            data.blueRate = tempRate
            data.otherRate = tempOther
            -- print("type 1 is ", leftTime)
            -- print("type 1 is ", leftTime)
            -- print("type 1 is ", leftTime)
            -- print("type 1 is ", leftTime)
            if leftTime == 0 then
                data.expireFreeTime = timer.getTimestampCache() + initFreeTime
                --记得加个时器
                hero.resetExpireFreeTime()
            else
                user.removeResource(initConOneItemId, initConOne, p.ResourceConsumeType.HERO_DRAW)
            end
            data.isFirst = false
            data.isSync = false
        else 
            --1.消耗判断
            if not user.isResourceEnough(initConTenItemId, initConTen) then
                -- print('draw ten not enough gold...')
                agent.sendNoticeMessage(p.ErrorCode.PUBLIC_GOLD_NOT_ENOUGH, '', 1)
                return
            end
            --2.确定卡库
            --10连抽：1紫橙+9蓝
            local drawId = 0
            local drawId2 = 0
            for _, v in pairs(t.heroDraw) do
                if drawType == v.drawType and v.isTen then
                    --根据品质  找到牌库ID
                    -- print('###v.drawQuality, data.nextQuality, p.ItemQualityType.BLUE, v.isTen ', v.drawQuality, data.nextQuality,p.ItemQualityType.BLUE, v.isTen)
                    if v.drawQuality == p.ItemQualityType.PURPLE then
                        drawId = v.id
                    end
                    if v.drawQuality == p.ItemQualityType.BLUE then
                        drawId2 = v.id
                    end
                    if drawId > 0 and drawId2 > 0 then
                        break
                    end
                end
            end
            --print('###p.heroDrawType.STAR (ten) ..id1, id2, drawType, nextQuality, curQuality', drawId, drawId2, drawType, nextQuality, data.nextQuality)
            --3.根据权重抽卡
            if drawId > 0 and drawId2 > 0 then
                draw(drawId, 1)
                draw(drawId2, 9)
            else
                data.nextQuality = nextQuality
                print('tpl_hero_draw (ten)id is wrong..id1, id2, drawType', drawId, drawId2, drawType)
                agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
                return
            end
            --4.消耗
            if not next(list) then
                print('tpl_hero_draw.heroPool no data..id1, id2, drawType', drawId, drawId2, drawType)
                agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
                return
            end
            user.removeResource(initConTenItemId, initConTen, p.ResourceConsumeType.HERO_DRAW)
            --基础数据变换
            local sumRate = data.blueRate + data.otherRate
            data.blueRate = sumRate
            data.otherRate = 0
            data.nextQuality = nextQuality
            data.isSync = false
        end 
        
    -- 类型 2，名将
    elseif drawType == p.heroDrawType.STAR then
        if not isTen then
            --1.消耗判断
            local leftTime = hero.getHeroLeftFreeTime(drawType, data.expireFreeTime)
            -- print("user.isResourceEnough(initConOneItemId, initConOne)", user.isResourceEnough(initConOneItemId, initConOne))
            if leftTime > 0 and not user.isResourceEnough(initConOneItemId, initConOne) then
                -- print('draw one not enough gold...')
                agent.sendNoticeMessage(p.ErrorCode.PUBLIC_GOLD_NOT_ENOUGH, '', 1)
                return
            end
            --2.确定卡库
            local tempCount = 0
            local tempRate = 0
            local tempOther = 0
            local sumRate = data.blueRate + data.otherRate
            local drawQuality = p.ItemQualityType.BLUE
            if data.nextQuality < p.ItemQualityType.PURPLE or data.nextQuality > p.ItemQualityType.ORANGE then
                data.nextQuality = p.ItemQualityType.PURPLE
            end
            if data.leftCount - 1 == 0 then
                drawQuality = data.nextQuality
                tempRate = initBlue + initOther
                tempOther = 0
                tempCount = MAX_LEFT_COUNT
            else
                local randNum = utils.getRandomNum(1, sumRate)
                --print('###randNum, data.blueRate, data.otherRate, sumRate', randNum, data.blueRate, data.otherRate, sumRate)
                if randNum > data.blueRate then
                    drawQuality = data.nextQuality
                    tempRate = initBlue + initOther
                    tempOther = 0
                    tempCount = MAX_LEFT_COUNT
                else
                    tempRate = data.blueRate - initChange
                    tempOther = data.otherRate + initChange
                    tempCount = data.leftCount - 1
                end
            end
            --set nextQuality
            local nextQuality = data.nextQuality
            if drawQuality > p.ItemQualityType.BLUE then
                if data.nextQuality == p.ItemQualityType.PURPLE then
                    --nextQuality = p.ItemQualityType.ORANGE
                    nextQuality = p.ItemQualityType.PURPLE
                elseif data.nextQuality == p.ItemQualityType.ORANGE then
                    nextQuality = p.ItemQualityType.PURPLE
                else
                    nextQuality = p.ItemQualityType.PURPLE
                end
            end
            --3.抽卡
            local drawId = 0
            if not data.isFirst then
                for _, v in pairs(t.heroDraw) do
                    if v.drawType == drawType and v.drawQuality == drawQuality and not v.isTen then
                        --根据品质  找到牌库ID
                        drawId = v.id
                        break
                    end
                end
                -- print('###p.heroDrawType.STAR...id, drawType, nextQuality, curQuality, drawQuality', drawId, drawType, nextQuality, data.nextQuality, drawQuality)
                if drawId > 0 then
                    draw(drawId, 1)
                else
                    data.nextQuality = nextQuality
                    -- print('tpl_hero_draw (one)id is wrong..id, drawType', drawId, drawType)
                    agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
                    return
                end
            else
                firstDraw(drawType)
            end
            --4.消耗
            if not next(list) then
                print('draw return empty list..id, drawType', drawId, drawType)
                agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
                return
            end
            --基础数据变换
            data.blueRate = tempRate
            data.otherRate = tempOther
            data.leftCount = tempCount
            data.nextQuality = nextQuality
            -- print('###p.heroDrawType.STAR...blueRate, otherRate, leftCount, nextQuality, leftTime', data.blueRate, data.otherRate, data.leftCount, data.nextQuality, leftTime)
            if leftTime == 0 then
                --记得加个时器
                data.expireFreeTime = timer.getTimestampCache() + initFreeTime
                hero.resetExpireFreeTime()
            else
                user.removeResource(initConOneItemId, initConOne, p.ResourceConsumeType.HERO_DRAW)
            end
            data.isFirst = false
            data.isSync = false
        else
            --1.消耗判断
            if not user.isResourceEnough(initConTenItemId, initConTen) then
                -- print('draw ten not enough gold...')
                agent.sendNoticeMessage(p.ErrorCode.PUBLIC_GOLD_NOT_ENOUGH, '', 1)
                return
            end
            --2.确定卡库
            if data.nextQuality < p.ItemQualityType.PURPLE or data.nextQuality > p.ItemQualityType.ORANGE then
                data.nextQuality = p.ItemQualityType.PURPLE
            end
            local drawQuality = data.nextQuality
            --next
            local nextQuality = data.nextQuality
            if data.nextQuality == p.ItemQualityType.PURPLE then
                --nextQuality = p.ItemQualityType.ORANGE
                nextQuality = p.ItemQualityType.PURPLE
            elseif data.nextQuality == p.ItemQualityType.ORANGE then
                nextQuality = p.ItemQualityType.PURPLE
            else
                nextQuality = p.ItemQualityType.PURPLE
            end
            --10连抽：1紫橙+9蓝
            local drawId = 0
            local drawId2 = 0
            for _, v in pairs(t.heroDraw) do
                if drawType == v.drawType and v.isTen then
                    --根据品质  找到牌库ID
                    -- print('###v.drawQuality, data.nextQuality, p.ItemQualityType.BLUE, v.isTen ', v.drawQuality, data.nextQuality,p.ItemQualityType.BLUE, v.isTen)
                    if v.drawQuality == drawQuality then
                        drawId = v.id
                    end
                    if v.drawQuality == p.ItemQualityType.BLUE then
                        drawId2 = v.id
                    end
                    if drawId > 0 and drawId2 > 0 then
                        break
                    end
                end
            end
            --print('###p.heroDrawType.STAR (ten) ..id1, id2, drawType, nextQuality, curQuality', drawId, drawId2, drawType, nextQuality, data.nextQuality)
            --3.根据权重抽卡
            if drawId > 0 and drawId2 > 0 then
                draw(drawId, 1)
                draw(drawId2, 9)
            else
                data.nextQuality = nextQuality
                print('tpl_hero_draw (ten)id is wrong..id1, id2, drawType', drawId, drawId2, drawType)
                agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
                return
            end
            --4.消耗
            if not next(list) then
                print('tpl_hero_draw.heroPool no data..id1, id2, drawType', drawId, drawId2, drawType)
                agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
                return
            end
            user.removeResource(initConTenItemId, initConTen, p.ResourceConsumeType.HERO_DRAW)
            --基础数据变换
            local sumRate = data.blueRate + data.otherRate
            data.blueRate = sumRate
            data.otherRate = 0
            data.leftCount = MAX_LEFT_COUNT
            data.nextQuality = nextQuality
            data.isSync = false
        end
    -- 类型 3，热门
    elseif drawType == p.heroDrawType.HOT then
        local consumeGold = initConOne
        local consumeItemId = initConOneItemId
        local drawCount = 1
        if isTen then
            consumeGold = initConTen
            consumeItemId = initConTenItemId
            drawCount = 10
        end
        --1.消耗判断
        if not user.isResourceEnough(consumeItemId, consumeGold) then
            -- print('draw one not enough gold...')
            agent.sendNoticeMessage(p.ErrorCode.PUBLIC_GOLD_NOT_ENOUGH, '', 1)
            return
        end
        --2.判断事件是否开放
        local leftTimeExpired = hero.achievementLeftTime(achievementId)
        if leftTimeExpired <= 0 then
            print('event not open...leftTimeExpired', leftTimeExpired)
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            return
        end

    --addBegin
        if not isTen then
            --2.确定卡库
            local tempCount = 0
            local tempRate = 0
            local tempOther = 0
            local sumRate = data.blueRate + data.otherRate
            local drawQuality = p.ItemQualityType.BLUE
            if data.nextQuality < p.ItemQualityType.PURPLE or data.nextQuality > p.ItemQualityType.ORANGE then
                data.nextQuality = p.ItemQualityType.PURPLE
            end
            if data.leftCount - 1 <= 0 then
                drawQuality = data.nextQuality
                tempRate = initBlue + initOther
                tempOther = 0
                tempCount = MAX_LEFT_COUNT
            else
                tempCount = data.leftCount - 1
            end

            --set nextQuality
            local nextQuality = data.nextQuality
            if drawQuality > p.ItemQualityType.BLUE then
                if data.nextQuality == p.ItemQualityType.PURPLE then
                    --nextQuality = p.ItemQualityType.ORANGE
                    nextQuality = p.ItemQualityType.PURPLE
                elseif data.nextQuality == p.ItemQualityType.ORANGE then
                    nextQuality = p.ItemQualityType.PURPLE
                else
                    nextQuality = p.ItemQualityType.PURPLE
                end
            end

            --3.抽卡
            local drawId = 0
            for _, v in pairs(t.heroDraw) do
                if v.drawType == drawType and achievementId == v.achievementId and v.drawQuality == drawQuality and not v.isTen then
                    --根据品质  找到牌库ID
                    drawId = v.id
                    break
                end
            end
            --print('###p.heroDrawType.HOT...id, drawType, nextQuality, curQuality, drawQuality', drawId, drawType, nextQuality, data.nextQuality, drawQuality)
            if drawId > 0 then
                draw(drawId, 1)
            else
                data.nextQuality = nextQuality
                print('tpl_hero_draw (one)id is wrong..id, drawType', drawId, drawType)
                agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
                return
            end

            if not next(list) then
                print('draw return empty list..id, drawType', drawId, drawType)
                agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
                return
            end
            --基础数据变换
            data.blueRate = tempRate
            data.otherRate = tempOther
            data.leftCount = tempCount
            data.nextQuality = nextQuality
            --print('###p.heroDrawType.HOT...blueRate, otherRate, leftCount, nextQuality, leftTime', data.blueRate, data.otherRate, data.leftCount, data.nextQuality, leftTime)
            data.isFirst = false
            data.isSync = false
        else
            --2.确定卡库
            if data.nextQuality < p.ItemQualityType.PURPLE or data.nextQuality > p.ItemQualityType.ORANGE then
                data.nextQuality = p.ItemQualityType.PURPLE
            end
            local drawQuality = data.nextQuality
            --next
            local nextQuality = data.nextQuality
            if data.nextQuality == p.ItemQualityType.PURPLE then
                --nextQuality = p.ItemQualityType.ORANGE
                nextQuality = p.ItemQualityType.PURPLE
            elseif data.nextQuality == p.ItemQualityType.ORANGE then
                nextQuality = p.ItemQualityType.PURPLE
            else
                nextQuality = p.ItemQualityType.PURPLE
            end
            --10连抽：1紫橙+9蓝
            local drawId = 0
            local drawId2 = 0
            for _, v in pairs(t.heroDraw) do
                if drawType == v.drawType and achievementId == v.achievementId and v.isTen then
                    --根据品质  找到牌库ID
                    -- print('###v.drawQuality, data.nextQuality, p.ItemQualityType.BLUE, v.isTen ', v.drawQuality, data.nextQuality,p.ItemQualityType.BLUE, v.isTen)
                    if v.drawQuality == drawQuality then
                        drawId = v.id
                    end
                    if v.drawQuality == p.ItemQualityType.BLUE then
                        drawId2 = v.id
                    end
                    if drawId > 0 and drawId2 > 0 then
                        break
                    end
                end
            end
            --print('###p.heroDrawType.HOT (ten) ..id1, id2, drawType, nextQuality, curQuality', drawId, drawId2, drawType, nextQuality, data.nextQuality)
            --3.根据权重抽卡
            if drawId > 0 and drawId2 > 0 then
                draw(drawId, 1)
                draw(drawId2, 9)
            else
                data.nextQuality = nextQuality
                print('tpl_hero_draw (ten)id is wrong..id1, id2, drawType', drawId, drawId2, drawType)
                agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
                return
            end
            
            if not next(list) then
                print('tpl_hero_draw.heroPool no data..id1, id2, drawType', drawId, drawId2, drawType)
                agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
                return
            end
            --基础数据变换
            local sumRate = data.blueRate + data.otherRate
            data.blueRate = sumRate
            data.otherRate = 0
            data.leftCount = MAX_LEFT_COUNT
            data.nextQuality = nextQuality
            data.isSync = false
        end
        user.removeResource(consumeItemId, consumeGold, p.ResourceConsumeType.HERO_DRAW)
        -- 当抽取类型大于3的时候统一走这个抽取逻辑
    elseif drawType > p.heroDrawType.HOT then
        -- print("*************************herodraw begin**********************")
        -- local sysUnlockId = t.heroDrawType[drawType].sysUnlockId
        -- if not agent.systemUnlock.checkUnlockedById(sysUnlockId) then
        --     print('###p.CS_HERO_DRAW, the type is not unlock', drawType, sysUnlockId)
        --     agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        --     return
        -- end
        if not isTen then
            --1.消耗判断
            local leftTime = hero.getHeroLeftFreeTime(drawType, data.expireFreeTime)
            if leftTime > 0 and not user.isResourceEnough(initConOneItemId, initConOne) then
                -- print('draw one not enough gold...')
                agent.sendNoticeMessage(p.ErrorCode.PUBLIC_GOLD_NOT_ENOUGH, '', 1)
                return
            end
            --2.确定卡库
            local tempCount = 0
            local tempRate = 0
            local tempOther = 0
            local sumRate = data.blueRate + data.otherRate
            local drawQuality = p.ItemQualityType.BLUE
            if data.nextQuality < p.ItemQualityType.PURPLE or data.nextQuality > p.ItemQualityType.ORANGE then
                data.nextQuality = p.ItemQualityType.PURPLE
            end
            local randNum = utils.getRandomNum(1, sumRate)
            --print('###randNum, data.blueRate, data.otherRate, sumRate', randNum, data.blueRate, data.otherRate, sumRate)
            if randNum > data.blueRate then
                drawQuality = data.nextQuality
                tempRate = initBlue + initOther
                tempOther = 0
            else
                tempRate = data.blueRate - initChange
                tempOther = data.otherRate + initChange
            end
            --set nextQuality
            local nextQuality = data.nextQuality
            if drawQuality > p.ItemQualityType.BLUE then
                if data.nextQuality == p.ItemQualityType.PURPLE then
                    --nextQuality = p.ItemQualityType.ORANGE
                    nextQuality = p.ItemQualityType.PURPLE
                elseif data.nextQuality == p.ItemQualityType.ORANGE then
                    nextQuality = p.ItemQualityType.PURPLE
                else
                    nextQuality = p.ItemQualityType.PURPLE
                end
            end
            --3.抽卡
            local drawId = 0
            if not data.isFirst then
                for _, v in pairs(t.heroDraw) do
                    if v.drawType == drawType and v.drawQuality == drawQuality and not v.isTen then
                        --根据品质  找到牌库ID
                        drawId = v.id
                        break
                    end
                end
                --print('###p.heroDrawType.STAR...id, drawType, nextQuality, curQuality, drawQuality', drawId, drawType, nextQuality, data.nextQuality, drawQuality)
                if drawId > 0 then
                    draw(drawId, 1)
                else
                    data.nextQuality = nextQuality
                    print('tpl_hero_draw (one)id is wrong..id, drawType', drawId, drawType)
                    agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
                    return
                end
            else
                firstDraw(drawType)
            end
            --4.消耗
            if not next(list) then
                print('draw return empty list..id, drawType', drawId, drawType)
                agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
                return
            end
            --基础数据变换
            data.blueRate = tempRate
            data.otherRate = tempOther
            data.nextQuality = nextQuality
            -- print('###p.heroDrawType.STAR...blueRate, otherRate, leftCount, nextQuality, leftTime', data.blueRate, data.otherRate, data.leftCount, data.nextQuality, leftTime)
            -- if leftTime == 0 then
            --     --记得加个时器
            --     data.expireFreeTime = timer.getTimestampCache() + STAR_SENCONDS
            --     hero.resetExpireFreeTime()
            -- else
            user.removeResource(initConOneItemId, initConOne, p.ResourceConsumeType.HERO_DRAW)
            -- end
            data.isFirst = false
            data.isSync = false
        else
            --1.消耗判断
            if not user.isResourceEnough(initConTenItemId, initConTen) then
                -- print('draw ten not enough gold...')
                agent.sendNoticeMessage(p.ErrorCode.PUBLIC_GOLD_NOT_ENOUGH, '', 1)
                return
            end
            --2.确定卡库
            if data.nextQuality < p.ItemQualityType.PURPLE or data.nextQuality > p.ItemQualityType.ORANGE then
                data.nextQuality = p.ItemQualityType.PURPLE
            end
            local drawQuality = data.nextQuality
            --next
            local nextQuality = data.nextQuality
            if data.nextQuality == p.ItemQualityType.PURPLE then
                --nextQuality = p.ItemQualityType.ORANGE
                nextQuality = p.ItemQualityType.PURPLE
            elseif data.nextQuality == p.ItemQualityType.ORANGE then
                nextQuality = p.ItemQualityType.PURPLE
            else
                nextQuality = p.ItemQualityType.PURPLE
            end
            --10连抽：1紫橙+9蓝
            local drawId = 0
            local drawId2 = 0
            for _, v in pairs(t.heroDraw) do
                if drawType == v.drawType and v.isTen then
                    --根据品质  找到牌库ID
                    -- print('###v.drawQuality, data.nextQuality, p.ItemQualityType.BLUE, v.isTen ', v.drawQuality, data.nextQuality,p.ItemQualityType.BLUE, v.isTen)
                    if v.drawQuality == drawQuality then
                        drawId = v.id
                    end
                    if v.drawQuality == p.ItemQualityType.BLUE then
                        drawId2 = v.id
                    end
                    if drawId > 0 and drawId2 > 0 then
                        break
                    end
                end
            end
            -- print('###p.heroDrawType.STAR (ten) ..id1, id2, drawType, nextQuality, curQuality', drawId, drawId2, drawType, nextQuality, data.nextQuality)
            --3.根据权重抽卡
            if drawId > 0 and drawId2 > 0 then
                draw(drawId, 1)
                draw(drawId2, 9)
            else
                data.nextQuality = nextQuality
                print('tpl_hero_draw (ten)id is wrong..id1, id2, drawType', drawId, drawId2, drawType)
                agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
                return
            end
            --4.消耗
            if not next(list) then
                print('tpl_hero_draw.heroPool no data..id1, id2, drawType', drawId, drawId2, drawType)
                agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
                return
            end
            user.removeResource(initConTenItemId, initConTen, p.ResourceConsumeType.HERO_DRAW)
            --基础数据变换
            local sumRate = data.blueRate + data.otherRate
            data.blueRate = sumRate
            data.otherRate = 0
            data.nextQuality = nextQuality
            data.isSync = false
        end
        -- print("*************************herodraw begin**********************")

    --addEnd

        -- --3.抽卡
        -- local drawId = 0
        -- for _, v in pairs(t.heroDraw) do
        --     if v.drawType == drawType and achievementId == v.achievementId and v.isTen == isTen then
        --         --根据achievementId  找到牌库ID
        --         --print('6666666666666666666666')
        --         drawId = v.id
        --         break
        --     end
        -- end
        -- print('###p.heroDrawType.STAR...drawType, achievementId, drawId', drawType, achievementId, drawId)
        -- if drawId > 0 then
        --     draw(drawId, drawCount)
        -- else
        --     print('tpl_hero_draw (one)id is wrong..drawType, achievementId, drawId', drawType, achievementId, drawId)
        --     agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        --     return
        -- end
        -- if not next(list) then
        --     print('draw return empty list..drawType, achievementId, drawId', drawType, achievementId, drawId)
        --     agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        --     return
        -- end

        --4.消耗
    end
    -- print('###p.CS_HERO_DRAW222..data', utils.serialize(data))

    hero.evtDrawHero:trigger(#list)

    local drawNum = 1
    if isTen then
        drawNum = 10
    end
    hero.evtDrawHeroByType:trigger(drawType, drawNum)

    

    hero.sendHeroDrawDataUpdate()
    hero.sendHeroUpdate()
    hero.sendHeroSkillUpdate()
    heroDrawResponse(true, list)
end

function hero.cs_hero_star_upgrade(heroId, bid, session)
    local function starUpgradeResponse(result)
        -- print("starUpgradeResponse..", result)
        agent.replyPktout(session, p.SC_HERO_STAR_UPGRADE_RESPONSE, result)
    end

    print('p.CS_HERO_STAR_UPGRADE...heroId, bid', heroId, bid)

    --1.检查背包
    local info = bag.bag_items[bid]
    if not info or info.count <= 0 then
        print('###p.CS_HERO_STAR_UPGRADE..bid not exist, bid=', bid)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --2.检查武将列表
    local heroInfo = hero.info[heroId]
    if not heroInfo then
        print('###p.CS_HERO_STAR_UPGRADE..hero not exist, heroId=', heroId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --3.检查是否满星
    local star = heroInfo.star
    local level = heroInfo.level
    local quaTpl = t.heroStarlevel[heroId][star + 1]
    if quaTpl == nil then
        print('###p.CS_HERO_STAR_UPGRADE..tpl_hero_star_level not exist, heroId, quality,level=', heroId, quality,level)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --3.1 取出星级最大值
    local maxLevel = #t.heroStarlevel[heroId]
    if star >= maxLevel then
        print('###p.CS_HERO_STAR_UPGRADE..hero star is full, heroId, star=', heroId, star)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --4.检查升星的武将碎片是否匹配
    local heroTpl = t.heros[heroId]
    if not heroTpl or heroTpl.heroDebris ~= info.tpl.id then
        print('###p.CS_HERO_STAR_UPGRADE..tpl_hero not exist or upgrade star material dismatch, heroDebrisId, itemId=', heroTpl.heroDebris, info.tpl.id)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --5.检查升星材料是否足够：碎片 银两
    local needDebris = quaTpl.debris
    --local starTpl = quaTpl[star + 1]
    if needDebris == nil then
        print('###p.CS_HERO_STAR_UPGRADE..tpl_hero_star_level not exist, heroId, quality, star=', heroId, quality, star)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    print('-------------p.CS_HERO_STAR_UPGRADE, needDebris, myDebris', needDebris, info.count)
    if needDebris > info.count then
        print('###p.CS_HERO_STAR_UPGRADE..debris is not enough...needCount, myCount', needDebris, info.count)
        agent.sendNoticeMessage(p.ErrorCode.HERO_DEBRIS_NOT_ENOUGH, '', 1)
        return
    end
    --6消耗
    bag.removeItemByBid(bid, info.tpl.id, needDebris, p.ResourceConsumeType.HERO_STAR_UPGRADE, true)

    heroInfo.star = heroInfo.star + 1
    heroInfo.isDirty = true
    heroInfo.isSync = false

    -- 检查技能槽位
    heroInfo:checkSkillSlot()

    hero.evtHeroStarUp:trigger(heroId, heroInfo.star)
    hero.evtChangeHeroBaseProperty:trigger(heroId, p.ArenaDataType.HERO_LEVEL_STAR)

    --log
    logStub.appendHeroUpgrade(user.uid, heroId, heroInfo.level, heroInfo.level, heroInfo.star - 1, heroInfo.star, timer.getTimestampCache())

    hero.sendHeroUpdate()
    starUpgradeResponse(p.ErrorCode.SUCCESS)

end

function hero.cs_hero_skill_upgrade(heroId, skillId)
    local function skillUpgradeResponse(result)
        -- print("skillUpgradeResponse..", result)
        agent.replyPktout(session, p.SC_HERO_SKILL_UPGRADE_RESPONSE, result)
    end

    local heroInfo = hero.info[heroId]
    if not heroInfo then
        print('###p.CS_HERO_SKILL_UPGRADE..hero not exist, heroId=', heroId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --3检查技能是否存在
    local skillInfo = heroInfo.skill[skillId]
    -- 只有天赋技才能升级
    if not skillInfo or skillInfo.type ~= p.HeroSkillType.HERO_SKILL_TALENT then
        print('###p.CS_HERO_SKILL_UPGRADE..skill is not exist, heroId, skillId=', heroId, skillId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --5检查技能是否升满
    local skillTpl = t.skill[skillInfo.tplId]
    if not skillTpl then
        print("###p.CS_HERO_SKILL_UPGRADE..skill tpl is not exist, heroId = ", skillInfo.tplId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    if skillInfo.level >= skillTpl.nLevelCap then
        print("###p.CS_HERO_SKILL_UPGRADE..skill level is full, skill = ",skillInfo.tplId, skillInfo.level)
        agent.sendNoticeMessage(p.ErrorCode.SKILL_IS_MAX_LEVEL, '', 1)
        return
    end
    local skillLevelInfo = t.heroTalent[heroInfo.tpl.sQuality][skillInfo.level]
    -- 检擦天赋技能书是否足够
    if not skillLevelInfo then
        print("###p.CS_HERO_SKILL_UPGRADE... skillLevelInfo is nil..", heroInfo.tpl.sQuality, skillInfo.level)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    if not bag.checkItemEnough(skillLevelInfo.consumeItem, skillLevelInfo.consume) then
        print("###p.CS_HERO_SKILL_UPGRADE... tpl is not enough.", skillLevelInfo.consumeItem, skillLevelInfo.consume)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    if not bag.removeItem(skillLevelInfo.consumeItem, skillLevelInfo.consume,p.ResourceConsumeType.UPGRADE_HERO_SKILL, true) then
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    skillInfo.level = skillInfo.level + 1
    skillInfo.isSync = false
    heroInfo.isDirty = true

    hero.evtSkillUpgrade:trigger(skillInfo.level)
    hero.evtChangeHeroBaseProperty:trigger(heroId, p.ArenaDataType.HERO_SKILL_LEVEL, skillId)

    hero.sendHeroSkillUpdate()
    skillUpgradeResponse(p.ErrorCode.SUCCESS)
end

function hero.cs_hero_debris_turn(turnType, heroDebrisList, session)
    local function heroDebrisTurnResponse(result)
        print("heroDebrisTurnResponse..", result)
        agent.replyPktout(session, p.SC_HERO_DEBRIS_TURN_RESPONSE, result)
    end
    local result = p.ErrorCode.SUCCESS
    -- 1.检查配置表
    local fragments = t.configure['fragments']
    if not fragments then
        print("tpl is nil")
        agent.sendNoticeMessage(p.ErrorCode.UNKNOWN, '', 1)
        return
    end
    -- 2.检查碎片是否足够
    if not bag.checkItemsEnough(heroDebrisList) then
        print("PUBLIC_PROP_NOT_ENOUGH")
        agent.sendNoticeMessage(p.ErrorCode.PUBLIC_PROP_NOT_ENOUGH, '', 1)
        return
    end
    print("###p.CS_HERO_DEBRIS_TURN, heroDebrisList", utils.serialize(heroDebrisList))
    local totalSpend, totalPointCount = 0, 0
    -- 3.计算货币总花费和技能点总获得
    for debrisId, count in pairs(heroDebrisList) do
        local tpl = t.item[debrisId]
        print("debrisId, count, tpl.subType", debrisId, count, tpl.subType)
        if tpl ~= nil then
            if tpl.subType == p.ItemPropType.HERO_DEBRIS then
                if tpl.quality == p.ItemQualityType.WHITE then
                    if turnType == p.ResourceType.GOLD then
                        totalSpend  = totalSpend + fragments.w.specialSpend * count
                        totalPointCount = totalPointCount + fragments.w.special * count
                    elseif turnType == p.ResourceType.SILVER then
                        totalSpend  = totalSpend + fragments.w.commonspend * count
                        totalPointCount = totalPointCount + fragments.w.common * count
                    end
                elseif tpl.quality == p.ItemQualityType.GREEN then
                    if turnType == p.ResourceType.GOLD then
                        totalSpend  = totalSpend + fragments.g.specialSpend * count
                        totalPointCount = totalPointCount + fragments.g.special * count
                    elseif turnType == p.ResourceType.SILVER then
                        totalSpend  = totalSpend + fragments.g.commonspend * count
                        totalPointCount = totalPointCount + fragments.g.common * count
                    end
                elseif tpl.quality == p.ItemQualityType.BLUE then
                    if turnType == p.ResourceType.GOLD then
                        totalSpend  = totalSpend + fragments.b.specialSpend * count
                        totalPointCount = totalPointCount + fragments.b.special * count
                    elseif turnType == p.ResourceType.SILVER then
                        totalSpend  = totalSpend + fragments.b.commonspend * count
                        totalPointCount = totalPointCount + fragments.b.common * count
                    end
                elseif tpl.quality == p.ItemQualityType.PURPLE then
                    if turnType == p.ResourceType.GOLD then
                        totalSpend  = totalSpend + fragments.p.specialSpend * count
                        totalPointCount = totalPointCount + fragments.p.special * count
                    elseif turnType == p.ResourceType.SILVER then
                        totalSpend  = totalSpend + fragments.p.commonspend * count
                        totalPointCount = totalPointCount + fragments.p.common * count
                    end
                elseif tpl.quality == p.ItemQualityType.ORANGE then
                    if turnType == p.ResourceType.GOLD then
                        totalSpend  = totalSpend + fragments.o.specialSpend * count
                        totalPointCount = totalPointCount + fragments.o.special * count
                    elseif turnType == p.ResourceType.SILVER then
                        totalSpend  = totalSpend + fragments.o.commonspend * count
                        totalPointCount = totalPointCount + fragments.o.common * count
                    end
                end
            end
        else 
            print("heroDebris is error")
            agent.sendNoticeMessage(p.ErrorCode.UNKNOWN, '', 1)
            return
        end
    end
    -- 4.检测货币是否足够
    if not user.isResourceEnough(turnType, totalSpend) then 
        if turnType == p.ResourceType.GOLD then
            result = p.ErrorCode.PUBLIC_GOLD_NOT_ENOUGH
        elseif turnType == p.ResourceType.SILVER then
            result = p.ErrorCode.PUBLIC_SILVER_NOT_ENOUGH
        end
        agent.sendNoticeMessage(result, '', 1)
        return 
    end
    -- 5.减碎片
    bag.removeItems(heroDebrisList, p.ResourceConsumeType.HERO_DEBRIS_TURN, true)
    -- 6.交钱
    user.removeResource(turnType, totalSpend, p.ResourceConsumeType.HERO_DEBRIS_TURN, 0, true)
    -- 7.给货
    user.addSkillPoint(totalPointCount, p.SkillPointGainType.HERO_DEBRIS_TURN, true)
    heroDebrisTurnResponse(result)
end

function hero.cs_hero_change_technical_skill(heroId, slot, skillId, session)
    local function heroChangeTechnicalSkillResponse(result)
        print("heroChangeTechnicalSkillResponse..", result)
        agent.replyPktout(session, p.SC_HERO_CHANGE_TECHNICAL_SKILL_RESPONSE, result)
    end
    print("###CS_HERO_CHANGE_TECHNICAL_SKILL, heroId, slot, skillId, ",heroId, slot, skillId)
    local heroInfo = hero.info[heroId]
    if not heroInfo then
        print("###CS_HERO_CHANGE_TECHNICAL_SKILL, hero is not exist", heroId)
        agent.sendNoticeMessage(p.ErrorCode.HERO_NOT_EXIST, '', 1)
        return
    end
    if slot > heroInfo.slotNum then
        print("###CS_HERO_CHANGE_TECHNICAL_SKILL, slot, heroInfo.slotNum",slot, heroInfo.slotNum)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    if not heroInfo.skill[skillId] or heroInfo.skill[skillId].type ~= p.HeroSkillType.HERO_SKILL_STAR then
        print("###CS_HERO_CHANGE_TECHNICAL_SKILL, skill is not technical")
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local itemCount = 0
    local goldCount = 0
    local leftCount = bag.getItemCountById(heroInfo.tpl.starSkillconsume[1])
    local itemTpl = t.item[heroInfo.tpl.starSkillconsume[1]]
    if leftCount >= heroInfo.tpl.starSkillconsume[2] then
        itemCount = heroInfo.tpl.starSkillconsume[2]
        goldCount = 0
    else
        itemCount = leftCount
        goldCount = heroInfo.tpl.starSkillconsume[2] - leftCount
    end
    local gold = goldCount * itemTpl.price
    -- 资源是否足够
    print("###p.CS_HERO_CHANGE_TECHNICAL_SKILL, itemCount, goldCount...", itemCount, goldCount)
    if not user.isResourceEnough(p.SpecialPropIdType.GOLD, gold) then
        print("###CS_HERO_CHANGE_TECHNICAL_SKILL, item is not enough",itemCount, gold)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    if itemCount > 0 then
        bag.removeItem(heroInfo.tpl.starSkillconsume[1], itemCount, p.ResourceConsumeType.HERO_CHANGE_SKILL, true)
    end
    if gold > 0 then
        user.removeResource(p.SpecialPropIdType.GOLD, gold, p.ResourceConsumeType.HERO_CHANGE_SKIL)
    end
    -- 先把之前的技能槽上技能的技能槽位置设置成0
    for k, info in pairs(heroInfo.skill) do
        if info.type == p.HeroSkillType.HERO_SKILL_STAR and info.slot == slot then
             info.slot = 0
             info.isSync = false
             -- print("info.slot",info.slot)
        end  
    end
    -- 再把现在的技能标志位设置成slot
    heroInfo.skill[skillId].slot = slot
    heroInfo.skill[skillId].isSync = false
    hero.sendHeroSkillUpdate()
    heroChangeTechnicalSkillResponse(p.ErrorCode.SUCCESS)
end


function hero.cs_hero_quick_recover(heroId, tplId, count, session)
    local function heroQuickRecover(result)
        agent.replyPktout(session, p.SC_HERO_QUICK_RECOVER_RESPONSE, result)
    end
    print("p.CS_HERO_QUICK_RECOVER", heroId, tplId, count)
    -- 1.检测武将是否存在
    local heroInfo = hero.info[heroId]
    if not heroInfo then
        print("p.CS_HERO_QUICK_RECOVER, hero is not exist", heroId)
        agent.sendNoticeMessage(p.ErrorCode.HERO_NOT_EXIST, '', 1)
        return
    end
    -- 2.检测物品
    local tpl = t.item[tplId]
    if not tpl then
        print("p.CS_HERO_QUICK_RECOVER, tpl not found, tplId = ", tplId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    -- 3.检测物品是否足够
    if not bag.removeItem(tplId, count, p.ResourceConsumeType.ITEM_USE, true ) then
        print("p.CS_HERO_QUICK_RECOVER, remove item fail, bid, tplId, count = ", tplId, 1)
        agent.sendNoticeMessage(p.ErrorCode.PUBLIC_PROP_NOT_ENOUGH, '', 1)
        return
    end
    -- 4.恢复体力
    for i=1, count do
        local addPhysical = (tpl.param1 / 10000) * heroInfo.heroSolohp
        if addPhysical + heroInfo.physical > heroInfo.heroSolohp then
            heroInfo.physical = heroInfo.heroSolohp
        else
            heroInfo.physical = heroInfo.physical + addPhysical
        end
        heroInfo.isSync = false
        heroInfo.isDirty = true
    end
    hero.sendHeroUpdate()
    heroQuickRecover(p.ErrorCode.SUCCESS)
end

function hero.cs_hero_soul_upgrade(heroId, soulId, session)
    local function heroSoulUpgrade(result)
        agent.replyPktout(session, p.SC_HERO_SOUL_UPGRADE_RESPONSE, result)
    end
    local heroInfo = hero.info[heroId]
    if not heroInfo then
        print("p.CS_HERO_SOUL_UPGRADE, hero is not exist", heroId)
        agent.sendNoticeMessage(p.ErrorCode.HERO_NOT_EXIST, '', 1)
        return
    end
    local isLevelUp = false 
    local skillId = nil
    -- 命魂属性升级
    if soulId ~= 0 then
        local soulTpl = t.heroSoul[soulId]
        if not soulTpl then
            print("p.CS_HERO_SOUL_UPGRADE, soul is not exist", soulId)
            agent.sendNoticeMessage(p.ErrorCode.UNKNOWN, '', 1)
            return
        end
        -- 消耗
        if not bag.checkItemEnough(soulTpl.riseConsumeT, soulTpl.riseConsume) then
            print("p.CS_HERO_SOUL_UPGRADE, riseConsume is not enough, soulTpl.riseConsumeT, soulTpl.riseConsume, soulId", soulTpl.riseConsumeT, soulTpl.riseConsume, soulId)
            agent.sendNoticeMessage(p.ErrorCode.HERO_SOUL_LEVEL_IS_FULL, '', 1)
            return
        end
        if soulTpl.nDamageType ~= heroInfo.tpl.nDamageType then
            print("p.CS_HERO_SOUL_UPGRADE, soul nDamageType is not equal hero nDamageType", soulId)
            agent.sendNoticeMessage(p.ErrorCode.UNKNOWN, '', 1)
            return
        end
        if soulTpl.soulLevel ~= heroInfo.soulLevel then
            print("p.CS_HERO_SOUL_UPGRADE, soulLevel is not equal heroSoulLevel, soulLevel, mylevel", soulTpl.soulLevel, heroInfo.soulLevel)
            agent.sendNoticeMessage(p.ErrorCode.UNKNOWN, '', 1)
            return
        end
        local soulInfo = heroInfo.soul[soulId]
        if not soulInfo then
            print("p.CS_HERO_SOUL_UPGRADE, soulInfo is nil", soulId)
            agent.sendNoticeMessage(p.ErrorCode.UNKNOWN, '', 1)
            return
        end
        if soulInfo.level + 1 > soulTpl.riseTimes then
            print("p.CS_HERO_SOUL_UPGRADE, soulLevel is full", soulId)
            agent.sendNoticeMessage(p.ErrorCode.HERO_SOUL_LEVEL_IS_FULL, '', 1)
            return
        end
        if not bag.removeItem(soulTpl.riseConsumeT, soulTpl.riseConsume, p.ResourceConsumeType.HERO_UPGRADE_SOUL_LEVEL, true) then
            print("p.CS_HERO_SOUL_UPGRADE, consume item is fail", soulId)
            agent.sendNoticeMessage(p.ErrorCode.UNKNOWN, '', 1)
            return
        end
        soulInfo.level = soulInfo.level + 1
        soulInfo.isSync = false
    -- 命魂升阶
    else
        --1.当前的阶级是否是满级
        if heroInfo.soulLevel >= heroInfo.tpl.maxSoul then
            print("p.CS_HERO_SOUL_UPGRADE, soul is full", soulId)
            agent.sendNoticeMessage(p.ErrorCode.HERO_SOUL_IS_FULL, '', 1)
            return
        end
        --2.判断当前的阶级属性是否都升满了
        local rankConsumeT1 = 0
        local rankConsume1 = 0
        local rankConsumeT2 = 0
        local rankConsume2 = 0
        local soulSkill = 0
        for _, tpl in pairs(t.heroSoulLib[heroInfo.tpl.nDamageType][heroInfo.soulLevel]) do
            local soulInfo = heroInfo.soul[tpl.id]
            if not soulInfo then
                print("p.CS_HERO_SOUL_UPGRADE, soul is nil", tpl.id)
                agent.sendNoticeMessage(p.ErrorCode.UNKNOWN, '', 1)
                return
            end
            if soulInfo.level < tpl.riseTimes then
                print("p.CS_HERO_SOUL_UPGRADE, soul level is not full", tpl.id)
                agent.sendNoticeMessage(p.ErrorCode.HERO_SOUL_LEVEL_IS_NOT_FULL, '', 1)
                return
            end
            rankConsumeT1 = tpl.rankConsumeT1
            rankConsume1 = tpl.rankConsume1
            rankConsumeT2 = tpl.rankConsumeT2
            rankConsume2 = tpl.rankConsume2
            soulSkill = tpl.soulSkill
        end
        --3.检测资源是否足够
        if not bag.checkItemEnough(rankConsumeT1, rankConsume1) or not bag.checkItemEnough(rankConsumeT2, rankConsume2) then
            print("p.CS_HERO_SOUL_UPGRADE, soul level material is not enough")
            agent.sendNoticeMessage(p.ErrorCode.HERO_SOUL_LEVEL_IS_FULL, '', 1)
            return
        end
        local list = {}
        list[rankConsumeT1] = rankConsume1
        list[rankConsumeT2] = rankConsume2
        --4.消耗资源
        bag.removeItems(list, p.ResourceConsumeType.HERO_UPGRADE_SOUL, true)
        --3.升阶
        heroInfo.soulLevel = heroInfo.soulLevel + 1
        isLevelUp = true
        skillId = soulSkill 
        --4.加命魂技能
        if not heroInfo.skill[soulSkill] then
            heroInfo.skill[soulSkill] = pubHero.skillInfo:new({tplId = soulSkill, type = p.HeroSkillType.HERO_SKILL_PASSIVE, level = 1, slot = 0})
        end
        --4.填充升阶之后的属性项
        if t.heroSoulLib[heroInfo.tpl.nDamageType][heroInfo.soulLevel] then
            for _, tpl in pairs(t.heroSoulLib[heroInfo.tpl.nDamageType][heroInfo.soulLevel]) do
                heroInfo.soul[tpl.id] = pubHero.soulInfo:new({tplId = tpl.id, soulLevel = tpl.soulLevel, level = 0, isSync = false})
            end
        end
    end
    heroInfo.isSync = false
    heroInfo.isDirty = true

    hero.evtChangeHeroBaseProperty:trigger(heroId, p.ArenaDataType.HERO_SKILL_ON,skillId)
    hero.sendHeroSoulUpdate()
    if isLevelUp then
        hero.sendHeroSkillUpdate()
    end
    heroSoulUpgrade(0)
end

function hero.cs_hero_upgrade(heroId, bid, session)
    local function heroUpgradeResponse(result)
        agent.replyPktout(session, p.SC_HERO_UPGRADE_RESPONSE, result)
    end
    print("###p.CS_HERO_UPGRADE...heroId, tplId, count", heroId, bid)
    local heroInfo = hero.info[heroId]
    if not heroInfo then
        print("p.CS_HERO_UPGRADE, hero is not exist", heroId)
        agent.sendNoticeMessage(p.ErrorCode.HERO_NOT_EXIST, '', 1)
        return
    end
    if not hero.canAddHeroExp(heroId) then
        print("p.CS_HERO_UPGRADE, hero is can not add exp", heroId)
        agent.sendNoticeMessage(p.ErrorCode.HERO_FULL_LEVEL, '', 1)
        return
    end
    if bid ~= 0 then
        local item = bag.getItemBybid(bid)
        if not item then

        end
        if not bag.checkItemEnough(item.itemId, 1) then
            print("p.CS_HERO_UPGRADE, item is not enough", tplId)
            agent.sendNoticeMessage(p.ErrorCode.HERO_UPGRADE_MATERIAL_IS_NOT_ENOUGH, '', 1)
            return
        end
        if not bag.removeItem(item.itemId, 1,p.ResourceConsumeType.HERO_UPGRADE_LEVEL, true) then
            print("p.checkItemEnough, consume item is fail", tplId)
            agent.sendNoticeMessage(p.ErrorCode.UNKNOWN, '', 1)
            return
        end
        local itemTpl = t.item[item.itemId]
        if itemTpl.subType ~= p.ItemPropType.HERO_EXP then
            print("p.CS_HERO_UPGRADE, tpl is not hero exp...", tplId)
            agent.sendNoticeMessage(p.ErrorCode.UNKNOWN, '', 1)
            return
        end
        hero.addHeroExp(heroId, itemTpl.param1, true)
    -- 一键升级，从品质低到品质高
    else
        local itemList = hero.getHeroExpItemListToFullHeroLevel(heroId)
        for k, v in pairs(itemList) do
            if hero.canAddHeroExp(heroId) then
                local itemTpl = t.item[k]
                hero.addHeroExp(heroId, itemTpl.param1 * v)
                if not bag.removeItem(k,v,p.ResourceConsumeType.HERO_UPGRADE_LEVEL, true) then
                    hero.sendHeroUpdate()
                    heroUpgradeResponse(p.ErrorCode.FAIL)
                    return                
                end
            end
        end
        hero.sendHeroUpdate()
    end
    heroUpgradeResponse(p.ErrorCode.SUCCESS)
end

return hero