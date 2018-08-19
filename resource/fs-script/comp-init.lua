local agent = ...
local utils = require('utils')
local vm = require('vm')
local p = require('protocol')
local t = require('tploader')
local clientTpl = require('clientTpl')

local init = {}

local _pktHandlers = {}

function init.onInit()
    agent.registerHandlers(_pktHandlers)
end

-- 处理模板更新检查
_pktHandlers[p.CS_TEMPLATE_CHECK] = function(pktin)
    local clientTemplates = clientTpl.getTemplates()
    local exist = {}
    local len = pktin:readInteger()
    for i = 1, len do
        local name, hash = pktin:read('ss')
        --print('client tpl name, hash = ',  name, hash)
        exist[name] = hash
    end
    local updates = {}
    for name, template in pairs(clientTemplates) do
        -- print('templates name = ', name)
        local existHash = exist[name]
        if existHash == nil or existHash ~= template.hash then
            table.insert(updates, template)
            --print('updates tpl name, hash = ',  name, template.hash)
        end
    end
    local total, contentMaxSize = #updates, 60000
    -- print('total, contentMaxSize ', total, contentMaxSize )
    if total == 0 then
        agent.sendPktout(p.SC_TEMPLATE_UPDATE, 0, 0, '', 0, 0, '')
    else
        for i = 1, total, 1 do
            local template = updates[i]
            local partTotal = math.ceil(string.len(template.content) / contentMaxSize)
            for partProgress = 1, partTotal, 1 do
                local content = string.sub(template.content, contentMaxSize * (partProgress - 1) + 1, contentMaxSize * partProgress)
                -- print('i, total, name, partProgress, partTotal', i, total, template.name, partProgress, partTotal)
                agent.sendPktout(p.SC_TEMPLATE_UPDATE, i, total, template.name, partProgress, partTotal, content)
            end
        end
    end
end


function init.onAllCompInit()
    local user = agent.user
    local building = agent.building
    local army = agent.army
    local hero = agent.hero
    local bag = agent.bag

    --[[  TODO test code  ]]
    if user.info.gold < 1000000 then
        --user.info.gold = 20000000
    end
    -- army.putInHospital({{armyType=1,count=1000, power=0}, {armyType=2,count=1000, power=0}, {armyType=3,count=1000, power=0}, {armyType=4,count=1000, power=0}, {armyType=5,count=1000, power=0}, {armyType=6,count=1000, power=0}, {armyType=7,count=1000, power=0}, {armyType=8,count=1000, power=0}})
    if user.sdkType == 'client_test' then -- only for testing
        if user.newPlayer then
            print("######### user for testing begin ##########")
            user.info.level = utils.getRandomNum(5, 30)
            user.info.silver = 2000000
            user.info.gold = 2000000
            user.info.food = 2000000
            user.info.wood = 2000000
            user.info.iron = 2000000
            user.info.stone = 2000000
            user.info.registerTimestamp = 0
            print("######### user for testing end ##########")

            print("######### army for testing begin ##########")
            for k,v in pairs(army.armies) do
                army.armies[k] = nil
            end
            army._add(p.ArmyType.SABER, p.ArmyState.NORMAL, 20000)
            army._add(p.ArmyType.PIKEMAN, p.ArmyState.NORMAL, 20000)
            army._add(p.ArmyType.HALBERDIER, p.ArmyState.NORMAL, 20000)
            army._add(p.ArmyType.ARCHER, p.ArmyState.NORMAL, 20000)
            army._add(p.ArmyType.RIDER, p.ArmyState.NORMAL, 20000)
            army._add(p.ArmyType.CHARIOT, p.ArmyState.NORMAL, 20000)
            army._add(p.ArmyType.STONE_THROWER, p.ArmyState.NORMAL, 20000)
            army._add(p.ArmyType.WAR_ELEPHANT, p.ArmyState.NORMAL, 20000)

            print("######### army for testing end ##########")

            print("######### building for testing begin ##########")
            -- 农田
            for i=100, 104 do
                building.list[i] =  building.newBuildingInfo({ gridId = i, tplId = 1230000, level = 3 })
            end
            -- 伐木场
            for i=105, 109 do
                building.list[i] =  building.newBuildingInfo({ gridId = i, tplId = 1231000, level = 3 })
            end
            print("######### building for testing end ##########")

            -- print("######## bag for testing begin ########")
            -- 装备
            -- for _, v in pairs(t.item) do
            --     if v.subType ~= 3001 and v.subType ~= 11001 and v.id > 1000 then
            --         bag.addItem(v.id, utils.getRandomNum(200, 1000), p.ResourceGainType.DEFAULT, false)
            --     end
            -- end


            -- bag.addItemByCond(2100001, utils.getRandomNum(1, 5), {level=1,exp=10}, p.ResourceGainType.DEFAULT, false)
            -- bag.addItemByCond(2100001, utils.getRandomNum(1, 5), {level=2,exp=0}, p.ResourceGainType.DEFAULT, false)
            -- bag.addItemByCond(2100002, utils.getRandomNum(1, 5), {level=1,exp=11}, p.ResourceGainType.DEFAULT, false)
            -- bag.addItemByCond(2100003, utils.getRandomNum(1, 5), {level=1,exp=12}, p.ResourceGainType.DEFAULT, false)
            -- bag.addItemByCond(2100004, utils.getRandomNum(1, 5), {level=1,exp=13}, p.ResourceGainType.DEFAULT, false)
            -- bag.addItemByCond(2100005, utils.getRandomNum(1, 5), {level=1,exp=14}, p.ResourceGainType.DEFAULT, false)
            -- bag.addItemByCond(2100006, utils.getRandomNum(1, 5), {level=1,exp=15}, p.ResourceGainType.DEFAULT, false)
            -- bag.addItemByCond(2190001, utils.getRandomNum(1, 5), {level=1,exp=16}, p.ResourceGainType.DEFAULT, false)
            -- bag.addItemByCond(2190002, utils.getRandomNum(1, 5), {level=1,exp=17}, p.ResourceGainType.DEFAULT, false)
            -- bag.addItemByCond(2190003, utils.getRandomNum(1, 5), {level=1,exp=18}, p.ResourceGainType.DEFAULT, false)
            -- bag.addItemByCond(2190004, utils.getRandomNum(1, 5), {level=1,exp=19}, p.ResourceGainType.DEFAULT, false)
            -- bag.addItemByCond(2190005, utils.getRandomNum(1, 5), {level=1,exp=20}, p.ResourceGainType.DEFAULT, false)
            -- bag.addItemByCond(2090001, utils.getRandomNum(1, 5), {level=1,exp=21}, p.ResourceGainType.DEFAULT, false)
            -- bag.addItemByCond(2090002, utils.getRandomNum(1, 5), {level=1,exp=22}, p.ResourceGainType.DEFAULT, false)

            -- print("######## bag for testing end ########")

            print("######### hero for testing begin ##########")
            hero.addHero(1300001)
            hero.addHero(1300002)
            hero.addHero(1300003)
            hero.addHero(1300004)
            hero.addHero(1300005)
            hero.addHeroByCond(1300001, 1, 2)
            hero.addHeroByCond(1300005, 3, 1)
            hero.addHeroByCond(1300006, 4, 3)
            print("######### hero for testing end ##########")
        end
    end
    
end

return init
