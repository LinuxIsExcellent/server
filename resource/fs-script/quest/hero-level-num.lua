-- 拥有？个达到X级的武将
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local heroLevelNum = questBase:extend()

function heroLevelNum:init(a, info)
    self.hero = a.hero
    self.evtHeroLevelUp = a.hero.evtHeroLevelUp
    if info.progress == 0 then
        local num = a.hero.getHeroCountByLevel(info.tpl.param1)
        if num > 0 then
            info.progress = num
        end
    end

    heroLevelNum.super.init(self, a, info)
end

function heroLevelNum:onHeroLevelUp(id, oldLevel)
    -- print('heroLevelNum:onHeroLevelUp',id, oldLevel)
    local heroInfo = self.hero.info[id]
    if heroInfo then
        if heroInfo.level < self.qInfo.tpl.param1 or oldLevel >= self.qInfo.tpl.param1 then
            return
        end
        heroLevelNum.super.advanceAndSendUpdate(self, 1)
    end
end

function heroLevelNum:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtHeroLevelUp:attachSelf(self, self.onHeroLevelUp)
end

return heroLevelNum