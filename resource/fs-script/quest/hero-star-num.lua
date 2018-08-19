-- 拥有X星武将达到X个
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local heroStarNum = questBase:extend()

function heroStarNum:init(a, info)
    self.hero = a.hero
    self.evtHeroStarUp = a.hero.evtHeroStarUp

    if info.progress == 0 then
        local num = a.hero.getHeroCountByStar(info.tpl.param1)
        if num > 0 then
            info.progress = num
        end
    end

    heroStarNum.super.init(self, a, info)
end

function heroStarNum:onHeroStarUp(id, star)
    if star >= self.qInfo.tpl.param1 then
        heroStarNum.super.advanceAndSendUpdate(self, 1)
    end
end

function heroStarNum:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtHeroStarUp:attachSelf(self, self.onHeroStarUp)
end

return heroStarNum