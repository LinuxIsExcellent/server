-- 征收x个x资源
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local takeResourceTax = questBase:extend()

function takeResourceTax:init(a, info)
    self.evtResourceTax = a.building.evtResourceTax
    takeResourceTax.super.init(self, a, info)
end

function takeResourceTax:resourceTax(resourceType, num)
    if resourceType == self.qInfo.tpl.param1 then
        takeResourceTax.super.advanceAndSendUpdate(self, num)
    end
end

function takeResourceTax:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtResourceTax:attachSelf(self, self.resourceTax)
end

return takeResourceTax