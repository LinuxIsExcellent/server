local agent = ...
local p = require('protocol')
local t = require('tploader')
local utils = require('utils')
local event = require('libs/event')

local dict = agent.dict
local user = agent.user
local hero = agent.hero
local bag = agent.bag
local cdlist = agent.cdlist
local vip = agent.vip


--关卡结构
local sectionInfo = {
    sectionId = 0,  --关卡ID
    star = 0,       --通关星星数量
    leftCount = 0,  --剩余闯关、扫荡次数
    resetCount = 0,  --已重置次数
    isPass = false, --是否通关
    extDropList = {},   --[tplId]=count

    sync = false,
}

--章节结构
local chapterInfo = {
    chapterId = 0,      --章节ID
    type = 0,           --章节类型ChapterType
    maxStar = 0,        --章节星星最大数量
    ownStar = 0,        --拥有星星数量
    curSectionNums = 0, --章节已通关关卡数
    list = {},          --已通关的关卡信息 <关卡ID, sectionInfo>

    sync = false,
}

local scenarioCopy = {
    maxChapterList = {},    --可挑战的最大章节<type,maxChapterId>
    chapterList = {},       --<章节ID, chapterInfo>
    drewList = {},          --<章节ID, table<target,1>>
    confList = {},             ----用于保存成功出征的队伍信息***
    evtFightScenarioCopy = event.new(),  --挑战副本关卡
    evtPassScenarioCopyChapter = event.new(),  --通关副本章节
    evtPassScenarioStarRefresh = event.new(),    --刷新副本通关星星
    evtScenarioMopup = event.new(),    -- 扫荡副本 (chapterId)
}

local pktHandlers = {}

function sectionInfo:new(o)
    o = o or {}
    if o.extDropList == nil then
        o.extDropList = {}
    end
    setmetatable(o,self)
    self.__index = self
    return o
end

function chapterInfo:new(o)
    o = o or {}
    if o.list == nil then
        o.list = {}
    end
    setmetatable(o,self)
    self.__index = self
    return o
end

function scenarioCopy.createSectionInfo(o)
    return sectionInfo:new(o)
end

function scenarioCopy.onInit()
    agent.registerHandlers(pktHandlers)
    scenarioCopy.dbLoad()

    if user.newPlayer or not next(scenarioCopy.chapterList) then
        local firstChapterId = 2600001
        --默认开启第一章
        scenarioCopy.maxChapterList[p.ChapterType.NORMAL] = firstChapterId
        local tpl = t.scenarioChapter[firstChapterId]
        if tpl == nil then
            utils.log('scenarioCopy first chapter is error chapterId = ', firstChapterId)
            return
        end
        local maxStar = tpl.maxStar
        scenarioCopy.chapterList[firstChapterId] = chapterInfo:new({ chapterId = firstChapterId, type = p.ChapterType.NORMAL, maxStar = maxStar, ownStar = 0 })
    end

    if cdlist.isHour5Refresh then
        if not user.newPlayer then
            scenarioCopy.refresh(false)
        end
    end
    cdlist.evtHour5Refresh:attachRaw(function()
        scenarioCopy.refresh(true)
    end)

    agent.combat.evtCombatOver:attachRaw(scenarioCopy.onCombatOver)
end

function scenarioCopy.onAllCompInit()
    scenarioCopy.sendScenarioChapterUpdate()
    scenarioCopy.sendScenarioStarRewardUpdate()
    --scenarioCopy.sendScenarioTeamInfoUpdate()
end

function scenarioCopy.onSave()
    scenarioCopy.dbSave()
end

function scenarioCopy.onClose()
    scenarioCopy.dbSave()
end

function scenarioCopy.dbLoad()
    local chapterList = dict.get("scenarioCopy.chapterList") or {}

    for chapterId, v in pairs(chapterList) do
        -- print('scenarioCopy.dbLoad......chapterId, v', chapterId, utils.serialize(v))
        local tpl = t.scenarioChapter[chapterId]
        if tpl then
            local list = {}
            local ownStar = 0
            local curSectionNums = 0
            for _, sec in pairs(v.list) do
                -- print('scenarioCopy.dbLoad...sectionId, isPass', sec.sectionId, sec.isPass, utils.serialize(sec.extDropList))
                list[sec.sectionId] = sectionInfo:new(sec)
                ownStar = ownStar + sec.star
                local tplSec = tpl.sections[sec.sectionId]
                if tplSec and not tplSec.isBranch then
                    curSectionNums = curSectionNums + 1
                end
            end

            local info = chapterInfo:new({ chapterId = chapterId, type = v.type, maxStar = tpl.maxStar, ownStar = ownStar, curSectionNums = curSectionNums, list = list })
            scenarioCopy.chapterList[chapterId] = info
        end
    end
    scenarioCopy.setMaxChapter()
    -- print('scenarioCopy.dbLoad..scenarioCopy.chapterList', utils.serialize(scenarioCopy.chapterList))

    scenarioCopy.drewList = dict.get("scenarioCopy.drewList") or {}
    -- print('scenarioCopy.dbLoad...drewList=', utils.serialize(scenarioCopy.drewList))
    scenarioCopy.confList = dict.get("scenarioCopy.confList") or {}
    -- print('scenarioCopy.dbLoad...drewList=', utils.serialize(scenarioCopy.drewList))
end


