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

local bronzeAnswerScore = t.configure['bronzeAnswerScore']

local bronzeSparrowTower = {
    todayCount = 0,     --今天答题数量
    todayRight = 0,     --今天答对题目数量
    todayWrong = 0,     --今天答错题目数量
    todayAnswerTime = 0,      --今天答题时间  
    isGetTodayReward = false,     --今天是否领取礼物 

    evtAnswer = event.new(), -- 
}



local pktHandlers = {}

function bronzeSparrowTower.onInit()
    agent.registerHandlers(pktHandlers)
    bronzeSparrowTower.dbLoad()
    --print('bronzeSparrowTower.onInit..')
    if cdlist.isHour0Refresh then
        bronzeSparrowTower.refresh(true)
    end
    cdlist.evtHour0Refresh:attachRaw(function()
        bronzeSparrowTower.refresh(true)
    end)
end

function bronzeSparrowTower.onAllCompInit()
    bronzeSparrowTower.sendBronzeSparrowTowerUpdate()
end

function bronzeSparrowTower.onSave()
    --print('bronzeSparrowTower.onSave..')
    bronzeSparrowTower.dbSave()
end

function bronzeSparrowTower.onClose()
    --print('bronzeSparrowTower.onClose..')
    bronzeSparrowTower.dbSave()
end

function bronzeSparrowTower.dbLoad()
    local data = dict.get("bronzeSparrowTower.data") or {}
    bronzeSparrowTower.todayCount = data.todayCount or bronzeSparrowTower.todayCount
    bronzeSparrowTower.todayRight = data.todayRight or bronzeSparrowTower.todayRight
    bronzeSparrowTower.todayWrong = data.todayWrong or bronzeSparrowTower.todayWrong
    bronzeSparrowTower.todayAnswerTime = data.todayAnswerTime or bronzeSparrowTower.todayAnswerTime
    bronzeSparrowTower.isGetTodayReward = data.isGetTodayReward or bronzeSparrowTower.isGetTodayReward
    --print('####bronzeSparrowTower.dbSave...todayRight, todayWrong,todayCount,isGetTodayReward', bronzeSparrowTower.todayRight, bronzeSparrowTower.todayWrong,bronzeSparrowTower.todayCount,bronzeSparrowTower.isGetTodayReward)
 
end

function bronzeSparrowTower.dbSave()
    --print('####bronzeSparrowTower.dbSave...todayRight, todayWrong  ', bronzeSparrowTower.todayRight, bronzeSparrowTower.todayWrong)
    dict.set('bronzeSparrowTower.data', {
        todayCount = bronzeSparrowTower.todayCount,
        todayRight = bronzeSparrowTower.todayRight,
        todayWrong = bronzeSparrowTower.todayWrong,
        todayAnswerTime = bronzeSparrowTower.todayAnswerTime,
        isGetTodayReward = bronzeSparrowTower.isGetTodayReward,
    })
end

function bronzeSparrowTower.refresh(sendUpdate)
    --print('###bronzeSparrowTower.refresh..')
    if not bronzeSparrowTower.isGetTodayReward and bronzeSparrowTower.todayCount > 0 then
        bronzeSparrowTower.receiveReward()    
    end
    bronzeSparrowTower.todayCount = 0
    bronzeSparrowTower.todayRight = 0
    bronzeSparrowTower.todayWrong = 0
    bronzeSparrowTower.todayAnswerTime = 0 
    bronzeSparrowTower.isGetTodayReward = false
    if sendUpdate then
        bronzeSparrowTower.sendBronzeSparrowTowerUpdate()
        bronzeSparrowTower.sendBronzeSparrowTowerReset()   
    end
end
function bronzeSparrowTower.sendBronzeSparrowTowerReset()
    --print('###bronzeSparrowTower.sendBronzeSparrowTowerReset..')
    agent.sendPktout(p.SC_BRONZE_SPARROW_TOWER_RESET)
end
function bronzeSparrowTower.sendBronzeSparrowTowerUpdate()
    local todatRight = bronzeSparrowTower.todayRight 
    local todayWrong = bronzeSparrowTower.todayWrong
    local isGetTodayReward = bronzeSparrowTower.isGetTodayReward
    --print('####bronzeSparrowTower.sendBronzeSparrowTowerUpdate...todatRight, todayWrong,isGetTodayReward', todatRight, todayWrong,isGetTodayReward)
    agent.sendPktout(p.SC_BRONZE_SPARROW_TOWER_UPDATE,todatRight, todayWrong,isGetTodayReward)
