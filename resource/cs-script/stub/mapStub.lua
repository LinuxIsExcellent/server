local cluster = require('cluster')
local p = require('protocol')
local t = require('tploader')
local timer = require('timer')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()
local utils = require('utils')
local pubMail = require('mail')
local idService = require('idService')
local loginService = require('loginService')
local dataService = require('dataService')
local pushService = require('pushService')
local achievementService = require('achievementService')
local allianceService = require('allianceService')
local chatService = require('chatService')
local palaceWarService = require('palaceWarService')
local reportService = require('reportService')
local marketService = require('marketService')

local rawStub
local mapStub = {
    mapGlobalData = {},
    allAllianceCity = {},
}

local subscriber = {
    crossTeleportGetUid = function(session)
        -- print("crossTeleportGetUid =", session)
        local uid = idService.createUid()
        rawStub:cast_crossTeleportGetUidResponse(session, uid)
    end,
    sendMail = function(uid, otherUid, type, subType, attachment, params, isDraw)
        jobExecutor:queue(function()
            -- print('sendMail')
            --create mail
            local mail = pubMail.newMailInfo({
                uid = uid,
                otherUid = otherUid,
                type = type,
                subType = subType,
                attachment = attachment,
                params = params,
                createTime = timer.getTimestampCache(),
                isDraw = isDraw,
            })
            mail.title, mail.content = pubMail.getTitleContentBySubType(mail.subType)
            mail.isLang = true
            dataService.appendMailInfo(mail)
        end)
    end,
    appendPush = function(pushInfo)
        --print("appendPush")
        pushService.appendPush(pushInfo)
    end,

    globalData = function(data)
        achievementService.onMapGlobalDataUpdate(mapStub.mapGlobalData,data)
        mapStub.mapGlobalData = data
        --print("globalData ",utils.serialize(data))
    end,

    allAllianceCity = function(data)
        mapStub.allAllianceCity = data
        --print("allAllianceCity ",utils.serialize(data))
    end,

    allianceOwnOccupyCity = function(alCity)
        allianceService.onAllianceOwnOccupyCity(alCity, alCity.allianceId)
    end,

    allianceLoseOccupyCity = function(alCity)
        allianceService.onAllianceLoseOccupyCity(alCity, alCity.allianceId)
    end,

    allianceOwnCity = function(alCity)
        local city = mapStub.allAllianceCity[alCity.allianceId]
        if city ~= nil then
            table.insert(city,alCity.cityId)
        else
            city = {}
            table.insert(city,alCity.cityId)
            mapStub.allAllianceCity[alCity.allianceId] = city
        end
        allianceService.onAllianceOwnCity(alCity, alCity.allianceId)

        --聊天
        local data = allianceService.getAllianceData(alCity.allianceId)
        if data then
            --local params = {}
            --table.insert(params, { allianceName = data.allianceName, cityId = alCity.cityId })
            local cityTpl = t.mapCity[alCity.cityId]
            local cityName = ''
            if cityTpl then
                cityName = cityTpl.name
            end
            local params = {param1 = cityName, param2 = data.allianceName}

            local chatInfo = {}
            chatInfo.subType = p.ChatSubType.OCCUPY_FAMOUS_CITY
            chatInfo.from_uid = data.uid
            chatInfo.from_nickname = data.nickname
            chatInfo.vipLevel = data.vipLevel
            chatInfo.headId = data.headId
            chatInfo.level = data.userLevel
            chatInfo.langType = 0
            chatInfo.allianceNickname = data.allianceName
            chatInfo.time = timer.getTimestampCache()
            chatInfo.params = utils.serialize(params)
            local chatType, content = t.getChatTypeAndContent(p.ChatSubType.OCCUPY_FAMOUS_CITY)
            if chatType and content then
                chatInfo.type = chatType
                chatInfo.content = content
                chatService.addChat(p.ChatType.ALL, alCity.allianceId, chatInfo)
            end
        end
    end,

    allianceLoseCity = function(alCity)
        local city = mapStub.allAllianceCity[alCity.allianceId]
        if city ~= nil then
            for k,v in pairs(city) do
                if v == alCity.cityId then
                    table.remove(city,k)
                    break
                end
            end
        end
        allianceService.onAllianceLoseCity(alCity, alCity.allianceId)
    end,

    bootstrapFinishResponse = function(...)
        print("bootstrapFinishResponse")
        palaceWarService.onBootstrapFinishResponse(...)
    end,
    
    palaceWarStart = function()
        -- print("palaceWarStart")
        palaceWarService.onPalaceWarStart()
    end,
    palaceWarEnd = function(aid, time)
        -- print("palaceWarEnd =", aid, time)
        palaceWarService.onPalaceWarEnd(aid, time)
    end,
    palaceWarPrepare = function(times, dropId, uids)
        -- print("palaceWarPrepare =", times, dropId, uids)
        palaceWarService.onPalaceWarPrepare(times, dropId, uids)
    end,

    addAllianceRecord = function(record)
        allianceService.addAllianceRecord(record.allianceId, record.type, record.param)
    end,

    addReport = function(report)
        reportService.addReport(report)
    end,

    addTransportRecord = function(record)
        marketService.addTransportRecord(record)
    end,
}

function mapStub.connectService(name)
    print("mapStub.connectService", name)
    rawStub = cluster.connectService(name)
    rawStub:setSubscriber(subscriber)
end

function mapStub.cast_alliance_sync_all(alliances)
    rawStub:cast_alliance_sync_all(alliances)
end

function mapStub.cast_alliance_create(alliance)
    rawStub:cast_alliance_create(alliance)
end

function mapStub.cast_alliance_update(alliance)
    rawStub:cast_alliance_update(alliance)
end

function mapStub.cast_alliance_disband(aid)
    rawStub:cast_alliance_disband(aid)
end

function mapStub.cast_alliance_add_member(aid, uid)
    rawStub:cast_alliance_add_member(aid, uid)
end

function mapStub.cast_alliance_remove_member(aid, uid, allianceCdTimestamp)
    rawStub:cast_alliance_remove_member(aid, uid, allianceCdTimestamp)
end

function mapStub.cast_alliance_record_invited(uid)
    rawStub:cast_alliance_record_invited(uid)
end

function mapStub.cast_alliance_buff_open(aid, allianceBuff)
    rawStub:cast_alliance_buff_open(aid, allianceBuff)
end

function mapStub.cast_alliance_buff_closed(aid, buffId)
    rawStub:cast_alliance_buff_closed(aid, buffId)
end

function mapStub.cast_alliance_science_update(aid, science)
    rawStub:cast_alliance_science_update(aid, science)
end

function mapStub.cast_activitiesUpdate(list)
    -- print("cast_activitiesUpdate")
    rawStub:cast_activitiesUpdate(list)
end

function mapStub.cast_report_sync(reportmax)
    rawStub:cast_report_sync(reportmax)
end

function mapStub.onBootstrapFinish()
    -- print("onCSBootstrapFinish")
    rawStub:cast_onCSBootstrapFinish()
end
--palace war begin
function mapStub.onKingChanged(uid)
    -- print("mapStub.onKingChanged", uid)
    rawStub:cast_onKingChanged(uid)
end

function mapStub.cast_report_sync(reportmax)
    rawStub:cast_report_sync(reportmax)
end

--list={uid = titleId}
function mapStub.onTitlesUpdate(list)
    -- print("mapStub.onTitlesUpdate")
    rawStub:cast_onTitlesUpdate(list)
end

--palace war end

return mapStub
