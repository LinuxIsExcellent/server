-- 服务器模板数据加载
local dbo = require('dbo')
local utils = require('utils')
local misc = require('libs/misc')
local p = require('protocol')

local tploader = {
    miscConf = {},  -- misc.txt
    configure = {},
    lang = {},
    localization = {},
    item = {},
    drop = {},
    lordLevel = {},
    lordHead = {},
    forest = {},
    building = {},
    buildingLevel = {},
    takeTax = {},
    army = {},
    armys = {},
    technology = {},
    techGroup = {},
    message = {},
    mail = {},
    mapUnit = {},
    -- activity = {},

    hero = {},
    heros = {},
    heroDraw = {},
    heroLevel = {},
    heroStarlevel = {},
    heroSlotUnlock = {},
    heroSkillLevel = {},
    equipLevel = {},
    qualityInfo = {},
    equipSuccinctInlay = {},
    succinctAttribute = {},
    heroFetter = {},
    heroOwnFetter = {},
    heroFetterAttribute = {},

    battleSpellBase = {},
    battleSpellNode = {},
    battleBuff = {},
    systemUnlock = {},
    mapNpcArmy = {groups = {}},
    scenarioSection = {},
    scenarioChapter = {},

    store = {},
    storeItemLibrary = {},
    storeItems = {},
    storeRefreshPrice = {},

    arenaRankReward = {},
    arenaRobot = {},
    arenaRule = {},
    vip = {},
    staminaBuy = {},
    energyBuy = {},

    allianceLevel = {},
    allianceBanner = {},
    allianceRankDetail = {},
    allianceScience = {},
    allianceDonate = {},
    allianceBuff = {},
    allianceMessage = {},

    quest = {},
    dailyTask = {},
    dailyTaskReward = {},
    achievement = {},

    babel = {},
    bronzeSparrowTower = {},
    bronzeSparrowTowerRewardList = {},
    mapCity = {},

    chat = {},
    ridingAlone = {},

    skill = {},  --Todo: skill
    armys = {}, --Todo: armys
    
    title = {},
    heroDrawType = {},
    storyQuest = {},
    questDay = {},
    questDayPoints = {},
    xing = {},
    womanName = {},
    manName = {},
    techTreeUnlock = {},
    techTreeList = {},
    heroSoul = {},
    heroSoulLib = {}, 
    heroTalent = {},
    heroLevelAttr = {},
    battleArrtTransform = {},
    skillPreBattle = {},
    battleArrt = {},

    effect = {},
}

-- ===== drop begin =====

local  dropTpl = {
    dropId = 0,
    items = {},
    groups = {},
    fixed = true
}

function dropTpl:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end

function dropTpl:addToGroup(tpl, groupId, countMin, countMax, probability)
    if self.groups[groupId] == nil then
        self.groups[groupId] = {}
    end
    local item = { tpl = tpl, groupId = groupId, countMin = countMin, countMax = countMax, probability = probability }
    table.insert(self.groups[groupId] , item)
end

function dropTpl:getDropRate(dropItem)
    for _, item in pairs(self.items) do
        if dropItem.tpl.id == item.tpl.id then
            return item.probability
        end
    end
    for _, items in pairs(self.groups) do
        for __, item in pairs(items) do
            if dropItem.tpl.id == item.tpl.id then
                return item.probability
            end
        end
    end
    return 0
end

-- do the drop work
function dropTpl:DoDrop(withTpl)
    local drops = {}

    for _, item in pairs(self.items) do
        local get = false
        if item.probability < 10000 then
            local r = utils.getRandomNum(10000)
            if r < item.probability then
                get = true
            end
        else
            get = true
        end
        if get == true then
            local count = item.countMin
            if item.countMin ~= item.countMax then
                count = utils.getRandomNum(item.countMin, item.countMax + 1)
            end
            if count > 0 then
                if withTpl == true then
                    table.insert(drops, { tpl = item.tpl, count = count })
                else
                    table.insert(drops, { tplId = item.tpl.id, count = count })
                end
            end
        end
    end

    for _, items in pairs(self.groups) do
        local temp = {}
        local total_probability = 0
        for _, item in pairs(items) do
            if item.probability > 0 then
                table.insert(temp, item)
                total_probability = total_probability + item.probability
            end
        end
        if total_probability > 0 then
            local r = utils.getRandomNum(1, total_probability + 1)
            local acc = 0
            for _, item in pairs(temp) do
                acc = acc + item.probability
                if acc >= r then
                    local count = item.countMin
                    if item.countMin ~= item.countMax then
                        count = utils.getRandomNum(item.countMin, item.countMax + 1)
                    end
                    if count > 0 then
                        if withTpl == true then
                            table.insert(drops, { tpl = item.tpl, count = count })
                        else
                            table.insert(drops, { tplId = item.tpl.id, count = count })
                        end
                    end
                    break
                end
            end
        end
    end
    --[[
    print('============drop begin=============== id=', self.dropId)
    for _, d in pairs(drops) do
        if withTpl == true then
            print(d.tpl.id, d.count)
        else
            print(d.tplId, d.count)
        end
    end
    print('============drop end===============')
    --]]
    return drops
end

--  create a drop item
function tploader.createDropItem(tplId, count, withTpl)
    local tpl = tploader.item[tplId]
    if tpl == nil then
        return nil
    end
    if withTpl == true then
        return { tpl = tpl, count = count }
    else
        return { tplId = tplId, count = count }
    end
end

-- merge drops
function tploader.mergeDrops(drops1, drops2)
    for _, d2 in ipairs(drops2) do
        local insert = true
        for _, d1 in ipairs(drops1) do
            if d2.tplId == d1.tplId then
                d1.count = d1.count + d2.count
                insert = false
                break
            end
        end
        if insert then
            table.insert(drops1, {tplId = d2.tplId, count = d2.count})
        end
    end
    --[[
    print("=================mergeDrops begin=================")
    var_dump(drops1)
    print("=================mergeDrops end=================")
    ]]
end

-- conf(map<tplId, count>) to drop table
function tploader.confToDrops(conf)
    local drops = {}
    for tplId, count in pairs(conf) do
        table.insert(drops, tploader.createDropItem(tplId, count))
    end
    return drops
end


-- init drop list for avtivity
function tploader.initActivityDrop(dropList)
    local drop = dropTpl:new({ dropId = 0, items = {},  groups = {}})
    for _, row in ipairs(dropList) do
        local tpl = tploader.item[row.itemId]
        if tpl == nil then
            utils.log('tploader.initActivityDrop not exist item tpl in drop tpl, tplId=' .. row.itemId)
        else
            if row.groupId == 0 then
                local item = { tpl = tpl, groupId = row.groupId, countMin = row.min, countMax = row.max, probability = row.rate }
                table.insert(drop.items, item)
            else
                drop:addToGroup(tpl, row.groupId, row.min, row.max, row.rate)
            end
        end
    end
    return drop
