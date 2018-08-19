local agent = ...
local p = require('protocol')
local utils = require('utils')
local t = require('tploader')
local event = require('libs/event')
local user = agent.user
local army = agent.army
local hero = agent.hero
local pubHero = require('hero')
local property = agent.property

local combat = {
    --events
    evtCombatOver = event.new(), --()
}

local battleDataCache = {}
local maxBattleId = 1


function combat.onInit()

end

function combat.onAllCompInit()
end

function combat.onReady()
end

function combat.getMyHeroProperty(heroId, armyType, armyLevel)
    local property = agent.property
    local heroInfo = hero.info[heroId]
    local additionList = heroInfo.additionList
    local secondAdditionList = heroInfo.secondAttrAddtionList
    return pubHero.heroInfo:getHeroProperty(additionList, secondAdditionList, armyType, armyLevel, property.getHeroProp())
end

function combat.getMyArmyProperty(armyType, armyLevel)
    return combat.getOtherArmyProperty(armyType,armyLevel, property.getArmyProp())
end

function combat.getOtherArmyProperty(armyType, armyLevel, propAttrList)
    local hp, attack, defense, speed = 0, 0, 0, 0
    local armyTpl = t.findArmysTpl(armyType, armyLevel)

    if propAttrList == nil then
        propAttrList = {}
    end

    if armyTpl then
        local function getAttrAdd( at )
            local base = 0
            local pct = 0
            local ext = 0
            --全局属性
            for attributeType, attr in pairs(propAttrList) do
                if at == attributeType then
                    local tempBase = attr[p.AttributeAdditionType.BASE] or 0
                    local tempPct = attr[p.AttributeAdditionType.PCT] or 0
                    local tempExt = attr[p.AttributeAdditionType.EXT] or 0
                    base = base + tempBase
                    pct = pct + tempPct
                    ext = ext + tempExt
                end
            end
            pct = pct/10000

            return base,pct,ext
        end

        -- print(" armyTpl ---------------- ")
        -- print(" traceback  ---  ", debug.traceback())
        --var_dump(armyTpl)

        local base,pct,ext = getAttrAdd(p.AttributeType.TROOPS_HEALTH)
        -- hp = math.floor((armyTpl.hp+base) * (1+pct) + ext)

        -- base,pct,ext = getAttrAdd(p.AttributeType.TROOPS_ATTACK)
        -- --attack = math.floor((armyTpl.attack+base) * (1+pct) + ext) 
        -- attack = math.floor((armyTpl.nJobPower+base) * (1+pct) + ext) 
        -- --attack = 1

        -- base,pct,ext = getAttrAdd(p.AttributeType.TROOPS_DEFENSE)
        -- defense = math.floor((armyTpl.nJobDefense+base) * (1+pct) + ext)

        -- base,pct,ext = getAttrAdd(p.AttributeType.TROOPS_SPEED)
        -- --speed = math.floor((armyTpl.speed+base) * (1+pct) + ext)
        -- speed = math.floor((armyTpl.nJobAgile+base) * (1+pct) + ext)
    end
    return hp, attack, defense, speed
end

--return battleId
function combat.createBattle( battleType, teamId,  npcArmyGroup, npcArmyCount, dataOnCombatOver, extParam)
    local armyList = army.getTeamArmyListByTeam(teamId)
    print('combat.createBattle ')
    if armyList == nil then
        return 0
    end

    if extParam == nil then
        extParam = {}
    end

    return combat.createBattleByArmyList(battleType, armyList,  npcArmyGroup, npcArmyCount, dataOnCombatOver, extParam)
end

