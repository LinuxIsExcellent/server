local agent = ...
local user = agent.user
local cluster = require('cluster')
local p = require('protocol')
local t = require('tploader')
local event = require('libs/event')
local timer = require('timer')
local misc = require('libs/misc')
local utils = require('utils')
local trie = require('trie')
local logStub = require('stub/log')
local rangeStub = require('stub/range')
local allianceStub = require('stub/alliance')


local hero = agent.hero
local army = agent.army
local building = agent.building
local mail = agent.mail
local bag = agent.bag
local cdlist = agent.cdlist
local buff = agent.buff
local alliance = agent.alliance
local technology = agent.technology
local skill = agent.skill
local dict = agent.dict
local vip = agent.vip
local activity

local info = {
    k = 0,
    x = 0,
    y = 0,
    castleTpl = {},
    sendFirstCastleBattle = false       -- send frist castle battle mail
}

local bookmarkInfo = {
    id = 0,
    name = '',
    type = 1,
    x = 1,
    y = 1,

    sync = false,
}

function bookmarkInfo:new(o)
    o = o or {}
    if o.id == nil then
        o.id = utils.createItemId()
    end
    setmetatable(o, self)
    self.__index = self
    return o
end

local map = {
    info = info,
    mapService = nil,
    ncProperty = {},

    bookmarkList = {},                -- <id, bookmarkInfo>

    --events
    evtAttackMonster = event.new(), --(monsterTplId, isWin)
    evtResourceGather = event.new(), --(resourceType, count)
    evtScout = event.new(), --(isWin, targetTplId)
    evtBeScout = event.new(), --(isWin)
    evtDefenseCastle = event.new(), --(isWin)
    evtTeleport = event.new(), --(x, y, isAdvance)
    evtKillPower = event.new(), --(killPower)
    evtResourceHelp = event.new(),
    evtReinforcements = event.new(),
    evtOccupyCity = event.new(), --(tplId)
    evtOccupyCityByUnitType = event.new(),  --(unitId)

    evtAttackPlayer = event.new(), --(isWin)
    evtRobPlayer = event.new(),  
    evtCastleRebuild = event.new(),
    evtKillMonster = event.new(),    --(id)
}

--sync personal info
local burnEndTimestamp = 0
local cityDefense = 0

local msgIdFlags = {}

local checkTroopTimer

-- local function fillMailParam( msg )
--     if msg.drops == nil then
--         msg.drops = {}
--     end
--     if msg.food == nil then
--         msg.food = 0
--     end
--     if msg.wood == nil then
--         msg.wood = 0
--     end
--     if msg.iron == nil then
--         msg.iron = 0
--     end
--     if msg.stone == nil then
--         msg.stone = 0
--     end
--     if msg.foodRm == nil then
--         msg.foodRm = 0
--     end
--     if msg.woodRm == nil then
--         msg.woodRm = 0
--     end
--     if msg.ironRm == nil then
--         msg.ironRm = 0
--     end
--     if msg.stoneRm == nil then
--         msg.stoneRm = 0
--     end
-- end

