local M = {}
local cluster = require('cluster')
local p = require('protocol')
local t = require('tploader')
local timer = require('timer')
local dbo = require('dbo')
local pubHero = require('hero')
local misc = require('libs/misc')
local utils = require('utils')
local framework = require('framework')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()

local idService = require('idService')
local dataService = require('dataService')
local loginService = require('loginService')
local pushService = require('pushService')

local rawService
local impl = {}

--[[
--擂台信息arena[uid] = arenaInfo
arenaInfo = {
    uid = 0,                --玩家UID
    nickname = '',          --玩家昵称
    level = 0,              --玩家等级
    headId = 0,             --玩家头像

    aid = 0,                --玩家联盟ID
    allianceName = '',      --玩家联盟全称
    allianceNickname = '',  --玩家联盟昵称
    bannerId = 0,           --玩家联盟旗帜ID

    rank = 0,               --玩家排名
    winCount = 0,           --赢的数量
    loseCount = 0,          --输的数量
    winStreak = 0,          --连胜
    createTime = 0,         --创建时间
    isRobbot = false,       --是否是机器人

    lastRank = 0,           --上一次排行榜最终排名
    heroPower = 0,          --武将总战力
    heroList = {},          --武将防守列表 map<locate, heroInfo>  --武将战力改变 事件触发。只改变不通知
    attrList = {},          --武将相关的全局属性 {[attributeType]={base=0,pct=0,ext=0}, ...}
    isDirty = false,
}

rankMap[rank]=uid
--]]

--武将防守阵容
local arenaMap = {}    --map<uid, arenaInfo>
--排名
local rankMap = {}     --map<rank, uid>
--定点刷新数据
local arenaData = {
    rankExpiredTime = 0,
    isDirty = false,
}
--战力
local powerMap = {}    --map<uid, arenaInfo>
local powerSort = {}   --map<i, <uid=i, heroPoer=i>>

-- framework node shutdown
framework.beforeShutdown(function(cb)
    M.queueJob(function()
        utils.debug('framework.beforeShutdown arena')
        if M.timer ~= nil then
            M.timer:cancel()
            M.timer = nil
        end
        M.dbSave()
        cb()
    end, cb)
end)

function M.queueJob(threadFun, errorFun)
    jobExecutor:queue(threadFun, errorFun)
end

local function publishArenaRankUpdate(uid, rank)
    -- print('publishArenaRankUpdate....uid, rank', uid, rank)
    rawService:publishToAll('arenaRankUpdate', uid, rank)
end

local function publishArenaChangeOpponent(uid, list)
    -- print('publishArenaChangeOpponent....uid, list', uid, list)
    rawService:publishToAll('arenaChangeOpponent', uid, list)
end

local function checkHeroFetter(heroIdList)
    local function isExistHero(id)
        local isExist = false
        for _, heroId in pairs(heroIdList) do
            if heroId == id then
                isExist = true
                break
            end
        end
        return isExist
    end

    local ownFetters = {}
    local heroOwnFetterAttr = {}
    for _, heroId in pairs(heroIdList) do
        local ownFetterTpl = t.heroOwnFetter[heroId]
        if ownFetterTpl then
            for _,fetterId in pairs(ownFetterTpl) do
                if ownFetters[fetterId] == nil then
                    ownFetters[fetterId] = fetterId

                    local fetterHeros = {}
                    local fetterHeroCnt = 0
                    local fetterTpl = t.heroFetter[fetterId]
                    if fetterTpl then
                        for _,heroId in pairs(fetterTpl.heroList) do
                            if isExistHero(heroId) then
                                fetterHeros[heroId] = heroId
                                fetterHeroCnt = fetterHeroCnt + 1
                            end
                        end
                        if fetterHeroCnt > 1 then
                            local fetterAttributeTpl = t.heroFetterAttribute[fetterTpl.attrId]
                            if fetterAttributeTpl then
                               local attrCnt = #(fetterAttributeTpl.attrSet)
                               for i = attrCnt, 1, -1 do
                                    local attrSet = fetterAttributeTpl.attrSet[i]
                                    if fetterHeroCnt >= attrSet.cnt then
                                        local attr = attrSet.attr
                                        for k,v in pairs(fetterHeros) do
                                           if heroOwnFetterAttr[k] == nil then
                                                heroOwnFetterAttr[k] = {}
                                           end
                                           table.insert(heroOwnFetterAttr[k], attr)
                                        end
                                        break
                                    end
                                end
                            end
                        end
                    end
                end
            end
        end
    end
    -- print("CheckHeroFetter", utils.serialize(heroOwnFetterAttr))
    return heroOwnFetterAttr
end

local function getHeroProperty(heroBaseList, additionList, fetterAttrList, propAttrList)
    --heroBaseList = { hp=0, attack=0, defense=0, strategy=0, speed=0, leadership=0, rage=0, challenge=0, intellect=0 }
    --additionList = { [1008]={[1]=0,[2]=0,[3]=0}, ...}
    --propAttrList = { [1008]={base=0,pct=0,ext=0}, ...}
    --fetterAttrList = { [1320021]={ {1008,2,400,}, {1009,2,300,} } }

    -- print('combat.getOtherHeroProperty...heroBaseList', utils.serialize(heroBaseList))
    -- print('combat.getOtherHeroProperty...additionList', utils.serialize(additionList))
    -- print('combat.getOtherHeroProperty...fetterAttrList', utils.serialize(fetterAttrList))
    -- print('combat.getOtherHeroProperty...propAttrList', utils.serialize(propAttrList))
    if heroBaseList == nil then
        heroBaseList = {}
    end
    if additionList == nil then
        additionList = {}
    end
    if fetterAttrList == nil then
        fetterAttrList = {}
    end
    if propAttrList == nil then
        propAttrList = {}
    end

    local hp = heroBaseList.hp or 0
    local attack = heroBaseList.attack or 0
    local defense = heroBaseList.defense or 0
    local strategy = heroBaseList.strategy or 0
    local speed = heroBaseList.speed or 0
    local leadership = heroBaseList.leadership or 0
    local rage = heroBaseList.rage or 0
    local challenge = heroBaseList.challenge or 0
    local intellect = heroBaseList.intellect or 0

    for k, v in pairs(additionList) do
        local base = v[p.AttributeAdditionType.BASE] or 0
        local pct = v[p.AttributeAdditionType.PCT] or 0
        local ext = v[p.AttributeAdditionType.EXT] or 0
        --羁绊属性
        for _, fetter in pairs(fetterAttrList) do
            for _, attr in pairs(fetter) do
                if k == attributeType then
                    local attributeType = attr[1] or 0
                    local attrAddType = attr[2] or 0
                    local value = attr[3] or 0
                    if attrAddType == p.AttributeAdditionType.BASE then
                        base = base + value
                    elseif attrAddType == p.AttributeAdditionType.PCT then
                        pct = pct + value
                    elseif attrAddType == p.AttributeAdditionType.EXT then
                        ext = ext + value
                    end
                end
            end
        end
        --全局属性
        for attributeType, attr in pairs(propAttrList) do
            if k == attributeType then
                local tempBase = attr[p.AttributeAdditionType.BASE] or 0
                local tempPct = attr[p.AttributeAdditionType.PCT] or 0
                local tempExt = attr[p.AttributeAdditionType.EXT] or 0
                base = base + tempBase
                pct = pct + tempPct
                ext = ext + tempExt
            end
        end
        pct = pct/10000

        if k == p.AttributeType.HERO_ATTACK then
            attack = attack + base
            attack = math.floor( attack * (1 + pct) + ext )
        elseif k == p.AttributeType.HERO_DEFENSE then
            defense = defense + base
            defense = math.floor( defense * (1 + pct) + ext )
        elseif k == p.AttributeType.HERO_HEALTH then
            hp = hp + base
            hp = math.floor( hp * (1 + pct) + ext )
        elseif k == p.AttributeType.HERO_STRATEGY then
            strategy = strategy + base
            strategy = math.floor( strategy * (1 + pct) + ext )
        elseif k == p.AttributeType.HERO_SPEED then
            speed = speed + base
            speed = math.floor( speed * (1 + pct) + ext )
        elseif k == p.AttributeType.HERO_LEADERSHIP then
            leadership = leadership + base
            leadership = math.floor( leadership * (1 + pct) + ext )
        elseif k == p.AttributeType.HERO_RAGE then
            rage = rage + base
            rage = math.floor( rage * (1 + pct) + ext )
        elseif k == p.AttributeType.HERO_CHALLENGE then
            challenge = challenge + base
            challenge = math.floor( challenge * (1 + pct) + ext )
        elseif k == p.AttributeType.HERO_INTELLECT then
            intellect = intellect + base
            intellect = math.floor( intellect * (1 + pct) + ext )
        end
    end

    -- print('combat.getOtherHeroProperty...hp, attack, defense, strategy, speed, leadership, rage, challenge, intellect', hp, attack, defense, strategy, speed, leadership, rage, challenge, intellect)
    return hp, attack, defense, strategy, speed, leadership, rage, challenge, intellect