function combat.createBattleByArmyList(battleType, armyList,  npcArmyGroup, npcArmyCount, dataOnCombatOver, extParam)
    if extParam == nil then
        extParam = {}
    end

    -- print('combat.createBattleByArmyList..battleType, armyList,  npcArmyGroup, npcArmyCount, dataOnCombatOver, extParam', 
    --     battleType, utils.serialize(armyList),  npcArmyGroup, npcArmyCount, utils.serialize(dataOnCombatOver), 
    --     utils.serialize(extParam))

    --在这里加入战斗双方的数据
    print("---- hero --------")
    local attackInput = combat.createPlayerBattleData(armyList, extParam)
    print("attackInput", utils.serialize(attackInput))
    -- var_dump(attackInput)
    print("---- npc --------- ")
    -- print("extParam", utils.serialize(extParam))
    -- var_dump(extParam)
    local defenceInput = combat.createNPCBattleData(npcArmyGroup, npcArmyCount, extParam)

    if attackInput == nil or defenceInput == nil then
        return 0
    end

    local randSeed = utils.getRandomNum(1, 80000000)
    -- randSeed = 13501226
    local battleId = maxBattleId
    maxBattleId = maxBattleId + 1
    local battleData = {}
    battleData.randSeed = randSeed
    battleData.battleType = battleType
    battleData.levelType = extParam.levelType
    -- print('combat.createBattleByArmyList...battleData', var_dump(battleData))
    if extParam.singleCombat ~= nil then
        battleData.singleCombat = extParam.singleCombat
    else
        battleData.singleCombat = 0
    end
    -- 把战斗双方的数据加入到battleData中来
    battleData.attack = attackInput
    battleData.defence = defenceInput
    battleData.dataOnCombatOver = dataOnCombatOver
    battleDataCache[battleId] = battleData
    battleDataCache[battleId - 1] = nil

    print("new hero data.utils.serialize...", utils.serialize(battleData))
    local isWin, attackWinRound, maxRound, beginArmys, endArmys, warReport = agent.startCombat(battleData)
    print("combat.createBattleByArmyList, isWin, attackWinRound, maxRound, beginArmys, endArmys, warReport", isWin, attackWinRound, maxRound, beginArmys, endArmys, utils.serialize(warReport))
    --print("combat.createBattleByArmyList,  battleData.dataOnCombatOver", utils.serialize(battleData.dataOnCombatOver))
    
    combat.evtCombatOver:trigger(battleId, battleData.battleType, isWin == 1, battleData.dataOnCombatOver,attackWinRound, maxRound, beginArmys, endArmys )
  
    return battleId, warReport
end

function combat.createPlayerBattleData(armyList, extParam)
    if not armyList or not next(armyList) then
        return nil
    end

    local heroIdList = {}
    for k, v in pairs(armyList) do
        table.insert(heroIdList,k)
    end

    local tempList = {}
    --指定必须上场的武将
    local heroConf = extParam.heroConf or {}
    if heroConf then
        for _, npcId in pairs(heroConf) do
            local npcArmyTpl = t.mapNpcArmy[npcId]
            if npcArmyTpl then
                tempList[npcArmyTpl.hero[1]] = npcArmyTpl
            end
        end
        -- print('###combat.createPlayerBattleDataByArmyList, tempList..', utils.serialize(tempList))
    end

    --指定必须单挑的武将
    local singleCombatHero = extParam.singleCombatHero or {}
    -- print('combat.createPlayerBattleData...singleCombatHero', utils.serialize(singleCombatHero))
    local npcId = singleCombatHero[1] or 0
    local battleSingleHero = 0
    if t.mapNpcArmy[npcId] then
        battleSingleHero = t.mapNpcArmy[npcId].hero[1] or 0
    end
    -- print('armyList........', var_dump(armyList))
    local heroId = 0
    for k , _ in pairs(armyList) do 
        heroId = k
    end
    if extParam.levelType == 0 then
        battleSingleHero = heroId
    end

    local battleInput = {}
    battleInput.uid = user.uid
    battleInput.headId = user.info.headId
    battleInput.nickname = user.info.nickname
    battleInput.level = user.info.level
    battleInput.singleCombatHero = battleSingleHero
    battleInput.team = {}
    -- 如果是单挑则直接返回
    -- if extParam.levelType == 0 then
    --    return battleInput
    -- end
    -- print('###armyList', utils.serialize(armyList))
    for k, v in pairs(armyList) do
        local group = {}
        local iHero = {}
        iHero.id = k
        local heroInfo = hero.info[k]
        -- print('###heroInfo', utils.serialize(hero.info))
        if heroInfo then
            iHero.level = heroInfo.level
            iHero.star = heroInfo.star

            local attrList = combat.getMyHeroProperty(k, v.armyType, v.level)

            iHero.heroPower = attrList.heroPower
            iHero.heroDefense = attrList.heroDefense
            iHero.heroWisdom = attrList.heroWisdom
            iHero.heroLucky = attrList.heroLucky
            iHero.heroSkill = attrList.heroSkill
            iHero.heroAgile = attrList.heroAgile
            iHero.heroLife = attrList.heroLife
            iHero.heroPhysicalPower = attrList.heroPhysicalPower
            iHero.heroPhysicalDefense = attrList.heroPhysicalDefense
            iHero.heroSkillPower = attrList.heroSkillPower
            iHero.heroSkillDefense = attrList.heroSkillDefense
            iHero.heroHit = attrList.heroHit
            iHero.heroAvoid = attrList.heroAvoid
            iHero.heroCritHit = attrList.heroCritHit
            iHero.heroCritAvoid = attrList.heroCritAvoid
            iHero.heroSpeed = attrList.heroSpeed
            iHero.heroCityLife = attrList.heroCityLife
            iHero.heroFight = attrList.heroFight
            iHero.heroSolohp = attrList.heroSolohp


            local iSkill = {}
            for _, v2 in pairs(heroInfo.skill) do
                table.insert(iSkill, {tplId = v2.tplId, level = v2.level})
            end
            -- table.insert(iSkill, {tplId = 1446001, level = 5})
            -- table.insert(iSkill, {tplId = 1446002, level = 5})
            iHero.skill = iSkill
            -- print('###hero.iSkill', utils.serialize(iSkill))

            group.hero = iHero

            local iArmy = {}
            iArmy.armyType = v.armyType
            iArmy.armyLevel = v.level
            iArmy.armyCount = v.count
            iArmy.armyCountMax = iArmy.armyCount

            -- print("heroId ----- ", v.id , " armyCount+ ", v.count, " leadership_ ", leadership)

            if iArmy.armyCount > iHero.heroFight then
                iArmy.armyCount = iHero.heroFight
                iArmy.armyCountMax = iHero.heroFight
            end

            -- local hp, attack, defense, speed = combat.getMyArmyProperty(iArmy.armyType, iArmy.armyLevel)
            -- iArmy.hp = hp
            -- iArmy.attack = attack
            -- iArmy.defense = defense
            -- iArmy.speed = speed

            group.army = iArmy
            group.position = v.position
            table.insert(battleInput.team, group)
        else
            --玩家没有这个武将
            local npcArmyTpl = tempList[k]
            -- print('###combat.createPlayerBattleDataByArmyList, npcArmyTpl..', utils.serialize(npcArmyTpl))
            local group = combat.initNpcGroupData(npcArmyTpl)
            if group ~= nil then
                group.position = v.position
                table.insert(battleInput.team, group)
            end
        end
    end
    return battleInput