function scenarioCopy.dbSave()
    local chapterList = {}
    for _, v in pairs(scenarioCopy.chapterList) do
        local secList = {}
        for _, sec in pairs(v.list) do
            -- print('scenarioCopy.dbSave...sectionId, isPass', sec.sectionId, sec.isPass)
            secList[sec.sectionId] = { sectionId = sec.sectionId, star = sec.star, leftCount = sec.leftCount, resetCount = sec.resetCount, isPass = sec.isPass, extDropList = sec.extDropList }
        end
        chapterList[v.chapterId] = {}
        chapterList[v.chapterId].list = secList
        chapterList[v.chapterId].type = v.type
    end
    -- print('scenarioCopy.dbSave...chapterList=', utils.serialize(chapterList))
    dict.set('scenarioCopy.chapterList', chapterList)

    -- print('scenarioCopy.dbSave...drewList=', utils.serialize(scenarioCopy.drewList))
    dict.set('scenarioCopy.drewList', scenarioCopy.drewList)
    -- print('scenarioCopy.dbSave...confList=', utils.serialize(scenarioCopy.confList))
    dict.set('scenarioCopy.confList', scenarioCopy.confList)
end

function scenarioCopy.refresh(sendUpdate)
    for cid, chapter in pairs(scenarioCopy.chapterList) do
        local tpl = t.scenarioSection[cid]
        for sid, section in pairs(chapter.list) do
            local maxNum = 0
            if tpl and tpl[sid] then
                maxNum = tpl[sid].maxNum
            end
            -- print('scenarioCopy.refresh...cid, sid, maxNum', cid, sid, maxNum)
            section.leftCount = maxNum
            section.resetCount = 0
            section.sync = 0
        end
    end

    if sendUpdate then
        scenarioCopy.sendScenarioChapterUpdate()
    end
end

function scenarioCopy.setMaxChapter()
    print('scenarioCopy.setMaxChapter...old maxChapterList=', utils.serialize(scenarioCopy.maxChapterList))
    local list = {}
    for chapterId, info in pairs(scenarioCopy.chapterList) do
        local tpl = t.scenarioChapter[chapterId]
        if tpl then
            if tpl.type ~= info.type then
                info.type = tpl.type
            end
            if list[tpl.type] == nil then
                list[tpl.type] = {}
                list[tpl.type].maxChapterId = chapterId
                list[tpl.type].curSectionNums = info.curSectionNums
                scenarioCopy.maxChapterList[tpl.type] = chapterId
            end
            if list[tpl.type].maxChapterId < chapterId then
                list[tpl.type].maxChapterId = chapterId
                list[tpl.type].curSectionNums = info.curSectionNums
                scenarioCopy.maxChapterList[tpl.type] = chapterId
            end
        end
    end
    --设置下一章
    for k, v in pairs(list) do
        local tpl = t.scenarioChapter[v.maxChapterId]
        if tpl then
            if v.curSectionNums >= tpl.maxSection then
                for _, nextId in pairs(tpl.nextId) do
                    ---判定下一章是否可以开启
                    local nextTpl = t.scenarioChapter[nextId]
                    if nextTpl then
                        local bNew = true
                        --nextId该章的前置章是否都已经通关了
                        for _, preId in pairs(nextTpl.preId) do
                            local preChapter = scenarioCopy.chapterList[preId]
                            local preTpl = t.scenarioChapter[preId]
                            if preChapter == nil or preTpl == nil or preChapter.curSectionNums < preTpl.maxSection then
                                bNew = false
                                break
                            end
                        end
                        if bNew then
                            local info = chapterInfo:new({ chapterId = nextId, type = nextTpl.type, maxStar = nextTpl.maxStar, ownStar = 0, curSectionNums = 0 })
                            scenarioCopy.chapterList[nextId] = info
                            scenarioCopy.maxChapterList[nextTpl.type] = nextId
                        end
                    end
                end
            end
        end
    end
    -- print('scenarioCopy.setMaxChapter...new maxChapterList=', utils.serialize(scenarioCopy.maxChapterList))
end

function scenarioCopy.getSection(sectionId)
    for _, chapter in pairs(scenarioCopy.chapterList) do
        if chapter.list[sectionId] then
            return chapter.list[sectionId]
        end
    end
    return nil
end

function scenarioCopy.pickDropItems(drops, gainType, heroList)
    -- print('scenarioCopy.pickDropItems....drops, gainType, heroList', utils.serialize(drops), gainType, heroList)
    if not gainType then
        gainType = p.ResourceGainType.SCENARIO_COPY
    end
    for _, drop in pairs(drops) do
        if drop.tplId == p.SpecialPropIdType.FOOD then
            user.addResource(p.ResourceType.FOOD, drop.count, gainType, 0, false)
        elseif drop.tplId == p.SpecialPropIdType.WOOD then
            user.addResource(p.ResourceType.WOOD, drop.count, gainType, 0, false)
        elseif drop.tplId == p.SpecialPropIdType.IRON then
            user.addResource(p.ResourceType.IRON, drop.count, gainType, 0, false)
        elseif drop.tplId == p.SpecialPropIdType.STONE then
            user.addResource(p.ResourceType.STONE, drop.count, gainType, 0, false)
        elseif drop.tplId == p.SpecialPropIdType.GOLD then
            user.addResource(p.ResourceType.GOLD, drop.count, gainType, 0, false)
        elseif drop.tplId == p.SpecialPropIdType.SILVER then
            user.addResource(p.ResourceType.SILVER, drop.count, gainType, 0, false)
        elseif drop.tplId == p.SpecialPropIdType.STAMINA then
            user.addStamina(drop.count, gainType)
        elseif drop.tplId == p.SpecialPropIdType.LORD_EXP then
            user.addExp(drop.count, false)
        elseif drop.tplId == p.SpecialPropIdType.VIP_EXP then
        elseif drop.tplId == p.SpecialPropIdType.HONOR then
        elseif drop.tplId == p.SpecialPropIdType.HERO_EXP then
            if heroList then
                for _, v in pairs(heroList) do
                     hero.addHeroExp(v.heroId, drop.count)
                end
            end
        else
            --item
            if t.item[drop.tplId] == nil then
                print('tpl_item not exist tplId=', drop.tplId)
            else
                bag.addItem(drop.tplId, drop.count, gainType, false)
            end
        end
    end
    hero.sendHeroUpdate()
    bag.sendBagUpdate()
    user.sendUpdate()
