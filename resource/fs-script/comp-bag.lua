local agent = ...
local p = require('protocol')
local t = require('tploader')
local timer = require('timer')
local event = require('libs/event')
local misc = require('libs/misc')
local dbo = require('dbo')
local utils = require('utils')
local idStub = require('stub/id')
local dataStub = require('stub/data')
local logStub = require('stub/log')

local user = agent.user
local buff = agent.buff

--背包格子数上限
local bagItemLimit = t.configure['bagItemLimit'] or 500
local equipMeltBaseSilver = t.configure['equipMeltBaseSilver'] or 100


local bag = {
    bag_items = {}, -- 背包物品[bid]=bagItemInfo
    bagCount = 0,   --背包已用格子数

    evtUse = event.new(),  --(itemId, count)
    evtTypeUse = event.new(), --(type, count)
}

local bagItemInfo = {
    bid = 0,
    tpl = {},
    count = 0,
    data = {},    --针对装备 宝物 技能书 {level=0,exp=0}
    ext1 = {},
    ext2 = {},
    --[[
    if 装备 then
        ext1 = 洗练属性列表{[洗练位]={属性ID1},...}
        ext2 = 镶嵌宝石列表{[孔位]=宝石ID,...}
    end
    --]]

    sync = false,
    dirty = false,
}

function bagItemInfo:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end

function bagItemInfo:create(tpl, count)
    local data = {}
    local ext1 = {}
    local ext2 = {}
    if tpl.type == p.ItemType.EQUIP
        or tpl.type == p.ItemType.TREASURE
        or tpl.subType == p.ItemPropType.ATTACK_GEM
        or tpl.subType == p.ItemPropType.DEFENSE_GEM
        or tpl.subType == p.ItemPropType.SPECIAL_GEM
        or tpl.subType == p.ItemPropType.SKILL_BOOK then
        data = { level = 1, exp = 0 }
    end

    local o = {
        bid = utils.createItemId(),
        tpl = tpl,
        count = count,
        data = data,
        ext1 = ext1,
        ext2 = ext2,
        sync = false,
        dirty = true,
    }
    local info = bagItemInfo:new(o)
    return info
end

function bag.onInit()
    bag.dbLoad()

    if user.newPlayer then
        local conf = t.configure['InitBag']
        if conf then
            for tplId, count in pairs(conf) do
                local tpl = t.item[tplId]
                if tpl ~= nil and count > 0 then
                    local info = bagItemInfo:create(tpl, count)
                    if info and info.bid > 0 then
                        bag.bag_items[info.bid] = info
                        bag.bagCount = bag.bagCount + 1
                    end
                end
            end
        end
    end
end

function bag.onAllCompInit()
    bag.sendBagUpdate()
end

function bag.onClose()
    bag.dbSave()
end

function bag.onSave()
    bag.dbSave()
end

function bag.dbLoad()
    local db = dbo.open(0)
    -- load bag
    local rs = db:executePrepare('SELECT bid, itemId, count, data, ext1, ext2 from s_bag where uid = ? and count > 0', user.uid)
    if rs.ok then
        for _, row in ipairs(rs) do
            local tpl = t.item[row.itemId]
            if tpl ~= nil and row.count > 0 then
                row.data = misc.deserialize(row.data)
                row.ext1 = misc.deserialize(row.ext1)
                row.ext2 = misc.deserialize(row.ext2)
                local info = bagItemInfo:new({ bid = row.bid, tpl = tpl, count = row.count, data = row.data, ext1 = row.ext1, ext2 = row.ext2 })
                bag.bag_items[info.bid] = info
                bag.bagCount = bag.bagCount + 1
            end
        end
    end
end

function bag.dbSave(timingSave)
    local db = dbo.open(0)

    local bag_list = {}
    for _, v in pairs(bag.bag_items) do
        if v.dirty then
            v.dirty = false
            table.insert(bag_list, { bid = v.bid, uid = user.uid, itemId = v.tpl.id, count = v.count, data = utils.serialize(v.data), ext1 = utils.serialize(v.ext1), ext2 = utils.serialize(v.ext2) })
        end
    end
    dataStub.appendDataList(p.DataClassify.BAG, bag_list)
end

function bag.pickDropItemsByDropId(dropId, gainType)
    local dropTpl = t.drop[dropId]
    local drops = {}
    if dropTpl then
        drops = dropTpl:DoDrop() or {}
        bag.pickDropItems(drops, gainType)
    else
        print('dropId not exist', dropId)
    end
    return drops
end

