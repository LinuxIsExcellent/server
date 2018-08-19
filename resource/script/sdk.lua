local utils = require('utils')
local http = require('http')
local misc = require('libs/misc')
local t = require('tploader')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()

local sdk = {
    list = {},
    tsdkHost = 'http://tsdk-test.aok.xianyugame.com',
    tsdkAppId = 6,
    tsdkAppKey = 'faAfeMVeiAgcAFe',

    verify_handlers = {},
    report_handlers = {},
}

function sdk.init()
    local conf = t.miscConf.sdk
    
    local list = ''
    for i,v in ipairs(conf.list) do
        list = list .. v .. ','
    end
    print('sdk.list=', list)
    print('sdk.tsdkHost=', conf.tsdkHost)
    --print('sdk.tsdkAppId=', conf.tsdkAppId)
    --print('sdk.tsdkAppKey=', conf.tsdkAppKey)

    sdk.list = conf.list
    sdk.tsdkHost = conf.tsdkHost
    sdk.tsdkAppId = conf.tsdkAppId
    sdk.tsdkAppKey = conf.tsdkAppKey

    -- mapping
    sdk.verify_handlers['client_test'] = sdk.verify_test
    sdk.verify_handlers['test'] = sdk.verify_test
    sdk.verify_handlers['tsdk'] = sdk.verify_tsdk

    sdk.report_handlers['tsdk'] = sdk.report_tsdk
end

function sdk.isValid(sdkType)
    for k,v in pairs(sdk.list) do
        if v == '*' or v == sdkType then
            return true
        end
    end
    return false
end

function sdk.createSignature(secret, params)
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

function sdk.verify(callback, sdkType, params)
    local func = sdk.verify_handlers[sdkType]
    if func then
        func(callback, params)
    else
        print("---- sdkType ------- ", sdkType)
        callback(false)
    end
end

function sdk.report(sdkType, params)
    local func = sdk.report_handlers[sdkType]
    if func then
        func(params)
    end
end


-----------    SDK  VERIFY    ------------

function sdk.verify_test(callback, params)
    callback(true)
end

function sdk.verify_tsdk(callback, params)
    --print('### sdk.verify_tsdk')
    local url = sdk.tsdkHost .. '/api/verifySession'
    params.appId = sdk.tsdkAppId
    params.signature = sdk.createSignature(sdk.tsdkAppKey, params)

    local code, obj = http.postForObject(url, params)
    local ok = false
    if code == 200 then
        if obj.rc == 0 then
            ok = true
        else
            utils.log('sdk.verify_tsdk fail :  rc, message, openId, session =' .. tostring(obj.rc) .. ' ' .. obj.message .. ' ' .. params.openId .. ' ' .. params.session)
        end
    else
        utils.log('sdk.verify_tsdk fail :  http code =' .. tostring(code))
    end

    callback(ok)
end

function sdk.report_tsdk(params)
    --print('### sdk.report_tsdk')
    local url = sdk.tsdkHost .. '/api/reportGameData'
    params.appId = sdk.tsdkAppId
    params.signature = sdk.createSignature(sdk.tsdkAppKey, params)

    local code, obj = http.postForObject(url, params)
    if code == 200 then
        if obj.rc ~= 0 then
            utils.log('sdk.report_tsdk fail :  obj =' .. utils.toJson(obj))
        end
    else
        utils.log('sdk.report_tsdk fail :  http code =' .. tostring(code))
    end
end


return sdk
