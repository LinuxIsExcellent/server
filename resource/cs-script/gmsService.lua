local M = {}
local t = require('tploader')
local p = require('protocol')
local cluster = require('cluster')
local dbo = require('dbo')
local http = require('http')
local timer = require('timer')
local misc = require('libs/misc')
local utils = require('utils')
local pubMail = require('mail')
local framework = require('framework')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()

local loginService = require('loginService')
local dataService = require('dataService')

local rawService
local impl = {}
local marquees = {}
local notices = {}
local mailBatches = {}
local chargeTpls = {}
local timerIndex = 0

-- framework node shutdown
framework.beforeShutdown(function(cb)
    M.queueJob(function()
        utils.debug('framework.beforeShutdown gms')
        if M.timer ~= nil then
            M.timer:cancel()
            M.timer = nil
        end
        cb()
    end, cb)
end)

function M.start()
    rawService = cluster.createService('gms', impl)
    M.loadMarquees()
    M.loadNotices()
    M.loadMailBatches()
    M.loadChargeTpls()
    M.timer = timer.setInterval(M.update, 10 * 1000)
end

function M.queueJob(threadFun, errorFun)
    jobExecutor:queue(threadFun, errorFun)
end

local function publistAllChargeTplsToClient()
    local list = {}
    for _, v in pairs(chargeTpls) do
        table.insert(list, v)
    end
    rawService:publishToAll('chargeTpls_update', list)
end

function M.loadMarquees()
    local db = dbo.open(0)
    local now = timer.getTimestampCache()
    local rs = db:executePrepare('SELECT * FROM s_marquee where closeTime >? and active=1 order by id desc', now)
    if rs.ok then
        local list = {}
        for k, row in ipairs(rs) do
            row.matchLang = misc.strTrim(row.matchLang)
            row.matchVersion = misc.strTrim(row.matchVersion)
            row.matchChannel = misc.strTrim(row.matchChannel)
            row.matchLevel = misc.strTrim(row.matchLevel)
            table.insert(list, row)
            -- print('#loadMarquees  id, msg_CN=', row.id, row.msg_CN)
        end
        marquees = list
    end
    -- publish
    rawService:publishToAll('marquees_update', marquees)
end

function M.loadNotices()
    local db = dbo.open(0)
    local now = timer.getTimestampCache()
    local rs = db:executePrepare('SELECT * FROM s_notice where expireTime >? order by id desc', now)
    if rs.ok then
        local list = {}
        for k, row in ipairs(rs) do
            table.insert(list, row)
            -- print('#loadNotices  id, title_CN=', row.id, row.title_CN)
        end
        notices = list
    end
    -- publish
    rawService:publishToAll('notices_update', notices)
end

function M.loadMailBatches()
    local db = dbo.open(0)
    local now = timer.getTimestampCache()
    local rs = db:executePrepare('SELECT * FROM s_mail_batch where closeTime > ? order by id desc', now)
    if rs.ok then
        local list = {}
        for k, row in ipairs(rs) do
            if row.dropList == '' then row.dropList = '{}' end
            row.dropList = misc.deserialize(row.dropList, false)
            row.matchLang = misc.strTrim(row.matchLang)
            row.matchVersion = misc.strTrim(row.matchVersion)
            row.matchChannel = misc.strTrim(row.matchChannel)
            row.matchLevel = misc.strTrim(row.matchLevel)
            row.matchLoginTime = misc.strTrim(row.matchLoginTime)
            if row.dropList ~= nil then
                table.insert(list, row)
            end
            -- print('#loadMailBatches  id,  title_CN=', row.id, row.title_CN)
        end
        mailBatches = list
    end
    -- publish
    rawService:publishToAll('mailBatches_update', mailBatches)
end

function M.loadChargeTpls()
    local db = dbo.open(4)
    local now = timer.getTimestampCache()
    local rs = db:execute('SELECT * FROM s_charge')
    if rs.ok then
        local list = {}
        for k, row in ipairs(rs) do
            if row.extraItems == '' then row.extraItems = '{}' end
            local items = misc.deserialize(row.extraItems, false)
            row.extraItems = {}
            for tplId,count in pairs(items) do
                table.insert(row.extraItems, t.createDropItem(tplId, count))
            end

            if now >= row.openTime and row.closeTime > now then
                row.active = true
            else
                row.active = false
            end

            list[row.id] = row
            -- print('#loadChargeTpls  id,  remark, active=', row.id, row.remark, row.active)
        end
        chargeTpls = list
    end
    -- publish
    rawService:publishToAll('chargeTpls_update', chargeTpls)