end

-- ===== drop end =====

function tploader.getLangString(name, langType)
    local row = tploader.localization[name]
    if row then
        for key, val in pairs(p.LangType) do
            if langType == val then
                local key = 'lang_' .. string.lower(key)
                return row[key]
            end
        end
    end
    return ''
end

function tploader.timeToGold(second)
    second = math.ceil(second)
    if second == 0 then
        return 0
    end
    local gold = 0
    local tpl = tploader.configure["GoldConversion"]
    if tpl then
        for _, v in ipairs(tpl.time) do
            if v.m <= second and second <= v.n then
                gold = math.ceil(second * tpl.base * v.coe + v.cor)
                break
            end
        end
    end
    return gold
end

function tploader.resourceToGold(food, wood, iron, stone)
    local gold = 0
    local tpl = tploader.configure["GoldExchange"]
    if tpl then
        gold = gold + food / tpl.food + wood / tpl.wood + iron / tpl.iron + stone / tpl.stone
    end
    return math.ceil(gold)
end

function tploader.sanToGold(san)
    local gold = 0
    local goldSan = tploader.configure["goldSan"]
    if goldSan then
        gold = gold + san / goldSan
    end
    return math.ceil(gold)
end

function tploader.findArmy(level)
    local list, size = {}, 0
    for _, v in pairs(tploader.army) do
        if v.level == level and v.type >= 1 and v.type <= 8 and v.source ~= 0 then
            table.insert(list, v.id)
            size = size + 1
        end
    end
    return list, size
end

-- 为了兼容以前的表，先留着，以后应该是用tploader.findArmysTpl
function tploader.findArmyTpl(type, level)
    local tpl = nil
    for _, v in pairs(tploader.army) do
        if v.level == level and v.subType == type then
            tpl = v
            break
        end
    end
    return tpl
end

function tploader.findArmysTpl(type, level)
    local tpl = nil
    for _, v in pairs(tploader.armys) do
        if v.level == level and v.nJobType == type then
            tpl = v
            break
        end
    end
    return tpl
end

function tploader.findArmysLevel(type, exp)
    -- body
    local level = 1
    for _, v in pairs(tploader.armys) do
        if v.nJobType == type then
            if exp >= v.exp then
                -- 每次把等级最大的赋值给level     
                -- (a and {b} or {c})[1]   lua中的三目运算符
                level = (v.level > level and {v.level} or {level})[1]
            end
        end
    end
    return level   
end

function tploader.isTrap(type)
    return type == p.ArmyType.CHEVAL_DE_FRISE or type == p.ArmyType.ROLLING_LOGS or type == p.ArmyType.GRIND_STONE
end

function tploader.findMailTplBySubType(subType)
    for _, v in pairs(tploader.mail) do
        if v.subType == subType then
            return v
        end
    end
    return nil
end

function tploader.findBuildingAttribute(attrList)
    if not attrList or not next(attrList) then
        return nil
    end
    local list = {}
    for _, v in pairs(attrList) do
        local attrType = v[1] or 0
        local addType = v[2] or 0
        local value = v[3] or 0
        if attrType > 0 and addType > 0 and value > 0 then
            if not list[attrType] then
                list[attrType] = {}
            end
            list[attrType][addType] = value
        end
    end
    return list
end

function tploader.getSkillExpByLevel(level)
    if level == nil then level = 0 end
    local exp = 0
    for k, v in ipairs(tploader.heroSkillLevel) do
        if k < level then
            exp = exp + v.exp
        else
            break
        end
    end
    return exp
end

function tploader.getEquipExpByLevel(level)
    if level == nil then level = 0 end
    local exp = 0
    for k, v in ipairs(tploader.equipLevel) do
        if k < level then
            exp = exp + v.exp
        else
            break
        end
    end
    return exp
end

function tploader.getAllianceMessageContent(messageType)
    if tploader.allianceMessage[messageType] then
        return tploader.allianceMessage[messageType].id, tploader.allianceMessage[messageType].content
    end
    return nil
end

function tploader.findAchievement(type, star)
    for _, v in pairs(tploader.achievement) do
        if v.type == type and v.star == star then
            return v
        end
    end
    return nil
end

function tploader.getChatTypeAndContent(chatSubType)
    if tploader.chat[chatSubType] then
        return tploader.chat[chatSubType].type, tploader.chat[chatSubType].content
    end
    return nil
end

function tploader.getArmyTypeByHero(heroId)
    local armyType = p.ArmyType.SABER
    local tpl = tploader.hero[heroId]
    if tpl then
        local value = tpl.saberPrac
        if value < tpl.pikemanPrac then
            value = tpl.pikemanPrac
            armyType = p.ArmyType.PIKEMAN
        end
        if value < tpl.halberdierPrac then
            value = tpl.halberdierPrac
            armyType = p.ArmyType.HALBERDIER
        end
        if value < tpl.archerPrac then
            value = tpl.archerPrac
            armyType = p.ArmyType.ARCHER
        end
        if value < tpl.riderPrac then
            value = tpl.riderPrac
            armyType = p.ArmyType.RIDER
        end
        if value < tpl.chariotPrac then
            value = tpl.chariotPrac
            armyType = p.ArmyType.CHARIOT
        end
        if value < tpl.stoneThrowerPrac then
            value = tpl.stoneThrowerPrac
            armyType = p.ArmyType.STONE_THROWER
        end
        if value < tpl.warElephantPrac then
            value = tpl.warElephantPrac
            armyType = p.ArmyType.WAR_ELEPHANT
        end
    end
    return armyType
end

function tploader.findMapUnit(unitType, level)
    for _, v in pairs(tploader.mapUnit) do
        if v.unitType == unitType and v.level == level then
            return v
        end
    end
    return nil
end

