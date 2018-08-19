-- 通关XX章节
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local passSpecifyScenarioChapter = questBase:extend()

function passSpecifyScenarioChapter:init(a, info)
    self.evtPassScenarioCopyChapter = a.scenarioCopy.evtPassScenarioCopyChapter
    passSpecifyScenarioChapter.super.init(self, a, info)
    -- print('passSpecifyScenarioChapter:init ...')
end

function passSpecifyScenarioChapter:onPass(tpl)
    -- print('passSpecifyScenarioChapter:onPass... tplId:', tpl.id)
    if tpl == nil or tpl.id ~= self.qInfo.tpl.param1 then
        return
    end

    passSpecifyScenarioChapter.super.advanceAndSendUpdate(self, 1)
end

function passSpecifyScenarioChapter:onInit()
    -- print('passSpecifyScenarioChapter:onInit...')
    if self:questInfo():isFinish() then
        return
    end
    self.evtPassScenarioCopyChapter:attachSelf(self, self.onPass)
end

return passSpecifyScenarioChapter