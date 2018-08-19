-- 通关XX关卡
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local passSpecifyScenario = questBase:extend()

function passSpecifyScenario:init(a, info)
    self.evtFightScenarioCopy = a.scenarioCopy.evtFightScenarioCopy
    passSpecifyScenario.super.init(self, a, info)
    -- print('passSpecifyScenario:init ...')
end

function passSpecifyScenario:onFight(tpl, isWin, count)
    -- print('passSpecifyScenario:onFight... tplId:', tpl.id, 'isWin:', isWin)
    if tpl == nil or tpl.id ~= self.qInfo.tpl.param1 or not isWin then
        return
    end

    passSpecifyScenario.super.advanceAndSendUpdate(self, count)
end

function passSpecifyScenario:onInit()
    -- print('passSpecifyScenario:onInit...')
    if self:questInfo():isFinish() then
        return
    end
    self.evtFightScenarioCopy:attachSelf(self, self.onFight)
end

return passSpecifyScenario