end

--初始化NPCGroup
function combat.initNpcGroupData(npcArmyTpl)
    local group = nil
    if npcArmyTpl ~= nil then
        local iHero = {}
        -- print("npcArmyTpl", utils.serialize(npcArmyTpl))
        iHero.id = npcArmyTpl.hero[1]
        iHero.level = npcArmyTpl.hero[2]
        iHero.star = npcArmyTpl.hero[3]

        local iSkill = {}
        for k,v in pairs(npcArmyTpl.heroSkill) do
            table.insert(iSkill, {tplId = v[1], level = v[2]})
        end
        iHero.skill = iSkill

        local heroInfo = pubHero.newHeroInfo({id = iHero.id, level = iHero.level, star = iHero.star, soulLevel = npcArmyTpl.soul, skill = iSkill})
        if heroInfo ~= nil then
            --print('###combat.initNpcGroupData, heroInfo..', utils.serialize(heroInfo))
            local additionList = heroInfo:getHeroAdditionList()
            local secondAdditionList = heroInfo:getHeroSecondAdditionList()
            -- print('combat.initNpcGroupData....heroBaseList', utils.serialize(heroBaseList))
            -- print('combat.initNpcGroupData....additionList', utils.serialize(additionList))
            local attrList = pubHero.heroInfo:getHeroProperty(additionList,secondAdditionList,npcArmyTpl.army[1],npcArmyTpl.army[2])

            group = {}

            iHero.heroPower = attrList.heroPower
            iHero.heroDefense = attrList.heroDefense
            iHero.heroWisdom = attrList.heroWisdom
            iHero.heroLucky = attrList.heroLucky
            iHero.heroSkill = attrList.heroSkill
            iHero.heroAgile = attrList.heroAgile
            iHero.heroLife = attrList.heroLife
            iHero.heroPhysicalPower = attrList.heroPhysicalPower
            iHero.heroPhysicalDefense = attrList.heroPhysicalDefense
            iHero.heroSkillPower = attrList.heroSkillPower
            iHero.heroSkillDefense = attrList.heroSkillDefense
            iHero.heroHit = attrList.heroHit
            iHero.heroAvoid = attrList.heroAvoid
            iHero.heroCritHit = attrList.heroCritHit
            iHero.heroCritAvoid = attrList.heroCritAvoid
            iHero.heroSpeed = attrList.heroSpeed
            iHero.heroCityLife = attrList.heroCityLife
            iHero.heroFight = attrList.heroFight
            iHero.heroSolohp = attrList.heroSolohp

            group.hero = iHero

            local iArmy = {}
            iArmy.armyType = npcArmyTpl.army[1]
            iArmy.armyLevel = npcArmyTpl.army[2]
            iArmy.armyCount = npcArmyTpl.army[3]
            iArmy.armyCountMax = iArmy.armyCount

            -- if iArmy.armyCount > leadership then
            --     iArmy.armyCount = leadership
            -- end

            local hp, attack, defense, speed = combat.getOtherArmyProperty(iArmy.armyType, iArmy.armyLevel)
            iArmy.hp = hp
            iArmy.attack = attack
            iArmy.defense = defense
            iArmy.speed = speed

            group.army = iArmy
            group.position = npcArmyTpl.hero[4] or 0
        end
    end
    return group
