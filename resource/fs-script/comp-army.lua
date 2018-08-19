local agent = ...
local t = require('tploader')
local timer = require('timer')
local utils = require('utils')
local p = require('protocol')
local event = require('libs/event')
local logStub = require('stub/log')
local allianceStub = require('stub/alliance')

local user = agent.user
local dict = agent.dict
local hero = agent.hero
local alliance = agent.alliance
local building
local map

local lastUpkeepTime = timer.getTimestampCache()
local upkeepInterval = t.configure["upkeepInterval"]
local dieSoldiersPro = t.configure["WallsValue"].soldiers

local army = {
    armies = {},    --map<armyType, armyInfo>
    teams = {},     --map<team, teamInfo> ***之前用于保存成功出征的队伍信息***，现在改成用于保存部队编组信息
    
    myDefenerTeam = 0, --城防出战队伍  参加了城防战斗时赋值为队伍id
    MAX_TEAM = 5,

    evtArmyAdd = event.new(), --(armyType, state, count, ArmyGainType)
    evtArmySub = event.new(), --(armyType, state, count, ArmyConsumeType)

    evtTeamChange = event.new(), --()
    evtTraining = event.new(), --(armyType, count)
    evtTrainFinished = event.new(), --(armyType, count)
    evtCastleDefenderChange = event.new(), --()
    evtHealArmy = event.new(),    -- (count)
}


--兵种信息
local armyInfo = {
    armyType = 0,   --兵种类型 ArmyType
    level = 0,      --兵种等级
    states = {},    --map<state,count> --MARCHING弃用转点将台

    sync = false,
}

--点将台队伍信息
local teamInfo = {
    team = 0,       --队伍序号
    troopId = 0,
    troopType = 0,  --MapTroopType(采集 打怪...)
    teamState = 1,  --MapTeamState(锁定 闲置 守城 出征...) //初始状态是锁定的
    x = 0,
    y = 0,
    srcX = 0,    -- 行营或驻扎地原坐标(TODO:最好是ms传回来)
    srcY = 0,
    confList = {},  --队伍配置列表 map<heroId, table<heroId=i, armyType=i, count=i, position=i> >
                    --heroId武将ID, armyType兵种类型(ArmyType), count士兵数量, position站位(九宫格)
    sync = false,
    -- isNeedSyncToMap = false,    -- 是否需要同步更新到map
}

function armyInfo:new(o)
    o = o or {}
    if o.states == nil then o.states = {} end
    setmetatable(o, self)
    self.__index = self
    return o
end

function teamInfo:new(o)
    o = o or {}
    if o.confList == nil then o.confList = {} end
    setmetatable(o, self)
    self.__index = self
    return o
end

function army.onInit()
    building = agent.building
    map = agent.map
    army.dbLoad()

    if user.newPlayer then
        --新手伤兵
        local conf = t.configure["initialWounded"] or {}
        for armyType, count in pairs(conf) do
            army.add(armyType, p.ArmyState.WOUNDED, count, p.ArmyGainType.INIT, false)
        end
        -- 新手正常状态的兵
        conf = t.configure["initialArmy"] or {}
        for armyType, count in pairs(conf) do
            army.add(armyType, p.ArmyState.NORMAL, count, p.ArmyGainType.INIT, false)
        end
        local i = 1
        while ( i <= army.MAX_TEAM )
        do
            -- print("create a team = ", i)
            army.teams[i] = teamInfo:new({
                        team = i,
                        troopId = 0,
                        troopType = p.MapTroopType.UNKNOWN,
                        x = 0,
                        y = 0,
                        srcX = 0,
                        srcY = 0,
                        teamState = p.TeamState.LOCKED,
                        confList = {},
                        sync = false,
                    })
            i = i + 1;
        end
        -- 初始化第一个点将台状态是开启的
        army.teams[1].teamState = p.TeamState.IDLE
    end

end

function army.onAllCompInit()
    army.upKeep()
    army.sendArmyInfosUpdate()
    army.sendTeamInfosUpdate()
    -- print('###army.onAllCompInit....')
    -- army.dump()
end

function army.onClose()
    army.dbSave()
end

function army.onSave()
    army.dbSave()
end

function army.onTimerUpdate(timerIndex)
    if timerIndex % upkeepInterval == 0 then
        army.upKeep()
    end
end

function army.dbLoad()
    local data = dict.get("army.armies")
    -- print('####army.dbLoad...', utils.serialize(data))
    if data then
        lastUpkeepTime = data.lastUpkeepTime or lastUpkeepTime
        local lists = data.infos or {}
        for _, info in pairs(lists) do
             local level = army.getArmyLevelByArmyType(info.armyType)
             local tpl = t.findArmysTpl(info.armyType, level)
             if tpl and tpl.nJobType > 0 then
                army.armies[info.armyType] = armyInfo:new({
                    armyType = info.armyType,
                    level = level,
                    states = info.states,
                    sync = false,
                })
             end
        end
    end

    local teams = dict.get("army.teams")
    if teams then
        for _, v in pairs(teams) do
            army.teams[v.team] = teamInfo:new({
                    team = v.team,
                    troopId = v.troopId,
                    troopType = v.troopType,
                    x = v.x,
                    y = v.y,
                    srcX = v.srcX,
                    srcY = v.srcY,
                    teamState = v.teamState,
                    confList = v.confList,
                    sync = false,
                })
        end
    end

    army.myDefenerTeam = dict.get("army.myDefenerTeam") or 0
end

