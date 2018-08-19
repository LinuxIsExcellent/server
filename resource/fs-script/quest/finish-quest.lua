-- 完成特定任务
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local finishQuest = questBase:extend()

function finishQuest:init(a, info)
    self.evtFinishQuest = a.quest.evtFinishQuest
    finishQuest.super.init(self, a, info)
end

function finishQuest:finishQuest(id)
	-- print("###finishQuest:finishQuest, id", id)
    -- 写成通用的
    local param1 = self.qInfo.tpl.param1
    local param2 = self.qInfo.tpl.param2
    local param3 = self.qInfo.tpl.param3
    local param4 = self.qInfo.tpl.param4
    local param5 = self.qInfo.tpl.param5
    local param6 = self.qInfo.tpl.param6
    if (param1 ~= 0 and id == param1) or 
        (param2 ~= 0 and id == param2) or 
        (param3 ~= 0 and id == param3) or 
        (param4 ~= 0 and id == param4) or 
        (param5 ~= 0 and id == param5) or 
        (param6 ~= 0 and id == param6) then
        finishQuest.super.advanceAndSendUpdate(self, 1)
    end
end

function finishQuest:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtFinishQuest:attachSelf(self, self.finishQuest)
end

return finishQuest