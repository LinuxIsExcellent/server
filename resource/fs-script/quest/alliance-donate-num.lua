-- 捐献x次联盟科技
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local allianceDonateNum = questBase:extend()

function allianceDonateNum:init(a, info)
     self.evtAllianceDonate = a.alliance.evtAllianceDonate
    allianceDonateNum.super.init(self, a, info)
end

function allianceDonateNum:alliaceDonate()
    -- print('allianceDonateNum:alliaceDonate')
    allianceDonateNum.super.advanceAndSendUpdate(self, 1)
end

function allianceDonateNum:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtAllianceDonate:attachSelf(self, self.alliaceDonate)
end

return allianceDonateNum