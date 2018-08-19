-- 擂台挑战次数
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local arenaFightNum = questBase:extend()

function arenaFightNum:init(a, info)
    self.evtFightArena = a.arena.evtFightArena
    arenaFightNum.super.init(self, a, info)
end

function arenaFightNum:onFight(isWin)
    arenaFightNum.super.advanceAndSendUpdate(self, 1)
end

function arenaFightNum:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtFightArena:attachSelf(self, self.onFight)
end

return arenaFightNum