function army.dbSave()
    --army
    local data = {}
    data.lastUpkeepTime = lastUpkeepTime
    data.infos = {}
    for k, v in pairs(army.armies) do
        local info = {}
        info.armyType = k
        info.level = v.level
        info.states = v.states
        table.insert(data.infos, info)
    end
    -- print('####army.dbSave...armies...', utils.serialize(data))
    dict.set("army.armies", data)

    -- print('####army.dbSave...teams...', utils.serialize(army.teams))
    --team
    local teams = {}
    for k, v in pairs(army.teams) do
        if v.team > 0 then
            local info = {}
            info.team = k
            info.troopId = v.troopId
            info.troopType = v.troopType
            info.x = v.x
            info.y = v.y
            info.srcX = v.srcX
            info.srcY = v.srcY
            info.teamState = v.teamState
            info.confList = v.confList
            table.insert(teams, info)
        end
    end
    -- print('####army.dbSave...teams...', utils.serialize(teams))
    dict.set("army.teams", teams)
    dict.set("army.myDefenerTeam", army.myDefenerTeam)

    dict.dbSaveByKey("army.armies")
    dict.dbSaveByKey("army.teams")
    dict.dbSaveByKey("army.myDefenerTeam")
end

function army.upKeep()
    local food = 0
    local interval = timer.getTimestampCache() - lastUpkeepTime
    if interval > 0 then
        --城内士兵
        for _, info in pairs(army.armies) do
            local level = army.getArmyLevelByArmyType(info.armyType)
            local tpl = t.findArmysTpl(info.armyType, level)
            if tpl then
                local normal = info.states[p.ArmyState.NORMAL] or 0
                food = food + tpl.foodConsumption * normal
            end
        end
        --出征士兵
        for _, info in pairs(army.teams) do
            if info.troopType > p.MapTroopType.UNKNOWN then
                for _, conf in pairs(info.confList) do
                    local level = army.getArmyLevelByArmyType(conf.armyType)
                    local tpl = t.findArmysTpl(conf.armyType, level)
                    if tpl then
                        local normal = conf.count or 0
                        food = food + tpl.foodConsumption * normal
                    end
                end
            end
        end

        food = math.ceil((food * interval / 3600) * (1 - agent.property.list.upkeepReducePct))
        -- print('###army.upKeep...interval, food', interval, food)
        if food > user.info.food then
            food = user.info.food
        end
        user.removeResource(p.ResourceType.FOOD, food, p.ResourceConsumeType.ARMY_UPKEEP)
        lastUpkeepTime = timer.getTimestampCache()
    end
end

function army.isTrap(armyType)
        return armyType == p.ArmysType.ROLLING_LOGS or armyType == p.ArmysType.GRIND_STONE or armyType == p.ArmysType.CHEVAL_DE_FRISE or armyType == p.ArmysType.KEROSENE
end

function army.getTrapsTotal()
    local total = 0
    for armyType, v in pairs(army.armies) do
        if army.isTrap(armyType) then
            total = total + (v.states[p.ArmyState.NORMAL] or 0)
        end
    end
    return total
end

--TODO:获取兵种等级
function army.getArmyLevelByArmyType(armyType)
    -- 先默认level等于1
    local level = 1
    local exp = 0
    local cities = {}
    if agent.alliance.info == nil then
        return level,exp
    end
    local aid = agent.alliance.info.id
    if aid == 0 or aid == nil then
        return level,exp
    end
    cities = allianceStub.allAllianceCity[aid]
    if not cities then
        return level,exp
    end
    local loseBuffCities = allianceStub.uselessBuffCity[aid]
    -- print("#army.getArmyLevelByArmyType,cities", utils.serialize(cities))
    -- 根据获得的名城来获得当前兵种经验
    for _,v in pairs (cities) do
        local isDisable = false
        if loseBuffCities ~= nil then
            for _,m in pairs (loseBuffCities) do
                if v == m then
                    isDisable = true
                end
            end
        end
        local tpl = t.mapCity[v]
        if not isDisable and tpl and tpl.ArmExperience[armyType] then
            -- print("tpl.ArmExperience[armyType], exp,armyType",tpl.ArmExperience[armyType],armyType)
            exp = exp + (tpl.ArmExperience[armyType] or 0)
        end
    end
    level = t.findArmysLevel(armyType, exp)
    -- print("army.getArmyLevelByArmyType", level,exp)
    return level,exp
end

function army.getArmyCount(armyType, state)
    local info = army.armies[armyType]
    --local info = army.armies[p.ArmysType.REDIF]
    if info and info.states then
        return info.states[state] or 0
    end
    return 0
end

function army.getTotalByState(state)
    local total = 0
    for _, info in pairs(army.armies) do
        if info and info.states then
            total = total + (info.states[state] or 0)
        end
    end
    return total
end

-- 武将出征就不能恢复体力
function army.heroIsMarch(id)
    for k, v in pairs(army.teams) do
        local confList = {}
        if v.team > 0 then
            if next(v.confList) then
                for _, conf in pairs(v.confList) do
                    if conf.heroId == id  and v.teamState == p.TeamState.MARCH then
                        return true
                    end
                end
            end
        end
    end
    return false
end

-- ArmyGainType默认DEFAULT，sendUpdate默认true
function army.add(armyType, state, count, armyGainType, sendUpdate)
    if armyGainType == nil then
        armyGainType = p.ArmyGainType.DEFAULT 
    end
    if sendUpdate == nil then
        sendUpdate = true
    end
    -- 这里增加的兵应该通过类型和转换的比率去转换成预备兵
    local level = army.getArmyLevelByArmyType(armyType)
    print("armyType, level", armyType, level)
    local tpl = t.findArmysTpl(armyType, level)
    if tpl == nil then
        print("###army.add, tpl is nil")
        return
    end
    --新版兵营不再通过预备兵来转换 2018.04.18 cg--
    -- 通过转换比率变成预备兵
    --if armyType == p.ArmysType.INFANTRY or armyType == p.ArmysType.RIDER or armyType == p.ArmysType.ARCHER or armyType == p.ArmysType.MECHANICAL or armyType == p.ArmysType.REDIF then
    --    count = count * tpl.changeNum
    --    armyType = p.ArmysType.REDIF
    --end
    -- print('###army.add...armyType, state, count', armyType, state, count)
    army._add(armyType, state, count)
    if sendUpdate then
        army.sendArmyInfosUpdate()
    end
    army.evtArmyAdd:trigger(armyType, state, count, ArmyGainType)
    if armyGainType == p.ArmyGainType.INIT or armyGainType == p.ArmyGainType.TRAIN or ArmyGainType == p.ArmyGainType.PROP then
        logStub.appendArmy(user.uid, armyType, 0, count, p.LogItemType.GAIN, armyGainType, timer.getTimestampCache())
    end
