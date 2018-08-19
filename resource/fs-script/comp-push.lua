local agent = ...
local p = require('protocol')
local t = require('tploader')
local event = require('libs/event')
local pushStub = require('stub/push')

local user = agent.user
local dict = agent.dict

local pktHandlers = {}

local push = {
    data = {
        langType = p.LangType.CN,
        channel = '',
        device = '',
        deviceToken = '',
        settings = {}
    },
}

function push.dbLoad()
    push.data = dict.get("push.data") or {}

    push.data.device = push.data.device or ''
    push.data.deviceToken = push.data.deviceToken or ''
    push.data.settings = push.data.settings or {}
    for k,v in pairs(p.PushGroup) do
        if push.data.settings[v] == nil then
            push.data.settings[v] = true
        end
    end
end

function push.dbSave()
    dict.set("push.data", push.data)
end

function push.onSave()
    push.dbSave()
end

function push.onInit()
    agent.registerHandlers(pktHandlers)

    push.dbLoad()
end

function push.onAllCompInit()
    push.data.langType = user.info.langType
    push.data.channel = user.info.channel
    push.sendPushSettingUpdate()
end

function push.onClose()
    push.dbSave()
end

function push.sendPushSettingUpdate()
    --print('sendPushSettingUpdate')
    local list = {}
    for k, v in pairs(push.data.settings) do
        table.insert(list, { group = k, isPush = v })
    end
    agent.sendPktout(p.SC_PUSH_SETTING_UPDATE, '@@1=[group=i,isPush=b]', list)
end


pktHandlers[p.CS_PUSH_SETTING_SET] = function(pktin)
    local group, isPush = pktin:read('ib')
    print('PUSH_SETTING_SET', group, isPush)

    -- push switch
    if group == 0 then
        agent.isPush = isPush
        pushStub.syncSwitch(user.uid, isPush)
        return
    end

    if push.data.settings[group] == nil then
        return
    end
    push.data.settings[group] = isPush
    push.sendPushSettingUpdate()
    -- sync to cs
    pushStub.syncData(user.uid, push.data)
end

pktHandlers[p.CS_PUSH_DEVICE_SYNC] = function(pktin)
    local device, deviceToken = pktin:read('ss')
    print('PUSH_DEVICE_SYNC', device, deviceToken)
    push.data.device = string.lower(device)
    push.data.deviceToken = deviceToken

    push.data.langType = user.info.langType
    push.data.channel = user.info.channel
    -- sync to cs
    pushStub.syncData(user.uid, push.data)
end

return push
