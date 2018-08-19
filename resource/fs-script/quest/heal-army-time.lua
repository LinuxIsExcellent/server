-- 治疗伤兵伤兵次数到达XX次
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local healArmyTime = questBase:extend()

function healArmyTime:init(a, info)
    self.evtBuildingHeal = a.building.evtBuildingHeal
    healArmyTime.super.init(self, a, info)
end

function healArmyTime:HealArmy(num)	
    healArmyTime.super.advanceAndSendUpdate(self, 1)
end

function healArmyTime:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtBuildingHeal:attachSelf(self, self.HealArmy)
end

return healArmyTime