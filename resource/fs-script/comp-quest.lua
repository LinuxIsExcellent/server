local agent = ...
local p = require('protocol')
local t = require('tploader')
local timer = require('timer')
local dbo = require('dbo')
local utils = require('utils')
local questInfo = require('quest/questInfo')
local questProducer = require('quest/quest-producer')
local dataStub = require('stub/data')
local event = require('libs/event')
local user = agent.user
local bag = agent.bag
local dict = agent.dict
--local pktHandlers = {}

local quest = {
    normalQuests = {}, --支线任务map<questId,questInfo>
    noviceQuests = {}, --主线任务(引导)map<questId,questInfo>
    storyQuests = {},  --剧情任务
    questFinishAndDraws = {}, --map<questId,questId>全部已领取奖励任务
    storyQuestFinishAndDraws = {},  --map<questId,questId>全部已领取剧情任务奖励
    questDrewInfos = {}, --map<questId,questInfo>当前在线已领取奖励任务
    evtFinishQuest = event.new(),   -- (questId)
}

local cumulative_quest = {
    id = 0,
    target = 0,     --目标
    progress = 0,   --我的进度
    drawList = {},  --已领取奖励的target {[id]=true,}
}

function quest.dbLoad()
    local db = dbo.open(0)
    local rs = db:executePrepare('SELECT questId, total, progress, createTime, finishTime, drawTime  from s_quest where uid = ?', user.uid)
    if rs.ok then
        for _, row in ipairs(rs) do
            local tpl = t.quest[row.questId]
            if tpl ~= nil then
                local info = questInfo:new({})
                info.tpl = tpl
                info.total = tpl.count
                info.progress = row.progress
                info.createTime = row.createTime
                info.finishTime = row.finishTime
                info.drawTime = row.drawTime
                if info.total > info.progress then
                    info.finishTime = 0
                else
                    info.progress = info.total
                end
                -- print('quest.dbLoad ... questid, info.tpl.type, total, progress, createTime, finishTime, drawTime', info.tpl.id, info.tpl.type, info.total, info.progress, info.createTime, info.finishTime, info.drawTime)
                if info.drawTime ~= 0 then
                    quest.questFinishAndDraws[row.questId] = row.questId
                else
                    local q = questProducer.createQuest(agent, info)
                    if q ~= nil then
                        if tpl.type == p.QuestType.NOVICE then
                            -- print('quest.dbLoad ......  insert into noviceQuests, questId:', tpl.id, 'type', tpl.type, info.total, info.progress)
                            quest.noviceQuests[tpl.id] = q
                        elseif tpl.type == p.QuestType.NORMAL then
                            -- print('quest.dbLoad ......  insert into normalQuests, questId:', tpl.id, 'type', tpl.type, info.total, info.progress)
                            quest.normalQuests[tpl.id] = q
                        end
                        q:onInit()
                    end
                end
            end
            local tpl_story = t.storyQuest[row.questId]
            if tpl_story ~= nil then
                local info = questInfo:new({})
                info.tpl = tpl_story
                info.total = tpl_story.count
                info.progress = row.progress
                info.createTime = row.createTime
                info.finishTime = row.finishTime
                info.drawTime = row.drawTime
                if info.total > info.progress then
                    info.finishTime = 0
                else
                    info.progress = info.total
                end
                -- print('quest.dbLoad ... questid, info.tpl.type, total, progress, createTime, finishTime, drawTime', info.tpl.id, info.tpl.type, info.total, info.progress, info.createTime, info.finishTime, info.drawTime)
                if info.drawTime ~= 0 then
                    quest.storyQuestFinishAndDraws[row.questId] = row.questId
                else
                    local q = questProducer.createQuest(agent, info)
                    if q ~= nil then
                        quest.storyQuests[tpl_story.id] = q
                        q:onInit()
                    end
                end
            end
        end
    end
    -- print("###quest.dbLoad", utils.serialize(quest.storyQuestFinishAndDraws))
end

