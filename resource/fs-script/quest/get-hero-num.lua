-- 招募获得x个武将
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local getHeroNum = questBase:extend()

function getHeroNum:init(a, info)
    self.evtDrawHeroById = a.hero.evtDrawHeroById
    getHeroNum.super.init(self, a, info)
end

function getHeroNum:get(id)	
	getHeroNum.super.advanceAndSendUpdate(self, 1)
end

function getHeroNum:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtDrawHeroById:attachSelf(self, self.get)
end

return getHeroNum