function bag.pickDropItems(drops, gainType)
    for _, drop in pairs(drops) do
        if drop.tplId == p.SpecialPropIdType.FOOD then
            user.addResource(p.ResourceType.FOOD, drop.count, gainType)
        elseif drop.tplId == p.SpecialPropIdType.WOOD then
            user.addResource(p.ResourceType.WOOD, drop.count, gainType)
        elseif drop.tplId == p.SpecialPropIdType.IRON then
            user.addResource(p.ResourceType.IRON, drop.count, gainType)
        elseif drop.tplId == p.SpecialPropIdType.STONE then
            user.addResource(p.ResourceType.STONE, drop.count, gainType)
        elseif drop.tplId == p.SpecialPropIdType.GOLD then
            user.addResource(p.ResourceType.GOLD, drop.count, gainType)
        elseif drop.tplId == p.SpecialPropIdType.SILVER then
            user.addResource(p.ResourceType.SILVER, drop.count, gainType)
        elseif drop.tplId == p.SpecialPropIdType.STAMINA then
            user.addStamina(drop.count, gainType)
        elseif drop.tplId == p.SpecialPropIdType.ENERGY then
            user.addEnergy(drop.count, gainType)
        elseif drop.tplId == p.SpecialPropIdType.LORD_EXP then
            user.addExp(drop.count)
        elseif drop.tplId == p.SpecialPropIdType.VIP_EXP then
            agent.vip.addExp(drop.count, true)
        elseif drop.tplId == p.SpecialPropIdType.ALLIANCE_STORE then
            agent.alliance.AddAllianceScore(drop.count, gainType)
        elseif drop.tplId == p.SpecialPropIdType.WIN_POINT then
            agent.arena.addWinPoint(drop.count, gainType)
        elseif drop.tplId == p.SpecialPropIdType.BABEL_SCORE then
            agent.babel.addBabelScore(drop.count, gainType)
        -- 武将经验 不放入背包
        elseif drop.tplId == p.SpecialPropIdType.HERO_EXP then
            
        else
            --item
            if t.item[drop.tplId] == nil then
                print('bag.pickDropItems  not exist drop.tplId=', drop.tplId)
            else
                local cond = drop.cond or {}
                bag.addItemByCond(drop.tplId, drop.count, cond, gainType, false)
            end
        end
    end
    bag.sendBagUpdate()
end

function bag.checkItemEnough(tplId, count)
    -- return isEnough, lackCount, lackGold
    -- 返回 false, 0 表示异常

    local tpl = t.item[tplId]
    if not tpl or count < 1 then
        utils.log(string.format('bag.checkItemEnough tpl not exist or count < 1 : uid=%i, tplId=%i, count=%i', user.uid, tplId, count))
        return false, 0, 0
    end

    local gotCount = 0
    for _, v in pairs(bag.bag_items) do
        if v.tpl.id == tplId and v.count > 0 then
            gotCount = gotCount + v.count
        end
    end

    if gotCount > 0 then
        if gotCount >= count then
            return true, 0, 0
        else
            local lackCount = count - gotCount
            return false, lackCount, lackCount * tpl.price
        end
    end

    return false, count, count * tpl.price
end

function bag.checkItemsEnough(list)
    -- list = {[tplId]=count,...}
    for tplId, count in pairs(list) do
        local isEnough = bag.checkItemEnough(tplId, count)
        if not isEnough then
            return false
        end
    end
    return true
end

function bag.checkResourcesEnough(list)
    for tplId, count in pairs(list) do
        local isEnough = user.isResourceEnough(tplId, count)
        if not isEnough then
            return false
        end
    end
    return true
end

function bag.removeResources(list, consumeType)
    for tplId, count in pairs(list) do
        if not user.removeResource(tplId, count, consumeType) then
            utils.log(string.format('bag.removeResources failed : uid=%i, tplId=%i, count=%i', user.uid, tplId, count))
        end
    end
    return true
end

--物品是否可叠加 cond={ level = i, exp = i, ext1 = {...}, ext2 = {...} }
function bag.checkCanAccumulateByCond(cond)
    local level, exp, ext1, ext2 = 1, 0, {}, {}
    if cond then
        level  = cond.level or level
        exp = cond.exp or exp
        ext1 = cond.ext1 or ext1
        ext2 = cond.ext2 or ext2
    end
    -- print('bag.checkCanAccumulateByCond...level, exp, ext1, ext2 ', level, exp, utils.serialize(ext1), utils.serialize(ext2))
    if level == 1 and exp == 0 and not next(ext1) and not next(ext2) then
        return true
    end
    return false
end

function bag.addItems(items, gainType)
    -- print('scenarioCopy.pickDropItems....drops, gainType, heroList', utils.serialize(drops), gainType, heroList)
    for _, item in pairs(items) do
        if item.tplId == p.SpecialPropIdType.FOOD then
            user.addResource(p.ResourceType.FOOD, item.count, gainType, 0, false)
        elseif item.tplId == p.SpecialPropIdType.WOOD then
            user.addResource(p.ResourceType.WOOD, item.count, gainType, 0, false)
        elseif item.tplId == p.SpecialPropIdType.IRON then
            user.addResource(p.ResourceType.IRON, item.count, gainType, 0, false)
        elseif item.tplId == p.SpecialPropIdType.STONE then
            user.addResource(p.ResourceType.STONE, item.count, gainType, 0, false)
        elseif item.tplId == p.SpecialPropIdType.GOLD then
            user.addResource(p.ResourceType.GOLD, item.count, gainType, 0, false)
        elseif item.tplId == p.SpecialPropIdType.SILVER then
            user.addResource(p.ResourceType.SILVER, item.count, gainType, 0, false)
        elseif item.tplId == p.SpecialPropIdType.STAMINA then
            user.addStamina(item.count, gainType)
        elseif item.tplId == p.SpecialPropIdType.ENERGY then
            user.addEnergy(item.count, gainType)
        elseif item.tplId == p.SpecialPropIdType.LORD_EXP then
            user.addExp(item.count, false)
        else
            --item
            if t.item[item.tplId] == nil then
                print('tpl_item not exist tplId=', item.tplId)
            else
                bag.addItem(item.tplId, item.count, gainType, false)
            end
        end
    end
    bag.sendBagUpdate()
    user.sendUpdate()
end


