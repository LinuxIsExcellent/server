-- 铜雀台中单次答对N题
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local bronzeRightNum = questBase:extend()

function bronzeRightNum:init(a, info)
	self.questTpl = info.tpl

    self.evtAnswer = a.bronzeSparrowTower.evtAnswer
    bronzeRightNum.super.init(self, a, info)
end

function bronzeRightNum:onBronze(num)
	if num >= self.questTpl.param1 then
    	bronzeRightNum.super.advanceAndSendUpdate(self, num)
    end
end

function bronzeRightNum:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtAnswer:attachSelf(self, self.onBronze)
end

return bronzeRightNum