local agent = ...
local t = require('tploader')
local p = require('protocol')
local dbo = require('dbo')
local mapService
local timer = require('timer')
local utils = require('utils')
local misc = require('libs/misc')
local pubHero = require('hero')
local event = require('libs/event')
local trie = require('trie')
local logStub = require('stub/log')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()

local allianceStub = require('stub/alliance')
local loginStub = require('stub/login')
local pushStub = require('stub/push')
local chatStub = require('stub/chat')
local allianceInfo = require('alliance')

local user = agent.user
local dict = agent.dict
local cdlist = agent.cdlist
local bag = agent.bag
local building = agent.building
local technology = agent.technology
local hero = agent.hero
local vip = agent.vip
local army = agent.army

--联盟CD
local allianceCdInterval = 24 * 60 * 60
--盟主替换不在线时间
local replaceInterval = 7 * 24 * 60 * 60
--新人进入联盟后需要满4小时才能捐献
local newMemberDonateInterval = 0
--武将雇佣时间
local employHeroInterval = 12 * 60 * 60
--雇佣到期时间
local employExpire = { expireTime = 0, ownerUid = 0, heroId = 0 }
--联盟公告长度
local announcementLen = 100
--每个人出租武将数量上限
local HERO_LEASE_MAX = 1

local create_level_cost = t.configure['alliance_create_level_cost']
local change_banner_cost_gold = t.configure['change_banner_cost_gold']
local donate_cd = t.configure['alliance_donate_cd']
local donate_cd_max = t.configure['alliance_donate_cd_max']
local alliance_hero_lease = t.configure['alliance_hero_lease']
local allianceEmployCond = t.configure['allianceEmployCond']
local userRecalHeroInterval = t.configure['userRecalHeroInterval'] or 0

local alliance = {
    info = nil,
    joinAllianceCount = 0,  --加入联盟次数
    allianceCdTime = 0,     --联盟CD, fs单独保存,cs有相应的操作; 登陆需同步到ms
    allianceScore = 0,      --联盟积分
    donateCdTime = 0,       --捐献CD
    scienceItemList = {},   --联盟科技捐献物品表 scienceItemList[groupId] = {{doanteType=1, donationId=0}, ...}
    requestHelpList = {},   --申请帮助的建筑列表,防止重复申请 requestHelpList[cdid] = buildingId
    myHelps = {},           --我申请的帮助列表 myHelps[helpId] = helpInfo
    otherHelps = {},        --盟友申请的帮助列表 otherHelps[helpId] = helpInfo
    employHeroList = {},    --我雇佣别人的武将 employHeroList[uid][heroId] = { ownerUid=0, ownerNickname='', expireTime=0, heroInfo={} }
    myAllianceBuffs = {},   --激活的个人联盟buff [buffId]={ type=0, cdId=0, open=false, closedTime=0 }

    --event
    evtAllianceJoin = event.new(), --()
    evtAllianceQuit = event.new(), --()
    evtAllianceUpgrade = event.new(), --()
    evtAllianceHelp = event.new(), --(count)

    evtScienceUpgrade = event.new(), --()
    evtAllianceBuffAdd = event.new(), --()
    evtAllianceBuffRemove = event.new(), --()

    evtEmployHeroUpdate = event.new(), --()

    evtAllianceDonate = event.new(),  --()
}

local pktHandlers = {}

function alliance.onInit()
    agent.registerHandlers(pktHandlers)
    alliance.dbLoad()
    alliance.info = allianceStub.findMyAlliance(user.uid)

    --event
    building.evtBuildingStateClose:attachRaw(alliance.onBuildingStateClose)
    building.evtMarketCreate:attachRaw(alliance.onMemberUpdate)
    user.evtNicknameChange:attachRaw(alliance.onMemberUpdate)
    user.evtTotalPowerChange:attachRaw(alliance.onMemberUpdate)
    user.evtHeadIdChange:attachRaw(alliance.onMemberUpdate)
    vip.evtUpgrade:attachRaw(alliance.onMemberUpdate)
    agent.map.evtTeleport:attachRaw(alliance.onMemberUpdate)
end

function alliance.onAllCompInit()
    if alliance.info == nil or alliance.info.id == 0 then
        alliance.initNoAllianceData(true)
    else
        alliance.initAlliance()
    end

    --雇佣的武将检查
    alliance.checkEmployHero(true)
end

function alliance.onReady()
    mapService = agent.map.mapService
    alliance.sendAllianceMemberUpdate()
end

function alliance.onSave()
    alliance.dbSave()
end

function alliance.onTimerUpdate(timerIndex)
    --mixEmployExpireTime
    local now = timer.getTimestampCache()
    if employExpire.expireTime ~= 0 and employExpire.expireTime <= now then
        alliance.checkEmployHero()
    end
end

function alliance.onClose()
    alliance.dbSave()
end

function alliance.dbLoad()
    local data = dict.get("alliance.data") or {}
    alliance.joinAllianceCount = data.joinAllianceCount or alliance.joinAllianceCount
    alliance.allianceScore = data.allianceScore or alliance.allianceScore
    alliance.donateCdTime = data.donateCdTime or alliance.donateCdTime
    alliance.scienceItemList = data.scienceItemList or alliance.scienceItemList
    alliance.requestHelpList = data.requestHelpList or alliance.requestHelpList
    alliance.myAllianceBuffs = data.myAllianceBuffs or alliance.myAllianceBuffs
    --武将雇佣
    alliance.employHeroList = {}
    local employHeroList = data.employHeroList or {}
    for _, v in pairs(employHeroList) do
        if alliance.employHeroList[v.ownerUid] == nil then
            alliance.employHeroList[v.ownerUid] = {}
        end
        local heroId = v.heroInfo.id
        if alliance.employHeroList[v.ownerUid][heroId] == nil then
            alliance.employHeroList[v.ownerUid][heroId] = {}
        end
        local employ = {}
        employ.ownerUid = v.ownerUid
        employ.ownerNickname = v.ownerNickname
        employ.expireTime = v.expireTime
        local info = pubHero.newHeroInfo(v.heroInfo)
        employ.heroInfo = info
        alliance.employHeroList[v.ownerUid][heroId] = employ
    end
    --联盟CD
    local dataCd = dict.get("alliance.cd") or {}
    alliance.allianceCdTime = dataCd.allianceCdTime or alliance.allianceCdTime
end

function alliance.dbSave()
    local employHeroList = {}
    for _, v in pairs(alliance.employHeroList) do
        for _, employ in pairs(v) do
            local heroInfo = employ.heroInfo
            if heroInfo then
                local skill = heroInfo:getHeroSkill()
                local equip = heroInfo:getHeroEquip()
                local treasure = heroInfo:getHeroTreasure()
                local temphero = { id = heroInfo.id, level = heroInfo.level, star = heroInfo.star, skill = skill, equip = equip, treasure = treasure }
                table.insert(employHeroList, { ownerUid = employ.ownerUid, ownerNickname = employ.ownerNickname, expireTime = employ.expireTime, heroInfo = temphero })
            end
        end
    end
    dict.set("alliance.data", {
        joinAllianceCount = alliance.joinAllianceCount,
        allianceScore = alliance.allianceScore,
        donateCdTime = alliance.donateCdTime,
        scienceItemList = alliance.scienceItemList,
        requestHelpList = alliance.requestHelpList,
        employHeroList = employHeroList,
        myAllianceBuffs = alliance.myAllianceBuffs,
        })

    --单独保存 cs有相应的操作
    if alliance.allianceCdTime > timer.getTimestampCache() then
        dict.set("alliance.cd", {
            allianceCdTime = alliance.allianceCdTime,
            })
    end
end

function alliance.checkEmployHero(bLoad)
    -- print('alliance.checkEmployHero...bLoad', bLoad)
    local updates = nil
    local deletes = nil
    if bLoad then
        local now = timer.getTimestampCache()
        for _, v in pairs(alliance.employHeroList) do
            for heroId, employ in pairs(v) do
                if now >= employ.expireTime then
                    alliance.employHeroList[employ.ownerUid][heroId] = nil
                end
            end
        end
    else
        if employExpire.ownerUid > 0 then
            updates = {}
            deletes = {}
            if alliance.employHeroList[employExpire.ownerUid] then
                alliance.employHeroList[employExpire.ownerUid][employExpire.heroId] = nil
            end
            table.insert(deletes, { ownerUid = employExpire.ownerUid, id = employExpire.heroId })
            --trigger
            alliance.evtEmployHeroUpdate:trigger()
        end
    end
    alliance.sendAllianceUserEmployHeroUpdate(updates, deletes)
    alliance.resetEmployExpire()
end

function alliance.resetEmployExpire()
    employExpire.expireTime = 0
    employExpire.ownerUid = 0
    employExpire.heroId = 0

    local now = timer.getTimestampCache()
    for _, v in pairs(alliance.employHeroList) do
        for heroId, employ in pairs(v) do
            if now < employ.expireTime and
                (employExpire.expireTime == 0 or employExpire.expireTime > employ.expireTime) then
                employExpire.expireTime = employ.expireTime
                employExpire.ownerUid = employ.ownerUid
                employExpire.heroId = heroId
            end
        end
    end
end

function alliance.employHeroForSync()
    local heroList = {}

    --employHeroList[uid][heroId] = { ownerUid=0, ownerNickname='', expireTime=0, heroInfo={} }
    for _, uHero in pairs(alliance.employHeroList) do
        -- print('alliance.employHeroForSync...uHero', utils.serialize(uHero))
        for _, temp in pairs(uHero) do
            local v = temp.heroInfo
            if v then
                local skill = {}
                for slot, info in pairs(v.skill) do
                    -- print('hero.heroForSync...heroId, slot, isOpen', v.id, slot, info.isOpen)
                    if info.isOpen then
                        table.insert(skill, { tplId = info.tplId, level = info.level })
                    end
                end

                local equip = {}
                for _, info in pairs(v.equip) do
                    -- table.insert(equip, { tplId = info.tplId, level = info.level, succinct = info.succinct, inlay = info.inlay })
                    table.insert(equip, { tplId = info.tplId, level = info.level })
                end

                local treasure = {}
                for _, info in pairs(v.treasure) do
                    table.insert(treasure, { tplId = info.tplId, level = info.level })
                end

                local hero = { id = v.id, level = v.level, star = v.star, skill = skill, equip = equip, treasure = treasure, additionList = v.additionList, expireTime = temp.expireTime }

                table.insert(heroList, hero)
                -- print('###hero.heroForSync..heroId, type(skill), skill', v.id, type(v.skill), v.skill)
            end
        end
    end
    -- print('alliance.employHeroForSync....heroList', utils.serialize(heroList))
    return heroList
end

--event
function alliance.onBuildingStateClose(buildingId, cdId)
    -- print('alliance.onBuildingStateClose ...buildingId, cdId', buildingId, cdId)
    if alliance.requestHelpList[cdId] ~= nil then
        alliance.requestHelpList[cdId] = nil
        alliance.sendAllianceRequestHelp()
    end
    local info = alliance.info
    if info == nil or info.id == 0 then
        return
    end
    for _, v in pairs(alliance.myHelps) do
        if v.buildingId == buildingId and v.cdId == cdId then
            local helpIds = {}
            table.insert(helpIds, v.helpId)
            allianceStub.cast_help_close(info.id, user.uid, helpIds)
            break
        end
    end
end

function alliance.onMemberUpdate()
    local info = alliance.info
    if info == nil or info.id == 0 then
        return
    end
    local u = user.info
    local vipLevel = vip.vipLevel()
    local x = agent.map.info.x
    local y = agent.map.info.y
    local level = building.getBuildingLevelByType(p.BuildingType.MARKET)
    local marketIsOpen = false
    if level > 0 then
        marketIsOpen = true
    end
    -- print("info.id, user.uid, u.nickname, u.headId, u.level, vipLevel, u.totalPower, u.castleLevel, x, y", info.id, user.uid, u.nickname, u.headId, u.level, vipLevel, u.totalPower, u.castleLevel, x, y)
    allianceStub.cast_member_update(info.id, user.uid, u.nickname, u.headId, u.level, vipLevel, u.totalPower, u.castleLevel, x, y, marketIsOpen)
end

--对外接口函数
function alliance.allianceId()
    local info = alliance.info
    if info == nil then
        return 0
    end
    return info.id
end

function alliance.allianceName()
    local info = alliance.info
    if info == nil or info.name == nil then
        return ''
    end
    return info.name
end

function alliance.allianceNickname()
    local info = alliance.info
    if info == nil or info.nickname == nil then
        return ''
    end
    return info.nickname
end

function alliance.allianceBannerId()
    local info = alliance.info
    if info == nil then
        return 0
    end
    return info.bannerId
end

function alliance.allianceLevel()
    local info = alliance.info
    if info == nil then
        return 0
    end
    return info.level
end

function alliance.scienceList()
    local info = alliance.info
    if info == nil then
        return nil
    end
    return info.scienceList
end

function alliance.allianceBuffList()
    local info = alliance.info
    if info == nil then
        return nil
    end
    return info.allianceBuffList
end

function alliance.getAllianceCountByLevel(allianceLevel)
    if allianceLevel == nil then
        allianceLevel = 1
    end
    local count = allianceStub.getAllianceCount(allianceLevel)
    -- print('alliance.getAllianceCountByLevel...allianceLevel, count', allianceLevel, count)

    return count
end

function alliance.AddAllianceScore(score, gainType, sendUpdate)
    if gainType == nil then
        gainType = p.ResourceGainType.ALLIANCE_DONATE
    end
    if sendUpdate == nil then
        sendUpdate = true
    end
    if score > 0 then
        alliance.allianceScore = alliance.allianceScore + score
        --log
        logStub.appendAllianceScore(user.uid, score, p.LogItemType.GAIN, gainType, timer.getTimestampCache())

        if sendUpdate then
            alliance.sendAllianceScoreUpdate()
        end
    end
