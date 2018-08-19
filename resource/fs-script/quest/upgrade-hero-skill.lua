-- 有x个武将技能升级到x级
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local upgradeHeroSkill = questBase:extend()

function upgradeHeroSkill:init(a, info)
     self.evtSkillUpgrade = a.hero.evtSkillUpgrade
    upgradeHeroSkill.super.init(self, a, info)
end

function upgradeHeroSkill:upgrade(level)
    if level >= self.qInfo.tpl.param1 then
        upgradeHeroSkill.super.advanceAndSendUpdate(self, 1)
    end
end

function upgradeHeroSkill:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtSkillUpgrade:attachSelf(self, self.upgrade)
end

return upgradeHeroSkill