end

--解锁条件
function scenarioCopy.checkUnlockCond(list)
    -- print('scenarioCopy.checkUnlockCond...list', utils.serialize(list))
    return agent.systemUnlock.checkUnlockedByList(list)
end

--X星通关计算  
function scenarioCopy.calculateStarByCombatResult(result, starList , type)
    local starCount = 0
    local compData = math.floor(0)
    local star1cond = starList[3] or 0
    local star2cond = starList[2] or 0
    local star3cond = starList[1] or 0
    -- type 0: 单挑  1: 混战   2:混合战斗
    if type == 0 then
        -- 失败次数/单挑总次数 = 百分比判定条件  在配置表配置
        local winRound = result.winRound or 0
        local totalRound = result.totalRound or 0
        if totalRound == 0 or winRound == 0 then
            return 0
        end
        if totalRound > 0 then
            compData = math.floor((totalRound - winRound)/totalRound * 100)
            -- print('totalRound, winRound, compData, star1cond, star2cond, star3cond', totalRound, winRound, compData, star1cond, star2cond, star3cond)     
        end 
    elseif type == 1 then
        -- 损失兵力/总兵力 = 百分比判定条件  在配置表配置
        local left = result.leftArmys or 0
        local total = result.totalArmys or 0
        if total == 0 or left == 0 then
            return 0
        end
        if total > 0 then
            compData = math.floor((total - left)/total * 100)
            -- print('total, left, compData, star1cond, star2cond, star3cond', total, left, compData, star1cond, star2cond, star3cond)
        end
    elseif type == 2 then
        --给混合战斗预留
        return 3
    end
    -- 判断星级
    if compData <= star3cond then
        starCount = 3
    elseif compData <= star2cond then
        tarCount = 2
    else
        starCount = 1
    end
    return starCount
end

function scenarioCopy.onCombatOver(battleId, battleType, isWin, dataOnCombatOver, attackWinRound, maxRound, beginArmys, endArmys)
    if battleType == p.BattleType.SCENARIO_COPY_COMBAT or battleType == p.BattleType.SCENARIO_COPY_ANSWER then
        -- print('###scenarioCopy.onCombatOver...battleId, battleType, isWin, result, dataOnCombatOver', battleId, battleType, isWin, utils.serialize(result), utils.serialize(dataOnCombatOver))
        local chapterId = dataOnCombatOver.chapterId
        local sectionId = dataOnCombatOver.sectionId
        local heroList = dataOnCombatOver.heroList or {}
        local oldLevel = user.info.level or 1
        local oldExp = user.info.exp or 0
        local starCount = 0
        local firstList = {}
        local publicList = {}
        local baseList = {}
        --print('###scenarioCopy.onCombatOver...result',utils.serialize(result))
        if isWin then
            local tpl = t.scenarioChapter[chapterId]
            if tpl and tpl.sections[sectionId]then
                local tplSec = tpl.sections[sectionId]
                --1.根据战斗结果 判断通关的星级
                local starResult = {}
                --{["attack"]={["oldPower"]=7789,["newPower"]=3438,},["defense"]={["oldPower"]=38684,["newPower"]=10899,},}
                if dataOnCombatOver.isBattle then
                    -- levelType 0: 单挑  1: 混战   2:混合战斗
                    local levelType = tplSec.levelType 
                    starResult = { leftArmys = endArmys, totalArmys = beginArmys, winRound = attackWinRound, totalRound = maxRound}
                    if tplSec.canRepeat then
                        starCount = scenarioCopy.calculateStarByCombatResult(starResult, tplSec.starList , levelType)
                    end
                    print('###scenarioCopy.onCombatOver..levelType,starCount', levelType,starCount)
                else
                    --答题不计算星级
                     starCount = 0 
                end

                local chapter = scenarioCopy.chapterList[chapterId]
                if chapter then
                    --2.普通章节和精英副本消耗体力
                    if tpl.type == p.ChapterType.NORMAL or tpl.type == p.ChapterType.ELITE then
                        user.removeStamina(tplSec.stamina, p.StaminaConsumeType.SCENARIO_COPY)
                    end
                    --3.获取掉落<首通，普通>
                    local publicDropTpl = t.drop[tplSec.publicDropId]
                    if publicDropTpl then
                        publicList = publicDropTpl:DoDrop()
                    end
                    --基础掉落
                    for tplId, count in pairs(tplSec.rewardList) do
                        table.insert(publicList, { tplId = tplId, count = count })
                    end
                    --4.更新本地数据
                    local section = chapter.list[sectionId]
                    if section then
                        if section.star < starCount then
                            chapter.ownStar = chapter.ownStar + (starCount - section.star)
                            section.star = starCount
                        end
                        section.leftCount = section.leftCount - 1
                        section.isPass = true
                        section.sync = false
                    else
                        --首通掉落
                        for tplId, count in pairs(tplSec.firstRewardList) do
                            table.insert(firstList, t.createDropItem(tplId, count))
                        end

                        section = sectionInfo:new({ sectionId = sectionId, star = starCount, leftCount = tplSec.maxNum - 1, isPass = true })
                        chapter.list[sectionId] = section
                        chapter.ownStar = chapter.ownStar + starCount
                        print('scenarioCopy.onCombatOver....isBranch, oldSectionNums, newSectionNums', tplSec.isBranch, chapter.curSectionNums, chapter.curSectionNums+1)
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
                    scenarioCopy.evtPassScenarioStarRefresh:trigger(chapter.chapterId, chapter.ownStar)
                    chapter.sync = false
                    --额外掉落
                    for _, v in pairs(tplSec.extDropList) do
                        local tplId = v[1]
                        local count = v[2]
                        local weight = v[3]
                        local maxNums = v[4]
                        local fightCount = section.extDropList[tplId] or 0
                        -- print('###scenarioCopy.onCombatOver, ext drop..tplId, count, weight, maxNums, fightCount', tplId, count, weight, maxNums, fightCount, utils.serialize(v))
                        local randValue = utils.getRandomNum(1, 10000)
                        if randValue <= weight or (fightCount+1) >= maxNums then
                            table.insert(publicList, { tplId = tplId, count = count })
                            section.extDropList[tplId] = 0
                        else
                            section.extDropList[tplId] = fightCount + 1
                        end
                    end
                    scenarioCopy.sendScenarioChapterUpdate()
                end
                --pick
                local pickList = {}
                for _, v in pairs(publicList) do
                    local item = pickList[v.tplId]
                    if item == nil then
                        pickList[v.tplId] = { tplId = v.tplId, count = v.count }
                    else
                        item.count = item.count + v.count
                    end
                end
                for _, v in pairs(firstList) do
                    local item = pickList[v.tplId]
                    if item == nil then
                        pickList[v.tplId] = { tplId = v.tplId, count = v.count }
                    else
                        item.count = item.count + v.count
                    end
                end
                -- print('###scenarioCopy.onCombatOver...pickList', utils.serialize(pickList))
                scenarioCopy.pickDropItems(pickList ,p.ResourceGainType.SCENARIO_COPY, heroList)
                scenarioCopy.evtFightScenarioCopy:trigger(tplSec, isWin, 1, tpl)
            end
        end
        -- print('###scenarioCopy.onCombatOver...over....battleId, battleType, isWin', battleId, battleType, isWin)
        print('###scenarioCopy.onCombatOver...firstList, publicList', utils.serialize(firstList),utils.serialize(publicList))
        --战斗结算
        agent.sendPktout(p.SC_SCENARIO_BATTLE_RESULT,'@@1=i,2=i,3=i,4=b,5=i,6=i,7=i,8=[tplId=i,count=i],9=[tplId=i,count=i],10=[heroId=i,oldLevel=i,oldExp=i]',battleType, chapterId, sectionId, isWin, starCount, oldLevel, oldExp, firstList, publicList, heroList)
    end
