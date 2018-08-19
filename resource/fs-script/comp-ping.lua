local agent = ...
local p = require('protocol')
local t = require('tploader')
local timer = require('timer')
local utils = require('utils')
local event = require('libs/event')
local pushStub = require('stub/push')

local ping = {
    delay = 0,
    lastReplyTime = timer.getTimestampCache(),

    evtOnlineTime = event.new(),  --(seconds)
}

local pingInfo = {
    sequence = 0,
    lastTick = 0,
}
local interval = t.configure["HeartbeatInterval"] or 50

local pktHandlers = {}

function ping.onInit()
    agent.registerHandlers(pktHandlers)
end

function ping.onAllCompInit()
end

function ping.onClose()
end

function ping.onTimerUpdate(timerIndex)
    -- ping client
    if timerIndex % interval == 0 then
        pingInfo.sequence = pingInfo.sequence + 1
        pingInfo.lastTick = timer.getTickCache()
        agent.sendPktout(p.SC_PING, pingInfo.sequence)
        -- utils.log(string.format('ping username=%s, nickname=%s, sequence=%i', agent.user.info.username, agent.user.info.nickname, pingInfo.sequence))
        local ts = timer.getTimestampCache() - ping.lastReplyTime
        if ts >= 2 * interval then
            agent.isPush = true
            pushStub.syncSwitch(agent.user.uid, true)
        end
    end

    -- check auto kick
    local autoKickTime = t.configure["autoKickTime"] or 300
    if timer.getTimestampCache() - ping.lastReplyTime > autoKickTime then
        -- utils.log(string.format('auto_kick username=%s, nickname=%s', agent.user.info.username, agent.user.info.nickname))
        agent.sendKick(p.KickType.HEART_BEAT_REPLY)
        -- agent.exit()
    end

    if timerIndex % 60 == 0 then
        ping.evtOnlineTime:trigger(60)
    end
end


pktHandlers[p.CS_PING_REPLY] = function(pktin)
    local replaySequence = pktin:readInteger()
    if replaySequence == 0 then
        return
    end

    if replaySequence == pingInfo.sequence then
        ping.delay = timer.getTickCache() - pingInfo.lastTick
        agent.sendPktout(p.SC_PING_RESULT, ping.delay)
        -- utils.log(string.format('ping_reply username=%s, nickname=%s, replaySequence=%i', agent.user.info.username, agent.user.info.nickname, replaySequence))
    else
        -- network delay is high, notice client to know
        -- utils.log(string.format('delay_high username=%s, nickname=%s', agent.user.info.username, agent.user.info.nickname))
    end

    ping.lastReplyTime = timer.getTimestampCache()
end

return ping

