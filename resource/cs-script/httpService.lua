local M = {}
local t = require('tploader')
local dbo = require('dbo')
local cluster = require('cluster')
local timer = require('timer')
local utils = require('utils')
local http = require('http')
local misc = require('libs/misc')
local framework = require('framework')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()

local loginService = require('loginService')
local gmsService = require('gmsService')
local rangeService = require('rangeService')
local dataService = require('dataService')
local pushService = require('pushService')

local rawService
local impl = {}
local conf
local server
local areaInfo = {}

local exitCheckCount = 0

-- public interface
function M.getOpenServerTime()
    return areaInfo.ost or 0
end

-- framework node shutdown
framework.beforeShutdown(function(cb)
    M.queueJob(function()
        utils.debug('framework.beforeShutdown http')
        if M.timer then
            M.timer:cancel()
            M.timer = nil
        end
        M.timer = timer.setInterval(function ()
            exitCheckCount = exitCheckCount + 1
            utils.debug('framework.beforeShutdown http ' .. exitCheckCount)
            if M.timer and (loginService.getOnlineCount() == 0 or exitCheckCount > 90) then
                M.timer:cancel()
                M.timer = nil
                dataService.setWaiting(false)
                cb()
                utils.debug('framework.beforeShutdown http end')
            end
        end, 1000)

        pushService.stopPush()
        dataService.setWaiting(true)
        loginService.kickAllPlayer()

    end, cb)
end)


local function createSignature(secret, params)
    local keys = {}
    for k,v in pairs(params) do
        if k ~= 'signature' then
            table.insert(keys, k)
        end
    end
    table.sort(keys)

    local text = ''
    for k,v in pairs(keys) do
        --print(v, params[v])
        text = text .. tostring(params[v])
    end
    text = text .. secret
    local signature = string.lower(utils.sha1(text))

    return signature
end


