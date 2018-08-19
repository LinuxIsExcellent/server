-- 通关指定关卡次数
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local passSpecifyScenarioNum = questBase:extend()

function passSpecifyScenarioNum:init(a, info)
    self.evtFightScenarioCopy = a.scenarioCopy.evtFightScenarioCopy
    passSpecifyScenarioNum.super.init(self, a, info)
    -- print('passSpecifyScenarioNum:init ...')
end

function passSpecifyScenarioNum:onFight(tpl, isWin, count)
    -- print('passSpecifyScenarioNum:onFight ... tplId:', tpl.id, 'isWin:', isWin)
    if tpl == nil or tpl.id ~= self.qInfo.tpl.param1 or not isWin then
        return
    end
        
    passSpecifyScenarioNum.super.advanceAndSendUpdate(self, count)
end

function passSpecifyScenarioNum:onInit()
    -- print('passSpecifyScenarioNum:onInit...')
    if self:questInfo():isFinish() then
        return
    end
    self.evtFightScenarioCopy:attachSelf(self, self.onFight)
end

return passSpecifyScenarioNum