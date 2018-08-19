-- 使用资源道具达到X个
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local useResItem = questBase:extend()

function useResItem:init(a, info)
    self.evtUse = a.bag.evtUse
    useResItem.super.init(self, a, info)
end

function useResItem:onUse(itemId, count)
     -- print('useResItem:onUse... itemId:', itemId, count)
    local tpl = t.item[itemId]
    if tpl.type == p.ItemType.PROP then
        if tpl.subType == p.ItemPropType.FOOD or tpl.subType == p.ItemPropType.WOOD or tpl.subType == p.ItemPropType.STONE or tpl.subType == p.ItemPropType.IRON then
            useResItem.super.advanceAndSendUpdate(self, count)
        end
    end
end

function useResItem:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtUse:attachSelf(self, self.onUse)
end

return useResItem