local agent = ...
local p = require('protocol')
local t = require('tploader')
local dbo = require('dbo')
local event = require('libs/event')
local timer = require('timer')
local utils = require('utils')
local gmsStub = require('stub/gms')
local dataStub = require('stub/data')
local pubMail = require('mail')
local user = agent.user
local dict = agent.dict
local cdlist = agent.cdlist
local bag = agent.bag

local monthDays = 30
local daySeconds = 24 * 60 * 60
local firstChargeDropId = 2758003

local charge = {
    -- buy marks
    buyList = {},

    -- first charge
    isCharged = false,

    -- month card
    monthCardLastDrawTime = timer.getTimestampCache() - daySeconds,
    monthCardExpireTime = timer.getTimestampCache() - daySeconds,

    checkTimer = nil,
    -- events
    evtCharge = event.new(), -- (tpl)
}

local buyInfo = {
    id = '',
    openTime = 0,
    closeTime = 0,

    leftSeconds = 0,
    buyCount = 0,
    lastResetTime = 0,
    sync = false,
}
function buyInfo:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end


function charge.onInit()
    charge.dbLoad()

    for _, v in pairs(gmsStub.chargeTpls) do
        if not charge.buyList[v.id] then
            charge.buyList[v.id] = buyInfo:new({ id = v.id, openTime = v.openTime, closeTime = v.closeTime })
        end
    end
    charge.CheckChargeBuyList(false)

    -- evt
    cdlist.evtHour0Refresh:attachRaw(charge.onHour0Refresh)
end

function charge.onAllCompInit()
    charge.invokeCharge()

    charge.sendChargeOptions()
    charge.sendChargeBuyUpdate()
    charge.sendMonthCardUpdate()
end

function charge.onClose()
    charge.dbSave()
end

function charge.onSave()
    charge.dbSave()
end

function charge.getCrossTeleportInfo()
    -- db select
    local infos = {}
    local db = dbo.open(0)
    local rs = db:executePrepare('SELECT * FROM s_charge where uid = ?', user.uid)
    if rs.ok then
        for _, row in ipairs(rs) do
            table.insert(infos, row)
        end
    end
    return infos
end

function charge.onTimerUpdate(timerIndex)
    charge.CheckChargeBuyList(true)
end

function charge.dbLoad()
    local data = dict.get('charge.data') or {}
    if data.buyList then
        for _, v in pairs(data.buyList) do
            charge.buyList[v.id] = buyInfo:new(v)
        end
    end
    if data.isCharged ~= nil then
        charge.isCharged = data.isCharged
    end
    if data.monthCardLastDrawTime ~= nil then
        charge.monthCardLastDrawTime = data.monthCardLastDrawTime
    end
    if data.monthCardExpireTime ~= nil then
        charge.monthCardExpireTime = data.monthCardExpireTime
    end
end

function charge.dbSave()
    local buyList = {}
    for _, v in pairs(charge.buyList) do
        if gmsStub.chargeTpls[v.id] then
            table.insert(buyList, { id = v.id, openTime = v.openTime, closeTime = v.closeTime, buyCount = v.buyCount, lastResetTime = v.lastResetTime})
        end
    end
    dict.set('charge.data', {
        buyList = buyList,
        isCharged = charge.isCharged,
        monthCardLastDrawTime = charge.monthCardLastDrawTime,
        monthCardExpireTime = charge.monthCardExpireTime,
    })
end


function charge.invokeCharge()
    local db = dbo.open(0)
    local rs = db:executePrepare('SELECT * FROM s_charge where uid = ? and draw = 0', user.uid)
    if rs.ok then
        for k, row in ipairs(rs) do
            charge.processCharge(row)
        end
    end
end

function charge.loadCharge(orderId)
    agent.queueJob(function()
        local db = dbo.open(0)
        local rs = db:executePrepare('SELECT * FROM s_charge where orderId = ?', orderId)
        if rs.ok then
            for k, row in ipairs(rs) do
                if not row.draw and row.uid == user.uid then
                    charge.processCharge(row)
                end
            end
        end
    end)
end