function quest.dbSave(timingSave)
    local noviceQuests = quest.noviceQuests
    local normalQuests = quest.normalQuests
    local storyQuests = quest.storyQuests
    local questDrewInfos = quest.questDrewInfos
    local changeQuestInfo = {}

    for _, v in pairs(noviceQuests) do
        if v:questInfo():isDirty() then
            changeQuestInfo[v:questId()] = v:questInfo()
        end
    end
    for _, v in pairs(normalQuests) do
        if v:questInfo():isDirty() then
            changeQuestInfo[v:questId()] = v:questInfo()
        end
    end

    for _, v in pairs(storyQuests) do
        if v:questInfo():isDirty() then
            changeQuestInfo[v:questId()] = v:questInfo()
        end
    end

    for _, v in pairs(questDrewInfos) do
        changeQuestInfo[v.tpl.id] = v
    end

    local list = {}
    for _, v in pairs(changeQuestInfo) do
        v:setClean()
        table.insert(list, { uid = user.uid, questId = v.tpl.id, total = v.total, progress = v.progress, createTime = v.createTime, finishTime = v.finishTime, drawTime = v.drawTime })
    end

    dataStub.appendDataList(p.DataClassify.QUEST, list)

    --清除已领取的任务info
    for k, v in pairs(questDrewInfos) do
        if not v:isDirty() and v:isSync() then
            quest.questDrewInfos[k] = nil 
        end
    end
end

function quest.onInit()
--    agent.registerHandlers(pktHandlers)
    quest.dbLoad()

    --cumulative quest
    -- quest.loadCumulative()
    -- quest.fetchCumulativeQuest(cumulative_quest.id)

    quest.checkNewNormalQuest()
    quest.checkNewNoviceQuest()
    quest.checkNewStoryQuest()
end

function quest.onAllCompInit()
    quest.sendQuestUpdate()
    quest.sendNoviceQuestUpdate()
    quest.sendStoryQuestUpdate()
    -- quest.sendCumulativeQuestUpdate()
end

function quest.onClose()
    quest.dbSave()
    -- quest.saveCumulative()
end

function quest.onSave()
    quest.dbSave()
    -- quest.saveCumulative()
end

function quest.isAcceptQuestFull(type)
    local full = false
    local count = 0
    if type == p.QuestType.NOVICE then
        for _, v in pairs(quest.noviceQuests) do
            if not v:questInfo():isFinish() then
                count = count + 1
                full = true
                break
            end
        end
    elseif type == p.QuestType.NORMAL then
        -- for _, v in pairs(quest.normalQuests) do
        --     count = count + 1
        -- end
        -- if count >= 3 then
        --     full = true
        -- end
    elseif type == p.QuestType.STORY then

    end
    -- print('quest.isAcceptQuestFull ......  type:', type, 'count:', count, 'full:', full)
    return full
end

function quest.isQuestFinish(questId, type)
    local qid = nil
    if type == p.QuestType.STORY then
        qid = quest.storyQuestFinishAndDraws[questId]
    else
        qid = quest.questFinishAndDraws[questId]
    end
    if qid == nil then
        if type == p.QuestType.NORMAL then
            local q = quest.normalQuests[questId]
            if q ~= nil and q:questInfo():isFinish() then
                return true
            end
            return false
        elseif type == p.QuestType.NOVICE then
            local q = quest.noviceQuests[questId]
            if q ~= nil and q:questInfo():isFinish() then
                return true
            end
            return false
        elseif type ==  p.QuestType.STORY then
            local q = quest.storyQuests[questId]
            if q ~= nil and q:questInfo():isFinish() then
                return true
            end
            return false
        end
    end
    return true
end

