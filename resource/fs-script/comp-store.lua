local agent = ...
local p = require('protocol')
local t = require('tploader')
local dbo = require('dbo')
local timer = require('timer')
local event = require('libs/event')
local utils = require('utils')
local misc = require('libs/misc')
local logStub = require('stub/log')

local user = agent.user
local bag = agent.bag
local dict = agent.dict
local cdlist = agent.cdlist
local arena = agent.arena
local building = agent.building
local alliance = agent.alliance
local babel = agent.babel
local bronzeSparrowTower = agent.bronzeSparrowTower

local store = {
    infos = {}, --map<storeType, table<id, storeInfo> >
    datas = {}, --map<storeType, dataInfo>
    evtBuyItem = event.new(), --(type, count)
}

local storeInfo = {
    type = 0,   --StoreType
    id = 0,     --物品库表 id
    isBuy = false,  --是否已购买

    sync = false,
}

local dataInfo = {
    type = 0,               --StoreType
    hadRefreshCount = 0,    --已刷新次数
    -- refresh = false,        --是否可以刷新，到点就变更，后端不主动刷新只做标志，前端根据需求主动请求刷新

    sync = false,
}

--local pktHandlers = {}

function storeInfo:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end

function dataInfo:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end

function store.onInit()
    --agent.registerHandlers(pktHandlers)
    store.dbLoad()

    --元宝商店:每日 0点,12点; 擂台商店:每日 9点,21点;  联盟:每日 5点; 千层楼:每日 5点,17点; 铜雀台: 每日 9点 14点 21点
    --0
    if cdlist.isHour0Refresh then
        store.refresh(p.StoreType.STORE_GOLD, false)
    end
    cdlist.evtHour0Refresh:attachRaw(function()
        store.refresh(p.StoreType.STORE_GOLD, true)
    end)
    --5
    if cdlist.isHour5Refresh then
        store.refresh(p.StoreType.STORE_ALLIANCE, false)
        store.refresh(p.StoreType.STORE_BABEL, false)
    end
    cdlist.evtHour5Refresh:attachRaw(function()
        store.refresh(p.StoreType.STORE_ALLIANCE, true)
        store.refresh(p.StoreType.STORE_BABEL, true)
    end)
    --9
    if cdlist.isHour9Refresh then
        store.refresh(p.StoreType.STORE_ARENA, false)
       -- store.refresh(p.StoreType.STORE_BRONZE_SPARROW_TOWER, false)
    end
    cdlist.evtHour9Refresh:attachRaw(function()
        store.refresh(p.StoreType.STORE_ARENA, true)
       -- store.refresh(p.StoreType.STORE_BRONZE_SPARROW_TOWER, true)
    end)
    --12
    if cdlist.isHour12Refresh then
        store.refresh(p.StoreType.STORE_GOLD, false)
    end
    cdlist.evtHour12Refresh:attachRaw(function()
        store.refresh(p.StoreType.STORE_GOLD, true)
    end)
    --14
    if cdlist.isHour14Refresh then
--        store.refresh(p.StoreType.STORE_BRONZE_SPARROW_TOWER, false)
    end
    cdlist.evtHour14Refresh:attachRaw(function()
--        store.refresh(p.StoreType.STORE_BRONZE_SPARROW_TOWER, true)
    end)
    --17
    if cdlist.isHour17Refresh then
        store.refresh(p.StoreType.STORE_BABEL, false)
    end
    cdlist.evtHour17Refresh:attachRaw(function()
        store.refresh(p.StoreType.STORE_BABEL, true)
    end)
    --21
    if cdlist.isHour21Refresh then
        store.refresh(p.StoreType.STORE_ARENA, false)
       -- store.refresh(p.StoreType.STORE_BRONZE_SPARROW_TOWER, false)
    end
    cdlist.evtHour21Refresh:attachRaw(function()
        store.refresh(p.StoreType.STORE_ARENA, true)
       -- store.refresh(p.StoreType.STORE_BRONZE_SPARROW_TOWER, true)
    end)

    --加入联盟
    alliance.evtAllianceJoin:attachRaw(store.onAllianceJoin)
    --联盟升级
    alliance.evtAllianceUpgrade:attachRaw(store.onAllianceUpgrade)
    --建筑建造或升级
    building.evtBuildingLevelUp:attachRaw(store.onBuildingLevelUp)

    -- 确保所有开放的商店都有数据
    for k, v in pairs(t.store) do
        if store.checkUnlockCond(k) then
            if not store.infos[k] then
                store.storeRefresh(k)
            end
            if not store.datas[k] then
                store.datas[k] = dataInfo:new({
                    type = k,
                    hadRefreshCount = 0,
                    sync = false,
                    })
            end
        end
    end
