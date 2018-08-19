local M = {}
local p = require('protocol')
local t = require('tploader')
local cluster = require('cluster')
local dbo = require('dbo')
local allianceInfo = require('alliance')
local timer = require('timer')
local trie = require('trie')
local utils = require('utils')
local framework = require('framework')
local misc = require('libs/misc')
local pubMail = require('mail')
local pubHero = require('hero')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()

local idService = require('idService')
local loginService = require('loginService')
local dataService = require('dataService')
local rangeService = require('rangeService')
local arenaService = require('arenaService')
local achievementService = require('achievementService')
local mapStub

local rawService
local impl = {}

function M.queueJob(threadFun)
    jobExecutor:queue(threadFun)
end



local function dbLoadMaxReport()
    local maxReportId = 0
    local db = dbo.open(0)
    local rs = db:executePrepare('SELECT max(id) as id from s_report')
    if rs.ok then
        for _, row in ipairs(rs) do
            if row.id then
                maxReportId = row.id
            end
        end
    end
    return maxReportId
end

local function dbSaveReport(report)
    M.queueJob(function()
        local db = dbo.open(0)
        local rs = db:executePrepare('INSERT INTO s_report (id, uid, createTime, data) VALUES (?, ?, ?, ?)', report.id, report.uid, report.createTime, report.data)
        if rs.ok then
        end
    end)
end


function M.addReport(report)
    if report then
        dbSaveReport(report)
    end
end


--
-- base
--
function M.start(mapStub_)

    rawService = cluster.createService('report', impl)
    mapStub = mapStub_
    local max = dbLoadMaxReport()
    mapStub.cast_report_sync(max)
end

function M.onSave()
    -- M.queueJob(function()
    --     dbSaveAlliances()
    -- end)
end

return M