local mapEventHandler = {
    onMarch = function(msgId, msg)
        -- print(debug.traceback())
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true

        army.onMarch(msg.pos[1], msg.pos[2], msg.troopType, msg.armyList.team, msg.troopId)

        local param = utils.serialize(msg)
        -- print("serialize = ", param)

        map.mapService:cast("confirmMsg", msgId)
    end,

    onMarchBack = function(msgId, msg)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true

        if msg.isArriveCastle then
            --army back
            army.onMarchBack(msg.troopType, msg.armyList.team, msg.isArriveCastle)
            -- print("onMarchBack msg", utils.serialize(msg))

            --resource 
            local gaintype = p.ResourceGainType.DEFAULT
            if msg.troopType == p.MapTroopType.SIEGE_CASTLE then
                gaintype = p.ResourceGainType.PLUNDER
            elseif msg.troopType == p.MapTroopType.GATHER  then
                gaintype = p.ResourceGainType.GATHER
            elseif msg.troopType == p.MapTroopType.MONSTER  then
                gaintype = p.ResourceGainType.MOSTER_DROP
            elseif msg.troopType == p.MapTroopType.SIEGE_CITY  then
                gaintype = p.ResourceGainType.CITY_DROP
            elseif msg.troopType == p.MapTroopType.TRANSPORT  then
                gaintype = p.ResourceGainType.TRANSPORT_FAIL
            end
            user.addResource(p.ResourceType.FOOD, msg.food, gaintype)
            user.addResource(p.ResourceType.WOOD, msg.wood, gaintype)
            user.addResource(p.ResourceType.IRON, msg.iron, gaintype)
            user.addResource(p.ResourceType.STONE, msg.stone, gaintype)
            --add item
            if msg.drops then
                bag.pickDropItems(msg.drops, gaintype)
            end

            local param = utils.serialize(msg)
            --print("serialize = ", param)
        else
            --arrive campfixed
            army.onMarchBack(msg.troopType, msg.armyList.team, msg.isArriveCastle)
        end

        map.mapService:cast("confirmMsg", msgId)
        -- print("onMarchBack confirmMsg", msgId, msg)
    end,

    onTroopReachInvalid = function(msgId, msg)
        --print('onTroopReachInvalid', msgId, msg)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true
        local troopType = msg.type
        if troopType == p.MapTroopType.MONSTER then
             local manualCost = t.configure["energyConsume"].monster or 0
             user.addEnergy(manualCost, p.EnergyGainType.ATTACK_MONSTER_INVALID, true)
        elseif troopType == p.MapTroopType.WORLDBOSS then
             local manualCost = t.configure["energyConsume"].wolrdboss or 0
             user.addEnergy(manualCost, p.EnergyGainType.ATTACK_MONSTER_INVALID, true)
        elseif troopType == p.MapTroopType.CAMP_FIXED then
            -- local foodCost = t.configure['campFixedConsumed'].food;
            -- local woodCost = t.configure['campFixedConsumed'].wood;
            -- local stoneCost = t.configure['campFixedConsumed'].stone;
            -- local ironCost = t.configure['campFixedConsumed'].iron;
            -- user.addResource(p.ResourceType.FOOD, foodCost)
            -- user.addResource(p.ResourceType.WOOD, woodCost)
            -- user.addResource(p.ResourceType.IRON, stoneCost)
            -- user.addResource(p.ResourceType.STONE, ironCost)
        end
        map.mapService:cast("confirmMsg", msgId)
    end,

    onCreateCampFixedFailed = function(msgId, msg)
        --print('onCreateCampFixedFailed', msgId, msg)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true
        local foodCost = t.configure['campFixedConsumed'].food;
        local woodCost = t.configure['campFixedConsumed'].wood;
        local stoneCost = t.configure['campFixedConsumed'].stone;
        local ironCost = t.configure['campFixedConsumed'].iron;
        user.addResource(p.ResourceType.FOOD, foodCost, p.ResourceGainType.CAMP_FIXED_FAILED)
        user.addResource(p.ResourceType.WOOD, woodCost, p.ResourceGainType.CAMP_FIXED_FAILED)
        user.addResource(p.ResourceType.IRON, stoneCost, p.ResourceGainType.CAMP_FIXED_FAILED)
        user.addResource(p.ResourceType.STONE, ironCost, p.ResourceGainType.CAMP_FIXED_FAILED)

        map.mapService:cast("confirmMsg", msgId)
    end,

    onScout = function(msgId, msg)
        -- print("------------ onScout -------------- !!!")
        -- print("onScout", msgId, msg)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true
        --mail and event
        local mailSubType, isWin = 0, false
        if msg.myAttackType == p.AttackType.ATTACK then
            if msg.winner == p.AttackType.ATTACK then
                mailSubType = p.MailSubType.SCOUT_ATTACK_CASTLE_SUCCESS
                local targetTplId = msg.defender.targetTplId
                local resTpl = t.mapUnit[targetTplId]
                if resTpl then
                    if resTpl.unitType == p.MapUnitType.CASTLE then
                        mailSubType = p.MailSubType.SCOUT_ATTACK_CASTLE_SUCCESS
                    elseif resTpl.unitType == p.MapUnitType.FARM_FOOD
                        or resTpl.unitType == p.MapUnitType.FARM_WOOD
                        or resTpl.unitType == p.MapUnitType.MINE_STONE
                        or resTpl.unitType == p.MapUnitType.MINE_IRON then
                        mailSubType = p.MailSubType.SCOUT_ATTACK_RESOURCE_SUCCESS
                    elseif resTpl.unitType == p.MapUnitType.CAMP_FIXED then
                        mailSubType = p.MailSubType.SCOUT_ATTACK_CAMP_FIXED_SUCCESS
                    elseif resTpl.unitType == p.MapUnitType.CAMP_TEMP then
                        mailSubType = p.MailSubType.SCOUT_ATTACK_CAMP_TEMP_SUCCESS
                    elseif resTpl.unitType == p.MapUnitType.CAPITAL
                        or resTpl.unitType == p.MapUnitType.CHOW
                        or resTpl.unitType == p.MapUnitType.PREFECTURE
                        or resTpl.unitType == p.MapUnitType.COUNTY then
                        mailSubType = p.MailSubType.SCOUT_ATTACK_FAMOUS_CITY_SUCCESS
                    elseif resTpl.unitType == p.MapUnitType.MONSTER then
                        --
                    end
                end
                isWin = true
                map.evtScout:trigger(true, targetTplId)
            else
                mailSubType = p.MailSubType.SCOUT_ATTACK_CASTLE_FAIL
                map.evtScout:trigger(false, targetTplId)
            end
            map.addScoutCount()
            -- log
            logStub.appendMarch(user.uid, p.MapTroopType.SCOUT, isWin, msg.timestamp)

            local param = utils.serialize(msg)
            -- print("serialize = ", param)
            local title, content = mail.getTitleContentBySubType(mailSubType)
            mail.saveMailRaw(user.uid, 0, p.MailType.SCOUT, mailSubType, title, content, msg.timestamp, true, true, '', param, 0)

        else
            if msg.winner == p.AttackType.ATTACK then
                mailSubType = p.MailSubType.SCOUT_DEFENSE_FAIL
            else
                mailSubType = p.MailSubType.SCOUT_DEFENSE_SUCCESS
                isWin = true
            end
            map.evtBeScout:trigger(isWin)

            msg.defender.result = nil
            local param = utils.serialize(msg)
            -- print("serialize = ", param)
            local title, content = mail.getTitleContentBySubType(mailSubType)
            mail.saveMailRaw(user.uid, 0, p.MailType.SCOUT, mailSubType, title, content, msg.timestamp, true, true, '', param, 0)
        end

        map.mapService:cast("confirmMsg", msgId)
    end,

    onGatherResource = function(msgId, msg)
        -- print("onGatherResource", msgId, msg)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true

        --TODO:gather drop

        local param = utils.serialize(msg)
        -- print("onGatherResource.......serialize = ", param)
        --add item
        bag.pickDropItems(msg.drops, p.ResourceGainType.GATHER)

        --trigger
        for _, drop in pairs(msg.drops) do
            if drop.tplId == p.SpecialPropIdType.FOOD then
                map.evtResourceGather:trigger(p.ResourceType.FOOD, drop.count)
            elseif drop.tplId == p.SpecialPropIdType.WOOD then
                map.evtResourceGather:trigger(p.ResourceType.WOOD, drop.count)
            elseif drop.tplId == p.SpecialPropIdType.IRON then
                map.evtResourceGather:trigger(p.ResourceType.IRON, drop.count)
            elseif drop.tplId == p.SpecialPropIdType.STONE then
                map.evtResourceGather:trigger(p.ResourceType.STONE, drop.count)
            elseif drop.tplId == p.SpecialPropIdType.GOLD then
                map.evtResourceGather:trigger(p.ResourceType.GOLD, drop.count)
            end
        end

        --mail
        local mailSubType = p.MailSubType.GATHER_FOOD
        local resTpl = t.mapUnit[msg.resTplId]
        if resTpl then
            if resTpl.unitType == p.MapUnitType.FARM_FOOD then
                mailSubType = p.MailSubType.GATHER_FOOD
            elseif resTpl.unitType == p.MapUnitType.FARM_WOOD then
                mailSubType = p.MailSubType.GATHER_WOOD
            elseif resTpl.unitType == p.MapUnitType.MINE_STONE then
                mailSubType = p.MailSubType.GATHER_STONE
            elseif resTpl.unitType == p.MapUnitType.MINE_IRON then
                mailSubType = p.MailSubType.GATHER_IRON
            end
        end
        local title, content = mail.getTitleContentBySubType(mailSubType)
        local attachments = utils.serialize(msg.drops)
        if attachments == nil then attachments = '' end
        mail.saveMailRaw(user.uid, 0, p.MailType.GATHER, mailSubType, title, content, msg.timestamp, true, true, attachments, param, 0)

        -- log
        logStub.appendMarch(user.uid, p.MapTroopType.GATHER, true, msg.timestamp)

        map.mapService:cast("confirmMsg", msgId)
    end,

    onAttackResource = function(msgId, unitId, troopId, msg, reportId)
        -- print("onAttackResource", msgId, unitId, troopId, utils.serialize(msg))
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true

        local isAttackPlayer = msg.defender.uid ~= 0
        --mail and event
        local mailSubType, detail, isWin = 0, {armyList = {}}, false

        if not isAttackPlayer then
            --打野怪
            if msg.myAttackType == p.AttackType.ATTACK then
                if msg.winner == p.AttackType.ATTACK then
                    mailSubType = p.MailSubType.REPORT_GATHER_BATTLE_SUCCESS
                    isWin = true
                    --add item
                    -- print("onAttackResource drops", utils.serialize(msg.drops))
                    bag.pickDropItems(msg.drops, p.ResourceGainType.MONSTER_DROP)
                else
                    mailSubType = p.MailSubType.REPORT_GATHER_BATTLE_FAIL
                end
                detail = msg.attacker.detail
                -- log
                logStub.appendMarch(user.uid, p.MapTroopType.GATHER, isWin, msg.timestamp)
            end

            -- --handle army die
            msg.dies = army.onDie(detail.armyList)

            -- --killPower
            -- map.calculateKillPower(msg)
            -- fillMailParam(msg)
            local param = utils.serialize(msg)
            -- print("onAttackResource.....1serialize = ", param)
            local title, content = mail.getTitleContentBySubType(mailSubType)
            local attachments = utils.serialize(msg.drops)
            if attachments == nil then attachments = '' end
            mail.saveMailRaw(user.uid, 0, p.MailType.REPORT, mailSubType, title, content, msg.timestamp, true, true, attachments, param, reportId)
        else
            --打玩家
            if msg.myAttackType == p.AttackType.ATTACK then
                if msg.winner == p.AttackType.ATTACK then
                    mailSubType = p.MailSubType.REPORT_ATTACK_RESOURCE_SUCCESS
                    isWin = true
                else
                    mailSubType = p.MailSubType.REPORT_ATTACK_RESOURCE_FAIL
                end

                map.evtAttackPlayer:trigger(isWin)
                detail = msg.attacker.detail
                -- log
                logStub.appendMarch(user.uid, p.MapTroopType.GATHER, isWin, msg.timestamp)
            else
                if msg.winner == p.AttackType.ATTACK then
                    mailSubType = p.MailSubType.REPORT_DEFENSE_RESOURCE_FAIL
                else
                    mailSubType = p.MailSubType.REPORT_DEFENSE_RESOURCE_SUCCESS
                    isWin = true
                end
                detail = msg.defender.detail
            end

            -- --handle army die
            msg.dies = army.onDie(detail.armyList)

            -- --killPower
            -- map.calculateKillPower(msg)
            -- fillMailParam(msg)
            local param = utils.serialize(msg)
            -- print("onAttackResource.....2serialize = ", param)
            local title, content = mail.getTitleContentBySubType(mailSubType)
            local attachments = utils.serialize(msg.drops)
            if attachments == nil then attachments = '' end
            mail.saveMailRaw(user.uid, 0, p.MailType.REPORT, mailSubType, title, content, msg.timestamp, true, true, attachments, param, reportId)
        end
        
        map.addBattleCount(msg.winner, msg.myAttackType)

        map.mapService:cast("confirmMsg", msgId)
    end,

    onAttackMonsterInvalid = function(msgId, msg)
        --print('onAttackMonsterInvalid', msgId, msg)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true
         local manualCost = t.configure["energyConsume"].monster or 0
         user.addEnergy(manualCost, p.EnergyGainType.ATTACK_MONSTER_INVALID, true)

        map.mapService:cast("confirmMsg", msgId)
    end,

    onAttackMonster = function(msgId, unitId, troopId, msg, reportId)
        -- print("onAttackMonster", msgId, unitId, troopId, msg, msg.winner, msg.timestamp, reportId)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true
        local monsterTpl = t.mapUnit[msg.tplId]
        if not monsterTpl then
            print('monster tpl not exist', msg.tplId)
            return
        end

        --handle army die
        army.onDie(msg.armyList, false)

        --mail
        local isWin, mailSubType = false, 0
        if msg.winner == p.AttackType.ATTACK then
            isWin = true
            mailSubType = p.MailSubType.REPORT_ATTACK_MONSTER_SUCCESS

            --drop
            local dropTpl = t.drop[monsterTpl.dropId]
            if not dropTpl then
                print('dropId not exist', dropId)
                return
            end
            msg.drops = dropTpl:DoDrop() or {}
            --local actDrop = activity.handleCollectData(p.ActivityType.MONSTER_COLLECTION, msg.monsterTplId)
            --t.mergeDrops(msg.drops, {})

            --add item
            bag.pickDropItems(msg.drops, p.ResourceGainType.MONSTER_DROP)
            --add hero exp
            army.pickDropItems(msg.drops, p.ResourceGainType.MONSTER_DROP, msg.armyList)
            map.evtKillMonster:trigger(msg.tplId)
        else
            mailSubType = p.MailSubType.REPORT_ATTACK_MONSTER_FAIL
        end

        --trigger
        map.evtAttackMonster:trigger(monsterTpl, isWin)

        --add puppet
        --msg.puppet = agent.tower.onMapAttackMonster(monsterTpl, isWin)
        
        local param = utils.serialize(msg)
        --print("serialize = ", param)
        agent.sendPktout(p.SC_MAP_UNIT_BATTLE, unitId, troopId, param)

        -- save mail
        local title, content = mail.getTitleContentBySubType(mailSubType)
        local attachments = utils.serialize(msg.drops)
        if attachments == nil then attachments = '' end
        mail.saveMailRaw(user.uid, 0, p.MailType.REPORT, mailSubType, title, content, msg.timestamp, true, true, 
            attachments, param, reportId)

        -- log
        logStub.appendMarch(user.uid, p.MapTroopType.MONSTER, isWin, msg.timestamp)

        map.mapService:cast("confirmMsg", msgId)
    end,

    onAttackCity = function(msgId, unitId, troopId, msg, reportId)
        -- print("onAttackCity", msgId, unitId, troopId, msg, msg.winner, msg.timestamp)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true

        local isAttackPlayer = msg.defender.uid ~= 0
        --mail and event
        local mailSubType, detail, isWin = 0, {armyList = {}}, false

        if not isAttackPlayer then
            --打野怪
            if msg.myAttackType == p.AttackType.ATTACK then
                if msg.winner == p.AttackType.ATTACK then
                    mailSubType = p.MailSubType.REPORT_OCCUPY_FAMOUS_CITY_SUCCESS
                    isWin = true
                    --add item
                    bag.pickDropItems(msg.drops, p.ResourceGainType.CITY_DROP)
                    --add hero exp
                    army.pickDropItems(msg.drops, p.ResourceGainType.CITY_DROP, msg.attacker.detail.armyList)
                    map.evtOccupyCity:trigger(msg.tplId)
                    map.evtOccupyCityByUnitType:trigger(unitId)
                else
                    mailSubType = p.MailSubType.REPORT_OCCUPY_FAMOUS_CITY_FAIL
                end
                detail = msg.attacker.detail
                -- log
                logStub.appendMarch(user.uid, p.MapTroopType.SIEGE_CITY, isWin, msg.timestamp)
            end
        else
            --打玩家
            if msg.myAttackType == p.AttackType.ATTACK then
                if msg.winner == p.AttackType.ATTACK then
                    mailSubType = p.MailSubType.REPORT_ATTACK_FAMOUS_CITY_SUCCESS
                    isWin = true
                    --add item
                    bag.pickDropItems(msg.drops, p.ResourceGainType.CITY_DROP)
                    army.pickDropItems(msg.drops, p.ResourceGainType.CITY_DROP, msg.attacker.detail.armyList)
                else
                    mailSubType = p.MailSubType.REPORT_ATTACK_FAMOUS_CITY_FAIL
                end

                map.evtAttackPlayer:trigger(isWin)
                detail = msg.attacker.detail
                -- log
                logStub.appendMarch(user.uid, p.MapTroopType.SIEGE_CITY, isWin, msg.timestamp)
            else
                if msg.winner == p.AttackType.ATTACK then
                    mailSubType = p.MailSubType.REPORT_DEFENSE_FAMOUS_CITY_FAIL
                else
                    mailSubType = p.MailSubType.REPORT_DEFENSE_FAMOUS_CITY_SUCCESS
                    isWin = true
                end
                detail = msg.defender.detail
            end
        end

        --handle army die
        msg.dies = army.onDie(detail.armyList, false)
        -- print("onAttackCity", utils.serialize(detail.armyList))

        -- fillMailParam(msg)
        local param = utils.serialize(msg)
        --print("utils.serialize = ", param)
        -- save mail
        local title, content = mail.getTitleContentBySubType(mailSubType)
        local attachments = utils.serialize(msg.drops)
        if attachments == nil then attachments = '' end
        mail.saveMailRaw(user.uid, 0, p.MailType.REPORT, mailSubType, title, content, msg.timestamp, true, true, 
            attachments, param, reportId)

        map.addBattleCount(msg.winner, msg.myAttackType)

        map.mapService:cast("confirmMsg", msgId)
    end,

    onAttackCatapult = function(msgId, unitId, troopId, msg, reportId)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true

        local isAttackPlayer = msg.defender.uid ~= 0
        --mail and event
        local mailSubType, detail, isWin = 0, {armyList = {}}, false

        
        if msg.myAttackType == p.AttackType.ATTACK then
            if msg.winner == p.AttackType.ATTACK then
                mailSubType = p.MailSubType.REPORT_ATTACK_CATAPULT_SUCCESS
                isWin = true
                bag.pickDropItems(msg.drops, p.ResourceGainType.CATAPULT_DROP)
                --add hero exp
                army.pickDropItems(msg.drops, p.ResourceGainType.CATAPULT_DROP, msg.attacker.detail.armyList)
            else
                mailSubType = p.MailSubType.REPORT_ATTACK_CATAPULT_FAIL
            end
            map.evtAttackPlayer:trigger(isWin)
            detail = msg.attacker.detail
            -- log
            logStub.appendMarch(user.uid, p.MapTroopType.SIEGE_CATAPULT, isWin, msg.timestamp)
        else
            if msg.winner == p.AttackType.ATTACK then
                mailSubType = p.MailSubType.REPORT_DEFENSE_CATAPULT_FAIL
            else
                mailSubType = p.MailSubType.REPORT_DEFENSE_CATAPULT_SUCCESS
                isWin = true
            end
            detail = msg.defender.detail
        end
        

        --handle army die
        msg.dies = army.onDie(detail.armyList, false)

        -- fillMailParam(msg)
        local param = utils.serialize(msg)
        --print("utils.serialize = ", param)
        -- save mail
        local title, content = mail.getTitleContentBySubType(mailSubType)
        local attachments = utils.serialize(msg.drops)
        if attachments == nil then attachments = '' end
        mail.saveMailRaw(user.uid, 0, p.MailType.REPORT, mailSubType, title, content, msg.timestamp, true, true, 
            attachments, param, reportId)

        map.addBattleCount(msg.winner, msg.myAttackType)

        map.mapService:cast("confirmMsg", msgId) 
    end,

    onAttackWorldBoss = function(msgId, unitId, troopId, msg)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true

        local isAttackPlayer = msg.defender.uid ~= 0
        --mail and event
        local mailSubType, detail, isWin = 0, {armyList = {}}, false

        
        if msg.myAttackType == p.AttackType.ATTACK then
            if msg.winner == p.AttackType.ATTACK then
                mailSubType = p.MailSubType.REPORT_ATTACK_WORLDBOSS_SUCCESS
                isWin = true
            else
                mailSubType = p.MailSubType.REPORT_ATTACK_WORLDBOSS_FAIL
            end
            map.evtAttackPlayer:trigger(isWin)
            detail = msg.attacker.detail
            -- log
            logStub.appendMarch(user.uid, p.MapTroopType.WORLDBOSS, isWin, msg.timestamp)
        end
        

        --handle army die
        msg.dies = army.onDie(detail.armyList, false)

        -- fillMailParam(msg)
        local param = utils.serialize(msg)
        --print("utils.serialize = ", param)
        -- save mail
        local title, content = mail.getTitleContentBySubType(mailSubType)
        local attachments = utils.serialize(msg.drops)
        if attachments == nil then attachments = '' end
        mail.saveMailRaw(user.uid, 0, p.MailType.REPORT, mailSubType, title, content, msg.timestamp, true, true, attachments, 
            param, 0)
        -- print("title, content ,mailSubType= ", title, content, mailSubType)

        map.addBattleCount(msg.winner, msg.myAttackType)

        map.mapService:cast("confirmMsg", msgId) 
    end,

    --攻击世界BOSS结算 
    onAttackWorldBossEnd = function(msgId, unitId, troopId, msg)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true
        local worldBossTpl = t.mapUnit[msg.wolrdboss.tplId]
        if not worldBossTpl then
            print('world boss tpl not exist', msg.worldBoss.tplId)
            return
        end
        local mailSubType = 0
        local Ratio = (msg.wolrdboss.beginArmyCount - msg.wolrdboss.endArmyCount) / msg.wolrdboss.maxArmyCount
         -- hurt mail
        mailSubType = p.MailSubType.SYSTEM_WORLDBOSS_HURT_REWARD
        local params = {}
        params.params1 = worldBossTpl.name .. "," .. (msg.wolrdboss.beginArmyCount - msg.wolrdboss.endArmyCount)
        local dropTimes = math.floor(Ratio / (worldBossTpl.paramExt.hurtDropsCoe / 10000))
        -- print("dropTimes, worldBossTpl.name = ", dropTimes, worldBossTpl.name, msg.wolrdboss.beginArmyCount - msg.wolrdboss.endArmyCount)
        local hurtDrops = {}
        for var = 1, dropTimes, 1 do
            local dropTpl = t.drop[worldBossTpl.paramExt.hurtDrops]
            if not dropTpl then
                print('dropId not exist', worldBossTpl.paramExt.killDropId)
                break
            end 
            local OneDrops = dropTpl:DoDrop() or {}
            t.mergeDrops(hurtDrops, OneDrops)
        end
        local title, content = mail.getTitleContentBySubType(mailSubType)
        local attachments = utils.serialize(msg.drops)
        if attachments == nil then attachments = '' end
        -- print("worldBossTpl.paramExt.hurtDrops", utils.serialize(worldBossTpl.paramExt.hurtDrops))
        -- print("hurtDrops", utils.serialize(hurtDrops))
        if dropTimes ~= 0 then
            mail.saveMailRaw(user.uid, 0, p.MailType.SYSTEM, mailSubType, title, content, msg.timestamp, true, false, 
                utils.serialize(hurtDrops), utils.serialize(params), 0)
        end

        -- kill mail
        if msg.wolrdboss.endArmyCount == 0 then
            repeat
                local mailSubType = p.MailSubType.SYSTEM_WORLDBOSS_KILL_REWARD
                local params = {}
                params.params1 = worldBossTpl.name
                -- kill drop
                
                local dropTpl = t.drop[worldBossTpl.paramExt.killDropId]
                if not dropTpl then
                    print('dropId not exist', worldBossTpl.paramExt.killDropId)
                    break
                end
                local killDrops = dropTpl:DoDrop() or {}
                local title, content = mail.getTitleContentBySubType(mailSubType)
                mail.saveMailRaw(user.uid, 0, p.MailType.SYSTEM, mailSubType, title, content, msg.timestamp, true, false, 
                    utils.serialize(killDrops), utils.serialize(params), 0)
                print("killDrops", utils.serialize(killDrops))
            until true
        end    

        map.mapService:cast("confirmMsg", msgId) 
    end,

    --攻击名城结算 
    onAttackCityEnd = function(msgId, unitId, troopId, msg)
        -- print("onAttackCityEnd", msgId, unitId, troopId, msg)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true
        local cityTpl = t.mapUnit[msg.wolrdboss.tplId]
        if not cityTpl then
            print('world boss tpl not exist', msg.wolrdboss.tplId)
            return
        end
        -- print("onAttackCityEnd", utils.serialize(msg.drops))
        local mailSubType = 0
        -- print("msg.wolrdboss.beginArmyCount, msg.wolrdboss.endArmyCount, msg.wolrdboss.maxArmyCount", msg.wolrdboss.beginArmyCount,  msg.wolrdboss.endArmyCount, msg.wolrdboss.maxArmyCount)
        local Ratio = (msg.wolrdboss.beginArmyCount - msg.wolrdboss.endArmyCount) / msg.wolrdboss.maxArmyCount
         -- hurt mail
        mailSubType = p.MailSubType.SYSTEM_CITY_HURT_REWARD
        local params = {}
        params.params1 = cityTpl.name .. "," .. (msg.wolrdboss.beginArmyCount - msg.wolrdboss.endArmyCount)
        local dropTimes = math.floor(Ratio / (cityTpl.paramExt.hurtDropsCoe / 10000))
        -- print("Ratio, dropTimes, worldBossTpl.name = ", Ratio, dropTimes, cityTpl.name, msg.wolrdboss.beginArmyCount - msg.wolrdboss.endArmyCount)
        local hurtDrops = {}
        for var = 1, dropTimes, 1 do
            local dropTpl = t.drop[cityTpl.paramExt.hurtDrops]
            if not dropTpl then
                print('dropId not exist', cityTpl.paramExt.killDropId)
                break
            end 
            local OneDrops = dropTpl:DoDrop() or {}
            t.mergeDrops(hurtDrops, OneDrops)
        end
        local title, content = mail.getTitleContentBySubType(mailSubType)
        local attachments = utils.serialize(msg.drops)
        if attachments == nil then attachments = '' end
        -- print("worldBossTpl.paramExt.hurtDrops", utils.serialize(cityTpl.paramExt.hurtDrops))
        -- print("hurtDrops", utils.serialize(hurtDrops))
        if dropTimes ~= 0 then
            mail.saveMailRaw(user.uid, 0, p.MailType.SYSTEM, mailSubType, title, content, msg.timestamp, true, false, 
                utils.serialize(hurtDrops), utils.serialize(params), 0)
        end
        -- kill mail
        if msg.wolrdboss.endArmyCount == 0 then
            repeat
                local mailSubType = p.MailSubType.SYSTEM_CITY_KILL_REWARD
                local params = {}
                params.params1 = cityTpl.name
                -- kill drop
                
                local dropTpl = t.drop[cityTpl.paramExt.killDropId]
                if not dropTpl then
                    print('dropId not exist', cityTpl.paramExt.killDropId)
                    break
                end
                local killDrops = dropTpl:DoDrop() or {}
                local title, content = mail.getTitleContentBySubType(mailSubType)
                mail.saveMailRaw(user.uid, 0, p.MailType.SYSTEM, mailSubType, title, content, msg.timestamp, true, false, 
                    utils.serialize(killDrops), utils.serialize(params), 0)
                print("killDrops", utils.serialize(killDrops))
            until true
        end    

        map.mapService:cast("confirmMsg", msgId) 
    end,

    --攻击名城结算 
    onAttackCatapultEnd = function(msgId, unitId, troopId, msg)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true
        local cityTpl = t.mapUnit[msg.wolrdboss.tplId]
        if not cityTpl then
            print('world boss tpl not exist', msg.worldBoss.tplId)
            return
        end
        local mailSubType = 0
        -- print("msg.wolrdboss.beginArmyCount, msg.wolrdboss.endArmyCount, msg.wolrdboss.maxArmyCount", msg.wolrdboss.beginArmyCount,  msg.wolrdboss.endArmyCount, msg.wolrdboss.maxArmyCount)
        local Ratio = (msg.wolrdboss.beginArmyCount - msg.wolrdboss.endArmyCount) / msg.wolrdboss.maxArmyCount
         -- hurt mail
        mailSubType = p.MailSubType.SYSTEM_CATAPULT_HURT_REWARD
        local params = {}
        params.params1 = cityTpl.name .. "," .. (msg.wolrdboss.beginArmyCount - msg.wolrdboss.endArmyCount)
        local dropTimes = math.floor(Ratio / (cityTpl.paramExt.hurtDropsCoe / 10000))
        -- print("dropTimes, worldBossTpl.name = ", dropTimes, cityTpl.name, msg.wolrdboss.beginArmyCount - msg.wolrdboss.endArmyCount)
        local hurtDrops = {}
        for var = 1, dropTimes, 1 do
            local dropTpl = t.drop[cityTpl.paramExt.hurtDrops]
            if not dropTpl then
                print('dropId not exist', cityTpl.paramExt.killDropId)
                break
            end 
            local OneDrops = dropTpl:DoDrop() or {}
            t.mergeDrops(hurtDrops, OneDrops)
        end
        local title, content = mail.getTitleContentBySubType(mailSubType)
        local attachments = utils.serialize(msg.drops)
        if attachments == nil then attachments = '' end
        -- print("worldBossTpl.paramExt.hurtDrops", utils.serialize(cityTpl.paramExt.hurtDrops))
        -- print("hurtDrops", utils.serialize(hurtDrops))
        if dropTimes ~= 0 then
            mail.saveMailRaw(user.uid, 0, p.MailType.SYSTEM, mailSubType, title, content, msg.timestamp, true, false, 
                utils.serialize(hurtDrops), utils.serialize(params), 0)
        end
        -- kill mail
        if msg.wolrdboss.endArmyCount == 0 then
            repeat
                local mailSubType = p.MailSubType.SYSTEM_CATAPULT_KILL_REWARD
                local params = {}
                params.params1 = cityTpl.name
                -- kill drop
                
                local dropTpl = t.drop[cityTpl.paramExt.killDropId]
                if not dropTpl then
                    print('dropId not exist', cityTpl.paramExt.killDropId)
                    break
                end
                local killDrops = dropTpl:DoDrop() or {}
                local title, content = mail.getTitleContentBySubType(mailSubType)
                mail.saveMailRaw(user.uid, 0, p.MailType.SYSTEM, mailSubType, title, content, msg.timestamp, true, false, 
                    utils.serialize(killDrops), utils.serialize(params), 0)
                print("killDrops", utils.serialize(killDrops))
            until true
        end    

        map.mapService:cast("confirmMsg", msgId)
    end,

    --名城主权改变
    onChangeSovereign = function(msgId, unitId, troopId, msg) 
        -- print("onChangeSovereign", msgId, unitId, troopId, msg)
        -- old_sovereign  sovereign  
        local sovereign = msg.sovereign 
        local old_sovereign = msg.old_sovereign
        -- print("onChangeSovereign", msgId, unitId, troopId, msg, msg.sovereign,  msg.old_sovereign, msg.timestamp)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true

        local isAttackPlayer = msg.defender.uid ~= 0
        --mail and event
        local mailSubType, detail, isWin = 0, {armyList = {}}, false
        mailSubType = p.MailSubType.REPORT_OCCUPY_FAMOUS_CITY_SUCCESS

       --打玩家
        if msg.myAttackType == p.AttackType.ATTACK then
            if msg.winner == p.AttackType.ATTACK then
                mailSubType = p.MailSubType.REPORT_ATTACK_FAMOUS_CITY_SUCCESS
                isWin = true
                --add item
                bag.pickDropItems(msg.drops, p.ResourceGainType.CITY_DROP)
            else
                mailSubType = p.MailSubType.REPORT_ATTACK_FAMOUS_CITY_FAIL
            end

            map.evtAttackPlayer:trigger(isWin)
            detail = msg.attacker.detail
            -- log
            logStub.appendMarch(user.uid, p.MapTroopType.SIEGE_CITY, isWin, msg.timestamp)
        else
            if msg.winner == p.AttackType.ATTACK then
                mailSubType = p.MailSubType.REPORT_DEFENSE_FAMOUS_CITY_FAIL
            else
                mailSubType = p.MailSubType.REPORT_DEFENSE_FAMOUS_CITY_SUCCESS
                isWin = true
            end
            detail = msg.defender.detail
        end

        -- save mail
        local title, content = mail.getTitleContentBySubType(mailSubType)
        local attachments = utils.serialize(msg.drops)
        if attachments == nil then attachments = '' end
        mail.saveMailRaw(user.uid, 0, p.MailType.SYSTEM, mailSubType, title, content, msg.timestamp, true, true, 
            attachments, param, reportId)

        --map.addBattleCount(msg.winner, msg.myAttackType)

        map.mapService:cast("confirmMsg", msgId)

    end,
    
    onAttackCastle = function(msgId, unitId, troopId, msg, reportId)
        -- print("####onAttackCastle", msgId, unitId, troopId, utils.serialize(msg))
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true

        local mailSubType, detail, isWin = 0, {armyList = {}}, false

        if msg.myAttackType == p.AttackType.ATTACK then
            if msg.winner == p.AttackType.ATTACK then
                mailSubType = p.MailSubType.REPORT_ATTACK_CASTLE_SUCCESS
                isWin = true
            else
                mailSubType = p.MailSubType.REPORT_ATTACK_CASTLE_FAIL
            end

            map.evtAttackPlayer:trigger(isWin)
            detail = msg.attacker.detail
            -- log
            logStub.appendMarch(user.uid, p.MapTroopType.SIEGE_CASTLE, isWin, msg.timestamp)
        else
            if msg.winner == p.AttackType.ATTACK then
                mailSubType = p.MailSubType.REPORT_DEFENSE_CASTLE_FAIL
                map.evtDefenseCastle:trigger(false)
            else
                mailSubType = p.MailSubType.REPORT_DEFENSE_CASTLE_SUCCESS
                map.evtDefenseCastle:trigger(true)
                isWin = true
            end
            detail = msg.defender.detail
            --resource
            if not isWin then
                user.removeResource(p.ResourceType.FOOD, msg.foodRm, p.ResourceConsumeType.PLUNDERED)
                user.removeResource(p.ResourceType.WOOD, msg.woodRm, p.ResourceConsumeType.PLUNDERED)
                user.removeResource(p.ResourceType.IRON, msg.ironRm, p.ResourceConsumeType.PLUNDERED)
                user.removeResource(p.ResourceType.STONE, msg.stoneRm, p.ResourceConsumeType.PLUNDERED)
                -- -- -- 如果防守失败，则减少预备兵
                -- -- msg.dies = army.castleBattleSub()

                -- --改变资源区的时间戳
                -- for gridId, timestamp in pairs(msg.collectInfos) do
                --     local target = building.list[gridId]
                --     if target and target:isCollectBuilding() then
                --         target.param1 = timestamp
                --         target.sync = false
                --     end
                -- end
                building.sendBuildingUpdate()
            end
            burnEndTimestamp = msg.burnEndTs
        end

        -- local drops = {
        --     {tplId = p.SpecialPropIdType.FOOD, count = msg.food},
        --     {tplId = p.SpecialPropIdType.WOOD, count = msg.wood},
        --     {tplId = p.SpecialPropIdType.IRON, count = msg.iron},
        --     {tplId = p.SpecialPropIdType.STONE, count = msg.stone},
        -- }

        -- msg.drops = drops
        msg.dies = army.onDie(detail.armyList, false)
        local param = utils.serialize(msg)
        --print("utils.serialize = ", param)
        local attachments = ''
        if attachments == nil then attachments = '' end
        local title, content = mail.getTitleContentBySubType(mailSubType)
        mail.saveMailRaw(user.uid, 0, p.MailType.REPORT, mailSubType, title, content, msg.timestamp, true, true, attachments, param, reportId)

        map.addBattleCount(msg.winner, msg.myAttackType)
        
        map.mapService:cast("confirmMsg", msgId)
        -- user.sendUpdate()
    end,

    --击败玩家城池
    onBeatCastle = function(msgId, unitId, troopId, msg)
        -- print("###onBeatCastle", utils.serialize(msg))
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true

        local mailSubType, detail, isWin = 0, {armyList = {}}, false
        local player 

        if msg.myAttackType == p.AttackType.ATTACK then
            if msg.winner == p.AttackType.ATTACK then
                mailSubType = p.MailSubType.SYSTEM_ATTACK_CASTLE_SUCCESS
                isWin = true
                map.evtRobPlayer:trigger()
            else
                -- mailSubType = p.MailSubType.SYSTEM_BEAT_CASTLE_FAIL
            end
            player = msg.defender
            map.evtAttackPlayer:trigger(isWin)
        else
           if msg.winner == p.AttackType.ATTACK then
                mailSubType = p.MailSubType.SYSTEM_DEFENSE_CASTLE_FAIL 
                -- 如果防守失败，则减少预备兵
                msg.dies = army.castleBattleSub() 

                -- resource
                -- user.removeResource(p.ResourceType.FOOD, msg.foodRm, p.ResourceConsumeType.PLUNDERED)
                -- user.removeResource(p.ResourceType.WOOD, msg.woodRm, p.ResourceConsumeType.PLUNDERED)
                -- user.removeResource(p.ResourceType.IRON, msg.ironRm, p.ResourceConsumeType.PLUNDERED)
                -- user.removeResource(p.ResourceType.STONE, msg.stoneRm, p.ResourceConsumeType.PLUNDERED)

                --改变资源区的时间戳
                for gridId, timestamp in pairs(msg.collectInfos) do
                    local target = building.list[gridId]
                    if target and target:isCollectBuilding() then
                        target.param1 = timestamp
                        target.sync = false
                    end
                end
            end
            player = msg.attacker
        
           --map.evtAttackPlayer:trigger(isWin)
            if army.myDefenerTeam ~= 0 then
                local team =  msg.defender.detail.armyList.team
                local info = army.teams[team]
                print('onBeatCastle ... info..',utils.serialize(info))
                if info and info.confList then
                    info.teamState = p.TeamState.IDLE
                    for _,m in pairs(info.confList) do
                        print("m.armyType,p.ArmyState.NORMAL,m.count",m.armyType,p.ArmyState.NORMAL,m.count)
                        army.add(m.armyType,p.ArmyState.NORMAL,m.count)
                        army.sub(m.armyType,p.ArmyState.MARCHING,m.count)
                        local heroInfo = hero.info[m.heroId]
                        m.count = heroInfo.heroLeadership
                    end
                    info.sync = false
                    army.sendArmyInfosUpdate()
                    army.sendTeamInfosUpdate()

                    -- save it real time
                    army.dbSave()
                end
                army.myDefenerTeam = 0
            end
        end 
        --local params = {} 
        msg.params1 = player.nickname
        --local param = utils.serialize(msg)
        local param = utils.serialize(msg)

        -- 策划说，掠夺显示放在一场战斗的最后一个战报中显示
        -- local drops = {
        --     {tplId = p.SpecialPropIdType.FOOD, count = msg.food},
        --     {tplId = p.SpecialPropIdType.WOOD, count = msg.wood},
        --     {tplId = p.SpecialPropIdType.IRON, count = msg.iron},
        --     {tplId = p.SpecialPropIdType.STONE, count = msg.stone},
        -- }

        -- msg.drops = drops
        local param = utils.serialize(msg)
        --print("utils.serialize = ", param)
        -- local attachments = utils.serialize(drops)
        local attachments = nil
        if attachments == nil then attachments = '' end
        -- print("utils.serialize = ", utils.serialize(attachments))
        local title, content = mail.getTitleContentBySubType(mailSubType)
        mail.saveMailRaw(user.uid, 0, p.MailType.SYSTEM, mailSubType, title, content, msg.timestamp, true, false, 
            attachments, param, 0)

        map.addBattleCount(msg.winner, msg.myAttackType)
        
        map.mapService:cast("confirmMsg", msgId)
        -- user.sendUpdate()
    end,

    onAttackCampFixed = function(msgId, unitId, troopId, msg, reportId)
        -- print("onAttackCampFixed", msgId, unitId, troopId, msg, msg.winner, msg.timestamp)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true

        --mail and event
        local mailSubType, detail, isWin = 0, {armyList = {}}, false

        --打玩家
        if msg.myAttackType == p.AttackType.ATTACK then
            if msg.winner == p.AttackType.ATTACK then
                mailSubType = p.MailSubType.REPORT_ATTACK_CAMP_FIXED_SUCCESS
                isWin = true
            else
                mailSubType = p.MailSubType.REPORT_ATTACK_CAMP_FIXED_FAIL
            end

            map.evtAttackPlayer:trigger(isWin)
            detail = msg.attacker.detail
            -- log
            logStub.appendMarch(user.uid, p.MapTroopType.CAMP_FIXED_ATTACK, isWin, msg.timestamp)
        else
            if msg.winner == p.AttackType.ATTACK then
                mailSubType = p.MailSubType.REPORT_DEFENSE_CAMP_FIXED_FAIL
            else
                mailSubType = p.MailSubType.REPORT_DEFENSE_CAMP_FIXED_SUCCESS
                isWin = true
            end
            detail = msg.defender.detail
        end

        --handle army die
        msg.dies = army.onDie(detail.armyList, false)

        -- fillMailParam(msg)
        local param = utils.serialize(msg)
        --print("utils.serialize = ", param)
        -- save mail
        local title, content = mail.getTitleContentBySubType(mailSubType)
        local attachments = ''
        -- p.MailType.REPORT
        mail.saveMailRaw(user.uid, 0, p.MailType.REPORT, mailSubType, title, content, msg.timestamp, true, true, attachments, param, reportId)

        map.addBattleCount(msg.winner, msg.myAttackType)

        map.mapService:cast("confirmMsg", msgId)
    end,

    onOccupyCampFixed = function(msgId, unitId, troopId, msg, reportId)
        -- print("onOccupyCampFixed", msgId, unitId, troopId, msg, msg.winner, msg.timestamp)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true

        --mail and event
        local mailSubType, detail, isWin = 0, {armyList = {}}, false

        --打玩家
        if msg.myAttackType == p.AttackType.ATTACK then
            if msg.winner == p.AttackType.ATTACK then
                mailSubType = p.MailSubType.REPORT_OCCUPY_CAMP_FIXED_SUCCESS
                isWin = true
            else
                mailSubType = p.MailSubType.REPORT_OCCUPY_CAMP_FIXED_FAIL
            end

            map.evtAttackPlayer:trigger(isWin)
            detail = msg.attacker.detail
            -- log
            logStub.appendMarch(user.uid, p.MapTroopType.CAMP_FIXED_OCCUPY, isWin, msg.timestamp)
        else
            if msg.winner == p.AttackType.ATTACK then
                mailSubType = p.MailSubType.REPORT_DEFENSE_CAMP_FIXED_FAIL
            else
                mailSubType = p.MailSubType.REPORT_DEFENSE_CAMP_FIXED_SUCCESS
                isWin = true
            end
            detail = msg.defender.detail
        end

        --handle army die
        msg.dies = army.onDie(detail.armyList, false)

        -- fillMailParam(msg)
        local param = utils.serialize(msg)
        --print("utils.serialize = ", param)
        -- save mail
        local title, content = mail.getTitleContentBySubType(mailSubType)
        local attachments = ''
        -- p.MailType.REPORT
        mail.saveMailRaw(user.uid, 0, p.MailType.SYSTEM, mailSubType, title, content, msg.timestamp, true, true, attachments, param, reportId)

        map.addBattleCount(msg.winner, msg.myAttackType)

        map.mapService:cast("confirmMsg", msgId)
    end,

    onAttackCampTemp = function(msgId, unitId, troopId, msg, reportId)
        -- print("onAttackCampTemp", msgId, unitId, troopId, msg, msg.winner, msg.timestamp)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true

        --mail and event
        local mailSubType, detail, isWin = 0, {armyList = {}}, false

        --打玩家
        if msg.myAttackType == p.AttackType.ATTACK then
            if msg.winner == p.AttackType.ATTACK then
                mailSubType = p.MailSubType.REPORT_ATTACK_CAMP_TEMP_SUCCESS
                isWin = true
            else
                mailSubType = p.MailSubType.REPORT_ATTACK_CAMP_TEMP_FAIL
            end
            
            map.evtAttackPlayer:trigger(isWin)
            detail = msg.attacker.detail
            -- log
            logStub.appendMarch(user.uid, p.MapTroopType.CAMP_TEMP_ATTACK, isWin, msg.timestamp)
        else
            if msg.winner == p.AttackType.ATTACK then
                mailSubType = p.MailSubType.REPORT_DEFENSE_CAMP_TEMP_FAIL
            else
                mailSubType = p.MailSubType.REPORT_DEFENSE_CAMP_TEMP_SUCCESS
                isWin = true
            end
            detail = msg.defender.detail
        end

        --handle army die
        msg.dies = army.onDie(detail.armyList, false)

        -- fillMailParam(msg)
        local param = utils.serialize(msg)
        --print("utils.serialize = ", param)
        -- save mail
        local title, content = mail.getTitleContentBySubType(mailSubType)
        local attachments = ''
        mail.saveMailRaw(user.uid, 0, p.MailType.REPORT, mailSubType, title, content, msg.timestamp, true, true, attachments, param, reportId)

        map.addBattleCount(msg.winner, msg.myAttackType)

        map.mapService:cast("confirmMsg", msgId)
    end,

    onCityDefenseUpdate = function(msgId, cityDef)
        -- print("onCityDefenseUpdate", msgId, cityDef)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true

        cityDefense = cityDef

        map.mapService:cast("confirmMsg", msgId)
    end,
    
    onCastleRebuild = function(msgId, msg)
        --print("onCastleRebuild", msgId, msg)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true
            
        info.x = msg.castlePos[1]
        info.y = msg.castlePos[2]      

        map.evtCastleRebuild:trigger()
        --send mail
        -- local mailSubType = p.MailSubType.CASTLE_REBUILD
        -- local title, content = mail.getTitleContentBySubType(mailSubType)
        -- local attachments = utils.serialize(msg.drops)
        -- if attachments == nil then attachments = '' end
        -- local param = ''
        -- mail.saveMailRaw(user.uid, 0, p.MailType.SYSTEM, mailSubType, title, content, msg.timestamp, true, true, attachments, param)

        map.mapService:cast("confirmMsg", msgId)
    end,

    onBuffRemove = function(msgId, msg)
        --print("onBuffRemove", msgId, msg)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId]  = true
        buff.remove(msg.type)

        map.mapService:cast("confirmMsg", msgId)
    end,

    onCityPatrol = function(msgId, msg)
        -- print("onCityPatrol", msgId, msg)
        if msgIdFlags[msgId] == true then
            return
        end

        msgIdFlags[msgId]  = true
        
        for _,v in pairs(msg.events) do
            bag.pickDropItems(v.drops)

            for _, remove in pairs(v.removes) do
                local type, count = remove.tplId, remove.count
                if type == p.ResourceType.FOOD then
                    if user.info.food < count then
                        count = user.info.food
                    end
                elseif type == p.ResourceType.WOOD then
                    if user.info.wood < count then
                        count = user.info.wood
                    end
                elseif type == p.ResourceType.IRON then
                    if user.info.iron < count then
                        count = user.info.iron
                    end
                elseif type == p.ResourceType.STONE then
                    if user.info.stone < count then
                        count = user.info.stone
                    end
                elseif type == p.ResourceType.GOLD then
                    if user.info.gold < count then
                        count = user.info.gold
                    end
                elseif type == p.ResourceType.SILVER then
                    if user.info.silver < count then
                        count = user.info.silver
                    end
                end
                remove.count = count
                if count > 0 then
                    user.removeResource(type, count)
                end
            end

            if v.armyList then
                army.onDie(v.armyList)
            end
        end

        local mailSubType = p.MailSubType.REPORT_PATROL_MAIL
        local param = utils.serialize(msg)
        -- print("utils.serialize = ", param)
        -- save mail
        local title, content = mail.getTitleContentBySubType(mailSubType)
        mail.saveMailRaw(user.uid, 0, p.MailType.REPORT, mailSubType, title, content, msg.timestamp, true, true, '', param, 0)

        map.mapService:cast("confirmMsg", msgId)
    end,

    onTransport = function(msgId, msg)
        print("onTransport", msgId, msg.isSuccess, msg.timestamp)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId] = true

        --mail and event
        local mailSubType, isSuccess = 0, false
        -- 运输成功
        if msg.isSuccess then
            if msg.myTransportType == p.TransportType.TRANSMIT then
                
            else
                user.addResource(p.ResourceType.FOOD, msg.food, p.ResourceGainType.TRANSPORT)
                user.addResource(p.ResourceType.WOOD, msg.wood, p.ResourceGainType.TRANSPORT)
                user.addResource(p.ResourceType.IRON, msg.iron, p.ResourceGainType.TRANSPORT)
                user.addResource(p.ResourceType.STONE, msg.stone, p.ResourceGainType.TRANSPORT)
            end
        -- 运输失败
        else

        end
        map.mapService:cast("confirmMsg", msgId)
    end,

    onCompensate = function(msgId, msg)
        print("onCompensate", msgId, msg.timestamp)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId] = true

        local param = utils.serialize(msg)

        --mail
        local mailSubType = p.MailSubType.SYSTEM_COMPENSATE

        local title, content = mail.getTitleContentBySubType(mailSubType)
        local attachments = utils.serialize(msg.drops)
        if attachments == nil then attachments = '' end
        mail.saveMailRaw(user.uid, 0, p.MailType.SYSTEM, mailSubType, title, content, msg.timestamp, true, false, attachments, param, 0)

        map.mapService:cast("confirmMsg", msgId)
    end,

    onCityDefenerFill = function(msgId, msg)
        print("onCityDefenerFill", msgId, msg.timestamp)
        if msgIdFlags[msgId] == true then
            return
        end
        msgIdFlags[msgId] = true
        army.onCityDefenerFill(msg.armyList.team)
        local param = utils.serialize(msg)
        --print("param..",param)

        map.mapService:cast("confirmMsg", msgId)
    end,
}

