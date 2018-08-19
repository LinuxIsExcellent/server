-- 拥有武将个数达到XX个
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local heroNum = questBase:extend()

function heroNum:init(a, info)
    self.evtAddHero = a.hero.evtAddHero
    if info.progress == 0 then
        local num = a.hero.getHeroCountByQuality(nil)
        if num > 0 then
            info.progress = num
        end
    end

    heroNum.super.init(self, a, info)
end

function heroNum:onAddHero(id)	
    heroNum.super.advanceAndSendUpdate(self, 1)
end

function heroNum:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtAddHero:attachSelf(self, self.onAddHero)
end

return heroNum