end

function store.onAllCompInit()
    store.sendStoreInfoUpdate()
end

function store.onSave()
    store.dbSave()
end

function store.onClose()
    store.dbSave()
end

function store.onTimerUpdate()
end

function store.dbLoad()
    local infos = dict.get("store.infos")
    if infos then
        for k, v in pairs(infos) do
            if store.infos[v.type] == nil then
                store.infos[v.type] = {}
            end
            store.infos[v.type][v.id] = storeInfo:new({
                    type = v.type,
                    id = v.id,
                    isBuy = v.isBuy,
                    sync = false,
                    })
        end
    end

    local datas = dict.get("store.datas")
    if datas then
        for _, v in pairs(datas) do
            store.datas[v.type] = dataInfo:new({
                    type = v.type,
                    hadRefreshCount = v.hadRefreshCount,
                    sync = false,
                    })
        end
    end

    -- print('####store.dbLoad...infos...', utils.serialize(store.infos))
    -- print('####store.dbLoad...datas...', utils.serialize(store.datas))
end

function store.dbSave()
    --store.infos
    local infos = {}
    for k, store in pairs(store.infos) do
        for _, v in pairs(store) do
            local info = {}
            info.type = k
            info.id = v.id
            info.isBuy = v.isBuy
            table.insert(infos, info)
        end
    end
    dict.set("store.infos", infos)

    --store.datas
    local datas = {}
    for k, v in pairs(store.datas) do
        local info = {}
        info.type = k
        info.hadRefreshCount = v.hadRefreshCount
        table.insert(datas, info)
    end
    dict.set("store.datas", datas)

    -- print('####store.dbSave...infos...', utils.serialize(infos))
    -- print('####store.dbSave...datas...', utils.serialize(datas))
end

function store.getStoreLevel(storeType, tpl)
    local level = 1
    if storeType == p.StoreType.STORE_ALLIANCE then
        --不同的联盟等级对应相应等级的商店
        local allianceLevel = alliance.allianceLevel()
        for _, v in pairs(tpl) do
            --openAllianceLevel
            if allianceLevel >= v.openAllianceLevel and level < v.level then
                level = v.level
            end
        end
    end
    return level
end

function store.refresh(storeType, sendUpdate)
    -- print('store.refresh...storeType', storeType)
    if store.checkUnlockCond(storeType) then
        store.storeRefresh(storeType)
        store.storeDataRefresh(storeType)
        if sendUpdate then
            store.sendStoreInfoUpdate()
        end
    end
end

function store.checkUnlockCond(storeType)
    if storeType == p.StoreType.STORE_GOLD then
        --元宝商店
        local buildingLevel = building.getBuildingLevelByType(p.BuildingType.MARKET)
        return buildingLevel > 0
    elseif storeType == p.StoreType.STORE_ARENA then
        --擂台商店
        local buildingLevel = building.getBuildingLevelByType(p.BuildingType.ARENA)
        return buildingLevel > 0
    elseif storeType == p.StoreType.STORE_ALLIANCE then
        --联盟
        local aid = alliance.allianceId()
        return aid > 0
    elseif storeType == p.StoreType.STORE_BABEL then
        --千层楼
        local buildingLevel = building.getBuildingLevelByType(p.BuildingType.THOUSAND_PARIDIS)
        return buildingLevel > 0
    elseif storeType == p.StoreType.STORE_BRONZE_SPARROW_TOWER then
        --铜雀台
        local buildingLevel = building.getBuildingLevelByType(p.BuildingType.BRONZE_SWALLOW_TERRACE)
        return buildingLevel > 0
    end
    return false
end