function bag.addItem(itemId, count, gainType, sendUpdate)
    -- print('###bag.addItem...itemId, count, gainType, sendUpdate', itemId, count, gainType, sendUpdate)
    if not gainType then
        gainType = p.ResourceGainType.DEFAULT
    end
    if sendUpdate == nil then
        sendUpdate = true
    end

    local tpl = t.item[itemId]
    if not tpl then
        utils.log(string.format('tpl_item no such item, uid=%i, tplId=%i', user.uid, itemId))
        return false
    end
    if count < 1 then
        return false
    end

    local item = nil
    for _, v in pairs(bag.bag_items) do
        local cond = { level = v.data.level or 1, exp = v.data.exp or 0, ext1 = v.ext1, ext2 = v.ext2 }
        if v.tpl.id == itemId and v.count > 0 and bag.checkCanAccumulateByCond(cond) then
            item = v
            break
        end
    end
    local tempCount = 0
    if item == nil then
        if bag.checkBagIsFull() then
            --邮件发送
            local mailType = p.MailType.SYSTEM
            local mailSubType = p.MailSubType.SYSTEM_BAG_FULL
            local title, content = agent.mail.getTitleContentBySubType(mailSubType)
            local drops = {}
            table.insert(drops, { tplId = itemId, count = count })
            local attachments = utils.serialize(drops)
            if attachments == nil then attachments = '' end
            local timestamp = timer.getTimestampCache()
            agent.mail.saveMailRaw(user.uid, 0, mailType, mailSubType, title, content, timestamp, true, false, attachments, '', 0)
            -- agent.sendNoticeMessage(p.ErrorCode.BAG_FULL_MAIL, '', 1)
            return true
        else
            item = bagItemInfo:create(tpl, count)
            if item and item.bid > 0 then
                bag.bag_items[item.bid] = item
                bag.bagCount = bag.bagCount + 1
                tempCount = count
            end
        end
    else
        item.count = item.count + count
    end
    item.dirty = true
    item.sync = false

    if sendUpdate then
        bag.sendBagUpdate()
    end

    if tempCount > 0 then
        logStub.appendItem(user.uid, itemId, tempCount, p.LogItemType.GAIN, gainType, timer.getTimestampCache())
    end
    return true
end

function bag.addItemByCond(itemId, count, cond, gainType, sendUpdate)
    --cond = {level=i, exp=i, ext1, ext2}
    -- print('bag.addItemByCond..itemId, count, cond', itemId, count, utils.serialize(cond))
    if not gainType then
        gainType = p.ResourceGainType.DEFAULT
    end
    if sendUpdate == nil then
        sendUpdate = true
    end

    local tpl = t.item[itemId]
    if not tpl or count < 1 then
        return false
    end

    if bag.checkCanAccumulateByCond(cond) then
        bag.addItem(itemId, count, gainType, sendUpdate)
    else
        local tempCount = 0
        for i=1, count do
            if bag.checkBagIsFull() then
                --邮件发送
                local mailType = p.MailType.SYSTEM
                local mailSubType = p.MailSubType.SYSTEM_BAG_FULL
                local title, content = agent.mail.getTitleContentBySubType(mailSubType)
                local drops = {}
                local addCount = count - i + 1
                table.insert(drops, { tplId = itemId, count = addCount, cond = cond })
                local attachments = utils.serialize(drops)
                if attachments == nil then attachments = '' end
                local timestamp = timer.getTimestampCache()
                agent.mail.saveMailRaw(user.uid, 0, mailType, mailSubType, title, content, timestamp, true, false, attachments, '', 0)
                -- agent.sendNoticeMessage(p.ErrorCode.BAG_FULL_MAIL, '', 1)
                break
            else
                local info = bagItemInfo:create(tpl, 1)
                if info and info.bid > 0 then
                    info.data.level = cond.level or 1
                    info.data.exp = cond.exp or 0
                    info.ext1 = cond.ext1 or {}
                    info.ext2 = cond.ext2 or {}
                    bag.bag_items[info.bid] = info
                    bag.bagCount = bag.bagCount + 1
                    tempCount = tempCount + 1
                end
            end
        end
        if sendUpdate then
            bag.sendBagUpdate()
        end
        if tempCount > 0 then
            logStub.appendItem(user.uid, itemId, tempCount, p.LogItemType.GAIN, gainType, timer.getTimestampCache())
        end
    end
    return true
end

function bag.removeItem(itemId, count, consumeType, sendUpdate)
    -- print('###bag.removeItem...itemId, count, consumeType', itemId, count, consumeType)
    if consumeType == nil then
        consumeType = p.ResourceConsumeType.DEFAULT
    end
    if sendUpdate == nil then
        sendUpdate = true
    end

    local tpl = t.item[itemId]
    if not tpl or count < 1 then
        utils.log(string.format('bag.removeItem tpl not exist or count < 1 : uid=%i, tplId=%i, count=%i', user.uid, tplId, count))
        return false
    end

    if tpl.type == p.ItemType.PROP and tpl.subType ~= p.ItemPropType.SKILL_BOOK then
        for _, v in pairs(bag.bag_items) do
            if v.tpl.id == itemId and v.count >= count then
                v.count = v.count - count
                if v.count <= 0 then
                    bag.bagCount = bag.bagCount - 1
                end
                v.dirty = true
                v.sync = false
                if sendUpdate then
                    bag.sendBagUpdate()
                end
                -- log
                logStub.appendItem(user.uid, itemId, count, p.LogItemType.CONSUME, consumeType, timer.getTimestampCache())
                return true
            end
        end
    else
        --TODO:消耗规则:优先品阶低的 然后是等级低的 然后才是强化低的 最后吞洗练过的 然后是宝石镶嵌过的 宝石镶嵌过的随机排序即可
        local tempList = {}
        local tempCount = 0
        for bid, v in pairs(bag.bag_items) do
            if v.tpl.id == itemId and v.count > 0 then
                -- print('bid, itemId, count, quality, level, exp', bid, itemId, v.count, v.tpl.quality, v.data.level, v.data.exp)
                table.insert(tempList, { bid = bid, itemId = itemId, count = v.count, quality = v.tpl.quality, level = v.data.level, exp = v.data.exp })
                tempCount = tempCount + v.count
            end
        end
        if tempCount >= count then
            table.sort( tempList, function(infoLeft, infoRight)
                local ret = false
                if infoLeft.quality == infoRight.quality then
                    if infoLeft.level == infoRight.level then
                        ret = infoLeft.exp < infoRight.exp
                    else
                        ret = infoLeft.level < infoRight.level
                    end
                else
                    ret = infoLeft.quality < infoRight.quality
                end
                return ret
            end)

            tempCount = count
            for _, info in ipairs(tempList) do
                if tempCount > 0 then
                    tempCount = tempCount - info.count
                    local consume = info.count
                    if tempCount < 0 then
                        consume = info.count + tempCount
                    end
                    bag.removeItemByBid(info.bid, info.itemId, consume, consumeType, false)
                    if tempCount <= 0 then
                        break
                    end
                end
            end
        end
    end

    return false
