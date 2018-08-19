local agent = ...
local user = agent.user
local cluster = require('cluster')
local p = require('protocol')
local t = require('tploader')
local event = require('libs/event')
local timer = require('timer')
local misc = require('libs/misc')
local utils = require('utils')
local arenaStub = require('stub/arena')

local user = agent.user
local dict = agent.dict
local cdlist = agent.cdlist
local hero = agent.hero

local ridingAlone = {
    toDefense = {}, --[sectionId]
    myHero = {},    --[heroId]
    employHero = {},--[uid][heroId]

    flag = 0,   --0已经刷新数据,战斗数据不用处理; >0数据没有刷新,保存新的战斗数据. 准备完战斗数据后+1
    resetCount = 0, --已重置次数
    curSectionId = 0,
}

--local pktHandlers = {}

function ridingAlone.onInit()
    --agent.registerHandlers(pktHandlers)
    ridingAlone.dbLoad()

    --0点刷新
    if cdlist.isHour0Refresh then
        if not user.newPlayer then
            ridingAlone.refresh(false)
        end
    end
    cdlist.evtHour0Refresh:attachRaw(function()
        ridingAlone.refresh(true)
    end)

    --test
    ridingAlone.refresh(false)
end

function ridingAlone.onAllCompInit()
    ridingAlone.sendRidingAloneUpdate()
end

function ridingAlone.onSave()
    ridingAlone.dbSave()
end

function ridingAlone.onClose()
    ridingAlone.dbSave()
end

function ridingAlone.dbLoad()
    local data = dict.get("ridingAlone.datas")
    if data then
        ridingAlone.toDefense = data.toDefense or ridingAlone.toDefense
        ridingAlone.myHero = data.myHero or ridingAlone.myHero
        ridingAlone.employHero = data.employHero or ridingAlone.employHero
        ridingAlone.curSectionId = data.curSectionId or ridingAlone.curSectionId
        ridingAlone.resetCount = data.resetCount or ridingAlone.resetCount
    end
end

function ridingAlone.dbSave()
    dict.set("ridingAlone.datas", {
        toDefense = ridingAlone.toDefense,
        myHero = ridingAlone.myHero,
        employHero = ridingAlone.employHero,
        curSectionId = ridingAlone.curSectionId,
        resetCount = ridingAlone.resetCount,
        })
end

function ridingAlone.refresh(bSendUpdate)
    agent.queueJob(function()
        local heroPower = hero.getTotalTop5HeroPower()
        local firstSectionId = t.ridingAlone.firstId or 0
        local list = arenaStub.call_getRidingAloneDefenseData(user.uid, heroPower)

        -- local test = {11679, 10340, 10177, 9889, 9749, 9739, 9050}
        -- for i=1,#test do
        --     list = arenaStub.call_getRidingAloneDefenseData(user.uid, test[i])
        -- end
        if firstSectionId > 0 and list then
            ridingAlone.curSectionId = firstSectionId
            ridingAlone.toDefense = list
        end
        if bSendUpdate then
            ridingAlone.sendRidingAloneUpdate()
        end
    end)
end

--TODO:
function ridingAlone.getArmyAverageLevel()
    local level = 1
    return level
end

function ridingAlone.onCombatOver(battleId, battleType, isWin, result, dataOnCombatOver)
end

function ridingAlone.sendRidingAloneUpdate(myHeroList)
    -- print('ridingAlone.sendRidingAloneUpdate...')
    local data = ridingAlone.toDefense[curSectionId]
    if data then
        --TODO:
        local armyLevel = 1
        if data.state == p.RidingAloneState.INIT then
            armyLevel = ridingAlone.getArmyAverageLevel()
        end
        local toDefenseList = {}
        for _, v in pairs(data.heroList) do
            local isDie = false
            if v.armyCount <= 0 then
                isDie = true
            end
            table.insert(toDefenseList, {
                isDie = isDie,
                heroId = v.heroId,
                heroLevel = v.level,
                heroStar = v.star,
                armyType = v.armyType,
                armyLevel = v.armyLevel,
                armyCount = v.armyCount,
                position = v.position,
                })
        end

        if myHeroList == nil then
            myHeroList = {}
            for _, v in pairs(ridingAlone.myHero) do
                local isDie = false
                if v.armyCount <= 0 then
                    isDie = true
                end
                table.insert(myHeroList, {
                    isDie = isDie,
                    ownUid = user.uid,
                    heroId = v.heroId,
                    armyType = v.armyType,
                    armyCount = v.armyCount,
                    })
            end
            for ownUid, employ in pairs(ridingAlone.employHero) do
                for _, v in pairs(employ) do
                    local isDie = false
                    if v.armyCount <= 0 then
                        isDie = true
                    end
                    table.insert(myHeroList, {
                        isDie = isDie,
                        ownUid = ownUid,
                        heroId = v.heroId,
                        armyType = v.armyType,
                        armyCount = v.armyCount,
                        })
                end
            end
        end

        agent.sendPktout(p.SC_RIDING_ALONE_UPDATE, '@@1=i,2=i,3=s,4=i,5=i,6=i,7=[isDie=b,heroId=i,heroLevel=i,heroStar=i,armyType=i,armyLevel=i,armyCount=i,position=i],8=[isDie=b,ownUid=i,heroId=i,armyType=i,armyCount=i]',
            curSectionId, data.uid, data.nickname, data.level, data.headId, data.heroPower, toDefenseList, myHeroList)
    end
end

function ridingAlone.cs_riding_alone_fight(sectionId , size)
    if size <= 0 then
        print('p.CS_RIDING_ALONE_FIGHT...size=', size)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local armyList = {}
    for i=1, size do
        local ownUid, heroId, armyType, armyCount, position = pktin:read('iiiii')
        -- print('p.CS_RIDING_ALONE_FIGHT...ownUid, heroId, armyType, armyCount, position', ownUid, heroId, armyType, armyCount, position)
        if heroId > 0 and t.hero[heroId] then
            local level = agent.army.getArmyLevelByArmyType(armyType)
            armyList[heroId] = { heroId = heroId, armyType = armyType, level = level, count = armyCount, position = position }
        end
    end
    if not next(armyList) then
        -- print('p.CS_RIDING_ALONE_FIGHT...armyList is empty')
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
end

return ridingAlone