local pktHandlers = {}

--sync begin

function map.syncAllResource()
    local u = user.info
    map.cast("syncAllResource", u.food, u.wood, u.iron, u.stone, u.gold)
end

function map.syncEmployHero()
    map.cast("syncEmployHero", alliance.employHeroForSync())
end

function map.syncHero()
    -- print("sync ------------ Hero ------------- ")
    map.cast("syncHero", hero.heroSyncToMs())
end

function  map.syncArmy()
    map.cast("syncArmy", army.armyForSync())
end

function  map.syncTeam()
    -- print("sync ------------ Team ------------- ")
    map.cast("syncTeam", army.teamsForSync())
end

function map.syncCastleTeam( ... )
    map.cast("syncCastleTeam", army.castleDefenderSync())
end

function map.syncLordInfo()
    local uinfo = user.info
    map.cast("syncLordInfo", uinfo.attackWins, uinfo.attackLosses, uinfo.defenseWins, uinfo.defenseLosses, uinfo.scoutCount, uinfo.kills, uinfo.losses, uinfo.heals)
end

function map.syncCollectInfo()
    local infos = {}
    for i = 100, 50 do
        local binfo = building.list[i]
        if binfo and binfo.param1 > 0 then
            local tpl = t.building[binfo.tplId]
            if tpl then
                local type = tpl.buildingType
                if type == p.BuildingType.FARM or type == p.BuildingType.SAWMILL or type == p.BuildingType.IRON_MINE or type == p.BuildingType.STONE_MINE then
                    tpl = tpl.levels[binfo.level]
                    if tpl then
                        local attrList = t.findBuildingAttribute(tpl.attrList)
                        if attrList then
                            local info = {}
                            if type == p.BuildingType.FARM then
                                info.type = p.ResourceType.FOOD
                                info.output = attrList[p.AttributeType.FOOD_OUTPUT][p.AttributeAdditionType.BASE]
                                info.capacity = attrList[p.AttributeType.FOOD_CAPACITY][p.AttributeAdditionType.BASE]
                            elseif type == p.BuildingType.SAWMILL then
                                info.type = p.ResourceType.WOOD
                                info.output = attrList[p.AttributeType.WOOD_OUTPUT][p.AttributeAdditionType.BASE]
                                info.capacity = attrList[p.AttributeType.WOOD_CAPACITY][p.AttributeAdditionType.BASE]
                            elseif type == p.BuildingType.IRON_MINE then
                                info.type = p.ResourceType.IRON
                                info.output = attrList[p.AttributeType.IRON_OUTPUT][p.AttributeAdditionType.BASE]
                                info.capacity = attrList[p.AttributeType.IRON_CAPACITY][p.AttributeAdditionType.BASE]
                            elseif type == p.BuildingType.STONE_MINE then
                                info.type = p.ResourceType.STONE
                                info.output = attrList[p.AttributeType.STONE_OUTPUT][p.AttributeAdditionType.BASE]
                                info.capacity = attrList[p.AttributeType.STONE_CAPACITY][p.AttributeAdditionType.BASE]
                            end
                            info.timestamp = binfo.param1
                            infos[i] = info
                        end
                    end
                end
            end
        end
    end
    -- print("infos", utils.serialize(infos))
    map.cast("syncCollectInfo", infos)