function tploader.getHeroAddAttrByLevelStar(id, level, star)
    local addPower = 0
    local addDefense = 0
    local addWisdom = 0
    local addLucky = 0
    local addSkill = 0
    local addAgile = 0
    local addLife = 0
    local addSolohp = 0
    local addTroops = 0
    local list = {}
    -- star add
    if not tploader.heroStarlevel[id] or not tploader.heroLevelAttr[id] then
        -- utils.log('getHeroAddAttrByLevelStar hero is not exist in star or level table' .. id)
        return list
    end
    for _,v in pairs(tploader.heroStarlevel[id]) do
        if v.level <= star then
            addPower = addPower + v.PowerAdd
            addDefense = addDefense + v.DefenseAdd
            addWisdom = addWisdom + v.WisdomAdd
            addSkill = addSkill + v.SkillAdd
            addAgile = addAgile + v.AgileAdd
            addLucky = addLucky + v.LuckyAdd
            addLife = addLife + v.LifeAdd
            addSolohp = addSolohp + v.SolohpAdd
            addTroops = addTroops + v.TroopsAdd
        end
    end
    -- level add
    for _,v in pairs(tploader.heroLevelAttr[id]) do
        while true do
            if level > v.levelmax then
                addPower = addPower + v.nGrowthPower * (v.levelmax - v.levelmin + 1)
                addDefense = addDefense + v.nGrowthDefense * (v.levelmax - v.levelmin + 1)
                addWisdom = addWisdom + v.nGrowthWisdom * (v.levelmax - v.levelmin + 1)
                addSkill = addSkill + v.nGrowthSkill * (v.levelmax - v.levelmin + 1)
                addAgile = addAgile + v.nGrowthAgile * (v.levelmax - v.levelmin + 1)
                addLucky = addLucky + v.nGrowthLucky * (v.levelmax - v.levelmin + 1)
                addLife = addLife + v.nGrowthLife * (v.levelmax - v.levelmin + 1)
                addTroops = addTroops + v.nGrowthTroops * (v.levelmax - v.levelmin + 1)
                addSolohp = addSolohp + v.nGrowthSolohp * (v.levelmax - v.levelmin + 1)
                break
            end 
            if level >= v.levelmin then
                addPower = addPower + v.nGrowthPower * (level - v.levelmin + 1)
                addDefense = addDefense + v.nGrowthDefense * (level - v.levelmin + 1)
                addWisdom = addWisdom + v.nGrowthWisdom * (level - v.levelmin + 1)
                addSkill = addSkill + v.nGrowthSkill * (level - v.levelmin + 1)
                addAgile = addAgile + v.nGrowthAgile * (level - v.levelmin + 1)
                addLucky = addLucky + v.nGrowthLucky * (level - v.levelmin + 1)
                addLife = addLife + v.nGrowthLife * (level - v.levelmin + 1)
                addTroops = addTroops + v.nGrowthTroops * (level - v.levelmin + 1)
                addSolohp = addSolohp + v.nGrowthSolohp * (level - v.levelmin + 1)
                break
            end
            break
        end
    end

    list = {
        addPower = addPower,
        addDefense = addDefense,
        addWisdom = addWisdom, 
        addLucky = addLucky, 
        addSkill = addSkill, 
        addAgile = addAgile, 
        addLife = addLife, 
        addSolohp = addSolohp, 
        addTroops = addTroops,
    }
    return list
end

local function _loadMiscConf()
    local file = io.open(utils.getResourceDir() .. '/misc.txt', 'r')
    local txt = file:read('*all')
    file:close()
    tploader.miscConf = misc.deserialize(txt)
end

local function _loadLang(db)
    local rs = db:execute('SELECT * FROM tpl_lang')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.lang[row.id] = row
        end
    end
end

local function _loadLocalization(db)
    local rs = db:execute('SELECT * FROM tpl_localization')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.localization[row.name] = row
        end
    end
    rs = db:execute('SELECT * FROM tpl_localization_patch')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.localization[row.name] = row
        end
    end
end

local function _loadItem(db)
    local rs = db:execute('SELECT * FROM tpl_item')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if row.param4 == '' then row.param4 = '{}' end
            if row.param5 == '' then row.param5 = '{}' end
            if row.param6 == '' then row.param6 = '{}' end
            if row.attr == '' then row.attr = '{}' end
            row.param4 = misc.deserialize(row.param4)
            row.param5 = misc.deserialize(row.param5)
            row.param6 = misc.deserialize(row.param6)
            row.attr = misc.deserialize(row.attr)
            tploader.item[row.id] = row
        end
    end
end

-- drop table
local function _loadDrop(db)
    local rs = db:execute('SELECT * FROM tpl_drop')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do

            local drop = tploader.drop[row.dropId]
            if drop == nil then
                tploader.drop[row.dropId] = dropTpl:new({ dropId = row.dropId, items = {},  groups = {}})
                drop = tploader.drop[row.dropId]
            end
            local tpl = tploader.item[row.tplId]
            if tpl == nil then
                utils.log('_loadDrop not exist item tpl in drop tpl, when drop_id=' .. row.dropId ..'. tplId=' .. row.tplId)
            else
                if row.groupId == 0 then
                    if row.probability < 10000 then
                        drop.fixed = false
                    end
                    local item = { tpl = tpl, groupId = row.groupId, countMin = row.countMin, countMax = row.countMax, probability = row.probability }
                    table.insert(drop.items, item)
                else
                    drop.fixed = false
                    drop:addToGroup(tpl, row.groupId, row.countMin, row.countMax, row.probability)
                end
            end
        end
    end
end

local function _loadConfigure(db)
    local rs = db:execute('SELECT name, value FROM tpl_configure')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            local name = misc.strTrim(row.name)
            tploader.configure[name] = row.value
            if row.name ~= "InCityOrder" and row.name ~= "OutCityOrder" then
                tploader.configure[name] = misc.deserialize(row.value)
            end
        end
    end
end

local function _loadLordLevel(db)
    local rs = db:execute('SELECT * FROM tpl_lord_level')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if row.attrList == '' then
                row.attrList = '{}'
            end
            row.attrList = misc.deserialize(row.attrList)
            tploader.lordLevel[row.level] = row
        end
    end
end
local function _loadLordHead(db)
    local rs = db:execute('SELECT * FROM tpl_lord_head')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if row.unlockCond == '' then row.unlockCond = '{}' end
            row.unlockCond = misc.deserialize(row.unlockCond)
            tploader.lordHead[row.id] = row
        end
    end
end

local function _loadForest(db)
    local rs = db:execute('SELECT * FROM tpl_forest')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.forest[row.id] = row
        end
    end
end

local function _loadBuildingLevel(db)
    local function filter(val)
        if val ~= '' then
            return misc.deserialize(val)
        end
        return nil
    end

    local rs = db:execute('SELECT * FROM tpl_building_level order by buildingType, level asc')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if tploader.buildingLevel[row.buildingType] == nil then
                  tploader.buildingLevel[row.buildingType] = {}
            end
            row.case1 = filter(row.case1)
            row.case2 = filter(row.case2)
            row.case3 = filter(row.case3)
            row.case4 = filter(row.case4)
            row.case5 = filter(row.case5)
            row.case6 = filter(row.case6)
            if row.case7 == '' then row.case7 = '{}' end
            row.case7 = filter(row.case7)
            if row.attrList == '' then row.attrList = '{}' end
            row.attrList = filter(row.attrList)
            tploader.buildingLevel[row.buildingType][row.level] = row
        end
    end
