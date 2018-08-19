local agent = ...
local p = require('protocol')
local t = require('tploader')
local dbo = require('dbo')
local timer = require('timer')
local event = require('libs/event')
local utils = require('utils')
local misc = require('libs/misc')
local logStub = require('stub/log')

local user = agent.user
local bag = agent.bag
local dict = agent.dict
local cdlist = agent.cdlist
local hero = agent.hero

local babelRelated = t.configure['babelRelated']

local babel = {
    layer = 0,              --层数
    historylayer = 0,       --历史最高层
    historyTime = 0,        --最高层千层楼通关时间
    leftFreeCouunt = 0,     --当天剩余免费次数
    buyResetCouunt = 0,     --当天已购买重置次数
    babelScore = 0,         --千层楼积分
    layerPassList = {},     --千层楼通关层 [layer]=1

    evtPassBabel = event.new(),  --通关(layer)
}

function babel.onInit()
    babel.dbLoad()

    if cdlist.isHour5Refresh then
        babel.refresh(false)
    end
    cdlist.evtHour5Refresh:attachRaw(function()
        babel.refresh(true)
    end)

    agent.combat.evtCombatOver:attachRaw(babel.onCombatOver)
end

function babel.onAllCompInit()
    babel.sendBabelDataUpdate()
end

function babel.onSave()
    babel.dbSave()
end

function babel.onClose()
    babel.dbSave()
end

function babel.onTimerUpdate()
end

function babel.dbLoad()
    local data = dict.get("babel.data") or {}
    babel.layer = data.layer or babel.layer
    babel.historylayer = data.historylayer or babel.historylayer
    babel.historyTime = data.historyTime or babel.historyTime
    babel.leftFreeCouunt = data.leftFreeCouunt or babel.leftFreeCouunt
    babel.buyResetCouunt = data.buyResetCouunt or babel.buyResetCouunt
    babel.babelScore = data.babelScore or babel.babelScore
    babel.layerPassList = data.layerPassList or babel.layerPassList
end

function babel.dbSave()
    dict.set('babel.data', {
        layer = babel.layer,
        historylayer = babel.historylayer,
        historyTime = babel.historyTime,
        leftFreeCouunt = babel.leftFreeCouunt,
        buyResetCouunt = babel.buyResetCouunt,
        babelScore = babel.babelScore,
        layerPassList = babel.layerPassList,
    })
end

function babel.refresh(sendUpdate)
    babel.leftFreeCouunt = babelRelated.freeCount
    babel.buyResetCouunt = 0
    if sendUpdate then
        babel.sendBabelDataUpdate()
    end
end

function babel.addBabelScore(score, gainType, sendUpdate)
    -- print('babel.addBabelScore...', score, gainType, sendUpdate)
    if gainType == nil then
        gainType = p.ResourceGainType.BABEL
    end
    if sendUpdate == nil then
        sendUpdate = true
    end
    if score > 0 then
        babel.babelScore = babel.babelScore + score
        --log
        logStub.appendBabelScore(user.uid, score, p.LogItemType.GAIN, gainType, timer.getTimestampCache())

        if sendUpdate then
            babel.sendBabelDataUpdate()
        end
    end
end

function babel.isBabelScoreEnough(score)
    return babel.babelScore >= score
end

function babel.removeBabelScore(score, consumeType, sendUpdate)
    if consumeType == nil then
        consumeType = p.ResourceConsumeType.STORE_BUY
    end
    if sendUpdate == nil then
        sendUpdate = true
    end
    if score > 0 then
        babel.babelScore = babel.babelScore - score
        if babel.babelScore < 0 then
            babel.babelScore = 0
        end
        --log
        logStub.appendBabelScore(user.uid, score, p.LogItemType.CONSUME, consumeType, timer.getTimestampCache())

        if sendUpdate then
            babel.sendBabelDataUpdate()
        end
    end
    return true
end

