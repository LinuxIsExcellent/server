-- 采集x资源x个
local p = require('protocol')
local event = require('libs/event')
local t = require('tploader')
local questInfo = require ('quest/questInfo')
local questBase = require ('quest/quest-base')

local gatherResource = questBase:extend()

function gatherResource:init(a, info)
     self.evtResourceGather = a.map.evtResourceGather
    gatherResource.super.init(self, a, info)
end

function gatherResource:onGather(type, count)
    if type == self.qInfo.tpl.param1 then
        gatherResource.super.advanceAndSendUpdate(self, count)
    end
end

function gatherResource:onInit()
    if self:questInfo():isFinish() then
        return
    end
    self.evtResourceGather:attachSelf(self, self.onGather)
end

return gatherResource