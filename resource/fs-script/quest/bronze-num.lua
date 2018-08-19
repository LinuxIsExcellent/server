--铜雀台中答题N次
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local bronzeNum = questBase:extend()

function bronzeNum:init(a, info)
     self.evtAnswer = a.bronzeSparrowTower.evtAnswer
    bronzeNum.super.init(self, a, info)
end

function bronzeNum:onBronze(num)
	--print("bronze --------- num ----------- ")
    bronzeNum.super.advanceAndSendUpdate(self, 1)
end

function bronzeNum:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtAnswer:attachSelf(self, self.onBronze)
end

return bronzeNum