function charge.processCharge(order)
    if order.draw then
        --print('charge.processCharge order.draw', order.draw)
        return
    end
    local tpl = gmsStub.chargeTpls[order.chargeId]
    if not tpl then
        --print('charge.processCharge tpl not found, order.chargeId=', order.chargeId)
        return
    end
    local info = charge.buyList[tpl.id]

    -- set draw state to 1
    local db = dbo.open(0)
    local now = timer.getTimestampCache()
    local rs = db:executePrepare('update s_charge set draw = 1, drawTime = ? where orderId = ?', now, order.orderId)
    if rs.ok and rs.affectedRows > 0 then
        -- fisrt charge
        if not charge.isCharged then
            charge.isCharged = true
            local dropTpl = t.drop[firstChargeDropId]
            if dropTpl then
                local dropList = dropTpl:DoDrop()
                bag.pickDropItems(dropList, p.ResourceGainType.CHARGE_REWARD)
                -- send confirm mail
                charge.sendFirstChargeMail(dropList)
            else
                utils.log(string.format('firstChargeDropId not exist, uid=%s, dropId=%s', tostring(user.uid), tostring(firstChargeDropId)))
            end
        end
        -- month card
        if tpl.type == 3 then
            -- goldCharged
            user.info.goldCharged = user.info.goldCharged + tpl.moneyToGold
            -- add days
            if charge.monthCardExpireTime > charge.monthCardLastDrawTime then
                charge.monthCardExpireTime = charge.monthCardExpireTime + monthDays * daySeconds
            else
                charge.monthCardLastDrawTime = timer.getTimestampCache() - daySeconds
                charge.monthCardExpireTime = charge.monthCardLastDrawTime + monthDays * daySeconds
            end
            info.buyCount = info.buyCount + 1
            info.sync = false
            -- send confirm mail
            charge.sendMonthCardMail(tpl.gold, tpl.extraItems)
            agent.sendPktout(p.SC_CHARGE_RESPONSE, tpl.id, true)
            -- trigger event
            charge.evtCharge:trigger(tpl)
            -- sync to client
            charge.sendMonthCardUpdate()
        else
            local gold, extraGold, extraItems = 0, 0, {}
            if tpl.type == 1 then
                gold = tpl.gold
                -- buy limit
                if tpl.limitCount == 0 or info.buyCount < tpl.limitCount then
                    extraGold = tpl.extraGold
                end
            elseif tpl.type == 2 then
                -- buy limit
                if tpl.limitCount > 0 and  info.buyCount >= tpl.limitCount then
                    -- ignore it ???
                    utils.log(string.format('ignore charge(buyCount >= limitCount), uid=%s orderId=%s', tostring(user.uid), order.orderId))
                    agent.sendPktout(p.SC_CHARGE_RESPONSE, tpl.id, false)
                else
                    gold = tpl.gold
                    -- buy limit
                    if tpl.limitCount == 0 or info.buyCount < tpl.limitCount then
                        extraGold = tpl.extraGold
                    end
                    if #tpl.extraItems > 0 then
                        extraItems = tpl.extraItems
                    end
                end
            end
            if gold > 0 then
                -- goldCharged
                user.info.goldCharged = user.info.goldCharged + tpl.moneyToGold
                -- add resource
                user.addResource(p.ResourceType.GOLD, gold, p.ResourceGainType.CHARGE)
                if extraGold > 0 then
                    user.addResource(p.ResourceType.GOLD, extraGold, p.ResourceGainType.CHARGE_REWARD)
                end
                if #extraItems > 0 then
                    bag.pickDropItems(extraItems, p.ResourceGainType.CHARGE_REWARD)
                end
                info.buyCount = info.buyCount + 1
                info.sync = false
                -- send confirm mail
                charge.sendChargeMail(gold, extraGold, extraItems)
                agent.sendPktout(p.SC_CHARGE_RESPONSE, tpl.id, true)
                -- trigger event
                charge.evtCharge:trigger(tpl)
            end
        end

        -- sendupdate
        charge.sendChargeBuyUpdate()
        user.sendUpdate()
        -- save all data
        agent.onSave() 
    end
end

