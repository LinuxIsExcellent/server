local p = require('protocol')
local t = require('tploader')
local dbo = require('dbo')
local vm = require('vm')
local cluster = require('cluster')
local timer = require('timer')
local utils = require('utils')
local logStub = require('stub/log')
local loginStub = require('stub/login')
local pushStub = require('stub/push')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()

local rawAgent = ...
local agent = {
    timer = nil,
    isExit = false,
    isInitCompleted = false,
    isPush = false
}
local allPktHandlers = {}
local waitPktinThreads = {}
local timerIndex = utils.getRandomNum(1, 600)
local timerUpdateList = {}

local agentMT = {
    __gc = function()
        print('##agent.__gc')
    end
}
setmetatable(agent, agentMT)

print('current luavm memory useage:', collectgarbage('count') * 1024, ' g_object_count =', utils.getGObjectCount())

local mailboxHandler = function(method, ...)
    if method == 'kick' then
        local kickType = ...
        agent.sendKick(kickType)
    elseif agent.isInitCompleted then
        if method == 'mail_reload' then
            local mid = ...
            agent.mail.reloadMailById(mid)
        elseif method == 'arena_record_reload' then
            local recordId = ...
            agent.arena.reloadRecordById(recordId)

        elseif method == 'transport_record_reload' then
            local recordId = ...
            agent.market.reloadRecordById(recordId)

		elseif method == 'lock' then
			local lockedTimestamp, lockedReason = ...
			local kickType = p.KickType.ACCOUNT_LOCKED
			agent.sendKick(kickType)
		elseif method == 'ban_chat' then
			local banChatTimestamp, banChatReason = ...
			agent.user.info.banChatTimestamp = banChatTimestamp
			agent.user.info.banChatReason = banChatReason
			agent.user.sendUpdate()
		 elseif method == 'charge' then
		     local orderId = ...
		     agent.charge.loadCharge(orderId)
        elseif method == 'update_title' then
             local tplid = ...
             agent.palaceWar.onUpdateTitle(tplid)
        elseif method == 'notice_message' then
            local id, type, param = ...
            agent.sendPktout(p.SC_NOTICE_MESSAGE, id, param, type)
        -- elseif method == 'alliance_multi_explore_update' then
        --     -- print('alliance_multi_explore_update', ...)
        --     agent.alliance.sendAllianceMultiExploreUpdate(...)
        else
            utils.log(string.format('agent unhandled agent method=%s', method))
        end
    end
end
agent.mailbox = cluster.createMailbox(mailboxHandler)

loginStub.addUser(rawAgent, agent)

local compArr = {
    'init',
    'ping',
    'user',
    'dict',
    'cdlist',
    'buff',
    'vip',
    'bag',
    'hero',
    'army',
    'technology',
    'scenarioCopy',
    'building',
    'mail',
    'alliance',
    'palaceWar',
    'arena',
    'babel',
    'bronzeSparrowTower',
    'ridingAlone',
    'map',
    'property',
    'chat',
    'store',
    'range',
    'activity',
    'gms',
    -- 'push',
    'charge',
    'misc',
    'quest',
    'dailyTask',
    'achievement',
    'combat',
    'systemUnlock',
    'report',
    'novice',
    'market',
    'gm'
}

function agent.queueErrorFun()
    print("queueErorFun ---------------")
    print(debug.traceback())
    agent.sendKick(p.KickType.FOUND_ERROR)
end

function agent.queueJob(threadFun)
    jobExecutor:queue(threadFun, agent.queueErrorFun)
end

function agent.registerHandlers(pktHandlers)
    for code, hd in pairs(pktHandlers) do
        assert(allPktHandlers[code] == nil)
        allPktHandlers[code] = hd
    end
end

function agent.isTrust()
    return rawAgent:isTrust()
end

function agent.setTrust()
    rawAgent:setTrust()
end

function agent.sendPktout(...)
    return rawAgent:sendPktout(...)
end

function agent.send(pktout)
    rawAgent:send(pktout)
