-- 在x(特定商城)购买x次道具
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local shopBuyItem = questBase:extend()

function shopBuyItem:init(a, info)
     self.evtBuyItem = a.store.evtBuyItem
    shopBuyItem.super.init(self, a, info)
end

function shopBuyItem:buyItem(type, count)
    -- print('shopBuyItem:alliaceHelp, type, count', type, count)
    if self.qInfo.tpl.param1 == type then
	    shopBuyItem.super.advanceAndSendUpdate(self, count)
    end
end

function shopBuyItem:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtBuyItem:attachSelf(self, self.buyItem)
end

return shopBuyItem