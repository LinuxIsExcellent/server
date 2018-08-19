-- 建造陷阱达到X个
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local buildTrap = questBase:extend()

function buildTrap:init(a, info)
    self.evtTraining = a.army.evtTraining
    self.evtTrainFinished = a.army.evtTrainFinished
    buildTrap.super.init(self, a, info)
    --print('build-trap:init ...')
end

function buildTrap:onTraining(armyType, count)
    local tpl = t.findArmysTpl(armyType, 1)
    if tpl.type ~= p.ArmysType.ROLLING_LOGS and tpl.type ~= p.ArmysType.GRIND_STONE and tpl.type ~= p.ArmysType.CHEVAL_DE_FRISE and tpl.type ~= p.ArmysType.KEROSENE then
        return 
    end
    buildTrap.super.advanceAndSendUpdate(self, count)
end

function buildTrap:onInit()
    --print('buid-trap:onInit...')
    if self:questInfo():isFinish() then
        return
    end
    if self:type() == p.QuestType.RECOMMENDED then
        self.evtTraining:attachSelf(self, self.onTraining)
    else
        self.evtTrainFinished:attachSelf(self, self.onTraining)
    end
end

return buildTrap