end

local function _loadBuilding(db)
    local rs = db:execute('SELECT * FROM tpl_building')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            local levelList = tploader.buildingLevel[row.buildingType]
            row.maxLevel = #levelList
            row.levels = levelList
            tploader.building[row.id] = row
        end
    end
end

local function _loadTakeTax(db)
    local rs = db:execute('SELECT * FROM tpl_take_tax')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.takeTax[row.id] = row
        end
    end
end


local function _loadArmy(db)
    local rs = db:execute('SELECT * FROM tpl_army')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            row.priority = misc.deserialize(row.priority)
            tploader.army[row.id] = row
        end
    end
end

local function _loadArmys(db)
    local rs = db:execute('SELECT * FROM tpl_armys')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            row.priority = misc.deserialize(row.priority)
            row.arrtValue = misc.deserialize(row.arrtValue)
            tploader.armys[row.id] = row
        end
    end
end

local function _loadTechnologyTreeUnlock(db)
    local rs = db:execute('SELECT * FROM tpl_seowon')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.techTreeUnlock[row.id] = row
        end
    end
end

local function _loadTechnology(db)
    local rs = db:execute('SELECT * FROM tpl_technology')
    local groupList = {}
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if row.unlockCond == '' then
                row.unlockCond = '{}'
            end
            if row.attrList == '' then
                row.attrList = '{}'
            end
            row.unlockCond = misc.deserialize(row.unlockCond)
            row.attrList = misc.deserialize(row.attrList)
            if not groupList[row.group] then 
                groupList[row.group] = row.group
            end
            
            if not tploader.techTreeList[row.type] then
                local tree = {}
                local grouplist = {}
                grouplist[row.group] = row.group
                table.insert(tree,{type = row.type,grouplist = grouplist})
                tploader.techTreeList[row.type]= tree  
            else
                local tree = tploader.techTreeList[row.type]
                for _,v in ipairs(tree) do
                    local grouplist = v.grouplist
                    if not grouplist[row.group] then
                        grouplist[row.group] = row.group   
                        if not tploader.techTreeList[row.type] then
                            table.insert(tree,{type = row.type,grouplist = grouplist})
                        end
                    end
                end  
                tploader.techTreeList[row.type]= tree  
            end
            tploader.technology[row.id] = row
        end
        --按组存放
        for _,v in pairs(groupList) do
            local group = {}
            for _, row in ipairs(rs) do
                if row.group == v then
                    row.nextLevel = misc.deserialize(row.nextLevel)
                    table.insert(group,{id = row.id,level = row.level,nextLevel = row.nextLevel,maxLevel = row.maxLevel,type = row.type})
                end    
            end
             tploader.techGroup[v] = group
        end
        --print("_loadTechnology..techGroup", misc.serialize(tploader.techGroup))
    end
end

local function _loadMapUnit(db)
    local rs = db:execute('SELECT * FROM tpl_map_unit')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            row.paramExt = misc.deserialize(row.paramExt)
            tploader.mapUnit[row.id] = row
        end
    end
end

local function _loadMessage(db)
    local rs = db:execute('SELECT * FROM tpl_message')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.message[row.id] = row
        end
    end
end

local function _loadMail(db)
    local rs = db:execute('SELECT * FROM tpl_mail')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.mail[row.id] = row
        end
    end
end

local function _loadActivity(db)
    local rs = db:execute('SELECT * FROM tpl_activity')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.activity[row.id] = row
        end
    end
end

local function _loadHero(db)
    local rs = db:execute('SELECT * FROM tpl_hero')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.hero[row.id] = row
        end
    end
end

local function _loadHeros(db)
    local rs = db:execute('SELECT * FROM tpl_heros')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            row.support = {} 
            if row.nSupportPlus1 ~= 0 then
                row.support[row.nSupportPlus1] = misc.deserialize(row.nSupportValues1)
            end
            if row.nSupportPlus2 ~= 0 then
                row.support[row.nSupportPlus2] = misc.deserialize(row.nSupportValues2)
            end
            if row.nSupportPlus3 ~= 0 then
                row.support[row.nSupportPlus3] = misc.deserialize(row.nSupportValues3)
            end
            if row.nSupportPlus4 ~= 0 then
                row.support[row.nSupportPlus4] = misc.deserialize(row.nSupportValues4)
            end
            if row.nSupportPlus5 ~= 0 then
                row.support[row.nSupportPlus5] = misc.deserialize(row.nSupportValues5)
            end
            if row.nSupportPlus6 ~= 0 then
                row.support[row.nSupportPlus6] = misc.deserialize(row.nSupportValues6)
            end
            if row.starHoleUnlock ~= '' then
                row.starHoleUnlock = misc.deserialize(row.starHoleUnlock)
            end
            if row.starSkillconsume ~= '' then
                row.starSkillconsume = misc.deserialize(row.starSkillconsume)
            end
            tploader.heros[row.id] = row
        end
    end
end

local function _loadSkill(db)
    local rs = db:execute('SELECT * FROM tpl_skill')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.skill[row.id] = row
        end
    end
end

local function _loadHeroDraw(db)
    local rs = db:execute('SELECT * FROM tpl_hero_draw')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.heroDraw[row.id] = row
        end
    end
end

local function _loadHeroLevel(db)
    local rs = db:execute('SELECT * FROM tpl_hero_level')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.heroLevel[row.level] = row
        end
    end
end

local function _loadHeroStarLevel(db)
    local rs = db:execute('SELECT * FROM tpl_hero_star_level')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if tploader.heroStarlevel[row.heroesID] == nil then
                tploader.heroStarlevel[row.heroesID] = {}
            end
            tploader.heroStarlevel[row.heroesID][row.level] = row
        end
    end
end

local function _loadHeroSlotUnlock(db)
    local rs = db:execute('SELECT * FROM tpl_hero_slot_unlock')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if tploader.heroSlotUnlock[row.type] == nil then
                tploader.heroSlotUnlock[row.type] = {}
            end
            if row.unlockCond == '' then
                row.unlockCond = '{}'
            end
            row.unlockCond = misc.deserialize(row.unlockCond)

            tploader.heroSlotUnlock[row.type][row.slot] = row
        end
    end
end