end


function M.reloadMarquees()
    M.queueJob(function()
        M.loadMarquees()
    end)
end

function M.reloadNotices()
    M.queueJob(function()
        M.loadNotices()
    end)
end

function M.reloadMailBatches()
    M.queueJob(function()
        M.loadMailBatches()
    end)
end

function M.reloadChargeTpls()
    M.queueJob(function()
        M.loadChargeTpls()
    end)
end

function M.update()
    timerIndex = timerIndex + 1
    M.updateChargeTpls()
end

-- check if any charge tpls to be sync to client
function M.updateChargeTpls()
    local needPublish = false
    local now = timer.getTimestampCache()
    --print('updateChargeTpls now=', now)
    for _, v in pairs(chargeTpls) do
        if v.active then
            if  now >=  v.closeTime then
                v.active = false
                needPublish = true
                --print('change id, active=', v.id, v.active)
            end
        else
            if  now >= v.openTime and v.closeTime > now then
                v.active = true
                needPublish = true
                --print('change id, active=', v.id, v.active)
            end
        end
    end
    if needPublish then
        publistAllChargeTplsToClient()
    end
end

function M.loadBandwidthStat(callback)
    M.queueJob(function()
        local stat = {}
        local db = dbo.open(0)
        local rs = db:execute("select k,v from s_stat where k like 'bandwidth_%'")
        if rs.ok and rs.rowsCount > 0 then
            for _, row in ipairs(rs) do
                if row.v ~= '' then
                    local fsId = tonumber(string.match(row.k, '%w+_(%d+)'))
                    local obj = utils.fromJson(row.v)
                    obj.fsId = fsId
                    table.insert(stat, obj)
                end
            end
        end
        callback(stat)
    end, function ()
        callback({})
    end)
end

function M.loadCodePacket(callback)
    M.queueJob(function()
        local stat = {}
        local tempMap = {}
        local db = dbo.open(0)
        local rs = db:execute("select k,v from s_stat where k like 'code_packet_%'")
        if rs.ok and rs.rowsCount > 0 then
            for _, row in ipairs(rs) do
                if row.v ~= '' then
                    local obj = utils.fromJson(row.v)
                    for k,v in pairs(obj) do
                        if not tempMap[v.code] then
                            tempMap[v.code] = v
                        else
                            -- merge all stat of fs
                            tempMap[v.code].count = tempMap[v.code].count + v.count
                            tempMap[v.code].size = tempMap[v.code].size + v.size
                        end
                    end
                end
            end
        end
        for k,v in pairs(tempMap) do
            table.insert(stat, v)
        end
        callback(stat)
    end, function ()
        callback({})
    end)
end

function M.loadSqlCounts(callback)
    M.queueJob(function()
        local stat = {}
        local db = dbo.open(0)
        local rs = db:execute("select k,v from s_stat where k like 'sql_counts_%'")
        if rs.ok and rs.rowsCount > 0 then
            for _, row in ipairs(rs) do
                if row.v ~= '' then
                    local obj = utils.fromJson(row.v)
                    table.insert(stat, { k = row.k, v = obj })
                end
            end
        end
        callback(stat)
    end, function ()
        callback({})
    end)
end

function M.setLocked(callback, uid, seconds, lockedReason)
    M.queueJob(function()
        local lockedTimestamp = seconds
        if seconds <= 0 then
            lockedTimestamp = 0
        else
            lockedTimestamp = timer.getTimestampCache() + seconds
        end
        local db = dbo.open(0)
        local rs = db:executePrepare('update s_user set lockedTimestamp=?, lockedReason=? where uid=?', lockedTimestamp, lockedReason, uid)
        if rs.ok and rs.affectedRows > 0 then
            if seconds > 0 then
                loginService.castLock(uid, lockedTimestamp, lockedReason)
            end
            callback(true)
        else
            callback(false)
        end
    end, function ()
        callback(false)
    end)
end

function M.banChat(callback, uid, seconds, banChatReason)
    M.queueJob(function()
        local banChatTimestamp = seconds
        if seconds <= 0 then
            banChatTimestamp = 0
        else
            banChatTimestamp = timer.getTimestampCache() + seconds
        end
        local db = dbo.open(0)
        local rs = db:executePrepare('update s_user set banChatTimestamp=?, banChatReason=? where uid=?', banChatTimestamp, banChatReason, uid)
        if rs.ok and rs.affectedRows > 0 then
            loginService.castBanChat(uid, banChatTimestamp, banChatReason)
            callback(true)
        else
            callback(false)
        end
    end, function ()
        callback(false)
    end)
