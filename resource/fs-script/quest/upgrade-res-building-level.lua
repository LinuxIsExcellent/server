-- XX个资源建筑达到XX级
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local upgradeResBuildingLevel = questBase:extend()

function upgradeResBuildingLevel:init(a, info)
    self.building = a.building
    self.evtBuildingLevelUp = a.building.evtBuildingLevelUp
    
    upgradeResBuildingLevel.super.init(self, a, info)
end

function upgradeResBuildingLevel:onUpgrade(tplId, level)
    if level - 1 >= self.qInfo.tpl.param1 or self:questInfo():isFinish() then
        return
    end

    upgradeResBuildingLevel.super.advanceAndSendUpdate(self, 1)
end

function upgradeResBuildingLevel:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtBuildingLevelUp:attachSelf(self, self.onUpgrade)
end

return upgradeResBuildingLevel