function quest.canAccept(tpl)
    if tpl == nil then
        return false
    end
    -- check finish quest
    local finishQuest = nil
    if tpl.type == p.QuestType.STORY then
        finishQuest = quest.storyQuestFinishAndDraws[tpl.id]
    else
        finishQuest = quest.questFinishAndDraws[tpl.id]
    end
    if finishQuest ~= nil then
        return false
    end
    -- check current quest
    local currentQuest = quest.normalQuests[tpl.id]
    if currentQuest ~= nil then
        return false
    end
    currentQuest = quest.noviceQuests[tpl.id]
    if currentQuest ~= nil then
        return false
    end
    currentQuest = quest.storyQuests[tpl.id]
    if currentQuest ~= nil then
        return false
    end
    -- if user.info.level < tpl.minLevel or user.info.level > tpl.maxLevel then
    --     return false
    -- end

    -- print("f33333333333333333333333333333333333333333333333333")
    --check tpl.preQuestId
    local tplPre = nil
    if tpl.type == p.QuestType.STORY then
        tplPre = t.storyQuest[tpl.preQuestId]
        if tpl.preQuestId ~= 0 and not quest.isQuestFinish(tplPre.id, tplPre.type) then
            return false
        end
    else
        tplPre = t.quest[tpl.preQuestId]
        -- 支线和主线任务是领取完之后才能接取下一个任务
        if tpl.preQuestId ~= 0 and not quest.questFinishAndDraws[tpl.preQuestId] then
            return false
        end
    end
    -- print("f4444444444444444444444444444444444444444")

    -- FIXME andrew just accept some
    if tpl.CondType == p.QuestCondType.PASS_SPECIFY_SCENARIO_CHAPTER
        or tpl.CondType == p.QuestCondType.PASS_SPECIFY_SCENARIO_NUM
        or tpl.CondType == p.QuestCondType.PASS_SPECIFY_SCENARIO
        or tpl.CondType == p.QuestCondType.EQUIP_LEVELUP_NUM_LEVEL
        or tpl.CondType == p.QuestCondType.ARENA_WIN_NUM
        or tpl.CondType == p.QuestCondType.ARENA_FIGHT_NUM
        or tpl.CondType == p.QuestCondType.UPGRADE_BUILDING_LEVEL
        or tpl.CondType == p.QuestCondType.UPGRADE_RES_BUILDING_LEVEL
        or tpl.CondType == p.QuestCondType.HERO_NUM
        or tpl.CondType == p.QuestCondType.PURPLE_HERO_NUM
        or tpl.CondType == p.QuestCondType.HERO_NUM_LEVEL
        or tpl.CondType == p.QuestCondType.HERO_NUM_STAR
        or tpl.CondType == p.QuestCondType.PLAYER_POWER
        or tpl.CondType == p.QuestCondType.PLAYER_LEVEL

        or tpl.CondType == p.QuestCondType.BUILDING_TRAP
        or tpl.CondType == p.QuestCondType.CREATE_BUILDING
        or tpl.CondType == p.QuestCondType.TRAIN_ARMY
        or tpl.CondType == p.QuestCondType.TRAIN_ARMY_NUM
        or tpl.CondType == p.QuestCondType.TELEPORT
        or tpl.CondType == p.QuestCondType.USE_RES_ITEM
        or tpl.CondType == p.QuestCondType.SCOUT_NUM
        or tpl.CondType == p.QuestCondType.BE_SCOUT_NUM
        or tpl.CondType == p.QuestCondType.TOTAL_ADD_SILVER
        or tpl.CondType == p.QuestCondType.ATTACK_PLAYER_WIN_NUM
        or tpl.CondType == p.QuestCondType.BABEL_WIN_NUM
        or tpl.CondType == p.QuestCondType.HEAL_ARMY_NUM
        or tpl.CondType == p.QuestCondType.HEAL_ARMY_TIME
        or tpl.CondType == p.QuestCondType.KILL_MONSTER_NUM
        or tpl.CondType == p.QuestCondType.TAKE_RESOURCE_TAX
        or tpl.CondType == p.QuestCondType.TAKE_RESOURCE_TAX_NUM
        or tpl.CondType == p.QuestCondType.MOPUP_SCENARIO
        or tpl.CondType == p.QuestCondType.PASS_SCENARIO_COPY_CHAPTER
        or tpl.CondType == p.QuestCondType.ALLIANCE_DONATE_NUM
        or tpl.CondType == p.QuestCondType.SCIENCE_STUDY
        or tpl.CondType == p.QuestCondType.SCIENCE_STUDY_NUM
        or tpl.CondType == p.QuestCondType.ALLIANCE_HELP_NUM
        or tpl.CondType == p.QuestCondType.SHOP_BUY_ITEM
        or tpl.CondType == p.QuestCondType.OCCUPY_CITY
        or tpl.CondType == p.QuestCondType.FINISH_QUEST
        or tpl.CondType == p.QuestCondType.GET_HERO
        or tpl.CondType == p.QuestCondType.GET_HERO_NUM
        or tpl.CondType == p.QuestCondType.HERO_ACCUMULATIVE_LEVELUP
        or tpl.CondType == p.QuestCondType.HERO_LEVELUP
        or tpl.CondType == p.QuestCondType.UPGRADE_HERO_SKILL_NUM
        or tpl.CondType == p.QuestCondType.UPGRADE_HERO_SKILL
        or tpl.CondType == p.QuestCondType.BRONZE_NUM
