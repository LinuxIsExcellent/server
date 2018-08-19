local p = require('protocol')
local cluster = require('cluster')

local pushStub = {
    rawStub,
}

local subscriber = {
}

local rawStub

function pushStub.connectService()
    rawStub = cluster.connectService('push@cs')
    pushStub.rawStub = rawStub
    rawStub:setSubscriber(subscriber)
end

function pushStub.appendPush(uid, classify, tag, cdId, sendTime)
    rawStub:cast_appendPush({  uid = uid, classify = classify, tag = tag, cdId = cdId, sendTime = sendTime })
end

function pushStub.appendPushList(uidList, classify, tag, cdId, sendTime)
    local list = {}
    for k,v in pairs(uidList) do
        table.insert(list, {  uid = v, classify = classify, tag = tag, cdId = cdId, sendTime = sendTime })
    end
    rawStub:cast_appendPushList(list)
end

function pushStub.cancelPush(uid, cdId)
    rawStub:cast_cancelPush(uid, cdId)
end

function pushStub.speedupPush(uid, cdId, sendTime)
    rawStub:cast_speedupPush(uid, cdId, sendTime)
end

function pushStub.syncData(uid, data)
    rawStub:cast_syncData(uid, data)
end

function pushStub.syncSwitch(uid, switch)
    rawStub:cast_syncSwitch(uid, switch)
end


return pushStub
