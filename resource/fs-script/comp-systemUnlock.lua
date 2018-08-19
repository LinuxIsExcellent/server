local agent = ...
local p = require('protocol')
local t = require('tploader')
local utils = require('utils')
 local allianceStub = require('stub/alliance')
local alliance = agent.alliance

local user = agent.user
local building = agent.building
local hero = agent.hero
local technology = agent.technology

local systemUnlock = {}

function systemUnlock.onInit()
end

function systemUnlock.onAllCompInit()
end

function systemUnlock.onClose()
end

function systemUnlock.checkUnlockedByList(unLockList, inputParam)
    -- print('systemUnlock.checkUnlockedByList....unLockList, inputParam', utils.serialize(unLockList), inputParam)
    if unLockList == nil  then
        return true
    end
    for _, unLockId in pairs(unLockList) do
        -- print('systemUnlock.checkUnlockedByList....unLockId', unLockId)
        if not systemUnlock.checkUnlockedById(unLockId, inputParam) then
            return false
        end
    end

    return true
end

function systemUnlock.checkUnlockedById(unLockId, inputParam)
    -- print('systemUnlock.checkUnlockedById...unLockId, inputParam', unLockId, inputParam)
    local tpl = t.systemUnlock[unLockId]
    if tpl then
        --1 建筑等级 {建筑tplId,等级}
        if tpl.type == p.SystemUnlockType.BUILDING_LEVEL then
            local tplId = tpl.condValue[1] or 0
            local level = tpl.condValue[2] or 0
            local buildingLevel = building.getBuildingLevel(tplId)
            -- print('systemUnlock.checkUnlockedById...type, tplId, level, buildingLevel', tpl.type, tplId, level, buildingLevel)
            return buildingLevel >= level
        -- --2 武将等级{等级}
        -- elseif tpl.type == p.SystemUnlockType.HERO_LEVEL then
        --     local heroLevel = inputParam or 0
        --     local level = tpl.condValue[1] or 0
        --     return heroLevel >= level
        -- --3 VIP等级{等级}
        -- elseif tpl.type == p.SystemUnlockType.VIP_LEVEL then
        --     local needVipLevel = tpl.condValue[1] or 0
        --     local vipLevel = agent.vip.vipLevel()
        --     return vipLevel >= needVipLevel
        --4 任务限制{任务ID}
        elseif tpl.type == p.SystemUnlockType.TASK_LIMIT then
            local questId = tpl.condValue[1] or 0
            return quest.isQuestFinish(questId, p.QuestType.NORMAL)
        --5 成就限制{成就ID}
        elseif tpl.type == p.SystemUnlockType.ACHIEVEMENT_LIMIT then
            local achievementId = tpl.condValue[1] or 0
            return agent.achievement.isFinish(achievementId)
        --6 科技限制{科技tplId,等级}
        elseif tpl.type == p.SystemUnlockType.TECHNOLOGY_LIMIT then
            local tplId = tpl.condValue[1] or 0
            local level = tpl.condValue[2] or 0
            local techLevel = technology.getLevel(tplId)
            -- print('systemUnlock.checkUnlockedById...type, tplId, level, techLevel', tpl.type, tplId, level, techLevel)
            return techLevel >= level
        --7 城主等级 {城主等级}
        elseif tpl.type == p.SystemUnlockType.USER_LEVEL then
            local level = tpl.condValue[1] or 0
            -- print('systemUnlock.checkUnlockedById...type, level, userLevel', tpl.type, level, user.info.level)
            return user.info.level >= level
        --8 头像解锁{武将ID,武将等级，武将星级}
        elseif tpl.type == p.SystemUnlockType.USER_HEAD then
            local heroId = tpl.condValue[1] or 0
            local level = tpl.condValue[2] or 0
            local star = tpl.condValue[3] or 0
            local heroLevel, heroStar = hero.getHeroLevelAndStar(heroId)
            -- print('systemUnlock.checkUnlockedById....type, heroId, level, star, heroLevel, heroStar', tpl.type, heroId, level, star, heroLevel, heroStar)
            return heroLevel >= level and heroStar >= star
        --9 武将星级 {武将星级}
        elseif tpl.type == p.SystemUnlockType.HERO_STAR then
            local heroStar = inputParam or 0
            local star = tpl.condValue[1] or 0
            -- print('systemUnlock.checkUnlockedById...type, star, heroStar', tpl.type, star, heroStar)
            return heroStar >= star
        --10 装备等级 {装备等级}
        elseif tpl.type == p.SystemUnlockType.EQUIP_LEVEL then
            local equipLevel = inputParam or 0
            local level = tpl.condValue[1] or 0
            -- print('systemUnlock.checkUnlockedById....type, level, equipLevel', tpl.type, level, equipLevel)
            return equipLevel >= level
        --11 关卡限制{关卡ID}
        elseif tpl.type == p.SystemUnlockType.SECTION_LIMIT then
            local sectionId = tpl.condValue[1] or 0
            local sec = agent.scenarioCopy.getSection(sectionId)
            if sec == nil then
                return false
            end
            return true
        --12 武将数量{类型，数量}
        elseif tpl.type == p.SystemUnlockType.HERO_NUMS then
            local quality = tpl.condValue[1] or 0
            local needNums = tpl.condValue[2] or 0
            local myNums = hero.getHeroCountByQuality(quality)
            return myNums >= needNums
        --13 获得名城 {兵种，名城}
        elseif tpl.type == p.SystemUnlockType.OWN_CITY then
            local famousCity = tpl.condValue[1] or 0 
            local kind = inputParam or 0 
            local aid = alliance.allianceId()
            -- print("aid is ", aid)
            local cities = allianceStub.allAllianceCity[aid]
            if cities == nil then 
                -- print("cities is not nil")
                return false
            end
            local ret = false
            for _, v in ipairs(cities) do
                if v == famousCity then
                    local mapCityTpl = t.mapCity[v]
                    local armsId = mapCityTpl.ArmsId
                    if armsId == kind then
                        ret = true
                        break
                    end
                end
            end
            return ret
        else
            -- print('unknown unlock type, unLockId, type=', unLockId, tpl.type)
            return false
        end
    end
    return true
end

return systemUnlock