end

function army._add(armyType, state, count)
    -- print('###army._add...armyType, state, count', armyType, state, count)
    if count > 0 then
        local info = army.armies[armyType]
        if info then
            local num = info.states[state] or 0
            --print('###army._add...armyType,num,count',armyType,num,count)
            info.states[state] = num + count
        else
            local level = army.getArmyLevelByArmyType(armyType)
            army.armies[armyType] = armyInfo:new({ armyType = armyType, level = level, states = { [state] = count } })
        end
        army.armies[armyType].sync = false
    end
end

-- 城池防守失败了之后，按照比例削减城内预备兵
function army.castleBattleSub() 
   --取消预备兵的概念 削减各个兵种的数量 2018.04.18 cg --
    --local info = army.armies[p.ArmysType.REDIF]
    --if not info then
    --    return 0
    --end
    --local num = info.states[p.ArmyState.NORMAL]
    --local dieNum = math.floor(num * dieSoldiersPro / 10000)
    --army.sub(p.ArmysType.REDIF, p.ArmyState.NORMAL, dieNum)
    --return dieNum
    local dieNumTotal = 0
    for _,info in pairs(army.armies) do
       local num = info.states[p.ArmyState.NORMAL] or 0
       local dieNum = math.floor(num * dieSoldiersPro / 10000)
       army.sub(info.type, p.ArmyState.NORMAL, dieNum)
       dieNumTotal = dieNumTotal + dieNum
    end
    return dieNumTotal
end

function army.sub(armyType, state, count, armyConsumeType)  
    local level = army.getArmyLevelByArmyType(armyType)
    local tpl = t.findArmysTpl(armyType, level)
    if tpl == nil then
        return
    end
    --新版兵营不再通过预备兵来转换 2018.04.18 cg--
    -- 这里增加的兵应该通过类型和转换的比率去转换成预备兵
    --if armyType == p.ArmysType.INFANTRY or armyType == p.ArmysType.RIDER or armyType == p.ArmysType.ARCHER or armyType == p.ArmysType.MECHANICAL or armyType == p.ArmysType.REDIF then
    --    count = count * tpl.changeNum
    --    armyType = p.ArmysType.REDIF
    --end
    if army._sub(armyType, state, count) > 0 then
        army.sendArmyInfosUpdate()
        army.evtArmySub:trigger(armyType, state, count, armyConsumeType)
    end
end

function army._sub(armyType, state, count)
    -- print('###army._sub...armyType, state, count', armyType, state, count)
    local level = army.getArmyLevelByArmyType(armyType)
    local tpl = t.findArmysTpl(armyType, level)
    if tpl == nil then
        return
    end

    if count > 0 then
        local info = army.armies[armyType]
        if info then
            local num = info.states[state] or 0
            if num > 0 then
                if num >= count then
                    info.states[state] = num - count
                    info.sync = false
                    return count
                else
                    info.states[state] = 0
                    info.sync = false
                    return num
                end
            end
        end
    end
    return 0
end

function army.getTeamArmyListByTeam(team)
    local armyList = {}
    local info = army.teams[team]
    if info ~= nil then
        for _, v in pairs(info.confList) do
            local level = army.getArmyLevelByArmyType(v.armyType)
            armyList[v.heroId] = { heroId = v.heroId, armyType = v.armyType, level = level, count = v.count, position = v.position }
        end
    end
    return armyList
end

function army.getUnlockTeamCount()
    local count = 0
    for _, v in pairs(army.teams) do
        if v.teamState ~= p.TeamState.LOCKED then
            count = count + 1
        end
    end
    return count
end

function army.armyForSync()
    local list = {}
    for _, info in pairs(army.armies) do 
        local normal = info.states[p.ArmyState.NORMAL] or 0
        if normal > 0 then
            local level = army.getArmyLevelByArmyType(info.armyType)
            list[info.armyType] = { armyType = info.armyType, level = level, states = info.states }
        end
    end
    return list
end

function army.teamsForSync()
    local teamList = {}
    for k, v in pairs(army.teams) do
        if v.team > 0 then
            local list = {}
            for _, conf in pairs(v.confList) do
                local level = army.getArmyLevelByArmyType(conf.armyType)
                list[conf.heroId] = { heroId = conf.heroId, armyType = conf.armyType, level = level, count = conf.count, position = conf.position }
                -- print("heroId, armyType, count, position*************", conf.heroId, conf.armyType, conf.count, conf.position)
            end
            -- if next(list) then
                teamList[k] = list
            -- end
        end
    end
    -- print("###army.teamsForSync", utils.serialize(teamList))
    return teamList
end

function army.castleDefenderSync()
    local castleDefendserId = 0
    for k, v in pairs(army.teams) do
        if v.team > 0 then
            if v.teamState == p.TeamState.DEFENSE then
                castleDefendserId = v.team
            end
        end
    end
    return castleDefendserId
end