end

function map.syncWatchtower(tpl,level)
    if tpl then
        local type = tpl.buildingType
        if type == p.BuildingType.WATCH_TOWER then
            map.mapService:cast("syncWatchtower", level)
        end
    end
end

function map.syncTurretInfo(tpl,level)
    if tpl then
        local type = tpl.buildingType
        if type == p.BuildingType.TURRET then
            map.mapService:cast("syncTurretInfo", level)
        end
    end
end

function map.syncBuffList()
    local cityList, skillList = {}, {}
    for _, info in pairs(buff.cityList) do
        local b = {}
        b.type = info.type
        b.endTimestamp = 0
        local cd = cdlist.getCD(info.cdId)
        if cd then
            b.endTimestamp = cd.endTime
        end
        b.param1 = info.param1
        b.attr = info.attr
        table.insert(cityList, b)
    end

    if #cityList > 0 or #skillList > 0 then
        map.cast("syncBuffList", cityList, skillList)
    end
end

function map.syncPlayerInfo()
    local u = user.info
    map.cast("syncPlayerInfo", u.nickname, u.headId, u.level, u.lastLoginTimestamp, u.langType, u.lordPower, u.troopPower, u.buildingPower, u.sciencePower, u.trapPower, u.heroPower, u.totalPower, u.exp, alliance.allianceCdTime)
