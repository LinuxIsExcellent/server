local M = {}
local cluster = require('cluster')
local dbo = require('dbo')
local utils = require('utils')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()

local rawService
local impl = {}

local uid = 0
local aid = 0

function M.start()
    rawService = cluster.createService('id', impl)
    M.initId()
end

function M.queueJob(threadFun)
    jobExecutor:queue(threadFun)
end

function M.initId()
    local db = dbo.open(0)
    local rs = db:execute('SELECT k,v from s_incr')
    if rs.ok then
        for _, row in ipairs(rs) do
            if row.k == 'uid' then uid = row.v end
            if row.k == 'aid' then aid = row.v end
        end
    end
end

function M.createUid()
    local sid = utils.getMapServiceId()
    -- print("***************sid, uid", sid, uid)
    uid = uid + 1
    M.queueJob(function()
        local db = dbo.open(0)
        local rs = db:executePrepare('INSERT INTO s_incr (k, v) VALUES (?, ?) ON DUPLICATE KEY UPDATE v = VALUES(v)', 'uid', uid)
    end)
    return utils.createUid(sid, uid)
end

function M.createAid()
    local sid = utils.getMapServiceId()
    aid = aid + 1
    M.queueJob(function()
        local db = dbo.open(0)
        local rs = db:executePrepare('INSERT INTO s_incr (k, v) VALUES (?, ?) ON DUPLICATE KEY UPDATE v = VALUES(v)', 'aid', aid)
    end)
    return utils.createUid(sid, aid)
end

--
-- id service api implement
--

function impl.createUid()
    -- print("center server impl createUid")
    return M.createUid()
end

function impl.createAid()
    return M.createAid()
end

return M