function store.storeRefresh(storeType)
    -- print('store.storeRefresh....storeType', storeType)
    local tpl = t.store[storeType]
    if tpl then
        local level = store.getStoreLevel(storeType, tpl)
        local maxStoreLevel = #tpl
        if level > maxStoreLevel then
            level = maxStoreLevel
        end
        local storeTpl = tpl[level]
        if storeTpl then
            local libList = {}
            local itemLid = storeTpl.lowItemLid or 0
            local count = storeTpl.lowCount or 0
            table.insert(libList, { itemLid = itemLid, count = count })

            itemLid = storeTpl.mediumItemLid or 0
            count = storeTpl.mediumCount or 0
            table.insert(libList, { itemLid = itemLid, count = count })

            itemLid = storeTpl.highItemLid or 0
            count = storeTpl.highCount or 0
            table.insert(libList, { itemLid = itemLid, count = count })

            local tempList = {}
            local tempCount = 0
            for _, lib in pairs(libList) do
                local itemLid = lib.itemLid
                local count = lib.count
                if itemLid > 0 and count > 0 then
                    local itemList = t.storeItemLibrary[itemLid]
                    if itemList then
                        --商品列表
                        local myRandList = {}
                        local total = 0
                        for _, v in pairs(itemList) do
                            if v.weight == 0 then
                                -- 权重为0必刷
                                tempList[v.id] = storeInfo:new({ type = storeType, id = v.id, isBuy = false })
                                count = count - 1
                                if count == 0 then
                                    break
                                end
                            else
                                total = total + v.weight
                                table.insert(myRandList, { id = v.id, rate = v.weight })
                            end
                        end
                        --随机商品
                        while count > 0 and #myRandList > 0 do
                            local randValue = utils.getRandomNum(1, total+1)
                            local preKey = 1
                            local sum = 0
                            for k, v3 in ipairs(myRandList) do
                                if sum + v3.rate > randValue then
                                    break
                                else
                                    preKey = k
                                    sum = sum + v3.rate
                                end
                            end
                            local item = myRandList[preKey]
                            table.remove(myRandList, preKey)

                            tempList[item.id] = storeInfo:new({ type = storeType, id = item.id, isBuy = false })
                            total = total - item.rate
                            count = count - 1
                            tempCount = tempCount + 1
                        end
                    end
                end
            end

            -- print('store.storeRefresh...tempCount, tempList', tempCount, utils.serialize(tempList))
            if tempCount > 0 then
                store.infos[storeType] = {}
                store.infos[storeType] = tempList
                return true
            end
        end
    end
    return false
end

function store.storeDataRefresh(storeType)
    -- print('store.storeDataRefresh....storeType', storeType)
    if not store.datas[storeType] then
        store.datas[storeType] = dataInfo:new({
            type = storeType,
            hadRefreshCount = 0,
            sync = false,
        })
    else
        store.datas[storeType].hadRefreshCount = 0
        store.datas[storeType].sync = false
    end
end

function store.onAllianceJoin()
    -- print('store.onAllianceJoin...')
    store.refresh(p.StoreType.STORE_ALLIANCE, true)
end

function store.onAllianceUpgrade()
    -- print('store.onAllianceUpgrade...')
    store.refresh(p.StoreType.STORE_ALLIANCE, true)
end

function store.onBuildingLevelUp(tplId, level)
    -- print('store.onBuildingLevelUp...tplId, level', tplId, level)
    local tpl = t.building[tplId]
    if tpl then
        if tpl.buildingType == p.BuildingType.MARKET then
            store.refresh(p.StoreType.STORE_GOLD, true)
        elseif tpl.buildingType == p.BuildingType.ARENA then
            store.refresh(p.StoreType.STORE_ARENA, true)
        elseif tpl.buildingType == p.BuildingType.THOUSAND_PARIDIS then
            store.refresh(p.StoreType.STORE_BABEL, true)
    --    elseif tpl.buildingType == p.BuildingType.BRONZE_SWALLOW_TERRACE then
    --        store.refresh(p.StoreType.STORE_BRONZE_SPARROW_TOWER, true)
        end
    end
end

function store.isPriceEnogh(coinType, price)
    if coinType == nil or price == nil then
        return
    end
    if coinType == p.StoreCoinType.GOLD then
        return user.isResourceEnough(p.SpecialPropIdType.GOLD, price)
    elseif coinType == p.StoreCoinType.SILVER then
        return user.isResourceEnough(p.SpecialPropIdType.SILVER, price)
    elseif coinType == p.StoreCoinType.WIN_POINT then
        return arena.isWinPointEnough(price)
    elseif coinType == p.StoreCoinType.ALLIANCE_SCORE then
        return alliance.isAllianceScoreEnough(price)
    elseif coinType == p.StoreCoinType.BABEL_SCORE then
        return babel.isBabelScoreEnough(price)
--    elseif coinType == p.StoreCoinType.BRONZE_SPARROW_TOWER_SCORE then
--        return bronzeSparrowTower.isBronzeScoreEnough(price)
    end
    return false
end

function store.storeConsume(coinType, price, consumeType)
    if coinType == p.StoreCoinType.GOLD then
        return user.removeResource(p.SpecialPropIdType.GOLD, price, consumeType)
    elseif coinType == p.StoreCoinType.SILVER then
        return user.removeResource(p.SpecialPropIdType.SILVER, price, consumeType)
    elseif coinType == p.StoreCoinType.WIN_POINT then
        return arena.removeWinPoint(price, consumeType)
    elseif coinType == p.StoreCoinType.ALLIANCE_SCORE then
        return alliance.removeAllianceScore(price, consumeType)
    elseif coinType == p.StoreCoinType.BABEL_SCORE then
        return babel.removeBabelScore(price, consumeType)
