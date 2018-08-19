local p = require('protocol')
local t = require('tploader')
local timer = require('timer')
local class = require('libs/30log')

local questBase = class('questBase')

function questBase:init(a, info)
    self.agent = a
    if info.progress >= info.total then
        --print('questBase:init .... progress:', info.progress, 'total:', info.total)
        info.progress = info.total
        info.finishTime = timer.getTimestampCache()
    end
    self.qInfo = info
end

function questBase:questId()
    return self.qInfo.tpl.id
end
function questBase:questInfo()
    return self.qInfo
end
function questBase:isFinish()
    return self.qInfo:isFinish()
end

function questBase:type()
    return self.qInfo.tpl.type
end

function questBase:condType()
    return self.qInfo.tpl.CondType
end


function questBase:Finish()
    self.qInfo.progress = self.qInfo.total
    self.qInfo.finishTime = timer.getTimestampCache()
    self.agent.quest.evtFinishQuest:trigger(self.qInfo.tpl.id)
    self.qInfo:setSync(false)
    self.qInfo:setDirty()
end


-- 逐步累加
function questBase:advance(step, isReplace)
    if (self.qInfo:finish()) then
        return
    end
    local old = self.qInfo.progress
    -- print("self.qInfo.progress....",self.qInfo.progress)
    -- 是否直接赋值
    if isReplace then
        self.qInfo.progress = step
    else
        self.qInfo.progress = self.qInfo.progress + step
    end   
    if self.qInfo.progress >= self.qInfo.total then
        self.qInfo.progress = self.qInfo.total
        self.qInfo.finishTime = timer.getTimestampCache()
        self.agent.quest.evtFinishQuest:trigger(self.qInfo.tpl.id)
    end

    print("----- quest old --- ", old, "  ---now --- ", self.qInfo.progress)
    if old ~= self.qInfo.progress then
        self.qInfo:setSync(false)
        self.qInfo:setDirty()
    end
end

function questBase:advanceAndSendUpdate(step, isReplace)
    -- print('questBase:advanceAndSendUpdate....id:', self.qInfo.tpl.id, "CondType:", self.qInfo.tpl.CondType, "step:", step)
    questBase.advance(self, step, isReplace)

    if not self.qInfo:isSync() then
        -- send to client
        --print('questBase:advanceAndSendUpdate.... sync')
        self.agent.quest.onQuestUpdate(self.qInfo.tpl.id, self.qInfo.tpl.type)
    end
end

return questBase
