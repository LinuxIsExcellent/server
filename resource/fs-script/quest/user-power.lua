-- 战力达到XXX
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local userPower = questBase:extend()

function userPower:init(a, info)
    self.evtTotalPowerChange = a.user.evtTotalPowerChange
    if info.progress == 0 then
        local totalPower = a.user.info.totalPower
        info.progress = totalPower
    end

    userPower.super.init(self, a, info)
end

function userPower:onTotalPowerChange(totalPower)
    if totalPower <= self.qInfo.progress then
        return
    end
    
    userPower.super.advanceAndSendUpdate(self, totalPower - self.qInfo.progress)
end

function userPower:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtTotalPowerChange:attachSelf(self, self.onTotalPowerChange)
end

return userPower