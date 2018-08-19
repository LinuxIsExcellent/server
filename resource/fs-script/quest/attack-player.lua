-- 攻击其他玩家并获胜X次
local p = require('protocol')
local event = require('libs/event')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local attackPlayer = questBase:extend()

function attackPlayer:init(a, info)
    self.evtAttackPlayer = a.map.evtAttackPlayer
    attackPlayer.super.init(self, a, info)
    -- print('attackPlayer:init ...')
end

function attackPlayer:onAttack(isWin)
    if not isWin then
        return
    end

    -- print('attackPlayer:onAttack isWin')
    attackPlayer.super.advanceAndSendUpdate(self, 1)
end

function attackPlayer:onInit()
    -- print('attackPlayer:onInit...')
    if self:questInfo():isFinish() then
        return
    end
    self.evtAttackPlayer:attachSelf(self, self.onAttack)
end

return attackPlayer
