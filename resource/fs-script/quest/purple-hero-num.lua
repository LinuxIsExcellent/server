-- 拥有XX品质武将达到X个
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local purpleHeroNum = questBase:extend()

function purpleHeroNum:init(a, info)
    self.evtAddHero = a.hero.evtAddHero
    if info.progress == 0 then
        local num = a.hero.getHeroCountByQuality(info.tpl.param1)
        if num > 0 then
            info.progress = num
        end
    end

    purpleHeroNum.super.init(self, a, info)
end

function purpleHeroNum:onAddHero(id)
    local tpl = t.hero[id]
    if tpl then
        if tpl.quality ~= self.qInfo.tpl.param1 then
            return
        end
        
        purpleHeroNum.super.advanceAndSendUpdate(self, 1)
    end
end

function purpleHeroNum:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtAddHero:attachSelf(self, self.onAddHero)
end

return purpleHeroNum