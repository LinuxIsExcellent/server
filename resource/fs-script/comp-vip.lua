local agent = ...
local p = require('protocol')
local t = require('tploader')
local event = require('libs/event')
local timer = require('timer')
local utils = require('utils')
local logStub = require('stub/log')
local user = agent.user
local dict = agent.dict

local vip = {
    level = 0,
    exp = 0,
    takeBox = 0,
    tpl = nil,
    -- events
    evtUpgrade = event.new(), --()
}


--local pktHandlers = {}

function vip.onInit()
    --agent.registerHandlers(pktHandlers)
    vip.dbLoad()
end

function vip.onAllCompInit()
    -- vip.addExp(1000,false)
    vip.sendVipUpdate()
end

function vip.onClose()
    vip.dbSave()
end

function vip.onSave()
    vip.dbSave()
end

function vip.dbLoad()
    local info = dict.get('vip.info') or {}
    vip.level = info.level or 0
    vip.exp = info.exp or 0
    vip.takeBox = info.takeBox or 0
    vip.tpl = t.vip[vip.level]
end

function vip.dbSave()
    dict.set("vip.info", {
    	level = vip.level,
    	exp = vip.exp,
        takeBox = vip.takeBox
    	})
end

function vip.isVip()
    return vip.level > 0
end

function vip.vipLevel()
    return vip.level
end

function vip.addExp(exp, sendUpdate)
    exp = math.ceil(exp)
    local oldLevel = vip.level
    local oldExp = vip.exp
    local total = vip.exp

    if vip.tpl and vip.tpl.maxExp > 0 then
        total = vip.exp + exp
        while total >= vip.tpl.maxExp do
            local level = vip.level + 1
            local tpl = t.vip[level]
            if tpl.maxExp <= 0 then
                total = vip.tpl.maxExp
                vip.level = level
                vip.tpl = tpl
                break
            end
            vip.level = level
            vip.tpl = tpl
            user.setVipLevel(vip.level)
        end
    end

    if oldExp ~= total then
        vip.exp = total

        if sendUpdate then
            vip.sendVipUpdate()
        end
    end

    if oldLevel ~= vip.level then
        vip.evtUpgrade:trigger()
        -- log
        logStub.appendVipUpgrade(user.uid, oldLevel, vip.level, timer.getTimestampCache())
    end
end

function vip.sendVipUpdate()
    agent.sendPktout(p.SC_VIP_UPDATE,  '@@1=i,2=i,3=i',  vip.level, vip.exp, vip.takeBox)
    -- print('sendVipUpdate level=============', vip.level, vip.exp, vip.takeBox)
end

function vip.isTakeBox( level )
    return ((vip.takeBox >> level) & 1) == 1
end

function vip.canTakeBox( level )
    if level > 0 and level <= vip.level then
        if not vip.isTakeBox(level) then
            return true
        end
    end
    return false
end

function vip.setTakeBox( level )
    if level > 0 and level <= vip.level then
        vip.takeBox = (vip.takeBox | (1 << level))
    end
end

-- pktHandlers[p.CS_VIP_TAKE_BOX] = function(pktin, session)
function vip.cs_vip_take_box(level) 
    local function takeBoxResponse(ok)
        agent.replyPktout(session, p.SC_VIP_TAKE_BOX_RESPONSE, '@@1=b', ok)
    end

    --print("CS_VIP_TAKE_BOX level ", level)
    if vip.canTakeBox(level) then
        local tpl = t.vip[level]
        if user.removeResource(p.ResourceType.GOLD, tpl.boxPrice, p.ResourceConsumeType.VIP_BOX) then
            agent.bag.pickDropItemsByDropId(tpl.boxId, p.ResourceGainType.VIP_BOX)
            vip.setTakeBox(level)
            takeBoxResponse(true)
            print("CS_VIP_TAKE_BOX takeBoxResponse(true)")
            vip.sendVipUpdate()
        else
            takeBoxResponse(false)
        end
    else
        print("CS_VIP_TAKE_BOX takeBoxResponse(false)")
        takeBoxResponse(false)
    end
end

return vip