end

function alliance.isAllianceScoreEnough(score)
    return alliance.allianceScore >= score
end

function alliance.removeAllianceScore(score, consumeType, sendUpdate)
    if consumeType == nil then
        consumeType = p.ResourceConsumeType.STORE_BUY
    end
    if sendUpdate == nil then
        sendUpdate = true
    end
    if score >0 then
        alliance.allianceScore = alliance.allianceScore - score
        if alliance.allianceScore < 0 then
            alliance.allianceScore = 0
        end
        --log
        logStub.appendAllianceScore(user.uid, score, p.LogItemType.CONSUME, consumeType, timer.getTimestampCache())

        if sendUpdate then
            alliance.sendAllianceScoreUpdate()
        end
    end
    return true
end

--联盟邮件
function alliance.sendAllianceMail(fromUid, fromNickname, fromHeadId, fromLangType, content)
    local info = alliance.info
    if info == nil or info.id == 0 then
        return p.ErrorCode.ALLIANCE_NOT_EXIST
    end
    local fromMenber = info.memberList[fromUid]
    if fromMenber == nil then
        return p.ErrorCode.ALLIANCE_NOT_MEMBER
    end
    --是否有发送权限
    if not fromMenber:haveRankPermit(p.AllianceRankPermitType.MAIL) then
        return p.ErrorCode.ALLIANCE_WITHOUT_PERMIT
    end
    local maxCount = fromMenber:haveSendMailMax()
    -- print('alliance.sendAllianceMail....sendMailCount, sendMailMax', fromMenber.sendMailCount, maxCount)
    if fromMenber.sendMailCount >= maxCount then
        return p.ErrorCode.ALLIANCE_SEND_MAIL_MAX
    end
    -- send
    return allianceStub.call_alliance_mail(info.id, fromUid, fromNickname, fromHeadId, fromLangType, content)
end

function alliance.sendAllianceEventToChat(chatSubType, params)
    local info = alliance.info
    if info == nil or info.id == 0 then
        print('alliance.sendAllianceEventToChat, alliance is not exist..')
        return
    end

    local chatInfo = {}
    chatInfo.subType = chatSubType
    chatInfo.from_uid = user.uid
    chatInfo.from_nickname = user.info.nickname
    chatInfo.vipLevel = agent.vip.vipLevel()
    chatInfo.headId = user.info.headId
    chatInfo.level = user.info.level
    chatInfo.langType = user.info.langType
    chatInfo.allianceNickname = alliance.allianceName()
    chatInfo.time = timer.getTimestampCache()
    chatInfo.params = params
    local chatType, content = t.getChatTypeAndContent(chatSubType)
    if chatType and content then
        chatInfo.content = content
        if chatType == p.ChatType.KINGDOM then
            chatInfo.type = p.ChatType.KINGDOM
            chatStub.cast_kingdomChat(chatInfo)
        elseif chatType == p.ChatType.ALLIANCE then
            chatInfo.type = p.ChatType.ALLIANCE
            chatStub.cast_allianceChat(info.id,chatInfo)
        elseif chatType == p.ChatType.ALL then
            chatInfo.type = p.ChatType.KINGDOM
            chatStub.cast_kingdomChat(chatInfo)

            chatInfo.type = p.ChatType.ALLIANCE
            chatStub.cast_allianceChat(info.id,chatInfo)
        end
    end
end

--初始化
function alliance.initNoAllianceData(bLoad)
    -- alliance.donateCdTime = 0
    alliance.requestHelpList = {}
    alliance.scienceItemList = {}
    alliance.myhelps = {}
    alliance.otherHelps = {}
    alliance.myAllianceBuffs = {}
    --邀请信息
    alliance.sendAllianceInvitedUpdate()
    --个人信息
    alliance.sendAlliancePersonalUpdate()
end

function alliance.initAlliance()
    alliance.info = allianceStub.findMyAlliance(user.uid)

    print('alliance.initAlliance........')
    --联盟基本数据
    alliance.sendAllianceUpdate()
    --联盟成员列表
    alliance.sendAllianceMemberUpdate()
    --联盟科技捐献列表
    alliance.sendAllianceScienceUpdate()
    --联盟科技捐献物品列表
    alliance.checkAllianceDonateItems()
    --联盟积分
    alliance.sendAllianceScoreUpdate()
    --武将租借列表
    alliance.sendAllianceHireHeroUpdate()
    --武将出租记录(针对个人)
    alliance.sendAllianceHeroLeaseRecordUpdate()
    --联盟日记
    alliance.sendAllianceRecordUpdate()

    --联盟申请列表
    alliance.initAllianceApply()
    --联盟帮助
    alliance.initAllianceHelp()
    --联盟buff
    alliance.initAllianceBuff()
end

--联盟基本信息
function alliance.updateAlliance(allianceUp)
    -- print('alliance.updateAlliance...allianceUp', allianceUp)
    alliance.info = allianceStub.findMyAlliance(user.uid)
    alliance.sendAllianceUpdate()
    if allianceUp then
        alliance.evtAllianceUpgrade:trigger()
    end
end

function alliance.sendAllianceUpdate()
    local info = alliance.info
    if info == nil or info.id == 0 then
        return
    end
    local cities = allianceStub.allAllianceCity[info.id]
    if cities == nil then
        cities = {}
    end
    local list = {}
    for _, v in ipairs(cities) do
        -- print("value is", v)
        table.insert(list, {id = v})
    end

    local loseBuffCities = allianceStub.allAllianceCity[info.id]
    if loseBuffCities == nil then
        loseBuffCities = {}
    end
    local loseBuffCityList = {}
    for _, v in ipairs(loseBuffCities) do
        -- print("value is", v)
        table.insert(loseBuffCityList, {id = v})
    end
    local armyLevel = {}
    local armyType = p.ArmysType.INFANTRY 
    local level, exp = army.getArmyLevelByArmyType(armyType)
    local armyTpl = t.findArmysTpl(armyType,level)
    local armyId = armyTpl.id
    table.insert(armyLevel, {Id = armyId, ArmysType = armyType, level = level, exp = exp})
    armyType = p.ArmysType.RIDER
    level, exp = army.getArmyLevelByArmyType(armyType)
    armyTpl = t.findArmysTpl(armyType,level)
    armyId = armyTpl.id
    table.insert(armyLevel, {Id = armyId, ArmysType = armyType, level = level, exp = exp})
    armyType = p.ArmysType.ARCHER
    level, exp = army.getArmyLevelByArmyType(armyType)
    armyTpl = t.findArmysTpl(armyType,level)
    armyId = armyTpl.id
    table.insert(armyLevel, {Id = armyId, ArmysType = armyType, level = level, exp = exp})
    armyType = p.ArmysType.MECHANICAL
    level, exp = army.getArmyLevelByArmyType(armyType)
    armyTpl = t.findArmysTpl(armyType,level)
    armyId = armyTpl.id
    table.insert(armyLevel, {Id = armyId, ArmysType = armyType, level = level, exp = exp})
    agent.sendPktout(p.SC_ALLIANCE_INFO_UPDATE,'@@1=i,2=s,3=s,4=i,5=i,6=i,7=i,8=s,9=i,10=i,11=i,12=i,13=i,14=i,15=i,16=i,17=i,18=s,19=[id=i],20=[id=i],21=[Id=i,ArmysType=i,level=i,exp=i]',
        info.id,
        info.name,
        info.nickname,
        info.level,
        info.exp,
        info.bannerId,
        info.leaderId,
        info.leaderName,
        info.leaderHeadId,
        info.alliesCount,
        info.alliesMax,
        info.towerCount,
        info.towerMax,
        info.castleCount,
        info.castleMax,
        info.toatlActive,
        info.power,
        info.announcement,
        list,
        loseBuffCityList,
        armyLevel
        )
end

--联盟成员信息
function alliance.updateMember(m, joinType)
    -- print('alliance.updateMember...', joinType, utils.serialize(m))
    local info = alliance.info
    if info == nil or info.id == 0 then
        alliance.initAlliance()
        -- print('alliance.updateMember....m.uid, user.uid, joinType', m.uid, user.uid, joinType)

        --joinType:1创建加入，2申请加入，3邀请加入
        if joinType then
            if joinType == 1 then
                --local params = {}
                local allianceName = alliance.allianceName()
                --table.insert(params, { uid = user.uid, nickname = user.info.nickname, allianceName = allianceName })
                local params = {param1 = user.info.nickname, param2 = allianceName}
                alliance.sendAllianceEventToChat(p.ChatSubType.ALLIANCE_CREATE, utils.serialize(params))
            elseif joinType == 2 then
                agent.sendPktout(p.SC_ALLIANCE_APPLY_ACCEPT_RESPONSE, p.ErrorCode.SUCCESS)
            end
        end

        alliance.evtAllianceJoin:trigger()
        alliance.joinAllianceCount = alliance.joinAllianceCount + 1
        return
    end
    if m and m.uid > 0 then
        local updates = {}
        table.insert(updates, {
            uid = m.uid,
            nickname = m.nickname,
            headId = m.headId,
            userLevel = m.userLevel,
            vipLevel = m.vipLevel,
            rankLevel = m.rankLevel,
            activeWeek = m.activeWeek,
            contribution = m.contribution,
            lastOnlineTime = m.lastOnlineTime,
            sendMailCount = m.sendMailCount,
            totalPower = m.totalPower,
            castleLevel = m.castleLevel,
            x = m.x,
            y = m.y,
            marketIsOpen = m.marketIsOpen,
            })
        -- print("alliance.updateMember", utils.serialize(updates))
        alliance.sendAllianceMemberUpdate(updates)
    end
end

function alliance.deleteMember(uid, allianceCdTime, isKick)
    if uid == user.uid then
        alliance.info = nil
        alliance.allianceCdTime = allianceCdTime
        alliance.initNoAllianceData(false)
        --TODO:tips 退出提示
        agent.sendPktout(p.SC_ALLIANCE_KICK_MEMBER_RESPONSE, p.ErrorCode.SUCCESS, uid)

        --trigger
        alliance.evtAllianceQuit:trigger()
    else
        local updates = {}
        local deletes = {}
        table.insert(deletes, { uid = uid })
        alliance.sendAllianceMemberUpdate(updates, deletes)
        alliance.updateAlliance()
    end
end

function alliance.sendAllianceMemberUpdate(updates, deletes)
    -- print(debug.traceback())
    -- print('alliance.sendAllianceMemberUpdate...', updates, deletes)
    local info = alliance.info
    if info == nil or info.id == 0 or info.memberList[user.uid] == nil then
        return
    end
    -- print('alliance.sendAllianceMemberUpdate...updates', utils.serialize(info.memberList))
    if updates == nil then
        updates = {}
        for _, v in pairs(info.memberList) do
            table.insert(updates, {
                uid = v.uid,
                nickname = v.nickname,
                headId = v.headId,
                userLevel = v.userLevel,
                vipLevel = v.vipLevel,
                rankLevel = v.rankLevel,
                activeWeek = v.activeWeek,
                contribution = v.contribution,
                lastOnlineTime = v.lastOnlineTime,
                sendMailCount = v.sendMailCount,
                totalPower = v.totalPower,
                castleLevel = v.castleLevel,
                x = v.x,
                y = v.y,
                marketIsOpen = v.marketIsOpen,
                })
        end
    end
    if deletes == nil then
        deletes = {}
    end
    -- print('alliance.sendAllianceMemberUpdate...updates', utils.serialize(updates))
    agent.sendPktout(p.SC_ALLIANCE_MEMBER_INFO_UPDATE, '@@1=[uid=i,nickname=s,headId=i,userLevel=i,vipLevel=i,rankLevel=i,activeWeek=i,contribution=i,lastOnlineTime=i,sendMailCount=i,totalPower=i,castleLevel=i,x=i,y=i,marketIsOpen=b],2=[uid=i]', updates, deletes)
end

--联盟申请信息
function alliance.initAllianceApply()
    local info = alliance.info
    if info == nil or info.id == 0 then
        return
    end

    alliance.sendAllianceApplyUpdate()
end

function alliance.updateApply(updates, deletes)
    -- print('alliance.updateApply...', updates, deletes)
    alliance.sendAllianceApplyUpdate(updates, deletes)
end

function alliance.sendAllianceApplyUpdate(updates, deletes)
    -- print('alliance.sendAllianceApplyUpdate...', updates, deletes)
    local info = alliance.info
    if info == nil or info.id == 0 or info.memberList[user.uid] == nil then
        return
    end
    if updates == nil then
        updates = {}
        for _, v in pairs(info.applyList) do
            table.insert(updates, { uid = v.uid, nickname = v.nickname, headId = v.headId, userLevel = v.userLevel, vipLevel = v.vipLevel })
        end
    end
    if deletes == nil then
        deletes = {}
    end
    agent.sendPktout(p.SC_ALLIANCE_APPLY_INFO_UPDATE, '@@1=[uid=i,nickname=s,headId=i,userLevel=i,vipLevel=i],2=[uid=i]', updates, deletes)
end

--玩家被联盟邀请信息(没有联盟时用到)
function alliance.updateInvited(aid)
    if alliance.info == nil or  alliance.info.id == 0 then
        local info = allianceStub.findAllianceByAid(aid)
        if info then
            local updates = {}
            local deletes = {}
            table.insert(updates, {
                aid = info.id,
                name = info.name,
                nickname = info.nickname,
                bannerId = info.bannerId,
                level = info.level,
                alliesCount = info.alliesCount,
                alliesMax = info.alliesMax,
                toatlActive = info.toatlActive,
                })
            alliance.sendAllianceInvitedUpdate(updates, deletes)
        end
    end
