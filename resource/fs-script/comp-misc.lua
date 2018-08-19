local agent = ...
local p = require('protocol')
local t = require('tploader')
local dbo = require('dbo')
local http = require('http')
local event = require('libs/event')
local miscUtil = require('libs/misc')
local timer = require('timer')
local utils = require('utils')
local logStub = require('stub/log')
local pushStub = require('stub/push')
local gmsStub = require('stub/gms')
local user = agent.user
local dict = agent.dict
local cdlist = agent.cdlist
local bag = agent.bag
local map = agent.map

local miscInfo = {
    -- login infos
    loginLogs = {},

    -- player settings
    settings = {},

    -- bug patch
    patch = {},
}

function miscInfo:new(o)
    o = o or {}
    if o.loginLogs == nil then o.loginLogs = {} end
    if o.settings == nil then o.settings = {} end
    if o.settings[p.PlayerSettingType.EQUIP_VIEW] == nil then
        o.settings[p.PlayerSettingType.EQUIP_VIEW] = true 
    else
        o.settings[p.PlayerSettingType.EQUIP_VIEW] = o.settings[p.PlayerSettingType.EQUIP_VIEW]
    end
    if o.patch == nil then o.patch = {} end
    setmetatable(o, self)
    self.__index = self
    return o
end

local misc = {
    data = miscInfo:new(),

    -- events
    evtOpenMCard = event.new(),
}

function misc.onInit()
    misc.dbLoad()

    if user.newPlayer then
    end

    if cdlist.isHour0Refresh then
        if not user.newPlayer then
            misc.refresh(false)
        end
    end
    cdlist.evtHour0Refresh:attachRaw(function()
        misc.refresh(true)
    end)
end

function misc.onAllCompInit()
end

function misc.onClose()
    misc.dbSave()
end

function misc.onSave()
    misc.dbSave()
end

function misc.onTimerUpdate(timerIndex)
end

function misc.dbLoad()
    local info = dict.get('misc.info') or {}
    misc.data = miscInfo:new(info)
end

function misc.dbSave()
    dict.set("misc.info", {
        -- login infos
        loginLogs = misc.data.loginLogs,
        -- player set
        settings = misc.data.settings,
        -- bug patch
        patch = misc.data.patch,
        })
end

function misc.finishLoginLog()
    if user and user.loginInfo.uid then
        local info = user.loginInfo
        local logout_time = timer.getTimestampCache()
        logStub.appendLogin(info.uid, info.exp, info.login_level, user.info.level, info.login_time, logout_time, info.isReconnect)

        table.insert(misc.data.loginLogs, { login_time = info.login_time, logout_time = logout_time })
        while #misc.data.loginLogs > 100 do
            table.remove(misc.data.loginLogs, 1)
        end
    end
end


function misc.refresh(sendUpdate)
    if sendUpdate then
    end
end

function misc.sendOnlineUpdate()
    local leftSeconds = misc.data.onlineExpired - timer.getTimestampCache()
    if leftSeconds < 0 then leftSeconds = 0 end
    agent.sendPktout(p.SC_MISC_ONLINE_UPDATE,  '@@1=i,2=i,3=i,4=i',  misc.data.onlineStep, leftSeconds, misc.data.onlineItemId, misc.data.onlineItemCount)
    -- print('sendOnlineUpdate onlineStep, leftSeconds, onlineItemId, onlineItemCount =', misc.data.onlineStep, leftSeconds, misc.data.onlineItemId, misc.data.onlineItemCount)
end

function misc.sendPlayerSetUpdate()
    local list = {}
    for k, v in pairs(gmsStub.uiSwitches) do
        table.insert(list, {type = k, switch=v})
    end
    for k, v in pairs(misc.data.settings) do
        table.insert(list, {type = k, switch=v})
    end
    table.insert(list, {type = p.PlayerSettingType.RANKING, switch = user.info.isJoinRank})
    agent.sendPktout(p.SC_PLAYER_SET_UPDATE, '@@1=[type=i,switch=b]', list)
end

