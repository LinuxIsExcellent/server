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

local mapStub

local rawService
local impl = {}



framework.beforeShutdown(function(cb)
    M.queueJob(function()
        utils.debug('framework.beforeShutdown market')
        if M.timer ~= nil then
            M.timer:cancel()
            M.timer = nil
        end
        M.dbSave()
        cb()
    end, cb)
end)

function M.queueJob(threadFun, errorFun)
    jobExecutor:queue(threadFun, errorFun)
end

function M.start(mapStub_)
    rawService = cluster.createService('market', impl)
    mapStub = mapStub_
    -- sync to ms
end


function M.dbSave()
end

-- 增加运输状态的记录
function M.addTransportRecord(record)
    --运输记录
    --传输者
    local myRecord = {
        id = record.transportId1,
        troopId = record.troopId,
        uid = record.uid,
        headId = record.headId,
        nickName = record.nickName,
        toUid = record.toUid,
        toHeadId = record.toHeadId,
        toNickName = record.toNickName,
        transportType = p.TransportType.TRANSMIT,
        carry = utils.serialize(record.carry),
        arriveType = record.ArriveType,
        arriveTime = record.arriveTime
    }
    dataService.appendData(p.DataClassify.TRANSPORT_RECORD, myRecord)
    --接收者
    local toRecord = {
        id = record.transportId2,
        troopId = record.troopId,
        uid = record.toUid,
        headId = record.toHeadId,
        nickName = record.toNickName,
        toUid = record.uid,
        toHeadId = record.headId,
        toNickName = record.nickName,
        transportType = p.TransportType.RECEIVE,
        carry = utils.serialize(record.carry),
        arriveType = record.ArriveType,
        arriveTime = record.arriveTime
    }
    dataService.appendData(p.DataClassify.TRANSPORT_RECORD, toRecord)
end

return M