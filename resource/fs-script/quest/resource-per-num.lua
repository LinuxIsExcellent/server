-- X资源的基础产量达到N/小时
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local resourcePerNum = questBase:extend()

function resourcePerNum:init(a, info)
	self.questTpl = info.tpl

    local flag = false
    if info.tpl.param1 == 15 and a.property.list.foodOutPut >= info.tpl.param2 then
        flag = true
    elseif info.tpl.param1 == 17 and a.property.list.woodOutPut >= info.tpl.param2 then
        flag = true
    elseif info.tpl.param1 == 19 and a.property.list.stoneOutPut >= info.tpl.param2 then
        flag = true
    elseif info.tpl.param1 == 21 and a.property.list.ironOutPut >= info.tpl.param2 then
        flag = true
    end

    if flag then
        info.progress = 1
    end

    self.evtResourceChange = a.property.evtResourceChange
    resourcePerNum.super.init(self, a, info)
end

function resourcePerNum:change(resourceType, num)
    if resourceType == self.qInfo.tpl.param1 and num >= self.qInfo.tpl.param2 then
    	--print("--- resourceType --- ", resourceType, " --- num --- ", num)
    	--print("--- type --- ", self.qInfo.tpl.param1, " --- count --- ", self.qInfo.tpl.param2)
        resourcePerNum.super.advanceAndSendUpdate(self, 1)
    end
end

function resourcePerNum:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtResourceChange:attachSelf(self, self.change)
end

return resourcePerNum