end

function bag.removeItems(list, consumeType, sendUpdate)
    -- list = {[tplId]=count,...}
    for tplId, count in pairs(list) do
        if not bag.removeItem(tplId, count, consumeType, false) then
            utils.log(string.format('bag.removeItems failed : uid=%i, tplId=%i, count=%i', user.uid, tplId, count))
        end
    end
    if sendUpdate == nil then
        sendUpdate = true
    end
    if sendUpdate then
        bag.sendBagUpdate()
    end
end

function bag.removeItemByBid(bid, itemId, count, consumeType, sendUpdate)
    print('###bag.removeItemByBid..bid, itemId, count, consumeType', bid, itemId, count, consumeType)
    if not consumeType then
        consumeType = p.ResourceConsumeType.DEFAULT
    end
    if sendUpdate == nil then
        sendUpdate = true
    end

    local tpl = t.item[itemId]
    if not tpl or count < 1 then
        utils.log(string.format('bag.removeItemByBid tpl not exist or count < 1 : uid=%i, tplId=%i, count=%i', user.uid, tplId, count))
        return false
    end

    local item = bag.bag_items[bid]
    if item
        and item.tpl.id == itemId
        and item.count >= count then
        item.count = item.count - count
        if item.count <= 0 then
            bag.bagCount = bag.bagCount - 1
        end
        item.dirty = true
        item.sync = false
        if sendUpdate then
            bag.sendBagUpdate()
        end
        -- log
        logStub.appendItem(user.uid, itemId, count, p.LogItemType.CONSUME, consumeType, timer.getTimestampCache())
        return true
    end
    return false
end

function bag.getItemBybid(bid)
    local info = bag.bag_items[bid]
    if not info then
        return nil
    else
        return { itemId = info.tpl.id, count = info.count, subType = info.tpl.subType }
    end
end

function bag.getItemsBySubType(subType)
    local itemList = {}
    for _, v in pairs(bag.bag_items) do
        if v.tpl.subType == subType then
            print("is hereis hereis here")
            table.insert(itemList, {id = v.tpl.id, count = v.count, quality = v.tpl.quality})
        end
    end
    return itemList
end

function bag.getItemCountBybid(bid)
    local info = bag.bag_items[bid]
    if not info then
        return 0
    else
        return info.count
    end
end

function bag.getItemCountById(id)
    local count = 0
    for bid, v in pairs(bag.bag_items) do
        if v.tpl.id == id and v.count > 0 then
            -- print('bid, itemId, count, quality, level, exp', bid, itemId, v.count, v.tpl.quality, v.data.level, v.data.exp)
            count = count + v.count
        end
    end
    return count
end

function bag.checkBagIsFull()
    return bag.bagCount >= bagItemLimit
end

function bag.checkBagIsFullByAdd(add)
    return bag.bagCount + add > bagItemLimit
end

function bag.checkBagCanSynthesize(subType)
    if subType ~= p.ItemPropType.SKILL_DEBRIS
        and subType ~= p.ItemPropType.EQUIP_DEBRIS
        and subType ~= p.ItemPropType.TREASURE_DEBRIS
        and subType ~= p.ItemPropType.ATTACK_GEM
        and subType ~= p.ItemPropType.DEFENSE_GEM
        and subType ~= p.ItemPropType.SPECIAL_GEM
        and subType ~= p.ItemPropType.HERO_DEBRIS then
        return false
    end
    return true
end

function bag.sendBagUpdate()
    local list = {}
    local removeList = {}
    for _, v in pairs(bag.bag_items) do
        if not v.sync then
            v.sync = true
            if v.count > 0 then
                local data = utils.serialize(v.data)
                local ext1 = utils.serialize(v.ext1)
                local ext2 = utils.serialize(v.ext2)
                --print('bag.sendBagUpdate...bag bid, itemId, count, data, ext1, ext2 = ', v.bid, v.tpl.id, v.count, data, ext1, ext2)
                table.insert(list, { bid = v.bid, tplId = v.tpl.id,  count = v.count, data = data, ext1 = ext1, ext2 = ext2 })
            else
                -- print("bag remove bid, itemId = ", v.bid, v.tpl.id)
                table.insert(removeList, { bid = v.bid })
            end
        end
    end
    local tempCount = 0
    for _, v in pairs(bag.bag_items) do
        tempCount = tempCount + 1
    end
    -- print('---------bag.sendBagUpdate.....bagCount, tempCount', bag.bagCount, tempCount)

    if next(list) or next(removeList) then
        -- print('###bag.sendBagUpdate..list=', utils.serialize(list))
        -- print('###bag.sendBagUpdate..removeList=', utils.serialize(removeList))
        agent.sendPktout(p.SC_BAG_UPDATE,  '@@1=[bid=i,tplId=i,count=i,data=s,ext1=s,ext2=s],2=[bid=i]',  list, removeList)
    end
