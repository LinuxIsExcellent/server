-- 治疗伤兵个数达到XX个
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local healArmyNum = questBase:extend()

function healArmyNum:init(a, info)
    self.evtBuildingHeal = a.building.evtBuildingHeal
    healArmyNum.super.init(self, a, info)
end

function healArmyNum:HealArmy(num)	
    healArmyNum.super.advanceAndSendUpdate(self, num)
end

function healArmyNum:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtBuildingHeal:attachSelf(self, self.HealArmy)
end

return healArmyNum