end

function combat.createNPCBattleData(armyGroup, armyCount, extParam)
    local npcArmyGroupTpl = t.mapNpcArmy.groups[armyGroup]
    -- print("----------------------- armyGroup, armyCount, npcArmyGroupTpl -----------------", armyGroup, armyCount, var_dump(npcArmyGroupTpl))
    if npcArmyGroupTpl == nil or armyCount <=0 then
        utils.log("combat.createNPCBattleData armyGroup=%i,armyCount=%i", armyGroup, armyCount)
        return nil
    end

    local singleCombatHero = extParam.singleCombatHero or {}
    -- print('combat.createNPCBattleData...singleCombatHero', utils.serialize(singleCombatHero))
    local npcId = singleCombatHero[2] or 0
    local defenceSingleHero = 0
    -- print("npcId ---------- --------!!! ", npcId)
    if t.mapNpcArmy[npcId] then
        defenceSingleHero = t.mapNpcArmy[npcId].hero[1] or 0
    end


    local battleInput = {}
    battleInput.uid = 0
    battleInput.singleCombatHero = defenceSingleHero or 0
    battleInput.team = {}
    -- 如果是单挑，则不用设置站位 直接找到单挑英雄返回即可
    
    local heroIdList = {}
    for k,v in pairs(npcArmyGroupTpl) do
        local npcArmyTpl = v
        table.insert(heroIdList, npcArmyTpl.hero[1])
    end

    local heroTable = {}
    local count = 0
    for k,v in pairs(npcArmyGroupTpl) do
        if battleInput.headId == nil or battleInput.nickname == nil then
            battleInput.headId = v.headId
            battleInput.nickname = v.name
            battleInput.level = v.level
        end

        local npcArmyTpl = v
        if npcArmyTpl ~= nil then
            if heroTable[npcArmyTpl.hero[1]] ~= 1 then
                heroTable[npcArmyTpl.hero[1]] = 1;
                local group = combat.initNpcGroupData(v)
                if group ~= nil then
                    table.insert(battleInput.team, group)
                    count = count + 1
                    if count == armyCount then
                        break
                    end
                end
            end
        end
    end

    print("begin assign position..", utils.serialize(battleInput.team))
    --设置站位
    local function assignArmyPosition()
        -- 已设置位置的武将
        local positionAssigned = {}
        for g, group in ipairs(battleInput.team) do
            if group.position ~= 0 then
                positionAssigned[group.position] = group.hero.id
            end
        end
        local assignedCount = 0
        local i = 1
        while(i <= 9 and assignedCount < armyCount) do
            for g, group in ipairs(battleInput.team) do
                if group.position == 0 then
                    local armyTpl = t.findArmysTpl(group.army.armyType, group.army.armyLevel)
                    if armyTpl ~= nil then
                        if positionAssigned[armyTpl.priority[i]] == nil then
                            group.position = armyTpl.priority[i]
                            positionAssigned[armyTpl.priority[i]] = group.hero.id
                            assignedCount = assignedCount + 1
                        end
                    end
                end
            end
            i = i + 1
        end
    end

    assignArmyPosition()
    print("end assign position..", utils.serialize(battleInput.team))
    return battleInput
end