end

function alliance.removeInvited(aid)
    local updates = {}
    local deletes = {}
    table.insert(deletes, { aid = aid })
    alliance.sendAllianceInvitedUpdate(updates, deletes)
end

function alliance.sendAllianceInvitedUpdate(updates, deletes)
    local info = alliance.info
    if info and info.id > 0 then
        return
    end
    if updates == nil then
        updates = {}
        local myInvitedList = allianceStub.allianceInviteList[user.uid] or {}
        for _, aid in pairs(myInvitedList) do
            local info = allianceStub.findAllianceByAid(aid)
            if info then
                table.insert(updates, {
                    aid = info.id,
                    name = info.name,
                    nickname = info.nickname,
                    bannerId = info.bannerId,
                    level = info.level,
                    alliesCount = info.alliesCount,
                    alliesMax = info.alliesMax,
                    toatlActive = info.toatlActive,
                    })
            end
        end
    end
    if deletes == nil then
        deletes = {}
    end
    -- print('alliance.sendAllianceInvitedUpdate...updates', utils.serialize(updates))
    -- print('alliance.sendAllianceInvitedUpdate...deletes', utils.serialize(deletes))
    if next(updates) or next(deletes) then
        agent.sendPktout(p.SC_ALLIANCE_INVITED_INFO_UPDATE, '@@1=[aid=i,name=s,nickname=s,bannerId=i,level=i,alliesCount=i,alliesMax=i,toatlActive=i],2=[aid=i]', updates, deletes)
    end
end

--个人信息(没有联盟时用到)
function alliance.sendAlliancePersonalUpdate()
    local cdTime = alliance.allianceCdTime - timer.getTimestampCache()
    if cdTime < 0 then
        cdTime = 0
    end
    agent.sendPktout(p.SC_ALLIANCE_PERSONAL_UPDATE, alliance.joinAllianceCount, cdTime)
end

--联盟科技
function alliance.updateScience(science, uplevel)
    -- print('alliance.updateScience...science, uplevel', utils.serialize(science), uplevel)
    if uplevel and uplevel > 0 then
        --科技升级 event trigger
        alliance.evtScienceUpgrade:trigger()
    end

    if science and science.groupId > 0 and science.tplId > 0 then
        local updates = {}
        table.insert(updates, {
            groupId = science.groupId,
            tplId = science.tplId,
            level = science.level,
            exp = science.exp,
            })
        alliance.sendAllianceScienceUpdate(updates)
    end
end

function alliance.getDonateCdTime()
    local info = alliance.info
    if info == nil or info.id == 0 then
        return 0
    end
    local cdTime = 0
    if alliance.donateCdTime >= 0 then
        cdTime = alliance.donateCdTime - timer.getTimestampCache()
        if cdTime < 0 then
            cdTime = 0
        end
    end
    return cdTime
end

function alliance.sendAllianceScienceUpdate(updates)
    -- print('alliance.sendAllianceScienceUpdate...', updates)
    local info = alliance.info
    if info == nil or info.id == 0 or info.memberList[user.uid] == nil then
        return
    end
    if updates == nil then
        updates = {}
        for _, v in pairs(info.scienceList) do
            table.insert(updates, { groupId = v.groupId, tplId = v.tplId, level = v.level, exp = v.exp })
        end
    end
    local donateCdTime = alliance.getDonateCdTime()
    -- print('alliance.sendAllianceScienceUpdate...donateCdTime, allianceScore, updates', donateCdTime, utils.serialize(updates))
    agent.sendPktout(p.SC_ALLIANCE_SCIENCE_UPDATE, '@@1=[groupId=i,tplId=i,level=i,exp=i],2=i', updates, donateCdTime)
end
--联盟科技捐献物品
function alliance.checkAllianceDonateItems()
    for groupId, v in pairs(t.allianceScience) do
        alliance.getAllianceDonateItems(groupId)
    end
    alliance.sendAllianceDonateItemsUpdate()
end

function alliance.getAllianceDonateItems(groupId)
    local function getDonationId(doanteType)
        local donateTpl = t.allianceDonate[doanteType]
        local donationId = 0
        if donateTpl then
            local total = 0
            local myRandList = {}
            for _, v in pairs(donateTpl) do
                total = total + v.weight
                table.insert(myRandList, { id = v.id, rate = v.weight })
            end
            local randValue = utils.getRandomNum(1, total+1)
            -- print('alliance.getAllianceDonateItems...doanteType, randValue, myRandList', doanteType, randValue, utils.serialize(myRandList))
            local preKey = 1
            local sum = 0
            for k, v3 in ipairs(myRandList) do
                if sum + v3.rate > randValue then
                    break
                else
                    preKey = k
                    sum = sum + v3.rate
                end
            end
            donationId = myRandList[preKey].id
        end

        return donationId
    end

    local itemList = alliance.scienceItemList[groupId]
    if itemList == nil then
        alliance.scienceItemList[groupId] = {}
        itemList = alliance.scienceItemList[groupId]
    end

    local tempTypeList = {
        p.AllianceDonateType.DONATE_RESOURCE,
        p.AllianceDonateType.DONATE_GOLD,
    }
    for _, tempType in ipairs(tempTypeList) do
        local isHave = false
        for _, v in pairs(itemList) do
            if v.doanteType == tempType then
                isHave = true
                break
            end
        end
        if not isHave then
            local tempId = getDonationId(tempType)
            if tempId > 0 then
                table.insert(itemList, { doanteType = tempType, donationId = tempId })
            end
        end
    end
end

function alliance.sendAllianceDonateItemsUpdate(updates)
    -- print('alliance.sendAllianceDonateItemsUpdate...updates', updates)
    if updates == nil then
        updates = {}
        for groupId, v in pairs(alliance.scienceItemList) do
            table.insert(updates, { groupId = groupId, itemList = v })
        end
    end
    -- print('alliance.sendAllianceDonateItemsUpdate...updates', utils.serialize(updates))
    agent.sendPktout(p.SC_ALLIANCE_DONATE_ITEMS_UPDATE, '@@1=[groupId=i,itemList=[doanteType=i,donationId=i]]', updates)
end

function alliance.sendAllianceScoreUpdate()
    agent.sendPktout(p.SC_ALLIANCE_SCORE_UPDATE, alliance.allianceScore)
end

--联盟帮助
function alliance.initAllianceHelp()
    local info = alliance.info
    if info == nil or info.id == 0 then
        return
    end

    local allianceHelps = allianceStub.allianceHelps[info.id]
    if allianceHelps ~= nil then
        local closeHelpIds = {}
        local now = timer.getTimestampCache()
        for _, v in pairs(allianceHelps) do
            if v.uid == user.uid then
                --
                if v.accept < v.times then
                    alliance.helpAcceptAndSubTime(v)
                end
                if not alliance.isHelpClose(v.buildingId, v.cdId) then
                    --自己未关闭的帮助申请
                    alliance.myHelps[v.helpId] = v
                    -- print('alliance.initAllianceHelp....myHelp', utils.serialize(v))
                else
                    --需通知盟友删除该帮助
                    table.insert(closeHelpIds, v.helpId)
                end
            else
                if v.times < v.max then
                    if not v.helpUserList[user.uid] and now < v.cdTime then
                        alliance.otherHelps[v.helpId] = v
                        -- print('alliance.initAllianceHelp....otherHelp', utils.serialize(v))
                    end
                end
            end
        end

        --need sync to ms
        if #closeHelpIds > 0 then
            -- print('alliance.initAllianceHelp....closeHelpIds', utils.serialize(closeHelpIds))
            allianceStub.cast_help_close(info.id, user.uid, closeHelpIds)
        end
    end

    alliance.sendAllianceHelpUpdate()
    alliance.sendAllianceRequestHelp()
end

function alliance.isHelpClose(buildingId, cdId)
    local buildingInfo = building.getBuildingById(buildingId)
    if buildingInfo == nil then
        return true
    end
    if buildingInfo.jobList[cdId] == nil then
        return true
    end
    local cd = cdlist.getCD(cdId)
    if cd == nil or cd:getRemainTime() == 0 then
        return true
    end
    return false
end

function alliance.updateHelpAdd(help)
    local updates = {}
    local deletes = {}
    if help.uid == user.uid then
        alliance.myHelps[help.helpId] = help
    else
        alliance.otherHelps[help.helpId] = help
    end
    table.insert(updates, help)

    alliance.sendAllianceHelpUpdate(updates, deletes)
end

function alliance.updateHelpClose(helpIds)
    -- print('alliance.updateHelpClose...helpIds', utils.serialize(helpIds))
    local deletes = {}
    for _, helpId in pairs(helpIds) do
        alliance.myHelps[helpId] = nil
        alliance.otherHelps[helpId] = nil
        table.insert(deletes, { helpId = helpId })
    end
    if #deletes > 0 then
        -- print('updateHelpClose..deletes...', utils.serialize(deletes))
        alliance.sendAllianceHelpUpdate({}, deletes)
    end
end

function alliance.updateHelpOne(fromUid, help)
    local updates = {}
    local deletes = {}
    if fromUid == user.uid then
        --I help other
        alliance.otherHelps[help.helpId] = nil
        table.insert(deletes, { helpId = help.helpId })
    else
        if help.uid == user.uid then
            -- other help me, update myHelps
            if alliance.myHelps[help.helpId] ~= nil then
                alliance.myHelps[help.helpId] = help
                table.insert(updates, help)

                -- subTime
                alliance.helpAcceptAndSubTime(help)
            end
        else
            if alliance.otherHelps[help.helpId] ~= nil then
                -- other help other, update otherHelps
                if help.times >= help.max then
                    alliance.otherHelps[help.helpId] = nil
                    table.insert(deletes, { helpId = help.helpId })
                else
                    alliance.otherHelps[help.helpId] = help
                    table.insert(updates, help)
                end
            end
        end
    end

    alliance.sendAllianceHelpUpdate(updates, deletes)
end

function alliance.updateHelpAll(fromUid, helps)
    local updates = {}
    local deletes = {}
    if fromUid == user.uid then
        --I help other
        for _, help in pairs(helps) do
            alliance.otherHelps[help.helpId] = nil
            table.insert(deletes, { helpId = help.helpId })
        end
    else
        for _, help in pairs(helps) do
            if help.uid == user.uid then
                -- other help me, update myHelps
                if alliance.myHelps[help.helpId] ~= nil then
                    alliance.myHelps[help.helpId] = help
                    table.insert(updates, help)

                    -- subTime
                    alliance.helpAcceptAndSubTime(help)
                end
            else
                if alliance.otherHelps[help.helpId] ~= nil then
                    -- other help other, update otherHelps
                    if help.times >= help.max then
                        alliance.otherHelps[help.helpId] = nil
                        table.insert(deletes, { helpId = help.helpId })
                    else
                        alliance.otherHelps[help.helpId] = help
                        table.insert(updates, help)
                    end
                end
            end
        end
    end

    alliance.sendAllianceHelpUpdate(updates, deletes)
end

function alliance.helpAcceptAndSubTime(help)
    local info = alliance.info
    if info == nil or info.id == 0 then
        return
    end
    if help == nil or help.uid ~= user.uid then
        return
    end
    local buildingInfo = building.getBuildingById(help.buildingId)
    if buildingInfo == nil then
        return
    end
    local times = help.times - help.accept
    if times <= 0 then
        return
    end

    cdlist.subEndTime(help.cdId, help.reduceTime * times)
    allianceStub.cast_help_accept(info.id, user.uid, help.helpId)
end

function alliance.updateHelpAccept(help)
    local info = alliance.info
    if info == nil or info.id == 0 then
        return
    end
    if alliance.myHelps[help.helpId] ~= nil then
        alliance.myHelps[help.helpId] = help
    end
end

function alliance.sendAllianceHelpUpdate(updates, deletes)
    -- print('alliance.sendAllianceHelpUpdate...', updates, deletes)
    local info = alliance.info
    if info == nil or info.id == 0 or info.memberList[user.uid] == nil then
        return
    end
    if updates == nil then
        updates = {}
        local now = timer.getTimestampCache()
        for _, v in pairs(alliance.myHelps) do
            if now < v.cdTime and v.max > v.times then
                table.insert(updates, v)
            end
        end
        for _, v in pairs(alliance.otherHelps) do
            if now < v.cdTime and v.max > v.times then
                table.insert(updates, v)
            end
        end
    end
    if deletes == nil then
        deletes = {}
    end

    -- print('alliance.sendAllianceHelpUpdate...updates', utils.serialize(updates))
    -- print('alliance.sendAllianceHelpUpdate...deletes', utils.serialize(deletes))
    if next(updates) or next(deletes) then
        agent.sendPktout(p.SC_ALLIANCE_HELP_UPDATE, '@@1=[helpId=i,buildingId=i,times=i,max=i,uid=i,helpDesc=s],2=[helpId=i]', updates, deletes)
    end
end

function alliance.sendAllianceRequestHelp()
    local list = {}
    for cdId, buildingId in pairs(alliance.requestHelpList) do
        if not alliance.isHelpClose(buildingId, cdId) then
            table.insert(list, { buildingId = buildingId })
        else
            alliance.requestHelpList[cdId] = nil
        end
    end
    -- print('alliance.sendAllianceRequestHelp....', utils.serialize(list))
    agent.sendPktout(p.SC_ALLIANCE_REQUEST_HELP, '@@1=[buildingId=i]', list)
end

--武将租借
function alliance.hireHeroUpdate(ownerUid, updates)
    --[[
    注意:
        如果sendAllianceHireHeroUpdate(),
        协议ALLIANCE_HIRE_HERO_UPDATE字段有修改,
        记得同时修改stub/alliance中的hire_hero_update函数中的updates也要相应字段的修改,以协议要的字段为准
    --]]
    -- print('alliance.hireHeroUpdate...ownerUid, updates', ownerUid, updates)
    alliance.sendAllianceHireHeroUpdate(updates, {})