--        or tpl.CondType == p.QuestCondType.BRONZE_SCORE
        or tpl.CondType == p.QuestCondType.GATHER_RESOURCE_NUM
        or tpl.CondType == p.QuestCondType.GATHER_RESOURCE
        or tpl.CondType == p.QuestCondType.ALLIANCE_JOIN
        or tpl.CondType == p.QuestCondType.USER_UPGRADE
        or tpl.CondType == p.QuestCondType.GET_HERO_TYPE_NUM
        or tpl.CondType == p.QuestCondType.RESOURCE_OUTPUT
        or tpl.CondType == p.QuestCondType.ARENA_RANK
        or tpl.CondType == p.QuestCondType.BRONZE_RIGHT_NUM
        then
        return true
    else
        return false
    end

    return true
end

function quest.tryAccept(tpl)
    if tpl == nil then
        return false
    end
    if not quest.canAccept(tpl) then
        print('quest.tryAccept ...... ********** \n\n ***** not can ***** accept questid:', tpl.id, 'type:', tpl.type, 'condtype:', tpl.CondType)
        return false
    end
    --print('quest.tryAccept ...... accept questid:', tpl.id, 'type:', tpl.type, 'condtype:', tpl.CondType)

    -- accept
    -- print(debug.traceback())
    local info = questInfo:new({tpl = tpl, uid = user.uid})
    info.total = tpl.count
    info.createTime = timer.getTimestampCache()
    info:setDirty()

    local q = questProducer.createQuest(agent, info)
    -- print("###quest.tryAccept", utils.serialize(info))
    if q == nil then 
        print('quest.tryAccept ....... create quest failed. quest id:', tpl.id, 'quest condtype:', tpl.condtype) 
        return 
    end
    if tpl.type == p.QuestType.NOVICE then
        quest.noviceQuests[tpl.id] = q
    elseif tpl.type == p.QuestType.NORMAL then
        quest.normalQuests[tpl.id] = q
    elseif tpl.type == p.QuestType.STORY then
        quest.storyQuests[tpl.id] = q
    end
    q:onInit()

    return true
end 

function quest.checkNewNormalQuest()
    --1.是否需要接任务
    if quest.isAcceptQuestFull(p.QuestType.NORMAL) then
        return
    end

    for _, v in pairs(t.quest) do
        if v.type == p.QuestType.NORMAL and quest.canAccept(v) then
            if quest.isAcceptQuestFull(p.QuestType.NORMAL) then
                break
            end
            quest.tryAccept(v)
        end
    end

    -- --2.找可以接的任务
    -- local canAccetpTpls = {}
    -- local normalCount = 0
    -- if t.quest == nil then
    --     return
    -- end
    -- -- print("111111 t.quest", utils.serialize(t.quest))

    -- for _, v in pairs(t.quest) do
    --     if v.type == p.QuestType.NORMAL and quest.canAccept(v) then
    --         normalCount = normalCount + 1
    --         canAccetpTpls[normalCount]  = v.id
    --     end
    -- end

    -- print("111111 normalCount", normalCount)
    -- --3.随机任务
    -- while normalCount ~= 0 do
    --     if quest.isAcceptQuestFull(p.QuestType.NORMAL) then
    --         break
    --     end
    --     local r = utils.getRandomNum(1, normalCount+1)
    --     local id = canAccetpTpls[r]
    --     quest.tryAccept(t.quest[id])
    --     for i = r, normalCount-1 do
    --         canAccetpTpls[i] = canAccetpTpls[i+1]
    --     end
    --     canAccetpTpls[normalCount] = nil
    --     normalCount = normalCount - 1
    -- end
end

