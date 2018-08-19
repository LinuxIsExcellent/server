-- 加入一个同盟
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local allianceJoin = questBase:extend()

function allianceJoin:init(a, info)
     self.evtAllianceJoin = a.alliance.evtAllianceJoin
    allianceJoin.super.init(self, a, info)
end

function allianceJoin:onJoin(score)
    allianceJoin.super.advanceAndSendUpdate(self, 1)
end

function allianceJoin:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtAllianceJoin:attachSelf(self, self.onJoin)
end

return allianceJoin