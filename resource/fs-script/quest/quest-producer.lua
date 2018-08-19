local p = require('protocol')
local t = require('tploader')
local timer = require('timer')
local qi = require('quest/questInfo')

local passSpecifyScenarioChapter = require('quest/pass-specify-scenario-chapter')
local passSpecifyScenarioNum = require('quest/pass-specify-scenario-num')
local passSpecifyScenario = require('quest/pass-specify-scenario')
local equipLevelUp = require('quest/equip-level-up')
local arenaWinNum = require('quest/arena-win-num')
local arenaFightNum = require('quest/arena-fight-num')
local upgradeBuildingLevel = require('quest/upgrade-building-level')
local upgradeResBuildingLevel = require('quest/upgrade-res-building-level')
local heroNum = require('quest/hero-num')
local purpleHeroNum = require('quest/purple-hero-num')
local heroLevelNum = require('quest/hero-level-num')
local heroStarNum = require('quest/hero-star-num')
local userPower = require('quest/user-power')
local userLevel = require('quest/user-level')
local buildingTrap = require('quest/building-trap')
local createBuilding = require('quest/create-building')
local trainArmy = require('quest/train-army')
local trainArmyNum = require('quest/train-army-num')
local teleport = require('quest/teleport')
local useResItem = require('quest/use-res-item')
local scoutTimes = require('quest/scout-times')
local useSpeedUpItem = require('quest/use-speedup-item')
local beScoutTimes = require('quest/bescout-times')

local addSilver = require('quest/add-silver')
local attackPlayer = require('quest/attack-player')
local passBabel = require('quest/pass-babel')
local healArmyNum = require('quest/heal-army-num')
local healArmyTime = require('quest/heal-army-time')
local killMonsterNum = require('quest/kill-monster-num')
local takeResourceTaxNum = require('quest/take-resource-tax-num')
local takeResourceTax = require('quest/take-resource-tax')
local mopupScenario = require('quest/mopup-scenario')
local passScenarioCopyChapter = require('quest/pass-scenario-copy-chapter')
local allianceDonateNum = require('quest/alliance-donate-num')
local scienceStudy = require('quest/science-study')
local scienceStudyNum = require('quest/science-study-num')
local allianceHelpNum = require('quest/alliance-help-num')
local shopBuyItem = require('quest/shop-buy-item')
local occupyCity = require('quest/occupy-city')
local finishQuest = require('quest/finish-quest')
local getHero = require('quest/get-hero')
local getHeroNum = require('quest/get-hero-num')
local heroAccumulativeLevelUp = require('quest/hero-accumulative-levelup')
local heroLevelUp = require('quest/hero-levelup')
local upgradeHeroSkill = require('quest/upgrade-hero-skill')
local upgradeHeroSkillNum = require('quest/upgrade-hero-skill-num')
local bronzeNum = require('quest/bronze-num')
-- local bronzeScore = require('quest/bronze-score')
local gatherResourceNum = require('quest/gather-resource-num')
local gatherResource = require('quest/gather-resource')
local allianceJoin = require('quest/alliance-join')
local userUpgrade = require('quest/user-upgrade')
local getHeroTypeNum = require('quest/get-hero-type-num')
local resourcePerNum = require('quest/resource-per-num')
local arenaRank = require('quest/arena-rank')
local bronzeRightNum = require('quest/bronze-right-num')

local questProducer = {}


function questProducer.createQuest(agent, info)
    if agent == nil or info == nil then
        return nil
    end
    local tpl = info.tpl
