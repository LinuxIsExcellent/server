local M = {}
local p = require('protocol')
local t = require('tploader')
local cluster = require('cluster')
local dbo = require('dbo')
local http = require('http')
local timer = require('timer')
local pubMail = require('mail')
local misc = require('libs/misc')
local utils = require('utils')

local framework = require('framework')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()
local mapStub
local allianceService = require('allianceService')
local loginService = require('loginService')
local dataService = require('dataService')

local rawService
local impl = {}

local winAid = 0             --胜利联盟的ID
local lastEndTimestamp = 0

local timerChooseKing
local king = {
    uid = 0,
}
local titles = {} --<tplid, titleInfo>
local gifts = {} --<tplid, [uids]> 已分配过礼包的玩家记录

local dbSavePalaceWar

-- framework node shutdown
framework.beforeShutdown(function(cb)
    jobExecutor:queue(function()
        utils.debug('framework.beforeShutdown palaceWar')
        if timerChooseKing then
            timerChooseKing:cancel()
            timerChooseKing = nil
        end
        cb()
    end, cb)
end)

--publish begin
local function publishTitlesUpdate(list)
    -- print("publishTitlesUpdate")
    rawService:publishToAll("onTitlesUpdate", list)
end

local function publishGiftsUpdate(list)
    -- print("publishGiftsUpdate")
    rawService:publishToAll("onGiftsUpdate", list)
end
--publish end

--称号 title begin
--[[
local titleInfo = {
    tplid = 0,
    uid = 0,
    headId = 0,
    nickname = "",
    lastCancelTimestamp = 0,
}
]]

--db begin
local function dbLoadUserInfo(uid)
    local info = {}
    local db = dbo.open(0)
    -- print("*****************************************dbLoadUserInfo")
    local rs = db:executePrepare('SELECT headId, nickname FROM s_user WHERE uid = ?', uid)
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            info.headId = row.headId
            info.nickname = row.nickname
            break
        end
    else
        return nil
    end
    return info
end

local function dbLoadPalaceWar()
    local url = 'http://' .. t.miscConf.hubSite.ip .. ':' .. tostring(t.miscConf.hubSite.port) .. '/gms_api.php'
    local urlParams = {}
    urlParams.api = 'cs_load_palaceWar'
    urlParams.id = utils.getMapServiceId()

    local code, obj = http.getForObject(url, urlParams)
    if code == 200 and obj ~= nil then
        if obj.kingUid then
            king.uid = obj.kingUid
            titles = misc.deserialize(obj.titles)
            gifts = misc.deserialize(obj.gifts)
        end
    end
end

local lastSaveParams = nil

dbSavePalaceWar = function ()
    local url = 'http://' .. t.miscConf.hubSite.ip .. ':' .. tostring(t.miscConf.hubSite.port) .. '/gms_api.php'
    local urlParams = {}
    urlParams.api = 'cs_save_palaceWar'
    urlParams.id = utils.getMapServiceId()
    urlParams.kingUid = king.uid
    urlParams.titles = misc.serialize(titles)
    urlParams.gifts = misc.serialize(gifts)

    local save = false
    if lastSaveParams == nil then
        save = true
        lastSaveParams = urlParams
    else
        if urlParams.kingUid ~= lastSaveParams.kingUid or urlParams.titles ~= lastSaveParams.titles or urlParams.gifts ~= lastSaveParams.gifts then
            save = true
            lastSaveParams = urlParams
        end
    end
    if save then
        local code, obj = http.getForObject(url, urlParams)
        if code == 200 and obj ~= nil then
            -- done
        end
    end
end
--db end

local function isPlayerHaveTitle(uid)
    for _, v in pairs(titles) do
        if v.uid == uid then
            return true
        end
    end
    return false
end