function quest.checkNewNoviceQuest(questId)
    --print('quest.checkNewNoviceQuest ......   ',questId)
    if questId then
        local newQuestId = questId
        while newQuestId ~= 0 do
            if quest.isAcceptQuestFull(p.QuestType.NOVICE) then
                break
            end
            local tpl = nil
            for _, v in pairs(t.quest) do
                if v.preQuestId == newQuestId and v.type == p.QuestType.NOVICE then
                    tpl = v
                    break
                end
            end
            if tpl and tpl.preQuestId == newQuestId and tpl.type == p.QuestType.NOVICE then
                if quest.tryAccept(tpl) then
                    newQuestId = tpl.id
                else
                    newQuestId = 0
                end
            else
                newQuestId = 0
            end
        end
    else
        local noviceFull = quest.isAcceptQuestFull(p.QuestType.NOVICE)
        if t.quest == nil then
            return
        end

        for _, v in pairs(t.quest) do
            if noviceFull then
                break
            end
            if quest.canAccept(v) then
                if v.type == p.QuestType.NOVICE and not recommendFull then
                    quest.tryAccept(v)
                    noviceFull = quest.isAcceptQuestFull(p.QuestType.NOVICE)
                end
            end
        end
    end
end

function quest.checkNewStoryQuest()
    --1.是否需要接任务
    if quest.isAcceptQuestFull(p.QuestType.NORMAL) then
        return
    end

    for _, v in pairs(t.storyQuest) do
        if v.type == p.QuestType.STORY and quest.canAccept(v) then
            if quest.isAcceptQuestFull(p.QuestType.STORY) then
                break
            end
            quest.tryAccept(v)
        end
    end
end

function quest.onQuestUpdate(questId, type)
    -- --print('quest.onQuestUpdate ......questId, type', questId, type)
    -- -- check have finish quest
    if type == p.QuestType.NORMAL then
        quest.checkNewNormalQuest()
        quest.sendQuestUpdate()
    elseif type == p.QuestType.STORY then
        quest.checkNewStoryQuest()
        quest.sendStoryQuestUpdate()
    else
        local qN = quest.noviceQuests[questId]
        if qN ~= nil then
            --print('quest.onQuestUpdate ......noviceQuests ID, protocol, total', qN:questId(), qN:questInfo().progress, qN:questInfo().total)
            if qN:questInfo():isFinish() then
                quest.checkNewNormalQuest()
                quest.checkNewNoviceQuest(questId)
                quest.sendQuestUpdate()
                quest.sendStoryQuestUpdate()
                quest.sendNoviceQuestUpdate()
            end
        end
    end
end

function quest.sendQuestUpdate()
    -- print('//////////quest.sendQuestUpdate ......')
    local noviceQuests = quest.noviceQuests
    local questDrewInfos = quest.questDrewInfos
    local normalQuests = quest.normalQuests
    local questFinishAndDraws = quest.questFinishAndDraws
    local updateList = {}
    local drawList = {}
    local finishAndDrawList = {}

    for _, v in pairs(noviceQuests) do
        local qinfo = v:questInfo()
        if qinfo ~= nil and not qinfo:isSync() then
            qinfo:setSync(true)
            table.insert(updateList, {tplId = qinfo.tpl.id, progress = qinfo.progress})
            -- print("###noviceQuests...", utils.serialize(v))
        end
    end

    for _, v in pairs(normalQuests) do
        local qinfo = v:questInfo()
        if qinfo ~= nil  and not qinfo:isSync() then
            qinfo:setSync(true)
            table.insert(updateList, {tplId = qinfo.tpl.id, progress = qinfo.progress})
            -- print("###normalQuests...", utils.serialize(v))
        end
    end
    
    for _, v in pairs(questDrewInfos) do 
        local qinfo = v
        -- print("###questDrewInfos...", v.tpl.id, qinfo:isSync())
        if not qinfo:isSync() and qinfo.tpl.type ~= p.QuestType.STORY then 
            qinfo:setSync(true)
            table.insert(drawList, {tplId = qinfo.tpl.id})
        end
    end
    for _,v in pairs(questFinishAndDraws) do
        table.insert(finishAndDrawList, {tplId = v})
    end
    -- [[
    -- for _,v in pairs(updateList) do
    --     print('quest id, progress',v.tplId, v.progress)
    -- end
    -- ]]
    
    -- print("updateList, drawList, questFinishAndDraws", utils.serialize(updateList),utils.serialize(drawList), utils.serialize(questFinishAndDraws))
    agent.sendPktout(p.SC_QUEST_UPDATE,  '@@1=[tplId=i,progress=i],2=[tplId=i],3=[tplId=i]',  updateList, drawList, questFinishAndDraws)
