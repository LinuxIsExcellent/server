-- 获得x个x级武将
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local utils = require('utils')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local heroLevelUp = questBase:extend()

function heroLevelUp:init(a, info)
	self.hero = a.hero
    self.evtHeroLevelUpOne = a.hero.evtHeroLevelUpOne

    if info.progress == 0 then
        local num = a.hero.getHeroCountByLevel(info.tpl.param1)
        if num > 0 then
            info.progress = num
        end
    end
    heroLevelUp.super.init(self, a, info)
end

function heroLevelUp:levelUp(id, level)
	if level >= self.qInfo.tpl.param1 then
		heroLevelUp.super.advanceAndSendUpdate(self, 1)
	end	
end

function heroLevelUp:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtHeroLevelUpOne:attachSelf(self, self.levelUp)
end

return heroLevelUp