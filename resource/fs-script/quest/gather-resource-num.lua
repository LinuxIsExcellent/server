-- 采集x次资源
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local gatherResourceNum = questBase:extend()

function gatherResourceNum:init(a, info)
     self.evtResourceGather = a.map.evtResourceGather
    gatherResourceNum.super.init(self, a, info)
end

function gatherResourceNum:onGather(type, count)
    if type == self.qInfo.tpl.param1 then
        gatherResourceNum.super.advanceAndSendUpdate(self, 1)
    end
end

function gatherResourceNum:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtResourceGather:attachSelf(self, self.onGather)
end

return gatherResourceNum