-- agent LuaVM启动时执行一次
local bootstrapCallback = ...

local timer = require('timer')
local vm = require('vm')
local utils = require('utils')

print ('[cs] LuaVM bootstrap, resDir=', utils.getResourceDir())

-- 缓存代码
local codeDir = utils.getResourceDir() .. '/script'
local files = utils.listFilesInDirectory(codeDir, 3)
for _, file in ipairs(files) do
    local shortName = string.match(file, '.*script/([^%.]*%.lua)')
    if shortName ~= nil then
        if shortName ~= 'bootstrap.lua'
            and shortName ~= 'test.lua' then
            vm.cacheScript(shortName, file)
            print('cache script', shortName, file)
        end
    end
end

local codeDir = utils.getResourceDir() .. '/cs-script'
local files = utils.listFilesInDirectory(codeDir, 3)
for _, file in ipairs(files) do
    local shortName = string.match(file, '.*cs%-script/([^%.]*%.lua)')
    if shortName ~= nil then
        if shortName ~= 'bootstrap.lua'
            and shortName ~= 'test.lua' then
            vm.cacheScript(shortName, file)
            print('cache script', shortName, file)
        end
    end
end


-- 优先从代码缓存中搜索
table.insert(package.searchers, 1, vm.searcher)

require('libs/global')

local thread = coroutine.create(function()
    -- 加载服务器模板
    local tploader = require('tploader')
    tploader.loadAllTemplate()

    -- connect to map server
    local mapStub = require("stub/mapStub")
    local id = utils.getMapServiceId()
    local name = 'map.' .. id .. '@ms'
    mapStub.connectService(name)
    -- 添加需要启动的组件
    require('idService').start()

    require('loginService').start()

    require('dataService').start()

    require('logService').start()

    require('chatService').start()

    require('arenaService').start()

    require('allianceService').start(mapStub)

    require('palaceWarService').start(mapStub)

    require('rangeService').start()

    require('pushService').start()

    require('gmsService').start()

    require('httpService').start()

    require('achievementService').start(mapStub)

    require('activityService').start(mapStub)

    require('reportService').start(mapStub)

    require('marketService').start(mapStub)    

    bootstrapCallback(true)

    mapStub:onBootstrapFinish()

    collectgarbage()
end)

local ok, err = coroutine.resume(thread)
if not ok then
    bootstrapCallback(false)
    print(err)
end

tm = timer.setInterval(function() collectgarbage('collect') end, 600 * 1000)

local disableSetGlobal = {
    __newindex = function(t, k, v)
        error('not allow to set global variable ' .. k)
    end
}

setmetatable(_G, disableSetGlobal)