end

-- local function copyArenaMapData()
--     -- print('copyArenaMapData....begin...')
--     local sortList = {}
--     for _, info in pairs(arenaMap) do
--         --powerMap
--         local heroIdList = {}
--         for _, v in pairs(info.heroList) do
--             table.insert(heroIdList, v.id)
--         end
--         local fetterAttrList = checkHeroFetter(heroIdList)

--         local heroList = {}
--         local positionAssigned = {}
--         for _, v in pairs(info.heroList) do
--             local skill = v:getHeroSkill(false)
--             local equip = v:getHeroEquip()
--             local treasure = v:getHeroTreasure()
--             local additionList = v:getHeroAdditionList()
--             --士兵
--             local armyType = t.getArmyTypeByHero(v.id)
--             local heroBaseList = {
--                 hp = v.hp,
--                 attack = v.attack,
--                 defense = v.defense,
--                 strategy = v.strategy,
--                 speed = v.speed,
--                 leadership = v.leadership,
--                 rage = v.rage,
--                 challenge = v.challenge,
--                 intellect = v.intellect,
--             }
--             local hp, attack, defense, strategy, speed, leadership, rage, challenge, intellect = getHeroProperty(heroBaseList, additionList, fetterAttrList, info.attrList)
--             --站位
--             local position = 0
--             local armyTpl = t.findArmysTpl(armyType, 1)
--             if armyTpl then
--                 for _, v in ipairs(armyTpl.priority) do
--                     if positionAssigned[v] == nil then
--                         position = v
--                         positionAssigned[v] = 1
--                         break
--                     end
--                 end
--             end

--             heroList[v.id] = {
--                 id = v.id,
--                 level = v.level,
--                 star = v.star,
--                 hp = hp,
--                 attack = attack,
--                 defense = defense,
--                 strategy = strategy,
--                 speed = speed,
--                 leadership = leadership,
--                 rage = rage,
--                 challenge = challenge,
--                 intellect = intellect,
--                 skill = skill,
--                 equip = equip,
--                 treasure = treasure,
--                 additionList = additionList,

--                 singleHp = hp,     --武将单挑剩余血量
--                 singleRage = 0,   --武将单挑剩余怒气

--                 armyType = armyType,
--                 armyLevel = 1,  --TODO:根据玩家开放兵种等级的平均值设定
--                 armyCount = leadership,
--                 armyRage = 0,    --士兵剩余怒气
--                 position = position,
--                 }
--         end
--         powerMap[info.uid] = {
--             state = p.RidingAloneState.INIT,  --0初始 1准备 2战斗 3死亡
--             uid = info.uid,
--             nickname = info.nickname,
--             level = info.level,
--             headId = info.headId,
--             heroPower = info.heroPower,
--             heroList = heroList,
--             attrList = info.attrList,
--             }
--         table.insert(sortList, { uid = info.uid, heroPower = info.heroPower })
--     end
--     table.sort(sortList, function(left,right)
--         return left.heroPower < right.heroPower
--     end)
--     powerSort = sortList

--     -- for k, v in pairs(powerSort) do
--     --     print('k, uid, heroPower', k, v.uid, v.heroPower)
--     -- end

