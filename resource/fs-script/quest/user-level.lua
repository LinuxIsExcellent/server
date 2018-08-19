-- 城主等级达到XXX
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local userLevel = questBase:extend()

function userLevel:init(a, info)
    self.evtLevelUp = a.user.evtLevelUp
    if info.progress == 0 then
        local level = a.user.info.level
        info.progress = level
    end

    userLevel.super.init(self, a, info)
end

function userLevel:onLevelUp(level)
    if level <= self.qInfo.progress then
        return
    end
    
    userLevel.super.advanceAndSendUpdate(self, level - self.qInfo.progress)
end

function userLevel:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtLevelUp:attachSelf(self, self.onLevelUp)
end

return userLevel