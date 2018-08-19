-- 训练X兵种达到X个
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local trainArmyNum = questBase:extend()

function trainArmyNum:init(a, info)
    self.questTpl = info.tpl
    
    self.evtBuildingTrain = a.building.evtBuildingTrain
    trainArmyNum.super.init(self, a, info)
end

function trainArmyNum:onTraining(armyType, count)
    -- print("armyType --------  ", armyType, " ---- count --------- ", count)
    if self.questTpl.param1 ~= armyType then
        return 
    end

    trainArmyNum.super.advanceAndSendUpdate(self, count)
end

function trainArmyNum:onInit()
    --print('train-archer:onInit...')
    if self:questInfo():isFinish() then
        return
    end
    self.evtBuildingTrain:attachSelf(self, self.onTraining)
end

return trainArmyNum