--    elseif coinType == p.StoreCoinType.BRONZE_SPARROW_TOWER_SCORE then
--        return bronzeSparrowTower.removeBronzeScore(price, consumeType)
    end
    return false
end

function store.getStoreRefreshLeftTime(storeType)
    local leftTime = 0
    local now = timer.getTimestampCache()
    local t = os.date('*t', now)
    local expired5Timestamp = os.time({year = t.year, month = t.month, day = t.day, hour = 5})
    local expired9Timestamp = os.time({year = t.year, month = t.month, day = t.day, hour = 9})
    local expired12Timestamp = os.time({year = t.year, month = t.month, day = t.day, hour = 12})
    local expired14Timestamp = os.time({year = t.year, month = t.month, day = t.day, hour = 14})
    local expired17Timestamp = os.time({year = t.year, month = t.month, day = t.day, hour = 17})
    local expired21Timestamp = os.time({year = t.year, month = t.month, day = t.day, hour = 21})
    local expired24Timestamp = os.time({year = t.year, month = t.month, day = t.day, hour = 24})
    if storeType == p.StoreType.STORE_GOLD then
        --元宝商店:每日 0点,12点
        if now > expired12Timestamp and now <= expired24Timestamp then
            leftTime = expired24Timestamp - now
        else
            leftTime = expired12Timestamp - now
        end
    elseif storeType == p.StoreType.STORE_ARENA then
        --擂台商店:每日  9点,21点
        if now > expired9Timestamp and now <= expired21Timestamp then
            leftTime = expired21Timestamp - now
        else
            if now <= expired9Timestamp then
                leftTime = expired9Timestamp - now
            elseif now > expired21Timestamp then
                leftTime = expired9Timestamp + 24 * 60 * 60 - now
            end
        end
    elseif storeType == p.StoreType.STORE_ALLIANCE then
        --联盟:每日 5点
        if now <= expired5Timestamp then
            leftTime = expired5Timestamp - now
        else
            leftTime = expired5Timestamp + 24 * 60 * 60 - now
        end
    elseif storeType == p.StoreType.STORE_BABEL then
        --千层楼:每日 5点,17点
        if now > expired5Timestamp and now <= expired17Timestamp then
            leftTime = expired17Timestamp - now
        else
            if now <= expired5Timestamp then
                leftTime = expired5Timestamp - now
            elseif now > expired17Timestamp then
                leftTime = expired17Timestamp + 24 * 60 * 60 - now
            end
        end
    --elseif storeType == p.StoreType.STORE_BRONZE_SPARROW_TOWER then
    --    --铜雀台: 每日 9点 14点 21点
    --    if now > expired9Timestamp and now <= expired14Timestamp then
    --        leftTime = expired14Timestamp - now
    --    elseif now > expired14Timestamp and now <= expired21Timestamp then
    --        leftTime = expired21Timestamp - now
    --    else
    --        if now <= expired9Timestamp then
    --           leftTime = expired9Timestamp - now
    --        elseif now > expired21Timestamp then
    --            leftTime = expired9Timestamp + 24 * 60 * 60 - now
    --        end
    --    end
    end
    if leftTime < 0 then
        leftTime = 0
    end
    -- print('store.getStoreRefreshLeftTime.....storeType, leftTime', storeType, leftTime)
    return leftTime
end

function store.sendStoreInfoUpdate()
    local updateList = {}
    for k, info in pairs(store.infos) do
        local itemList = {}
        for _, v in pairs(info) do
            if not v.sync then
                v.sync = true
                table.insert(itemList, { id = v.id, isBuy = v.isBuy })
            end
        end
        if next(itemList) then
            local data = store.datas[k] or {}
            local refreshCount = data.hadRefreshCount or 0
            local leftTime = store.getStoreRefreshLeftTime(k)
            table.insert(updateList, { storeType = k, refreshCount = refreshCount, leftTime = leftTime, itemList = itemList })
            -- print('store.sendStoreInfoUpdate...storeType, leftTime, itemList', k, leftTime, utils.serialize(itemList))
        end
    end

    -- print('-----------------store.sendStoreInfoUpdate...store.infos', utils.serialize(store.infos))

    if next(updateList) then
        -- print('store.sendStoreInfoUpdate...updateList', utils.serialize(updateList))
        agent.sendPktout(p.SC_STORE_INFO_UPDATE, '@@1=[storeType=i,refreshCount=i,leftTime=i,itemList=[id=i,isBuy=b]]', updateList)
    end
