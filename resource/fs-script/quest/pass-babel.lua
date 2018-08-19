-- 通关千重楼到X层
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local passBabel = questBase:extend()

function passBabel:init(a, info)
    self.evtPassBabel = a.babel.evtPassBabel

    if info.progress == 0 then
        if a.babel.historylayer >= info.tpl.param1 then
            info.progress = 1
        end
    end
    

    passBabel.super.init(self, a, info)
    -- print('passBabel:init ...')
end

function passBabel:onPass(layer)
    if layer >= self.qInfo.tpl.param1 then
        passBabel.super.advanceAndSendUpdate(self, 1)
    end
end

function passBabel:onInit()
    -- print('passBabel:onInit...')
    if self:questInfo():isFinish() then
        return
    end
    self.evtPassBabel:attachSelf(self, self.onPass)
end

return passBabel