local function _loadHeroSkillLevel(db)
    local rs = db:execute('SELECT * FROM tpl_hero_skill_level')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if row.initSkill == '' then
                row.initSkill = '{}'
            end
            if row.normalSkill == '' then
                row.normalSkill = '{}'
            end
            if row.uniqueSkill == '' then
                row.uniqueSkill = '{}'
            end
            row.initSkill = misc.deserialize(row.initSkill)
            row.normalSkill = misc.deserialize(row.normalSkill)
            row.uniqueSkill = misc.deserialize(row.uniqueSkill)

            tploader.heroSkillLevel[row.level] = row
        end
    end
end

local function _loadEquipLevel(db)
    local rs = db:execute('SELECT * FROM tpl_equip_level')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.equipLevel[row.level] = row
        end
    end
end

local function _loadQualityInfo(db)
    local rs = db:execute('SELECT * FROM tpl_quality_info')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.qualityInfo[row.quality] = row
        end
    end
end

local function _loadEquipSuccinctInlay(db)
    local rs = db:execute('SELECT * FROM tpl_equip_succinct_inlay')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if row.inlayCond == ''then
                row.inlayCond = '{}'
            end
            if row.succinctCond == '' then
                row.succinctCond = '{}'
            end
            if row.succinctConsume == '' then
                row.succinctConsume = '{}'
            end
            row.inlayCond = misc.deserialize(row.inlayCond)
            row.succinctCond = misc.deserialize(row.succinctCond)
            row.succinctConsume = misc.deserialize(row.succinctConsume)

            tploader.equipSuccinctInlay[row.locate] = row
        end
    end
end

local function _loadSuccinctAttribute(db)
    local rs = db:execute('SELECT * FROM tpl_succinct_attribute')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if tploader.succinctAttribute[row.locate] == nil then
                tploader.succinctAttribute[row.locate] = {}
            end
            if row.value == '' then
                row.value = '{}'
            end
            row.value = misc.deserialize(row.value)

            tploader.succinctAttribute[row.locate][row.id] = row
        end
    end
end

local function _loadBattleSpellBase(db)
    local rs = db:execute('SELECT * FROM tpl_battle_spell_base')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.battleSpellBase[row.id] = row
        end
    end
end

local function _loadBattleSpellNode(db)
    local rs = db:execute('SELECT * FROM tpl_battle_spell_node')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.battleSpellNode[row.id] = row
        end
    end
end

local function _loadBattleBuff(db)
    local rs = db:execute('SELECT * FROM tpl_battle_buff')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.battleBuff[row.id] = row
        end
    end
end

local function _loadSystemunlock(db)
    local rs = db:execute('SELECT * FROM tpl_system_unlock')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if row.condValue == '' then
                row.condValue = '{}'
            end
            row.condValue = misc.deserialize(row.condValue)
            tploader.systemUnlock[row.id] = row
        end
    end
end

local function _loadMapNpcArmy(db)
    local rs = db:execute('SELECT * FROM tpl_map_npc_army')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.mapNpcArmy[row.id] = row
            if row.hero == '' then
                row.hero = '{}'
            end
            row.hero = misc.deserialize(row.hero)
            if row.heroeEquipment == '' then
                row.heroeEquipment = '{}'
            end
            row.heroeEquipment = misc.deserialize(row.heroeEquipment)
            if row.heroSkill == '' then
                row.heroSkill = '{}'
            end
            row.heroSkill = misc.deserialize(row.heroSkill)
            if row.army == '' then
                row.army = '{}'
            end
            row.army = misc.deserialize(row.army)

            if tploader.mapNpcArmy.groups[row.groupId] == nil then
                tploader.mapNpcArmy.groups[row.groupId] = {}
            end
            tploader.mapNpcArmy.groups[row.groupId][row.id] = row
        end
    end
end

local function _loadScenarioSection(db)
    local rs = db:execute('SELECT * FROM tpl_scenario_section')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if tploader.scenarioSection[row.chapterId] == nil then
                  tploader.scenarioSection[row.chapterId] = {}
            end
            if row.condList == '' then
                row.condList = '{}'
            end
            if row.firstRewardList == '' then
                row.firstRewardList = '{}'
            end
            if row.extDropList == '' then
                row.extDropList = '{}'
            end
            if row.rewardList == '' then
                row.rewardList = '{}'
            end
            if row.mopupRewardList == '' then
                row.mopupRewardList = '{}'
            end
            if row.questionPool == '' then
                row.questionPool = '{}'
            end
            if row.singleCombatHero == '' then
                row.singleCombatHero = '{}'
            end
            if row.heroConf == '' then
                row.heroConf = '{}'
            end
            if row.preId == '' then
                row.preId = '{}'
            end
            if row.nextId == '' then
                row.nextId = '{}'
            end
            if row.starList == '' then
                row.starList = '{}'
            end
            if row.resetPrice == '' then
                row.resetPrice = '{}'
            end
            row.condList = misc.deserialize(row.condList)
            row.firstRewardList = misc.deserialize(row.firstRewardList)
            row.extDropList = misc.deserialize(row.extDropList)
            row.rewardList = misc.deserialize(row.rewardList)
            row.mopupRewardList = misc.deserialize(row.mopupRewardList)
            row.questionPool = misc.deserialize(row.questionPool)
            row.singleCombatHero = misc.deserialize(row.singleCombatHero)
            row.heroConf = misc.deserialize(row.heroConf)
            row.preId = misc.deserialize(row.preId)
            row.nextId = misc.deserialize(row.nextId)
            row.starList = misc.deserialize(row.starList)
            row.resetPrice = misc.deserialize(row.resetPrice)

            tploader.scenarioSection[row.chapterId][row.id] = row
        end
    end
end

local function _loadScenarioChapter(db)
    local function getChapterDatas(list)
        local maxStar, maxSection = 0, 0
        for _, v in pairs(list) do
            if not v.isBranch then
                maxSection = maxSection + 1
            end
            if v.canRepeat then
                maxStar = maxStar + 3
            end
        end
        return maxStar, maxSection
    end

    local rs = db:execute('SELECT * FROM tpl_scenario_chapter')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            local sectionList = tploader.scenarioSection[row.id]
            if sectionList == nil then 
                print('_loadScenarioChapter id ', row.id)
            end
            row.sections = sectionList
            row.maxStar, row.maxSection = getChapterDatas(sectionList)

            if row.preId == '' then
                row.preId = '{}'
            end
            if row.nextId == '' then
                row.nextId = '{}'
            end
            if row.sectionList == '' then
                row.sectionList = '{}'
            end
            if row.rewardList == '' then
                row.rewardList = '{}'
            end
            row.preId = misc.deserialize(row.preId)
            row.nextId = misc.deserialize(row.nextId)
            row.sectionList = misc.deserialize(row.sectionList)
            row.rewardList = misc.deserialize(row.rewardList)
            tploader.scenarioChapter[row.id] = row
        end
    end