end

    --TODO:chengong 2018.4.12
    --铜雀台答题奖励邮件 只有在刷新重置时才会调用这里
function bronzeSparrowTower.receiveReward()
    local drops = {}
    for k,v in pairs(t.bronzeSparrowTowerRewardList) do 
        if bronzeSparrowTower.todayRight >= v.accessCountMin and bronzeSparrowTower.todayRight <= v.accessCountMax then
            for tplId,count in pairs(v.rewardList) do
                 table.insert(drops, {tplId = tplId, count = count})
            end
            --print('####bronzeSparrowTower.receiveReward...drops',utils.serialize(drops))
        end
    end
    if not next(drops) then
        --print('####bronzeSparrowTower.receiveReward...right count is not enough ,the reward is nil')
        return
    end    
    --邮件发送
    local mailType = p.MailType.SYSTEM
    local mailSubType = p.MailSubType.SYSTEM_BRONZESPARROWTOWER_REWARD
    local title, content = agent.mail.getTitleContentBySubType(mailSubType)
    local attachments = utils.serialize(drops)
    local timestamp = timer.getTimestampCache()
    local time = os.date("%Y-%m-%d %H:%M:%S",bronzeSparrowTower.todayAnswerTime)
    local params = time.. "," .. (bronzeSparrowTower.todayRight)
    --print('bronzeSparrowTower..reward mail..title,content,params',title,content,utils.serialize(params))
    agent.mail.saveMailRaw(user.uid, 0, mailType, mailSubType, title, content, timestamp, true, false, attachments, utils.serialize(params), 0)
end

pktHandlers[p.CS_BRONZE_SPARROW_TOWER_START] = function(pktin, session)
    local function bronzeSparrowTowerStartResponse(result)
        --print('####p.SC_BRONZE_SPARROW_TOWER_START_RESPONSE...result', result)
        agent.replyPktout(session, p.SC_BRONZE_SPARROW_TOWER_START_RESPONSE, '@@1=b', result)
    end
    local curTime = timer.getTimestampCache()
    local ativityDeadlineTime = timer.todayZeroTimestamp() + 235 * 360
    if curTime > ativityDeadlineTime then
        bronzeSparrowTowerStartResponse(false)
        agent.sendNoticeMessage(p.ErrorCode.CANNOT_ANSWER_NOW,'',1)
        return
    end
    bronzeSparrowTower.todayAnswerTime = curTime
    bronzeSparrowTowerStartResponse(true)
end

pktHandlers[p.CS_BRONZE_SPARROW_TOWER_ANSWER] = function(pktin, session)
    local function bronzeSparrowTowerAnswerResponse(result)
        --print('####p.CS_BRONZE_SPARROW_TOWER_ANSWER...result', result)
        agent.replyPktout(session, p.SC_BRONZE_SPARROW_TOWER_ANSWER_RESPONSE, '@@1=i', result)
    end
    local right, wrong = pktin:read('ii')
    local totalCount = 0
    for k,v in pairs( t.bronzeSparrowTower) do
        totalCount = totalCount + t.bronzeSparrowTower[k]
    end
    local answerCount = right + wrong
    if answerCount > totalCount then
        --print('#####p.CS_BRONZE_SPARROW_TOWER_ANSWER....tpl_bronze_sparrow_tower wrong number...answerCount,totalCount', answerCount, totalCount)
        agent.sendNoticeMessage(p.ErrorCode.BRONZE_SUBJECT_COUNT_NOT_MATCH, '', 1)
        bronzeSparrowTowerAnswerResponse(p.ErrorCode.FAIL)
        return
    else
        if  bronzeSparrowTower.todayCount < totalCount then
            bronzeSparrowTower.todayRight =  right
            bronzeSparrowTower.todayWrong =  wrong
            bronzeSparrowTower.todayCount = bronzeSparrowTower.todayRight + bronzeSparrowTower.todayWrong
            --print('#####p.CS_BRONZE_SPARROW_TOWER_ANSWER.., right, wrong,todayCount', bronzeSparrowTower.todayRight, bronzeSparrowTower.todayWrong,bronzeSparrowTower.todayCount)
        end      
    end

    if bronzeSparrowTower.todayCount > totalCount then
        --print('#####p.CS_BRONZE_SPARROW_TOWER_ANSWER...Count not match...answerCount,totalCount', answerCount, totalCount)
        agent.sendNoticeMessage(p.ErrorCode.BRONZE_SUBJECT_COUNT_NOT_MATCH, '', 1)
        bronzeSparrowTowerAnswerResponse(p.ErrorCode.FAIL)
        return
    end

    if bronzeSparrowTower.todayCount == totalCount then
        bronzeSparrowTower.evtAnswer:trigger(right)
    end
    
    bronzeSparrowTower.sendBronzeSparrowTowerUpdate()
    bronzeSparrowTowerAnswerResponse(p.ErrorCode.SUCCESS)