function army.teamCanMarchForMap(team,type)
    if team and team > 0 and team <= army.MAX_TEAM then
        local info = army.teams[team]
        if info then
            -- print('army.teamCanMarchForMap...team, troopType', team, info.troopType)
            local tempArmy = {}
            local tempCount = 0
            for _, v in pairs(info.confList) do
                if tempArmy[v.armyType] == nil then
                    tempArmy[v.armyType] = 0
                end
                tempArmy[v.armyType] = tempArmy[v.armyType] + v.count
                tempCount = tempCount + v.count
            end
            --1.判断出征状态
            local troopType = info.troopType or 0
            if troopType > p.MapTroopType.UNKNOWN then
                if troopType ==  p.MapTroopType.CAMP_TEMP
                    or troopType ==  p.MapTroopType.CAMP_FIXED 
                    or (troopType == p.MapTroopType.SIEGE_CITY and type == p.MapTroopType.PATROL_CITY) then
                    -- print('army.teamCanMarchForMap...count = ', tempCount)
                    if tempCount > 0 then
                        return true
                    end
                    print('army.teamCanMarchForMap...team no armies, count = ', tempCount)
                end
                return false
            end
            --2.判断兵种数量是否足够
            for armyType, needCount in pairs(tempArmy) do
                local normalCount = army.getArmyCount(armyType, p.ArmyState.NORMAL)
                if normalCount < needCount then
                    print('army.teamCanMarchForMap...army is not enough, needCount > normalCount...armyType, needCount, normalCount', armyType, needCount, normalCount)
                    return false
                end
            end
            return true
        end
    end
    return false
end

function army.onMarch(x, y, troopType, team, troopId)
    -- print('####army.onMarch...x, y, troopType, team', x, y, troopType, team)
    -- army.dump()
    local info = army.teams[team]
    if info and info.confList then
        local bSave = true
        if bSave then
            --点将台配置:设置出征状态
            if troopType == p.MapTroopType.CAMP_FIXED_OCCUPY then
                troopType = p.MapTroopType.CAMP_FIXED
            end
            if troopType == p.MapTroopType.CAMP_TEMP
                or troopType == p.MapTroopType.CAMP_FIXED then
                info.srcX = x or 0
                info.srcY = y or 0
            end
            for _,m in pairs(info.confList) do
                army.sub(m.armyType,p.ArmyState.NORMAL,m.count)
                army.add(m.armyType,p.ArmyState.MARCHING,m.count)
            end 
            -- print('army.onMarch...x, y, info.srcX,info.srcX ', x, y, info.srcX,info.srcY )
            info.troopType = troopType
            info.troopId = troopId
            info.x = x or 0
            info.y = y or 0
            info.teamState = p.TeamState.MARCH
            info.sync = false

            army.sendArmyInfosUpdate()
            army.sendTeamInfosUpdate()

            -- save it real time
            army.dbSave()
        end
    end
    -- army.dump()
end

function army.onMarchBack(troopType, team, isArriveCastle)
    -- army.dump()
    print('####army.onMarchBack...troopType, team, isArriveCastle',troopType, team, isArriveCastle)
    local info = army.teams[team]
    if info and info.confList and info.troopType > p.MapTroopType.UNKNOWN then
        if isArriveCastle then
            info.troopType = p.MapTroopType.UNKNOWN
            info.troopId = 0
            info.srcX = 0
            info.srcY = 0
            info.x = 0
            info.y = 0
            info.teamState = p.TeamState.IDLE
            for _,m in pairs(info.confList) do
                local heroInfo = hero.info[m.heroId]
                army.sub(m.armyType,p.ArmyState.MARCHING,m.count)
                army.add(m.armyType,p.ArmyState.NORMAL,m.count)
            end
        else
            info.troopType = troopType
            info.x = info.srcX
            info.y = info.srcY
        end
        info.sync = false

        army.sendArmyInfosUpdate()
        army.sendTeamInfosUpdate()

        -- save it real time
        army.dbSave()
    end
    -- army.dump()
end

--城防出战士兵补充
function army.onCityDefenerFill(team)
    local info = army.teams[team]
     -- { heroId = v.heroId, armyType = v.armyType, level = level, count = v.count, position = v.position }
    for heroId,armyInfo in pairs(info.confList) do
        local type = armyInfo.armyType
        local heroInfo = hero.info[heroId]
        if heroInfo then
            local needCount = 0  
            needCount = hero.info[heroId].heroLeadership 
            local armyCount = army.getArmyCount(type, p.ArmyState.NORMAL)
            if armyCount >= needCount then
                -- 直接扣拥有的预备兵
                print('---------------###army.onCityDefenerFill,type,p.ArmyState.NORMAL,needCount',type,p.ArmyState.NORMAL,needCount)
                army.sub(type,p.ArmyState.NORMAL,needCount)
                army.add(type,p.ArmyState.MARCHING,needCount)
                -- 加入到点将台的城防部队中
                armyInfo.count =  needCount
            else
                -- 直接扣拥有的预备兵
                print('---------------###army.onCityDefenerFill,type,p.ArmyState.NORMAL,needCount',type,p.ArmyState.NORMAL,needCount)
                army.sub(type,p.ArmyState.NORMAL,armyCount)
                army.add(type,p.ArmyState.MARCHING,armyCount)
                -- 加入到点将台的城防部队中
                armyInfo.count = armyCount
            end
        end       
    end
    army.myDefenerTeam = team
    info.teamState = p.TeamState.DEFENSE
    info.sync = false
    army.evtTeamChange:trigger()
    army.sendTeamInfosUpdate()
    -- save it real time
    army.dbSave()
end