end

local function _loadStore(db)
    local rs = db:execute('SELECT * FROM tpl_store')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if tploader.store[row.storeType] == nil then
                tploader.store[row.storeType] = {}
            end
            tploader.store[row.storeType][row.level] = row
        end
    end
end

local function _loadStoreItemLibrary(db)
    local rs = db:execute('SELECT * FROM tpl_store_item_library')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if tploader.storeItemLibrary[row.libId] == nil then
                tploader.storeItemLibrary[row.libId] = {}
            end
            tploader.storeItemLibrary[row.libId][row.id] = row
            tploader.storeItems[row.id] = row
        end
    end
end

local function _loadStoreRefreshPrice(db)
    local rs = db:execute('SELECT * FROM tpl_store_refresh_price')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if tploader.storeRefreshPrice[row.storeType] == nil then
                tploader.storeRefreshPrice[row.storeType] = {}
            end
            tploader.storeRefreshPrice[row.storeType][row.count] = row
        end
    end
end

local function _loadArenaRankReward(db)
    local rs = db:execute('SELECT * FROM tpl_arena_rank_reward')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if tploader.arenaRankReward[row.rewardType] == nil then
                tploader.arenaRankReward[row.rewardType] = {}
            end
            if row.rewardList == '' then
                row.rewardList = '{}'
            end
            row.rewardList = misc.deserialize(row.rewardList)
            table.insert(tploader.arenaRankReward[row.rewardType], row)
        end
    end
end

local function _loadArenaRobot(db)
    local rs = db:execute('SELECT * FROM tpl_arena_robot')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if row.rankRange == '' then
                row.rankRange = '{}'
            end
            if row.userRange == '' then
                row.userRange = '{}'
            end
            if row.qualityRange == '' then
                row.qualityRange = '{}'
            end
            if row.heroLevelRange == '' then
                row.heroLevelRange = '{}'
            end
            if row.heroStarRange == '' then
                row.heroStarRange = '{}'
            end
            if row.skillRange == '' then
                row.skillRange = '{}'
            end
            if row.soulRange == '' then
                row.soulRange = '{}'
            end
            -- if row.invalidHeros == '' then
            --     row.invalidHeros = '{}'
            -- end
            row.rankRange = misc.deserialize(row.rankRange)
            row.userRange = misc.deserialize(row.userRange)
            row.qualityRange = misc.deserialize(row.qualityRange)
            row.heroLevelRange = misc.deserialize(row.heroLevelRange)
            row.heroStarRange = misc.deserialize(row.heroStarRange)
            row.skillRange = misc.deserialize(row.skillRange)
            row.soulRange = misc.deserialize(row.soulRange)
            tploader.arenaRobot[row.id] = row
        end
    end
end

local function _loadArenaRule(db)
    local rs = db:execute('SELECT * FROM tpl_arena_rule')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if row.rankRange == '' then
                row.rankRange = '{}'
            end
            if row.leftRule == '' then
                row.leftRule = '{}'
            end
            if row.midRule == '' then
                row.midRule = '{}'
            end
            if row.rightRule == '' then
                row.rightRule = '{}'
            end
            row.rankRange = misc.deserialize(row.rankRange)
            row.leftRule = misc.deserialize(row.leftRule)
            row.midRule = misc.deserialize(row.midRule)
            row.rightRule = misc.deserialize(row.rightRule)
            tploader.arenaRule[row.id] = row
        end
    end
end

local function _loadVip(db)
    local rs = db:execute('SELECT * FROM tpl_vip')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if row.attrList == '' then
                row.attrList = '{}'
            end
            row.attrList = misc.deserialize(row.attrList)
            tploader.vip[row.level] = row
        end
    end
end

local function _loadStaminaBuy(db)
    local rs = db:execute('SELECT * FROM tpl_stamina_buy')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.staminaBuy[row.count] = row
        end
    end
end
local function _loadEnergyBuy(db)
    local rs = db:execute('SELECT * FROM tpl_energy_buy')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.energyBuy[row.count] = row
        end
    end
end
local function _loadAllianceLevel(db)
    local rs = db:execute('SELECT * FROM tpl_alliance_level')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.allianceLevel[row.level] = row
        end
    end
end

local function _loadAllianceBanner(db)
    local rs = db:execute('SELECT * FROM tpl_alliance_banner')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.allianceBanner[row.id] = row
        end
    end
end

local function _loadAllianceRankDetail(db)
    local rs = db:execute('SELECT * FROM tpl_alliance_rank_detail')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.allianceRankDetail[row.level] = row
            local permit = misc.stringSplit(row.permit, ',')
            tploader.allianceRankDetail[row.level].permit = {}
            for _, v in pairs(permit) do
                local p = tonumber(v)
                tploader.allianceRankDetail[row.level].permit[p] = true
            end
        end
    end
end

local function _loadAllianceScience(db)
    local rs = db:execute('SELECT * FROM tpl_alliance_science')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if tploader.allianceScience[row.groupId] == nil then
                tploader.allianceScience[row.groupId] = {}
            end
            if row.attrList == '' then
                row.attrList = '{}'
            end
            row.attrList = misc.deserialize(row.attrList)
            tploader.allianceScience[row.groupId][row.level] = row
        end
    end
end

local function _loadAllianceDonate(db)
    local rs = db:execute('SELECT * FROM tpl_alliance_donate')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if tploader.allianceDonate[row.type] == nil then
                tploader.allianceDonate[row.type] = {}
            end
            if row.donate == '' then
                row.donate = '{}'
            end
            row.donate = misc.deserialize(row.donate)
            tploader.allianceDonate[row.type][row.id] = row
        end
    end
end

local function _loadAllianceBuff(db)
    local rs = db:execute('SELECT * FROM tpl_alliance_buff')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if row.attrList == '' then
                row.attrList = '{}'
            end
            row.attrList = misc.deserialize(row.attrList)
            tploader.allianceBuff[row.id] = row
        end
    end
end

local function _loadAllianceMessage(db)
    local rs = db:execute('SELECT * FROM tpl_alliance_message')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.allianceMessage[row.type] = row
        end
    end
end

local function _loadQuest(db)
    local rs = db:execute('SELECT * FROM tpl_quest')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if row.dropList == '' then
                row.dropList = '{}'
            end
            local dropList = misc.deserialize(row.dropList)
            row.dropList = tploader.confToDrops(dropList)
            tploader.quest[row.id] = row
        end
    end
