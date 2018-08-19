-- 招募获得x个武将
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local getHeroTypeNum = questBase:extend()

function getHeroTypeNum:init(a, info)
	self.questTpl = info.tpl

    self.evtDrawHeroByType = a.hero.evtDrawHeroByType
    getHeroTypeNum.super.init(self, a, info)
end

function getHeroTypeNum:get(drawType, count)
	if self.questTpl.param1 == drawType then
		getHeroTypeNum.super.advanceAndSendUpdate(self, count)
	end
end

function getHeroTypeNum:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtDrawHeroByType:attachSelf(self, self.get)
end

return getHeroTypeNum