end

function agent.replyPktout(...)
    rawAgent:replyPktout(...)
end

function agent.newPktout(code, size, session)
    --TODO 返回有可能为nil
    return rawAgent:newPktout(code, size, session)
end

function agent.sendKick(kickType)
    agent.sendPktout(p.SC_LOGOUT, kickType)
    if kickType ~= p.KickType.MAINTENANCE then
        utils.debug(string.format('kick type=%i, username=%s, nickname=%s', kickType, agent.user.info.username, agent.user.info.nickname))
    end
    agent.exitTimer = timer.setTimeout(agent.exit, 500)
end

function agent.exit()
    --print('agent.exit : isExit', agent.isExit)
    if not agent.isExit then
        agent.isExit = true
        rawAgent:exit()
    end
end

function agent.connectMapService(uid)
    return rawAgent:connectMapService(uid)
end

function agent.connectMapServiceEx(kid)
    return rawAgent:connectMapServiceEx(kid)
end

--battle
function agent.startCombat(battleData)
    return rawAgent:startCombat(battleData)
end

function agent.startHeroCombat(battleData)
    return rawAgent:startHeroCombat(battleData)
end

function agent.waitPktin(code, callback)
    assert(waitPktinThreads[code] == nil)
    waitPktinThreads[code] = coroutine.running()
    return coroutine.yield()
end

function agent.onUpdate()
    timerIndex = timerIndex + 1
    if timerIndex % 1800 == 0 then
        agent.onTimeSync()
    end
    -- check onTimerUpdate
    for i,timerUpdate in ipairs(timerUpdateList) do
        timerUpdate(timerIndex)
    end
    -- auto save all data
    if timerIndex % loginStub.autoSaveInterval == 0 then
        agent.onSave(true)
        --print('auto save all data, interval=', loginStub.autoSaveInterval)
    end
end

function agent.onSave(timingSave)
    agent.queueJob(function()
        for i = #compArr, 1, -1 do
            local c = agent[compArr[i]]
            if c.onSave ~= nil then
                c.onSave(timingSave)
            --print('  comp', compArr[i], 'onSave')
            end
        end
    end)
end

function agent.onTimeSync()
    agent.queueJob(function()
        local time = timer.getTimestampCache()
        local zone = timer.getZone()
        local isdst = timer.getIsdst()
        --print('onTimeSync..... time, zone, isdst', time, zone, isdst)
        agent.sendPktout(p.SC_TIME_SYNC, time, zone, isdst)
    end)
end

function agent.onCrossTeleport()
    local i = #compArr
    for i = #compArr, 1, -1 do
        local c = agent[compArr[i]]
        if c.onSave ~= nil then
            c.onSave()
        end
    end
end

function agent.sendNoticeMessage(code, param1, type)
    -- print('sendNoticeMessage..... code, param1, type', code, param1, type)
    agent.sendPktout(p.SC_NOTICE_MESSAGE, code, param, type)
end

-- event handler
local evtHandler = {}
local lockPktin = {}

local function handlePktin(pktin)
    local code = pktin:code()
    local session = pktin:session()
    -- print("#evtHandler.onReceive = ", code, session)
    do
        local co = waitPktinThreads[code]
        if co ~= nil then
            coroutine.resume(co, pktin, session)
            waitPktinThreads[code] = nil
            return
        end
    end
    do
        if agent.isExit or loginStub.csShutdown then
            return
        end
        local hd = allPktHandlers[code]
        if hd ~= nil then
            if code ~= 1 and not agent.isTrust() then
                -- print('agent.isLogined, code =', agent.isLogined, code)
                -- it may be attacked, kick this connecttion
                agent.exit()
            else
                -- make sure pushing is closed while we have pktin to handle with 
                if agent.isPush then
                    -- close push
                    agent.isPush = false
                    pushStub.syncSwitch(agent.user.uid, false)
                end
                -- put this after pushing close checking because this pktin may be the one to open pushing
                hd(pktin, session)
            end
        else
            if agent.isTrust() then
                utils.log(string.format('unhandled packet received, code=%i', code))
            end
            -- it must be attacked, kick this connecttion
            if not p.CS_MAP[code] then
                agent.exit()
            end
        end
    end