end

function alliance.deleteHireHero(ownerUid, heroIds)
    -- print('alliance.deleteHireHero...ownerUid, heroIds', ownerUid, utils.serialize(heroIds))
    local deletes = {}
    for _, v in pairs(heroIds) do
        table.insert(deletes, { ownerUid = ownerUid, id = v.heroId })
    end

    if ownerUid == user.uid then
        alliance.sendAllianceUserEmployHeroUpdate({}, deletes)
    end

    alliance.sendAllianceHireHeroUpdate({}, deletes)
end

function alliance.sendAllianceHireHeroUpdate(updates, deletes)
    -- print('alliance.sendAllianceHireHeroUpdate...updates, deletes', updates, deletes)
    local info = alliance.info
    if info == nil or info.id == 0 or info.memberList[user.uid] == nil then
        return
    end
    if updates == nil then
        updates = {}
        local allianceHeroLeases = allianceStub.allianceHeroLeases[info.id]
        if allianceHeroLeases then
            for ownerUid, v in pairs(allianceHeroLeases) do
                local member = info.memberList[ownerUid]
                if member then
                    for _, leaseInfo in pairs(v) do
                        local tempInfo = leaseInfo.heroInfo
                        local power = tempInfo:getSingleHeroPower()
                        local rentSilver = leaseInfo.rentSilver
                        local rentTime = leaseInfo.leaseBeginTime
                        local rentCount = leaseInfo.rentCount
                        local todayRentCount = leaseInfo.todayRentCount
                        local newRentCount = leaseInfo.newRentCount
                        -- print('rentSilver, rentTime', rentSilver, rentTime)
                        table.insert(updates, {
                            ownerUid = ownerUid,
                            ownerNickname = member.nickname,
                            id = tempInfo.id,
                            level = tempInfo.level,
                            star = tempInfo.star,
                            power = power,
                            rentSilver = rentSilver,
                            rentTime = rentTime,
                            rentCount = rentCount,
                            todayRentCount = todayRentCount,
                            newRentCount = newRentCount,
                            })
                    end
                end
            end
        end
    end
    if deletes == nil then
        deletes = {}
    end

    -- print('alliance.sendAllianceHireHeroUpdate...updates', utils.serialize(updates))
    -- print('alliance.sendAllianceHireHeroUpdate...deletes', utils.serialize(deletes))
    if #updates > 0 or #deletes > 0 then
        agent.sendPktout(p.SC_ALLIANCE_HIRE_HERO_UPDATE, '@@1=[ownerUid=i,ownerNickname=s,id=i,level=i,star=i,power=i,rentSilver=i,rentTime=i,rentCount=i],2=[ownerUid=i,id=i]', updates, deletes)
    end
end

function alliance.isEmployHeroOpen()
    --allianceEmployCond  {{0,0}, {10,30}, {20,50}}    雇佣武将开启条件 {主城等级, 城主等级}
    local open = false
    local count = 1
    for _, uemploy in pairs(alliance.employHeroList) do
        for _, v in pairs(uemploy) do
            count = count + 1
        end
    end
    local castleLevel = building.castleLevel()
    local userLevel = user.info.level
    local employCond = allianceEmployCond[count]
    -- print('alliance.isEmployHeroOpen...castleLevel, userLevel, employCond', castleLevel, userLevel, utils.serialize(employCond))
    if employCond then
        if castleLevel >= employCond[1] and userLevel >= employCond[2] then
            open = true
        end
    end
    return open
end

function alliance.employHeroUpdate(ownerUid, ownerNickname, heroInfo)
    -- print('alliance.employHeroUpdate....ownerUid, ownerNickname, heroInfo', ownerUid, ownerNickname, heroInfo)
    local employ = {}
    local now = timer.getTimestampCache()
    employ.ownerUid = ownerUid
    employ.ownerNickname = ownerNickname
    employ.expireTime = now + employHeroInterval
    local info = pubHero.newHeroInfo(heroInfo)
    employ.heroInfo = info
    -- print('########employ', utils.serialize(employ))

    -- print('########employHeroList-----begin', utils.serialize(alliance.employHeroList[ownerUid]))
    if alliance.employHeroList[ownerUid] == nil then
        alliance.employHeroList[ownerUid] = {}
    end
    if alliance.employHeroList[ownerUid][info.id] == nil then
        alliance.employHeroList[ownerUid][info.id] = {}
    end
    alliance.employHeroList[ownerUid][info.id] = employ
    alliance.resetEmployExpire()
    -- print('########employHeroList-----end', utils.serialize(alliance.employHeroList[ownerUid]))
    --trigger
    alliance.evtEmployHeroUpdate:trigger()

    local updates = {}
    local power = info:getSingleHeroPower()
    local leftTime = employHeroInterval
    table.insert(updates, {
        ownerUid = ownerUid,
        ownerNickname = ownerNickname,
        id = info.id,
        level = info.level,
        star = info.star,
        power = power,
        leftTime = leftTime,
    })

    alliance.sendAllianceUserEmployHeroUpdate(updates, {})
end

function alliance.sendAllianceUserEmployHeroUpdate(updates, deletes)
    -- print('alliance.sendAllianceUserEmployHeroUpdate...updates, deletes', updates, deletes)
    -- print('alliance.sendAllianceUserEmployHeroUpdate...alliance.employHeroList', utils.serialize(alliance.employHeroList))
    local info = alliance.info
    if info == nil or info.id == 0 or info.memberList[user.uid] == nil then
        return
    end
    if updates == nil then
        updates = {}
        local now = timer.getTimestampCache()
        for _, v in pairs(alliance.employHeroList) do
            for _, employ in pairs(v) do
                local info = employ.heroInfo
                if info then
                    local power = info:getSingleHeroPower()
                    local leftTime = employ.expireTime - now
                    if leftTime > 0 then
                        table.insert(updates, {
                            ownerUid = employ.ownerUid,
                            ownerNickname = employ.ownerNickname,
                            id = info.id,
                            level = info.level,
                            star = info.star,
                            power = power,
                            leftTime = leftTime,
                            })
                    end
                end
            end
        end
    end
    if deletes == nil then
        deletes = {}
    end

    -- print('alliance.sendAllianceUserEmployHeroUpdate...updates', utils.serialize(updates))
    -- print('alliance.sendAllianceUserEmployHeroUpdate...deletes', utils.serialize(deletes))
    agent.sendPktout(p.SC_ALLIANCE_USER_EMPLOY_HERO_UPDATE, '@@1=[ownerUid=i,ownerNickname=s,id=i,level=i,star=i,power=i,leftTime=i],2=[ownerUid=i,id=i]', updates, deletes)
end

function alliance.collectHeroHireCostUpdate(collectSilver, todayRentCount, newRentCount, timeSilver)
    if collectSilver + timeSilver > 0 then
        user.addResource(p.ResourceType.SILVER, collectSilver + timeSilver, p.ResourceGainType.ALLIANCE_HIRE_HERO)
    end

    if collectSilver > 0 then
        print('alliance.collectHeroHireCostUpdate collectSilver todayRentCount newRentCount = ', collectSilver, todayRentCount, newRentCount)
        local remainCount = alliance_hero_lease.count - todayRentCount
        agent.sendPktout(p.SC_ALLIANCE_HERO_HIRE_COLLECT_SILVER_UPDATE, '@@1=i,2=i,3=i', collectSilver, remainCount, newRentCount)
    end

    if timeSilver > 0 then
        print('alliance.collectHeroHireCostUpdate timeSilver = ', timeSilver)
        agent.sendPktout(p.SC_ALLIANCE_HERO_HIRE_TIME_SIVLER_UPDATE, '@@1=i', timeSilver)
    end
end

function alliance.allianceHeroLeaseRecordAdd(leaseRecord)
    -- print('alliance.allianceHeroLeaseRecordUpdate...leaseRecord', leaseRecord)
    if leaseRecord and leaseRecord.employId > 0 then
        local updates = {}
        table.insert(updates, {
            employId = leaseRecord.employId,
            heroId = leaseRecord.heroId,
            nickname = leaseRecord.nickname,
            rentSilver = leaseRecord.rentSilver,
            createTime = leaseRecord.createTime,
        })
        -- print('alliance.allianceHeroLeaseRecordUpdate...updates', utils.serialize(updates))
        alliance.sendAllianceHeroLeaseRecordUpdate(updates)
    end
end

function alliance.allianceHeroLeaseRecordRemove(employIds)
    -- print('alliance.allianceHeroLeaseRecordRemove...employIds', employIds)
    if #employIds > 0 then
        alliance.sendAllianceHeroLeaseRecordUpdate({}, employIds)
    end
end

function alliance.sendAllianceHeroLeaseRecordUpdate(updates, deletes)
    -- print('alliance.sendAllianceHeroLeaseRecordUpdate...updates, deletes', updates, deletes)
    local info = alliance.info
    if info == nil or info.id == 0 then
        return
    end
    local aid = info.id
    local uid = user.uid
    if updates == nil then
        updates = {}
        local arecord = allianceStub.allianceHeroleaseRecords[uid]
        if arecord then
            for _, v in ipairs(arecord) do
                table.insert(updates, { employId = v.employId, heroId = v.heroId, nickname = v.nickname, rentSilver = v.rentSilver, createTime = v.createTime })
            end
        end
    end
    if deletes == nil then
        deletes = {}
    end
    local totalSilver = allianceStub.getHeroLeaseIncome(aid, uid)
    -- print('alliance.sendAllianceHeroLeaseRecordUpdate...totalSilver', utils.serialize(totalSilver))
    -- print('alliance.sendAllianceHeroLeaseRecordUpdate...updates', utils.serialize(updates))
    -- print('alliance.sendAllianceHeroLeaseRecordUpdate...deletes', utils.serialize(deletes))
    agent.sendPktout(p.SC_ALLIANCE_HERO_LEASE_RECORD_UPDATE, '@@1=i,2=[employId=i,heroId=i,nickname=s,rentSilver=i,createTime=i],3=[employId=i]', totalSilver, updates, deletes)
end

--联盟记录
function alliance.allianceRecordUpdate(record)
    -- print('alliance.allianceRecordUpdate...record', utils.serialize(record))
    local updates = {}
    table.insert(updates, { id = record.id, param1 = record.param1, createTime = record.createTime })
    alliance.sendAllianceRecordUpdate(updates)
end

function alliance.sendAllianceRecordUpdate(updates)
    -- print('alliance.sendAllianceRecordUpdate...updates', updates)
    local info = alliance.info
    if info == nil or info.id == 0 or info.memberList[user.uid] == nil then
        return
    end
    if updates == nil then
        updates = {}
        local arecord = allianceStub.allianceRecords[info.id]
        -- print('alliance.sendAllianceRecordUpdate...arecord', utils.serialize(arecord))
        if arecord then
            for _, v in ipairs(arecord) do
                if v.id and v.id > 0 then
                    table.insert(updates, { id = v.id, param1 = v.param1, createTime = v.createTime })
                end
            end
        end
    end
    -- print('alliance.sendAllianceRecordUpdate...updates', utils.serialize(updates))
    if #updates > 0 then
        agent.sendPktout(p.SC_ALLIANCE_RECORD_UPDATE, '@@1=[id=i,param1=s,createTime=i]', updates)
    end
end

--联盟增益buff
function alliance.initAllianceBuff()
    --检查联盟buff
    alliance.checkAllianceBuff()
    --处理联盟增益CD
    for buffId, v in pairs(alliance.myAllianceBuffs) do
        if v.cdId > 0 then
            local cd = cdlist.getCD(v.cdId)
            if cd == nil then
                print('alliance.initAllianceBuff...cd is nil.. buffId, cdId', buffId, v.cdId)
                alliance.cdlistCallback(v.cdId)
            elseif cd:isCooling() then
                print('alliance.initAllianceBuff...cd isCooling..buffId, cdId, cdLeftSesonds =', buffId, v.cdId, cd:getRemainTime())
                cd:setCb(alliance.cdlistCallback)
            else
                print('alliance.initAllianceBuff... buffId, cdId', buffId, v.cdId)
                alliance.cdlistCallback(v.cdId)
            end
        end
    end
    --
    alliance.sendAllianceBuffUpdate()
end

function alliance.checkAllianceBuff()
    local info = alliance.info
    if info == nil or info.id == 0 then
        return
    end
    local myself = info.memberList[user.uid]
    if myself == nil then
        return
    end
    local now = timer.getTimestampCache()
    for _, v in pairs(info.allianceBuffList) do
        local leftTime = v.endTimestamp - now
        if leftTime < 0 then
            leftTime = 0
        end
        --有效buff
        if leftTime > 0 and myself.joinTime <= v.createTimestamp then
            local myBuff = alliance.myAllianceBuffs[v.buffId]
            if myBuff == nil then
                --离线时开启buff
                alliance.addAllianceBuff(v.buffId, v.type, leftTime, v.createTimestamp)
            else
                --自己主动撤销，但buff还没到期,离线后又开启同一个buff
                if not myBuff.open and myBuff.closedTime <= v.createTimestamp then
                    alliance.addAllianceBuff(v.buffId, v.type, leftTime, v.createTimestamp)
                end
            end
        end
    end
end

function alliance.cdlistCallback(cdId)
    for buffId, v in pairs(alliance.myAllianceBuffs) do
        if v.cdId == cdId then
            v.cdId = 0
            v.open = false
            v.closedTime = timer.getTimestampCache()

            --event trigger
            alliance.evtAllianceBuffRemove:trigger()
        end
    end
