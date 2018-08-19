-- 主公升至N级
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local userUpgrade = questBase:extend()

function userUpgrade:init(a, info)
    self.evtLevelUp = a.user.evtLevelUp
    if info.progress == 0 then
        local level = a.user.info.level
        info.progress = level
    end

    userUpgrade.super.init(self, a, info)
end

function userUpgrade:onLevelUp(level)
    if level <= self.qInfo.progress then
        return
    end
    
    userUpgrade.super.advanceAndSendUpdate(self, level - self.qInfo.progress)
end

function userUpgrade:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtLevelUp:attachSelf(self, self.onLevelUp)
end

return userUpgrade