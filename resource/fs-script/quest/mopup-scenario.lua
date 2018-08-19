-- 扫荡x次x副本
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local mopupScenario = questBase:extend()

function mopupScenario:init(a, info)
    self.evtScenarioMopup = a.scenarioCopy.evtScenarioMopup
    mopupScenario.super.init(self, a, info)
end

function mopupScenario:scenarioMopup(id, count)
    if id == self.qInfo.tpl.param1 then
        mopupScenario.super.advanceAndSendUpdate(self, count)
    end
end

function mopupScenario:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtScenarioMopup:attachSelf(self, self.scenarioMopup)
end

return mopupScenario