end

function alliance.addAllianceBuff(buffId, type, leftTime, createTimestamp)
    -- print('alliance.addAllianceBuff...buffId, type, leftTime, createTimestamp', buffId, type, leftTime, createTimestamp)
    local cdId = cdlist.addCD(0, leftTime, alliance.cdlistCallback)
    local myBuff = alliance.myAllianceBuffs[buffId]
    if myBuff == nil then
        alliance.myAllianceBuffs[buffId] = {}
        myBuff = alliance.myAllianceBuffs[buffId]
    end
    myBuff.type = type
    myBuff.cdId = cdId
    myBuff.open = true
    myBuff.createTimestamp = createTimestamp
    myBuff.closedTime = 0

    --event trigger
    alliance.evtAllianceBuffAdd:trigger()
end

function alliance.removeAllianceBuff(buffId)
    -- print('alliance.removeAllianceBuff...buffId', buffId)
    local myBuff = alliance.myAllianceBuffs[buffId]
    if myBuff == nil then
        alliance.myAllianceBuffs[buffId] = {}
        myBuff = alliance.myAllianceBuffs[buffId]
    end
    myBuff.cdId = 0
    myBuff.open = false
    myBuff.closedTime = timer.getTimestampCache()

    --event trigger
    alliance.evtAllianceBuffRemove:trigger()
end

function alliance.allianceBuffOpen(allianceBuff)
    -- print('alliance.allianceBuffOpen...allianceBuff', utils.serialize(allianceBuff))
    local updates = {}
    local leftTime = allianceBuff.endTimestamp - timer.getTimestampCache()
    if leftTime < 0 then
        leftTime = 0
    else
        --
        alliance.addAllianceBuff(allianceBuff.buffId, allianceBuff.type, leftTime, allianceBuff.createTimestamp)
    end
    table.insert(updates, { buffId = allianceBuff.buffId, useCount = allianceBuff.useCount, leftTime = leftTime })
    alliance.sendAllianceBuffUpdate(updates)
end

function alliance.allianceBuffClosed(buffId)
    -- print('alliance.allianceBuffClosed...buffId', buffId)
    local allianceBuff = alliance.info.allianceBuffList[buffId]
    if allianceBuff then
        local updates = {}
        table.insert(updates, { buffId = buffId, useCount = allianceBuff.useCount, leftTime = 0 })
        alliance.sendAllianceBuffUpdate(updates)
    end

    --
    alliance.removeAllianceBuff(buffId)
end

function alliance.allianceBuffUpdate()
    -- print('alliance.allianceBuffUpdate...')
    alliance.sendAllianceBuffUpdate()
end

function alliance.sendAllianceBuffUpdate(updates)
    -- print('alliance.sendAllianceBuffUpdate...updates', updates)
    local info = alliance.info
    if info == nil or info.id == 0 then
        return
    end
    local myself = info.memberList[user.uid]
    if myself == nil then
        return
    end

    if updates == nil then
        updates = {}
        local now = timer.getTimestampCache()
        for _, v in pairs(info.allianceBuffList) do
            local leftTime = v.endTimestamp - now
            if leftTime < 0 then
                leftTime = 0
            end
            table.insert(updates, { buffId = v.buffId, useCount = v.useCount, leftTime = leftTime })
        end
    end
    -- print('alliance.sendAllianceBuffUpdate... aid, updates', info.id, utils.serialize(updates))
    agent.sendPktout(p.SC_ALLIANCE_BUFF_UPDATE, '@@1=i,2=[buffId=i,useCount=i,leftTime=i]', info.id, updates)
end

pktHandlers[p.CS_ALLIANCE_CHECK_NAME] = function(pktin)
    local isCheckName = pktin:read('b')
    local name = pktin:read('s')
    print('pktHandlers CS_ALLIANCE_CHECK_NAME ...isCheckName, name:', isCheckName, name)
    agent.queueJob(function()
        local ret = allianceStub.call_checkName(isCheckName, name)
        print('pktHandlers CS_ALLIANCE_CHECK_NAME ... name, ret.', name, ret)
        agent.sendPktout(p.SC_ALLIANCE_CHECK_NAME_RESPONSE, ret)
    end)
end

pktHandlers[p.CS_ALLIANCE_CREATE] = function(pktin)
    if alliance.info ~= nil and alliance.info.id ~= 0 then
        print('p.CS_ALLIANCE_CREATE...alliance is exist')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    local allianceName, allianceNickname, bannerId = pktin:read('ssi')
    -- print('p.CS_ALLIANCE_CREATE...allianceName, allianceNickname, bannerId', allianceName, allianceNickname, bannerId)

    --检查等级
    local castleLevel = building.castleLevel()
    if castleLevel < create_level_cost.castleLevel then
        print('p.CS_ALLIANCE_CREATE...castleLevel < needCastleLevel', castleLevel, create_level_cost.castleLevel)
        agent.sendNoticeMessage(p.ErrorCode.CASTLE_LEVEL_NOT_ENOUGH, '', 1)
        return
    end
    local userLevel = user.info.level
    if userLevel < create_level_cost.userLevel then
        print('p.CS_ALLIANCE_CREATE...userLevel < needUserLevel', userLevel, create_level_cost.userLevel)
        agent.sendNoticeMessage(p.ErrorCode.USER_LEVEL_NOT_ENOUGH, '', 1)
        return
    end
    --检查资源
    local gold = create_level_cost.gold
    if not user.isResourceEnough(p.ResourceType.GOLD, gold) then
        print('p.CS_ALLIANCE_CREATE...gold is not enough')
        agent.sendNoticeMessage(p.ErrorCode.PUBLIC_GOLD_NOT_ENOUGH, '', 1)
        return
    end
    local silver = create_level_cost.silver
    if not user.isResourceEnough(p.ResourceType.SILVER, silver) then
        print('p.CS_ALLIANCE_CREATE...silver is not enough')
        agent.sendNoticeMessage(p.ErrorCode.PUBLIC_SILVER_NOT_ENOUGH, '', 1)
        return
    end

    --
    local uid = user.uid
    local userNickname = user.info.nickname
    local headId = user.info.headId
    local vipLevel = agent.vip.vipLevel()
    local power = user.info.totalPower
    local castleLevel = user.info.castleLevel
    local x = agent.map.info.x
    local y = agent.map.info.y
    local level = building.getBuildingLevelByType(p.BuildingType.MARKET)
    local marketIsOpen = false
    if level > 0 then
        marketIsOpen = true
    end
    agent.queueJob(function()
        print('CS_ALLIANCE_CREATE, allianceName, allianceNickname, bannerId, uid, userNickname, headId, userLevel, vipLevel, power, castleLevel, marketIsOpen', allianceName, allianceNickname, bannerId, uid, userNickname, headId, userLevel, vipLevel, power, castleLevel, marketIsOpen)
        local ret = allianceStub.call_create(allianceName, allianceNickname, bannerId, uid, userNickname, headId, userLevel, vipLevel, power, castleLevel, x, y, marketIsOpen)
        agent.sendPktout(p.SC_ALLIANCE_CREATE_RESPONSE, ret)
        if ret ~= p.ErrorCode.SUCCESS then
            agent.sendNoticeMessage(ret, '', 1)
            print('pktHandlers CS_ALLIANCE_CREATE ... allianceName, allianceNickname, bannerId, uid, userNickname, ret error', allianceName, allianceNickname, bannerId, uid, userNickname, ret)
            return
        end
        if gold > 0 then
            user.removeResource(p.ResourceType.GOLD, gold, p.ResourceConsumeType.ALLIANCE_CREATE)
        end
        if silver > 0 then
            user.removeResource(p.ResourceType.SILVER, silver, p.ResourceConsumeType.ALLIANCE_CREATE)
        end
    end)
end

pktHandlers[p.CS_ALLIANCE_SEARCH] = function(pktin)
    local allianceName, flag = pktin:read('si')
    -- print('p.CS_ALLIANCE_SEARCH...allianceName, flag', allianceName, flag)
    local list = allianceStub.searchByName(allianceName, flag)
    -- print('pktHandlers CS_ALLIANCE_SEARCH ...list', #list)
    if list then
        agent.sendPktout(p.SC_ALLIANCE_SEARCH_RESPONSE, '@@1=[aid=i,name=s,nickname=s,bannerId=i,level=i,alliesCount=i,alliesMax=i,toatlActive=i,power=i]', list)
    end
end

pktHandlers[p.CS_ALLIANCE_SEARCH_BY_ID] = function(pktin)
    local aid = pktin:read('i')
    -- print('p.CS_ALLIANCE_SEARCH_BY_ID...aid', aid)
    local list = allianceStub.searchById(aid)
    -- print('pktHandlers CS_ALLIANCE_SEARCH_BY_ID ...list', #list)
    if list then
        -- for k, v in pairs(list) do
        --     print('pktHandlers CS_ALLIANCE_SEARCH_BY_ID ...alliance', utils.serialize(v))
        -- end
        agent.sendPktout(p.SC_ALLIANCE_SEARCH_RESPONSE, '@@1=[aid=i,name=s,nickname=s,bannerId=i,level=i,alliesCount=i,alliesMax=i,toatlActive=i,power=i]', list)
    end
end

pktHandlers[p.CS_ALLIANCE_APPLY] = function(pktin)
    local aid = pktin:read('i')
    local uid = user.uid
    local nickname = user.info.nickname
    local headId = user.info.headId
    local userLevel = user.info.level
    local vipLevel = agent.vip.vipLevel()
    --看是否有加入联盟CD
    local now = timer.getTimestampCache()
    if alliance.allianceCdTime - now > 0 then
        print('p.CS_ALLIANCE_APPLY...alliance CD', alliance.allianceCdTime - now)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_CD_COOLING, '', 1)
        -- agent.sendNoticeMessage(p.ErrorCode.SUCCESS, '', 1)
        return
    end
    --联盟是否存在
    if not allianceStub.isAllianceExist(aid) then
        print('p.CS_ALLIANCE_APPLY...alliance does not exist...aid', aid)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    --检查申请玩家是否有联盟
    if allianceStub.isJoinAlliance(uid) then
        print('p.CS_ALLIANCE_APPLY...user is join alliance...uid', uid)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_JOINED, '', 1)
        return
    end
    --检查该玩家是否在申请列表里
    if allianceStub.isApplyListHasUser(aid, uid) then
        print('p.CS_ALLIANCE_APPLY...user do not need to repeat the application...')
        --不发tips
        agent.sendPktout(p.SC_ALLIANCE_APPLY_RESPONSE, p.ErrorCode.SUCCESS)
        return
    end
    --检查联盟人数是否已满
    if allianceStub.isAllianceFull(aid) then
        print('p.CS_ALLIANCE_APPLY...alliance is full...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_FULL, '', 1)
        return
    end
    agent.queueJob(function()
        local ret = allianceStub.call_apply_request(aid, uid, nickname, headId, userLevel, vipLevel)
        print('p.CS_ALLIANCE_APPLY...ret', ret)
        if ret ~= p.ErrorCode.SUCCESS then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        agent.sendPktout(p.SC_ALLIANCE_APPLY_RESPONSE, ret)
    end)
end

pktHandlers[p.CS_ALLIANCE_APPLY_ACCEPT] = function(pktin)
    local applyUid, isAccept = pktin:read('ib')
    local info = alliance.info
    --联盟是否存在
    if info == nil or info.id == 0 then
        print('p.CS_ALLIANCE_APPLY_ACCEPT...no join alliance...')
        agent.sendPktout(p.SC_ALLIANCE_APPLY_ACCEPT_RESPONSE, p.ErrorCode.FAIL)
        return
    end
    local aid = info.id
    local uid = user.uid
    --检查申请玩家是否有联盟
    if allianceStub.isJoinAlliance(applyUid) then
        print('p.CS_ALLIANCE_APPLY_ACCEPT...user is join alliance...applyUid', applyUid)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    --检查联盟人数是否已满
    if allianceStub.isAllianceFull(aid) then
        print('p.CS_ALLIANCE_APPLY_ACCEPT...alliance is full...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_FULL, '', 1)
        return
    end
    --是否有权限
    local m = info.memberList[uid]
    if m == nil then
        print('p.CS_ALLIANCE_APPLY_ACCEPT...member is nil...aid, uid', aid, uid)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_MEMBER, '', 1)
        return
    end
    if not m:haveRankPermit(p.AllianceRankPermitType.ALLIANCE_ACCEPT_JOIN) then
        print('p.CS_ALLIANCE_APPLY_ACCEPT...user without permit...uid, rankLevel', uid, m.rankLevel)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_WITHOUT_PERMIT, '', 1)
        return
    end

    agent.queueJob(function()
        local now = timer.getTimestampCache()
        local allianceCdTimestamp = 0
        local userInfo = agent.map.call_getPlayerInfo(applyUid)
        if userInfo then
            allianceCdTimestamp = userInfo.allianceCdTimestamp
        end
        if now - allianceCdTimestamp < 0 then
            print('p.CS_ALLIANCE_APPLY_ACCEPT...alliance CD', allianceCdTimestamp - now)
            agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_CD_COOLING, '', 1)
            return
        end
        local ret = allianceStub.call_apply_accept(aid, uid, isAccept, applyUid)
        -- print('p.CS_ALLIANCE_APPLY_ACCEPT...ret', ret)
        -- print("userInfo", utils.serialize(userInfo))
        if ret ~= p.ErrorCode.SUCCESS then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        agent.sendPktout(p.SC_ALLIANCE_APPLY_ACCEPT_RESPONSE, ret)
    end)
end