local function addTitle(tplid, uid, headId, nickname)
    -- print("addTitle***************************")

    local info = titles[tplid]
    if info then
        info.tplid = tplid
        info.uid = uid
        info.headId = headId
        info.nickname = nickname
    else
        titles[tplid] = {
            tplid = tplid,
            uid = uid,
            headId = headId,
            nickname = nickname,
            lastCancelTimestamp = 0,
        }
    end
    --publish all fs
    local list = {}
    table.insert(list, titles[tplid])
    publishTitlesUpdate(list)
    --notice uid's agent update
    loginService.cast(uid, 'update_title', tplid)
    --sync map
    local tomsList = {}
    tomsList[uid] = tplid
    mapStub.onTitlesUpdate(tomsList)
    --broad notice message
    if tplid == p.TitleType.KING then
        -- local gmsService = require('gmsService')
        -- gmsService.setUiSwitch(104, true)
    else
        loginService.broadNoticeMessage(1903003, 2, tplid .. ',' .. nickname)
    end
end
--title end

--gift begin
local function hadGiven(uid)
    for _, v in pairs(gifts) do
        for _, hadGivenUid in pairs(v) do
            if hadGivenUid == uid then
                return true
            end
        end
    end
    return false
end
--gift end


--impl begin
function impl._onStubConnect(mbid)
    --print("onStubConnect")
end

function impl.fetch_all_titles()
    return titles
end

function impl.fetch_all_gifts()
    return gifts
end

--萌主选择国王 giver:分配者 givener:被分配者
function impl.chooseKing(giver, givener)
    local session = rawService:session()
    if timerChooseKing == nil then
        print("palace state is not right")
        rawService:reply(session, 5)
        return
    end
    --国王是否存在
    if king.uid ~= 0 then
        rawService:reply(session, 1)
        return
    end
    --判断权限
    local al = allianceService.findAllianceByAid(winAid)
    if not al then
        print("alliance not found : winAid", winAid)
        rawService:reply(session, 2)
        return
    end
    if al.leaderId ~= giver then
        print("not leader : leaderId, giver", al.leaderId, giver)
        rawService:reply(session, 3)
        return
    end
    local m = al:findMember(givener)
    if not m then
        print("member not found : givener", givener)
        rawService:reply(session, 4)
        return
    end
    --分配
    king.uid = givener
    addTitle(p.TitleType.KING, givener, m.headId, m.nickname)
    --通知地图服务器
    mapStub.onKingChanged(king.uid)
    rawService:reply(session, 0)
    --解除自动选王定时器
    timerChooseKing:cancel()
    timerChooseKing = nil
    --db
    jobExecutor:queue(function()
        dbSavePalaceWar()
    end)
end

--国王分配称号 giver:分配者 tplid:称号模板ID givener:被分配者
function impl.giveTitle(giver, tplid, givener)
    local session = rawService:session()
    jobExecutor:queue(function()
        --分配者是否国王
        if giver ~= king.uid then
            print("giver is not king : giver, kingUid", giver, king.uid)
            rawService:reply(session, 1)
            return
        end
        local tpl = t.title[tplid]
        if not tpl or tplid == p.TitleType.KING then
            print("tpl not found : tplid", tplid)
            rawService:reply(session, 2)
            return
        end

        local info = titles[tplid]
        if info and info.uid ~= 0 then
            print("title have player : tplid, uid", tplid, info.uid)
            rawService:reply(session, 3)
            return
        end

        if isPlayerHaveTitle(givener) then
            print("player have title : givener", givener)
            rawService:reply(session, 4)
            return
        end

        local uInfo = dbLoadUserInfo(givener)
        if not uInfo then
            print("uInfo is nil : givener", givener)
            rawService:reply(session, 5)
            return
        end

        addTitle(tplid, givener, uInfo.headId, uInfo.nickname)
        rawService:reply(session, 0)
        --db
        dbSavePalaceWar()
    end)
end

