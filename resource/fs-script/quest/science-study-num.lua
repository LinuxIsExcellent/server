-- 研究科技x次
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local scienceStudyNum = questBase:extend()

function scienceStudyNum:init(a, info)
    self.evtAddLevel = a.technology.evtAddLevel
    scienceStudyNum.super.init(self, a, info)
end

function scienceStudyNum:studyScience(id, level)
    -- print('scienceStudyNum:studyScience')
    scienceStudyNum.super.advanceAndSendUpdate(self, 1)
end

function scienceStudyNum:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtAddLevel:attachSelf(self, self.studyScience)
end

return scienceStudyNum