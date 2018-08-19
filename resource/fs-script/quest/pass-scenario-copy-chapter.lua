-- 通过x关卡并获得x数量的星星
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local passScenarioCopyChapter = questBase:extend()

function passScenarioCopyChapter:init(a, info)
    self.evtPassScenarioStarRefresh = a.scenarioCopy.evtPassScenarioStarRefresh
    passScenarioCopyChapter.super.init(self, a, info)
end

function passScenarioCopyChapter:passCopy(chapterId, star)
    -- print("###passScenarioCopyChapter:passCopy, chapterId, star", chapterId, star)
    if chapterId == self.qInfo.tpl.param1 then
        passScenarioCopyChapter.super.advanceAndSendUpdate(self, star, true)
    end
end

function passScenarioCopyChapter:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtPassScenarioStarRefresh:attachSelf(self, self.passCopy)
end

return passScenarioCopyChapter