local function setupHttpServer()
    conf = t.miscConf.http
    server = http.createServer(conf.ip, conf.port)

    server:setHandlers({
        ['/'] = function(req, resp)
            resp:sendHtml('welcome')
        end,

        ['/api/getRegCount'] = function(req, resp)
            --print('/api/getRegCount')
            local jsonParams = {
                regCount = loginService.getRegCount()
            }
            resp:sendJson(jsonParams)
        end,

        ['/api/getOnlineCount'] = function(req, resp)
            --print('/api/getOnlineCount')
            local jsonParams = {
                onlineCount = loginService.getOnlineCount()
            }
            resp:sendJson(jsonParams)
        end,

        ['/api/getFsPayload'] = function(req, resp)
            --print('/api/getFsPayload')
            local jsonParams = loginService.getFsPayload()
            resp:sendJson(jsonParams)
        end,

        ['/api/getBandwidth'] = function(req, resp)
            --print('/api/getBandwidth')
            local function response(obj)
                resp:sendJson(obj)
            end
            gmsService.loadBandwidthStat(response)
        end,

        ['/api/getCodePacket'] = function(req, resp)
            --print('/api/getCodePacket')
            local function response(obj)
                resp:sendJson(obj)
            end
            gmsService.loadCodePacket(response)
        end,

        ['/api/getServerStat'] = function(req, resp)
            --print('/api/getServerStat')
            local memTotal, memFree, diskTotal, diskUsed, cpuUsage = utils.getServerStat()
            local obj = { disk_total = diskTotal, disk_used = diskUsed, memory_total = memTotal, memory_free = memFree, cpu_usage = cpuUsage }
            resp:sendJson(obj)
        end,

        ['/api/getSqlCounts'] = function(req, resp)
            --print('/api/getSqlCounts')
            local function response(obj)
                resp:sendJson(obj)
            end
            gmsService.loadSqlCounts(response)
        end,

        ['/api/reload'] = function(req, resp)
            local type = req.queries.type
            --print('/api/reload  type=', type)
            if type == 'notice' then
            	gmsService.reloadNotices()
            elseif type == 'marquee' then
            	gmsService.reloadMarquees()
            elseif type == 'mail_batch' then
            	gmsService.reloadMailBatches()
            elseif type == 'charge' then
                gmsService.reloadChargeTpls()
            elseif type == 'activity' then
                require('activityService').reLoadActivities()
            end
            resp:sendJson({ ok = true })
        end,

        ['/api/range'] = function(req, resp)
            local type = tonumber(req.queries.type)
            --print('/api/range  type=', type)
            local range = rangeService.getTopRange(type)
            resp:sendJson(range)
        end,

        ['/api/isOnline'] = function(req, resp)
            local uid = tonumber(req.queries.uid)
            local online = false
            if loginService.findUser(uid) then
                online = true
            end
            resp:sendJson({ online = online })
        end,

        ['/api/setLocked'] = function(req, resp)
            local params = {}
            params.uid = tonumber(req.queries.uid)
            params.seconds = tonumber(req.queries.seconds)
            params.reason = misc.decodeURI(req.queries.reason)
            params.signature = misc.decodeURI(req.queries.signature)
            --print('/api/setLocked  uid, seconds, reason=', params.uid, params.seconds, params.reason)
            local function response(ok)
                resp:sendJson({ ok = ok })
            end

            local signature = createSignature(areaInfo.http_key, params)
            if signature ~= params.signature then
                response(false)
            else
                gmsService.setLocked(response, params.uid, params.seconds, params.reason)
            end
        end,

        ['/api/banChat'] = function(req, resp)
            local params = {}
            params.uid = tonumber(req.queries.uid)
            params.seconds = tonumber(req.queries.seconds)
            params.reason = misc.decodeURI(req.queries.reason)
            params.signature = misc.decodeURI(req.queries.signature)
            --print('/api/banChat  uid, seconds, reason=', params.uid, params.seconds, params.reason)
            local function response(ok)
                resp:sendJson({ ok = ok })
            end
            
            local signature = createSignature(areaInfo.http_key, params)
            if signature ~= params.signature then
                response(false)
            else
                gmsService.banChat(response, params.uid, params.seconds, params.reason)
            end
        end,

        ['/api/sendMail'] = function(req, resp)
            local params = {}
            params.uid = tonumber(req.queries.uid)
            params.title = misc.decodeURI(req.queries.title)
            params.content = misc.decodeURI(req.queries.content)
            params.dropList = misc.decodeURI(req.queries.dropList)
            params.signature = misc.decodeURI(req.queries.signature)
            --print('/api/sendMail  uid, title, dropList=', params.uid, params.title, params.dropList)
            local dropList = misc.deserialize(params.dropList, false)

            local function response(ok)
                resp:sendJson({ ok = ok })
            end

            local signature = createSignature(areaInfo.http_key, params)
            if signature ~= params.signature or dropList == nil then
                response(false)
            else
                gmsService.sendMail(response, params.uid, params.title, params.content, dropList)
            end
        end,

        ['/api/shutdown'] = function(req, resp)
            local params = {}
            params.afterSeconds = tonumber(req.queries.afterSeconds)
            if params.afterSeconds < -1 then
                params.afterSeconds = -1
            end
            --print('/api/shutdown  afterSeconds=', params.afterSeconds)

            local function response(ok)
                resp:sendJson({ ok = ok })
            end

            local signature = createSignature(areaInfo.http_key, params)
            if signature ~= params.signature then
                response(false)
            else
                loginService.shutdownByGms(response, params.afterSeconds)
            end
        end,

        ['/api/processCharge'] = function(req, resp)
            local params = {}
            params.uid = tonumber(req.queries.uid)
            params.orderId = misc.decodeURI(req.queries.orderId)
            params.chargeId = misc.decodeURI(req.queries.chargeId)
            params.gold = tonumber(req.queries.gold)
            params.payType = misc.decodeURI(req.queries.payType)
            params.payTime = tonumber(req.queries.payTime)
            params.signature = misc.decodeURI(req.queries.signature)
            --print('/api/processCharge  uid, orderId, chargeId, gold, payType, payTime=', params.uid, params.orderId, params.chargeId, params.gold, params.payType, params.payTime)

            local function response(rc, message)
                resp:sendJson({ rc = rc, message = message })
            end

            local signature = createSignature(areaInfo.http_key, params)
            if signature ~= params.signature then
                response(2, 'signature is wrong')
            else
                gmsService.processCharge(response, params.uid, params.orderId, params.chargeId, params.gold, params.payType, params.payTime)
            end
        end,
     })

end

function M.queueJob(threadFun)
    jobExecutor:queue(threadFun)
end

function M.start()
    rawService = cluster.createService('http', impl)
    M.queryServerInfo()
    M.timer = timer.setInterval(M.timerUpdate, 1000)

    setupHttpServer()
end

function M.timerUpdate()
    local now = timer.getTimestampCache()
    if now % 10 == 0 then
        M.queueJob(function()
            M.queryServerInfo()
        end)
    end
end

function M.queryServerInfo()
    --print('### queryServerInfo')
    local url = 'http://' .. t.miscConf.hubSite.ip .. ':' .. tostring(t.miscConf.hubSite.port) .. '/gms_api.php'
    local urlParams = {}
    urlParams.api = 'cs_load_area'
    urlParams.id = utils.getMapServiceId()

    local code, obj = http.getForObject(url, urlParams)
    if code == 200 and obj ~= nil then
        areaInfo = obj
    end
end

return M