end

--pktHandlers[p.CS_STORE_BUY] = function(pktin, session)
function store.cs_store_buy(storeType, id)
    local function storeBuyResponse(result)
        -- print('p.CS_STORE_BUY...result', result)
        agent.replyPktout(session, p.SC_STORE_BUY_RESPONSE, result)
    end

    local storeItemsTpl = t.storeItems[id]
    if storeItemsTpl == nil then
        print('p.CS_STORE_BUY...tpl_store_item_library no datas , id=', id)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    --1.检查是否有该商店及是否有该商品
    local storeTemp = store.infos[storeType]
    if storeTemp == nil then
        print('p.CS_STORE_BUY... no this store, storeType=', storeType)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local storeGoods = storeTemp[id]
    if storeGoods == nil then
        print('p.CS_STORE_BUY... store no this goods, storeType, id =', storeType, id)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --2.检查商品是否卖完
    if storeGoods.isBuy then
        print('p.CS_STORE_BUY... the goods have been sold out , storeType, id, isBuy =', storeType, id, isBuy)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --3.检查货币是否足够
    local coinType = storeItemsTpl.coinType
    local price = storeItemsTpl.price
    if price <= 0 or not store.isPriceEnogh(coinType, price) then
        print('p.CS_STORE_BUY...  , price is not enogh, storeType, coinType, price =', storeType, coinType, price)
        local param1 = tostring(coinType) or ''
        agent.sendNoticeMessage(p.ErrorCode.STORE_COIN_NOT_ENOUGH, param1, 1)
        return
    end
    --消耗
    store.storeConsume(coinType, price, p.ResourceConsumeType.STORE_BUY)
    --pick bag
    local tplId = storeItemsTpl.itemId or 0
    local count = storeItemsTpl.count or 0
    bag.addItemByCond(tplId, count, {}, p.ResourceGainType.STORE_BUY, true)
    --log
    logStub.appendStore(user.uid, storeType, coinType, price, tplId, count, timer.getTimestampCache())

    storeGoods.isBuy = true
    storeGoods.sync = false
    store.evtBuyItem:trigger(storeType, 1)
    store.sendStoreInfoUpdate()
    storeBuyResponse(p.ErrorCode.STORE_BUY_SUCCESS)
end

--pktHandlers[p.CS_STORE_REFRESH] = function(pktin, session)
function store.cs_store_refresh(storeType)
    --1.检查是否有该商店
    local storeTpl = t.store[storeType]
    if storeTpl == nil then
        print('p.CS_STORE_REFRESH...tpl_store no datas , storeType=', storeType)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local refreshTpl = t.storeRefreshPrice[storeType]
    if refreshTpl == nil then
        print('p.CS_STORE_REFRESH...tpl_store_refresh_price no datas , storeType=', storeType)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --2.
    local storeData = store.datas[storeType]
    --检查刷新次数是否已达最大值 最大值max跟vip有关
    local max = agent.vip.tpl.storeRefreshCount
    if storeData.hadRefreshCount > max then
        print('p.CS_STORE_REFRESH...tpl_store refresh, hadRefreshCount > max...storeType, hadRefreshCount, max=', storeType, storeData.hadRefreshCount, max)
        agent.sendNoticeMessage(p.ErrorCode.PUBLIC_VIP_REFRESH_MAX, '', 1)
        return
    end
    --
    local refreshData = refreshTpl[storeData.hadRefreshCount + 1]
    if refreshData == nil then
        print('p.CS_STORE_REFRESH...tpl_store_refresh_price no datas , storeType, count=', storeType, storeData.hadRefreshCount+1)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --检查货币是否足够
    if not store.isPriceEnogh(refreshData.coinType, refreshData.price) then
        print('p.CS_STORE_REFRESH...  , price is not enogh, storeType, coinType, price =', storeType, refreshData.coinType, refreshData.price)
        local param1 = tostring(refreshData.coinType) or ''
        agent.sendNoticeMessage(p.ErrorCode.STORE_COIN_NOT_ENOUGH, param1, 1)
        return
    end
    --刷新
    if store.storeRefresh(storeType) then
        --消耗
        store.storeConsume(refreshData.coinType, refreshData.price, p.ResourceConsumeType.STORE_REFRESH)
        storeData.hadRefreshCount = storeData.hadRefreshCount + 1
        storeData.sync = false
    else
        print('p.CS_STORE_REFRESH... refresh is wrong, storeType=', storeType)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    store.sendStoreInfoUpdate()
end

return store
