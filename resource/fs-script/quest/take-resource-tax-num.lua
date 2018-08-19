-- 征收x次x资源
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local takeResourceTaxNum = questBase:extend()

function takeResourceTaxNum:init(a, info)
    self.evtResourceTax = a.building.evtResourceTax
    takeResourceTaxNum.super.init(self, a, info)
end

function takeResourceTaxNum:resourceTax(resourceType, num)
    if resourceType == self.qInfo.tpl.param1 then
        takeResourceTaxNum.super.advanceAndSendUpdate(self, 1)
    end
end

function takeResourceTaxNum:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtResourceTax:attachSelf(self, self.resourceTax)
end

return takeResourceTaxNum