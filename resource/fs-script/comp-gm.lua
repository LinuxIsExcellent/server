local agent = ...
local utils = require('utils')
local user = agent.user
local user = agent.user
local alliance = agent.alliance
local dict = agent.dict
local quest = agent.quest
local army = agent.army

local p = require('protocol')
local t = require('tploader')
local timer = require('timer')

local M = { handles = {} }

local function split(s, p)
    local rt= {}
    string.gsub(s, '[^'..p..']+', function(w) table.insert(rt, w) end )
    return rt
end

local function chatGMResponse(session, ok)
    agent.replyPktout(session, p.SC_CHAT_GM_RESPONSE, '@@1=b', ok)
end

function M.register(name, handle)
    M.handles[name] = handle
end

function M.command(cmd, session)
    local params = split(cmd, ',')
    if #params < 1 then
        chatGMResponse(session, false)
        return
    end
    print("CS_CHAT_GM = params = ", utils.serialize(params))
    local handle = M.handles[params[1]]
    if not handle then return end
    handle(session, params)
end

local addres = function(session, params)
    if #params < 3 then
        chatGMResponse(session,false)
        return
    end
    local count = tonumber(params[3])
    if count > 0 then
        if count > 900000000 then
            count = 900000000
        end
        user.addResource(tonumber(params[2]), count)
    elseif count < 0 then
        user.removeResource(tonumber(params[2]), -count)
    end
end

local additem = function(session, params)
    if #params < 3 then
        chatGMResponse(session,false)
        return
    end

    local tplId = tonumber(params[2])
    local count = tonumber(params[3])
    if count > 0 then
        if count > 900000000 then
            count = 900000000
        end
        --agent.bag.addItem(tonumber(params[2]), count)
        local pickList = {{ tplId = tplId, count = count }}
        agent.bag.pickDropItems(pickList, p.ResourceGainType.DEFAULT)
    elseif count < 0 then
        agent.bag.removeItem(tplId, -count)
    end
    chatGMResponse(session, true)
end

local addexp = function(session, params)
    if #params < 2 then
        chatGMResponse(session,false)
        return
    end
    local exp = tonumber(params[2])
    user.addExp(exp, true)
    chatGMResponse(session, true)
end

local vipexp = function(session, params)
    if #params < 2 then
        chatGMResponse(session,false)
        return
    end

    local exp = tonumber(params[2])
    agent.vip.addExp(exp, true)
    chatGMResponse(session, true)
end

local upbuild = function(session, params)
    if #params < 2 then
        chatGMResponse(session, false)
        return
    end

    local level = tonumber(params[2])
    if level < 1 then
        level = 1
    end

    local gridId = 0
    if #params == 3 then
        gridId = tonumber(params[3])
    end

    if gridId > 0 then
        local target = agent.building.list[gridId]
        if target then
            local tpl = t.building[target.tplId]
            local maxLevel = #tpl.levels
            if level >= maxLevel then
                target.level = maxLevel
            else    
                target.level = level
            end
            target.sync = false
            target:onBuildFinish()
            agent.building.sendBuildingQueuesUpdate()
            agent.building.sendBuildingUpdate()
        end
    else
        for _,target in pairs(agent.building.list) do
            if target then
                if target.tplId > 0 then
                    local tpl = t.building[target.tplId]
                    local maxLevel = #tpl.levels
                    if level >= maxLevel then
                        target.level = maxLevel
                    else    
                        target.level = level
                    end
                    target.sync = false
                    target:onBuildFinish()
                end
            end
        end
        agent.building.sendBuildingQueuesUpdate()
        agent.building.sendBuildingUpdate()
    end

    chatGMResponse(session, true)
end

local qcl = function(session, params)
    if #params < 2 then
        chatGMResponse(session, false)
        return
    end

    local layer = tonumber(params[2])
    if layer > 30 or layer < 0 then
        layer = 30
    end

    local babel = agent.babel
    --更新数据
    babel.layer = layer
    if layer > babel.historylayer then
        babel.historylayer = layer
        babel.historyTime = timer.getTimestampCache()
    end
    babel.sendBabelDataUpdate()

    babel.evtPassBabel:trigger(layer)   

    chatGMResponse(session, true)