end

pktHandlers[p.CS_BRONZE_SPARROW_TOWER_RECIVE_REWARD] = function(pktin, session)
    local function bronzeSparrowTowerReciveRewardResponse(result)
        print('####p.CS_BRONZE_SPARROW_TOWER_RECIVE_REWARD....result', result)
        agent.replyPktout(session, p.SC_BRONZE_SPARROW_TOWER_RECIVE_REWARD_RESPONSE, '@@1=i', result)
    end
    --判断奖励是否已被领取
    if bronzeSparrowTower.isGetTodayReward then
        bronzeSparrowTowerReciveRewardResponse(p.ErrorCode.FAIL)
        return
    end
    --读取奖励是否双倍领取
    local isDouble = pktin:read('b')
    local totalCount = 0
    for k,v in pairs(t.bronzeSparrowTower) do
        totalCount = totalCount + t.bronzeSparrowTower[k]
    end         
    --判断题目是否全部答完
    if bronzeSparrowTower.todayCount ~= totalCount then
         bronzeSparrowTowerReciveRewardResponse(p.ErrorCode.BRONZE_SUBJECT_COUNT_NOT_MATCH)
        return
    end 
    --遍历配置表得到对应的奖励LIST
    local rewardList = {}
    local expense = {}
    for k,v in pairs(t.bronzeSparrowTowerRewardList) do 
        if bronzeSparrowTower.todayRight >= v.accessCountMin and bronzeSparrowTower.todayRight <= v.accessCountMax then
            for tplId,count in pairs(v.rewardList) do
                table.insert(rewardList,{tplId=tplId,count=count})
            end
            for tplId,count in pairs(v.expense) do
                expense[tplId]=count
            end
        end
    end 
    --print('####bronzeSparrowTower.receiveReward...rewardList,expense',utils.serialize(rewardList),utils.serialize(expense))  
    --TODO:20180414待完善
    --领取到背包
    local needBagSpace = 0
    for _,item in pairs(rewardList) do
        if isDouble then
           item.count = item.count * 2
        end
        needBagSpace = needBagSpace +  item.count 
    end
    --如果是双倍领取，判断账户中元宝数量是否足够
    if isDouble then 
        if next(expense) == nil then
            --print('####p.CS_BRONZE_SPARROW_TOWER_RECIVE_REWARD...提示花费列表为空')
            bronzeSparrowTowerReciveRewardResponse(p.ErrorCode.FAIL)
            return
        end
       local isEnough = bag.checkResourcesEnough(expense)
       if false == isEnough then
            --print('####p.CS_BRONZE_SPARROW_TOWER_RECIVE_REWARD...提示元宝数量不足')
            bronzeSparrowTowerReciveRewardResponse(p.ErrorCode.PUBLIC_GOLD_NOT_ENOUGH)
            return   
       end
       --扣除相应的元宝
       bag.removeResources(expense,p.ResourceConsumeType.ITEM_BUY_USE)
    end
    bag.addItems(rewardList, p.ResourceGainType.BRONZE)
    bronzeSparrowTower.isGetTodayReward = true
    bronzeSparrowTowerReciveRewardResponse(p.ErrorCode.SUCCESS)
    bronzeSparrowTower.sendBronzeSparrowTowerUpdate()
end

return bronzeSparrowTower

