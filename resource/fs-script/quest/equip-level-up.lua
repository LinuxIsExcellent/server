-- XX件装备强化到XX级
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local equipLevelUp = questBase:extend()

function equipLevelUp:init(a, info)
    self.evtEquipUpgrade = a.hero.evtEquipUpgrade
    equipLevelUp.super.init(self, a, info)
    -- print('equipLevelUp:init ...')
end

function equipLevelUp:onUpgrade(heroId, level, oldLevel)
    -- print('equipLevelUp:onUpgrade...heroId,  level, oldLevel', heroId, level, oldLevel)
    if level < self.qInfo.tpl.param1 or oldLevel >= self.qInfo.tpl.param1 then
        return
    end

    equipLevelUp.super.advanceAndSendUpdate(self, 1)
end

function equipLevelUp:onInit()
    -- print('equipLevelUp:onInit...')
    if self:questInfo():isFinish() then
        return
    end
    self.evtEquipUpgrade:attachSelf(self, self.onUpgrade)
end

return equipLevelUp