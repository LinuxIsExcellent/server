-- 迁城X次
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local teleport = questBase:extend()

function teleport:init(a, info)
    self.evtTeleport = a.map.evtTeleport
    teleport.super.init(self, a, info)
end

function teleport:onTeleport(x,y,isAdvance)
	-- if not isAdvance then
	-- 	return
	-- end
    teleport.super.advanceAndSendUpdate(self, 1)
end

function teleport:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtTeleport:attachSelf(self, self.onTeleport)
end

return teleport