pktHandlers[p.CS_ALLIANCE_INVITE] = function(pktin)
    --邀请者是否有联盟
    local info = alliance.info
    if info  == nil or info.id == 0 then
        print('p.CS_ALLIANCE_INVITE...no alliance...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    local aid = info.id
    --检查联盟人数是否已满
    if allianceStub.isAllianceFull(aid) then
        print('p.CS_ALLIANCE_INVITE...alliance is full...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_FULL, '', 1)
        return
    end
    --邀请者是否有权限
    local m = info.memberList[user.uid]
    if m == nil then
        print('p.CS_ALLIANCE_INVITE...member is nil...aid, uid', aid, user.uid)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_MEMBER, '', 1)
        return
    end
    if not m:haveRankPermit(p.AllianceRankPermitType.ALLIANCE_INVITE) then
        print('p.CS_ALLIANCE_INVITE...user without permit...uid, rankLevel', user.uid, m.rankLevel)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_WITHOUT_PERMIT, '', 1)
        return
    end
    --被邀请者是否有联盟
    local invitedUids = {}
    local len = pktin:readInteger()
    for i = 1, len do
        local uid = pktin:readInteger()
        if not allianceStub.isJoinAlliance(uid) then
            table.insert(invitedUids, uid)
        end
    end
    if next(invitedUids) == nil then
        print('p.CS_ALLIANCE_INVITE..inviteList is empty...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_JOINED, '', 1)
        return
    end
    --5.cast
    allianceStub.cast_invite(aid, user.uid, invitedUids)

    agent.sendPktout(p.SC_ALLIANCE_INVITE_RESPONSE, p.ErrorCode.SUCCESS)
end

pktHandlers[p.CS_ALLIANCE_INVITE_ACCEPT] = function(pktin)
    local aid, isAccept = pktin:read('ib')
    local info = alliance.info
    --是否已加入联盟
    if info and info.id > 0 then
        print('p.CS_ALLIANCE_INVITE_ACCEPT...join alliance...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_JOINED, '', 1)
        return
    end
    --联盟是否存在
    if not allianceStub.isAllianceExist(aid) then
        print('p.CS_ALLIANCE_INVITE_ACCEPT...alliance is not exist...aid', aid)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    --检查联盟人数是否已满
    if allianceStub.isAllianceFull(aid) then
        print('p.CS_ALLIANCE_INVITE_ACCEPT...alliance is full...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_FULL, '', 1)
        return
    end
    --看是否有加入联盟CD
    local now = timer.getTimestampCache()
    if alliance.allianceCdTime - now > 0 then
        print('p.CS_ALLIANCE_INVITE_ACCEPT...alliance CD', alliance.allianceCdTime - now)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_CD_COOLING, '', 1)
        return
    end
    --是否被邀请 myInvitedList
    local uid = user.uid
    local userNickname = user.info.nickname
    local headId = user.info.headId
    local userLevel = user.info.level
    local totalPower = user.totalPower
    local castleLevel = user.castleLevel
    local vipLevel = agent.vip.vipLevel()
    local x = agent.map.info.x
    local y = agent.map.info.y
    local level = building.getBuildingLevelByType(p.BuildingType.MARKET)
    local marketIsOpen = false
    if level > 0 then
        marketIsOpen = true
    end
    if not allianceStub.isAllianceInvited(aid, uid)then
        print('p.CS_ALLIANCE_INVITE_ACCEPT...no alliance invited...uid, aid', uid, aid)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_INVITE_INVALID, '', 1)
        return
    end
    agent.queueJob(function()
        local ret = allianceStub.call_invite_accept(aid, uid, userNickname, headId, userLevel, vipLevel, isAccept, castleLevel, x, y, marketIsOpen)
        print('p.CS_ALLIANCE_INVITE_ACCEPT...ret', ret)
        if ret == p.ErrorCode.SUCCESS then
            agent.sendPktout(p.SC_ALLIANCE_INVITE_ACCEPT_RESPONSE, ret)
        else
            agent.sendNoticeMessage(ret, '', 1)
        end
    end)
end

pktHandlers[p.CS_ALLIANCE_QUIT] = function(pktin)
    --联盟是否存在
    if alliance.info == nil or alliance.info.id == 0 then
        print('p.CS_ALLIANCE_QUIT...no join alliance...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    --盟主不可以退出联盟
    if user.uid == alliance.info.leaderId then
        print('p.CS_ALLIANCE_QUIT...leader can not quit alliance')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_LEADER_TRANSFER, '', 1)
        return
    end

    agent.queueJob(function()
        local ret = allianceStub.call_quit(alliance.info.id, user.uid)
        print('p.CS_ALLIANCE_QUIT...ret', ret)
        if ret ~= p.ErrorCode.SUCCESS then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        agent.sendPktout(p.SC_ALLIANCE_QUIT_RESPONSE, ret)
    end)
end

pktHandlers[p.CS_ALLIANCE_KICK_MEMBER] = function(pktin)
    local kickUid = pktin:read('i')
    print('p.CS_ALLIANCE_KICK_MEMBER...kickUid', kickUid)
    --1.联盟是否存在
    local info = alliance.info
    if info == nil or info.id == 0 then
        print('p.CS_ALLIANCE_KICK_MEMBER...no join alliance...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    --2.是否有权限
    local myself = info.memberList[user.uid]
    if myself == nil or not myself:haveRankPermit(p.AllianceRankPermitType.KICK) then
        print('p.CS_ALLIANCE_KICK_MEMBER... no permit')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_WITHOUT_PERMIT, '', 1)
        return
    end
    --3.被踢人是否是联盟成员
    local kickMember = info.memberList[kickUid]
    if kickMember == nil then
        print('p.CS_ALLIANCE_KICK_MEMBER...alliance not exist kickUser...kickUid', kickUid)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_MEMBER, '', 1)
        return
    end
    --职位高低
    local rankDiff = kickMember.rankLevel - myself.rankLevel
    if rankDiff < 1 then
        print('p.CS_ALLIANCE_KICK_MEMBER...myself.rankLevel, kickMember.rankLevel', myself.rankLevel, kickMember.rankLevel)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_WITHOUT_PERMIT, '', 1)
        return
    end

    agent.queueJob(function()
        local ret = allianceStub.call_kick(info.id, user.uid, kickUid)
        print('p.CS_ALLIANCE_KICK_MEMBER...ret', ret)
        if ret ~= p.ErrorCode.SUCCESS then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        agent.sendPktout(p.SC_ALLIANCE_KICK_MEMBER_RESPONSE, ret, kickUid)
    end)
end

pktHandlers[p.CS_ALLIANCE_BANNER_CHANGE] = function(pktin)
    local bannerId = pktin:read('i')
    print('p.CS_ALLIANCE_BANNER_CHANGE...bannerId', bannerId)
    --联盟是否存在
    local info = alliance.info
    if info == nil or info.id == 0 then
        print('p.CS_ALLIANCE_BANNER_CHANGE...no join alliance...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    --是否有权限
    local myself = info.memberList[user.uid]
    if myself == nil or not myself:haveRankPermit(p.AllianceRankPermitType.CHANGE_BANNER) then
        print('p.CS_ALLIANCE_BANNER_CHANGE... no permit')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_WITHOUT_PERMIT, '', 1)
        return
    end
    --换联盟旗帜CD
    local now = timer.getTimestampCache()
    if now < info.changeBannerCdTime then
        print('p.CS_ALLIANCE_BANNER_CHANGE... change Banner cd', info.changeBannerCdTime - now)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_CHANGE_BANER_CD_COOLING, '', 1)
        return
    end
    --修改联盟旗帜要花费元宝
    if not user.isResourceEnough(p.ResourceType.GOLD, change_banner_cost_gold) then
        print('p.CS_ALLIANCE_BANNER_CHANGE...gold is not enough')
        agent.sendNoticeMessage(p.ErrorCode.PUBLIC_GOLD_NOT_ENOUGH, '', 1)
        return
    end

    agent.queueJob(function()
        local ret = allianceStub.call_change_banner(info.id, user.uid, bannerId)
        print('p.CS_ALLIANCE_BANNER_CHANGE...ret', ret)
        if ret == p.ErrorCode.SUCCESS then
            agent.sendPktout(p.SC_ALLIANCE_BANNER_CHANGE_RESPONSE, ret)
            user.removeResource(p.ResourceType.GOLD, change_banner_cost_gold, p.ResourceConsumeType.ALLIANCE_CHANGE_BANNER)
        else
            agent.sendNoticeMessage(ret, '', 1)
        end
    end)
end

pktHandlers[p.CS_ALLIANCE_ANNOUNCEMENT_CHANGE] = function(pktin)
    local announcement = pktin:read('s')
    print('p.CS_ALLIANCE_ANNOUNCEMENT_CHANGE...announcement', announcement)
    --联盟是否存在
    local info = alliance.info
    if info == nil or info.id == 0 then
        print('p.CS_ALLIANCE_ANNOUNCEMENT_CHANGE...no join alliance...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    --是否有权限
    local myself = info.memberList[user.uid]
    if myself == nil or not myself:haveRankPermit(p.AllianceRankPermitType.CHANGE_ANNOUNCEMENT) then
        print('p.CS_ALLIANCE_ANNOUNCEMENT_CHANGE... no permit')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_WITHOUT_PERMIT, '', 1)
        return
    end
    --脏字过滤
    announcement = trie.filter(announcement)
    --限制200字节以内
    local length = string.len(announcement)
    if length <=0 or length > announcementLen then
        print('p.CS_ALLIANCE_ANNOUNCEMENT_CHANGE... announcement length is wrong...length, maxLength=', length, announcementLen)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    agent.queueJob(function()
        local ret = allianceStub.call_change_announcement(info.id, user.uid, announcement)
        print('p.CS_ALLIANCE_ANNOUNCEMENT_CHANGE...ret', ret)
        if ret ~= p.ErrorCode.SUCCESS then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        agent.sendPktout(p.SC_ALLIANCE_ANNOUNCEMENT_CHANGE_RESPONSE, ret)
    end)
end

pktHandlers[p.CS_ALLIANCE_RANK_UP] = function(pktin)
    local upUid = pktin:read('i')
    print('p.CS_ALLIANCE_RANK_UP...upUid', upUid)

    --联盟是否存在
    local info = alliance.info
    if info == nil or info.id == 0 then
        print('p.CS_ALLIANCE_RANK_UP...no join alliance...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    --是否有权限
    local myself = info.memberList[user.uid]
    if myself == nil or not myself:haveRankPermit(p.AllianceRankPermitType.RANK_UP) then
        print('p.CS_ALLIANCE_RANK_UP... no permit')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_WITHOUT_PERMIT, '', 1)
        return
    end
    --升职人是否是联盟成员
    local upMember = info.memberList[upUid]
    if upMember == nil then
        print('p.CS_ALLIANCE_RANK_UP...alliance not exist upUser...upUid', upUid)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_MEMBER, '', 1)
        return
    end
    --职位高低 副盟主不能再提升职位
    local rankDiff = upMember.rankLevel - myself.rankLevel
    if rankDiff < 1 or upMember.rankLevel == 2 then
        print('p.CS_ALLIANCE_RANK_UP...myself.rankLevel, upMember.rankLevel', myself.rankLevel, upMember.rankLevel)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_RANK_UP, '', 1)
        return
    end

    agent.queueJob(function()
        local ret = allianceStub.call_rankUp(info.id, user.uid, upUid)
        print('p.CS_ALLIANCE_RANK_UP...ret', ret)
        if ret ~= p.ErrorCode.SUCCESS then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        agent.sendPktout(p.SC_ALLIANCE_RANK_UP_RESPONSE, ret)
    end)
end

pktHandlers[p.CS_ALLIANCE_RANK_DOWN] = function(pktin)
    local downUid = pktin:read('i')
    print('p.CS_ALLIANCE_RANK_DOWN...downUid', downUid)

    --联盟是否存在
    local info = alliance.info
    if info == nil or info.id == 0 then
        print('p.CS_ALLIANCE_RANK_DOWN...no join alliance...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    --是否有权限
    local myself = info.memberList[user.uid]
    if myself == nil or not myself:haveRankPermit(p.AllianceRankPermitType.RANK_DOWN) then
        print('p.CS_ALLIANCE_RANK_DOWN... no permit')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_WITHOUT_PERMIT, '', 1)
        return
    end
    --升职人是否是联盟成员
    local downMember = info.memberList[downUid]
    if downMember == nil then
        print('p.CS_ALLIANCE_RANK_DOWN...alliance not exist downUser...downUid', downUid)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_MEMBER, '', 1)
        return
    end
    --职位高低 成员不能再降低职位
    local rankDiff = downMember.rankLevel - myself.rankLevel
    if rankDiff < 1 or downMember.rankLevel == 4 then
        print('p.CS_ALLIANCE_RANK_DOWN...myself.rankLevel <= downMember.rankLevel', myself.rankLevel, downMember.rankLevel)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_RANK_DOWN, '', 1)
        return
    end

    agent.queueJob(function()
        local ret = allianceStub.call_rankDown(info.id, user.uid, downUid)
        print('p.CS_ALLIANCE_RANK_DOWN...ret', ret)
        if ret ~= p.ErrorCode.SUCCESS then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        agent.sendPktout(p.SC_ALLIANCE_RANK_DOWN_RESPONSE, ret)
    end)
end