end

function scenarioCopy.sendScenarioTeamInfoUpdate()
    local updateList = {}
    updateList = scenarioCopy.confList
    if next(updateList) then
         -- print('###scenarioCopy.sendScenarioTeamInfosUpdate..updateList', utils.serialize(updateList))
        agent.sendPktout(p.SC_SCENARIO_TEAM_INFOS_UPDATE, '@@1=[heroId=i,type=i,count=i,position=i]', updateList)
    end
end

function scenarioCopy.sendScenarioChapterUpdate()
    local updateList = {}
    for _, chapter in pairs(scenarioCopy.chapterList) do
        -- print('scenarioCopy.sendScenarioChapterUpdate....chapterId, curSectionNums', chapter.chapterId, chapter.curSectionNums)
        if not chapter.sync then
            chapter.sync = true
            local list = {}
            local tpl = t.scenarioChapter[chapter.chapterId]
            for _, section in pairs(chapter.list) do
                --TODO:前端说要当前章全部已通关的关卡都推送,看数据量是否大，大则需要优化
                -- if not section.sync then
                    section.sync = true
                    local tplSec = tpl.sections[section.sectionId]
                    local resetPrice = tplSec.resetPrice
                    local nextResetCount = section.resetCount + 1
                    local cost = resetPrice[nextResetCount] or resetPrice[(#resetPrice)]
                    local maxCount = tplSec.maxNum

                    local leftResetCount = vip.tpl.copyResetCount - section.resetCount
                    table.insert(list, { sectionId = section.sectionId, starCount = section.star, maxNum = maxCount, leftCount = section.leftCount, leftResetCount = leftResetCount, resetCost = cost})
                    -- print('scenarioCopy.sendScenarioChapterUpdate....sectionId, isPass, ', section.sectionId, section.isPass)
                    -- print("LeftResetCount... chapter.chapterId, section.sectionId, leftResetCount, section.leftCount", vip.tpl.copyResetCount - section.resetCount, chapter.chapterId, section.sectionId, leftResetCount, section.leftCount)

                -- end
            end

            table.insert(updateList, { chapterId = chapter.chapterId, type = chapter.type, ownStar = chapter.ownStar, maxStar = chapter.maxStar, sectionList = list })
        end
    end
    local maxList = {}
    for k, v in pairs(scenarioCopy.maxChapterList) do
        table.insert(maxList, { maxChapterId = v, type = k })
    end
    -- print('scenarioCopy.sendScenarioChapterUpdate...maxList', utils.serialize(maxList))
    -- print('scenarioCopy.sendScenarioChapterUpdate...updateList', utils.serialize(updateList))
    -- if next(updateList) or next(maxList) then
        agent.sendPktout(p.SC_SCENARIO_CHAPTER_UPDATE, '@@1=[maxChapterId=i,type=i],2=[chapterId=i,type=i,ownStar=i,maxStar=i,sectionList=[sectionId=i,starCount=i,maxNum=i,leftCount=i,leftResetCount=i,resetCost=i]]', maxList, updateList)
    -- end
end

function scenarioCopy.sendScenarioStarRewardUpdate(updates)
    local updateList = {}
    if not updates then
        for chapterId, drews in pairs(scenarioCopy.drewList) do
            local drewList = {}
            for target, v in pairs(drews) do
                table.insert(drewList, { target = target })
            end
            -- print('scenarioCopy.sendScenarioStarRewardUpdate..chapterId, drewList=', chapterId, utils.serialize(drewList))
            table.insert(updateList, { chapterId = chapterId, drewList = drewList })
        end
    else
        updateList = updates
    end
    --print('scenarioCopy.sendScenarioStarRewardUpdate...updateList', utils.serialize(updateList))
    if next(updateList) then
        agent.sendPktout(p.SC_SCENARIO_STAR_REWARD_UPDATE, '@@1=[chapterId=i,drewList=[target=i]]', updateList)
    end
end


pktHandlers[p.CS_SCENARIO_FIGHT] = function(pktin, session)
    local function scenarioFightResponse(result, battleId, warReport)
        print('scenarioFightResponse', result, battleId)
        agent.replyPktout(session, p.SC_SCENARIO_FIGHT_RESPONSE, result, battleId, warReport)
    end

    local battleId = 0
    local warReport 
    local chapterId, sectionId, size = pktin:read('iii')

    print("scenario ------------ chapterId, sectionId !!! ", chapterId, sectionId)

    --1.检查配置表
    local tpl = t.scenarioChapter[chapterId]
    if not tpl then
        print('p.CS_SCENARIO_FIGHT..chapter is not exists..chapterId=', chapterId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local tplSec = tpl.sections[sectionId]
    if not tplSec then
        print('p.CS_SCENARIO_FIGHT..section is not exists..sectionId=', sectionId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    -- print('p.CS_SCENARIO_FIGHT...tplSec', var_dump(tplSec))
    print('p.CS_SCENARIO_FIGHT...chapterId, sectionId, size', chapterId, sectionId, size)

    if size <= 0 then
        print('p.CS_SCENARIO_FIGHT...size=', size)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local armyList = {}
    local heroList ={}
    local confList = {}
    local ownHeroCount = 0
    for i=1, size do
        local heroId, armyType, count, position = pktin:read('iiii')
        print('p.CS_SCENARIO_FIGHT...heroId, armyType, count, position', heroId, armyType, count, position)
        if heroId > 0 and t.heros[heroId] then
         --TODO:
            -- print("heroId --- ", heroId, " --- count ", count)
            local level = agent.army.getArmyLevelByArmyType(armyType)
            armyList[heroId] = { heroId = heroId, armyType = armyType, level = level, count = count, position = position }           
            local level = 0
            local exp = 0
            local info = hero.info[heroId]
            if info then
                ownHeroCount = ownHeroCount + 1
                table.insert(confList,{ heroId = heroId, armyType = armyType, count = count, position = position })
                level = info.level or 0
                exp = info.exp or 0
            end
            table.insert(heroList, { heroId = heroId, oldLevel = level, oldExp = exp })
        end
        
        if not next(armyList) then
            print('p.CS_SCENARIO_FIGHT...armyList is empty')
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            return
        end
    end
    -- print('-p.CS_SCENARIO_FIGHT...armyList', utils.serialize(armyList))
    --2.检查该关卡是否可以挑战
    local maxChapterId = scenarioCopy.maxChapterList[tpl.type] or 0
    if maxChapterId < chapterId then
        print('p.CS_SCENARIO_FIGHT..the previous chapter did not pass..chapterId > maxChapterId=', chapterId, maxChapterId)
        agent.sendNoticeMessage(p.ErrorCode.SCENARIO_NOT_UNLOCK, '', 1)
        return
    end
    --解锁条件
    if not scenarioCopy.checkUnlockCond(tplSec.condList) then
        print('p.CS_SCENARIO_FIGHT..not up to the challenge...chapterId=', chapterId)
        agent.sendNoticeMessage(p.ErrorCode.SCENARIO_NOT_UNLOCK, '', 1)
        return
    end
    --3.检查前置关卡是否已经通关
    local chapter = scenarioCopy.chapterList[chapterId]
    if not chapter then
        print('p.CS_SCENARIO_FIGHT..the chapter did not pass...chapterId', chapterId)
        agent.sendNoticeMessage(p.ErrorCode.SCENARIO_NOT_UNLOCK, '', 1)
        return
    end
    for _, preId in pairs(tplSec.preId) do
        if preId > 0 then
            local preSection = scenarioCopy.getSection(preId)
            if preSection == nil or not preSection.isPass then
                print('p.CS_SCENARIO_FIGHT..the previous section did not pass...preSection, sectionId, preId', preSection, sectionId, preId)
                agent.sendNoticeMessage(p.ErrorCode.SCENARIO_NOT_UNLOCK, '', 1)
                return
            end
        end
    end
    --4.是否可以闯关 次数 是否可以重复刷
    local section = chapter.list[sectionId]
    if section and (section.leftCount <= 0 or not tplSec.canRepeat) then
        print('p.CS_SCENARIO_FIGHT..can not fight...sectionId, leftCount, canRepeat=', sectionId, section.leftCount, tplSec.canRepeat)
        agent.sendNoticeMessage(p.ErrorCode.SCENARIO_FIGHT_COUNT_NOT_ENOUGH, '', 1)
        return
    end
    -- --5.检查体力是否足够
    if not user.isStaminaEnough(tplSec.stamina) then
         print('p.CS_SCENARIO_FIGHT..Stamina is not Enough')
         agent.sendNoticeMessage(p.ErrorCode.PUBLIC_STAMINA_NOT_ENOUGH, '', 1)
         return
    end
    if tplSec.battleType ~= 2 then  
        if tplSec.armyGroup <= 0 or tplSec.armyCount <= 0 then
            print('p.CS_SCENARIO_FIGHT..army group ie error...armyGroup, armyCount', tplSec.armyGroup, tplSec.armyCount)
            agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
            return
        end
        --调用战斗接口  事件处理结果
        local dataOnCombatOver = { chapterId = chapterId, sectionId = sectionId, heroList = heroList, isBattle = true }
        local extParam = { heroConf = tplSec.heroConf, singleCombat = tplSec.singleCombat, singleCombatHero = tplSec.singleCombatHero , levelType = tplSec.levelType}
        battleId, warReport = agent.combat.createBattleByArmyList(p.BattleType.SCENARIO_COPY_COMBAT, armyList, tplSec.armyGroup, tplSec.armyCount, dataOnCombatOver, extParam)
        
        -- print('p.CS_SCENARIO_FIGHT...tplSec.battleType, battleId, warReport', tplSec.battleType, battleId, utils.serialize(warReport))
        scenarioFightResponse(p.ErrorCode.SUCCESS, battleId, warReport)
        --if size == ownHeroCount then
        --   scenarioCopy.confList = confList
        --   scenarioCopy.sendScenarioTeamInfoUpdate()
        --end
       return  
    end

    agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
end

pktHandlers[p.CS_SCENARIO_RESET] = function(pktin, session)
    local chapterId, sectionId = pktin:read('ii')
    -- 1.检测章节
    -- print("chapterId, sectionId", chapterId, sectionId)
    local chapter = scenarioCopy.chapterList[chapterId]
    if not chapter then
        print('p.CS_SCENARIO_RESET..can not move up...chapterId', chapterId)
        -- scenarioMoveUpResponse(p.ErrorCode.FAIL)
        agent.sendNoticeMessage(p.ErrorCode.SCENARIO_NOT_UNLOCK, '', 1)
        return
    end
    -- 2.检测关卡
    local section = chapter.list[sectionId]
    if not section then
        print('p.CS_SCENARIO_RESET..can not move up...sectionId', sectionId)
        -- scenarioMoveUpResponse(p.ErrorCode.FAIL)
        agent.sendNoticeMessage(p.ErrorCode.SCENARIO_NOT_UNLOCK, '', 1)
        return
    end
    -- 3.检测重置次数
    if section.resetCount >= vip.tpl.copyResetCount then
        print('p.CS_SCENARIO_RESET..section is not resetCount..sectionId=', sectionId)
        agent.sendNoticeMessage(p.ErrorCode.SCENARIO_NOT_LEFT_RESET_COUNT, '', 1)
        return
    end
    -- 4.扣钱
    local tpl = t.scenarioChapter[chapterId]
    local tplSec = tpl.sections[sectionId]
    if not tplSec or not tpl then
        print('p.CS_SCENARIO_RESET..section is not exists..sectionId=', sectionId)
        -- scenarioMoveUpResponse(p.ErrorCode.FAIL, oldLevel, oldExp)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local resetPrice = tplSec.resetPrice
    if not resetPrice then
        print('p.CS_SCENARIO_RESET..resetPrice is not exists..sectionId=', sectionId)
        -- scenarioMoveUpResponse(p.ErrorCode.FAIL, oldLevel, oldExp)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

     -- 5.增加已经重置的次数
    local tpl = t.scenarioSection[chapterId]
    local maxNum = 0
    if tpl and tpl[sectionId] then
        maxNum = tpl[sectionId].maxNum
    end
    section.leftCount = maxNum
    section.resetCount = section.resetCount + 1
    local cost = resetPrice[section.resetCount] or resetPrice[(#resetPrice)]
    user.removeResource(p.ResourceType.GOLD, cost, p.ResourceConsumeType.RESET_SCENARIOCOPY)
    chapter.sync = false
    section.sync = false
    scenarioCopy.sendScenarioChapterUpdate()
    agent.replyPktout(session, p.SC_SCENARIO_RESET_RESPONSE, '@@1=i',p.ErrorCode.SUCCESS)
end

pktHandlers[p.CS_SCENARIO_MOPUP] = function(pktin, session)
    local function scenarioMoveUpResponse(result, oldLevel, oldExp, list)
        if list == nil then
            list = {}
        end
        print('p.CS_SCENARIO_MOPUP..result, oldLevel, oldExp, list', result, oldLevel, oldExp, list)
        agent.replyPktout(session, p.SC_SCENARIO_MOPUP_RESPONSE, '@@1=i,2=i,3=i,4=[publicList=[tplId=i,count=i]]',result, oldLevel, oldExp, list)
    end

    local chapterId, sectionId, count = pktin:read('iii')
    -- print('p.CS_SCENARIO_MOPUP...chapterId, sectionId, count', chapterId, sectionId, count)

    local oldLevel = user.info.level or 1
    local oldExp = user.info.exp or 0

    --1.检查配置表
    local tpl = t.scenarioChapter[chapterId]
    if not tpl then
        print('p.CS_SCENARIO_MOPUP..chapter is not exists..chapterId=', chapterId)
        -- scenarioMoveUpResponse(p.ErrorCode.FAIL, oldLevel, oldExp)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local tplSec = tpl.sections[sectionId]
    if not tplSec then
        print('p.CS_SCENARIO_MOPUP..section is not exists..sectionId=', sectionId)
        -- scenarioMoveUpResponse(p.ErrorCode.FAIL, oldLevel, oldExp)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --2.是否可以扫荡 3星 闯关次数 是否可以重复刷
    local chapter = scenarioCopy.chapterList[chapterId]
    if not chapter then
        print('p.CS_SCENARIO_MOPUP..can not move up...chapterId', chapterId)
        -- scenarioMoveUpResponse(p.ErrorCode.FAIL)
        agent.sendNoticeMessage(p.ErrorCode.SCENARIO_NOT_UNLOCK, '', 1)
        return
    end
    local section = chapter.list[sectionId]
    if not section or not tplSec.canRepeat then
        print('p.CS_SCENARIO_MOPUP..can not move up...sectionId', sectionId)
        -- scenarioMoveUpResponse(p.ErrorCode.FAIL)
        agent.sendNoticeMessage(p.ErrorCode.PUBLIC_RESOURCE_NOT_ENOUGH, '', 1)
        return
    end
    if section.star ~= 3 then
        agent.sendNoticeMessage(p.ErrorCode.SCENARIO_NO_THREE_STAR, '', 1)
        return
    end
    --vip
    if count == 1 then
        if not vip.tpl.scenarioOneMopup and not vip.tpl.scenarioManyMopup then
            agent.sendNoticeMessage(p.ErrorCode.PUBLIC_VIP_OPEN, '', 1)
            return
        end
    elseif count > 1 then
        if not vip.tpl.scenarioManyMopup then
            agent.sendNoticeMessage(p.ErrorCode.PUBLIC_VIP_OPEN, '', 1)
            return
        end
    end

    if section.leftCount < count then
        agent.sendNoticeMessage(p.ErrorCode.SCENARIO_FIGHT_COUNT_NOT_ENOUGH, '', 1)
        return
    end
     --3.检查体力是否足够
    if not user.isStaminaEnough(tplSec.stamina * count) then
         print('p.CS_SCENARIO_MOPUP..Stamina is not Enough')
          scenarioMoveUpResponse(p.ErrorCode.FAIL)
         agent.sendNoticeMessage(p.ErrorCode.PUBLIC_STAMINA_NOT_ENOUGH, '', 1)
         return
    end
    user.removeStamina(tplSec.stamina * count, p.StaminaConsumeType.SCENARIO_COPY)
    
    --pick drops
    local list = {}
    local pickList = {}
    local totalCount = count
    while count > 0 do
        local publicList = {}
        local dropTpl = t.drop[tplSec.publicDropId]
        if dropTpl then
            publicList = dropTpl:DoDrop()
        end
        for _, v in pairs(tplSec.extDropList) do
            local tplId = v[1]
            local count = v[2]
            local weight = v[3]
            local maxNums = v[4]
            local fightCount = section.extDropList[tplId] or 0
            -- print('###p.CS_SCENARIO_MOPUP, ext drop..tplId, fightCount, maxNums', tplId, fightCount, maxNums)
            local randValue = utils.getRandomNum(1, 10000)
            if randValue <= weight or (fightCount+1) >= maxNums then
                table.insert(publicList, { tplId = tplId, count = count })
                section.extDropList[tplId] = 0
            else
                section.extDropList[tplId] = fightCount + 1
            end
        end
        for _, v in pairs(publicList) do
            table.insert(pickList, { tplId = v.tplId, count = v.count })
        end

        for tplId, count in pairs(tplSec.rewardList) do
            if tplId ~= p.SpecialPropIdType.HERO_EXP then
                table.insert(publicList, { tplId = tplId, count = count })
                table.insert(pickList, { tplId = tplId, count = count })
            end
        end

       -- for tplId, count in pairs(tplSec.mopupRewardList) do
        --    table.insert(publicList, { tplId = tplId, count = count })
        --    table.insert(pickList, { tplId = tplId, count = count })
       -- end

        local mopupDropTpl = t.drop[tplSec.mopupRewardList]
        if mopupDropTpl then
            publicList = mopupDropTpl:DoDrop()
        end
       
        table.insert(list, { publicList = publicList})
        count = count - 1
        scenarioCopy.evtScenarioMopup:trigger(sectionId, count)
    end
    scenarioCopy.pickDropItems(pickList, p.ResourceGainType.SCENARIO_COPY)

    chapter.sync = false
    section.sync = false
    section.leftCount = section.leftCount - totalCount
    -- print("section.leftCount, count", section.leftCount, count)

    scenarioCopy.sendScenarioChapterUpdate()
    scenarioMoveUpResponse(p.ErrorCode.SUCCESS, oldLevel, oldExp, list)

    scenarioCopy.evtFightScenarioCopy:trigger(tplSec, true, totalCount, tpl)
end

pktHandlers[p.CS_SCENARIO_STAR_DRAW] = function(pktin, session)
    local function scenarioStarDrawResponse(result, chapterId, target)
        -- print('p.CS_SCENARIO_STAR_DRAW..result, chapterId, target', result, chapterId, target)
        agent.replyPktout(session, p.SC_SCENARIO_STAR_DRAW_RESPONSE, '@@1=i,2=i,3=i',result, chapterId, target)
    end

    local chapterId, target = pktin:read('ii')
    -- print('p.CS_SCENARIO_STAR_DRAW..chapterId, target', chapterId, target)

    --1.检查配置表
    local tpl = t.scenarioChapter[chapterId]
    if not tpl then
        print('p.CS_SCENARIO_STAR_DRAW..chapter is not exists..chapterId=', chapterId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    if not tpl.rewardList[target] then
        print('p.CS_SCENARIO_STAR_DRAW..reward target is not exists..chapterId, target=', chapterId, target)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --2.是否可以领取
    local chapter = scenarioCopy.chapterList[chapterId]
    if not chapter or chapter.ownStar < target then
        print('p.CS_SCENARIO_STAR_DRAW..can not draw...chapterId', chapterId)
        agent.sendNoticeMessage(p.ErrorCode.SCENARIO_NOT_ACHIEVE_CONDITION, '', 1)
        return
    end
    --3.是否已经领取过
    local drews = scenarioCopy.drewList[chapterId]
    if not drews then
        scenarioCopy.drewList[chapterId] = {}
        drews = scenarioCopy.drewList[chapterId]
    end
    if drews and drews[target] then
        print('p.CS_SCENARIO_STAR_DRAW..target is drew, target=', target)
        agent.sendNoticeMessage(p.ErrorCode.SCENARIO_IS_DREW, '', 1)
        return
    end
    --pick drops
    local reward = tpl.rewardList[target]
    local pickList = {}
    for _, v in pairs(reward) do
        table.insert(pickList, { tplId = v[1], count = v[2] })
    end
    -- print('p.CS_SCENARIO_STAR_DRAW..pickList', utils.serialize(pickList))
    scenarioCopy.pickDropItems(pickList, p.ResourceGainType.SCENARIO_COPY)

    scenarioCopy.drewList[chapterId][target] = 1
    local updateList, drewList = {}, {}
    for target, v in pairs(drews) do
        table.insert(drewList, { target = target } )
    end
    table.insert(updateList, { chapterId = chapterId, drewList = drewList })
    scenarioCopy.sendScenarioStarRewardUpdate(updateList)
    scenarioStarDrawResponse(p.ErrorCode.SUCCESS, chapterId, target)
end

pktHandlers[p.CS_SCENARIO_ANSWER] = function(pktin, session)
    local chapterId, sectionId, count = pktin:read('iii')
    -- print('p.CS_SCENARIO_ANSWER...chapterId, sectionId, count', chapterId, sectionId, count)

    --1.检查配置表
    local tpl = t.scenarioChapter[chapterId]
    if not tpl then
        print('p.CS_SCENARIO_ANSWER..chapter is not exists..chapterId=', chapterId)
        agent.sendNoticeMessage(p.ErrorCode.SCENARIO_NOT_UNLOCK, '', 1)
        return
    end
    local tplSec = tpl.sections[sectionId]
    if not tplSec or tplSec.battleType ~= 2 or count > tplSec.questionPool[2] then
        print('p.CS_SCENARIO_ANSWER..section is not exists..sectionId=', sectionId)
        agent.sendNoticeMessage(p.ErrorCode.SCENARIO_NOT_UNLOCK, '', 1)
        return
    end
    --2.检查该关卡是否可以挑战
    local maxChapterId = scenarioCopy.maxChapterList[tpl.type] or 0
    if maxChapterId < chapterId then
        print('p.CS_SCENARIO_ANSWER..the previous chapter did not pass..chapterId > maxChapterId', chapterId, maxChapterId)
        agent.sendNoticeMessage(p.ErrorCode.SCENARIO_NOT_UNLOCK, '', 1)
        return
    end
    --解锁条件
    if not scenarioCopy.checkUnlockCond(tplSec.condList) then
        print('p.CS_SCENARIO_ANSWER..not up to the challenge...chapterId=', chapterId)
        agent.sendNoticeMessage(p.ErrorCode.SCENARIO_NOT_UNLOCK, '', 1)
        return
    end
    --3.检查前置关卡是否已经通关
    local chapter = scenarioCopy.chapterList[chapterId]
    if not chapter then
        print('p.CS_SCENARIO_ANSWER..the chapter did not pass...chapterId', chapterId)
        agent.sendNoticeMessage(p.ErrorCode.SCENARIO_NOT_UNLOCK, '', 1)
        return
    end
    for _, preId in pairs(tplSec.preId) do
        if preId > 0 then
            local preSection = chapter.list[preId]
            if preSection == nil or not preSection.isPass then
                print('p.CS_SCENARIO_ANSWER..the previous section did not pass...sectionId, preId', sectionId, preId)
                agent.sendNoticeMessage(p.ErrorCode.SCENARIO_NOT_UNLOCK, '', 1)
                return
            end
        end
    end
    --4.是否可以闯关 次数 是否可以重复刷
    local section = chapter.list[sectionId]
    if section and (section.leftCount <= 0 or not tplSec.canRepeat) then
        print('p.CS_SCENARIO_ANSWER..can not fight...sectionId, leftCount=', sectionId, section.leftCount)
        agent.sendNoticeMessage(p.ErrorCode.SCENARIO_FIGHT_COUNT_NOT_ENOUGH, '', 1)
        return
    end
    -- --5.检查体力是否足够
    -- if not user.isStaminaEnough(tplSec.stamina) then
    --     print('p.CS_SCENARIO_ANSWER..Stamina is not Enough')
    --     agent.sendNoticeMessage(p.ErrorCode.PUBLIC_STAMINA_NOT_ENOUGH, '', 1)
    --     return
    -- end

    local dataOnCombatOver = { chapterId = chapterId, sectionId = sectionId, isBattle = false }
    local total = tplSec.questionCount
    local result = { total = total, left = count }
    local isWin = false
    if count >= math.ceil(total/2) then
        isWin = true
    end
    scenarioCopy.onCombatOver(battleId, p.BattleType.SCENARIO_COPY_ANSWER, isWin, count, total, 0, 0, dataOnCombatOver)
end

return scenarioCopy
