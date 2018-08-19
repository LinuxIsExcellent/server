-- 有x个武将技能升级到x级
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local upgradeHeroSkillNum = questBase:extend()

function upgradeHeroSkillNum:init(a, info)
     self.evtSkillUpgrade = a.hero.evtSkillUpgrade
    upgradeHeroSkillNum.super.init(self, a, info)
end

function upgradeHeroSkillNum:upgrade(level)
    upgradeHeroSkillNum.super.advanceAndSendUpdate(self, 1)
end

function upgradeHeroSkillNum:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtSkillUpgrade:attachSelf(self, self.upgrade)
end

return upgradeHeroSkillNum