--国王取消称号 giver:分配者
function impl.cancelTitle(giver, tplid)
    local session = rawService:session()
    if tplid == 1 then
        print("the title is king : giver, kingUid", giver, king.uid)
        rawService:reply(session, 4)
        return
    end
    --分配者是否国王
    if giver ~= king.uid then
        print("giver is not king : giver, kingUid", giver, king.uid)
        rawService:reply(session, 1)
        return
    end
    --称号是否有玩家
    local info = titles[tplid]
    if not info or info.uid == 0 then
        print("title have no player : tplid", tplid)
        rawService:reply(session, 2)
        return
    end
    --同一天只能取消一次
    if timer.isTodayFromTimestamp(info.lastCancelTimestamp) then
        print("today had canceled : lastCancelTimestamp", info.lastCancelTimestamp)
        rawService:reply(session, 3)
        return
    end
    --清理称号数据
    local oldUid = info.uid
    info.uid = 0
    info.headId = 0
    info.nickname = ""
    info.lastCancelTimestamp = timer.getTimestampCache()

    --publish all fs
    local list = {}
    table.insert(list, info)
    publishTitlesUpdate(list)
    --notice oldUid's agent update
    loginService.cast(oldUid, 'update_title', 0)
    rawService:reply(session, 0)
    --sync to ms
    local tomsList = {}
    tomsList[oldUid] = 0
    mapStub.onTitlesUpdate(tomsList)
    --db
    jobExecutor:queue(function()
        dbSavePalaceWar()
    end)
end

--国王分配礼包 giver:分配者 tplid:礼包模板ID giveners:被分配者列表
function impl.giveGift(giver, tplid, giveners)
    local session = rawService:session()
    jobExecutor:queue(function()
        --分配者是否国王
        if giver ~= king.uid then
            print("giver is not king : giver, kingUid", giver, king.uid)
            rawService:reply(session, 1)
            return
        end
        local KingInfo = dbLoadUserInfo(giver)
        if not KingInfo then
            return
        end 
        --检查模板
        local tpl = t.configure["palaceWarGift"]
        if not tpl or not tpl[tplid] then
            print("tpl not found : tplid", tplid)
            rawService:reply(session, 2)
            return
        end
        --礼包最大数量
        local max = tpl[tplid]
        local willGiveSize = #giveners
        local hadGivenUids = gifts[tplid] or {}
        local hadGivenSize = #hadGivenUids
        if hadGivenSize + willGiveSize > max then
            print("up to max : hadGivenSize, willGiveSize, max", hadGivenSize, willGiveSize, max)
            rawService:reply(session, 3)
            return
        end
        --进行分配
        local params = {}
        local drops = {{tplId = tplid, count = 1}}
        local Itemtpl = t.item[tplid]
        params.params1 = KingInfo.nickname .. "," .. Itemtpl.name
        for _, givener in ipairs(giveners) do
            if not hadGiven(givener) then
                local uInfo = dbLoadUserInfo(givener)
                if uInfo then
                    --record
                    table.insert(hadGivenUids, givener)
                    --mail
                    local mail = pubMail.newMailInfo()
                    mail.type = p.MailType.SYSTEM
                    mail.subType = p.MailSubType.SYSTEM_PALACE_WAR_GIFT
                    mail.createTime = timer.getTimestampCache()
                    mail.title, mail.content = pubMail.getTitleContentBySubType(mail.subType)
                    mail.attachment = misc.serialize(drops)
                    mail.params = utils.serialize(params)
                    mail.parentId = 0
                    mail.isDraw = true
                    mail.isLang = true
                    mail.uid = givener
                    dataService.appendMailInfo(mail)
                end
            end
        end
        gifts[tplid] = hadGivenUids
        publishGiftsUpdate({[tplid] = hadGivenUids})
        rawService:reply(session, 0)
        --db
        dbSavePalaceWar()
    end)
end
--impl end

function M.getKingUid()
    return king.uid
end