--isCastle = false(减少的是点将台的兵), isCastle = true(减少的是城内的兵)
function army.onDie(armyList, isCastle)
    if not armyList then
        return
    end
    -- print(debug.traceback())
    -- army.dump()

    local dies = 0
    local kills = 0
    local woundList = {}
    local list = armyList.list
    -- print('army.onDie, list...', utils.serialize(list))

    if not isCastle then
        --减少的是点将台的兵
        local info = army.teams[armyList.team]
        if info and info.confList and list then
            info.sync = false
            for heroId, v in pairs(list) do
                local conf = info.confList[heroId]
                -- 战斗之后的体力扣除
                local heroInfo = hero.info[v.hero.tplId]
                if heroInfo then
                    heroInfo.physical = v.hero.physical
                    heroInfo.isSync = false
                    heroInfo.isDirty = true
                end
                if conf and v.army and v.army.state then
                    for state, count in pairs(v.army.state) do
                        if count > 0 then
                            if state == p.ArmyState.DIE or state == p.ArmyState.WOUNDED then
                                -- print('-------减少的是点将台的兵------army.onDie...team, heroId, state, count------------', armyList.team, heroId, state, count)
                                conf.count = conf.count - count
                                if conf.count < 0 then
                                    conf.count = 0
                                end

                                local armyType = v.army.armyType
                                local level = army.getArmyLevelByArmyType(armyType)
                                if state == p.ArmyState.DIE then
                                    dies = dies + count
                                    logStub.appendArmy(user.uid, armyType, level, count, p.LogItemType.CONSUME, p.ArmyConsumeType.DIE, timer.getTimestampCache())
                                end
                                --伤兵
                                if state == p.ArmyState.WOUNDED then
                                    local tpl = t.findArmysTpl(armyType, level)
                                    local power = tpl.power or 0
                                    table.insert(woundList, { armyType = armyType, count = count, power = power })
                                end
                            elseif state == p.ArmyState.KILL then
                                kills = kills + count
                            end
                        end
                    end
                end
            end

            army.evtTeamChange:trigger()
            --伤兵处理
            army.putInHospital(woundList)

            army.sendArmyInfosUpdate()
            army.sendTeamInfosUpdate()
            hero.sendHeroUpdate()
            -- user.addKillsAndLosses(kills, dies)
            user.reCalculateTroopPower()
            -- save it real time
            army.dbSave()
        end
    else
        --减少的是城内的兵
        for heroId, v in pairs(list) do
            for state, count in pairs(v.army.state) do
                if count > 0 then
                    if state == p.ArmyState.DIE or state == p.ArmyState.WOUNDED then
                        -- print('--------减少的是城内的兵的兵-----army.onDie...team, heroId, state, count------------', armyList.team, heroId, state, count)
                        local armyType = v.army.armyType
                        local level = army.getArmyLevelByArmyType(armyType)
                        army._sub(armyType, p.ArmyState.NORMAL, count)
                        if state == p.ArmyState.DIE then 
                            dies = dies + count 
                            logStub.appendArmy(user.uid, armyType, level, count, p.LogItemType.CONSUME, p.ArmyConsumeType.DIE, timer.getTimestampCache())
                        end
                        --伤兵
                        if state == p.ArmyState.WOUNDED then
                            local tpl = t.findArmysTpl(armyType, level)
                            local power = tpl.power or 0
                            table.insert(woundList, { armyType = armyType, count = count, power = power })
                        end
                    elseif state == p.ArmyState.KILL then
                        kills = kills + count
                    end
                end
            end
        end

        --伤兵处理
        army.putInHospital(woundList)

        army.sendArmyInfosUpdate()

        -- user.addKillsAndLosses(kills, dies)
        -- user.reCalculateTroopPower()
        -- save it real time
        army.dbSave()
    end
    -- army.dump()
    return dies, kills
end

function army.pickDropItems(drops, gaintype, armyList)
    local info = army.teams[armyList.team]
    for _, drop in pairs(drops) do
        if drop.tplId == p.SpecialPropIdType.HERO_EXP then
            local info = army.teams[armyList.team]
            for _, v in pairs(info.confList) do
                if (hero.canAddHeroExp(v.heroId)) then
                    hero.addHeroExp(v.heroId, drop.count, true)
                end
            end
        end
    end
end

--陷阱消耗
function army.onTrapDie(trapSet)
    if not trapSet then
        return
    end
    -- print('###army.onTrapDie ', utils.serialize(trapSet) )

    local needSync = false
    for armyType,v in pairs(trapSet) do
        for state, count in pairs(v.state) do
            if count > 0 and state == p.ArmyState.DIE  then
                -- print('===army.onTrapDie count', count) 
                army._sub(armyType, p.ArmyState.NORMAL, count)
                needSync = true
            end
        end
    end
    user.reCalculateTrapPower()
    if needSync then
        army.sendArmyInfosUpdate()
    end
end

-- --伤害折算成兵种血量，根据兵种ID顺序扣除对应兵力，伤害不满1个兵力则扣除1个兵力取整。
-- function army.onArmyHurt(armyHurt)
--     if armyHurt <= 0 then
--         return
--     end

--     local armyType = p.ArmysType.REDIF
--     local info = army.armies[armyType]
--     if info then
--         local normal = info.states[p.ArmyState.NORMAL] or 0
--         if normal > 0 then
--             local tpl = t.findArmysTpl(info.armyType, info.level)
--             if tpl then
--                 local hurtCount = math.ceil(armyHurt / tpl.hp)
--                 if normal < hurtCount then
--                     hurtCount = normal
--                 end
--                 army._sub(armyType, p.ArmyState.NORMAL, hurtCount)
--                 --info.states[p.ArmyState.NORMAL] = normal - hurtCount
--                 info.sync = false
--                 armyHurt = armyHurt - hurtCount * tpl.hp
--                 if armyHurt <= 0 then
--                     break
--                 end
--             end
--         end
--     end
--     army.sendArmyInfosUpdate()
--     -- save it real time
--     army.dbSave()
-- end

function army.putInHospital(woundList)
    local maxWounded = agent.property.list.woundedCapacity
    local wdTotal = army.getTotalByState(p.ArmyState.WOUNDED)
    -- print('army.putInHospital(army.putInHospital(armyType, count',utils.serialize(woundList), maxWounded, wdTotal)
    if maxWounded > wdTotal then
        if woundList and next(woundList) then
            --按战力从高到低排序
            table.sort(woundList, function(a, b)
                return a.power > b.power
            end)

            for _, v in ipairs(woundList) do
                if v.count + wdTotal <= maxWounded then
                    army._add(v.armyType, p.ArmyState.WOUNDED, v.count)
                    --army._add(p.ArmysType.REDIF, p.ArmyState.WOUNDED, v.count)  --取消预备兵 ，添加到各个兵种的伤兵队列中2018.04.18 cg--
                    wdTotal = wdTotal + v.count
                    if wdTotal == maxWounded then
                        break
                    end
                else
                    local add = maxWounded - wdTotal
                    if add > 0 then
                        army._add(v.armyType, p.ArmyState.WOUNDED, add)
                        --army._add(p.ArmysType.REDIF, p.ArmyState.WOUNDED, add)  --取消预备兵 ，添加到各个兵种的伤兵队列中2018.04.18 cg--
                    end
                    break
                end
            end
        end
    end