pktHandlers[p.CS_ALLIANCE_LEADER_TRANSFER] = function(pktin)
    local transferUid = pktin:read('i')
    --联盟是否存在
    local info = alliance.info
    if info == nil or info.id == 0 then
        print('p.CS_ALLIANCE_LEADER_TRANSFER...no join alliance...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    if info.leaderId ~= user.uid then
        print('p.CS_ALLIANCE_LEADER_TRANSFER...user is not leader, can not transfer...myUid, leaderId', user.uid, info.leaderId)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_WITHOUT_PERMIT, '', 1)
        return
    end
    --是否有权限
    local myself = info.memberList[user.uid]
    if myself == nil or not myself:haveRankPermit(p.AllianceRankPermitType.TRANSFER) then
        print('p.CS_ALLIANCE_LEADER_TRANSFER... no permit')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_WITHOUT_PERMIT, '', 1)
        return
    end
    --
    local transferMember = info.memberList[transferUid]
    if transferMember == nil then
        print('p.CS_ALLIANCE_LEADER_TRANSFER...alliance not exist downUser...transferUid', transferUid)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_WITHOUT_PERMIT, '', 1)
        return
    end
    if myself.rankLevel ~= 1 or transferMember.rankLevel ~= 2 then
        print('p.CS_ALLIANCE_LEADER_TRANSFER..rankLevel is wrong....myself.rankLevel, transferMember.rankLevel', myself.rankLevel, transferMember.rankLevel)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_WITHOUT_PERMIT, '', 1)
        return
    end

    agent.queueJob(function()
        local ret = allianceStub.call_leader_transfer(info.id, user.uid, transferUid)
        print('p.CS_ALLIANCE_LEADER_TRANSFER...ret', ret)
        if ret ~= p.ErrorCode.SUCCESS then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        agent.sendPktout(p.SC_ALLIANCE_LEADER_TRANSFER_RESPONSE, ret)
        
        local params = {param1 = myself.nickname, param2 = transferMember.nickname}
        alliance.sendAllianceEventToChat(p.ChatSubType.ALLIANCE_LEADER_TRANSFER, utils.serialize(params))
    end)
end

pktHandlers[p.CS_ALLIANCE_LEADER_REPLACE] = function(pktin)
    --联盟是否存在
    local info = alliance.info
    if info == nil or info.id == 0 then
        print('p.CS_ALLIANCE_LEADER_REPLACE...no join alliance...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    if info.leaderId == user.uid then
        print('p.CS_ALLIANCE_LEADER_REPLACE...user is leader, can not replace...myUid, leaderId', user.uid, info.leaderId)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_WITHOUT_PERMIT, '', 1)
        return
    end
    --是否有权限
    local myself = info.memberList[user.uid]
    if myself == nil or not myself:haveRankPermit(p.AllianceRankPermitType.REPLACE_LEADER) then
        print('p.CS_ALLIANCE_LEADER_REPLACE... no permit')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_WITHOUT_PERMIT, '', 1)
        return
    end
    --比自己职位高 有7天内上线的
    local now = timer.getTimestampCache()
    for _, v in pairs(info.memberList) do
        if v.rankLevel < myself.rankLevel then
            if v.lastOnlineTime + replaceInterval < now then
                print('p.CS_ALLIANCE_LEADER_REPLACE... no permit')
                agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_WITHOUT_PERMIT, '', 1)
                return
            end
        end
    end

    agent.queueJob(function()
        local ret = allianceStub.call_leader_replace(info.id, user.uid)
        print('p.CS_ALLIANCE_LEADER_REPLACE...ret', ret)
        if ret ~= p.ErrorCode.SUCCESS then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        agent.sendPktout(p.SC_ALLIANCE_LEADER_REPLACE_RESPONSE, ret)
    end)
end

pktHandlers[p.CS_ALLIANCE_SCIENCE_DONATE] = function(pktin)
    local groupId, level, doanteType, donationId = pktin:read('iiii')
    print("###p.CS_ALLIANCE_SCIENCE_DONATE...", donationId)
    --联盟是否存在
    local info = alliance.info
    if info == nil or info.id == 0 then
        print('p.CS_ALLIANCE_SCIENCE_DONATE...no join alliance...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    --是否是联盟成员
    local myself = info.memberList[user.uid]
    if myself == nil then
        print('p.CS_ALLIANCE_SCIENCE_DONATE... not alliance member..aid, uid', info.id, user.uid)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_MEMBER, '', 1)
        return
    end
    --新人进入联盟后需要满4小时才能捐献
    local now = timer.getTimestampCache()
    if now < newMemberDonateInterval + myself.joinTime then
        print('p.CS_ALLIANCE_SCIENCE_DONATE... new member need 4 hours to donate...leftSeconds', now - myself.joinTime)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_SCIENCE_DONATE_CD, '', 1)
        return
    end

    --捐献物品是否存在
    local donateTpl = t.allianceDonate[doanteType]
    if donateTpl == nil or donateTpl[donationId] == nil then
        print('p.CS_ALLIANCE_SCIENCE_DONATE...donate item is not exist...doanteType, donationId', doanteType, donationId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    donateTpl = donateTpl[donationId]
    --科技是否存在
    local groupTpl = t.allianceScience[groupId]
    if groupTpl == nil then
        print('p.CS_ALLIANCE_SCIENCE_DONATE...alliance science is not exist...groupId', groupId)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local tplLevel = groupTpl[level]
    if tplLevel == nil then
        print('p.CS_ALLIANCE_SCIENCE_DONATE...alliance science is not exist...groupId, level', groupId, level)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local maxLevel = #groupTpl
    --科技是否满级
    if level >= maxLevel then
        print('p.CS_ALLIANCE_SCIENCE_DONATE...alliance science level is full...level, maxLevel', level, maxLevel)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_SCIENCE_LEVEL_FULL, '', 1)
        return
    end
    --科技是否开启
    if info.level < tplLevel.allianceLevel then
        print('p.CS_ALLIANCE_SCIENCE_DONATE...alliance science is not open...groupId, level, allianceLevel,needAllianceLevel =', groupId, level, info.level, tplLevel.allianceLevel)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_SCIENCE_NOT_OPEN, '', 1)
        return
    end
    --升级的科技比自身科技等级低
    local science = info:findScience(groupId)
    if science ~= nil and science.level < level then
        print('p.CS_ALLIANCE_SCIENCE_DONATE...alliance science level not match...myLevel < level', science.level, level)
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    --资源或道具是否足够
    local itemId = donateTpl.donate[1]
    local count = donateTpl.donate[2]
    if doanteType == p.AllianceDonateType.DONATE_PROP then
        if not bag.checkItemEnough(itemId, count) then
            print('p.CS_ALLIANCE_SCIENCE_DONATE...prop is not enough...itemId, count', itemId, count)
            agent.sendNoticeMessage(p.ErrorCode.PUBLIC_PROP_NOT_ENOUGH, '', 1)
            return
        end
        bag.removeItem(itemId, count, p.ResourceConsumeType.ALLIANCE_DONATE)
    else
        if not user.isResourceEnough(itemId, count) then
            print('p.CS_ALLIANCE_SCIENCE_DONATE...resource is not enough')
            agent.sendNoticeMessage(p.ErrorCode.PUBLIC_RESOURCE_NOT_ENOUGH, '', 1)
            return
        end
        --捐献CD
        local cdTime = alliance.getDonateCdTime()
        if cdTime >= donate_cd_max then
            print('p.CS_ALLIANCE_SCIENCE_DONATE...donate is in cd time...cdTime, donateMaxTime', cdTime, donate_cd_max)
            agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_SCIENCE_DONATE_CD, '', 1)
            return
        end
        user.removeResource(itemId, count, p.ResourceConsumeType.ALLIANCE_DONATE)
        alliance.donateCdTime = cdTime + donate_cd + timer.getTimestampCache()
    end

    local scienceId = tplLevel.id
    local scienceExp = donateTpl.scienceExp or 0
    local activeCount = donateTpl.activeCount or 0
    local allianceScore = donateTpl.allianceScore or 0
    alliance.AddAllianceScore(allianceScore, p.ResourceGainType.ALLIANCE_DONATE)

    agent.queueJob(function()
        local ret = allianceStub.call_science_donate(info.id, user.uid, groupId, scienceExp, activeCount)
        --log
        logStub.appendAllianceScienceDonate(info.id, user.uid, scienceId, groupId, level, itemId, count, scienceExp, activeCount, allianceScore, timer.getTimestampCache())

        -- print('p.CS_ALLIANCE_SCIENCE_DONATE...ret', ret)
        agent.sendPktout(p.SC_ALLIANCE_SCIENCE_DONATE_RESPONSE, ret)
        if ret == p.ErrorCode.SUCCESS then
            --科技捐献物品
            if doanteType ~= p.AllianceDonateType.PROP then
                local itemList = alliance.scienceItemList[groupId]
                if itemList then
                    for k, v in pairs(itemList) do
                        if v.doanteType == doanteType then
                            table.remove(itemList, k)
                        end
                    end
                    alliance.getAllianceDonateItems(groupId)
                else
                    alliance.getAllianceDonateItems(groupId)
                end
                local updates ={}
                table.insert(updates, { groupId = groupId, itemList = alliance.scienceItemList[groupId] })
                alliance.sendAllianceDonateItemsUpdate(updates)
                alliance.evtAllianceDonate:trigger()
            end
        else
            agent.sendNoticeMessage(ret, '', 1)
        end
    end)
end

pktHandlers[p.CS_ALLIANCE_HELP_REQUEST] = function(pktin)
    local buildingId = pktin:read('i')
    --联盟是否存在
    local info = alliance.info
    if info == nil or info.id == 0 then
        print('p.CS_ALLIANCE_HELP_REQUEST...no join alliance...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    --是否是联盟成员
    local myself = info.memberList[user.uid]
    if myself == nil then
        print('p.CS_ALLIANCE_HELP_REQUEST... not alliance member..aid, uid', info.id, user.uid)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_MEMBER, '', 1)
        return
    end
    --是否可以申请帮助
    local buildingInfo = building.getBuildingById(buildingId)
    if buildingInfo == nil
        or buildingInfo.jobList == nil
        or buildingInfo.state == p.BuildingState.NORMAL
        or buildingInfo.state == p.BuildingState.DEMOLISHING
        or buildingInfo.state == p.BuildingState.FORGING
        or buildingInfo.state == p.BuildingState.TRAINING
        or buildingInfo.state == p.BuildingState.TRAIN_FINISH
        or buildingInfo.state == p.BuildingState.DAMAGED then
        print('p.CS_ALLIANCE_HELP_REQUEST...building help request is invalid...buildingId=', buildingId)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_HELP_REQUEST_INVALID, '', 1)
        return
    end
    local tplId, cdId, cdTime, remainTime = 0, 0, 0, 0
    for _, v in pairs(buildingInfo.jobList) do
        local cd = cdlist.getCD(v.cdId)
        if cd then
            local leftTime = cd:getRemainTime()
            if remainTime == 0 then
                remainTime = leftTime
            end
            if leftTime > 0 and leftTime <= remainTime then
                cdId = v.cdId
                tplId = v.flagId
                cdTime = cd.endTime
                remainTime = leftTime
            end
        end
    end
    local cds = {}
    if cdId > 0 then
        table.insert(cds, { tplId = tplId, cdId = cdId, cdTime = cdTime })
    end
    if #cds <=0 then
        print('p.CS_ALLIANCE_HELP_REQUEST...building no cd, help request is invalid...buildingId=', buildingId)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_HELP_REQUEST_INVALID, '', 1)
        return
    end
    --是否是重复申请帮助
    if alliance.requestHelpList[cdId] then
        print('p.CS_ALLIANCE_HELP_REQUEST...building help request repeat..buildingId=', buildingId)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_HELP_REQUEST_REPETE, '', 1)
        return
    end
    alliance.requestHelpList[cdId] = buildingId
    alliance.sendAllianceRequestHelp()

    local maxHelp = agent.property.list.helpMax
    local reduceTime = agent.property.list.helpReduceTime
    agent.queueJob(function()
        local ret = p.ErrorCode.SUCCESS
        for _, v in ipairs(cds) do
            local helpDesc = { tplId = buildingInfo.tplId ,state = buildingInfo.state, level = buildingInfo.level }
            if buildingInfo.state == p.BuildingState.RESEARCHING then
                helpDesc.tplId = v.tplId
                helpDesc.level = technology.getLevel(v.tplId)
            end
            local strHelpDesc = utils.serialize(helpDesc)
            local helpInfo = { uid = user.uid, buildingId = buildingId, cdId = v.cdId, cdTime = v.cdTime, max = maxHelp, reduceTime = reduceTime, helpDesc = strHelpDesc, }

            ret = allianceStub.call_help_request(info.id, user.uid, helpInfo)
            print('p.CS_ALLIANCE_HELP_REQUEST...ret', ret)
            break
        end
        if ret ~= p.ErrorCode.SUCCESS then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        agent.sendPktout(p.SC_ALLIANCE_HELP_REQUEST_RESPONSE, ret)
    end)
end

pktHandlers[p.CS_ALLIANCE_HELP_ONE] = function(pktin)
    local helpId = pktin:read('i')
    --联盟是否存在
    local info = alliance.info
    if info == nil or info.id == 0 then
        print('p.CS_ALLIANCE_HELP_ONE...no join alliance...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    --是否是联盟成员
    local myself = info.memberList[user.uid]
    if myself == nil then
        print('p.CS_ALLIANCE_HELP_ONE... not alliance member..aid, uid', info.id, user.uid)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_MEMBER, '', 1)
        return
    end
    agent.queueJob(function()
        local ret = allianceStub.call_help_one(info.id, user.uid, user.info.nickname, helpId)
        print('p.CS_ALLIANCE_HELP_ONE..ret', ret)
        if ret == p.ErrorCode.SUCCESS then
            --trigger
            alliance.evtAllianceHelp:trigger(1)
        else
            local deletes = {}
            table.insert(deletes, { helpId = helpId })
            alliance.sendAllianceHelpUpdate({}, deletes)
        end
    end)
end