end

function quest.getCurrentQuestChapter()
    local chapterId = 0
    local storyQuests = quest.storyQuests
    for _, v in pairs(storyQuests) do
        local qInfo = v:questInfo()
        if qInfo.tpl.subtype == 1 and not qInfo:isDraw() then
            if chapterId == 0 then
                chapterId = qInfo.tpl.chapterId
            elseif qInfo.tpl.chapterId <= chapterId then 
                chapterId = qInfo.tpl.chapterId
            end 
        end
    end
    return chapterId
end


function quest.sendStoryQuestUpdate()
    local storyQuests = quest.storyQuests
    local questDrewInfos = quest.questDrewInfos
    local updateList = {}
    local drawList = {}
    local finishDrawList = {}
    local chapterId = quest.getCurrentQuestChapter()
    for _, v in pairs(storyQuests) do
        local qinfo = v:questInfo()
        if qinfo ~= nil and not qinfo:isSync() and chapterId == qinfo.tpl.chapterId then
            qinfo:setSync(true)
            table.insert(updateList, {tplId = qinfo.tpl.id, progress = qinfo.progress})
        end
    end

    for _, v in pairs(quest.storyQuestFinishAndDraws) do 
        local storyTpl = t.storyQuest[v]
        if storyTpl.chapterId == chapterId then
            table.insert(drawList, {tplId = v})
        end
        table.insert(finishDrawList, {tplId = v})
    end

    -- print("###p.SC_STORY_QUEST_UPDATE, updateList, drawList", utils.serialize(updateList),utils.serialize(drawList))
    agent.sendPktout(p.SC_STORY_QUEST_UPDATE,  '@@1=[tplId=i,progress=i],2=[tplId=i],3=[tplId=i]',  updateList, drawList, finishDrawList)
    -- body
end

function quest.sendNoviceQuestUpdate()
    -- local tplId = 0
    -- for _, v in pairs(quest.noviceQuests) do
    --     if not v:questInfo():isFinish() then
    --         tplId = v:questId()
    --         break
    --     end
    -- end
    -- if tplId ~= 0 then
    --     --print('sendNoviceQuestUpdate  id:',tplId)
    --     agent.sendPktout(p.SC_QUEST_NOVICE_UPDATE,  tplId)
    -- end
end

--cumulative quest
function quest.loadCumulative()
    local data = dict.get("cumulative_quest.info") or {}
    cumulative_quest.id = data.id or cumulative_quest.id
    cumulative_quest.target = data.target or cumulative_quest.target 
    cumulative_quest.progress = data.progress or cumulative_quest.progress
    cumulative_quest.drawList = data.drawList or cumulative_quest.drawList
end

function quest.saveCumulative()
    dict.set("cumulative_quest.info", {
        id = cumulative_quest.id,
        target = cumulative_quest.target,
        progress = cumulative_quest.progress,
        drawList = cumulative_quest.drawList,
    })
end

function quest.sendCumulativeQuestUpdate()
    --print('###quest.sendCumulativeQuestUpdate..id, target, progress...',cumulative_quest.id, cumulative_quest.target, cumulative_quest.progress)
    -- agent.sendPktout(p.SC_CUMULATIVE_QUEST_UPDATE,  '@@1=i,2=i,3=i', cumulative_quest.id, cumulative_quest.target, cumulative_quest.progress)
end

function quest.fetchCumulativeQuest(id)
    if id == nil then
        id = 0
    end
    local drawList = cumulative_quest.drawList or {}
    -- 根据当前目标,确定下一目标
    if id == 0 or drawList[id] then
        local tplIds = {}
        for _, v in pairs(t.quest_target) do
            table.insert(tplIds, {id=v.id})
        end
        table.sort(tplIds, function(lhs,rhs) return lhs.id < rhs.id end)
        for _, v in ipairs(tplIds) do
            if id < v.id and not drawList[v.id] then
                local tpl = t.quest_target[v.id]
                cumulative_quest.id = tpl.id
                cumulative_quest.target = tpl.target
                break
            end
        end
        --[[
        local tpl = t.quest_target
        for _, v in ipairs(tpl) do
            if id < v.id and not drawList[v.id] then
                cumulative_quest.id = v.id
                cumulative_quest.target = v.target
                break
            end
        end
        --]]
    end