--[[
    local tpl = t.quest[info.id]
    if tpl == nil then
        return nil
    end
--]]
    --print('questProducer.createQuest ... type:', tpl.type, 'condType:', tpl.CondType, 'questId:', tpl.id)
    local quest = nil
    if tpl.CondType == p.QuestCondType.PASS_SPECIFY_SCENARIO_CHAPTER then
        -- 通关XX章节
        quest = passSpecifyScenarioChapter:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.PASS_SPECIFY_SCENARIO_NUM then
        -- 通关指定关卡次数
        quest = passSpecifyScenarioNum:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.PASS_SPECIFY_SCENARIO then
        -- 通关XX关卡
        quest = passSpecifyScenario:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.EQUIP_LEVELUP_NUM_LEVEL then
        -- XX件装备强化到XX级
        quest = equipLevelUp:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.ARENA_WIN_NUM then
        -- 擂台胜利次数
        quest = arenaWinNum:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.ARENA_FIGHT_NUM then
        -- 擂台挑战次数
        quest = arenaFightNum:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.UPGRADE_BUILDING_LEVEL then
        -- XX建筑达到X级
        quest = upgradeBuildingLevel:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.UPGRADE_RES_BUILDING_LEVEL then
        -- XX个资源建筑达到XX级
        quest = upgradeResBuildingLevel:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.HERO_NUM then
        -- 拥有武将个数达到XX个
        quest = heroNum:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.PURPLE_HERO_NUM then
        -- 拥有紫武将达到X个
        quest = purpleHeroNum:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.HERO_NUM_LEVEL then
        -- 拥有？个达到X级的武将
        quest = heroLevelNum:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.HERO_NUM_STAR then
        -- 拥有X星武将达到X个
        quest = heroStarNum:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.PLAYER_POWER then
        -- 战力达到XXX
        quest = userPower:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.PLAYER_LEVEL then
        -- 城主等级达到XXX
        quest = userLevel:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.BUILDING_TRAP then
        -- 建造陷阱达到X个
        quest = buildingTrap:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.CREATE_BUILDING then
        -- 建造X资源地达到X个
        quest = createBuilding:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.TRAIN_ARMY then
        -- 建造部队达到X个
        quest = trainArmy:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.TRAIN_ARMY_NUM then
        -- 建造部队达到x次
        quest = trainArmyNum:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.TELEPORT then
        -- 迁城X次
        quest = teleport:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.USE_RES_ITEM then
        -- 使用资源道具达到X个
        quest = useResItem:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.SCOUT_NUM then
        -- 侦查X目标X次
        quest = scoutTimes:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.USE_SPEED_ITEM then
        -- 使用加速道具X次
        quest = useSpeedUpItem:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.BE_SCOUT_NUM then
        -- 被侦查X次
        quest = beScoutTimes:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.TOTAL_ADD_SILVER then
        -- 累计获得银两XX
        quest = addSilver:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.ATTACK_PLAYER_WIN_NUM then
        -- 攻击其他玩家并获胜X次
        quest = attackPlayer:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.BABEL_WIN_NUM then
        -- 通关千重楼到X层
        quest = passBabel:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.HEAL_ARMY_NUM then
        -- 治疗x个伤兵
        quest = healArmyNum:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.HEAL_ARMY_TIME then
        -- 治疗x次伤兵
        quest = healArmyTime:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.KILL_MONSTER_NUM then
        -- 击杀x次x级野怪
        quest = killMonsterNum:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.TAKE_RESOURCE_TAX_NUM then
        -- 征收x次x资源
        quest = takeResourceTaxNum:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.TAKE_RESOURCE_TAX then
        -- 征收x个x资源
        quest = takeResourceTax:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.MOPUP_SCENARIO then
        -- 扫荡x次x副本
        quest = mopupScenario:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.PASS_SCENARIO_COPY_CHAPTER then
        -- 通过x关卡并获得x数量的星星
        quest = passScenarioCopyChapter:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.ALLIANCE_DONATE_NUM then
        -- 捐献x次联盟科技
        quest = allianceDonateNum:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.SCIENCE_STUDY then
        -- 研究x科技
        quest = scienceStudy:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.SCIENCE_STUDY_NUM then
        -- 研究科技x次
        quest = scienceStudyNum:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.ALLIANCE_HELP_NUM then
        -- 帮助x次同盟玩家
        quest = allianceHelpNum:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.SHOP_BUY_ITEM then
        -- 在x(特定商城)购买x次道
        quest = shopBuyItem:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.OCCUPY_CITY then
        -- 占领x（城池类型）城池
        quest = occupyCity:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.FINISH_QUEST then
        -- 完成特定任务
        quest = finishQuest:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.GET_HERO then
        -- 招募获得武将x
        quest = getHero:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.GET_HERO_NUM then
        -- 招募获得x个武将
        quest = getHeroNum:new(agent, info) 
    elseif tpl.CondType == p.QuestCondType.HERO_ACCUMULATIVE_LEVELUP then
        -- 武将累计升x级
        quest = heroAccumulativeLevelUp:new(agent, info)  
    elseif tpl.CondType == p.QuestCondType.HERO_LEVELUP then
        -- 获得x个x级武将
        quest = heroLevelUp:new(agent, info) 
    elseif tpl.CondType == p.QuestCondType.UPGRADE_HERO_SKILL_NUM then
        -- 升级x次武将技能
        quest = upgradeHeroSkill:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.UPGRADE_HERO_SKILL then
        -- 有x个武将技能升级到x级
        quest = upgradeHeroSkillNum:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.BRONZE_NUM then
        -- 铜雀台答题x次
        quest = bronzeNum:new(agent, info)
--    elseif tpl.CondType == p.QuestCondType.BRONZE_SCORE then
        -- 铜雀台答题达到x分
--        quest = bronzeScore:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.GATHER_RESOURCE_NUM then
        -- 采集x次资源
        quest = gatherResourceNum:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.GATHER_RESOURCE then 
        -- 采集x资源x个
        quest = gatherResource:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.ALLIANCE_JOIN then
        -- 加入一个同盟
        quest = allianceJoin:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.USER_UPGRADE then
        -- 主公升至N级
        quest = userUpgrade:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.GET_HERO_TYPE_NUM then
        -- 招贤馆中普通招募N次  --扩展 精英、普通 
        quest = getHeroTypeNum:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.RESOURCE_OUTPUT then
        -- X资源的基础产量达到N/小时 
        quest = resourcePerNum:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.ARENA_RANK then
        -- 擂台中最高排名进入N名 
        quest = arenaRank:new(agent, info)
    elseif tpl.CondType == p.QuestCondType.BRONZE_RIGHT_NUM then
        -- 铜雀台中单次答对N题
        quest = bronzeRightNum:new(agent, info)
    end

    return quest
end


return questProducer
