-- 占领x（城池类型）城池
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local occupyCity = questBase:extend()

function occupyCity:init(a, info)
    self.evtOccupyCityByUnitType = a.map.evtOccupyCityByUnitType
    occupyCity.super.init(self, a, info)
end

function occupyCity:occupy(unitId)
    if self.qInfo.tpl.param1 == unitId then
	    occupyCity.super.advanceAndSendUpdate(self, 1)
    end
end

function occupyCity:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtOccupyCityByUnitType:attachSelf(self, self.occupy)
end

return occupyCity