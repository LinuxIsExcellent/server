-- 被侦查X次
local p = require('protocol')
local event = require('libs/event')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local beScoutTimes = questBase:extend()

function beScoutTimes:init(a, info)
    self.evtBeScout = a.map.evtBeScout
    beScoutTimes.super.init(self, a, info)
    --print('be-scout-times:init ...')
end

function beScoutTimes:onBeScout(isWin)
    --print('be-scout-times:onScout ... isWin:', isWin)
    beScoutTimes.super.advanceAndSendUpdate(self, 1)
end

function beScoutTimes:onInit()
    --print('be-scout-times:onInit...')
    if self:questInfo():isFinish() then
        return
    end
    self.evtBeScout:attachSelf(self, self.onBeScout)
end

return beScoutTimes