function combat.createInitialDataToSend( initialData )
    local dataToSend = {}
    dataToSend.uid = initialData.uid
    dataToSend.headId = initialData.headId or 0
    dataToSend.nickname = initialData.nickname or ''
    dataToSend.level = initialData.level or 0
    dataToSend.singleCombatHero = initialData.singleCombatHero or 0

    dataToSend.team = {}
    dataToSend.trap = {}
    dataToSend.trapAtkPercentage = 0
    dataToSend.turretAtkPower = 0

    for k, v in pairs(initialData.team) do
        local armyGroup = {}
        armyGroup.id = v.hero.id
        armyGroup.tplId = v.hero.id
        armyGroup.level = v.hero.level
        armyGroup.star = v.hero.star
        armyGroup.hp = v.hero.hp
        armyGroup.attack = v.hero.attack
        armyGroup.defense = v.hero.defense
        armyGroup.strategy = v.hero.strategy
        armyGroup.speed = v.hero.speed
        armyGroup.rage = v.hero.rage
        armyGroup.challenge = v.hero.challenge
        armyGroup.intellect = v.hero.intellect

        armyGroup.equip = {}

        armyGroup.treasure = {}

        armyGroup.spell = {}
        for _, v2 in pairs(v.hero.skill) do
            table.insert(armyGroup.spell, {tplId = v2.tplId, level = v2.level})
        end

        armyGroup.armyType = v.army.armyType
        armyGroup.armyLevel = v.army.armyLevel
        armyGroup.armyCount = v.army.armyCount
        armyGroup.armyHp = v.army.hp
        armyGroup.armyAttack = v.army.attack
        armyGroup.armyDefense = v.army.defense
        armyGroup.armySpeed = v.army.speed

        armyGroup.position = v.position
        table.insert(dataToSend.team, armyGroup)
    end
    return dataToSend
end

---------------------------擂台---------------------------------
function combat.createHeroBattle(attackInfo, defenseInfo, battleType, dataOnCombatOver)
    -- print('combat.createHeroBattle...', debug.traceback())
    local attackInput = combat.createHeroCombatData(attackInfo, true)
    local defenceInput = combat.createHeroCombatData(defenseInfo, false)
    if attackInput == nil or defenceInput == nil then
        return 0
    end

    local randSeed = utils.getRandomNum(1, 10000)
    local battleId = maxBattleId
    maxBattleId = maxBattleId + 1
    local battleData = {}
    battleData.battleId = battleId
    battleData.randSeed = randSeed
    battleData.battleType = battleType
    battleData.attack = attackInput
    battleData.defence = defenceInput
    battleData.dataOnCombatOver = dataOnCombatOver
    battleDataCache[battleId] = battleData
    battleDataCache[battleId - 1] = nil

    local isWin, warReport = agent.startHeroCombat(battleData)
    -- print("---battleType,isWin,result,warReport", battleData.battleType,isWin,utils.serialize(result), utils.serialize(warReport))
    combat.evtCombatOver:trigger(battleId, battleData.battleType, isWin == 1, warReport, battleData.dataOnCombatOver)
 
    return isWin, battleId, warReport
end

function combat.createHeroCombatData(attackInfo, isAttack)
    local heroIdList = {}
    for k, hero in pairs(attackInfo.heroList) do
        table.insert(heroIdList,hero.id)
    end

    if #heroIdList == 0 then
        return nil
    end

    if isAttack then
        for _, hero in pairs(attackInfo.heroList) do
            local attrList = combat.getMyHeroProperty(hero.id)
            hero.heroPower = attrList.heroPower
            hero.heroDefense = attrList.heroDefense
            hero.heroWisdom = attrList.heroWisdom
            hero.heroLucky = attrList.heroLucky
            hero.heroSkill = attrList.heroSkill
            hero.heroAgile = attrList.heroAgile
            hero.heroLife = attrList.heroLife
            hero.heroPhysicalPower = attrList.heroPhysicalPower
            hero.heroPhysicalDefense = attrList.heroPhysicalDefense
            hero.heroSkillPower = attrList.heroSkillPower
            hero.heroSkillDefense = attrList.heroSkillDefense
            hero.heroHit = attrList.heroHit
            hero.heroAvoid = attrList.heroAvoid
            hero.heroCritHit = attrList.heroCritHit
            hero.heroCritAvoid = attrList.heroCritAvoid
            hero.heroSpeed = attrList.heroSpeed
            hero.heroCityLife = attrList.heroCityLife
            hero.heroFight = attrList.heroFight
            hero.heroSolohp = attrList.heroSolohp
        end
    else
        for _, hero in pairs(attackInfo.heroList) do
            local attrList = pubHero.heroInfo:getHeroProperty(hero.additionList, hero.secondAdditionList,0,0,attackInfo.attrList)
            hero.heroPower = attrList.heroPower
            hero.heroDefense = attrList.heroDefense
            hero.heroWisdom = attrList.heroWisdom
            hero.heroLucky = attrList.heroLucky
            hero.heroSkill = attrList.heroSkill
            hero.heroAgile = attrList.heroAgile
            hero.heroLife = attrList.heroLife
            hero.heroPhysicalPower = attrList.heroPhysicalPower
            hero.heroPhysicalDefense = attrList.heroPhysicalDefense
            hero.heroSkillPower = attrList.heroSkillPower
            hero.heroSkillDefense = attrList.heroSkillDefense
            hero.heroHit = attrList.heroHit
            hero.heroAvoid = attrList.heroAvoid
            hero.heroCritHit = attrList.heroCritHit
            hero.heroCritAvoid = attrList.heroCritAvoid
            hero.heroSpeed = attrList.heroSpeed
            hero.heroCityLife = attrList.heroCityLife
            hero.heroFight = attrList.heroFight
            hero.heroSolohp = attrList.heroSolohp
        end
    end

    return attackInfo