function babel.onCombatOver(battleId, battleType, isWin, dataOnCombatOver, result)
    if battleType == p.BattleType.BABEL_COMBAT then
        -- print('babel.onCombatOver...battleId, battleType, isWin, result, dataOnCombatOver', battleId, battleType, isWin, result, dataOnCombatOver)
        local layer = dataOnCombatOver.layer
        local firstList = {}
        local publicList = {}
        if isWin then
            local tpl = t.babel[layer]
            if tpl then
                local pickList = {}
                --普通掉落
                local publicDropTpl = t.drop[tpl.publicDropId]
                if publicDropTpl then
                    publicList = publicDropTpl:DoDrop()
                    -- print('=============publicDropId, publicList', tpl.publicDropId, utils.serialize(publicList))
                    --pick
                    for _, v in pairs(publicList) do
                        local tplId = v.tplId
                        local count = v.count
                        local pick = pickList[tplId]
                        if pick == nil then
                            pickList[tplId] = { tplId = tplId, count = count }
                        else
                            pick.count = pick.count + count
                        end
                    end
                end
                --首通掉落
                if babel.layerPassList[layer] == nil then
                    babel.layerPassList[layer] = 1
                    local firstDropTpl = t.drop[tpl.firstDropId]
                    if firstDropTpl then
                        firstList = firstDropTpl:DoDrop()
                        -- print('=============firstDropId, firstList', tpl.firstDropId, utils.serialize(firstList))
                    end
                    --pick
                    for _, v in pairs(firstList) do
                        local tplId = v.tplId
                        local count = v.count
                        local pick = pickList[tplId]
                        if pick == nil then
                            pickList[tplId] = { tplId = tplId, count = count }
                        else
                            pick.count = pick.count + count
                        end
                    end
                    -- print('babel.onCombatOver...pickList', utils.serialize(pickList))
                    bag.pickDropItems(pickList, p.ResourceGainType.BABEL)
                end
                --更新数据
                babel.layer = layer
                if layer > babel.historylayer then
                    babel.historylayer = layer
                    babel.historyTime = timer.getTimestampCache()
                end
                babel.sendBabelDataUpdate()

                babel.evtPassBabel:trigger(layer)
            end
        end

        --
        -- print('babel.onCombatOver...battleType, layer, isWin', battleType, layer, isWin)
        -- print('babel.onCombatOver...firstList', utils.serialize(firstList))
        -- print('babel.onCombatOver...publicList', utils.serialize(publicList))
        agent.sendPktout(p.SC_BABEL_BATTLE_RESULT, '@@1=i,2=i,3=b,4=[tplId=i,count=i],5=[tplId=i,count=i]', battleType, layer, isWin, firstList, publicList)
    end
end

function babel.sendBabelDataUpdate()
    -- print('babel.sendBabelDataUpdate...layer, historylayer, leftFreeCouunt, buyResetCouunt, babelScor', babel.layer, babel.historylayer, babel.leftFreeCouunt, babel.buyResetCouunt, babel.babelScore)
    agent.sendPktout(p.SC_BABEL_DATA_UPDATE, babel.layer, babel.historylayer, babel.leftFreeCouunt, babel.buyResetCouunt, babel.babelScore)
end

function babel.cs_babel_fight(armyList, session)
    local function babelFightResponse(result, battleId, warReport)
        -- print('babelFightResponse', result, battleId)
        agent.replyPktout(session, p.SC_BABEL_FIGHT_RESPONSE, result, battleId, warReport)
    end

    local battleId = 0
    local warReport 
    if not next(armyList) then
        print('p.CS_BABEL_FIGHT...armyList is empty')
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --检查该层是否存在
    local tpl = t.babel[layer]
    if tpl == nil then
        print('p.CS_BABEL_FIGHT...tpl_babel no this layer...layer', layer)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --检查该层是否可以打
    local maxLayer = #t.babel
    -- print('p.CS_BABEL_FIGHT...fightLayer, maxLayer', layer, maxLayer)
    if layer ~= babel.layer + 1 or layer > maxLayer then
        print('p.CS_BABEL_FIGHT...this layer can not fight...curLayer, fightLayer, maxLayer', babel.layer, layer, maxLayer)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    --调用战斗接口  事件处理结果
    local dataOnCombatOver = { layer = layer }
    -- 纯混战
    local extParam = {levelType = 1}
    battleId, warReport = agent.combat.createBattleByArmyList(p.BattleType.BABEL_COMBAT, armyList, tpl.armyGroup, tpl.armyCount, dataOnCombatOver, extParam)
    -- print('p.CS_BABEL_FIGHT...battleId', battleId)
    babelFightResponse(p.ErrorCode.SUCCESS, battleId, warReport)