end

local hero = function(session, params)
    if #params < 2 then
        chatGMResponse(session, false)
        return
    end

    local heros = {}
    local heroId = tonumber(params[2])
    if heroId == nil then            
        local heroName = params[2]
        for k,v in pairs(t.heros) do
            if string.find(v.remark, heroName) then
                heroId = k
                table.insert(heros, heroId)
            end
        end
    else
        table.insert(heros, heroId)
    end

    local level = 1
    local star = 1
    if #params > 2 then
        level = tonumber(params[3])
        if level > 100 then
            level = 100
        end
    end
    if #params > 3 then
        star = tonumber(params[4])
        if star > 10 then
            star = 10
        end
    end

    for k,v in pairs(heros) do
        local tpl = t.heros[v] --Todo: modify
        --local tpl = t.hero[v]  
        if tpl.isCanUse == 1 then
            if tpl then
                agent.hero.addHeroByCond(v,level,star)
            end
        end
    end

    chatGMResponse(session, true)
end

local yfbz = function(session, params)
    -- -- 增加玩家经验
    user.addExp(1000000000, true)
    -- 增加资源
    local count = tonumber(2000000000)
    user.addResource(1, count)
    user.addResource(2, count)
    user.addResource(3, count)
    user.addResource(4, count)
    user.addResource(5, count)
    user.addResource(6, count)
    -- 增加vip经验
    local exp = tonumber(100000000)
    agent.vip.addExp(exp, true)
    chatGMResponse(session, true)

    -- 开启全建筑，并且设置顶级
    for _,target in pairs(agent.building.list) do
        if target then
            if target.tplId > 0 then
                local tpl = t.building[target.tplId]
                target.level = #tpl.levels
                target.state = p.BuildingState.NORMAL
                target.sync = false
                target:onBuildFinish()
            end
        end
    end
    agent.building.sendBuildingQueuesUpdate()
    agent.building.sendBuildingUpdate()

    -- 全科技满级
    for _, v in pairs(t.technology) do
        agent.technology.addLevel(v.type,v.group)
    end

    -- 加武将
    local heros = {50002, 50004, 50005, 50007, 50010, 50012, 50015, 50019, 50024, 51001, 51002, 51003, 51004, 51005, 51006, 51007, 51008, 51009, 51010, 51011}        
    local level = 80
    local star = 9
    for k,v in pairs(heros) do
        local tpl = t.heros[v] --Todo: modify
        --local tpl = t.hero[v]  
        if tpl.isCanUse == 1 then
            if tpl then
                agent.hero.addHeroByCond(v,level,star)
            end
        end
    end

    -- 加兵
    army.add(p.ArmysType.INFANTRY, p.ArmyState.NORMAL, 1000000)
    army.add(p.ArmysType.RIDER, p.ArmyState.NORMAL, 1000000)
    army.add(p.ArmysType.ARCHER, p.ArmyState.NORMAL, 1000000)
    army.add(p.ArmysType.MECHANICAL, p.ArmyState.NORMAL, 1000000)

    -- 开启所有副本
    local allChapter = {}
    for k,v in pairs(t.scenarioChapter) do
        -- if string.find(v.remark, chapterName) then
            table.insert(allChapter, v.id)
        -- end
    end
    if #allChapter == 0 then
        chatGMResponse(session, false)
        return
    end
    local scenarioCopy = agent.scenarioCopy
    local starCount = 3
    for _, chapterId in pairs(allChapter) do
        local tpl = t.scenarioChapter[chapterId]
        for _, sectionId in pairs(tpl.sectionList) do
            if tpl and tpl.sections[sectionId]then
                local tplSec = tpl.sections[sectionId]
                if not tplSec.canRepeat then
                    starCount = 0
                else
                    starCount = 3
                end

                local chapter = scenarioCopy.chapterList[chapterId]
                if chapter then
                    local section = chapter.list[sectionId]
                    if section then
                        if section.star < starCount then
                            chapter.ownStar = chapter.ownStar + (starCount - section.star)
                            section.star = starCount
                        end
                        section.isPass = true
                        section.sync = false
                    else
                        section = scenarioCopy.createSectionInfo({ sectionId = sectionId, star = starCount, leftCount = tplSec.maxNum - 1, isPass = true })
                        chapter.list[sectionId] = section
                        chapter.ownStar = chapter.ownStar + starCount
                        if not tplSec.isBranch then
                            --不是分支关卡
                            chapter.curSectionNums = chapter.curSectionNums + 1
                        end
                        print("chapter.curSectionNums, tpl.maxSection", chapter.curSectionNums, tpl.maxSection)
                        if chapter.curSectionNums >= tpl.maxSection then
                            scenarioCopy.evtPassScenarioCopyChapter:trigger(tpl)
                            --下一章
                            scenarioCopy.setMaxChapter()
                        end
                    end
                    chapter.sync = false
                    scenarioCopy.sendScenarioChapterUpdate()
                end
                scenarioCopy.evtFightScenarioCopy:trigger(tplSec, true, 1, tpl)
            end
        end
    end
    chatGMResponse(session, true)