end

function army.SubWounded(healList)
    for armyType, count in pairs(healList) do
        local info = army.armies[armyType]
        if info then
            local num = info.states[p.ArmyState.WOUNDED] or 0
            if num >= count then
                info.states[p.ArmyState.WOUNDED] = num - count
                army.evtArmySub:trigger(armyType, p.ArmyState.WOUNDED, count)
                info.sync = false
            end
        end
    end
    army.dbSave()
    army.sendArmyInfosUpdate()
end


function army.heal(healList)
    local healTotal = 0
    for armyType, count in pairs(healList) do
        print('healList',utils.serialize(healList))
        local info = army.armies[armyType]
        if info then
            local num = info.states[p.ArmyState.WOUNDED] or 0
            if num >= count then
                info.states[p.ArmyState.WOUNDED] = num - count
                army.evtArmySub:trigger(armyType, p.ArmyState.WOUNDED, count)

                local normal = info.states[p.ArmyState.NORMAL] or 0
                info.states[p.ArmyState.NORMAL] = normal + count
                army.evtArmyAdd:trigger(armyType, p.ArmyState.WOUNDED, count)

                info.sync = false
                healTotal = healTotal + count
            end
        end
    end
    army.evtHealArmy:trigger(healTotal)
    army.dbSave()
    army.sendArmyInfosUpdate()
end

-- 当地图没有行军时检查兵种信息是否异常（出征状态还原正常状态,防止丢兵）
-- function army.checkTroopException(list)
--     -- print('army.checkTroopException...list', utils.serialize(list))
--     -- army.dump()
--     -- print(debug.traceback())
--     for _, v in pairs(army.teams) do
--         if not list[v.team] then
--             if v.troopType > p.MapTroopType.UNKNOWN then
--                 for _, conf in pairs(v.confList) do
--                     army._add(conf.armyType, p.ArmyState.NORMAL, conf.count)
--                 end
--             end
--             v.sync = false
--             -- v.team = 0
--             v.confList = {}
--             v.troopType = p.MapTroopType.UNKNOWN
--         end
--     end
--     --检查无效team
--     army.checkInvalidTeam()

--     army.sendArmyInfosUpdate()
--     army.sendTeamInfosUpdate()
--     -- army.dump()
-- end

function army.sendArmyInfosUpdate()
    local list = {}
    -- print('###army.sendArmyInfosUpdate..army.armies', utils.serialize(army.armies))
    for armyType, info in pairs(army.armies) do
        if not info.sync then
            local states = {}
            for k, v in pairs(info.states) do
                table.insert(states, { state = k, count = v })
            end
            local level = army.getArmyLevelByArmyType(armyType)
            table.insert(list, { type = armyType, level = level, states = states})
            info.sync = true
        end
    end
    -- print("###army.sendArmyInfosUpdate, list", utils.serialize(list))
    if next(list) then
        agent.sendPktout(p.SC_ARMY_INFOS_UPDATE, '@@1=[type=i,level=i,states=[state=i,count=i]]', list)
    end
end

function army.sendTeamInfosUpdate()
    -- print(debug.traceback())
    local updateList = {}
    -- local propertyList = agent.property.list
    for k, v in pairs(army.teams) do
        if not v.sync then
            v.sync = true
            local confList = {}
            if v.team > 0 then
                if next(v.confList) then
                    for _, conf in pairs(v.confList) do
                        if t.heros[conf.heroId] then
                            table.insert(confList, { heroId = conf.heroId, type = conf.armyType, count = conf.count, position = conf.position })
                        end
                    end
                end
            end
            table.insert(updateList, { team = v.team, troopId = v.troopId, heroId = v.heroId, x = v.x, y = v.y, state = v.teamState, confList = confList })
            -- print("army.sendTeamInfosUpdate..propertyList.maxMarchingQueue", propertyList.maxMarchingQueue)
        end
    end
    -- print('army.sendTeamInfosUpdate..updateList', utils.serialize(updateList))
    if next(updateList) then
        agent.sendPktout(p.SC_TEAM_INFOS_UPDATE, '@@1=[team=i,troopId=i,x=i,y=i,state=i,confList=[heroId=i,type=i,count=i,position=i]]', updateList)
    end
end

function army.getHeroLeadArmy(Id)
    for team, v in pairs(army.teams) do
        for heroId, conf in pairs(v.confList) do
            -- print('team, heroId, troopType, x, y, srcX, srcY, armyType, count', team, heroId, v.troopType, v.x, v.y, v.srcX, v.srcY, conf.armyType, conf.count)
            if heroId == Id then
                return conf.count
            end
        end
    end
    return 0
end

function army.checkTeamCanMarch(id)
    local info = army.teams[id]
    if not info then
        return false
    end
    -- print("army.checkTeamCanMarcharmy.checkTeamCanMarch", utils.serialize(army.teams))
    local isCanMarch = true
    for team, v in pairs(army.teams) do
        if team == id then
            local isNil = true
            for heroId, conf in pairs(v.confList) do
                isNil = false
                -- print("army.checkTeamCanMarch", isNil, conf.count, isCanMarch)
                if conf.count <= 0 then
                    isCanMarch = false
                end
            end
            if isNil then
                return false
            end
        end
    end
    return isCanMarch
end

function army.dump()
    print('=========================dump  begin=====================')
    print('-------------------teamList------------------------------')
    for team, v in pairs(army.teams) do
        for heroId, conf in pairs(v.confList) do
            print('team, heroId, troopType, x, y, srcX, srcY, armyType, count', team, heroId, v.troopType, v.x, v.y, v.srcX, v.srcY, conf.armyType, conf.count)
        end
    end
    print('-------------------armyList------------------------------')
    for armyType, v in pairs(army.armies) do
        for state, count in pairs(v.states) do
            print('armyType, state, count', armyType, state, count)
        end
    end
    print('=========================dump  end  =====================')
