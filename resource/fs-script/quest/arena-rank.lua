-- 擂台中最高排名进入N名
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local arenaRank = questBase:extend()

function arenaRank:init(a, info)
    self.evtFightArena = a.arena.evtArenaRank
    arenaRank.super.init(self, a, info)
end

function arenaRank:onRank(myRank)
	if myRank <= self.qInfo.tpl.param1 then
    	arenaRank.super.advanceAndSendUpdate(self, 1)
    end
end

function arenaRank:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtFightArena:attachSelf(self, self.onRank)
end

return arenaRank