end

function M.sendMail(callback, uid, title, content, dropList)
    M.queueJob(function()
        local mail = pubMail.newMailInfo()
        mail.type = p.MailType.SYSTEM
        mail.subType = p.MailSubType.SYSTEM_OPERATION
        mail.createTime = timer.getTimestampCache()
        mail.uid = uid
        mail.title = title
        mail.content = content
        mail.params = utils.serialize({})
        -- make attachment
        local drops = {}
        for tplId, count in pairs(dropList) do
            table.insert(drops, t.createDropItem(tplId, count))
        end
        mail.attachment = utils.serialize(drops)
        -- create mail for user and notify to load it if online
        dataService.appendMailInfo(mail)
        utils.debug(string.format('gms_sendMail uid=%s title=%s content=%s attachment=%s', tostring(uid), title, content, mail.attachment))
        callback(true)
    end, function ()
        callback(false)
    end)
end

function M.logChargeError(message, uid, orderId, chargeId, gold, payType, payTime)
    utils.log(string.format('message=%s uid=%s orderId=%s chargeId=%s gold=%s payType=%s payTime=%s', message, tostring(uid), orderId, chargeId, tostring(gold), payType, tostring(payTime)))
end

function M.processCharge(callback, uid, orderId, chargeId, gold, payType, payTime)
    M.queueJob(function()
        local rc = 0
        local message = 'success'
        repeat
            -- check tpl
            local tpl = chargeTpls[chargeId]
            if not tpl then
                rc = 2
                message = 'charge tpl is not exist'
                M.logChargeError(message, uid, orderId, chargeId, gold, payType, payTime)
                break
            end
            if gold > 0 then -- use gold to charge
                if gold ~= tpl.moneyToGold then
                    rc = 2
                    message = 'chargeId and gold not match'
                    M.logChargeError(message, uid, orderId, chargeId, gold, payType, payTime)
                    break
                end
            else  -- use chargeId to charge
                gold = tpl.moneyToGold
            end

            local level = 1
            local chargeType = 0
            local db = dbo.open(0)
            -- check user
            local rs = db:executePrepare('select uid, level from s_user where uid=?', uid)
            if rs.ok and rs.rowsCount > 0 then
                for _, row in ipairs(rs) do
                    level = row.level
                end
            else
                rc = 2
                message = 'user is not exist'
                M.logChargeError(message, uid, orderId, chargeId, gold, payType, payTime)
                break
            end
            -- check order
            rs = db:executePrepare('select orderId from s_charge where orderId=?', orderId)
            if rs.ok and rs.rowsCount > 0 then
                rc = 1
                message = 'order repeat'
                M.logChargeError(message, uid, orderId, chargeId, gold, payType, payTime)
                break
            end
            -- check charge options

            -- save charge
            rs = db:executePrepare('insert into s_charge (orderId, uid, level, chargeId, chargeType, gold, payType, payTime) values (?, ?, ?, ?, ?, ?, ?, ?)',  orderId, uid, level, chargeId, chargeType, gold, payType, payTime)
            if not rs.ok or rs.affectedRows == 0 then
                rc = 2
                message = 'save charge to db fail'
                M.logChargeError(message, uid, orderId, chargeId, gold, payType, payTime)
                break
            end
        until true

        -- notify user to reload charge if online
        if rc == 0 then
            loginService.castCharge(uid, orderId)
        end

        callback(rc, message)
    end, function ()
        callback(-1, 'error')
    end)
end


--
-- gms service api implement
--
function impl.fetch_all_marquees()
    --print('impl.fetch_all_marquees ...')
    local list = {}
    for _, v in pairs(marquees) do
        table.insert(list, v)
    end
    return list
end

function impl.fetch_all_notices()
    --print('impl.fetch_all_notices ...')
    local list = {}
    for _, v in pairs(notices) do
        table.insert(list, v)
    end
    return list
end

function impl.fetch_all_mail_batches()
    --print('impl.fetch_all_mail_batches ...')
    local list = {}
    for _, v in pairs(mailBatches) do
        table.insert(list, v)
    end
    return list
end

function impl.fetch_all_chargeTpls()
    --print('impl.fetch_all_chargeTpls ...')
    local list = {}
    for _, v in pairs(chargeTpls) do
        table.insert(list, v)
    end
    return list
end

return M