end

local function _loadDailyTask(db)
    local rs = db:execute('SELECT * FROM tpl_daily_task')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if row.dropList == '' then
                row.dropList = '{}'
            end
            local dropList = misc.deserialize(row.dropList)
            row.dropList = tploader.confToDrops(dropList)
            tploader.dailyTask[row.id] = row
        end
    end
end

local function _loadDailyTaskReward(db)
    local rs = db:execute('SELECT * FROM tpl_daily_task_reward')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if row.dropList == '' then
                row.dropList = '{}'
            end
            local dropList = misc.deserialize(row.dropList)
            row.dropList = tploader.confToDrops(dropList)
            tploader.dailyTaskReward[row.id] = row
        end
    end
end

local function _loadHeroFetter(db)
    local rs = db:execute('SELECT * FROM tpl_hero_fetter')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if row.heroList == '' then
                row.heroList = '{}'
            end
            row.heroList = misc.deserialize(row.heroList)
            row.heroCount = #row.heroList
            tploader.heroFetter[row.id] = row

            for _, heroId in pairs(row.heroList) do
                --print(k,v)
                local ownFetter = tploader.heroOwnFetter[heroId]
                if ownFetter == nil then
                    tploader.heroOwnFetter[heroId] = {}
                    ownFetter = tploader.heroOwnFetter[heroId]
                end
                table.insert(ownFetter, row.id)
            end
        end
    end
end

local function _loadHeroFetterAttribute(db)
    local rs = db:execute('SELECT * FROM tpl_hero_fetter_attribute')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if row.attr2Set == '' then
                row.attr2Set = '{}'
            end
            if row.attr3Set == '' then
                row.attr3Set = '{}'
            end
            if row.attr4Set == '' then
                row.attr4Set = '{}'
            end
            if row.attr5Set == '' then
                row.attr5Set = '{}'
            end

            row.attr2Set = misc.deserialize(row.attr2Set)
            row.attr3Set = misc.deserialize(row.attr3Set)
            row.attr4Set = misc.deserialize(row.attr4Set)
            row.attr5Set = misc.deserialize(row.attr5Set)

            row.attrSet = {}
            if #(row.attr2Set) > 0 then
                table.insert(row.attrSet, {cnt = 2, attr = row.attr2Set})
            end
            if #(row.attr3Set) > 0 then
                table.insert(row.attrSet, {cnt = 3, attr = row.attr3Set})
            end
            if #(row.attr4Set) > 0 then
                table.insert(row.attrSet, {cnt = 4, attr = row.attr4Set})
            end
            if #(row.attr5Set) > 0 then
                table.insert(row.attrSet, {cnt = 5, attr = row.attr5Set})
            end
            tploader.heroFetterAttribute[row.id] = row
        end
    end
end

local function _loadAchievement(db)
    local rs = db:execute('SELECT * FROM tpl_achievement')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if row.dropList == '' then
                row.dropList = '{}'
            end
            local dropList = misc.deserialize(row.dropList)
            row.dropList = tploader.confToDrops(dropList)
            tploader.achievement[row.id] = row
        end
    end
end

local function _loadBabel(db)
    local rs = db:execute('SELECT * FROM tpl_babel')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.babel[row.layer] = row
        end
    end
end

local function _loadBronzeSparrowTower(db)
    local rs = db:execute('SELECT * FROM tpl_configure')
    if rs.ok and rs.rowsCount > 0 then
        for _,row in ipairs(rs) do
            local name = misc.strTrim(row.name)       
            if name == 'bronzeSparrow' then
                local list = misc.deserialize(row.value)
                for k,v in ipairs(list) do
                    tploader.bronzeSparrowTower[k] = v 
                end
            end
        end
    end
    --print("_loadBronzeSparrowTower..bronzeSparrowTower", misc.serialize(tploader.bronzeSparrowTower))
end

local function _loadBronzeSparrowTowerRewardList(db)
    local rs = db:execute('SELECT * FROM tpl_taixuedian_reward')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
             if row.rewardList == '' then
                row.rewardList = '{}'
            end
             if row.expense == '' then
                row.expense = '{}'
            end
            row.rewardList = misc.deserialize(row.rewardList)
            row.expense = misc.deserialize(row.expense)
            table.insert(tploader.bronzeSparrowTowerRewardList, {id = row.id,accessCountMin = row.accessCountMin,accessCountMax = row.accessCountMax,rewardList = row.rewardList,expense = row.expense })
        end
    end
    --print("_loadBronzeSparrowTowerRewardList..bronzeSparrowTowerRewardList", misc.serialize(tploader.bronzeSparrowTowerRewardList))
end


local function _loadMapCity(db)
    local rs = db:execute('SELECT * FROM tpl_map_city')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if row.attrList == '' then
                row.attrList = '{}'
            end
            row.attrList = misc.deserialize(row.attrList)
            row.ArmExperience = misc.deserialize(row.ArmExperience)
            tploader.mapCity[row.id] = row
        end
    end
end

local function _loadChat(db)
    local rs = db:execute('SELECT * FROM tpl_chat')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.chat[row.subType] = row
        end
    end
end

local function _loadRidingAlone(db)
    local rs = db:execute('SELECT * FROM tpl_riding_alone')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if tploader.ridingAlone.data == nil then
                tploader.ridingAlone.data = {}
            end
            tploader.ridingAlone.data[row.id] = row
            if row.preId == 0 then
                tploader.ridingAlone.firstId = row.id
            end
            if row.nextId == 0 then
                tploader.ridingAlone.lastId = row.id
            end
        end
    end
end

local function _loadTitle(db)
    local rs = db:execute('SELECT * FROM tpl_title')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            row.attrList = misc.deserialize(row.attrList)
            tploader.title[row.id] = row
        end
    end
end

local function _loadEffect(db)
    local rs = db:execute('SELECT * FROM tpl_effect')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.effect[row.id] = row
        end
    end
end

local function _loadHeroDrawType(db)
    local rs = db:execute('SELECT * FROM tpl_hero_draw_type')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.heroDrawType[row.id] = row
        end
    end
end

local function _loadQuestStory(db)
    local rs = db:execute('SELECT * FROM tpl_quest_story')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if row.dropList == '' then
                row.dropList = '{}'
            end
            local dropList = misc.deserialize(row.dropList)
            row.dropList = tploader.confToDrops(dropList)
            tploader.storyQuest[row.id] = row
        end
    end
end

local function _loadQuestDay(db)
    local rs = db:execute('SELECT * FROM tpl_quest_day')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.questDay[row.id] = row
        end
    end
