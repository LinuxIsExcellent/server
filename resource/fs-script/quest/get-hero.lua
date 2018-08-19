-- 招募获得武将x
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local getHero = questBase:extend()

function getHero:init(a, info)
    self.evtDrawHeroById = a.hero.evtDrawHeroById
    getHero.super.init(self, a, info)
end

function getHero:get(id)	
	if id == self.qInfo.tpl.param1 then
		getHero.super.advanceAndSendUpdate(self, 1)
	end
end

function getHero:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtDrawHeroById:attachSelf(self, self.get)
end

return getHero