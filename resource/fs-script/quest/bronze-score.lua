-- 铜雀台答题达到x分
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local bronzeScore = questBase:extend()

function bronzeScore:init(a, info)
     self.evtAnswer = a.bronzeSparrowTower.evtAnswer
    bronzeScore.super.init(self, a, info)
end

function bronzeScore:onBronze(score)
	bronzeScore.super.advanceAndSendUpdate(self, score, true)
end

function bronzeScore:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtAnswer:attachSelf(self, self.onBronze)
end

return bronzeScore