end

---------------------------------------------------------------

function combat.cs_battle_start(battleId, teamId)

    -- if battleId == 0 then
    --     battleId = combat.createBattle(1, teamId, 5001, 1, {})
    -- end
    if battleId == 0 then
        return
    end
    local battleData = battleDataCache[battleId]
    if battleData == nil then
        return
    end

    --send initialDataInput
    local attackData = combat.createInitialDataToSend(battleData.attack)
    local defenceData = combat.createInitialDataToSend(battleData.defence)
    local initialDataInput = {}
    table.insert(initialDataInput, attackData)
    table.insert(initialDataInput, defenceData)
    print('initialDataInput...', utils.serialize(initialDataInput))

    -- print("battleData", var_dump(battleData))
    -- print("attackData", var_dump(attackData))
    -- print("defenceData", var_dump(defenceData))
    agent.sendPktout(p.SC_BATTLE_START_RT, '@@1=i,2=i,3=i,4=i,5=i,6=i,7=[uid=i,headId=i,nickname=s,level=i,singleCombatHero=i,team=[id=i,tplId=i,level=i,star=i,hp=i,attack=i,defense=i,strategy=i,speed=i,rage=i,challenge=i,intellect=i,equip=[tplId=i],treasure=[tplId=i],spell=[tplId=i,level=i],armyType=i,armyLevel=i,armyCount=i,armyHp=i,armyAttack=i,armyDefense=i,armySpeed=i,position=i],trap=[type=i],trapAtkPercentage=i,turretAtkPower=i],8=i',
        battleId,battleData.battleType,0,0,battleData.randSeed,battleData.singleCombat,initialDataInput, 0)
end

function combat.cs_battle_over(battleId, win, singleReleaseSpells,mixedReleaseSpells)
    print('p.CS_BATTLE_OVER...battleId,win', battleId,win)
    local releaseSpells = {single = singleReleaseSpells, mixed = mixedReleaseSpells}

    local battleData = battleDataCache[battleId]
    -- print('p.CS_BATTLE_OVER...battleData', utils.serialize(battleData))
    if battleData ~= nil then
        print("---battleType,isWin,result,armyHurt---", battleData.battleType,isWin,utils.serialize(result),utils.serialize(armyHurt))
        -- print('battleData', var_dump(battleData))
        local isWin,result,armyHurt = agent.startCombat(battleData)
        --agent.sendPktout(p.SC_BATTLE_OVER_RT, '@@1=i,2=i,3=i,4=[uid=i,group=[heroId=i,armyType=i,dieCount=i]]', battleId,battleData.battleType,isWin,armyHurt)
        agent.sendPktout(p.SC_BATTLE_OVER_RT, '@@1=i,2=i,3=i,4=i', battleId, battleData.battleType, win, 0)
        -- 有些逻辑没测试这里先用客户端返回的结果

        combat.evtCombatOver:trigger(battleId, battleData.battleType, isWin == 1, result, battleData.dataOnCombatOver)
        battleDataCache[battleId] = nil
        print('win, isWin.........', win, isWin)
        if win ~= isWin then
            print("???????战斗结果前后端怎么会不一样？???")
        end
    end
end

return combat
