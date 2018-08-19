-- 累计获得银两XX
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local addSilver = questBase:extend()

function addSilver:init(a, info)
    self.evtResourceAdd = a.user.evtResourceAdd
    if info.progress == 0 then
        local level = a.user.info.level
        info.progress = level
    end

    addSilver.super.init(self, a, info)
end

function addSilver:onResourceAdd(type, count, gainType)
    if type ~= p.ResourceType.SILVER then
        return
    end
    
    addSilver.super.advanceAndSendUpdate(self, count)
end

function addSilver:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtResourceAdd:attachSelf(self, self.onResourceAdd)
end

return addSilver