end

function map.syncMedals()
    local list = {}
    for _, tpl in pairs(agent.achievement.medals) do
        table.insert(list, tpl.id)
    end
    if #list > 0 then
        map.cast("syncMedals", list)
    end
end

function map.syncTechnologies()
    local list = {}
    for type,techTreeInfo in pairs(technology.techTreeList) do
        if techTreeInfo then
            for groupId,techGroupInfo in pairs(techTreeInfo.techGroupList) do
                local tplId = techGroupInfo.tplId  
                local tpl = t.technology[tplId]
                if tpl and tpl.isScoutDisplay then
                   list[tplId] = techGroupInfo.level
                end
            end
        end
    end
    map.cast("syncTechnologies", list)
    print("syncTechnologies = ", utils.serialize(list))
end

function map.syncSkills()
    local list = {}
    for _, v in pairs(agent.skill.tree) do
        local tpl = t.skill[v.tplId]
        if tpl and tpl.isScoutDisplay then
            list[v.tplId] = v.level
        end
    end
    map.cast("syncSkills", list)
end

function map.syncEquips()
    local list = {}
    for _, v in pairs(bag.blacksmith_items) do
        if v.data.equip then
            table.insert(list, {tplId = v.tpl.id})
        end
    end
    if #list > 0 then
        map.cast("syncEquips", list)
    end