function misc.sendTurnplateDataUpdate()
    local turnplateConsume = turnplate.count or 1000
    local refreshGold = turnplate.baseGold * (1 + misc.data.turnplateGoldRefresh * misc.data.turnplateGoldRefresh)
    agent.sendPktout(p.SC_MISC_TURNPLATE_DATA_UPDATE, '@@1=i,2=i,3=i,4=i,5=i', misc.data.turnplateFreeCount, misc.data.turnplateCoinCount, turnplateConsume, misc.data.turnplateMul, refreshGold)
    -- print('###misc.sendTurnplateDataUpdate...turnplateFreeCount, turnplateCoinCount, turnplateConsume, turnplateMul, refreshGold', misc.data.turnplateFreeCount, misc.data.turnplateCoinCount, turnplateConsume, misc.data.turnplateMul, refreshGold)
end

function misc.cs_misc_draw_online(session)
    local function drawOnlineResponse(result, tplId, count)
        --print("misc drawOnlineResponse", result,  tplId, count)
        agent.replyPktout(session, p.SC_MISC_DRAW_ONLINE_RESPONSE, result, tplId, count)
    end
    if misc.data.onlineStep == 0 then
        print("onlineStep=", misc.data.onlineStep)
        drawOnlineResponse(false, 0, 0)
        return
    end
    if misc.data.onlineExpired > timer.getTimestampCache() then
        print("onlineExpired > now")
        drawOnlineResponse(false, 0, 0)
        return
    end
    if misc.data.onlineItemId == 0 then
        print("misc.data.onlineItemId = 0")
        drawOnlineResponse(false, 0, 0)
        return
    end

    local tplId = misc.data.onlineItemId
    local count = misc.data.onlineItemCount
    agent.bag.addItem(tplId, count, p.ResourceGainType.ONLINE_REWARD)
    -- log
    logStub.appendOnlineReward(user.uid, onlineTime[misc.data.onlineStep], timer.getTimestampCache())

    misc.data.onlineItemId = 0
    misc.data.onlineItemCount = 0
    misc.data.onlineStep = misc.data.onlineStep + 1
    local seconds = onlineTime[misc.data.onlineStep]
    if not seconds then
        misc.data.onlineStep = 0
        misc.data.onlineItemList = {}
    else
        misc.data.onlineExpired = timer.getTimestampCache() + seconds
        -- push
        pushStub.appendPush(user.uid, p.PushClassify.ONLINE_REWARD, 0, 0, misc.data.onlineExpired)
    end
    misc.evtOnlineDraw:trigger()

    misc.sendOnlineUpdate()
    drawOnlineResponse(true, tplId, count)
end

function misc.cs_player_set(list)
    local syncToMs = false
    local settings = misc.data.settings
    for type, switch in pairs(list) do
        if type == p.PlayerSettingType.EQUIP_VIEW then
            settings[type] = switch 
            syncToMs = true
        elseif type == p.PlayerSettingType.RANKING then
            user.info.isJoinRank = switch
        end
    end

    misc.sendPlayerSetUpdate()
    if syncToMs then
        map.syncPlayerSettings()
    end
end

function misc.cs_misc_draw_code(vcode, session)
    print('CS_MISC_DRAW_CODE')
    local function drawCodeResponse(result, dropList)
        print("misc drawCodeResponse", result)
        agent.replyPktout(session, p.SC_MISC_DRAW_CODE_RESPONSE, '@@1=i,2=[tplId=i,count=i]', result, dropList)
    end
    agent.queueJob(function()
        local url = 'http://' .. t.miscConf.hubSite.ip .. ':' .. tostring(t.miscConf.hubSite.port) .. '/gms_api.php'
        local urlParams = {}
        urlParams.api = 'activation'
        urlParams.serverId = utils.getMapServiceId()
        urlParams.uid = user.uid
        urlParams.code = vcode

        local result = 1
        local dropList = {}

        local code, obj = http.getForObject(url, urlParams)
        if code == 200 and obj ~= nil then
            result = obj.rc
            if result == 0 then
                local items = miscUtil.deserialize(obj.msg, false)
                if items then
                    for tplId,count in pairs(items) do
                        print('################################################', tplId, count)
                        table.insert(dropList, t.createDropItem(tplId, count))
                    end
                    agent.bag.pickDropItems(dropList, p.ResourceGainType.ACTIVATION_CODE)
                end
            end
        else
            utils.log('activation fail :  http code =' .. tostring(code) .. ' params= ' .. utils.toJson(urlParams))
        end

        drawCodeResponse(result, dropList)
    end)
end

return misc
