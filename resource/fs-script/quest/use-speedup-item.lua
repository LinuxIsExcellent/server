-- 使用加速道具X次
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local useSpeedUpItem = questBase:extend()

function useSpeedUpItem:init(a, info)
    self.evtUse = a.bag.evtUse
    useSpeedUpItem.super.init(self, a, info)
end

function useSpeedUpItem:onUse(itemId, count)
     -- print('useSpeedUpItem:onUse... itemId:', itemId, count)
    local tpl = t.item[itemId]
    if tpl.type == p.ItemType.PROP then
        if tpl.subType == self.qInfo.tpl.param1 or tpl.subType == self.qInfo.tpl.param2 or tpl.subType == self.qInfo.tpl.param3 or tpl.subType == self.qInfo.tpl.param4 then
            useSpeedUpItem.super.advanceAndSendUpdate(self, count)
        end
    end
end

function useSpeedUpItem:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtUse:attachSelf(self, self.onUse)
end

return useSpeedUpItem