end

function map.syncPlayerSettings()
    map.cast("syncPlayerSettings", agent.misc.data.settings)
end

function map.syncVip()
    -- print("vip.vipLevel() = ", vip.vipLevel())
    if vip.vipLevel() > 0 then
        map.cast("syncVip", vip.vipLevel())
    end
end
--sync end

function map.cast(...)
    if map.mapService then
        map.mapService:cast(...)
        return true
    else
        return false
    end
end

function map.addScoutCount()
    user.info.scoutCount = user.info.scoutCount + 1
    user.sendUpdate()
end

function map.addBattleCount(winner, myAttackType)
    if myAttackType == p.AttackType.ATTACK then
        if winner == p.AttackType.ATTACK then
            user.info.attackWins = user.info.attackWins + 1
        else
            user.info.attackLosses = user.info.attackLosses + 1
        end
    else
        if winner == p.AttackType.ATTACK then
            user.info.defenseLosses = user.info.defenseLosses + 1
        else
            user.info.defenseWins = user.info.defenseWins + 1
        end
    end
    user.sendUpdate()
end

function map.recordInvited(uid)
    map.mapService:cast("recordInvited", uid)
end

--return table : {uid, nickname, headId, langType, level, aid, allianceLevel, allianceName, allianceNickname, vipLevel, allianceCdTimestamp}
function map.call_getPlayerInfo(uid)
    local ok, info = map.mapService:call('getPlayerInfo', uid)
    return info
end

--return AllianceResultType
function map.call_canApplyAlliance(aid)
    local ok, ret = map.mapService:call('canApplyAlliance', aid)
    if not ok then
        ret = p.AllianceResultType.UNKNOWN
    end
    return ret
end

function map.checkMsExist(k)
    if map.mapService then
        return map.mapService:checkMsExist(k)
    end
    return false
end