end

-- recev
function quest.cs_quest_draw(questId)
    local function questDrawResponse(result)
        agent.replyPktout(session, p.SC_QUEST_DRAW_RESPONSE, result)
    end

    -- print('pktHandlers CS_QUEST_DRAW  questid:', questId)

    local tpl = t.quest[questId]
    if tpl == nil then
        --print('pktHandlers CS_QUEST_DRAW  tplis nil', questId)
        questDrawResponse(false)
        return
    end

    local questList = nil
    local qRecord = nil
    if tpl.type == p.QuestType.NOVICE then
        questList = quest.noviceQuests
        qRecord = questList[questId]
    elseif tpl.type == p.QuestType.NORMAL then
        questList = quest.normalQuests
        qRecord = questList[questId]
    end

    if qRecord == nil then
        print('pktHandlers CS_QUEST_DRAW  quest not accepte.', questId)
        questDrawResponse(false)
        return
    end

    -- check finish and not drew
    if not qRecord:questInfo():isFinish() or qRecord:questInfo():isDraw() then
        --print('pktHandlers CS_QUEST_DRAW  quest not finish or is draw', questId)
        questDrawResponse(false)
        return
    end

    -- drew
    quest.questFinishAndDraws[questId] = questId
    local qInfo = qRecord:questInfo()
    qInfo:draw(timer.getTimestampCache())
    quest.questDrewInfos[questId] = qInfo
    questList[questId] = nil

    -- pickitem
    -- print("tpl.dropList ", utils.serialize(tpl.dropList))
    bag.pickDropItems(tpl.dropList, p.ResourceGainType.QUEST)
    questDrawResponse(true)

    -- accepte new quest
    if tpl.type ~= p.QuestType.NOVICE then
        quest.checkNewNormalQuest()
    else
        quest.checkNewNormalQuest()
        quest.checkNewNoviceQuest(questId)
    end
    quest.sendQuestUpdate()

end

-- recev
function quest.cs_story_quest_draw(questId)
    local function questDrawResponse(result)
        agent.replyPktout(session, p.SC_STORY_QUEST_DRAW_RESPONSE, result)
    end

    print('pktHandlers CS_STORY_QUEST_DRAW  questid:', questId)

    local tpl = t.storyQuest[questId]
    if tpl == nil then
        --print('pktHandlers CS_QUEST_DRAW  tplis nil', questId)
        questDrawResponse(false)
        return
    end

    local qRecord = quest.storyQuests[questId]


    -- if
    -- if tpl.type == p.QuestType.NOVICE then
    --     questList = quest.noviceQuests
    --     qRecord = questList[questId]
    -- elseif tpl.type == p.QuestType.NORMAL then
    --     questList = quest.normalQuests
    --     qRecord = questList[questId]
    -- end
    if qRecord == nil then
        print('pktHandlers CS_STORY_QUEST_DRAW  quest not accepte.', questId)
        questDrawResponse(false)
        return
    end

    -- check finish and not drew
    if not qRecord:questInfo():isFinish() or qRecord:questInfo():isDraw() then
        print("questInfo.total,", qRecord:questInfo().total, qRecord:questInfo().progress)
        print('pktHandlers CS_STORY_QUEST_DRAW  quest not finish or is draw', qRecord:questInfo():isFinish(), qRecord:questInfo():isDraw())
        questDrawResponse(false)
        return
    end
    
    -- drew
    quest.storyQuestFinishAndDraws[questId] = questId
    local qInfo = qRecord:questInfo()
    qInfo:draw(timer.getTimestampCache())
    quest.questDrewInfos[questId] = qInfo
    quest.storyQuests[questId] = nil

    -- pickitem
    -- print("tpl.dropList ", utils.serialize(tpl.dropList))
    bag.pickDropItems(tpl.dropList, p.ResourceGainType.QUEST)
    questDrawResponse(true)

    quest.checkNewStoryQuest()
    quest.sendStoryQuestUpdate()
    --update cumulative quest
    -- cumulative_quest.progress = cumulative_quest.progress + 1
    -- quest.sendCumulativeQuestUpdate()
end

return quest