--     -- print('copyArenaMapData....end...', #powerSort)
-- end

function M.start()
    rawService = cluster.createService('arena', impl)
    M.dbLoad()
    --
    -- copyArenaMapData()

    M.timer = timer.setInterval(M.update, 1000)
end

function M.dbLoad()
    local db = dbo.open(0)
    local rs = db:executePrepare('SELECT uid, nickname, level, headId, aid, allianceName, allianceNickname, bannerId, rank, lastRank, winCount, loseCount, winStreak, createTime, isRobot, heroPower, heroList, attrList FROM s_arena where rank > 0')
    local count = 0
    if rs.ok then
        for k, row in ipairs(rs) do
            count = count + 1
            local uid = row.uid
            local nickname = row.nickname
            local level = row.level
            local headId = row.headId
            local aid = row.aid
            local allianceName = row.allianceName
            local allianceNickname = row.allianceNickname
            local bannerId = row.bannerId
            local rank = row.rank
            local lastRank = row.lastRank
            local winCount = row.winCount
            local loseCount = row.loseCount
            local winStreak = row.winStreak
            local createTime = row.createTime
            local isRobot = row.isRobot
            --武将防守
            if row.heroList == '' then
                row.heroList = '{}'
            end
            row.heroList = misc.deserialize(row.heroList)
            if row.attrList == '' then
                row.attrList = '{}'
            end
            local attrList = misc.deserialize(row.attrList)

            local heroList = {}
            local heroPower = 0
            for localte, v in pairs(row.heroList) do
                local heroInfo = pubHero.newHeroInfo(v)
                if heroInfo then
                    heroList[localte] = heroInfo
                    local power = heroInfo:getSingleHeroPower()
                    heroPower = heroPower + math.floor(power)
                    -- print('arena dbLoad...heroPower, heroInfo', heroPower, heroInfo:dump())
                end
            end
            arenaMap[uid] = {
                uid = uid,
                nickname = nickname,
                level = level,
                headId = headId,
                aid = aid,
                allianceName = allianceName,
                allianceNickname = allianceNickname,
                bannerId = bannerId,
                rank = rank,
                winCount = winCount,
                loseCount = loseCount,
                winStreak = winStreak,
                createTime = createTime,
                isRobot = isRobot,
                lastRank = lastRank,
                heroPower = heroPower,
                heroList = heroList,
                attrList = attrList,
            }
            rankMap[rank] = uid
        end

        if count == 0 then
            M.createRobot()
        end
        -- for _, v in pairs(arenaMap) do
        --     print('arena dbLoad...uid, nickname, level, headId, rank, winCount, loseCount, winStreak, isRobot, lastRank', v.uid, v.nickname, v.level, v.headId, v.rank, v.winCount, v.loseCount, v.winStreak, v.isRobot, v.lastRank)
        --     for _, info in pairs(v.heroList) do
        --         info:dump()
        --     end
        -- end
    end

    local rs2 = db:executePrepare('SELECT v FROM s_dict WHERE uid = 0 and k = ?', 'cs_arena.data')
    if rs2.ok then
        for _, row in ipairs(rs2) do
            local data = misc.deserialize(row.v)
            arenaData.rankExpiredTime = data.rankExpiredTime or 0
            break
        end
    end
    -- print('arena  dbLoad....data.rankExpiredTime', arenaData.rankExpiredTime)
end

function M.dbSave()
    local db = dbo.open(0)
    --arenaData
    if arenaData.isDirty then
        local db = dbo.open(0)
        local k = 'cs_arena.data'
        local v = utils.serialize({ rankExpiredTime = arenaData.rankExpiredTime })
        local rs = db:executePrepare('insert into s_dict(uid, k, v) values (0, ?, ?) on duplicate key update v=values(v)', k, v)
        if rs.ok then
            arenaData.isDirty = false
        else
            utils.log('arena db save arenaData error')
        end
    end

    local list = {}
    for _, v in pairs(arenaMap) do
        -- print('------------------dbSave..........v.isDirty', v.isDirty)
        if v.isDirty then
            --武将防守列表
            local heroList = {}
            for _, heroInfo in pairs(v.heroList) do
                local skillTemp = {}
                for slot, info in pairs(heroInfo.skill) do
                    skillTemp[slot] = { itemId = info.itemId, tplId = info.tplId, type = info.type, level = info.level}
                end

                table.insert(heroList, { id = heroInfo.id, level = heroInfo.level, star = heroInfo.star, soulLevel = heroInfo.soulLevel, skill = skillTemp})
            end
            heroList = utils.serialize(heroList)
            -- print('-- arena dbSave....heroList', heroList)
            local attrList = utils.serialize(v.attrList)

            table.insert(list, {
                uid = v.uid,
                aid = v.aid,
                nickname = v.nickname,
                level = v.level,
                headId = v.headId,
                allianceName = v.allianceName,
                allianceNickname = v.allianceNickname,
                bannerId = v.bannerId,
                rank = v.rank,
                lastRank = v.lastRank,
                winCount = v.winCount,
                loseCount = v.loseCount,
                winStreak = v.winStreak,
                createTime = v.createTime,
                isRobot = v.isRobot,
                heroPower = v.heroPower,
                heroList = heroList,
                attrList = attrList,
                })
        end
    end
    -- batch save
    while #list >= 10 do
        -- print('arena..batch save', #list)
        local saveList = {}
        for i=1, 10 do
            local data = table.remove(list, 1)
            table.insert(saveList, data)
        end
        local rs = db:insertBatch('s_arena', saveList, {'aid', 'nickname', 'level', 'headId', 'allianceName', 'allianceNickname', 'bannerId','rank', 'lastRank', 'winCount', 'loseCount', 'winStreak', 'isRobot', 'heroPower', 'heroList', 'attrList' })
        if rs.ok and rs.affectedRows > 0 then
            for _, v in pairs(saveList) do
                if arenaMap[v.uid] then
                    arenaMap[v.uid].isDirty = false
                end
            end
        elseif not rs.ok then
            utils.log('arena db batch save s_arena error' .. utils.toJson(rs))
        end
    end
    --single save
    while #list > 0 do
        -- print('arena..single save', #list)
        local saveList = {}
        local data = table.remove(list, 1)
        table.insert(saveList, data)
        local rs = db:insertBatch('s_arena', saveList, {'aid', 'nickname', 'level', 'headId', 'allianceName', 'allianceNickname', 'bannerId', 'rank', 'lastRank', 'winCount', 'loseCount', 'winStreak', 'isRobot', 'heroPower', 'heroList', 'attrList' })
        if rs.ok and rs.affectedRows > 0 then
            for _, v in pairs(saveList) do
                if arenaMap[v.uid] then
                    arenaMap[v.uid].isDirty = false
                end
            end
        elseif not rs.ok then
            utils.log('arena db single save s_arena error' .. utils.toJson(rs))
        end
    end
end

function M.removeArena(uid)
end

function M.createRobot()
    print('-----createRobot start.....')
    --武将按品质分类
    local totalHeroList = {}
    local totalHero = 0
    for _, v in pairs(t.heros) do
        if totalHeroList[v.sQuality] == nil then
            totalHeroList[v.sQuality] = {}
        end
        table.insert(totalHeroList[v.sQuality], v.id)
        totalHero = totalHero + 1
    end

    -- local test = {
    --     { rankRange = {1,5}, userRange = {18,20}, qualityRange = {up={5,3},low={4,2}}, heroLevelRange = {18,20}, heroStarRange = {4,7}, skillRange = {18,20}, equipRange = {} },
    --     { rankRange = {6,100}, userRange = {15,18}, qualityRange = {up={4,2},low={3,3}}, heroLevelRange = {15,18}, heroStarRange = {4,7}, skillRange = {15,18}, equipRange = {} },
    --     { rankRange = {101,200}, userRange = {12,16}, qualityRange = {up={5,3},low={4,2}}, heroLevelRange = {12,16}, heroStarRange = {2,7}, skillRange = {12,16}, equipRange = {} },
    --     { rankRange = {201,1000}, userRange = {10,13}, qualityRange = {up={5,3},low={4,2}}, heroLevelRange = {10,13}, heroStarRange = {2,4}, skillRange = {10,13}, equipRange = {} },
    --     { rankRange = {1001,3000}, userRange = {8,11}, qualityRange = {up={5,3},low={4,2}}, heroLevelRange = {8,11}, heroStarRange = {1,4}, skillRange = {8,11}, equipRange = {} },
    --     { rankRange = {3001,5000}, userRange = {7,9}, qualityRange = {up={5,3},low={4,2}}, heroLevelRange = {7,9}, heroStarRange = {1,2}, skillRange = {7,9}, equipRange = {} },
    -- }
    -- print("###test", utils.serialize(test))

    -- local test = t.arenaRobot

    local needSort = false
    for _, v in pairs(t.arenaRobot) do
        local rankMin = v.rankRange[1]
        local rankMax = v.rankRange[2]
        for rank=rankMin, rankMax do
            --区间
            local userMin = v.userRange[1]
            local userMax = v.userRange[2]

            local qualityUp = v.qualityRange.up[1]
            local qualityUpNum = v.qualityRange.up[2]

            local qualityLow = v.qualityRange.low[1]
            local qualityLowNum = v.qualityRange.low[2]


            local qualityMin = v.qualityRange[1]
            local qualityMax = v.qualityRange[2]


            local herolevelMin = v.heroLevelRange[1]
            local herolevelMax = v.heroLevelRange[2]
            local heroStarMin = v.heroStarRange[1]
            local heroStarMax = v.heroStarRange[2]
            local skillMin = v.skillRange[1]
            local skillMax = v.skillRange[2]
            local soulMin = v.soulRange[1]
            local soulMax = v.soulRange[2]

            --城主等级
            local uid = idService.createUid()
            local nickname = 'K' .. uid
            local headId = 3000001
            local userLevel = utils.getRandomNum(userMin, userMax+1)

            local heroPower = 0
            local heroList = {}
            local hasList = {}
            for i=1, 5 do
                --根据品质 获取武将ID
                local heroId = 0
                local quality
                if i <= qualityUpNum then
                    quality = qualityUp
                else
                    quality = qualityLow
                end
                local tempList = {}
                for _, tempHeroId in pairs(totalHeroList[quality]) do
                    if not hasList[tempHeroId] then
                        table.insert(tempList, tempHeroId)
                        table.insert(hasList, tempHeroId)
                    end
                end
                if #tempList == 0 then
                    for _, t in pairs(totalHeroList) do
                        for _, tempHeroId in pairs(t) do
                            if not hasList[tempHeroId] then
                                table.insert(tempList, tempHeroId)
                                table.insert(hasList, tempHeroId)
                            end
                        end
                    end
                end
                if #tempList > 0 then
                    local index = utils.getRandomNum(1, #tempList+1)
                    heroId = tempList[index]
                else
                    utils.log('arena createRobot error, quality=' .. tostring(quality) .. ',rankRange=' .. utils.serialize(v.rankRange) .. ', hasList=' .. utils.serialize(v.rankRange))
                end
                --设置武将
                local heroTpl = t.heros[heroId]
                if heroId > 0 and heroTpl then
                    --等级星级
                    local herolevel = utils.getRandomNum(herolevelMin, herolevelMax+1)
                    local heroStar = utils.getRandomNum(heroStarMin, heroStarMax+1)
                    local heroSoul = utils.getRandomNum(soulMin, soulMax+1)
                    if heroStar < heroTpl.star then
                        heroStar = heroTpl.star
                    end
                    if heroSoul >= heroTpl.maxSoul then
                        heroSoul = heroTpl.maxSoul - 1
                        if heroSoul < 0 then
                            heroSoul = 0
                        end
                    end
                    -- 擂台没有技能
                    -- --技能
                    -- local skill = {}
                    -- local skillLevel = utils.getRandomNum(skillMin, skillMax+1)
                    -- if heroTpl.initSkill > 0 then
                    --     skill[1] = { tplId = heroTpl.initSkill, itemId = 0, type = p.HeroSkillType.HERO_SKILL_INIT, level = skillLevel, exp = 0, isOpen = true }
                    -- end
                    -- if heroTpl.uniqueSkill > 0 then
                    --     skill[4] = { tplId = heroTpl.uniqueSkill, itemId = 0, type = p.HeroSkillType.HERO_SKILL_UNIQUE, level = skillLevel, exp = 0, isOpen = true }
                    -- end
                    --装备
                    --equipRange={[装备位1]={itemId=装备ID, levelRange={x,y}, inlay={[孔位1]=宝石ID, ...}, succinct={[洗练位1]=洗练属性ID}}, ....}
                    --
                    local heroInfo = pubHero.newHeroInfo({ id = heroId, level = herolevel, star = heroStar, skill = skill, soulLevel = heroSoul})
                    heroList[i] = heroInfo
                    --武将战力
                    local power = heroInfo:getSingleHeroPower()
                    heroPower = heroPower + math.floor(power)
                    -- print('arena createRobot...heroPower, heroInfo', heroPower, heroInfo:dump())
                end
            end

            if #heroList == 5 then
                arenaMap[uid] = {
                    uid = uid,
                    nickname = nickname,
                    level = userLevel,
                    headId = headId,
                    aid = 0,
                    allianceName = '',
                    allianceNickname = '',
                    bannerId = 0,
                    rank = rank,
                    winCount = 0,
                    loseCount = 0,
                    winStreak = 0,
                    createTime = timer.getTimestampCache(),
                    isRobot = true,
                    lastRank = 0,
                    heroPower = heroPower,
                    heroList = heroList,
                    attrList = {},
                    isDirty = true,
                }
                rankMap[rank] = uid
            else
                -- print("#heroList", #heroList)
                needSort = true
            end
        end
    end

    if needSort then
        -- print('createRobot...needSort,#arenaMap', needSort, #arenaMap)
        local sortList = {}
        for uid, v in pairs(arenaMap) do
            table.insert(sortList, { heroPower = v.heroPower, uid = uid })
        end
        table.sort(sortList, function(left, right)
            return left.heroPower > right.heroPower
        end)

        rankMap = {}
        for rank, v in pairs(sortList) do
            rankMap[rank] = v.uid
            arenaMap[v.uid].rank = rank
        end
    end

    for rank, uid in ipairs(rankMap) do
        local info = arenaMap[uid]
        -- print('createRobot...rank1, rank2, uid, level, heroPower', rank, info.rank, uid, info.level, info.heroPower)
    end

    --[[
    local mixStar = 1
    local maxStar = 6
    local mixEquipQ = 1
    local maxEquipQ = 5

    local totalHeroList = {}
    local totalHero = 0
    for _, v in pairs(t.hero) do
        table.insert(totalHeroList, v.id)
        totalHero = totalHero + 1
    end

    local equipList = {}
    local totalEquip = 0
    for _, v in pairs(t.item) do
        if v.isCanUse and v.type == p.ItemType.EQUIP and v.quality >= mixEquipQ and v.quality <= maxEquipQ then
            table.insert(equipList, { id = v.id, subType = v.subType})
            totalEquip = totalEquip + 1
        end
    end

    local maxRobot = 3000
    local rank = 1
    local createTime = timer.getTimestampCache()
    while maxRobot > 0 do
        local uid = idService.createUid()
        local nickname = 'K' .. uid
        local headId = 3000001
        local heroPower = 0
        local heroList = {}
        local userLevel = utils.getRandomNum(1, 30)
        local hasList = {}
        for i=1, 5 do
            local heroId = 0
            repeat
                local index = utils.getRandomNum(1, totalHero+1)
                if not hasList[index] then
                    hasList[index] = index
                    heroId = totalHeroList[index]
                end
            until heroId > 0
            if heroId > 0 then
                local level = utils.getRandomNum(1, userLevel+1)
                local star = utils.getRandomNum(mixStar, maxStar+1)
                local index = utils.getRandomNum(1, totalEquip+1)
                local equip = {}
                -- local tplId = equipList[index].id
                -- local subType = equipList[index].subType
                -- local equipLevel = utils.getRandomNum(1, level+1)
                -- equip[subType] = { tplId = tplId, type = subType, level = equipLevel, succinct = {}, inlay = {} }
                local heroInfo = pubHero.newHeroInfo({ id = heroId, level = level, star = star, skill = {}, equip = equip, treasure = {} })
                heroList[i] = heroInfo

                local power = heroInfo:getSingleHeroPower()
                heroPower = heroPower + math.floor(power)
                -- print('arena createRobot...heroPower, heroInfo', heroPower, heroInfo:dump())
            end
        end

        arenaMap[uid] = {
            uid = uid,
            nickname = nickname,
            level = userLevel,
            headId = headId,
            aid = 0,
            allianceName = '',
            allianceNickname = '',
            bannerId = 0,
            rank = rank,
            winCount = 0,
            loseCount = 0,
            winStreak = 0,
            createTime = createTime,
            isRobot = true,
            lastRank = 0,
            heroPower = heroPower,
            heroList = heroList,
            attrList = {},
            isDirty = true,
        }
        rankMap[rank] = uid

        maxRobot = maxRobot - 1
        rank = rank + 1
    end
    --]]

    --
    M.dbSave()
    print('-----createRobot end.....')
end

function M.update()
    local now = timer.getTimestampCache()
    if arenaData.rankExpiredTime == 0 then
        arenaData.rankExpiredTime = now
        arenaData.isDirty = true
    end

    --保存上期最终排名
    if timer.isTsShouldRefreshAtHour(arenaData.rankExpiredTime, 21) then
        -- print('arena lastRank update...rankExpiredTime', arenaData.rankExpiredTime)
        arenaData.rankExpiredTime = timer.getTimestampCache()
        arenaData.isDirty = true
        for _, v in pairs(arenaMap) do
            v.lastRank = v.rank
            v.isDirty = true
        end

        --todo by zds
        --TODO:看具体要求
        -- copyArenaMapData()
    end

    if now % 300 == 0 then
        print(' arena timerUpdate 5 minutes  job ...')
        M.queueJob(function()
            M.dbSave()
        end)
    end
end

--联盟等级旗帜全称简称
function M.onArenaAllianceDataChange(uid, aid, allianceName, allianceNickname, bannerId)
    -- print('M.onArenaAllianceDataChange...uid, aid, allianceName, allianceNickname, bannerId', uid, aid, allianceName, allianceNickname, bannerId)
    local info = arenaMap[uid]
    if info then
        info.aid = aid
        info.allianceName = allianceName
        info.allianceNickname = allianceNickname
        info.bannerId = bannerId
        info.isDirty = true
    end
end

--
-- alliance service api implement
--

function impl.arenaSetDefense(uid, nickname, level, headId, aid, allianceName, allianceNickname, bannerId, heroList, attrList)
    -- print('impl.arenaSetDefense...uid, nickname, level, headId, aid, allianceName, allianceNickname, bannerId, heroList, attrList',uid, nickname, level, headId, aid, allianceName, allianceNickname, bannerId, utils.serialize(heroList), attrList)
    -- print('impl.arenaSetDefense...attrList=', utils.serialize(attrList))
    local heroPower = 0
    local list = {}
    for locate, v in pairs(heroList) do
        local heroInfo = pubHero.newHeroInfo(v)
        list[locate] = heroInfo
        local power = heroInfo:getSingleHeroPower()
        heroPower = heroPower + math.floor(power)
    end
    if attrList == nil then
        attrList = {}
    end

    if arenaMap[uid] then
        -- print('------------impl.arenaSetDefense...befor---------------')
        -- for _, v in pairs(arenaMap[uid].heroList) do
        --     print('heroId, level, star', v.id, v.level, v.star)
        -- end
        local info = arenaMap[uid]
        info.nickname = nickname
        info.level = level
        info.headId = headId
        info.aid = aid
        info.allianceName = allianceName
        info.allianceNickname = allianceNickname
        info.bannerId = bannerId

        info.heroPower = heroPower
        info.heroList = nil
        info.heroList = list
        info.attrList = attrList
        info.isDirty = true
    else
        -- print('impl.arenaSetDefense...befor', arenaMap[uid])
        local createTime = timer.getTimestampCache()
        local rank = #rankMap + 1
        rankMap[rank] = uid

        arenaMap[uid] = {
            uid = uid,
            nickname = nickname,
            level = level,
            headId = headId,
            aid = aid,
            allianceName = allianceName,
            allianceNickname = allianceNickname,
            bannerId = bannerId,
            rank = rank,
            winCount = 0,
            loseCount = 0,
            winStreak = 0,
            createTime = createTime,
            isRobot = false,
            lastRank = 0,
            heroPower = heroPower,
            heroList = list,
            attrList = attrList,
            isDirty = true,
        }
        --publish
        publishArenaRankUpdate(uid, rank)
    end
    -- print('------------impl.arenaSetDefense...end--------------------')
    -- for _, v in pairs(arenaMap[uid].heroList) do
    --     print('heroId, level, star', v.id, v.level, v.star)
    -- end
end

function impl.arenaChangeOpponent(uid)
    -- print('impl.arenaChangeOpponent....uid', uid)
    -- local test = {
    --     { rankRange = {1,1}, leftRule = {1,1}, midRule = {2,2}, rightRule = {3,3} },
    --     { rankRange = {2,2}, leftRule = {-1,-1}, midRule = {1,1}, rightRule = {2,2} },
    --     { rankRange = {3,3}, leftRule = {-2,-2}, midRule = {-1,-1}, rightRule = {1,1} },
    --     { rankRange = {4,4}, leftRule = {-3,-3}, midRule = {-2,-2}, rightRule = {-3,-3} },

    --     { rankRange = {5,10}, leftRule = {-5,0}, midRule = {-2,2}, rightRule = {3,10} },
    --     { rankRange = {11,30}, leftRule = {-5,0}, midRule = {-2,5}, rightRule = {8,10} },

    --     { rankRange = {31,50}, leftRule = {-15,-10}, midRule = {-11,0}, rightRule = {8,10} },
    --     { rankRange = {51,100}, leftRule = {-20,-15}, midRule = {-2,5}, rightRule = {8,10} },

    --     { rankRange = {101,300}, leftRule = {-50,-15}, midRule = {-14,5}, rightRule = {8,10} },
    --     { rankRange = {301,500}, leftRule = {-200,-100}, midRule = {-99,0}, rightRule = {8,10} },
    --     { rankRange = {501,1000}, leftRule = {-300,-200}, midRule = {-199,-50}, rightRule = {-49,10} },
    --     { rankRange = {1001,999999999}, leftRule = {-500,-400}, midRule = {-399,-100}, rightRule = {-99,10} },
    -- }
    local t1 = os.clock()
    local updateList = {}
    local tempList = {}
    tempList[uid] = 1
    local maxRank = #rankMap
    local myRank = maxRank
    if arenaMap[uid] then
         myRank = arenaMap[uid].rank
    end

    local rule = {}
    -- for _, v in pairs(t.arenaRule) do
    -- for _, v in pairs(test) do
    --     local rankMin = v.rankRange[1]
    --     local rankMax = v.rankRange[2]
    --     if myRank >= rankMin and myRank <= rankMax then
    --         rule = v
    --         break
    --     end
    -- end

    local function get(ruleMin, ruleMax)
        --头
        if ruleMin < 1 then
            ruleMin = 1
        end
        if ruleMax < 1 then
            ruleMax = ruleMin + 10
        end
        --尾
        if ruleMin > maxRank then
            ruleMin = maxRank - 50
        end
        if ruleMax > maxRank then
            ruleMax = maxRank
        end
        --名次异常情况
        if ruleMin > ruleMax then
            ruleMin = maxRank - 50
            ruleMax = maxRank
        end

        --根据名次范围随机选定玩家
        local findCount = 0
        local bFind = false
        local otherUid = 0
        repeat
            local index = utils.getRandomNum(ruleMin, ruleMax + 1)
            -- print("###arenaChangeOpponent", index)
            otherUid = rankMap[index] or 0
            if arenaMap[otherUid] and not tempList[otherUid] then
                bFind = true
            end

            findCount = findCount + 1
            if not bFind and findCount >= 10 then
                --随机玩家异常情况
                ruleMin = myRank - 5
                ruleMax = myRank + 10
                for i=ruleMin, ruleMax do
                    otherUid = rankMap[i] or 0
                    if arenaMap[otherUid] and not tempList[otherUid] then
                        bFind = true
                        break
                    end
                end
                -- print('impl.arenaChangeOpponent...findCount, ruleMin, ruleMax, bFind', findCount, ruleMin, ruleMax, bFind)
                bFind = true
            end
        until bFind

        local info = arenaMap[otherUid]
        if info then
            local heroList = {}
            for locate, v in pairs(info.heroList) do
                local skill = v:getHeroSkill(false)
                local equip = {}
                local treasure = {}
                local additionList = v:getHeroAdditionList()
                local secondAdditionList = v:getHeroSecondAdditionList()
                table.insert(heroList, {
                    locate = locate,
                    heroId = v.id,
                    level = v.level,
                    star = v.star,
                    soulLevel = v.soulLevel,
                    hp = v.hp,
                    attack = v.attack,
                    defense = v.defense,
                    strategy = v.strategy,
                    speed = v.speed,
                    leadership = v.leadership,
                    rage = v.rage,
                    challenge = v.challenge,
                    intellect = v.intellect,
                    skill = skill,
                    equip = equip,
                    treasure = treasure,
                    additionList = additionList,
                    secondAdditionList = secondAdditionList,
                    })
            end
            table.insert(updateList, {
                uid = info.uid,
                nickname = info.nickname,
                allianceName = info.allianceName,
                level = info.level,
                headId = info.headId,
                rank = info.rank,
                winCount = info.winCount,
                loseCount = info.loseCount,
                power = info.heroPower,
                heroList = heroList,
                attrList = info.attrList,
                isRobot = info.isRobot,
                })

            tempList[info.uid] = 1
        end
    end
    
    -- 1.若玩家的排名在前10名，则在1-10名中随机选择3个非自己的玩家作为可选择挑战的领主
    if myRank <= 10 then
        local min = 1
        local max = 10
        get(min, max)

        get(min, max)

        get(min, max)
    -- 2.若玩家的排名在11-30名，则在自己排名前面1-10名的玩家中随机选择2个玩家作为可选择挑战的领主,自己排名后面1-5名的玩家中随机选择一个玩家作为挑战的领主
    elseif 10 < myRank and myRank <= 30 then
        local min = 1
        local max = 10
        get(min, max)
        get(min, max)
        min = myRank + 1
        max = myRank + 5
        get(min, max)
    -- 3.若玩家的排名在31-100名，则在自己排名前面1-15名的玩家中随机选择2个玩家作为可选择挑战的领主,自己排名后面1-5名的玩家中随机选择一个玩家作为挑战的领主。
    elseif 30 < myRank and myRank <= 100 then
        local min = myRank - 15
        local max = myRank - 1
        get(min, max)
        get(min, max)
        min = myRank + 1
        max = myRank + 5
        get(min, max)
    -- 4.若玩家的排名在100名以外，则在自己排名80%~100%的玩家中随机选择2个作为可选择挑战的领主，自己排名100%-105%的玩家中随机选择一个玩家作为挑战的领主
    elseif 100 < myRank then
        -- get(1, 2)
        local min = math.floor(myRank * 0.80)
        local max = myRank
        get(min, max)
        get(min, max)
        min = myRank
        max = math.floor(myRank * 1.05)
        get(min, max)
    else
        utils.log('impl.arenaChangeOpponent error, uid=' .. tostring(uid) .. ', myRank=' .. tostring(myRank))
    end
    -- if next(rule) then
    --     local min = rule.leftRule[1] + myRank
    --     local max = rule.leftRule[2] + myRank
    --     get(min, max)

    --     min = rule.midRule[1] + myRank
    --     max = rule.midRule[2] + myRank
    --     get(min, max)

    --     min = rule.rightRule[1] + myRank
    --     max = rule.rightRule[2] + myRank
    --     get(min, max)
    -- else
    --     utils.log('impl.arenaChangeOpponent error, uid=' .. tostring(uid) .. ', myRank=' .. tostring(myRank))
    -- end

    local t2 = os.clock()
    print('=================clock2-clock1', t2-t1)
    -- print('impl.arenaChangeOpponent.... #updateList, updateList', #updateList, utils.serialize(updateList))
    if #updateList > 0 then
        publishArenaChangeOpponent(uid, updateList)
    end
end

function impl.getArenaDefense(uid)
    local myRank, lastRank, heroList = 0, 0, {}
    local info = arenaMap[uid]
    if info then
        myRank = info.rank
        lastRank = info.lastRank
        for locate, v in pairs(info.heroList) do
            table.insert(heroList, { locate = locate, heroId = v.id })
        end
    end
    -- print('impl.getArenaDefense...myRank, lastRank, heroList, info', myRank, lastRank, heroList, info)
    return myRank, lastRank, heroList
end

function impl.getRevengeDataByUid(uid)
    local info = arenaMap[uid]
    local data = nil
    if info then
    -- print("impl.getRevengeDataByUid", utils.serialize(info))
        local heroList = {}
        for locate, v in pairs(info.heroList) do
            local skill = v:getHeroSkill(false)
            local equip = v:getHeroEquip()
            local treasure = v:getHeroTreasure()
            local additionList = v:getHeroAdditionList()
            local secondAttrAddtionList = v:getHeroSecondAdditionList()
            table.insert(heroList, {
                locate = locate,
                heroId = v.id,
                level = v.level,
                star = v.star,
                soulLevel = v.soulLevel,
                hp = v.hp,
                attack = v.attack,
                defense = v.defense,
                strategy = v.strategy,
                speed = v.speed,
                leadership = v.leadership,
                rage = v.rage,
                challenge = v.challenge,
                intellect = v.intellect,
                skill = skill,
                equip = equip,
                treasure = treasure,
                additionList = additionList,
                secondAttrAddtionList = secondAttrAddtionList,
                })
        end
        data = {heroList = heroList, uid = info.uid, nickname = info.nickname, rank = info.rank, level = info.level, headId = info.headId, attrList = info.attrList}
        return data
    end
end

function impl.arenaCombat(myUid, toUid, isWin, battleData)
    --details = { attack={}, defense={}, battleData='' }
    -- print('impl.arenaCombat...myUid, toUid, isWin, details', myUid, toUid, isWin)
    local myInfo = arenaMap[myUid]
    local toInfo = arenaMap[toUid]
    local changRank = 0
    if isWin then
        myInfo.winCount = myInfo.winCount + 1
        myInfo.winStreak = myInfo.winStreak + 1
        toInfo.loseCount = toInfo.loseCount + 1
        --排名变更 rankMap arenaMap
        local myRank = myInfo.rank
        local toRank = toInfo.rank
        -- print('impl.arenaCombat...myRank, toRank', myRank, toRank)
        if myRank > toRank then
            changRank = myRank - toRank
            myInfo.rank = toRank
            toInfo.rank = myRank
            rankMap[myRank] = toInfo.uid
            rankMap[toRank] = myInfo.uid
            --publish
            -- print('impl.arenaCombat...isRobot, uid, oldRank, newRank', myInfo.isRobot, myUid, myRank, myInfo.rank )
            if not myInfo.isRobot then
                -- arenaMap[myUid] = toInfo
                publishArenaRankUpdate(myUid, myInfo.rank)
            end
            if not toInfo.isRobot then
                -- arenaMap[toUid] = myInfo
                publishArenaRankUpdate(toUid, toInfo.rank)
            end
        end
        -- if myInfo.winStreak > 0 and myInfo.winStreak % 10 == 0 then
        --     --TODO 连胜10次、20次、50次、100次，都播放广播
        --     local param = myInfo.nickname .. ',' .. tostring(myInfo.winStreak)
        --     loginService.broadNoticeMessage(p.ErrorCode. , 2, param)

        --     --TODO 当连胜10场或20、30等特定场次时，加个推送功能
        --     local pushInfo = { uid = myUid, classify = p.PushClassify. , tag = 0, cdId = 0, sendTime = 0 }
        --     pushService.appendPush(pushInfo)
        -- end
    else
        myInfo.loseCount = myInfo.loseCount + 1
        myInfo.winStreak = 0
        toInfo.winCount = toInfo.winCount + 1
    end
    myInfo.isDirty = true
    toInfo.isDirty = true
    -- --TODO 玩家进入前20名，挑战对手时不管输赢，都播放广播?????
    -- if myInfo.rank <= 20 then
    --     local param = myInfo.nickname .. ',' .. tostring(myInfo.rank)
    --     loginService.broadNoticeMessage(p.ErrorCode. , 2, param)
    -- end
    --攻击者
    local myRecord = {
        id = utils.createItemId(),
        uid = myInfo.uid,
        nickname = myInfo.nickname,
        level = myInfo.level,
        headId = myInfo.headId,
        allianceName = myInfo.allianceName,
        allianceNickname = myInfo.allianceNickname,
        bannerId = myInfo.bannerId,

        toUid = toInfo.uid,
        toNickname = toInfo.nickname,
        toLevel = toInfo.level,
        toHeadId = toInfo.headId,
        toAllianceName = toInfo.allianceName,
        toAllianceNickname = toInfo.allianceNickname,
        toBannerId = toInfo.bannerId,

        isAttacker = true,
        isWin = isWin,
        changRank = changRank,
        createTime = timer.getTimestampCache(),
        -- details = myDetails,
        battleData = battleData,
        canRevenge = false,
        toPower = arenaMap[toUid].heroPower,
    }
    dataService.appendData(p.DataClassify.ARENA_RECORD, myRecord)

    --防守者
    if not toInfo.isRobot then
        local toWin = false
        local isCanRevenge = false
        if not isWin then
            toWin = true
        else
            isCanRevenge = true
        end
        local toRecord = {
            id = utils.createItemId(),
            uid = toInfo.uid,
            nickname = toInfo.nickname,
            level = toInfo.level,
            headId = toInfo.headId,
            allianceName = toInfo.allianceName,
            allianceNickname = toInfo.allianceNickname,
            bannerId = toInfo.bannerId,

            toUid = myInfo.uid,
            toNickname = myInfo.nickname,
            toLevel = myInfo.level,
            toHeadId = myInfo.headId,
            toAllianceName = myInfo.allianceName,
            toAllianceNickname = myInfo.allianceNickname,
            toBannerId = myInfo.bannerId,

            isAttacker = false,
            isWin = toWin,
            changRank = changRank * (-1),
            createTime = timer.getTimestampCache(),
            -- details = toDetails,
            battleData = battleData,
            canRevenge = isCanRevenge,
            toPower = arenaMap[myUid].heroPower,
        }
        dataService.appendData(p.DataClassify.ARENA_RECORD, toRecord)
    end
end

function impl.arenaHeroDataChange(uid, heroId, arenaDataType, data)
    -- print('impl.arenaHeroDataChange....uid, heroId, arenaDataType, data', uid, heroId, arenaDataType, utils.serialize(data))
    if data == nil then
        return
    end
    -- todo by zds  擂台武将属性改变
    if arenaMap[uid] then
        for _, heroInfo in pairs(arenaMap[uid].heroList) do
            if heroInfo.id == heroId then
                local isFresh = false
                --武将等级星级
                if p.ArenaDataType.HERO_LEVEL_STAR == arenaDataType then
                    if data.level and data.star then
                        heroInfo.level = data.level
                        heroInfo.star = data.star
                        heroInfo:checkSkillSlot()
                        isFresh =  true
                    end
                -- 命魂
                elseif p.ArenaDataType.HERO_SOUL == arenaDataType then
                    if data.tplId and data.level then
                        local soulInfo = heroInfo.soul[data.tplId]
                        if soulInfo then
                            soulInfo.level = data.level
                        else
                            local soulTpl = t.heroSoul[data.tplId]
                            if not soulTpl then
                                heroInfo.soul[data.tplId] = pubHero.soulInfo:new({tplId = soulTpl.id, soulLevel = soulTpl.soulLevel, level = data.level})
                            end
                        end
                        isFresh =  true
                    end
                elseif p.ArenaDataType.HERO_SKILL == arenaDataType then
                    if data.tplId and data.level and data.slot and data.type then
                        local skillInfo = heroInfo.skill[data.tplId]
                        if skillInfo then
                            for k, info in pairs(heroInfo.skill) do
                                if info.type == p.HeroSkillType.HERO_SKILL_STAR and info.slot == slot then
                                     info.slot = 0
                                end  
                            end
                            skillInfo.level = data.level
                            skillInfo.slot = data.slot
                        else
                            heroInfo.skill[data.tplId] = pubHero.skillInfo:new({tplId = data.tplId, type = data.type, level = data.level, slot = data.slot})
                        end
                        isFresh =  true
                    end
                elseif p.ArenaDataType.USER_DATA_CHANGE == arenaDataType then
                    if data.slot and data.level then
                        local equipInfo = heroInfo.equip[data.slot]
                        if equipInfo then
                            equipInfo.level = data.level
                            isFresh =  true
                        end
                    end
                end

                if isFresh then
                    -- print('mpl.arenaHeroDataChange...before heroPower=', arenaMap[uid].heroPower)
                    heroInfo:setHeroBaseProperty()
                    local heroPower = 0
                    for _, v in pairs(arenaMap[uid].heroList) do
                        local power = v:getSingleHeroPower()
                        heroPower = heroPower + math.floor(power)
                    end
                    arenaMap[uid].heroPower = heroPower
                    arenaMap[uid].isDirty = true
                    -- print('mpl.arenaHeroDataChange...end heroPower=', arenaMap[uid].heroPower)
                end
                break
            end
        end
    end
end

function impl.arenaUserDataChange(uid, level, nickname, headId)
    -- print('impl.arenaUserDataChange...uid, level, nickname, headId', uid, level, nickname, headId)
    local info = arenaMap[uid]
    if info then
        -- print('impl.arenaUserDataChange..before---.uid, level, nickname, headId', uid, info.level, info.nickname, info.headId)
        info.nickname = nickname or info.nickname
        info.level = level or info.level
        info.headId = headId or info.headId
        info.isDirty = true
        -- print('impl.arenaUserDataChange..end---.uid, level, nickname, headId', uid, info.level, info.nickname, info.headId)
    end
end

function impl.arenaPropChange(uid, attrList)
    -- print('impl.arenaPropChange...uid, attrList', uid, utils.serialize(attrList))
    local info = arenaMap[uid]
    if info and attrList then
        info.attrList = attrList
        info.isDirty = true
    end
end

--全部获取
function impl.getRidingAloneDefenseData(uid, heroPower)
    -- print('impl.getRidingAloneDefenseData...uid, heroPower', uid, heroPower)

    local test = {
        [1001] = { preId = 0,    id=1001, powerMinPct = 60, powerMaxPct = 70 },
        [1002] = { preId = 1001, id=1002, powerMinPct = 80, powerMaxPct = 90 },
        [1003] = { preId = 1002, id=1003, powerMinPct = 100, powerMaxPct = 110 },
        [1004] = { preId = 1003, id=1004, powerMinPct = 111, powerMaxPct = 120 },
        [1005] = { preId = 1004, id=1005, powerMinPct = 121, powerMaxPct = 130 },
        [1006] = { preId = 1005, id=1006, powerMinPct = 131, powerMaxPct = 140 },
        [1007] = { preId = 1006, id=1007, powerMinPct = 141, powerMaxPct = 150 },
        [1008] = { preId = 1007, id=1008, powerMinPct = 151, powerMaxPct = 160 },
        [1009] = { preId = 1008, id=1009, powerMinPct = 161, powerMaxPct = 170 },
        [1010] = { preId = 1009, id=1010, powerMinPct = 171, powerMaxPct = 180 },
        [1011] = { preId = 1010, id=1011, powerMinPct = 181, powerMaxPct = 190 },
        [1012] = { preId = 1011, id=1012, powerMinPct = 191, powerMaxPct = 200 },
        [1013] = { preId = 1012, id=1013, powerMinPct = 201, powerMaxPct = 210 },
        [1014] = { preId = 1013, id=1014, powerMinPct = 211, powerMaxPct = 220 },
        [1015] = { preId = 1014, id=1014, powerMinPct = 221, powerMaxPct = 230 },
    }
    local t1 = os.clock()

    local firstSectionId = 0
    local updateList = {}
    local findList = {}
    findList[uid] = 1

    local function search(value)
        local left = 1;
        local right = #powerSort
        local mid = math.ceil((left + right)/2)
        while left ~= mid do
            local midHeroPower = 0
            if powerSort[mid] then
                midHeroPower = powerSort[mid].heroPower or 0
            end
            -- print('search....mid, power, midHeroPower', mid, value, midHeroPower)
            if midHeroPower == value then
                break;
            elseif midHeroPower < value then
                left = mid + 1
            else
                right = mid -1
            end

            mid = math.ceil((left + right)/2)
        end
        return mid
    end

    -- for _, rid in pairs(t.ridingAlone.data) do
    for _, rid in pairs(test) do
        --test=========
        local t31 = os.clock()
        local tempMin = math.floor(heroPower * rid.powerMinPct/100)
        local tempMax = math.ceil(heroPower * rid.powerMaxPct/100)

        --test
        -- local otherUid, list = M.getRidingAloneDefenseData3(uid, tempMin, tempMax, findList)
        -- updateList[rid.id] = list
        -- findList[otherUid] = 1

        local tempMinIndex = search(tempMin)
        local tempMaxIndex = search(tempMax)
        if tempMinIndex > tempMaxIndex then
            local swapValue = tempMinIndex
            tempMinIndex = tempMaxIndex
            tempMaxIndex = swapValue
        end
        -- print('==============sectionId, tempMinIndex, tempMaxIndex, myPower, minPower, maxPower', rid.id, tempMinIndex, tempMaxIndex, heroPower, tempMin, tempMax)

        local otherUid = 0
        local bFind = false
        local findCount = 0
        repeat
            local index = utils.getRandomNum(tempMinIndex, tempMaxIndex+1)
            local v = powerSort[index]
            if v and not findList[v.uid] then
                otherUid = v.uid
                bFind = true
            end
            findCount = findCount + 1
            if not bFind and findCount >= 10 then
                bFind = true
                local intMin, intMax = tempMinIndex - 20, tempMaxIndex + 20
                if intMin <= 0 then
                    intMin = 1
                end
                if intMax > #powerSort then
                    intMax = #powerSort
                end
                -- print('============findCount, sectionId, tempMinIndex, tempMaxIndex, intMin, intMax', findCount, rid.id, tempMinIndex, tempMaxIndex, intMin, intMax)
                local tempList = {}
                for i=intMin, intMax do
                    local v = powerSort[i]
                    if v and not findList[v.uid] then
                        -- print('index, uid, heroPower', i, v.uid, v.heroPower)
                        table.insert(tempList, v.uid)
                    end
                end
                if #tempList then
                    index = utils.getRandomNum(1, #tempList+1)
                    otherUid = tempList[index]
                end
            end
        until bFind

        if otherUid and otherUid > 0 then
            local info = powerMap[otherUid]
            if info then
                findList[otherUid] = 1

                local heroList = {}
                for _, v in pairs(info.heroList) do
                    local tempSkill = {}
                    for slot, v in pairs(v.skill) do
                        tempSkill[slot] = { tplId = v.tplId, itemId = v.itemId, type = v.type, level = v.level, exp = v.exp }
                    end
                    local tempTreasure = {}
                    for slot, v in pairs(v.treasure) do
                        tempTreasure[slot] = { tplId = v.tplId, type = v.type, level = v.level, exp = v.exp }
                    end

                    heroList[v.id] = {
                        heroId = v.id,
                        level = v.level,
                        star = v.star,
                        soulLevel = v.soulLevel,
                        hp = v.hp,
                        attack = v.attack,
                        defense = v.defense,
                        strategy = v.strategy,
                        speed = v.speed,
                        leadership = v.leadership,
                        rage = v.rage,
                        challenge = v.challenge,
                        intellect = v.intellect,
                        skill = tempSkill,
                        equip = {},
                        treasure = tempTreasure,
                        additionList = {},

                        singleHp = v.singleHp,     --武将单挑剩余血量
                        singleRage = v.singleRage,   --武将单挑剩余怒气

                        armyType = v.armyType,
                        armyLevel = v.armyLevel,  --TODO:根据玩家开放兵种等级的平均值设定
                        armyCount = v.armyCount,
                        armyRage = v.armyRage,    --士兵剩余怒气
                        position = v.position,
                        }
                end
                updateList[rid.id] = {
                    state = p.RidingAloneState.INIT,  --0初始 1准备 2战斗 3死亡
                    uid = info.uid,
                    nickname = info.nickname,
                    level = info.level,
                    headId = info.headId,
                    heroPower = info.heroPower,
                    heroList = heroList,
                    attrList = {},
                    }
                -- print('get==================sectionId, uid, heroPower', rid.id, info.uid, info.heroPower)
            end
        end

        local t32 = os.clock()
        -- print('=================================sub', t32-t31)
    end

    local t2 = os.clock()
    print('=================clock2-clock1', t2-t1)

    -- print('impl.getRidingAloneDefenseData...updateList', utils.serialize(updateList))
    return updateList
end

--单个获取 根据当前自己最高战力获取 可以防止数据不匹配
function impl.getRidingAloneDefenseData3(uid, minPower, maxPower, findList)
    -- print('getRidingAloneDefenseData3....uid, minPower, maxPower, findList', uid, minPower, maxPower, utils.serialize(findList))
    local function search(value)
        local left = 1;
        local right = #powerSort
        local mid = math.ceil((left + right)/2)
        while left ~= mid do
            local midHeroPower = 0
            if powerSort[mid] then
                midHeroPower = powerSort[mid].heroPower or 0
            end
            -- print('search....mid, power, midHeroPower', mid, value, midHeroPower)
            if midHeroPower == value then
                break;
            elseif midHeroPower < value then
                left = mid + 1
            else
                right = mid -1
            end

            mid = math.ceil((left + right)/2)
        end
        return mid
    end

    local tempMinIndex = search(minPower)
    local tempMaxIndex = search(maxPower)
    if tempMinIndex > tempMaxIndex then
        local swapValue = tempMinIndex
        tempMinIndex = tempMaxIndex
        tempMaxIndex = swapValue
    end
    -- print('==============sectionId, tempMinIndex, tempMaxIndex, myPower, minPower, maxPower', rid.id, tempMinIndex, tempMaxIndex, heroPower, tempMin, tempMax)

    local otherUid = 0
    local bFind = false
    local findCount = 0
    repeat
        local index = utils.getRandomNum(tempMinIndex, tempMaxIndex+1)
        local v = powerSort[index]
        if v and not findList[v.uid] then
            otherUid = v.uid
            bFind = true
        end
        findCount = findCount + 1
        if not bFind and findCount >= 10 then
            bFind = true
            local intMin, intMax = tempMinIndex - 20, tempMaxIndex + 20
            if intMin <= 0 then
                intMin = 1
            end
            if intMax > #powerSort then
                intMax = #powerSort
            end
            -- print('============findCount, sectionId, tempMinIndex, tempMaxIndex, intMin, intMax', findCount, rid.id, tempMinIndex, tempMaxIndex, intMin, intMax)
            local tempList = {}
            for i=intMin, intMax do
                local v = powerSort[i]
                if v and not findList[v.uid] then
                    -- print('index, uid, heroPower', i, v.uid, v.heroPower)
                    table.insert(tempList, v.uid)
                end
            end
            if #tempList then
                index = utils.getRandomNum(1, #tempList+1)
                otherUid = tempList[index]
            end
        end
    until bFind

    local updateList = {}
    if otherUid > 0 then
        local info = powerMap[otherUid]
        if info then
            local heroList = {}
            for _, v in pairs(info.heroList) do
                local tempSkill = {}
                for slot, v in pairs(v.skill) do
                    tempSkill[slot] = { tplId = v.tplId, itemId = v.itemId, type = v.type, level = v.level, exp = v.exp }
                end
                local tempEquip = {}
                for slot, v in pairs(v.equip) do
                    local succinct = {}
                    local inlay = {}
                    for k, val in pairs(v.succinct) do
                        succinct[k] = val
                    end
                    for k, val in pairs(v.inlay) do
                        inlay[k] = val
                    end
                    tempEquip[slot] = { tplId = v.tplId, type = v.type, level = v.level, exp = v.exp, succinct = succinct, inlay = inlay }
                end
                local tempTreasure = {}
                for slot, v in pairs(v.treasure) do
                    tempTreasure[slot] = { tplId = v.tplId, type = v.type, level = v.level, exp = v.exp }
                end

                heroList[v.id] = {
                    heroId = v.id,
                    level = v.level,
                    star = v.star,
                    soulLevel = v.soulLevel,
                    hp = v.hp,
                    attack = v.attack,
                    defense = v.defense,
                    strategy = v.strategy,
                    speed = v.speed,
                    leadership = v.leadership,
                    rage = v.rage,
                    challenge = v.challenge,
                    intellect = v.intellect,
                    skill = tempSkill,
                    equip = tempEquip,
                    treasure = tempTreasure,
                    additionList = {},

                    singleHp = v.singleHp,     --武将单挑剩余血量
                    singleRage = v.singleRage,   --武将单挑剩余怒气

                    armyType = v.armyType,
                    armyLevel = v.armyLevel,  --TODO:根据玩家开放兵种等级的平均值设定
                    armyCount = v.armyCount,
                    armyRage = v.armyRage,    --士兵剩余怒气
                    position = v.position,
                    }
            end
            updateList = {
                state = p.RidingAloneState.INIT,  --0初始 1准备 2战斗 3死亡
                uid = info.uid,
                nickname = info.nickname,
                level = info.level,
                headId = info.headId,
                heroPower = info.heroPower,
                heroList = heroList,
                attrList = {},
                }
            -- print('get==================sectionId, uid, heroPower', rid.id, info.uid, info.heroPower)
        end
    end
    return otherUid, updateList
end

--[[
--TODO:实时 不建议使用
function impl.getRidingAloneDefenseData2(uid, heroPower)
    print('impl.getRidingAloneDefenseData...uid, heroPower', uid, heroPower)

    local test = {
        [1001] = { preId = 0,    id=1001, powerMinPct = 60, powerMaxPct = 70 },
        [1002] = { preId = 1001, id=1002, powerMinPct = 80, powerMaxPct = 90 },
        [1003] = { preId = 1002, id=1003, powerMinPct = 100, powerMaxPct = 110 },
        [1004] = { preId = 1003, id=1004, powerMinPct = 111, powerMaxPct = 120 },
        [1005] = { preId = 1004, id=1005, powerMinPct = 121, powerMaxPct = 130 },
        [1006] = { preId = 1005, id=1006, powerMinPct = 131, powerMaxPct = 140 },
        [1007] = { preId = 1006, id=1007, powerMinPct = 141, powerMaxPct = 150 },
        [1008] = { preId = 1007, id=1008, powerMinPct = 151, powerMaxPct = 160 },
        [1009] = { preId = 1008, id=1009, powerMinPct = 161, powerMaxPct = 170 },
        [1010] = { preId = 1009, id=1010, powerMinPct = 171, powerMaxPct = 180 },
        [1011] = { preId = 1010, id=1011, powerMinPct = 181, powerMaxPct = 190 },
        [1012] = { preId = 1011, id=1012, powerMinPct = 191, powerMaxPct = 200 },
        [1013] = { preId = 1012, id=1013, powerMinPct = 201, powerMaxPct = 210 },
        [1014] = { preId = 1013, id=1014, powerMinPct = 211, powerMaxPct = 220 },
        [1015] = { preId = 1014, id=1014, powerMinPct = 221, powerMaxPct = 230 },
    }
    local t1 = os.clock()

    local sortList = {}
    for k, v in pairs(arenaMap) do
        table.insert(sortList, { uid = k, heroPower = v.heroPower })
    end
    table.sort(sortList, function(left,right)
        return left.heroPower < right.heroPower
    end)
    local t6 = os.clock()
    print('---------------sort sub0, #sortList', t6-t1, #sortList)
    t1 = os.clock()

    local firstSectionId = 0
    local updateList = {}
    local findList = {}
    findList[uid] = 1

    local function search(value)
        local left = 1;
        local right = #sortList
        local mid = math.ceil((left + right)/2)
        while left ~= mid do
            local midHeroPower = 0
            if sortList[mid] then
                midHeroPower = sortList[mid].heroPower or 0
            end
            -- print('search....mid, power, midHeroPower', mid, value, midHeroPower)
            if midHeroPower == value then
                break;
            elseif midHeroPower < value then
                left = mid + 1
            else
                right = mid -1
            end

            mid = math.ceil((left + right)/2)
        end
        return mid
    end

    local function get(tempUid, sectionId)
        local info = arenaMap[tempUid]
        if info then
            findList[tempUid] = 1
            --羁绊
            local heroIdList = {}
            for _, v in pairs(info.heroList) do
                table.insert(heroIdList, v.id)
            end
            local fetterAttrList = checkHeroFetter(heroIdList)

            local heroList = {}
            local positionAssigned = {}
            for _, v in pairs(info.heroList) do
                local skill = v:getHeroSkill(false)
                local equip = v:getHeroEquip()
                local treasure = v:getHeroTreasure()
                local additionList = v:getHeroAdditionList()
                --士兵
                local armyType = t.getArmyTypeByHero(v.id)
                local heroBaseList = {
                    hp = v.hp,
                    attack = v.attack,
                    defense = v.defense,
                    strategy = v.strategy,
                    speed = v.speed,
                    leadership = v.leadership,
                    rage = v.rage,
                    challenge = v.challenge,
                    intellect = v.intellect,
                }
                local hp, attack, defense, strategy, speed, leadership, rage, challenge, intellect = getHeroProperty(heroBaseList, additionList, fetterAttrList, info.attrList)
                --站位
                local position = 0
                local armyTpl = t.findArmyTpl(armyType, 1)
                if armyTpl then
                    for _, v in ipairs(armyTpl.priority) do
                        if positionAssigned[v] == nil then
                            position = v
                            positionAssigned[v] = 1
                            break
                        end
                    end
                end

                heroList[v.id] = {
                    heroId = v.id,
                    level = v.level,
                    star = v.star,
                    hp = v.hp,
                    attack = v.attack,
                    defense = v.defense,
                    strategy = v.strategy,
                    speed = v.speed,
                    leadership = v.leadership,
                    rage = v.rage,
                    challenge = v.challenge,
                    intellect = v.intellect,
                    skill = skill,
                    equip = equip,
                    treasure = treasure,
                    additionList = additionList,

                    singleHp = hp,     --武将单挑剩余血量
                    singleRage = 0,   --武将单挑剩余怒气

                    armyType = armyType,
                    armyLevel = 1,  --TODO:根据玩家开放兵种等级的平均值设定
                    armyCount = leadership,
                    armyRage = 0,    --士兵剩余怒气
                    position = position,
                    }
            end
            updateList[sectionId] = {
                state = p.RidingAloneState.INIT,  --0初始 1准备 2战斗 3死亡
                uid = info.uid,
                nickname = info.nickname,
                level = info.level,
                headId = info.headId,
                heroPower = info.heroPower,
                heroList = heroList,
                attrList = info.attrList,
                }
            print('get==================sectionId, uid, heroPower', sectionId, info.uid, info.heroPower)
        end
    end

    -- for _, rid in pairs(t.ridingAlone.data) do
    for _, rid in pairs(test) do
        --test===========
        local t31 = os.clock()


        if rid.preId == 0 then
            firstSectionId = rid.id
        end
        local tempMin = math.floor(heroPower * rid.powerMinPct/100)
        local tempMax = math.ceil(heroPower * rid.powerMaxPct/100)
        local tempList = {}
        local tempMinIndex = search(tempMin)
        local tempMaxIndex = search(tempMax)
        if tempMinIndex > tempMaxIndex then
            local swapValue = tempMinIndex
            tempMinIndex = tempMaxIndex
            tempMaxIndex = swapValue
        end
        -- print('==============sectionId, tempMinIndex, tempMaxIndex, myPower, minPower, maxPower', rid.id, tempMinIndex, tempMaxIndex, heroPower, tempMin, tempMax)

        local otherUid = 0
        local bFind = false
        local findCount = 0
        repeat
            local index = utils.getRandomNum(tempMinIndex, tempMaxIndex+1)
            local v = sortList[index]
            if v and not findList[v.uid] then
                otherUid = v.uid
                bFind = true
            end
            findCount = findCount + 1
            if not bFind and findCount >= 10 then
                bFind = true
                local intMin, intMax = tempMinIndex - 20, tempMaxIndex + 20
                if intMin <= 0 then
                    intMin = 1
                end
                if intMax > #sortList then
                    intMax = #sortList
                end
                for i=intMax, intMin, -1 do
                    local v = sortList[i]
                    if v and not findList[v.uid] then
                        otherUid = v.uid
                        bFind = true
                        break
                    end
                end
            end
        until bFind

        if otherUid > 0 then
            get(otherUid, rid.id)
            local t33 = os.clock()
            -- print('get ===========otherUid, sub2', otherUid, t33-t31)
        end
    end

    local t2 = os.clock()
    -- print('=================clock2-clock1', t2-t1)

    -- print('impl.getRidingAloneDefenseData...firstSectionId, updateList', firstSectionId, utils.serialize(updateList))
    return firstSectionId, updateList
end
--]]

return M