end

local function _loadQuestDayPoints(db)
    local rs = db:execute('SELECT * FROM tpl_quest_day_points')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.questDayPoints[row.id] = row
        end
    end
end

local function _loadRandomName(db)
    local rs = db:execute('SELECT * FROM tpl_random_name')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if row.xing ~= "" then
                tploader.xing[row.id] = row.xing
            end
            if row.womanName ~= "" then
                tploader.womanName[row.id] = row.womanName
            end
            if row.manName ~= "" then
                tploader.manName[row.id] = row.manName
            end
        end
    end
    -- print("xing", utils.serialize(tploader.xing))
    -- print("womanName", utils.serialize(tploader.womanName))
    -- print("manName", utils.serialize(tploader.manName))
end

local function _loadHeroSoul(db)
    local rs = db:execute('SELECT * FROM tpl_hero_soul')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if not tploader.heroSoulLib[row.nDamageType] then
                tploader.heroSoulLib[row.nDamageType] = {}    
            end
            if not tploader.heroSoulLib[row.nDamageType][row.soulLevel] then
                tploader.heroSoulLib[row.nDamageType][row.soulLevel] = {}
            end
            row.attrList = misc.deserialize(row.attrList)
            table.insert(tploader.heroSoulLib[row.nDamageType][row.soulLevel], row)
            tploader.heroSoul[row.id] = row
        end
        -- print("_loadheroSoul, utils.serialize", utils.serialize(tploader.heroSoulLib))
        -- print("_loadheroSoul, utils.serialize", utils.serialize(tploader.heroSoul))
        -- print("_loadheroSoul, utils.serialize", utils.serialize(tploader.heroSoul))
    end
end

local function _loadHeroTalent(db)
    local rs = db:execute('SELECT * FROM tpl_hero_talent')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if not tploader.heroTalent[row.sQuality] then
                tploader.heroTalent[row.sQuality] = {}
            end
            tploader.heroTalent[row.sQuality][row.talentlevel] = row
        end
    end
end

local function _loadHeroLevelAttr(db)
    local rs = db:execute('SELECT * FROM tpl_hero_level_attr')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            if not tploader.heroLevelAttr[row.heroesId] then
                tploader.heroLevelAttr[row.heroesId] = {}
            end
            -- 给相同武将分类
            tploader.heroLevelAttr[row.heroesId][row.id] = row
        end
        -- print("_loadHeroLevelAttr(db), utils.serialize(tploader.heroLevelAttr)...", utils.serialize(tploader.heroLevelAttr))
    end
end

local function _loadBattleArrtTransform(db)
    local rs = db:execute('SELECT * FROM tpl_battle_arrt_transform')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.battleArrtTransform[row.sceondArrt] = row
        end
        -- print("_loadHeroLevelAttr(db), utils.serialize(tploader.battleArrtTransform)...", utils.serialize(tploader.battleArrtTransform))
    end
end

local function _loadSkillPreBattle(db)
    local rs = db:execute('SELECT * FROM tpl_skill_pre_battle')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            row.attrList = misc.deserialize(row.attrList)
            tploader.skillPreBattle[row.id] = row
        end
        -- print("_loadSkillPreBattle(db), utils.serialize(tploader.skillPreBattle)...", utils.serialize(tploader.skillPreBattle))
    end
end


local function _loadBattleArrt(db)
    local rs = db:execute('SELECT * FROM tpl_battle_arrt')
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            tploader.battleArrt[row.id] = row
        end
        -- print("_loadHeroLevelAttr(db), utils.serialize(tploader.battleArrtTransform)...", utils.serialize(tploader.battleArrtTransform))
    end
end

function tploader.loadAllTemplate(cb)
    -- print('tploader.loadAllTemplate')
    _loadMiscConf()

    local db = dbo.open(1)
    _loadLang(db)
    _loadLocalization(db)
    _loadItem(db)
    _loadDrop(db)
    _loadMapUnit(db)
    _loadConfigure(db)
    _loadLordLevel(db)
    _loadLordHead(db)
    _loadForest(db)
    _loadBuildingLevel(db) -- before building
    _loadBuilding(db)
    _loadTakeTax(db)
    _loadArmy(db)
    _loadArmys(db)
    _loadTechnology(db)
    _loadMessage(db)
    _loadMail(db)
    -- _loadActivity(db)
    _loadHero(db)
    _loadHeros(db)
    _loadSkill(db)
    _loadSkillPreBattle(db)
    _loadHeroDraw(db)
    _loadHeroLevel(db)
    _loadHeroStarLevel(db)
    _loadHeroSlotUnlock(db)
    _loadHeroSkillLevel(db)
    _loadEquipLevel(db)
    _loadQualityInfo(db)
    _loadEquipSuccinctInlay(db)
    _loadSuccinctAttribute(db)
    _loadHeroFetter(db)
    _loadHeroFetterAttribute(db)
    _loadBattleSpellBase(db)
    _loadBattleSpellNode(db)
    _loadBattleBuff(db)
    _loadSystemunlock(db)
    _loadMapNpcArmy(db)
    _loadScenarioSection(db)
    _loadScenarioChapter(db)
    _loadStore(db)
    _loadStoreItemLibrary(db)
    _loadStoreRefreshPrice(db)
    _loadArenaRankReward(db)
    _loadArenaRobot(db)
    -- _loadArenaRule(db)
    _loadVip(db)
    _loadStaminaBuy(db)
    _loadAllianceLevel(db)
    _loadAllianceBanner(db)
    _loadAllianceRankDetail(db)
    _loadAllianceScience(db)
    _loadAllianceDonate(db)
    _loadAllianceBuff(db)
    _loadAllianceMessage(db)
    _loadQuest(db)
    _loadDailyTask(db)
    _loadDailyTaskReward(db)
    _loadAchievement(db)
    _loadBabel(db)
    _loadBronzeSparrowTower(db)
    _loadBronzeSparrowTowerRewardList(db)
    _loadMapCity(db)
    _loadChat(db)
    _loadRidingAlone(db)
    _loadTitle(db)
    _loadHeroDrawType(db)
    _loadQuestStory(db)
    _loadQuestDay(db)
    _loadQuestDayPoints(db)
    _loadRandomName(db)
    _loadHeroSoul(db)
    _loadHeroTalent(db)
    _loadHeroLevelAttr(db)
    _loadBattleArrtTransform(db)
    _loadBattleArrt(db)
    _loadEnergyBuy(db)
    _loadTechnologyTreeUnlock(db)
    dbo.disconnectAll(1)
end

return tploader