end

local chapter = function(session, params)
    if #params < 2 then
        chatGMResponse(session, false)
        return
    end
    local chapterId = tonumber(params[2])
    -- 开启所有副本
    local allChapter = {}
    table.insert(allChapter, chapterId)
    if #allChapter == 0 then
        chatGMResponse(session, false)
        return
    end
    local scenarioCopy = agent.scenarioCopy
    local starCount = 3
    for _, chapterId in pairs(allChapter) do
        local tpl = t.scenarioChapter[chapterId]
        for _, sectionId in pairs(tpl.sectionList) do
            if tpl and tpl.sections[sectionId]then
                local tplSec = tpl.sections[sectionId]
                if not tplSec.canRepeat then
                    starCount = 0
                else
                    starCount = 3
                end

                local chapter = scenarioCopy.chapterList[chapterId]
                if chapter then
                    local section = chapter.list[sectionId]
                    if section then
                        if section.star < starCount then
                            chapter.ownStar = chapter.ownStar + (starCount - section.star)
                            section.star = starCount
                        end
                        section.isPass = true
                        section.sync = false
                    else
                        section = scenarioCopy.createSectionInfo({ sectionId = sectionId, star = starCount, leftCount = tplSec.maxNum - 1, isPass = true })
                        chapter.list[sectionId] = section
                        chapter.ownStar = chapter.ownStar + starCount
                        if not tplSec.isBranch then
                            --不是分支关卡
                            chapter.curSectionNums = chapter.curSectionNums + 1
                        end
                        if chapter.curSectionNums >= tpl.maxSection then
                            scenarioCopy.evtPassScenarioCopyChapter:trigger(tpl)
                            --下一章
                            scenarioCopy.setMaxChapter()
                        end
                    end
                    chapter.sync = false
                    scenarioCopy.sendScenarioChapterUpdate()
                end
                scenarioCopy.evtFightScenarioCopy:trigger(tplSec, true, 1, tpl)
            end
        end
    end
    chatGMResponse(session, true)
end

