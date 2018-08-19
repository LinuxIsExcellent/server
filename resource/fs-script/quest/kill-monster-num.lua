-- 击杀x次x级野
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local killMonsterNum = questBase:extend()

function killMonsterNum:init(a, info)
     self.evtKillMonster = a.map.evtKillMonster
    killMonsterNum.super.init(self, a, info)
end

function killMonsterNum:killMonster(id)
    local tpl = t.mapUnit[id]
    if tpl then
        -- Todo: 暂时先不区分 普通，精英 
        if tpl.level >= self.qInfo.tpl.param2 then
            killMonsterNum.super.advanceAndSendUpdate(self, 1)
        end
    end
end

function killMonsterNum:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtKillMonster:attachSelf(self, self.killMonster)
end

return killMonsterNum