function map.advancedTeleport(x, y)
    -- print("advancedTeleport")
    local ok = false
    local burnEndTs = 0
    --TODO begin k is pktin read
    local k = utils.getMapServiceId()
    -- end
    if k == utils.getMapServiceId() then
        ok,burnEndTs = map.mapService:call("advancedTeleport", x, y)
        print("ok = ", ok)
        if ok then
            info.x = x
            info.y = y
            map.evtTeleport:trigger(x, y, true)
            burnEndTimestamp = burnEndTs
        else
            agent.sendNoticeMessage(p.ErrorCode.AMI_POSITION_CANNOT_TELEPORT, '', 1)
        end
        agent.sendPktout(p.SC_MAP_TELEPORT_RESPONSE, ok)
    -- else
    --     if true then
    --         return
    --     end
    --     --check level and time
    --     --notice local mapServer : check troop and delete agent and castle
    --     --TODO map.mapService:call()
    --     --quit alliance
    --     if alliance.info ~= nil and alliance.info.id ~= 0 then
    --         alliance.quit()
    --     end
    --     --notice all component
    --     agent.onCrossTeleport()
    --     --get data from comp
    --     local userInfos = agent.user.getCrossTeleportInfo()
    --     local castleLevel = agent.building.getCrossTeleportInfo()
    --     local dictInfos = agent.dict.getCrossTeleportInfo()
    --     local bagInfos = agent.bag.getCrossTeleportInfo()
    --     local questInfos = agent.quest.getCrossTeleportInfo()
    --     --send all data to map (is not call
    --     map.mapService:crossTeleport(k, x, y, castleLevel, userInfos, dictInfos, bagInfos, questInfos)
    end
    return ok
end

function map.randomTeleport()
    --print("randomTeleport")
    local ok, x, y = map.mapService:call("randomTeleport")
    --print("ok = ", ok, x, y)
    if ok then
        info.x = x
        info.y = y
        map.evtTeleport:trigger(x, y, false)
    end
    agent.sendPktout(p.SC_MAP_TELEPORT_RESPONSE, ok)
    return ok
end

function map.calculateKillPower(msg)
    -- --print('calculateKillPower')
    -- local killPowerTotal, myKills, killsTotal = 0, 0, 0
    -- local ourDetails, theirDetails
    -- if msg.myAttackType == p.AttackType.ATTACK then
    --     ourDetails = msg.attacker.details
    --     theirDetails = msg.defender.details
    -- else
    --     ourDetails = msg.defender.details
    --     theirDetails = msg.attacker.details
    -- end

    -- if ourDetails and theirDetails then
    --     for _, detail in pairs(theirDetails) do
    --         if detail.uid > 0 and not detail.isCaptive then
    --             for tplId, states in pairs(detail.armyList) do
    --                 local armyTpl = t.army[tplId]
    --                 if armyTpl and not t.isTrap(armyTpl.type) then
    --                     local die = states[p.ArmyState.DIE] or 0
    --                     local wounded = states[p.ArmyState.WOUNDED] or 0
    --                     local loss = die + wounded
    --                     if loss > 0 then
    --                         killPowerTotal = killPowerTotal + loss * armyTpl.power
    --                     end
    --                 end
    --             end
    --         end
    --     end

    --     for _, detail in pairs(ourDetails) do
    --         for tplId, states in pairs(detail.armyList) do
    --             local kill = states[p.ArmyState.KILL] or 0
    --             if kill > 0 then
    --                 if detail.uid == user.uid then
    --                     myKills = myKills + kill
    --                 end
    --                 killsTotal = killsTotal + kill
    --             end
    --         end
    --     end

    --     if killPowerTotal <= 0 and killsTotal <= 0 then return end
    --     local rate = myKills / killsTotal
    --     if rate <= 0 then return end
    --     --print('killPowerTotal, myKills, killsTotal', math.floor(killPowerTotal), myKills, killsTotal)

    --     local killPower = math.floor(killPowerTotal * rate)
    --     if killPower > 0 then
    --         --print('my killPower', killPower)
    --         map.evtKillPower:trigger(killPower)
    --     end
    -- end
end

function map.sendNeutralCastlePropertyUpdate()
    local temp = {}
    for k, v in pairs(map.ncProperty) do
        table.insert(temp, {type=k, v=v})
    end
    agent.sendPktout(p.SC_MAP_NEUTRAL_CASTLE_PROPERTY_UPDATE, '@@1=[type=i,v=f]', temp)
end

-- db begin
function map.dbLoad()
    local data = dict.get('map.data')
    if data then
        local dataInfo = data.info
        info.sendFirstCastleBattle = dataInfo.sendFirstCastleBattle

        if data.bookmarkList then
            for k,v in pairs(data.bookmarkList) do
                map.bookmarkList[v.id] = bookmarkInfo:new(v)
            end
        end
    end
end

function map.dbSave()
    local data = {
        info={}, 
        bookmarkList = {}, 
    }

    data.info = {
        sendFirstCastleBattle = info.sendFirstCastleBattle,
    }
    for k,v in pairs(map.bookmarkList) do
        table.insert(data.bookmarkList, { id = v.id, type = v.type, name = v.name, x = v.x, y = v.y })
    end
    dict.set('map.data', data)
end
-- db end

function map.sendCanMarchMonsterLevelUpdate()
    map.mapService:cast("sendCanMarchMonsterLevelUpdate")
end

function map.onInit()
    agent.registerHandlers(pktHandlers)
    map.dbLoad()

    activity = agent.activity
end

function map.onAllCompInit()
    map.mapService = agent.connectMapService(user.uid)
    if not map.mapService then
        agent.sendKick(p.KickType.FOUND_ERROR)
        return
    end
    map.mapService:setMapEvtHandler(mapEventHandler)
    map.sendBookmarkUpdate()
    map.sendCanMarchMonsterLevelUpdate()
    --print("map.onAllCompInit---sendCanMarchMonsterLevelUpdate")
    info.k = utils.getMapServiceId()
    if info.x == 0 or info.y == 0 then
        local uinfo = user.info
        local ok, x, y, burnEndTs, needSync, teams = map.mapService:call("initCastle", user.uid, uinfo.nickname, uinfo.headId, building.castleLevel(), uinfo.registerTimestamp)
        -- print('map.onAllCompInit----teams', utils.serialize(teams))
        if ok then
            info.x = x
            info.y = y
            info.castleTpl = t.findMapUnit(p.MapUnitType.CASTLE, building.castleLevel())
            burnEndTimestamp = burnEndTs
            -- print("luaCall initCastle", info.x, info.y, burnEndTimestamp, needSync)

            --行军检查
            -- army.checkTroopException(teams)

            map.syncEmployHero()
            map.syncHero()
            map.syncArmy()
            map.syncTeam()
            map.syncAllResource()
            map.syncPlayerInfo()
            map.syncCastleTeam()

            --syncCollectInfo
            map.syncCollectInfo()

            --syncBuilding
            for _, v in pairs(building.list) do
                local tpl = t.building[v.tplId]
                if tpl then
                    local type = tpl.buildingType
                    if type == p.BuildingType.WATCH_TOWER then
                        map.syncWatchtower(tpl, v.level)
                    elseif type == p.BuildingType.TURRET then
                        map.syncTurretInfo(tpl, v.level)
                    end
                end
            end

            --buffs
            map.syncBuffList()
            map.syncTechnologies()
            map.syncLordInfo()
            map.syncVip()

            --[[
            map.ncProperty = ncPropertyList
            map.syncSkills()
            map.syncMedals()
            map.syncPlayerSettings()
            map.syncEquips()
            map.sendNeutralCastlePropertyUpdate()

            --check marching army
            if troopSize == 0 then
                checkTroopTimer = timer.setTimeout(function()
                    army.checkTroopException()
                end, 1 * 1000)
            end
            ]]--
        else
            map.mapService:cast("onOffline")
            agent.sendKick(p.KickType.FOUND_ERROR)
            return
        end
    else
        --print("Castle", info.x, info.y)
    end

    --evt
    user.evtResourceAdd:attachRaw(map.syncAllResource)
    user.evtResourceRemove:attachRaw(map.syncAllResource)

    building.evtBuildingLevelUp:attachRaw(function(tplId, level)
        local tpl = t.building[tplId]
        if tpl then
            local type = tpl.buildingType
            if type == p.BuildingType.CASTLE then
                map.mapService:cast("onCastleLevelUp", level)
                info.castleTpl = t.findMapUnit(p.MapUnitType.CASTLE, level)
            elseif type == p.BuildingType.WATCH_TOWER then
                map.syncWatchtower(tpl, level)
            elseif type == p.BuildingType.FARM or type == p.BuildingType.SAWMILL or type == p.BuildingType.IRON_MINE or type == p.BuildingType.STONE_MINE then
                map.syncCollectInfo()
            elseif type == p.BuildingType.TURRET then
                map.syncTurretInfo(tpl, level)
            end
        end
    end)

    building.evtBuildingDemolish:attachRaw(function(tplId)
        local tpl = t.building[tplId]
        if tpl then
            local type = tpl.buildingType
            if type == p.BuildingType.FARM or type == p.BuildingType.SAWMILL or type == p.BuildingType.IRON_MINE or type == p.BuildingType.STONE_MINE then
                map.syncCollectInfo()
            end
        end
    end)

    army.evtArmyAdd:attachRaw(function(armyType, state, count)
        if state == p.ArmyState.NORMAL then
            map.syncArmy()
        end
    end)

    army.evtArmySub:attachRaw(function(armyType, state, count)
        if state == p.ArmyState.NORMAL then
            map.syncArmy()
        end
    end)

    army.evtTeamChange:attachRaw(function()
        map.syncTeam()
    end)

    army.evtCastleDefenderChange:attachRaw(function()
        map.syncCastleTeam()
    end)

    hero.evtAddHero:attachRaw(function(heroId)
        map.syncHero()
    end)

    user.evtNicknameChange:attachRaw(function()
       map.syncPlayerInfo()
       map.cast("updateTroops")
       map.cast("updateMyUnits")
    end)

    user.evtHeadIdChange:attachRaw(function()
       map.syncPlayerInfo()
    end)

    user.evtLevelUp:attachRaw(function()
       map.syncPlayerInfo()
    end)

    user.evtTotalPowerChange:attachRaw(map.syncPlayerInfo)

    technology.evtAddLevel:attachRaw(map.syncTechnologies)

    vip.evtUpgrade:attachRaw(map.syncVip)

    alliance.evtEmployHeroUpdate:attachRaw(map.syncEmployHero)

    --[[
    bag.evtPutOn:attachRaw(map.syncEquips)
    bag.evtTakeOff:attachRaw(map.syncEquips)

    skill.evtSkillReset:attachRaw(map.syncSkills)
    skill.evtSkillAddPoint:attachRaw(map.syncSkills)

    agent.achievement.evtMedalGain:attachRaw(map.syncMedals)
    --]]
end

function map.onSave()
    map.dbSave()
end

function map.onClose()
    map.dbSave()
    if map.mapService then
        map.mapService:quitAllMap()
    end
end

function map.onTimerUpdate(timerIndex)
    if timerIndex % 30 == 0 then
        -- map.syncAllResource()
        -- map.mapService:cast("syncArmy", army.listSyncToMs())
    end
end

function map.sendBookmarkUpdate(removeList)
    local list = {}
    local removes = removeList or {}
    for _, v in pairs(map.bookmarkList) do
        if not v.sync then
            v.sync = true
            -- print("bookmark id, name, type, x, y = ", v.id, v.name, v.type, v.x, v.y)
            table.insert(list, { id = v.id, name = v.name, type = v.type, x = v.x, y = v.y })
        end
    end
    if next(list) or next(removes) then
        agent.sendPktout(p.SC_BOOKMARK_UPDATE,  '@@1=[id=i,name=s,type=i,x=i,y=i],2=[id=i]',  list, removes)
    end
end

pktHandlers[p.CS_MAP_JOIN] = function(pktin)
    if map.mapService then
        map.mapService:forward(pktin)
        -- print("pktHandlers[p.CS_MAP_JOIN]")
    end
end

pktHandlers[p.CS_MAP_LEAVE] = function(pktin)
    if map.mapService then
        map.mapService:forward(pktin)
    end
end

pktHandlers[p.CS_MAP_VIEW] = function(pktin)
    if map.mapService then
        map.mapService:forward(pktin)
    end
end

pktHandlers[p.CS_MAP_SEARCH] = function(pktin)
    if map.mapService then
        map.mapService:forward(pktin)
    end
end

pktHandlers[p.CS_MAP_MARCH] = function(pktin)
    local type, x, y, team = pktin:read("iiii")
    -- 判断是否可以出征
    if not army.checkTeamCanMarch(team) and type ~= p.MapTroopType.CAMP_FIXED_DISMANTLE and type ~= p.MapTroopType.SCOUT then
        agent.sendNoticeMessage(p.ErrorCode.HERO_ARMYCOUNT_CANNOT_ZERO, '', 1)
        print("armyCount is not enough...team = ", team)
        return
    end
    -- local size, armylist = pktin:readInteger(), {}
    -- for i = 1, size do
    --     local heroId = pktin:readInteger()
    --     local armyType = pktin:readInteger()
    --     local level = pktin:readInteger()
    --     local count = pktin:readInteger()
    --     local position = pktin:readInteger()
    --     if count > 0 then
    --         armylist[heroId] = { heroId = heroId, armyType = armyType, level = level, count = count, position = position }
    --     end
    -- end
    print('CS_MAP_MARCH...type, x, y, team', type, x, y, team)
    -- local propertyList = agent.property.list
    local speed = 20    --默认一个比较大的值
    local manualCost = 0
    local foodCost = 0
    local woodCost = 0
    local stoneCost = 0
    local ironCost = 0
    if type == p.MapTroopType.MONSTER then
        manualCost = t.configure["energyConsume"].monster
        if not user.isEnergyEnough(manualCost) then
            print("Energy is not Enough")
            agent.sendNoticeMessage(p.ErrorCode.PUBLIC_ENERGY_NOT_ENOUGH, '', 1)
            return
        end
    elseif type == p.MapTroopType.WORLDBOSS then
        manualCost = t.configure["energyConsume"].worldboss
        if not user.isEnergyEnough(manualCost) then
            print("Energy is not Enough")
            agent.sendNoticeMessage(p.ErrorCode.PUBLIC_ENERGY_NOT_ENOUGH, '', 1)
            return
        end
    elseif type == p.MapTroopType.SCOUT then
        foodCost = t.configure['scoutConsumed'].food;
        if not user.isResourceEnough(p.ResourceType.FOOD, foodCost) then
            print("scout food not enough")
            return
        end
    elseif type == p.MapTroopType.CAMP_FIXED then
        foodCost = t.configure['campFixedConsumed'].food
        woodCost = t.configure['campFixedConsumed'].wood
        stoneCost = t.configure['campFixedConsumed'].stone
        ironCost = t.configure['campFixedConsumed'].iron
        if not user.isResourceEnough(p.ResourceType.FOOD, foodCost) then
            agent.sendNoticeMessage(p.ErrorCode.PUBLIC_RESOURCE_NOT_ENOUGH, '', 1)
            print("CAMP_FIXED FOOD not enough")
            return
        end
        if not user.isResourceEnough(p.ResourceType.WOOD, woodCost) then
            agent.sendNoticeMessage(p.ErrorCode.PUBLIC_RESOURCE_NOT_ENOUGH, '', 1)
            print("CAMP_FIXED WOOD not enough")
            return
        end
        if not user.isResourceEnough(p.ResourceType.STONE, stoneCost) then
            agent.sendNoticeMessage(p.ErrorCode.PUBLIC_RESOURCE_NOT_ENOUGH, '', 1)
            print("CAMP_FIXED STONE not enough")
            return
        end
        if not user.isResourceEnough(p.ResourceType.IRON, ironCost) then
            agent.sendNoticeMessage(p.ErrorCode.PUBLIC_RESOURCE_NOT_ENOUGH, '', 1)
            print("CAMP_FIXED IRON not enough")
            return
        end

        if user.info.level < t.configure['campFixedConsumed'].level then
            agent.sendNoticeMessage(p.ErrorCode.USER_LEVEL_NOT_ENOUGH, '', 1)
            print("CAMP_FIXED user level not enough")
            return
        end
    elseif type == p.MapTroopType.SIEGE_CITY then 
    
    end
    --判断兵种数量是否足够 必须判断
    -- if type ~= p.MapTroopType.SCOUT and type ~= p.MapTroopType.CAMP_FIXED_DISMANTLE then
    --     if not army.teamCanMarchForMap(team,type) then
    --         agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
    --         print("army is not enough...team = ", team)
    --         return
    --     end
    -- end
    agent.queueJob(function()
	-- print("maptrooptype = ", type)
        local ok = map.mapService:call("march", type, x, y, team)
        if ok then
            if manualCost > 0 then
                user.removeEnergy(manualCost,p.EnergyConsumeType.ATTACK_MOSTER)
            end

            local resourceConsumeType = p.ResourceConsumeType.DEFAULT
            if type == p.MapTroopType.SCOUT then
                resourceConsumeType= p.ResourceConsumeType.SCOUT
            elseif type == p.MapTroopType.CAMP_FIXED then
                resourceConsumeType= p.ResourceConsumeType.CAMP_FIXED
            end

            if foodCost > 0 then
                user.removeResource(p.ResourceType.FOOD, foodCost, resourceConsumeType)
            end
            if woodCost > 0 then
                user.removeResource(p.ResourceType.WOOD, woodCost, resourceConsumeType)
            end
            if stoneCost > 0 then
                user.removeResource(p.ResourceType.STONE, stoneCost, resourceConsumeType)
            end
            if ironCost > 0 then
                user.removeResource(p.ResourceType.IRON, ironCost, resourceConsumeType)
            end
        else
            --出征失败
            -- army.teamCancle(team)
        end
    end)
end

pktHandlers[p.CS_MAP_GO_HOME] = function(pktin)
    map.mapService:forward(pktin)
end

pktHandlers[p.CS_MAP_SPEED_UP]  = function(pktin)
        local troopId = pktin:read('i')
        if troopId then
            -- print('SPEEDUP', troopId, tpl.param1)
            map.cast("speedUp", troopId, 50 / 100)
        end
end

pktHandlers[p.CS_MAP_RECALL]  = function(pktin)
        local troopId = pktin:read('i')
        if troopId then
            -- print('recall', troopId, tpl.param1)
            map.cast("recall", troopId)
        end
end

pktHandlers[p.CS_MAP_WALL_REPAIR] = function(pktin)
    local isGoldRepair = pktin:read('b')
    agent.queueJob(function()
        local cityDefenseMax = agent.property.list.cityDefense
        if cityDefenseMax > cityDefense then
            if isGoldRepair then
                local gold = math.ceil(100 * (cityDefenseMax - cityDefense)/cityDefenseMax * t.configure["WallsRecovery"].gold)
                -- print("WallsRecovery gold is", gold)
                if user.removeResource(p.ResourceType.GOLD, gold, p.ResourceConsumeType.WALL_REPAIR) then
                    local ok = map.mapService:call("wallRepair", isGoldRepair)
                    if not ok then
                        user.addResource(p.ResourceType.GOLD, gold)
                    end
                else
                    print("gold is not enough", gold)
                end
            else
                local silver = t.configure["WallsRecovery"].price
                if user.removeResource(p.ResourceType.SILVER, silver, p.ResourceConsumeType.WALL_REPAIR) then
                    local ok = map.mapService:call("wallRepair", isGoldRepair)
                    if not ok then
                        user.addResource(p.ResourceType.SILVER, silver)
                    end
                else
                    print("silver is not enough", silver)
                end
            end
        end
    end)
end

pktHandlers[p.CS_MAP_WALL_OUTFIRE] = function(pktin)
    agent.queueJob(function()
        local time = burnEndTimestamp - timer.getTimestampCache()
        if time > 0 then
            local gold = math.ceil(time / 60 * t.configure["WallsValue"].gold)
            if user.removeResource(p.ResourceType.GOLD, gold, p.ResourceConsumeType.WALL_OUTFIRE) then
                local ok = map.mapService:call("wallOutfire")
                if ok then
                    burnEndTimestamp = 0
                else
                    user.addResource(p.ResourceType.GOLD, gold)
                end
            else
                print("WALL_OUTFIRE gold is not enough", time, gold)
            end
        else
            print("WALL_OUTFIRE time <= 0", burnEndTimestamp, timer.getTimestampCache())
        end
    end)
end

pktHandlers[p.CS_MAP_ALLIANCE_INVITE_LIST] = function(pktin)
    map.mapService:forward(pktin)
end


-- bookmark
pktHandlers[p.CS_BOOKMARK_ADD] = function(pktin, session)
    local function bookmarkAddResponse(result)
        -- print("map bookmarkAddResponse", result)
        agent.replyPktout(session, p.SC_BOOKMARK_ADD_RESPONSE, result)
    end

    local name, type, x, y = pktin:read("siii")
    if x < 1 or x > 1200 or y < 1 or y > 1200 then
        print("invalid point, x, y=", x, y)
        bookmarkAddResponse(false)
        return
    end

    local info = bookmarkInfo:new({ name = name, type = type, x = x, y = y })
    map.bookmarkList[info.id] = info

    map.sendBookmarkUpdate()
    bookmarkAddResponse(true)
end

pktHandlers[p.CS_BOOKMARK_EDIT] = function(pktin, session)
    local function bookmarkEditResponse(result)
        -- print("map bookmarkEditResponse", result)
        agent.replyPktout(session, p.SC_BOOKMARK_EDIT_RESPONSE, result)
    end

    local id, name, type = pktin:read("isi")

    local info = map.bookmarkList[id]
    if not info then
        print("bookmark not exist, id=", id)
        bookmarkEditResponse(false)
        return
    end

    info.name = name
    info.type = type
    info.sync = false

    map.sendBookmarkUpdate()
    bookmarkEditResponse(true)
end

pktHandlers[p.CS_BOOKMARK_REMOVE] = function(pktin, session)
    local function bookmarkRemoveResponse(result)
        -- print("map bookmarkRemoveResponse", result)
        agent.replyPktout(session, p.SC_BOOKMARK_REMOVE_RESPONSE, result)
    end

    local id = pktin:read("i")

    local info = map.bookmarkList[id]
    if not info then
        print("bookmark not exist, id=", id)
        bookmarkRemoveResponse(false)
        return
    end
    map.bookmarkList[id] = nil

    local list = {}
    table.insert(list, { id = id })

    map.sendBookmarkUpdate(list)
    bookmarkRemoveResponse(true)
end

pktHandlers[p.CS_MAP_QUERY_PLAYERINFO] = function(pktin)
    local uid = pktin:read("i")
    if uid then
        local list = rangeStub.fetchUserRankByUid(uid) or {}
        -- print('p.CS_MAP_QUERY_PLAYERINFO...', uid, utils.serialize(list))
        map.cast("queryPlayerInfo", uid, list)
    end
end

pktHandlers[p.CS_MAP_GET_TROOP_EXTRA_INFO] = function(pktin)
    map.mapService:forward(pktin)
end

pktHandlers[p.CS_MAP_RECALL_ALLIANCE_TROOP]  = function(pktin)
        local troopId = pktin:read('i')
        agent.queueJob(function()
            if troopId then
                print('###CS_MAP_RECALL_ALLIANCE_TROOP', troopId)
                local ok = map.mapService:call("recallAllianceTroop", troopId)
            end
        end)
end

--[[
pktHandlers[p.CS_MAP_TELEPORT] = function(pktin, session)
    if true then
        return
    end
    local x, y = pktin:read("ii")
    agent.queueJob(function()
        --print("teleport")
        local ok = map.mapService:call("teleport", x, y)
        --print("ok = ", ok)
        if ok then
            info.x = x
            info.y = y
            map.evtTeleport:trigger(x, y)
        end
        agent.replyPktout(session, p.SC_MAP_TELEPORT_RESPONSE, ok)
    end)
end

pktHandlers[p.CS_MAP_DECLARE_CANCEL] = function(pktin)
    map.mapService:forward(pktin)
end

pktHandlers[p.CS_MAP_REPATRIATE] = function(pktin)
    map.mapService:forward(pktin)
end

pktHandlers[p.CS_MAP_ALLIANCE_INVITE_LIST_BY_NAME] = function(pktin)
    map.mapService:forward(pktin)
end

pktHandlers[p.CS_MAP_GET_ALLIES_EXTRA_INFOS] = function(pktin)
    map.mapService:forward(pktin)
end

pktHandlers[p.CS_MAP_ALLIANCE_UNIT_TELEPORT] = function(pktin, session)
    local fromX, fromY, toX, toY = pktin:read('iiii')
    agent.queueJob(function()
        --print("allianceUnitTeleport")
        local ok = false
        local tpl = t.configure['neutralCastle']
        if user.removeResource(p.ResourceType.GOLD, tpl.teleportCost, p.ResourceConsumeType.ALLIANCE_UNIT_TELEPORT) then
            ok = map.mapService:call("allianceUnitTeleport", fromX, fromY, toX, toY)
        end
        agent.replyPktout(session, p.SC_MAP_ALLIANCE_UNIT_TELEPORT_RESPONSE, ok)
    end)
end

pktHandlers[p.CS_MAP_NEUTRAL_CASTLE_REPATRIATE] = function(pktin)
    map.mapService:forward(pktin)
end

pktHandlers[p.CS_MAP_GET_ALLIANCE_TERRITORY_LIST] = function(pktin)
    map.mapService:forward(pktin)
end

pktHandlers[p.CS_MAP_GET_ALLIANCE_TERRITORY_INFO] = function(pktin)
    map.mapService:forward(pktin)
end

pktHandlers[p.CS_MAP_KINGMAP_UNIT_POSITION] = function(pktin)
    map.mapService:forward(pktin)
end

pktHandlers[p.CS_MAP_KINGDOM_INFO] = function(pktin)
    map.mapService:forward(pktin)
end

pktHandlers[p.CS_MAP_NEUTRAL_CASTLE_CHANGE_NAME] = function(pktin, session)
    local function response(result, x, y, name)
        if result == 0 then
            agent.replyPktout(session, p.SC_MAP_NEUTRAL_CASLTE_CHANGE_NAME_RESPONSE, result, x, y, name)
        else
            agent.replyPktout(session, p.SC_MAP_NEUTRAL_CASLTE_CHANGE_NAME_RESPONSE, result)
        end
    end
    local info = alliance.info
    if info == nil or info.id == 0 then
        print('CS_MAP_NEUTRAL_CASTLE_CHANGE_NAME...p.AllianceResultType.NOT_FOUND_ALLIANCE')
        response(1)
        return
    end
    local myself = info.memberList[user.uid]
    if myself == nil or not myself:haveRankPermit(p.AllianceRankPermitType.NEUTRAL_CASTLE_CHANGE_NAME) then
        print('CS_MAP_NEUTRAL_CASTLE_CHANGE_NAME...p.AllianceResultType.WITHOUT_PERMIT')
        response(2)
        return
    end
    local x, y, name = pktin:read('iis')
    local length = string.len(name)
    if length < 3 or length > 18 or trie.isContain(name) then
        print('contain bad word', name)
        response(3)
        return
    end

    agent.queueJob(function()
        local ok, res = map.mapService:call('changeNeutralCastleName', x, y, name)
        if ok then
            response(res, x, y, name)
        end
    end)
end

pktHandlers[p.CS_MAP_GET_NEAREST_UNIT] = function(pktin)
    map.mapService:forward(pktin)
end

pktHandlers[p.CS_MAP_GET_MARCH_DISTANCE] = function(pktin)
    map.mapService:forward(pktin)
end

pktHandlers[p.CS_MAP_CHECK_UNIT_CAN_PLACE] = function(pktin)
    map.mapService:forward(pktin)
end

pktHandlers[p.CS_MAP_GET_MYSTERIOUS_CITY_INFO] = function(pktin)
    map.mapService:forward(pktin)
end

pktHandlers[p.CS_MAP_GET_GOBLIN_CAMP_INFO] = function(pktin)
    map.mapService:forward(pktin)
end
--]]
return map

