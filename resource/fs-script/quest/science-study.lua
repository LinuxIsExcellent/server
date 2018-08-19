-- 研究x科技
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local scienceStudy = questBase:extend()

function scienceStudy:init(a, info)
     self.evtAddLevel = a.technology.evtAddLevel
    scienceStudy.super.init(self, a, info)
end

function scienceStudy:studyScience(id, level)
    if self.qInfo.tpl.param1 == id then
    	scienceStudy.super.advanceAndSendUpdate(self, level, true)
    end
end

function scienceStudy:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtAddLevel:attachSelf(self, self.studyScience)
end

return scienceStudy