end

function babel.cs_babel_mopup(session)
    local function babelMopupResponse(result, list)
        if list == nil then
            list = {}
        end
        -- print('p.CS_BABEL_MOPUP...result, list', result, list)
        agent.replyPktout(session, p.SC_BABEL_MOPUP_RESPONSE, '@@1=i,2=[layer=i,itemList=[tplId=i,count=i]]', result, list)
    end
    -- print('p.CS_BABEL_MOPUP...historylayer', babel.historylayer)
    local mopupCoe = babelRelated.mopupCoe or 10
    --一次最多扫荡mopupCoe层
    local momupMaxLayer = math.floor(babel.layer/mopupCoe) * mopupCoe + mopupCoe
    if momupMaxLayer <= babel.layer or momupMaxLayer > babel.historylayer then
        print('p.CS_BABEL_MOPUP...momup layer is wrong..historylayer, curLayer,momupMaxLayer',babel.historylayer, babel.layer, momupMaxLayer)
        agent.sendNoticeMessage(p.ErrorCode.BABEL_NOT_MOPUP, tostring(momupMaxLayer), 1)
        return
    end
    local list = {}
    local pickList = {}
    local tempLayer = babel.layer
    if babel.layer % mopupCoe == 0 then
        tempLayer = babel.layer + 1
    end
    -- print('p.CS_BABEL_MOPUP...', babel.layer, tempLayer)
    for i=tempLayer, momupMaxLayer, 1 do
        local tpl = t.babel[i]
        if tpl then
            local publicDropTpl = t.drop[tpl.publicDropId]
            local publicList = {}
            if publicDropTpl then
                publicList = publicDropTpl:DoDrop()
            end
            --pick
            for _, v in pairs(publicList) do
                local tplId = v.tplId
                local count = v.count
                local pick = pickList[tplId]
                if pick == nil then
                    pickList[tplId] = { tplId = tplId, count = count }
                else
                    pick.count = pick.count + count
                end
            end
            -- print('p.CS_BABEL_FIGHT...layer, publicDropId, publicList', i, tpl.publicDropId, utils.serialize(publicList))
            table.insert(list, { layer = i, itemList = publicList })
        end
    end
    -- print('p.CS_BABEL_FIGHT...pickList', utils.serialize(pickList))
    bag.pickDropItems(pickList, p.ResourceGainType.BABEL)
    -- print('p.CS_BABEL_FIGHT...list', utils.serialize(list))

    babel.layer = momupMaxLayer
    babel.sendBabelDataUpdate()

    babelMopupResponse(p.ErrorCode.SUCCESS, list)
end

function babel.cs_babel_reset()
    -- print('p.CS_BABEL_RESET...leftFreeCouunt, buyResetCouunt', babel.leftFreeCouunt, babel.buyResetCouunt)
    if babel.leftFreeCouunt > 0 then
        babel.leftFreeCouunt = babel.leftFreeCouunt - 1
    else
        local needGold = (babel.buyResetCouunt + 1) * babelRelated.baseGold
        if not user.isResourceEnough(p.SpecialPropIdType.GOLD, needGold) then
            print('p.CS_BABEL_RESET...gold is not enough...needGold', needGold)
            agent.sendNoticeMessage(p.ErrorCode.PUBLIC_GOLD_NOT_ENOUGH, '', 1)
            return
        end
        local babelResetCouunt = agent.vip.tpl.babelResetCouunt
        if babel.buyResetCouunt >= babelResetCouunt then
            print('p.CS_BABEL_RESET...babel reset times are full...buyResetCouunt, babelResetCouunt', babel.buyResetCouunt, babelResetCouunt)
            agent.sendNoticeMessage(p.ErrorCode.PUBLIC_VIP_RESET_MAX, '', 1)
            return
        end
        babel.buyResetCouunt = babel.buyResetCouunt + 1
        user.removeResource(p.SpecialPropIdType.GOLD, needGold, p.ResourceConsumeType.BABEL_RESET_BUY)
    end

    babel.layer = 0
    babel.sendBabelDataUpdate()
    agent.sendPktout(p.SC_BABEL_RESET_RESPONSE, p.ErrorCode.SUCCESS)
end

return babel