pktHandlers[p.CS_ALLIANCE_HELP_ALL] = function(pktin)
    --联盟是否存在
    local info = alliance.info
    if info == nil or info.id == 0 then
        print('p.CS_ALLIANCE_HELP_ALL...no join alliance...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    --是否是联盟成员
    local myself = info.memberList[user.uid]
    if myself == nil then
        print('p.CS_ALLIANCE_HELP_ALL... not alliance member..aid, uid', info.id, user.uid)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_MEMBER, '', 1)
        return
    end
    local helpIds = {}
    for _, v in pairs(alliance.otherHelps) do
        if user.uid ~= v.uid then
            table.insert(helpIds, { helpId = v.helpId })
        end
    end
    if #helpIds <= 0 then
        return
    end
    agent.queueJob(function()
        local ret, count = allianceStub.call_help_all(info.id, user.uid, user.info.nickname, helpIds)
        print('p.CS_ALLIANCE_HELP_ALL..ret', ret)
        if ret == p.ErrorCode.SUCCESS then
            --trigger
            alliance.evtAllianceHelp:trigger(count)
        else
            alliance.sendAllianceHelpUpdate({}, helpIds)
        end
    end)
end

pktHandlers[p.CS_ALLIANCE_USER_HIRE_HERO] = function(pktin)
    local heroId = pktin:read('i')
    --联盟是否存在
    local info = alliance.info
    if info == nil or info.id == 0 then
        print('p.CS_ALLIANCE_USER_HIRE_HERO...no join alliance...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    --是否是联盟成员
    local myself = info.memberList[user.uid]
    if myself == nil then
        print('p.CS_ALLIANCE_USER_HIRE_HERO... not alliance member..aid, uid', info.id, user.uid)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_MEMBER, '', 1)
        return
    end
    --武将是否存在
    local heroInfo = hero.info[heroId]
    if heroInfo == nil then
        print('p.CS_ALLIANCE_USER_HIRE_HERO... hero is not exist..heroId', heroId)
        agent.sendNoticeMessage(p.ErrorCode.HERO_NOT_EXIST, '', 1)
        return
    end
    --是否已经出租
    if allianceStub.isHeroLeased(aid, user.uid, heroId) then
        print('p.CS_ALLIANCE_USER_HIRE_HERO...hero has been hired...aid, uid, heroId', aid, user.uid, heroId)
        agent.sendNoticeMessage(p.ErrorCode.HERO_HAD_HIRE, '', 1)
        return
    end
    --出租数量限制
    local count = allianceStub.getHeroLeaseCount(aid, user.uid)
    if count >= HERO_LEASE_MAX then
        print('p.CS_ALLIANCE_USER_HIRE_HERO...hero lease is max..aid, uid, count', aid, user.uid, count)
        agent.sendNoticeMessage(p.ErrorCode.HERO_HAD_HIRE, '', 1)
        return
    end

    local skill = heroInfo:getHeroSkill(false)
    local equip = heroInfo:getHeroEquip()
    local treasure = heroInfo:getHeroTreasure()
    local tempHero = { id = heroId, level = heroInfo.level, star = heroInfo.star, skill = skill, equip = equip, treasure = treasure }

    agent.queueJob(function()
        local ret = allianceStub.call_user_hire_hero(info.id, user.uid, user.info.nickname, tempHero)
        -- print('p.CS_ALLIANCE_USER_HIRE_HERO...ret', ret)
        if ret ~= p.ErrorCode.SUCCESS then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        agent.sendPktout(p.SC_ALLIANCE_USER_HIRE_HERO_RESPONSE, ret)
    end)
end

pktHandlers[p.CS_ALLIANCE_USER_RECALL_HERO] = function(pktin)
    local heroId = pktin:read('i')
    -- print('p.CS_ALLIANCE_USER_RECALL_HERO...heroId', heroId)
    --联盟是否存在
    local info = alliance.info
    if info == nil or info.id == 0 then
        print('p.CS_ALLIANCE_USER_RECALL_HERO...no join alliance...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    --是否是联盟成员
    local myself = info.memberList[user.uid]
    if myself == nil then
        print('p.CS_ALLIANCE_USER_RECALL_HERO... not alliance member..aid, uid', info.id, user.uid)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_MEMBER, '', 1)
        return
    end
    --武将是否存在
    local heroInfo = hero.info[heroId]
    if heroInfo == nil then
        print('p.CS_ALLIANCE_USER_RECALL_HERO... hero is not exist..heroId', heroId)
        agent.sendNoticeMessage(p.ErrorCode.HERO_NOT_EXIST, '', 1)
        return
    end
    --召回有时间限制
    local endTimestamp = allianceStub.getHeroLeaseBeginTime(aid, user.uid) + userRecalHeroInterval
    local now = timer.getTimestampCache()
    if now < endTimestamp then
        print('p.CS_ALLIANCE_USER_RECALL_HERO...recall time is less than, leftTime=', endTimestamp - now)
        agent.sendNoticeMessage(p.ErrorCode.RECAL_HERO_TIME_NOT_OVER, '', 1)
        return
    end

    agent.queueJob(function()
        local ret, collectSilver = allianceStub.call_user_recall_hero(info.id, user.uid, heroId)
        print('p.CS_ALLIANCE_USER_RECALL_HERO...ret, collectSilver', ret, collectSilver)
        if ret ~= p.ErrorCode.SUCCESS then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        agent.sendPktout(p.SC_ALLIANCE_USER_RECALL_HERO_RESPONSE, ret, collectSilver)
    end)
end

pktHandlers[p.CS_ALLIANCE_USER_EMPLOY_HERO] = function(pktin)
    local ownerUid, heroId = pktin:read('ii')
    --联盟是否存在
    local info = alliance.info
    if info == nil or info.id == 0 then
        print('p.CS_ALLIANCE_USER_EMPLOY_HERO...no join alliance...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    --是否是联盟成员
    local myself = info.memberList[user.uid]
    if myself == nil then
        print('p.CS_ALLIANCE_USER_EMPLOY_HERO... not alliance member..aid, uid', info.id, user.uid)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_MEMBER, '', 1)
        return
    end
    --
    if ownerUid == user.uid then
        print('p.CS_ALLIANCE_USER_EMPLOY_HERO... can not hire own hero..ownerUid, myUid', ownerUid, user.uid)
        agent.sendNoticeMessage(p.ErrorCode.NOT_HIRE_OWN_HERO, '', 1)
        return
    end
    --是否已经雇佣了该武将
    if alliance.employHeroList[ownerUid] and alliance.employHeroList[ownerUid][heroId] then
        print('p.CS_ALLIANCE_USER_EMPLOY_HERO... can not hire the same user hero ..ownerUid, heroId', ownerUid, heroId)
        agent.sendNoticeMessage(p.ErrorCode.HERO_EMPLOY_REPEATE, '', 1)
        return
    end

    --联盟是否有该武将出租 [aid][uid][heroId] = leaseInfo
    local allianceHeroLeases = allianceStub.allianceHeroLeases[info.id]
    if allianceHeroLeases == nil then
        print('p.CS_ALLIANCE_USER_EMPLOY_HERO...alliance no hero can employ...aid', info.id)
        agent.sendNoticeMessage(p.ErrorCode.HERO_NOT_EXIST, '', 1)
        return
    end
    if allianceHeroLeases[ownerUid] == nil then
        print('p.CS_ALLIANCE_USER_EMPLOY_HERO...member no hero can employ...uid', ownerUid)
        agent.sendNoticeMessage(p.ErrorCode.HERO_NOT_EXIST, '', 1)
        return
    end
    local leaseInfo = allianceHeroLeases[ownerUid][heroId]
    if leaseInfo == nil then
        print('p.CS_ALLIANCE_USER_EMPLOY_HERO...hero is not exist...heroId', heroId)
        agent.sendNoticeMessage(p.ErrorCode.HERO_NOT_EXIST, '', 1)
        return
    end
    local rentSilver = leaseInfo.rentSilver or -1
    --银两是否足够
    if not user.isResourceEnough(p.SpecialPropIdType.SILVER, rentSilver) then
        print('p.CS_ALLIANCE_USER_EMPLOY_HERO...silver is not enough...rentSilver', rentSilver)
        agent.sendNoticeMessage(p.ErrorCode.PUBLIC_SILVER_NOT_ENOUGH, '', 1)
        return
    end
    --雇佣条件
    if not alliance.isEmployHeroOpen() then
        print('p.CS_ALLIANCE_USER_EMPLOY_HERO...employ conditions are not satisfied...')
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    agent.queueJob(function()
        local ret = allianceStub.call_user_employ_hero(info.id, user.uid, ownerUid, heroId)
        -- print('p.CS_ALLIANCE_USER_EMPLOY_HERO...ret', ret)
        if ret ~= p.ErrorCode.SUCCESS then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        --消耗
        user.removeResource(p.SpecialPropIdType.SILVER, rentSilver, p.ResourceConsumeType.ALLIANCE_HIRE_HERO)

        agent.sendPktout(p.SC_ALLIANCE_USER_EMPLOY_HERO_RESPONSE, ret)
    end)
end

pktHandlers[p.CS_ALLIANCE_COLLECT_HERO_HIRE_COST] = function(pktin)
    -- print('p.CS_ALLIANCE_COLLECT_HERO_HIRE_COST...')
    --联盟是否存在
    local info = alliance.info
    if info == nil or info.id == 0 then
        print('p.CS_ALLIANCE_COLLECT_HERO_HIRE_COST...no join alliance...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    --是否是联盟成员
    local myself = info.memberList[user.uid]
    if myself == nil then
        print('p.CS_ALLIANCE_COLLECT_HERO_HIRE_COST... not alliance member..aid, uid', info.id, user.uid)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_MEMBER, '', 1)
        return
    end
    agent.queueJob(function()
        local ret, collectSilver = allianceStub.call_collect_hero_hire_cost(info.id, user.uid)
        print('p.CS_ALLIANCE_COLLECT_HERO_HIRE_COST...ret, collectSilver', ret, collectSilver)
        if ret ~= p.ErrorCode.SUCCESS then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        agent.sendPktout(p.SC_ALLIANCE_COLLECT_HERO_HIRE_COST_RESPONSE, ret)
    end)
end

pktHandlers[p.CS_ALLIANCE_BUFF_OPEN] = function(pktin)
    local buffId = pktin:read('i')
    -- print('p.CS_ALLIANCE_BUFF_OPEN...buffId', buffId)
    --联盟是否存在
    local info = alliance.info
    if info == nil or info.id == 0 then
        print('p.CS_ALLIANCE_BUFF_OPEN...no join alliance...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    --是否是联盟成员
    local myself = info.memberList[user.uid]
    if myself == nil then
        print('p.CS_ALLIANCE_BUFF_OPEN... not alliance member..aid, uid', info.id, user.uid)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_MEMBER, '', 1)
        return
    end
    --是否有权限
    if not myself:haveRankPermit(p.AllianceRankPermitType.BUFF) then
        print('p.CS_ALLIANCE_BUFF_OPEN... no permit')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_WITHOUT_PERMIT, '', 1)
        return
    end
    --buff是否存在
    local tpl = t.allianceBuff[buffId]
    if tpl == nil then
        print('p.CS_ALLIANCE_BUFF_OPEN... tpl_alliance_buff no this buff...buffId', buffId)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_BUFF_NOT_EXIST, '', 1)
        return
    end
    --是否可以激活buff
    if info.level < tpl.allianceLevel then
        print('p.CS_ALLIANCE_BUFF_OPEN... can not open then buff...buffId, allianceLevel, needAllianceLevel', buffId, info.level, tpl.allianceLevel)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_BUFF_CANNOT_OPEN, '', 1)
        return
    end
    --是否还有其他已经激活的buff
    if not allianceStub.isCanOpenAllianceBuff(info.id) then
        print('p.CS_ALLIANCE_BUFF_OPEN... can not open multi alliance buff..')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_BUFF_CANNOT_OPENMULTI, '', 1)
        return
    end
    agent.queueJob(function()
        local ret = allianceStub.call_alliance_buff_open(info.id, user.uid, buffId)
        print('p.CS_ALLIANCE_BUFF_OPEN...ret', ret)
        if ret ~= p.ErrorCode.SUCCESS then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        agent.sendPktout(p.SC_ALLIANCE_BUFF_OPEN_RESPONSE, ret)
    end)
end

pktHandlers[p.CS_ALLIANCE_BUFF_CLOSED] = function(pktin)
    local buffId = pktin:read('i')
    --联盟是否存在
    local info = alliance.info
    if info == nil or info.id == 0 then
        print('p.CS_ALLIANCE_BUFF_CLOSED...no join alliance...')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_EXIST, '', 1)
        return
    end
    --是否是联盟成员
    local myself = info.memberList[user.uid]
    if myself == nil then
        print('p.CS_ALLIANCE_BUFF_CLOSED... not alliance member..aid, uid', info.id, user.uid)
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_NOT_MEMBER, '', 1)
        return
    end
    --是否有权限
    if not myself:haveRankPermit(p.AllianceRankPermitType.BUFF) then
        print('p.CS_ALLIANCE_BUFF_CLOSED... no permit')
        agent.sendNoticeMessage(p.ErrorCode.ALLIANCE_WITHOUT_PERMIT, '', 1)
        return
    end
    agent.queueJob(function()
        local ret = allianceStub.call_alliance_buff_closed(info.id, user.uid, buffId)
        -- print('p.CS_ALLIANCE_BUFF_OPEN...ret', ret)
        if ret ~= p.ErrorCode.SUCCESS then
            agent.sendNoticeMessage(ret, '', 1)
            return
        end
        agent.sendPktout(p.SC_ALLIANCE_BUFF_CLOSED_RESPONSE, ret)
    end)
end

pktHandlers[p.CS_GET_ALLIANCE_CITIES] = function(pktin)
    mapService:forward(pktin)
end

return alliance