end

function evtHandler.onReceive(pktin)
    handlePktin(pktin)
end

function evtHandler.onClose()
    --print('#evtHandlers.onClose')
    agent.isExit = true    --防止comp继续加载
    if agent.timer then
        agent.timer:cancel()
        agent.timer = nil
    end

    if not agent.isInitCompleted then
        loginStub.removeUser(rawAgent)
        return
    end

    agent.queueJob(function()
        -- save login info
        if agent.misc then
            agent.misc.finishLoginLog()
        end
        -- open push
        pushStub.syncSwitch(agent.user.uid, true)

        local i = #compArr
        for i = #compArr, 1, -1 do
            local c = agent[compArr[i]]
            if c.onClose ~= nil then
                c.onClose()
                --print('  comp', compArr[i], 'closed')
            end
        end
        loginStub.removeUser(rawAgent)
        -- print('agent close complete')
    end, function ()
        loginStub.removeUser(rawAgent)
    end)
end

function evtHandler.checkIsAllFinish()
    return (not agent.isInitCompleted) or (not jobExecutor:isBusy())
end

function evtHandler.onKick()
    --print('#evtHandlers.onKick')
    agent.sendPktout(p.SC_LOGOUT, p.KickType.MAINTENANCE) 
end

rawAgent:setEvtHandler(evtHandler)

-- init component
agent.queueJob(function() 
    for _, name in ipairs(compArr) do
        agent[name] = nil
    end
    -- load comps
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_activity.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_babel.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_cdlist.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_charge.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_chat.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_combat.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_gm.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_mail.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_misc.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_novice.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_user.lua', agent))

    agent.registerHandlers(vm.executeCachedScript('protocol/oper_vip.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_store.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_ridingAlone.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_report.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_quest.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_dailyTask.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_achievement.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_palaceWar.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_hero.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_army.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_arena.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_market.lua', agent))
    agent.registerHandlers(vm.executeCachedScript('protocol/oper_bag.lua', agent))
    
    print("load comps")
    for _, name in ipairs(compArr) do
        local c = vm.executeCachedScript('comp-' .. name .. '.lua', agent)
        agent[name] = c
    end
    -- onInit
    print("onInit")
    for _, name in ipairs(compArr) do
        if agent.isExit or loginStub.csShutdown then
            loginStub.removeUser(rawAgent)
            return
        end
        local c = agent[name]
        if c.onInit ~= nil then
            c.onInit(agent)
            --print('  comp', name, 'inited')
        end
    end
    -- onAllCompInit
    print("onAllCompInit")
    for _, name in ipairs(compArr) do
        if agent.isExit or loginStub.csShutdown then
            loginStub.removeUser(rawAgent)
            return
        end
        local c = agent[name]
        if c.onAllCompInit ~= nil then
            c.onAllCompInit(agent)
            --print('  comp', name, 'onAllCompInit')
        end
    end
    agent.isInitCompleted = true
    -- onReady
    for _, name in ipairs(compArr) do
        local c = agent[name]
        if c.onReady ~= nil then
            c.onReady()
            --print('   comp', name, 'onReady')
        end
    end
    -- onTimerUpdate
    for _, name in ipairs(compArr) do
        local c = agent[name]
        if c.onTimerUpdate ~= nil then
            table.insert(timerUpdateList, c.onTimerUpdate)
        end
    end
    
    agent.isPush = false
    pushStub.syncSwitch(agent.user.uid, false)
    agent.onTimeSync()
    agent.timer = timer.setInterval(agent.onUpdate, 1000)
    --protocol
    
    -- 所有模块加载完之后给前端一个提示
    agent.sendPktout(p.SC_AGENT_INIT_COMPLETE)
    -- print('agent init complete')
end)

