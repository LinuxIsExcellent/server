-- 侦查X目标X次
local p = require('protocol')
local event = require('libs/event')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local scoutTimes = questBase:extend()

function scoutTimes:init(a, info)
    self.evtScout = a.map.evtScout
    scoutTimes.super.init(self, a, info)
    --print('scout-times:init ...')
end

function scoutTimes:onScout(isWin, targetTplId)
    --print('scout-times:onScout ... isWin:', isWin)
    if self.qInfo.tpl.param1 > 0 then
        if targetTplId ~= self.qInfo.tpl.param1 then
            return
        end
    end

    scoutTimes.super.advanceAndSendUpdate(self, 1)
end

function scoutTimes:onInit()
    --print('scout-times:onInit...')
    if self:questInfo():isFinish() then
        return
    end
    self.evtScout:attachSelf(self, self.onScout)
end

return scoutTimes