function charge.sendFirstChargeMail(dropList)
    local mail = pubMail.newMailInfo()
    mail.id = utils.createItemId()
    mail.uid = user.uid
    mail.type = p.MailType.SYSTEM
    mail.subType = p.MailSubType.FIRST_CHARGE_CONFIRM
    local title, content = pubMail.getTitleContentBySubType(mail.subType)
    mail.title = title
    mail.content = content
    mail.createTime = timer.getTimestampCache()
    mail.params = utils.serialize({})
    mail.isDraw = true
    mail.attachment = utils.serialize(dropList)
    -- create and notify
    dataStub.appendMailInfo(mail)
end

function charge.sendMonthCardMail(gold, extraItems)
    local mail = pubMail.newMailInfo()
    mail.id = utils.createItemId()
    mail.uid = user.uid
    mail.type = p.MailType.SYSTEM
    mail.subType = p.MailSubType.MONTH_CARD_CONFIRM
    local title, content = pubMail.getTitleContentBySubType(mail.subType)
    mail.title = title
    mail.content = content
    mail.createTime = timer.getTimestampCache()
    mail.params = utils.serialize({ gold = gold })
    mail.isDraw = true
    mail.attachment = utils.serialize(extraItems)
    -- create and notify
    dataStub.appendMailInfo(mail)
end

function charge.sendChargeMail(gold, extraGold, extraItems)
    local mail = pubMail.newMailInfo()
    mail.id = utils.createItemId()
    mail.uid = user.uid
    mail.type = p.MailType.SYSTEM
    mail.subType = p.MailSubType.CHARGE_CONFIRM
    local title, content = pubMail.getTitleContentBySubType(mail.subType)
    mail.title = title
    mail.content = content
    mail.createTime = timer.getTimestampCache()
    mail.params = utils.serialize({ gold = gold, extraGold = extraGold })
    mail.isDraw = true
    mail.attachment = utils.serialize(extraItems)
    -- create and notify
    dataStub.appendMailInfo(mail)
end


function charge.CheckChargeBuyList(sendUpdate)
    local dirty = false
    local now = timer.getTimestampCache()
    for _, tpl in pairs(gmsStub.chargeTpls) do
        local info = charge.buyList[tpl.id]
        if info then
            -- tpl time changed ?  reset info
            if info.openTime ~= tpl.openTime or info.closeTime ~= tpl.closeTime then
                info.openTime = tpl.openTime
                info.closeTime = tpl.closeTime
                info.buyCount = 0
                info.sync = false
                dirty = true
            end
            if info.lastResetTime == 0 then
                info.lastResetTime = tpl.openTime
            end
            if tpl.active and tpl.limitDay > 0 and tpl.limitCount > 0 then
                local shouldReset = false
                local total = now - info.lastResetTime
                local unit = daySeconds * tpl.limitDay
                local passSeconds = total % unit
                if passSeconds == 0 then
                    shouldReset = true
                    info.lastResetTime = now
                end
                if info.lastResetTime > 0 and now - info.lastResetTime > unit then
                    shouldReset = true
                    info.lastResetTime = info.lastResetTime + unit * math.floor((now - info.lastResetTime) / unit)
                end
                if shouldReset then
                    info.buyCount = 0
                    info.sync = false
                    dirty = true
                end
            end
        end
    end
    if dirty and sendUpdate then
        charge.sendChargeBuyUpdate()
    end
end


function charge.sendChargeOptions()
    local tpls = {}
    local now = timer.getTimestampCache()
    local gotNewTpl = false
    for _, v in pairs(gmsStub.chargeTpls) do
        -- check if have new charge tpls
        if not charge.buyList[v.id] then
            gotNewTpl = true
            charge.buyList[v.id] = buyInfo:new({ id = v.id, openTime = v.openTime, closeTime = v.closeTime })
        end
        v.expireLeftSeconds = v.closeTime - now
        if v.expireLeftSeconds < 0 then
            v.expireLeftSeconds = 0
        end
        table.insert(tpls, v)
        --print('sendChargeOptions ', v.id, v.remark)
    end
    agent.sendPktout(p.SC_CHARGE_OPTIONS,  '@@1=[id=s,type=i,name=s,icon=s,inset=s,uiType=i,priority=i,moneyToGold=i,gold=i,discount=f,giftId=i,giftCount=i,isHot=b,expireLeftSeconds=i,minCastleLevel=i,maxCastleLevel=i,minChargeGold=i,maxChargeGold=i,limitDay=i,limitCount=i,extraGold=i,extraItems=[tplId=i,count=i]]',  tpls)

    if gotNewTpl then
        charge.CheckChargeBuyList(true)
    end