local fuben = function(session, params)
    if #params < 2 then
        chatGMResponse(session, false)
        return
    end

    local chapterName = params[2]
    local sectionCount = 0
    if #params > 2 then
        sectionCount = tonumber(params[3])
    end

    local allChapter = {}
    for k,v in pairs(t.scenarioChapter) do
        if string.find(v.remark, chapterName) then
            table.insert(allChapter, v.id)
        end
    end

    if #allChapter == 0 then
        chatGMResponse(session, false)
        return
    end

    if sectionCount <= 0 then
        sectionCount = 100
    end

    local scenarioCopy = agent.scenarioCopy
    local starCount = 3
    for _, chapterId in pairs(allChapter) do
        local curCount = 0
        local tpl = t.scenarioChapter[chapterId]
        for _, sectionId in pairs(tpl.sectionList) do
            if tpl and tpl.sections[sectionId]then
                local tplSec = tpl.sections[sectionId]
                if not tplSec.canRepeat then
                    starCount = 0
                else
                    starCount = 3
                end

                local chapter = scenarioCopy.chapterList[chapterId]
                if chapter then
                    local section = chapter.list[sectionId]
                    if section then
                        if section.star < starCount then
                            chapter.ownStar = chapter.ownStar + (starCount - section.star)
                            section.star = starCount
                        end
                        section.isPass = true
                        section.sync = false
                    else
                        section = scenarioCopy.createSectionInfo({ sectionId = sectionId, star = starCount, leftCount = tplSec.maxNum - 1, isPass = true })
                        chapter.list[sectionId] = section
                        chapter.ownStar = chapter.ownStar + starCount
                        if not tplSec.isBranch then
                            --不是分支关卡
                            chapter.curSectionNums = chapter.curSectionNums + 1
                        end
                        if chapter.curSectionNums >= tpl.maxSection then
                            scenarioCopy.evtPassScenarioCopyChapter:trigger(tpl)
                            --下一章
                            scenarioCopy.setMaxChapter()
                        end
                    end
                    chapter.sync = false
                    scenarioCopy.sendScenarioChapterUpdate()
                end
                scenarioCopy.evtFightScenarioCopy:trigger(tplSec, true, 1, tpl)
            end
            curCount = curCount + 1
            if curCount >= sectionCount then
                break
            end
        end
    end

    chatGMResponse(session, true)
end

local finishquest = function(session, params)
    if #params < 2 then
        chatGMResponse(session, false)
        return
    end
    local questId = tonumber(params[2])
    local tpl = t.quest[questId]
    local questList = nil
    local qRecord = nil
    if not tpl then
        return
    end
    if tpl.type == p.QuestType.NOVICE then
        questList = agent.quest.noviceQuests
        qRecord = questList[questId]
    elseif tpl.type == p.QuestType.NORMAL then
        questList = agent.quest.normalQuests
        qRecord = questList[questId]
    end
    qRecord:Finish()
    agent.quest.sendQuestUpdate()
    chatGMResponse(session, true)
end

local getquest = function(session, params)
    if #params < 2 then
        chatGMResponse(session, false)
        return
    end

    local questId = tonumber(params[2])
    local tpl = t.quest[questId]
    if not tpl then
        return
    end

    if tpl.type == p.QuestType.NOVICE then
        -- 清空之前的主线任务
        for k, v in pairs(agent.quest.noviceQuests) do
            local qInfo = v:questInfo()
            qInfo:draw(timer.getTimestampCache())
            qInfo:setSync(false)
            agent.quest.questDrewInfos[k] = qInfo
            agent.quest.questFinishAndDraws[k] = k
            v = nil
        end
        for k, v in pairs(agent.quest.noviceQuests) do
            agent.quest.noviceQuests[k] = nil
        end

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
            agent.quest.noviceQuests[tpl.id] = q
        end
        q:onInit()

    elseif tpl.type == p.QuestType.NORMAL then
        local info = questInfo:new({tpl = tpl, uid = user.uid})
        info.total = tpl.count
        info.createTime = timer.getTimestampCache()
        info:setDirty()
    
        local q = questProducer.createQuest(agent, info)
        agent.quest.normalQuests[tpl.id] = q
        q:onInit()
    end
    agent.quest.sendQuestUpdate()
    chatGMResponse(session, true)
end

M.register("addres", addres)
M.register("additem", additem)
M.register("addexp", addexp)
M.register("vipexp", vipexp)
M.register("upbuild", upbuild)
M.register("qcl", qcl)
M.register("hero", hero)
M.register("yfbz", yfbz)
M.register("chapter", chapter)
M.register("fuben", fuben)
M.register("finishquest", finishquest)
M.register("getquest", getquest)

return M