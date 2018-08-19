-- agent LuaV启动时执行一次
local bootstrapCallback = ...

local timer = require('timer')
local vm = require('vm')
local utils = require('utils')

print ('LuaVM bootstrap, resDir=', utils.getResourceDir())

-- 缓存代码
local codeDir = utils.getResourceDir() .. '/script'
local files = utils.listFilesInDirectory(codeDir, 3)
for _, file in ipairs(files) do
    local shortName = string.match(file, '.*script/([^%.]*%.lua)')
    if shortName ~= nil then
        if shortName ~= 'bootstrap.lua'
            and shortName ~= 'test.lua' then
            vm.cacheScript(shortName, file)
            -- print('cache script', shortName, file)
        end
    end
end

local codeDir = utils.getResourceDir() .. '/fs-script'
local files = utils.listFilesInDirectory(codeDir, 3)
for _, file in ipairs(files) do
    local shortName = string.match(file, '.*fs%-script/([^%.]*%.lua)')
    if shortName ~= nil then
        if shortName ~= 'bootstrap.lua'
            and shortName ~= 'test.lua' then
            vm.cacheScript(shortName, file)
        end
    end
end

-- 优先从代码缓存中搜索
table.insert(package.searchers, 1, vm.searcher)

require('libs/global')

local thread = coroutine.create(function()
    -- 加载客户端模板
    local clientTpl = require('clientTpl')
    clientTpl.loadAllTemplate()

    -- 加载服务器模板
    local tploader = require('tploader')
    tploader.loadAllTemplate()

    local loginStub = require('stub/login')
    loginStub.connectService()

    require('sdk').init()

    require('stub/id').connectService()

    require('stub/data').connectService()

    require('stub/chat').connectService()

    require('stub/arena').connectService()

    require('stub/alliance').connectService()

    require('stub/range').connectService()

    require('stub/palaceWar').connectService()

    require('stub/gms').connectService()

    require('stub/log').connectService()

    require('stub/push').connectService()

    require('stub/achievement').connectService()

    require('stub/activity').connectService()

    require('stub/report').connectService()

    bootstrapCallback(true)

    collectgarbage()
end)

tm = timer.setInterval(function()
    --debugHook()
    --var_dump(utils.fromJson(utils.getObjectCountsJson()))
    collectgarbage('collect')
end, 10 * 60 * 1000)

--[[
weak = {}
setmetatable(weak, { __mode = 'k' })
tm = timer.setInterval(function()
    print('weak begin')
    collectgarbage('collect')
    for k, v in pairs(weak) do
        print('weak', k, v)
    end
    print('weak end')
end, 5 * 1000)
]]

local ok, err = coroutine.resume(thread)
if not ok then
    bootstrapCallback(false)
    print(err)
end

local disableSetGlobal = {
    __newindex = function(t, k, v)
        error('not allow to set global variable ' .. k)
    end
}

setmetatable(_G, disableSetGlobal)