function M.start(mapStub_)
    rawService = cluster.createService('palaceWar', impl)
    mapStub = mapStub_
    dbLoadPalaceWar()
    --sync to ms
    local tomslist = {}
    for _, v in pairs(titles) do
        if v.uid > 0 then
            tomslist[v.uid] = v.tplid
        end
    end
    mapStub.onTitlesUpdate(tomslist)
end

function M.onPalaceWarStart()
    print("onPalaceWarStart")
    --清理国王数据
    king.uid = 0
    mapStub.onKingChanged(king.uid)
    --清理称号数据
    local tomsList = {}
    for tplid, v in pairs(titles) do
        local oldUid = v.uid
        v.uid = 0
        v.headId = 0
        v.nickname = ''
        --notice uid's agent update
        loginService.cast(oldUid, 'update_title', 0)
        tomsList[oldUid] = 0
    end
    publishTitlesUpdate(titles)
    --sync to ms
    mapStub.onTitlesUpdate(tomsList)
    titles = {}
    --清理礼包数据
    for tplid, v in pairs(gifts) do
        gifts[tplid] = {}
    end
    publishGiftsUpdate(gifts)
    --db
    jobExecutor:queue(function()
        dbSavePalaceWar()
    end)
end

function M.onPalaceWarEnd(aid, time)
    winAid = aid
    print("***********************onPalaceWarEnd")
    -- local uInfo = dbLoadUserInfo(winAid)
    -- if not uInfo then
    --         print("uInfo is nil : givener", givener)
    -- end
    print("time =", time, aid)
    timerChooseKing = timer.setTimeout(function()
        local al = allianceService.findAllianceByAid(winAid)
        if al then
            
            king.uid = al.leaderId
            --notice map service
            mapStub.onKingChanged(king.uid)
            --add title
            addTitle(p.TitleType.KING, king.uid, al.leaderHeadId, al.leaderName)
            --db
            jobExecutor:queue(function()
                dbSavePalaceWar()
            end)
            timerChooseKing = nil
        end
    end, time * 1000)
end

--王城争霸备战阶段 times:邮件通知次数
function M.onPalaceWarPrepare(times, dropId, uids)
    print("onPalaceWarPrepare uid size = ", #uids, times, dropId)
    local subType
    if times == 1 then
        subType = p.MailSubType.PALACE_WAR_PREPARE_FIRST
    elseif times == 2 then
        subType = p.MailSubType.PALACE_WAR_PREPARE_SECOND
    elseif times == 3 then
        subType = p.MailSubType.PALACE_WAR_PREPARE_THIRD
    else
        print("times invalid")
        return
    end
    if #uids == 0 then
        print("uids size is 0")
        return
    end

    local dropTpl = t.drop[dropId]
    local drops = {}
    if dropTpl then
        drops = dropTpl:DoDrop()
    end
    local params = {}
    params.params1 = "" .. "," ..""
    for _, uid in ipairs(uids) do
        --mail
        local mail = pubMail.newMailInfo()
        mail.type = p.MailType.SYSTEM
        mail.subType = subType
        mail.createTime = timer.getTimestampCache()
        mail.title, mail.content = pubMail.getTitleContentBySubType(mail.subType)
        mail.attachment = misc.serialize(drops)
        mail.params = utils.serialize(params)
        mail.parentId = 0
        mail.isLang = true
        mail.uid = uid
        dataService.appendMailInfo(mail)
    end
end

function M.onBootstrapFinishResponse(...)
    local isProtected, param1, param2 = ...
    if isProtected then
        --可能要选王
        --param1:胜利联盟ID param2:倒计时
        if king.uid == 0 and param1 > 0 and param2 >= 0 and timerChooseKing == nil then
            M.onPalaceWarEnd(param1, param2)
        end
    else
        if king.uid ~= 0 then
            M.onPalaceWarStart()
        end
    end
end

return M
