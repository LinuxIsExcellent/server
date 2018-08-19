local timer = require('timer')

local achievementInfo = {
    tpl = {},
    createTime = 0,
    finishTime = 0,
    progress = 0,

    sync = false,
}

function achievementInfo:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    if self.createTime == 0 then
        self.createTime = timer.getTimestampCache()
    end
    return o
end

function achievementInfo:isFinish()
    return self.progress >= self.tpl.count
end

function achievementInfo:finish(finishTime)
    self.finishTime = finishTime
    self.sync = false
end

function achievementInfo:setProgress(progress)
    self.progress = progress
    self.sync = false
end

function achievementInfo:setSync(v)
    self.sync = v
end

function achievementInfo:isSync()
    return self.sync
end

return achievementInfo
