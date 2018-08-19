-- 帮助x次同盟玩家
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local allianceHelpNum = questBase:extend()

function allianceHelpNum:init(a, info)
     self.evtAllianceHelp = a.alliance.evtAllianceHelp
    allianceHelpNum.super.init(self, a, info)
end

function allianceHelpNum:alliaceHelp(count)
    -- print('allianceHelpNum:alliaceHelp, count', count)
    allianceHelpNum.super.advanceAndSendUpdate(self, count)
end

function allianceHelpNum:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtAllianceHelp:attachSelf(self, self.alliaceHelp)
end

return allianceHelpNum