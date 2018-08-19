local cluster = require('cluster')
local p = require('protocol')
local timer = require('timer')
local misc = require('libs/misc')
local loginStub = require('stub/login')

local gmsStub = {
    rawStub,

    marquees = {},
    notices = {},
    mailBatches = {},
    chargeTpls = {},
}

local function parseMatchLang(tpl)
    local v = tpl.matchLang
    if v == '' then
        tpl.matchLang_ = ''
        return
    end

    tpl.matchLang_ = misc.stringSplitInt(v, '|')
end

local function parseMatchChannel(tpl)
    local v = tpl.matchChannel
    if v == '' then
        tpl.matchChannel_ = ''
        return
    end

    tpl.matchChannel_ = misc.stringSplit(v, '|')
end

local function parseMatchVersion(tpl)
    local v = tpl.matchVersion
    if v == '' then
        tpl.matchVersion_ = ''
        return
    end

    local opt = v:sub(1, 1)
    local vs = misc.stringSplitInt(v:sub(2, v:len()), '.')
    tpl.matchVersion_ = {
        opt = opt,
        major = vs[1] or 0,
        minor = vs[2] or 0,
        revision = vs[3] or 0,
        build = vs[4] or 0,
    }
end

local function parseMatchLevel(tpl)
    local v = tpl.matchLevel
    if v == '' then
        tpl.matchLevel_ = ''
        return
    end

    local opt = v:sub(1, 1)
    local level = tonumber(v:sub(2, v:len())) or 0
    tpl.matchLevel_ = {
        opt = opt,
        level = level,
    }
end

local function parseMatchLoginTime(tpl)
    local v = tpl.matchLoginTime
    if v == '' then
        tpl.matchLoginTime_ = ''
        return
    end

    local startTime, endTime = v:match("(%d+)|(%d+)")
    tpl.matchLoginTime_ = {
        startTime = tonumber(startTime),
        endTime = tonumber(endTime),
    }
end

local subscriber = {
    marquees_update = function(allMarquees)
        if allMarquees == nil then
            return
        end
        local list = {}
        for _, v in pairs(allMarquees) do
            parseMatchLang(v)
            parseMatchVersion(v)
            parseMatchChannel(v)
            parseMatchLevel(v)
            table.insert(list, { tpl = v, loop = v.doLoop, last_do_ts = 0 })
        end
        gmsStub.marquees = list
        gmsStub.update()
    end,

    notices_update = function(allNotices)
        if allNotices == nil then
            return
        end
        local list = {}
        for _, v in pairs(allNotices) do
            table.insert(list, v)
        end
        gmsStub.notices = list
        -- notify to client
        loginStub.broadNoticeUpdate()
    end,

    mailBatches_update = function(allMailBatches)
        if allMailBatches == nil then
            return
        end
        local list = {}
        for _, v in pairs(allMailBatches) do
            parseMatchLang(v)
            parseMatchVersion(v)
            parseMatchChannel(v)
            parseMatchLevel(v)
            parseMatchLoginTime(v)
            table.insert(list, v)
        end
        gmsStub.mailBatches = list
        -- notify to pick
        loginStub.broadMailBatchUpdate()
    end,

    chargeTpls_update = function(allChargeTpls)
        if allChargeTpls == nil then
            return
        end
        local list = {}
        for _, v in pairs(allChargeTpls) do
            list[v.id] = v
        end
        gmsStub.chargeTpls = list
        -- notify to client
        loginStub.broadChargeTplUpdate()
    end,
}

local rawStub

function gmsStub.connectService()
    rawStub = cluster.connectService('gms@cs')
    gmsStub.rawStub = rawStub
    rawStub:setSubscriber(subscriber)
    -- fetch cs data
    gmsStub.fetchAllMarquees()
    gmsStub.fetchAllNotices()
    gmsStub.fetchAllMailBatches()
    gmsStub.fetchAllChargeTpls()

    gmsStub.updateTimer = timer.setInterval(gmsStub.update, 30 * 1000)
end

function gmsStub.update()
    local now = timer.getTimestampCache()
    for _, v in pairs(gmsStub.marquees) do
        repeat
            if v.loop == 0 then
                break
            end
            if v.last_do_ts + v.tpl.doInterval > now then
                break
            end
            if now < v.tpl.openTime or now > v.tpl.closeTime then
                break
            end

            loginStub.broadMarquee(v.tpl)

            if v.loop > 0 then
                v.loop = v.loop - 1
            end
            v.last_do_ts = now

        until true
    end
end

function gmsStub.fetchAllMarquees()
    local ret, allMarquees = rawStub:call_fetch_all_marquees()
    if ret and allMarquees then
        local list = {}
        for _, v in pairs(allMarquees) do
            parseMatchLang(v)
            parseMatchVersion(v)
            parseMatchChannel(v)
            parseMatchLevel(v)
            table.insert(list, { tpl = v, loop = v.doLoop, last_do_ts = 0 })
        end
        gmsStub.marquees = list
    end
end

function gmsStub.fetchAllNotices()
    local ret, allNotices = rawStub:call_fetch_all_notices()
    if ret and allNotices then
        local list = {}
        for _, v in pairs(allNotices) do
            table.insert(list, v)
        end
        gmsStub.notices = list
    end
end

function gmsStub.fetchAllMailBatches()
    local ret, allMailBatches = rawStub:call_fetch_all_mail_batches()
    if ret and allMailBatches then
        local list = {}
        for _, v in pairs(allMailBatches) do
            parseMatchLang(v)
            parseMatchVersion(v)
            parseMatchChannel(v)
            parseMatchLevel(v)
            parseMatchLoginTime(v)
            table.insert(list, v)
        end
        gmsStub.mailBatches = list
    end
end

function gmsStub.fetchAllChargeTpls()
    local ret, allChargeTpls = rawStub:call_fetch_all_chargeTpls()
    if ret and allChargeTpls then
        local list = {}
        for _, v in pairs(allChargeTpls) do
            list[v.id] = v
        end
        gmsStub.chargeTpls = list
    end
end

return gmsStub
