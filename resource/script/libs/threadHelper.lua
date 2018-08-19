local utils = require('utils')
local timer = require('timer')
local threadHelper = {}

local JobExecutor = {
    isRunning = false,
    jobs = {},
    lastJob = nil,
    lastExecuteTick = 0
}
JobExecutor.__index = JobExecutor

-- 存储所有的执行器
local executorList = {}
setmetatable(executorList, { __mode = 'v' })

threadHelper.timerObj = timer.setInterval(function()
    local costTick = 0
    local nowTick = timer.getTickCache()
    for _, v in ipairs(executorList) do
        if v.lastExecuteTick > 0 then
            costTick = nowTick - v.lastExecuteTick
            if costTick > 180000 then
                v.lastExecuteTick = 0 -- 如果需要限制日志数量
                utils.log('JobExecutor cost too musch time: ' .. debug.traceback(v.lastJob) .. ', cost ms=' .. costTick)
            end
        end
    end
end, 10000)

function JobExecutor:queue(fun, errorFun)
    local job = coroutine.create(function()
        self.isRunning = true
        xpcall(fun, function(msg)
            -- utils.log('JobExecutor:queue ' .. debug.traceback(msg))
            if errorFun then
                errorFun()
            end
        end)
        self.isRunning = false
        self:_tryExecute()
    end)

    table.insert(self.jobs, job)
    self:_tryExecute()
end

function JobExecutor:isBusy()
    --print ('jobExecutor state:', #self.jobs, self.isRunning)
    --self:checkStatus()
    return #self.jobs > 0 or self.isRunning
end

function JobExecutor:checkStatus()
    if self.lastExecuteTick > 0 then
        local nowTick = timer.getTickCache()
        local costTick = nowTick - self.lastExecuteTick
        if costTick > 60000 then
            self.lastExecuteTick = 0 -- 如果需要限制日志数量
            utils.log('JobExecutor cost too musch time: ' .. debug.traceback(self.lastJob) .. ', cost ms=' .. costTick)
        end
    end
end

function JobExecutor:_tryExecute()
    if self.isRunning then
        return
    end

    self.lastJob = nil
    self.lastExecuteTick = 0

    if #self.jobs == 0 then
        return
    end

    local job = table.remove(self.jobs, 1)
    -- 上次开始执行时间
    self.lastJob = job
    self.lastExecuteTick = timer.getTickCache()
    local ok, err = coroutine.resume(job)
    if not ok then
        self.isRunning = false
        utils.log('JobExecutor:_tryExecute Error ' .. err)
    end
end

function threadHelper.createJobExecutor()
    local executor = { jobs = {} }
    setmetatable(executor, JobExecutor)
    table.insert(executorList, executor)
    return executor
end

return threadHelper

