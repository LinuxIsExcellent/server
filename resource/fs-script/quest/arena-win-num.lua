-- 擂台胜利次数
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local arenaWinNum = questBase:extend()

function arenaWinNum:init(a, info)
    self.evtFightArena = a.arena.evtFightArena
    arenaWinNum.super.init(self, a, info)
end

function arenaWinNum:onFight(isWin)
	if not isWin then
		return
	end
	
    arenaWinNum.super.advanceAndSendUpdate(self, 1)
end

function arenaWinNum:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtFightArena:attachSelf(self, self.onFight)
end

return arenaWinNum