end

function bag.cs_bag_use(bid, tplId, count, useGold, p1, session)
    local map = agent.map
    local function useResponse(result, messageCode,items)
        items = items or {}
        print("bag useResponse", result, messageCode, utils.serialize(items))
        agent.replyPktout(session, p.SC_BAG_USE_RESPONSE, '@@1=b,2=i,3=[tplId=i,count=i]', result, messageCode, items)
    end
    local function returnToUser(tplId, count, useGold)
        if useGold then
            user.addResource(p.ResourceType.GOLD, t.item[tplId].price * count)
        else
            bag.addItem(tplId, count)
        end
    end

    -- print("p.CS_BAG_USE..bid, tplId, count, useGold, p1", bid, tplId, count, useGold, p1)

    if count == 0 then
        count = 1
    end
    local tpl = t.item[tplId]
    if not tpl then
        print("p.CS_BAG_USE, tpl not found, tplId = ", tplId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    if tpl.type ~= p.ItemType.PROP then
        print("p.CS_BAG_USE, item is not prop")
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    print("tpl.subType，",tpl.subType, p.ItemPropType.TREASURE_BOX)

    local propType = tpl.subType
    local consumeType = p.ResourceConsumeType.ITEM_BUY_USE
    -- check
    if propType == p.ItemPropType.ADVANCED_TELEPORT then
        consumeType = p.ResourceConsumeType.ADVANCED_TELEPORT
    elseif propType == p.ItemPropType.HERO_EXP then
        local heroId = tonumber(p1)
        if heroId and not agent.hero.canAddHeroExp(heroId) then
            print('p.CS_BAG_USE, hero exp can not add', heroId)
            agent.sendNoticeMessage(p.ErrorCode.HERO_LEVEL_TOP, '', 1)
            return
        end
    elseif propType == p.ItemPropType.LORD_RENAME then
        if user.checkNicknameInvalid(p1) then
            print('p.CS_BAG_USE, nickname is invalid', p1)
            useResponse(false, p.ErrorCode.USER_NOT_RENAME)
            return
        end
    elseif propType == p.ItemPropType.CHANGE_APPREARANCE then
        local headId = tonumber(p1)
        if headId and not user.checkUnlockCond(headId) then
            print('p.CS_BAG_USE, head can not change', headId)
            agent.sendNoticeMessage(p.ErrorCode.USER_NOT_CHANGE_HEAD, '', 1)
            return
        end
    elseif propType == p.ItemPropType.TROOP_MARCHING_SPEEDUP then
        local troopId = tonumber(p1)
        if troopId == nil then
            print('p.CS_BAG_USE, troopId is nil', troopId)
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            return
        end
    end

    if useGold then
        --buy & use
        if not user.removeResource(p.ResourceType.GOLD, tpl.price * count, consumeType, tplId) then
            print("p.CS_BAG_USE, gold is not enough, price = ", tpl.price * count)
            useResponse(false, p.ErrorCode.PUBLIC_RESOURCE_NOT_ENOUGH)
            return
        end
    else
        --use item
        if not bag.removeItemByBid(bid, tplId, count, p.ResourceConsumeType.ITEM_USE) then
            print("p.CS_BAG_USE, remove item fail, bid, tplId, count = ", bid, tplId, count)
            useResponse(false, p.ErrorCode.PUBLIC_PROP_NOT_ENOUGH)
            return
        end
    end

    local messageCode = p.ErrorCode.PUBLIC_PROP_USE_SUCCESS
    local items = {}
    local tempItemId = 0
    local tempCount = 0
    if propType == p.ItemPropType.LORD_EXP then
        tempItemId = p.SpecialPropIdType.LORD_EXP
        tempCount = tpl.param1 * count
        user.addExp(tempCount)
    elseif propType == p.ItemPropType.HERO_EXP then
        local heroId = tonumber(p1)
        tempItemId = p.SpecialPropIdType.HERO_EXP
        tempCount = tpl.param1 * count
        agent.hero.addHeroExp(heroId, tempCount, true)
    elseif propType == p.ItemPropType.HERO_PHYSICAL_RECOVER then
        local heroId = tonumber(p1)
        tempCount = tpl.param1 * count
        agent.hero.recoveryPhysical(heroId, tempCount)
    elseif propType == p.ItemPropType.HERO_DEBRIS then
    --城内相关SPEEDUP不放这里实现, cdList实现
    elseif propType == p.ItemPropType.BUILDING_SPEEDUP then
    elseif propType == p.ItemPropType.TROOP_TRAINING_SPEEDUP then
    elseif propType == p.ItemPropType.WOUNDED_RECOVERY_SPEEDUP then
    elseif propType == p.ItemPropType.SCIENCE_RESEARCH_SPEEDUP then
    elseif propType == p.ItemPropType.TRAPS_BUILDING_SPEEDUP then
    elseif propType == p.ItemPropType.FORGE_SPEEDUP then
    elseif propType == p.ItemPropType.SPEEDUP then

    elseif propType == p.ItemPropType.VIP_EXP then
        --TODO
        tempItemId = p.SpecialPropIdType.VIP_EXP
        tempCount = tpl.param1 * count
    elseif propType == p.ItemPropType.VIP_TIME then
        --TODO
        tempItemId = tplId
        tempCount = tpl.param1 * count
    elseif propType == p.ItemPropType.WOOD then
        tempItemId = p.SpecialPropIdType.WOOD
        tempCount = tpl.param1 * count
        user.addResource(tempItemId, tempCount, p.ResourceGainType.BOX_DROP)
    elseif propType == p.ItemPropType.FOOD then
        tempItemId = p.SpecialPropIdType.FOOD
        tempCount = tpl.param1 * count
        user.addResource(tempItemId, tempCount, p.ResourceGainType.BOX_DROP)
    elseif propType == p.ItemPropType.STONE then
        tempItemId = p.SpecialPropIdType.STONE
        tempCount = tpl.param1 * count
        user.addResource(tempItemId, tempCount, p.ResourceGainType.BOX_DROP)
    elseif propType == p.ItemPropType.IRON then
        tempItemId = p.SpecialPropIdType.IRON
        tempCount = tpl.param1 * count
        user.addResource(tempItemId, tempCount, p.ResourceGainType.BOX_DROP)
    elseif propType == p.ItemPropType.SILVER then
        tempItemId = p.SpecialPropIdType.SILVER
        tempCount = tpl.param1 * count
        user.addResource(tempItemId, tempCount, p.ResourceGainType.BOX_DROP)
    elseif propType == p.ItemPropType.GOLD then
        tempItemId = p.SpecialPropIdType.GOLD
        tempCount = tpl.param1 * count
        user.addResource(tempItemId, tempCount, p.ResourceGainType.BOX_DROP)
    elseif propType == p.ItemPropType.STAMINA then
        if user.info.staminaPropUseCount >= agent.vip.tpl.staminaPropUseCount then
            --TODO 增加提示语，体力道具使用次数已达上限
            useResponse(false, p.ErrorCode.PUBLIC_PROP_NOT_ENOUGH)
            returnToUser(tplId, count, useGold)
            return
        end
        if user.info.staminaPropUseCount + count > agent.vip.tpl.staminaPropUseCount then
            local returnCount = user.info.staminaPropUseCount + count - agent.vip.tpl.staminaPropUseCount
            returnToUser(tplId,returnCount, useGold)
            count  = count - returnCount
        end    
        tempItemId = p.SpecialPropIdType.STAMINA
        tempCount = tpl.param1 * count
        user.addStamina(tempCount, p.StaminaGainType.STAMINA_WATER, true)
    elseif propType == p.ItemPropType.ENERGY then  
        tempItemId = p.SpecialPropIdType.ENERGY
        tempCount = tpl.param1 * count
        user.addEnergy(tempCount, p.EnergyGainType.ENERGY_WATER, true)
    elseif propType == p.ItemPropType.CHIP then
        --TODO
        tempItemId = p.SpecialPropIdType.CHIP
        tempCount = tpl.param1 * count
    elseif propType == p.ItemPropType.HONOR then
        --TODO
        tempItemId = p.SpecialPropIdType.HONOR
        tempCount = tpl.param1 * count
    elseif propType == p.ItemPropType.SAN then
        tempItemId = tplId
        tempCount = tpl.param1 * count
        agent.technology.addSan(tempCount)
    --buff
    elseif propType == p.ItemPropType.ANTI_SCOUT then
        buff.addCityBuff(p.BuffType.ANTI_SCOUT, tpl.param1 * count, tpl.param2, tpl.attr)
    elseif propType == p.ItemPropType.PEACE_SHIELD then
        buff.addCityBuff(p.BuffType.PEACE_SHIELD, tpl.param1 * count, tpl.param2, tpl.attr)
    elseif propType == p.ItemPropType.SMALL_UPKEEP_REDUCTION then
        buff.addCityBuff(p.BuffType.SMALL_UPKEEP_REDUCTION_PER, tpl.param1 * count, tpl.param2, tpl.attr)
    elseif propType == p.ItemPropType.GATHER_BONUS then
        buff.addCityBuff(p.BuffType.RESOURCE_CLLOECT_INCR_PER, tpl.param1 * count, tpl.param2, tpl.attr)
    elseif propType == p.ItemPropType.ATTACK_BONUS then
        buff.addCityBuff(p.BuffType.ATTACK_BONUS_PER, tpl.param1 * count, tpl.param2, tpl.attr)
    elseif propType == p.ItemPropType.DEFENSE_BONUS then
        buff.addCityBuff(p.BuffType.DEFENSE_BONUS_PER, tpl.param1 * count, tpl.param2, tpl.attr)

    elseif propType == p.ItemPropType.TROOP_MARCHING_SPEEDUP then
        local troopId = tonumber(p1)
        if troopId then
            print('SPEEDUP', troopId, tpl.param1)
            map.cast("speedUp", troopId, tpl.param1 /100)
        end
    elseif propType == p.ItemPropType.MARCH_RECALL then
        local troopId = tonumber(p1)
        if troopId then
            map.cast("recall", troopId)
        end
    --资源建筑产量提升在 BUILDING_COLLECT_BOOST 协议实现
    elseif propType == p.ItemPropType.FRAM_BOOST then
    elseif propType == p.ItemPropType.SAWMILL_BOOST then
    elseif propType == p.ItemPropType.STONE_MINE_BOOST then
    elseif propType == p.ItemPropType.IRON_MINE_BOOST then

    elseif propType == p.ItemPropType.RANDOM_TELEPORT then
        agent.queueJob(function()
            if not map.randomTeleport() then
                returnToUser(tplId, count, useGold)
            end
        end)
    elseif propType == p.ItemPropType.ADVANCED_TELEPORT then
        agent.queueJob(function()
            p1 = misc.deserialize(p1)
            if type(p1) == "table" then
                local x, y = tonumber(p1[1]), tonumber(p1[2])
                if x and y then
                    if not map.advancedTeleport(x, y) then
                        returnToUser(tplId, count, useGold)
                    end
                end
            end
        end)
    elseif propType == p.ItemPropType.LORD_RENAME then
        agent.queueJob(function()
            user.setNickname(p1, session)
        end)
        messageCode = p.ErrorCode.USER_RENAME_SUCCESS
    elseif propType == p.ItemPropType.CHANGE_APPREARANCE then
        local headId = tonumber(p1)
        if headId then
            user.setHead(headId)
            messageCode = p.ErrorCode.USER_CHANGE_HEAD_SUCCESS
        end
    elseif propType == p.ItemPropType.MARCH_RECALL then
    elseif propType == p.ItemPropType.SOLDIER_RECRUITMENT then
    elseif propType == p.ItemPropType.GOLD_ARROW then
    elseif propType == p.ItemPropType.WAR_HORN then
    elseif propType == p.ItemPropType.ADVANCED_RECALL then
    elseif propType == p.ItemPropType.WISHING_COIN then
    elseif propType == p.ItemPropType.LORD_SKILL_RESET then
    elseif propType == p.ItemPropType.ALLIANCE_QUEST_REFRESH then
    elseif propType == p.ItemPropType.MOLTEN_STONE then
    elseif propType == p.ItemPropType.HERO_DRAW_PROP then
        print('招贤道具 招贤馆使用')
    elseif propType == p.ItemPropType.GIFT then
    elseif propType == p.ItemPropType.TREASURE_BOX then
        local dropId = tpl.param1
        local propId = tpl.param2
        local propIsEnough = false;
        -- 消耗道具
        if propId > 0 then
            propIsEnough = bag.checkItemEnough(propId, count)
        end
        if not propIsEnough and propId > 0 then
            useResponse(false, p.ErrorCode.PUBLIC_PROP_NOT_ENOUGH)
            returnToUser(tplId, count, useGold)
            return
        end
        if propIsEnough and propId > 0 then
            bag.removeItem(propId, 1, consumeType)
        end
        local DropTpl = t.drop[dropId]
        if DropTpl then
            items = DropTpl:DoDrop()
            print("###USE_BAG, items", #items, utils.serialize(items))
        end
        if items ~= nil then
            bag.addItems(items, p.ResourceGainType.OPEN_TREASURE_BOX)
        end
    end
    if tempItemId > 0 and tempCount > 0 then
        table.insert(items, { tplId = tempItemId, count = tempCount })
    end

    bag.evtUse:trigger(tplId, count)
    if useGold then
        if messageCode == p.ErrorCode.PUBLIC_PROP_USE_SUCCESS then
            messageCode = 0
        end
    end

    useResponse(true, messageCode, items)
end

function bag.cs_bag_buy(stroeId, count, session)
    local function buyResponse(result)
        --print("bag buyResponse", result)
        agent.replyPktout(session, p.SC_BAG_BUY_RESPONSE, result)
    end

    if count < 1 or count > 10000 then
        return
    end
    local goods = t.item[stroeId]
    if not goods then
        buyResponse(false)
        return
    end
    if not user.removeResource(p.ResourceType.GOLD, goods.price * count, p.ResourceConsumeType.BUY) then
        print("gold is not enough, price = ", goods.price * count)
        buyResponse(false)
        return
    end
    if not bag.addItem(goods.id, count, p.ResourceGainType.BUY) then
        user.addResource(p.ResourceType.GOLD, goods.price * count)
        print("add item fail, itemId = ", goods.itemId)
        buyResponse(false)
        return
    end
    --print("bag_buy stroeId, itemId, count", stroeId, goods.itemId, count)

    buyResponse(true)
end

function bag.cs_bag_sell(bid, count, session)
    local function sellResponse(result, silver)
        print("bag sellResponse", result, silver)
        agent.replyPktout(session, p.SC_BAG_SELL_RESPONSE, result, silver)
    end
    print('p.CS_BAG_SELL...bid, count', bid, count)

    local info = bag.bag_items[bid]
    if not info or info.count <= 0 or info.count < count then
        print('###p.CS_BAG_SYNTHESIZE..bid not exist or sale is greater than the number of owned, bid, sellCount,myCount=', bid, count, info.count)
        sellResponse(p.ErrorCode.FAIL, 0)
        return
    end
    if info.tpl.sellPrice <= 0 then
        print('###p.CS_BAG_SYNTHESIZE..item sellPrice == 0, id, sellPrice=', info.tpl.id, info.tpl.sellPrice)
        sellResponse(p.ErrorCode.FAIL, 0)
        return
    end


    if bag.removeItemByBid(bid, info.tpl.id, count, p.ResourceConsumeType.BAG_SELL, false) then
        local price = info.tpl.sellPrice * count
        user.addResource(p.ResourceType.SILVER, price, p.ResourceGainType.BAG_SELL)
        bag.sendBagUpdate()
        sellResponse(p.ErrorCode.SUCCESS, price)
        return
    end

    sellResponse(p.ErrorCode.FAIL, 0)
end

function bag.cs_bag_synthesize(bid, session)
    local function synthesizeResponse(result, tplId)
        if tplId == nil then
            tplId = 0
        end
        print("synthesizeResponse..", result)
        agent.replyPktout(session, p.SC_BAG_SYNTHESIZE_RESPONSE, result, tplId)
    end
    print("p.CS_BAG_SYNTHESIZE", bid)
    local info = bag.bag_items[bid]
    if not info or info.count <= 0 then
        print('###p.CS_BAG_SYNTHESIZE..bid not exist, bid=', bid)
        synthesizeResponse(p.ErrorCode.BAG_SYNTHESIZE_FAIL)
        return
    end
    --1.检查类型 碎片才可以合成
    if not bag.checkBagCanSynthesize(info.tpl.subType) then
        print('###p.CS_BAG_SYNTHESIZE..this subType can not synthesize, bid, subType=', bid, info.tpl.subType)
        synthesizeResponse(p.ErrorCode.BAG_SYNTHESIZE_FAIL)
        return
    end

    if info.tpl.subType == p.ItemPropType.HERO_DEBRIS then
        --检查该武将是否已经存在
        local hero = agent.hero
        local heroId = info.tpl.param1
        if hero[heroId] then
            print('###p.CS_BAG_SYNTHESIZE..hero is exist...heroId=', heroId)
            synthesizeResponse(p.ErrorCode.BAG_SYNTHESIZE_FAIL)
            return
        end
        --
        if not t.heros[heroId] or not t.heros[heroId].isCanUse then
            print('###p.CS_BAG_SYNTHESIZE..synthesize id is not exist in tpl_hero...heroId=', heroId)
            synthesizeResponse(p.ErrorCode.BAG_SYNTHESIZE_FAIL)
            return
        end
        --检查武将碎片是否足够
        local needDebris = hero.getOpenHeroNeedDebris(heroId)
        print('------------------p.CS_BAG_SYNTHESIZE....needDebris, myCount, info.tpl.id, heroId', needDebris, info.count, info.tpl.id, heroId)
        if info.count < needDebris then
            print('###p.CS_BAG_SYNTHESIZE..hero debris are not enough...needDebris, count =', needDebris, info.count)
            synthesizeResponse(p.ErrorCode.HERO_DEBRIS_NOT_ENOUGH)
            return
        end
        if bag.removeItemByBid(bid, info.tpl.id, needDebris, p.ResourceConsumeType.HERO_SYNTHESIZE, false) then
            hero.addHero(heroId, p.HeroGainType.DEBRIS_SYNTHESIZE, true)

            bag.sendBagUpdate()
            synthesizeResponse(p.ErrorCode.SUCCESS, heroId)
            return
        end
    else
        --2.检查背包是否已经满了
        if bag.checkBagIsFull() then
            print('###p.CS_BAG_SYNTHESIZE..bag is full..count=', bag.bagCount)
            synthesizeResponse(p.ErrorCode.BAG_FULL_CLEAR)
            return
        end
        --3.检查消耗的道具和资源是否足够
        local tempItem = misc.deserialize(info.tpl.param4) or {}
        if not bag.checkItemsEnough(tempItem) then
            print('###p.CS_BAG_SYNTHESIZE..prop is not enough..')
            synthesizeResponse(p.ErrorCode.PUBLIC_PROP_NOT_ENOUGH)
            return
        end
        local tempResource = misc.deserialize(info.tpl.param5) or {}
        if not bag.checkResourcesEnough(tempResource) then
            print('###p.CS_BAG_SYNTHESIZE..resource is not enough..')
            synthesizeResponse(p.ErrorCode.PUBLIC_SILVER_NOT_ENOUGH)
            return
        end
        if not t.item[info.tpl.param1] then
            print('###p.CS_BAG_SYNTHESIZE..synthesize id is not exist in tpl_item...tplId=', info.tpl.param1)
            synthesizeResponse(p.ErrorCode.BAG_SYNTHESIZE_FAIL)
            return
        end
        --4.消耗
        bag.removeItems(tempItem, p.ResourceConsumeType.BAG_SYNTHESIZE, false)
        bag.removeResources(tempResource, p.ResourceConsumeType.BAG_SYNTHESIZE)
        --5.
        bag.addItem(info.tpl.param1, 1, p.ResourceGainType.BAG_SYNTHESIZE, false)

        bag.sendBagUpdate()
        synthesizeResponse(p.ErrorCode.SUCCESS, info.tpl.param1)
        return
    end

    synthesizeResponse(p.ErrorCode.BAG_SYNTHESIZE_FAIL)
end

function bag.cs_bag_melt(list, session)
    if #list <= 0 then
        print('p.CS_BAG_MELT...no melt items..size=', size)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    local itemList = {}
    for _,v in pairs(list) do
        local bid, count = v.bid, v.count
        print('p.CS_BAG_MELT...bid, count', bid, count)
        if bid > 0 and count > 0 then
            local info = bag.bag_items[bid]
            if info
                and info.tpl.type == p.ItemType.EQUIP
                and info.count >= count then
                local level = info.data.level or 0
                local silver = equipMeltBaseSilver * count
                --强化所耗费的银两
                local tempSilver = 0
                for i=1, level-1 do
                    local equipTpl = t.equipLevel[i]
                    if equipTpl then
                        tempSilver = tempSilver + equipTpl.silver
                    end
                end
                silver = silver + math.floor(tempSilver * 0.8)
                itemList[p.ResourceType.SILVER] = { tplId = p.ResourceType.SILVER, count = silver }
                --镶嵌的宝石
                for _, tplId in pairs(info.ext2) do
                    local item = itemList[tplId]
                    if item == nil then
                        itemList[tplId] = { tplId = tplId, count = 1 }
                    else
                        item.count = item.count + 1
                    end
                end
                --耗费
                bag.removeItemByBid(bid, info.tpl.id, count, p.ResourceConsumeType.BAG_MELT, false)
            end
        end
    end
    --得到
    bag.pickDropItems(itemList, p.ResourceGainType.BAG_MELT)
    print('p.CS_BAG_MELT...itemList', utils.serialize(itemList))
    agent.sendPktout(p.SC_BAG_MELT_RESPONSE, '@@1=i,2=[tplId=i,count=i]', p.ErrorCode.SUCCESS, itemList)
end

return bag
