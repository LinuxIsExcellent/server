local cluster = require('cluster')

local idStub = {
    rawStub,
}

local subscriber = {
}

local rawStub

function idStub.connectService()
    rawStub = cluster.connectService('id@cs')
    idStub.rawStub = rawStub
    rawStub:setSubscriber(subscriber)
end

function idStub.createUid()
    local rpcOk, uid =  rawStub:call_createUid()
    -- print("front server,idStub.createUid   uid is ", uid)
    if not rpcOk then
        return 0
    end
    return uid
end

function idStub.createAid()
    local rpcOk, aid =  rawStub:call_createAid()
    if not rpcOk then
        return 0
    end
    return aid
end

return idStub
