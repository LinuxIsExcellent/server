-- 建造X资源地达到X个
local p = require('protocol')
local event = require('libs/event')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local createBuilding = questBase:extend()

function createBuilding:init(a, info)
    self.building = a.building
    self.evtBuildingLevelUp = self.building.evtBuildingLevelUp
    
    self.questTpl = info.tpl
    -- init progress
    local progress = 0
    for _, v in pairs(self.building.list) do
        if v.tplId == self.questTpl.param1 and v.level >= self.questTpl.param2 then
            progress = progress + 1
        end
    end
    info.progress = progress

    createBuilding.super.init(self, a, info)
    --print('create-building:init ...')
end

function createBuilding:onUpgrade(tplId, level)
    --print('create-building:onUpgrade ... tplId:', tplId, 'level:', level)
    if self.questTpl.param1 ~= tplId or level < self.questTpl.param2 then
        return
    end
    -- get new progress
    local progress = 0
    for _, v in pairs(self.building.list) do
        if v.tplId == self.questTpl.param1 and v.level >= self.questTpl.param2 then
            progress = progress + 1
        end
    end
    if progress <= self.qInfo.progress then
        return
    end
    createBuilding.super.advanceAndSendUpdate(self, progress - self.qInfo.progress)
end

function createBuilding:onInit()
    --print('create-building:onInit...', self, self.onUpgrade)
    if self:questInfo():isFinish() then
        return
    end
    self.evtBuildingLevelUp:attachSelf(self, self.onUpgrade)
end

return createBuilding