end

function army.cs_save_team_info(teamLists, session)
    -- print("army.teams.size is = ", #army.teams)
    local propertyList = agent.property.list
    local function saveTeamInfoResponse(result)
        -- print('###p.saveTeamInfoResponse..result', result)
        agent.replyPktout(session, p.SC_SAVE_TEAM_INFO_RESPONSE, result)
    end

    local list = {}
    local tempList = {}
    print("###army.cs_save_team_info...teamLists", utils.serialize(teamLists))
    for team,teamList in pairs(teamLists)do
        local len = #teamList.confList
        if team > 0 and team <= army.MAX_TEAM and team <= propertyList.maxMarchingQueue then
            --1.判断上阵武将是否超过上限
            if len > propertyList.maxHeroNum then
                print('###p.CS_SAVE_TEAM_INFO...team max hero is 5', len)
                agent.sendNoticeMessage(p.ErrorCode.HERO_NUM_CANNOT_EXCEED, tostring(propertyList.maxHeroNum), 1)
                saveTeamInfoResponse(1)
                return 
            end
            --2.出征队列数量上限
            if team >  propertyList.maxMarchingQueue then
                print('p.CS_SAVE_TEAM_INFO, team marching queue is full ...marchQueues, maxMarchingQueue=', marchQueues, propertyList.maxMarchingQueue)
                saveTeamInfoResponse(1)
                return
            end
            --3.判断新编组信息中是否存在重复
            if list[team] then
                print('###p.CS_SAVE_TEAM_INFO...team is repeated..team', team)
                saveTeamInfoResponse(1)
                return
            end
            --4.判断原team号编组是否存在
            for k, v in pairs(army.teams) do
                if k == team then
                    if v.teamState  == p.TeamState.DEFENSE  or v.teamState  == p.TeamState.MARCH then
                        agent.sendNoticeMessage(p.ErrorCode.HERO_IS_MARCHING, '', 1)
                        print('###p.CS_SAVE_TEAM_INFO...team state is not IDLE,v.teamState ', team,v.teamState)
                        saveTeamInfoResponse(1)
                        return
                    end
                    --4.2如果是闲置状态，则将编组清空
                    for heroId, v2 in pairs(v.confList) do
                        army.teams[team].confList[heroId] = nil
                        army.teams[team].sync = false
                        hero.info[heroId].isDirty = true
                        hero.info[heroId].isSync = false
                    end
                end
            end

            --5.新编制数据检查
            local confList = {}
            print("###army.cs_save_team_info...conf", utils.serialize(teamList.confList))
            for n,m in pairs(teamList.confList) do
                print("###army.cs_save_team_info...m", utils.serialize(m))
                local heroId, armyType, count, position = m.heroId, m.armyType, m.count, m.position
                -- 检查是否阵位数据不正确                
                if position < 1 or position > 9 then
                    print('p.CS_SAVE_TEAM_INFO, position is error ')
                    saveTeamInfoResponse(1)
                    return
                end
                if heroId > 0 and t.heros[heroId] and hero.info[heroId] and not confList[heroId] and not tempList[heroId] then
                    tempList[heroId] = team
                else
                    saveTeamInfoResponse(1)
                    return
                end
                confList[heroId] = { heroId = heroId, armyType = armyType, count = count, position = position }                
            end
            list[team] = { confList = confList }
        end
    end

    --print('###p.CS_SAVE_TEAM_INFO...list', utils.serialize(list))
    if not next(list) then
        print('###p.CS_SAVE_TEAM_INFO...no data to save...')
       saveTeamInfoResponse(1)
       return
    end
    
    --3.将士兵补到最大值
    for k, v in pairs(list) do
        army.teams[k].confList = {}  
        for _,m in pairs(v.confList) do 
            local leftCount = army.getArmyCount(m.armyType,p.ArmyState.NORMAL)
            if leftCount < m.count then
                m.count = leftCount
            end
            --todo: cg 此处先假设从兵营中扣除士兵,以方便计算其他英雄带兵时部队中剩余的兵力 
            army.sub(m.armyType,p.ArmyState.NORMAL,m.count)
        end
        --print('v.confList..',utils.serialize(v.confList))
        army.teams[k].confList = v.confList
        for _,m in pairs(army.teams[k].confList) do
            hero.info[m.heroId].isDirty = true
            hero.info[m.heroId].isSync = false
            --todo: cg 将先前扣除的士兵，补充回去  部队编组不扣兵营中的士兵
            army.add(m.armyType,p.ArmyState.NORMAL,m.count)
        end 
        army.teams[k].teamState = p.TeamState.IDLE
        army.teams[k].sync = false
        -- print("sendTeamInfosUpdate, army.teams[k]", utils.serialize(army.teams[k]))
    end
    army.evtTeamChange:trigger()
    hero.sendHeroUpdate()
    army.sendTeamInfosUpdate()
    saveTeamInfoResponse(0)
    army.dbSave()
end

function army.cs_army_fill(team, session)
    local function armyFillResponse(result)
        -- print('###p.armyFillResponse..result', result)
        agent.replyPktout(session, p.SC_ARMY_FILL_RESPONSE, result)
    end
    local propertyList = agent.property.list
    
    --是否是出征队伍
    local info = army.teams[team]
    -- 如果编组不是处于闲置状态，则返回失败
    if info.teamState ~= p.TeamState.IDLE then
        print("###p.CS_ARMY_FILL, TeamState is not idle...")
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local fillList = {}
    for _,armyInfo in pairs(info.confList) do
        -- 得出需要补充的
        local type = armyInfo.armyType
        --检查兵营是否开启
        local buildingType 
        if type == p.ArmysType.INFANTRY then
            buildingType = p.BuildingType.INFANTRY_BARRACKS
        elseif type == p.ArmysType.RIDER then
            buildingType = p.BuildingType.RIDER_BARRACKS
        elseif type == p.ArmysType.ARCHER then
            buildingType = p.BuildingType.ARCHER_BARRACKS
        elseif type == p.ArmysType.MECHANICAL then
            buildingType = p.BuildingType.MECHANICAL_BARRACKS
        end
        local target = agent.building.list[buildingType]
        if not target then
            print("###p.CS_ARMY_FILL, Building not exist, gridId = ", gridId)
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            return
        end
        -- 如果状态还是  DAMAGED，则没有兵营还没有解锁
        if target.state == p.BuildingState.DAMAGED then
            print("###p.CS_ARMY_FILL, army building is lock...")
            agent.sendNoticeMessage(p.ErrorCode.BARRACKS_NOT_CREATE, '', 1)
            return
        end
        
        local heroInfo = hero.info[armyInfo.heroId]
        if not heroInfo then
            print("###p.CS_ARMY_FILL, heroInfo is nil...")
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            return
        end       
        local needCount = 0  
        needCount = hero.info[armyInfo.heroId].heroLeadership - armyInfo.count
        table.insert(fillList,{type = type,count = needCount})
        --print('###p.CS_ARMY_FILL,fillList..',utils.serialize(fillList))
    end
    if next(fillList) == nil then
        print('p.CS_ARMY_FILL, fillList no datas...')
        armyFillResponse(false)
        return
    end
    -- 扣除相应的兵
    for _,v in pairs(fillList) do 
        local armyCount = army.getArmyCount(v.type, p.ArmyState.NORMAL)
        if armyCount >= v.count then
            -- 直接扣拥有的预备兵
            army.sub(v.type,p.ArmyState.NORMAL,v.count)
        else
            -- 一部分去花资源扣除
            local needResourceCount = v.count - armyCount
            local level = army.getArmyLevelByArmyType(v.type)
            local tplArmy = t.findArmysTpl(v.type, level)
            if not tplArmy then
                print('p.CS_ARMY_FILL, is not exist this armyType...armyType=', v.armyType)
                armyFillResponse(false)
                return
            end
            local time = tplArmy.training_time * needResourceCount
            local food = tplArmy.training_food * needResourceCount
            local wood = tplArmy.training_wood * needResourceCount
            local stone = tplArmy.training_stone * needResourceCount
            local iron = tplArmy.training_iron * needResourceCount
            local isEnough, ret = building.checkRemoveResource(time, food, wood, iron, stone, 0, p.ResourceConsumeType.TRAINNING)
            if not isEnough then
                print('p.CS_ARMY_FILL, resources are not enough..')
                agent.sendNoticeMessage(ret, '', 1)
                return
            end
            -- 扣一部分拥有的
            army.sub(v.type,p.ArmyState.NORMAL,armyCount)
        end
    end    
    -- 补充点将台里面的兵
    for _, v in pairs(info.confList) do
        -- 得出需要补充的兵数
        local heroInfo = hero.info[v.heroId]
        if not heroInfo then
            print("###p.CS_ARMY_FILL, heroInfo is nil...")
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            return
        end
        -- 补满
        v.count = hero.info[v.heroId].heroLeadership        
    end
    info.sync = false
    army.evtTeamChange:trigger()
    army.sendTeamInfosUpdate()
    -- save it real time
    army.dbSave()
    armyFillResponse(true)
end

function army.cs_set_castle_defender(isUp, team, session)
    local function setCastleDefenderResponse(result)
        print('###p.setCastleDefenderResponse..result', result)
        agent.replyPktout(session, p.SC_SET_CASTLE_DEFENDER_RESPONSE, result)
    end
    print("###p.CS_SET_CASTLE_DEFENDER, isUp, team", isUp, team)
    local info = army.teams[team]
    if not info then
        print("###p.CS_SET_CASTLE_DEFENDER, team...", team)
    end
    if isUp then
        -- 判断状态是否可以设置成守城
        if info.teamState == p.TeamState.LOCKED or info.teamState == p.TeamState.MARCH then
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            setCastleDefenderResponse(p.ErrorCode.FAIL)
            return
        end
        for k, v in pairs(army.teams) do
            if v.team > 0 then
                if v.teamState == p.TeamState.DEFENSE then
                    v.teamState = p.TeamState.IDLE
                    v.sync = false
                    v.dirty = true
                end
            end
        end
        info.teamState = p.TeamState.DEFENSE
    else
        if info.teamState ~= p.TeamState.DEFENSE then
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            setCastleDefenderResponse(p.ErrorCode.FAIL)
            return
        end 
        info.teamState = p.TeamState.IDLE
    end
    info.sync = false
    info.dirty = true
    army.evtTeamChange:trigger()
    army.evtCastleDefenderChange:trigger()
    army.sendTeamInfosUpdate()
    setCastleDefenderResponse(p.ErrorCode.SUCCESS)
end

function army.cs_destroy_trap(type, count, session)
    local function destroyTrapResponse(result)
        print("###p.CS_DESTROY_TRAP...result", result)
        agent.replyPktout(session, p.SC_DESTROY_TRAP_RESPONSE, result)
    end
    print("###p.CS_DESTROY_TRAP, type, count...", type, count)
    local info = army.armies[type]
    if not info then
        print("###p.CS_DESTROY_TRAP, trap is nil,type",type)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        destroyTrapResponse(p.ErrorCode.FAIL)
    else
        local num = info.states[p.ArmyState.NORMAL] or 0
        if num < count then
            print("###p.CS_DESTROY_TRAP, trap is not enough",num)
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            destroyTrapResponse(p.ErrorCode.FAIL)
        else
            army.sub(type, p.ArmyState.NORMAL, count)
        end
    end
    army.sendArmyInfosUpdate()
    destroyTrapResponse(p.ErrorCode.SUCCESS)
end

return army
