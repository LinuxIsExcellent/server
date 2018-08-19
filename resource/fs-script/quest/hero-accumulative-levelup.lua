-- 武将累计升x级
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local heroAccumulativeLevelUp = questBase:extend()

function heroAccumulativeLevelUp:init(a, info)
    self.evtHeroLevelUpOne = a.hero.evtHeroLevelUpOne
    heroAccumulativeLevelUp.super.init(self, a, info)
end

function heroAccumulativeLevelUp:levelUp(id, level)	
	heroAccumulativeLevelUp.super.advanceAndSendUpdate(self, 1)
end

function heroAccumulativeLevelUp:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtHeroLevelUpOne:attachSelf(self, self.levelUp)
end

return heroAccumulativeLevelUp