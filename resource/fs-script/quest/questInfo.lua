
local questInfo = {
    tpl = {},
    uid = 0,
    --showTime = 0,
    createTime = 0,
    finishTime = 0,
    drawTime = 0,
    progress = 0,
    total = 0,

    -- sync and dirty
    sync = false,
    dirty = false,
}

function questInfo:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end

function questInfo:isFinish()
    return self.progress >= self.total
end

function questInfo:setSync(v)
    self.sync = v
end

function questInfo:isSync()
    return self.sync
end

function questInfo:isDirty()
    return self.dirty
end

function questInfo:setDirty()
    self.dirty = true
end

function questInfo:setClean()
    self.dirty = false
end

function questInfo:finish(finishTime)
    self.finishTime = finishTime
    self.sync = false
    self.dirty = true
end
function questInfo:isDraw()
    return self.drawTime ~= 0
end
function questInfo:draw(drawTime)
    self.drawTime = drawTime
    self.sync = false
    self.dirty = true
end

function questInfo:toCrossTeleportInfo()
    return {
        questId = self.tpl.id,
        createTime = self.createTime,
        finishTime = self.finishTime,
        drawTime = self.drawTime,
        progress = self.progress,
        total = self.total,
    }
end

return questInfo
