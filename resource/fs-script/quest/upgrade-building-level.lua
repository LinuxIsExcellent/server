-- XX建筑达到X级
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local upgradeBuildingLevel = questBase:extend()

function upgradeBuildingLevel:init(a, info)
    self.building = a.building
    self.evtBuildingLevelUp = a.building.evtBuildingLevelUp
    
    self.questTpl = info.tpl
    local maxLevel = 0
    -- init progress
    for _, v in pairs(self.building.list) do
        if v.tplId == self.questTpl.param1  and maxLevel < v.level then
             maxLevel = v.level
        end
    end
    info.progress = maxLevel
    upgradeBuildingLevel.super.init(self, a, info)
end

function upgradeBuildingLevel:onUpgrade(tplId, level)
    if self.questTpl.param1 ~= tplId or self.qInfo.progress > level then
        return
    end

    upgradeBuildingLevel.super.advanceAndSendUpdate(self, level - self.qInfo.progress)
end

function upgradeBuildingLevel:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtBuildingLevelUp:attachSelf(self, self.onUpgrade)
end

return upgradeBuildingLevel