end

function charge.sendChargeBuyUpdate()
    --print('sendChargeBuyUpdate isCharged', charge.isCharged)
    local buyList = {}
    for _, v in pairs(charge.buyList) do
        if not v.sync then
            v.sync = true
            local tpl = gmsStub.chargeTpls[v.id]
            if tpl then
                local now = timer.getTimestampCache()
                if now > tpl.closeTime then
                    now = tpl.closeTime
                end
                local total = now - v.lastResetTime
                if tpl.limitDay > 0 and tpl.limitCount > 0 and total > 0 then
                    local unit = daySeconds * tpl.limitDay
                    local passSeconds = total % unit
                    v.leftSeconds = unit - passSeconds
                else
                    v.leftSeconds = 0
                end
                table.insert(buyList, { id = v.id, leftSeconds = v.leftSeconds, buyCount = v.buyCount})
                --print('sendChargeBuyUpdate ', v.id, v.leftSeconds, v.buyCount)
            end
        end
    end
    agent.sendPktout(p.SC_CHARGE_BUY_UPDATE,  '@@1=b,2=[id=s,leftSeconds=i,buyCount=i]',  charge.isCharged, buyList)
end

function charge.sendMonthCardUpdate()
    local leftDays = math.floor((charge.monthCardExpireTime - charge.monthCardLastDrawTime) / daySeconds)
    local isTodayDrawed = os.date('%Y-%m-%d') == os.date('%Y-%m-%d', charge.monthCardLastDrawTime)
    local leftSeconds = daySeconds - (timer.getTimestampCache() + 28800) % daySeconds
    --print('sendMonthCardUpdate leftDays, isTodayDrawed, leftSeconds=', leftDays, isTodayDrawed, leftSeconds)
    agent.sendPktout(p.SC_MONTH_CARD_UPDATE,  '@@1=i,2=b,3=i',  leftDays, isTodayDrawed, leftSeconds)
end

function charge.onHour0Refresh()
    -- print('charge.onHour0Refresh')
    charge.sendMonthCardUpdate()
end

function charge.cs_month_card_draw(session)
    local function drawMonthCardResponse(result)
        --print("charge drawMonthCardResponse", result)
        agent.replyPktout(session, p.SC_MONTH_CARD_DRAW_RESPONSE, result)
    end
    local leftDays = math.floor((charge.monthCardExpireTime - charge.monthCardLastDrawTime) / daySeconds)
    local isTodayDrawed = os.date('%Y-%m-%d') == os.date('%Y-%m-%d', charge.monthCardLastDrawTime)
    if leftDays < 1 then
        --print("leftDays = 0")
        drawMonthCardResponse(false)
        return
    end
    if isTodayDrawed then
        --print("isTodayDrawed = true")
        drawMonthCardResponse(false)
        return
    end
    local tpl = nil
    for _, v in pairs(gmsStub.chargeTpls) do
        if v.type == 3 then
            tpl = v
            break
        end
    end
    if not tpl then
        --print("month card tpl = nil")
        drawMonthCardResponse(false)
        return
    end

    -- update monthCardLastDrawTime
    -- TODO 假如lastDrawTime是today的后一天就会导致死循环 但正常情况下不会出现 
    local today = os.date('%Y-%m-%d')
    repeat
        charge.monthCardLastDrawTime = charge.monthCardLastDrawTime + daySeconds
    until today == os.date('%Y-%m-%d', charge.monthCardLastDrawTime)

    -- draw reward
    user.addResource(p.ResourceType.GOLD, tpl.gold, p.ResourceGainType.MONTH_CARD_DRAW)
    if #tpl.extraItems > 0 then
        bag.pickDropItems(tpl.extraItems, p.ResourceGainType.MONTH_CARD_DRAW)
    end
    charge.sendMonthCardUpdate()
    drawMonthCardResponse(true)
end

return charge
