local M = {}
local p = require('protocol')
local t = require('tploader')
local cluster = require('cluster')
local dbo = require('dbo')
local allianceInfo = require('alliance')
local timer = require('timer')
local trie = require('trie')
local utils = require('utils')
local framework = require('framework')
local misc = require('libs/misc')
local pubMail = require('mail')
local pubHero = require('hero')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()

local idService = require('idService')
local loginService = require('loginService')
local dataService = require('dataService')
local rangeService = require('rangeService')
local arenaService = require('arenaService')
local achievementService = require('achievementService')
local mapStub

local rawService
local impl = {}

local alliance_hero_lease = t.configure['alliance_hero_lease']
local alliance_hero_hook = t.configure['alliance_hero_hook']
local helpActiveAdd = t.configure['helpActiveAdd']
local allianceActiveReword = t.configure['allianceActiveReword']
local userRecalHeroInterval = t.configure['userRecalHeroInterval'] or 0
-- local allianceScienceDonate = t.configure['alliance_donate_cont'] or 1000
--联盟CD
local allianceCdInterval = 24 * 60 * 60
--换联盟旗帜CD
local changeBannerCdInterval = t.configure['change_banner_cd'] or 24 * 60 * 60
--盟主替换CD
local replaceInterval = 7 * 24 * 60 * 60

--所有联盟信息
local alliances = {} --[aid]=allianceInfo
--所有联盟拥有的名城
local allAllianceCity = {} --[aid]={cityId, ...}
--所有联盟失效的名城
local uselessBuffCity = {} --[aid]={cityId, ...}
--联盟邀请列表
local ALLIANCE_INVITE_MAX = 20  --list保存上限
local allianceInviteList = {}  --[uid] = {isDirty, createTime, list={aid,....}}
--联盟帮助
local allianceHelps = {} --[aid][helpId] = helpInfo
local allianceHelpGlobalId = 0
--[[
helpInfo = {
    uid=i,              --申请帮助的玩家UID
    helpId=i,           --帮助ID
    buildingId=i,       --申请帮助的建筑ID
    times=i,            --被帮助次数
    accept=i,           --玩家已处理帮助次数
    max=i,              --总共可被帮助次数
    reduceTime=i,       --减少时间
    cdId = i,           --申请帮助的建筑CD ID
    cdTime=i,           --申请帮助的建筑CD结束时间
    helpUserList[helperUid]=1,  --提供帮助的玩家
    helpDesc={...},     --帮助详细信息
    isDirty,
}
--]]
--联盟武将租借
local HERO_LEASE_MAX = 1            --每个人出租武将数量上限
local allianceHeroLeases = {}       --[aid][uid] = {isDirty=false, incomeCount=0, list={[heroId]=leaseInfo,..}}
--[[
leaseInfo = {
    heroInfo = nil,     --武将信息
    incomeRentSilver = 0,     --出租银两收益
    incomeHookSilver = 0,     --挂机银两收益
    rentSilver = 0,     --出租价格
    canRecall = false,  --是否可以召回
    leaseBeginTime = 0, --武将出租开始时间
    hookBeginTime = 0,  --武将挂机时间(领了收益就重置)
    rentCount = 0,
}
--]]
local HERO_LEASE_RECORD_MAX = 10    --出租记录数量
local allianceHeroleaseRecords = {}   --租借记录[uid] = {leaseRecordInfo, ...}
--[[
local leaseRecordInfo = {
    employId = 0,
    heroId = 0,
    nickname = '',
    rentSilver = 0,
    createTime = 0,
}
--]]

--联盟日记
local ALLIANCE_RECORD_MAX = 20         -- 联盟记录数量上限
local allianceRecords = {}            -- 联盟日记记录[aid] = {recordInfo, ...}
--[[
local recordInfo = {
    id = 0,
    type = 0,
    content = '',
    param1 = '',
    createTime = 0,
}
--]]
local allianceHeroleaseRecords = {}   --


--定点刷新数据
local allianceHeroLeaseData = {
    last5RefreshTime = 0,--上次刷新时间
    isDirty = false,
}

local rankPermitType = p.AllianceRankPermitType

local timerUpdate
local dbSaveAlliances

-- framework node shutdown
framework.beforeShutdown(function(cb)
    jobExecutor:queue(function()
        print('framework.beforeShutdown alliance')
        if M.timer then
            M.timer:cancel()
            M.timer = nil
        end
        dbSaveAlliances()
        cb()
    end, cb)
end)

function M.queueJob(threadFun, errorFun)
    jobExecutor:queue(threadFun, errorFun)
end

local function genAllianceHelpId()
    if allianceHelpGlobalId >= 999999999 then
        allianceHelpGlobalId = 1
    else
        allianceHelpGlobalId = allianceHelpGlobalId + 1
    end
    return allianceHelpGlobalId
end

local function alliance2Net(alliance)
    -- print('alliance2Net....', utils.serialize(alliance))
    local list = {}
    list.id = alliance.id
    list.name = alliance.name
    list.nickname = alliance.nickname
    list.level = alliance.level
    list.exp = alliance.exp
    list.bannerId = alliance.bannerId
    list.power = alliance.power
    list.leaderId = alliance.leaderId
    list.leaderName = alliance.leaderName
    list.leaderHeadId = alliance.leaderHeadId

    list.alliesCount = alliance.alliesCount
    list.alliesMax = alliance.alliesMax
    list.towerCount = alliance.towerCount
    list.towerMax = alliance.towerMax
    list.castleCount = alliance.castleCount
    list.castleMax = alliance.castleMax

    list.toatlActive = alliance.toatlActive
    list.announcement = alliance.announcement
    list.openRecruitment = alliance.openRecruitment
    list.createTime = alliance.createTime
    list.disbandTime = alliance.disbandTime
    list.changeBannerCdTime = alliance.changeBannerCdTime

    return list
end

local function alliance2SimpleNet(alliance)
    local list = {}
    list.id = alliance.id
    list.name = alliance.name
    list.nickname = alliance.nickname
    list.level = alliance.level
    list.bannerId = alliance.bannerId
    list.toatlActive = alliance.toatlActive

    list.leaderId = alliance.leaderId
    list.leaderName = alliance.leaderName
    list.leaderHeadId = alliance.leaderHeadId

    list.towerMax = alliance.towerMax
    list.castleMax = alliance.castleMax
    return list
end

local function member2Net(member)
    local m = {
        uid = member.uid,
        nickname = member.nickname,
        headId = member.headId,
        userLevel = member.userLevel,
        vipLevel = member.vipLevel,
        rankLevel = member.rankLevel,
        activeWeek = member.activeWeek,
        contribution = member.contribution,
        sendMailCount = member.sendMailCount,
        totalPower = member.totalPower,
        castleLevel = member.castleLevel,
        x = member.x,
        y = member.y,
        marketIsOpen = member.marketIsOpen,
        lastOnlineTime = member.lastOnlineTime,
        joinTime = member.joinTime,
    }
    return m
end

local function memberList2Net(memberList)
    -- print('memberList2Net....', utils.serialize(memberList))
    local list = {}
    for _, v in pairs(memberList) do
        table.insert(list, member2Net(v))
    end
    return list
end

local function member2SimpleNet(member)
    local m = { uid = member.uid }
    return m
end

local function memberList2SimpleNet(memberList)
    local list = {}
    for _, m in pairs(memberList) do
        table.insert(list, member2SimpleNet(m))
    end
    return list
end

local function applyList2Net(applyList)
    -- print('applyList2Net....', utils.serialize(applyList))
    local list = {}
    for _, v in pairs(applyList) do
        table.insert(list, {
            uid = v.uid,
            nickname = v.nickname,
            headId = v.headId,
            userLevel = v.userLevel,
            vipLevel = v.vipLevel,
            time = v.time,
            })
    end
    return list
end

local function inviteList2Net(inviteList)
    -- print('inviteList2Net....', utils.serialize(inviteList))
    local list = {}
    for _, v in pairs(inviteList) do
        table.insert(list, {
            uid = v.uid,
            nickname = v.nickname,
            headId = v.headId,
            inviteUid = v.inviteUid,
            time = v.time,
            })
    end
    return list
end

local function science2Net(science)
    local s = {
        groupId = science.groupId,
        tplId = science.tplId,
        level = science.level,
        exp = science.exp,
    }

    return s
end

local function scienceList2Net(scienceList)
    local list = {}
    for _, v in pairs(scienceList) do
        table.insert(list, science2Net(v))
    end
    return list
end

local function simpleScience2Net(science)
    local s = {
        groupId = science.groupId,
        tplId = science.tplId,
        level = science.level,
    }

    return s
end

local function simpleScienceList2Net(scienceList)
    local list = {}
    for _, v in pairs(scienceList) do
        table.insert(list, simpleScience2Net(v))
    end
    return list
end

local function allianceBuff2Net(allianceBuff)
    local buff = {
        buffId = allianceBuff.buffId,
        type = allianceBuff.type,
        useCount = allianceBuff.useCount,
        createTimestamp = allianceBuff.createTimestamp,
        endTimestamp = allianceBuff.endTimestamp,
    }

    return buff
end

local function allianceBuffList2net(allianceBuffList)
    local list = {}
    for _, v in pairs(allianceBuffList) do
        table.insert(list, allianceBuff2Net(v))
    end
    return list
end

local function allianceBuff2SimpleNet(allianceBuff)
    local buff = {
        buffId = allianceBuff.buffId,
        type = allianceBuff.type,
        attr = allianceBuff.tpl.attrList,
        param1 = allianceBuff.tpl.addOther,
        endTimestamp = allianceBuff.endTimestamp,
    }

    return buff
end

local function heroInfo2Net(heroInfo)
    local skill = heroInfo:getHeroSkill()
    local equip = heroInfo:getHeroEquip()
    local treasure = heroInfo:getHeroTreasure()
    local additionList = heroInfo:getHeroAdditionList()
    local info = {
        id = heroInfo.id,
        level = heroInfo.level,
        star = heroInfo.star,
        skill = skill,
        equip = equip,
        treasure = treasure,
        additionList = additionList,
    }
    return info
end

local function leaseInfo2Net(leaseInfo)
    local heroInfo = heroInfo2Net(leaseInfo.heroInfo)
    local info = {
        incomeRentSilver = leaseInfo.incomeRentSilver,
        incomeHookSilver = leaseInfo.incomeHookSilver,
        rentSilver = leaseInfo.rentSilver,
        canRecall = leaseInfo.canRecall,
        leaseBeginTime = leaseInfo.leaseBeginTime,
        hookBeginTime = leaseInfo.hookBeginTime,
        rentCount = leaseInfo.rentCount,
        todayRentCount = leaseInfo.todayRentCount,
        newRentCount = leaseInfo.newRentCount,
        heroInfo = heroInfo,
    }
    return info
end

local function isAllianceExist(aid)
    if alliances[aid] and alliances[aid].disbandTime == 0 then
        return true
    end
    return false
end

local function isJoinAlliance(uid)
    for _, v in pairs(alliances) do
        if v.disbandTime == 0 then
            if v.memberList[uid] then
                return true
            end
        end
    end
    return false
end

local function isRepeatName(name)
    for _, v in pairs(alliances) do
        if v.disbandTime == 0 then
            if name == v.name then
                return true
            end
        end
    end
    return false
end

local function isValidName(name)
    local valid = true
    local nameLength = string.len(name)
    --长度
    if nameLength < 1 or nameLength > 24 then
        valid = false
    end
    -- ^[a-zA-Z0-9\u4e00-\u9fa5]+$
    for i=1, nameLength do
        local byte = string.byte(name, i)
        if not ((byte >= 48 and byte <= 57)  -- number
            or (byte >= 65 and byte <= 90) -- A ~ Z
            or (byte >= 97 and byte <= 122) -- a ~ z
            or (byte >127) -- 中文
            ) then
            valid = false
            break
        end
    end
    --脏字
    if trie.isContain(name) then
        valid = false
    end
    return valid
end

local function allianceDataSyncToService(uid, aid, allianceName, allianceNickname, bannerId)
    arenaService.onArenaAllianceDataChange(uid, aid, allianceName, allianceNickname, bannerId)
end

--
-- db
--
local function dbLoadUserInfo(uid)
    local info = {}
    local db = dbo.open(0)
    local rs = db:executePrepare('SELECT level, nickname, headId, lastLoginTimestamp, totalPower, castleLevel, x, y from s_user where uid=?', uid)
    if rs.ok then
        for _, row in ipairs(rs) do
            info.nickname = row.nickname
            info.headId = row.headId
            info.userLevel = row.level
            info.lastLoginTimestamp = row.lastLoginTimestamp
            info.totalPower = row.totalPower
            info.castleLevel = row.castleLevel
            info.x = row.x
            info.y = row.y
            break
        end
    end

    local rs = db:executePrepare('SELECT v from s_dict where uid = ? and k = ?', uid, 'vip.info')
    if rs.ok then
        for _, row in ipairs(rs) do
            local data = misc.deserialize(row.v)
            info.vipLevel = data.level
            break
        end
    end
    info.marketIsOpen = false
    local rs = db:executePrepare('SELECT v from s_dict where uid = ? and k = ?', uid, 'building.list')
    if rs.ok then
        for _, row in ipairs(rs) do
            local buildingList = misc.deserialize(row.v)
            for _, v in pairs(buildingList) do
                if v.gridId == p.BuildingType.MARKET and v.level > 0 then
                    info.marketIsOpen = true
                end
            end 
        end
    end
    return info
end

local function dbLoadAllianceInvite(db)
    local rs = db:executePrepare('SELECT uid, inviteList, createTime from s_alliance_invite')
    if rs.ok then
        for _, row in ipairs(rs) do
            if row.inviteList == '' then
                row.inviteList = '{}'
            end
            allianceInviteList[row.uid] = {}
            allianceInviteList[row.uid].list = misc.deserialize(row.inviteList)
            allianceInviteList[row.uid].createTime = row.createTime
            -- print('dbLoadAllianceInvite...uid, allianceInviteList', row.uid, utils.serialize(allianceInviteList[row.uid]))
        end
    end
end

local function dbSaveAllianceInvite(db)
    local list = {}
    for uid, v in pairs(allianceInviteList) do
        if v.isDirty then
            local inviteList = utils.serialize(v.list)
            table.insert(list, { uid = uid, inviteList = inviteList, createTime = v.createTime })
        end
    end
    -- batch save
    while #list >= 10 do
        -- print('dbSaveAllianceInvite..batch save', #list)
        local saveList = {}
        for i=1, 10 do
            local data = table.remove(list, 1)
            table.insert(saveList, data)
        end
        local rs = db:insertBatch('s_alliance_invite', saveList, { 'inviteList', 'createTime' })
        if rs.ok and rs.affectedRows > 0 then
            for _, v in pairs(saveList) do
                if allianceInviteList[v.uid] then
                    allianceInviteList[v.uid].isDirty = false
                end
            end
        elseif not rs.ok then
           utils.log('dbSaveAllianceMembers batch save s_alliance_invite error' .. utils.toJson(rs))
        end
    end

    --single save
    while #list > 0 do
        -- print('dbSaveAllianceInvite..single save', #list)
        local saveList = {}
        local data = table.remove(list, 1)
        table.insert(saveList, data)
        local rs = db:insertBatch('s_alliance_invite', saveList, { 'inviteList', 'createTime' })
        if rs.ok and rs.affectedRows > 0 then
            if allianceInviteList[data.uid] then
                allianceInviteList[data.uid].isDirty = false
            end
        elseif not rs.ok then
            -- error
            utils.log('dbSaveAllianceMembers single save s_alliance_invite error' .. utils.toJson(rs) .. ',  ' .. utils.toJson(data))
        end
    end
end

local function dbDeleteAllianceInvite(uid)
    local db = dbo.open(0)
    local rs = db:executePrepare('DELETE from s_alliance_invite where uid = ?', uid)
    if not rs.ok then
        utils.log('dbDeleteAllianceInvite error' .. utils.toJson(rs))
    end
end

local function dbDeleteAllianceMember(db, aid, uid)
    local rs = db:executePrepare('DELETE from s_alliance_member where uid = ? and aid = ?', uid, aid)
    if not rs.ok then
        utils.log('dbDeleteAllianceMember error' .. utils.toJson(rs))
    end
end

local function dbSaveAllianceCdTimestamp(db, uid, allianceCdTimestamp)
    -- print('dbSaveAllianceCdTimestamp...uid, allianceCdTimestamp', uid, allianceCdTimestamp)
    local k = 'alliance.cd'
    local v = utils.serialize({ allianceCdTime = allianceCdTimestamp, })
    local rs = db:executePrepare('insert into s_dict(uid, k, v) values (?, ?, ?) on duplicate key update v=values(v)', uid, k, v)
    if not rs.ok then
        utils.log('dbSaveAllianceCdTimestamp error' .. utils.toJson(rs))
    end
end

local function dbLoadAllianceMembers(db, aid)
    local rs = db:executePrepare('SELECT a.uid, u.nickname, u.headId, u.level, a.userLevel, a.vipLevel, a.rankLevel, a.activeWeek, a.contribution, a.sendMailCount, a.marketIsOpen, u.totalPower, u.castleLevel, u.x, u.y, a.lastOnlineTime, a.joinTime from s_alliance_member as a left join s_user as u on (a.uid = u.uid) where a.aid = ?', aid)
    if rs.ok then
        for _, row in ipairs(rs) do
            if row.userLevel ~= row.level then
                row.userLevel = row.level
            end
            local m = allianceInfo.member:new(row)
            alliances[aid]:addMember(m)
            if m.rankLevel == 1 then
                -- update leader info
                alliances[aid].leaderId = m.uid
                alliances[aid].leaderName = m.nickname
                alliances[aid].leaderHeadId = m.headId
            end
        end
    end
end

local function dbSaveAllianceMembers(db, aid, memberList)
    -- print('dbSaveAllianceMembers...aid', aid)
    local list = {}
    for _, v in pairs(memberList) do
        if v.isDirty then
            local lastOnlineTime = v.lastOnlineTime
            if lastOnlineTime == 0 then
                lastOnlineTime = timer.getTimestampCache()
            end
            table.insert(list, {
                uid = v.uid,
                aid = aid,
                userLevel = v.userLevel,
                vipLevel = v.vipLevel,
                rankLevel = v.rankLevel,
                activeWeek = v.activeWeek,
                contribution = v.contribution,
                sendMailCount = v.sendMailCount,
                marketIsOpen = v.marketIsOpen,
                lastOnlineTime = lastOnlineTime,
                joinTime = v.joinTime,
                })
        end
    end
    -- batch save
    while #list >= 10 do
        -- print('dbSaveAllianceMembers..batch save', #list)
        local saveList = {}
        for i=1, 10 do
            local data = table.remove(list, 1)
            table.insert(saveList, data)
        end
        local rs = db:insertBatch('s_alliance_member', saveList, { 'aid', 'userLevel', 'vipLevel', 'rankLevel', 'activeWeek', 'contribution', 'sendMailCount', 'marketIsOpen', 'lastOnlineTime', 'joinTime' })
        if rs.ok and rs.affectedRows > 0 then
            for _, v in pairs(saveList) do
                for _, m in pairs(memberList) do
                    if v.uid == m.uid then
                        m.isDirty = false
                        break
                    end
                end
            end
        elseif not rs.ok then
           utils.log('dbSaveAllianceMembers batch save s_alliance_member error' .. utils.toJson(rs))
        end
    end

    --single save
    while #list > 0 do
        -- print('dbSaveAllianceMembers..single save', #list)
        local saveList = {}
        local data = table.remove(list, 1)
        table.insert(saveList, data)
        local rs = db:insertBatch('s_alliance_member', saveList, { 'aid', 'userLevel', 'vipLevel', 'rankLevel', 'activeWeek', 'contribution', 'sendMailCount', 'marketIsOpen', 'lastOnlineTime', 'joinTime' })
        if rs.ok and rs.affectedRows > 0 then
            for _, m in pairs(memberList) do
                if m.uid == data.uid then
                    m.isDirty = false
                    break
                end
            end
        elseif not rs.ok then
            -- error
            utils.log('dbSaveAllianceMembers single save s_alliance_member error' .. utils.toJson(rs) .. ',  ' .. utils.toJson(data))
        end
    end
end

local function dbAddAlliance(alliance)
    -- print('dbAddAlliance....', utils.toJson(alliance))
    local a = alliance
    local id = idService.createAid()
    local db = dbo.open(0)
    local rs = db:executePrepare('INSERT INTO s_alliance (id, name, nickname, level, exp, bannerId, power, leaderId, leaderName, leaderHeadId, alliesCount, towerCount, castleCount, toatlActive, announcement, openRecruitment, applyList, inviteList, scienceList, allianceBuffList, allianceLoseBuffList, createTime, disbandTime, changeBannerCdTime) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)',
    id, a.name, a.nickname, a.level, a.exp, a.bannerId, a.power, a.leaderId, a.leaderName, a.leaderHeadId, a.alliesCount, a.towerCount, a.castleCount, a.toatlActive, a.announcement, a.openRecruitment, '{}', '{}', '{}', '{}', '{}', a.createTime, 0, 0)
    if not rs.ok then
        utils.log('dbAddAlliance error' .. utils.toJson(rs))
    end
    return id
end

local function dbLoadAllianceHelps(db, aid)
    local rs = db:executePrepare('SELECT * from s_alliance_help where aid = ?', aid)
    if rs.ok then
        local helps = allianceHelps[aid]
        if helps == nil then
            helps = {}
            allianceHelps[aid] = helps
        end
        for _, row in ipairs(rs) do
            local help = {}
            help.uid = row.uid
            help.helpId = row.id
            help.buildingId = row.buildingId
            help.times = row.times
            help.accept = row.accept
            help.max = row.max
            help.reduceTime = row.reduceTime
            help.cdId = row.cdId
            help.cdTime = row.cdTime
            help.helpUserList = misc.deserialize(row.helpUserList)
            help.helpDesc = row.helpDesc
            help.isDirty = false
            helps[help.helpId] = help

            -- init allianceHelpGlobalId
            if help.helpId > allianceHelpGlobalId then
                allianceHelpGlobalId = help.helpId
            end
        end
    end
end

local function dbSaveAllianceHelps(db, aid)
    -- print('dbSaveAllianceHelps...aid', aid)
    local helpList = allianceHelps[aid]
    if helpList == nil then
        return
    end
    local list = {}
    for _, v in pairs(helpList) do
        if v.isDirty then
            local helpUserList = utils.serialize(v.helpUserList)
            table.insert(list, {
                id = v.helpId,
                aid = aid,
                uid = v.uid,
                buildingId = v.buildingId,
                times = v.times,
                accept = v.accept,
                max = v.max,
                reduceTime = v.reduceTime,
                cdId = v.cdId,
                cdTime = v.cdTime,
                helpUserList = helpUserList,
                helpDesc = v.helpDesc,
                })
        end
    end

    --batch save
    while #list >= 10 do
        -- print('dbSaveAllianceHelps..batch save', #list)
        local saveList = {}
        for i=1, 10 do
            local data = table.remove(list, 1)
            table.insert(saveList, data)
        end
        local rs = db:insertBatch('s_alliance_help', saveList, { 'aid', 'uid', 'buildingId', 'times', 'accept', 'max', 'reduceTime', 'cdId', 'cdTime', 'helpUserList', 'helpDesc' })
        if rs.ok and rs.affectedRows > 0 then
            for _, v in pairs(saveList) do
                for _, help in pairs(helpList) do
                    if v.id == help.helpId then
                        help.isDirty = false
                        break
                    end
                end
            end
        end
    end
    --single save
    while #list > 0 do
        -- print('dbSaveAllianceHelps..single save', #list)
        local saveList = {}
        local data = table.remove(list, 1)
        table.insert(saveList, data)
        local rs = db:insertBatch('s_alliance_help', saveList, { 'aid', 'uid', 'buildingId', 'times', 'accept', 'max', 'reduceTime', 'cdId', 'cdTime', 'helpUserList', 'helpDesc' })
        if rs.ok and rs.affectedRows > 0 then
            for _, v in pairs(helpList) do
                if v.helpId == data.id then
                    v.isDirty = false
                    break
                end
            end
        end
    end

    -- delete help
    local nowTime = timer.getTimestampCache()
    local rs = db:executePrepare('DELETE FROM s_alliance_help where aid = ? and cdTIme < ?', aid, nowTime)
    if not rs.ok then
        -- error
    end
    for _, v in pairs(helpList) do
        if not v.isDirty and (v.cdTime == 0 or v.cdTime == nil) then
            helpList[v.helpId] = nil
        end
    end
end

local function dbLoadAllianceHeroLeases(db, aid)
    local rs = db:executePrepare('SELECT * from s_alliance_hero_lease where aid = ?', aid)
    if rs.ok then
        local alease = allianceHeroLeases[aid]
        if alease == nil then
            alease = {}
            allianceHeroLeases[aid] = alease
        end
        for _, row in ipairs(rs) do
            if alease[row.uid] == nil then
                alease[row.uid] = {}
                alease[row.uid].list = {}
            end
            row.leaseList = misc.deserialize(row.leaseList)
            local leaseList = {}
            for _, v in pairs(row.leaseList) do
                local leaseInfo = {}
                leaseInfo.incomeRentSilver = v.incomeRentSilver
                leaseInfo.incomeHookSilver = v.incomeHookSilver
                leaseInfo.rentSilver = v.rentSilver
                leaseInfo.canRecall = v.canRecall
                leaseInfo.leaseBeginTime = v.leaseBeginTime
                leaseInfo.hookBeginTime = v.hookBeginTime
                leaseInfo.rentCount = v.rentCount or 0
                leaseInfo.todayRentCount = v.todayRentCount or 0
                leaseInfo.newRentCount = v.newRentCount or 0
                local info = pubHero.newHeroInfo(v.heroInfo)
                leaseInfo.heroInfo = info
                leaseList[info.id] = leaseInfo
            end
            alease[row.uid].incomeCount = row.incomeCount
            alease[row.uid].isDirty = false
            alease[row.uid].list = leaseList
        end
        -- print('dbLoadAllianceHeroLeases..., aid, allianceHeroLeases', aid, utils.serialize(allianceHeroLeases))
    end
    --
end

local function dbSaveAllianceHeroLeases(db, aid)
    -- print('dbSaveAllianceHeroLeases...aid', aid)
    local alease = allianceHeroLeases[aid]
    if alease == nil then
        return
    end
    local list = {}
    for uid, ulease in pairs(alease) do
        -- print('dbSaveAllianceHeroLeases...ulease.isDirty', ulease.isDirty)
        if ulease.isDirty and ulease.list then
            local leaseList = {}
            for _, leaseInfo in pairs(ulease.list) do
                local info = leaseInfo2Net(leaseInfo)
                table.insert(leaseList, info)
            end
            leaseList = utils.serialize(leaseList)
            local incomeCount = ulease.incomeCount or 0
            table.insert(list, { uid = uid, aid = aid, incomeCount = incomeCount, leaseList = leaseList })
        end
    end

    --batch save
    while #list >= 10 do
        -- print('dbSaveAllianceHeroLeases..batch save', #list)
        local saveList = {}
        for i=1, 10 do
            local data = table.remove(list, 1)
            table.insert(saveList, data)
        end
        local rs = db:insertBatch('s_alliance_hero_lease', saveList, { 'aid', 'incomeCount', 'leaseList' })
        if rs.ok and rs.affectedRows > 0 then
            for _, v in pairs(saveList) do
                for uid, lease in pairs(alease) do
                    if v.uid == uid then
                        lease.isDirty = false
                        break
                    end
                end
            end
        end
    end
    --single save
    while #list > 0 do
        -- print('dbSaveAllianceHeroLeases..single save', #list)
        local saveList = {}
        local data = table.remove(list, 1)
        table.insert(saveList, data)
        local rs = db:insertBatch('s_alliance_hero_lease', saveList, { 'aid', 'incomeCount', 'leaseList' })
        if rs.ok and rs.affectedRows > 0 then
            for _, v in pairs(alease) do
                if v.uid == data.uid then
                    v.isDirty = false
                    break
                end
            end
        end
    end
end

local function dbdeleteAllianceHeroLeases(db, uid)
    -- print('dbdeleteAllianceHeroLeases...uid', uid)
    local rs = db:executePrepare('DELETE from s_alliance_hero_lease where uid = ?', uid)
    if not rs.ok then
        -- error
        utils.log('dbdeleteAllianceHeroLeases error' .. utils.toJson(rs))
    end
end

---
local function dbLoadAlliances()
    local db = dbo.open(0)
    local rs = db:executePrepare('SELECT * from s_alliance where disbandTime = 0')
    if rs.ok then
        for _, row in ipairs(rs) do
            local info = allianceInfo.info:new()
            info.id = row.id
            info.name = row.name
            info.nickname = row.nickname
            info.level = row.level
            info.exp = row.exp
            info.bannerId = row.bannerId
            -- 战斗力都是实时计算的，不需要从数据库中读取，进程一启动就把联盟中所有成员的战斗力加起来
            info.power = 0
            info.leaderId = row.leaderId
            info.leaderName = row.leaderName
            info.leaderHeadId = row.leaderHeadId

            info.alliesCount = row.alliesCount
            info.alliesMax = t.allianceLevel[row.level].alliesMax
            info.towerCount = row.towerCount
            info.castleMax = t.allianceLevel[row.level].towerMax
            info.castleCount = row.castleCount
            info.castleMax = t.allianceLevel[row.level].castleMax

            info.toatlActive = row.toatlActive
            info.announcement = row.announcement
            info.openRecruitment = row.openRecruitment
            info.createTime = row.createTime
            info.disbandTime = row.disbandTime
            info.changeBannerCdTime = row.changeBannerCdTime

            if row.applyList == '' then
                row.applyList = '{}'
            end
            if row.inviteList == '' then
                row.inviteList = '{}'
            end
            if row.scienceList == '' then
                row.scienceList = '{}'
            end
            if row.allianceBuffList == '' then
                row.allianceBuffList = '{}'
            end
            if row.allianceLoseBuffList == '' then
                row.allianceLoseBuffList = '{}'
            end
            info.applyList = misc.deserialize(row.applyList)
            info.inviteList = misc.deserialize(row.inviteList)
            local scienceList = misc.deserialize(row.scienceList)
            if scienceList then
                for _, v in pairs(scienceList) do
                    local s = allianceInfo.science:new(v)
                    if s then
                        info.scienceList[s.groupId] = s
                    end
                end
            end
            local allianceBuffList = misc.deserialize(row.allianceBuffList)
            if allianceBuffList then
                for _, v in pairs(allianceBuffList) do
                    local buffInfo = allianceInfo.newAllianceBuff(v)
                    if buffInfo then
                        info.allianceBuffList[buffInfo.buffId] = buffInfo
                    end
                end
            end
            local allianceLoseBuffList = misc.deserialize(row.allianceLoseBuffList)
            if allianceLoseBuffList then
                uselessBuffCity[info.id] = allianceLoseBuffList
            end
            info.isSync = true
            alliances[info.id] = info
        end
    end

    --其他数据载入
    for _, v in pairs(alliances) do
        local alliesCount = v.alliesCount
        local toatlActive = v.toatlActive
        v.alliesCount = 0
        v.toatlActive = 0

        --load
        dbLoadAllianceMembers(db, v.id)
        dbLoadAllianceHelps(db, v.id)
        dbLoadAllianceHeroLeases(db, v.id)

        if alliesCount ~= v.alliesCount or toatlActive ~= v.toatlActive then
            v.isDirty = true
        end
    end
    -- print('dbLoadAllianceHeroLeases..., allianceHeroLeases', utils.serialize(allianceHeroLeases))

    --联盟邀请列表
    dbLoadAllianceInvite(db)

    local rs2 = db:executePrepare('SELECT v FROM s_dict WHERE uid = 0 and k = ?', 'cs_alliance_hero_lease.data')
    if rs2.ok then
        for _, row in ipairs(rs2) do
            local data = misc.deserialize(row.v)
            allianceHeroLeaseData.last5RefreshTime = data.last5RefreshTime or 0
            break
        end
    end
end

dbSaveAlliances = function ()
    local change = {}
    local db = dbo.open(0)
    for _, v in pairs(alliances) do
        if v.isDirty then
            change[v.id] = v
        end
        -- save
        dbSaveAllianceMembers(db, v.id, v.memberList)
        dbSaveAllianceHelps(db, v.id)
        dbSaveAllianceHeroLeases(db, v.id)
    end

    for _, v in pairs(change) do
        local applyList = utils.serialize(v.applyList)
        local inviteList = utils.serialize(v.inviteList)
        local scienceList = {}
        for _, s in pairs(v.scienceList) do
            scienceList[s.groupId] = { groupId = s.groupId, tplId = s.tplId, level = s.level, exp = s.exp }
        end
        scienceList = utils.serialize(scienceList)
        local allianceBuffList = {}
        for _, b in pairs(v.allianceBuffList) do
            allianceBuffList[b.buffId] = { buffId = b.buffId, useCount = b.useCount, createTimestamp = b.createTimestamp, endTimestamp = b.endTimestamp }
        end
        allianceBuffList = utils.serialize(allianceBuffList)

        local allianceLoseBuffCity = uselessBuffCity[v.id]
        local allianceLoseBuffCityList = {}
        allianceLoseBuffCityList = utils.serialize(allianceLoseBuffCity)
        local rs = db:executePrepare('UPDATE s_alliance SET name = ?, nickname = ?, level = ?, exp = ?, bannerId = ?, power = ?, leaderId = ?, leaderName = ?, leaderHeadId = ?, alliesCount = ?, towerCount = ?, castleCount = ?, toatlActive = ?, announcement = ?, openRecruitment = ?, applyList = ?, inviteList = ?, scienceList = ?, allianceBuffList = ?, allianceLoseBuffList = ?, disbandTime = ?, changeBannerCdTime = ? WHERE id = ?',
        v.name, v.nickname, v.level, v.exp, v.bannerId, v.power, v.leaderId, v.leaderName, v.leaderHeadId, v.alliesCount, v.towerCount, v.castleCount, v.toatlActive, v.announcement, v.openRecruitment, applyList, inviteList, scienceList, allianceBuffList, allianceLoseBuffCityList, v.disbandTime, v.changeBannerCdTime, v.id)
        if rs.ok then
            v.isDirty = false
        end
    end

    --联盟邀请列表
    dbSaveAllianceInvite(db)

    if allianceHeroLeaseData.isDirty then
        local db = dbo.open(0)
        local k = 'cs_alliance_hero_lease.data'
        local v = utils.serialize({ last5RefreshTime = allianceHeroLeaseData.last5RefreshTime })
        local rs = db:executePrepare('insert into s_dict(uid, k, v) values (0, ?, ?) on duplicate key update v=values(v)', k, v)
        if rs.ok then
            allianceHeroLeaseData.isDirty = false
        else
            utils.log('dbSaveAlliances save allianceHeroLeaseData error')
        end
    end
end

--
-- publishToAll
--
local function publishAllianceUpdate(alliance, allianceUp)
    local al = alliance2Net(alliance)
    if allianceUp == nil then
        allianceUp = false
    end
    rawService:publishToAll('alliance_update', al, allianceUp)
end

local function publishMemberUpdate(aid, member, joinType)
    local m = member2Net(member)
    rawService:publishToAll('member_update', aid, m, joinType)
end

local function publishMemberOnline(aid, uid)
    rawService:publishToAll('member_online', aid, uid)
end

local function publishMemberOffline(aid, uid, offlineTime)
    rawService:publishToAll('member_offline', aid, uid, offlineTime)
end

local function publishApplyUpdate(aid, uid, nickname, headId, userLevel, vipLevel, time)
    rawService:publishToAll('apply_update', aid, uid, nickname, headId, userLevel, vipLevel, time)
end

local function publishApplyRemove(aid, uid)
    rawService:publishToAll('apply_remove', aid, uid)
end

local function publishInvitedUpdate(aid, uid)
    rawService:publishToAll('invited_update', aid, uid)
end

local function publishInvitedRemove(aid, uid)
    rawService:publishToAll('invited_remove', aid, uid)
end

local function publishInvitedDelete(uid)
    rawService:publishToAll('invited_delete', uid)
end

local function publishMemberDelete(aid, uid, allianceCdTimestamp, isKick)
    rawService:publishToAll('member_delete', aid, uid, allianceCdTimestamp, isKick)
end

local function publishScienceUpdate(aid, science, uplevel)
    local s = science2Net(science)
    rawService:publishToAll('science_update', aid, s, uplevel)
end

local function publishHelpAdd(aid, help)
    rawService:publishToAll('help_add', aid, help)
end

local function publishHelpOne(aid, fromUid, help)
    rawService:publishToAll('help_one', aid, fromUid, help)
end

local function publishHelpAll(aid, fromUid, helps)
    rawService:publishToAll('help_all', aid, fromUid, helps)
end

local function publishHelpAccept(aid, uid, help)
    rawService:publishToAll('help_accept', aid, uid, help)
end

local function publishHelpClose(aid, uid, help)
    rawService:publishToAll('help_close', aid, uid, help)
end

local function publishHireHeroUpdate(aid, uid, leaseInfo)
    local info = leaseInfo2Net(leaseInfo)
    rawService:publishToAll('hire_hero_update', aid, uid, info)
end

local function publishHireHeroRemove(aid, uid, heroIds)
    rawService:publishToAll('hire_hero_remove', aid, uid, heroIds)
end

local function publishEmployHeroUpdate(aid, uid, ownerUid, ownerNickname, heroInfo)
    local info = heroInfo2Net(heroInfo)
    rawService:publishToAll('employ_hero_update', aid, uid, ownerUid, ownerNickname, info)
end

local function publishCollectHeroHireCostUpdate(aid, uid, collectSilver, todayRentCount, newRentCount, timeSilver)
    rawService:publishToAll('collect_hero_hire_cost', aid, uid, collectSilver, todayRentCount, newRentCount, timeSilver)
end

local function publishAllianceRecordUpdate(aid, recordInfo)
    rawService:publishToAll('alliance_record_update', aid, recordInfo)
end

local function publishAllianceHeroleaseRecordAdd(aid, uid, leaseRecordInfo)
    rawService:publishToAll('alliance_hero_lease_record_Add', aid, uid, leaseRecordInfo)
end

local function publishAllianceHeroleaseRecordRemove(aid, uid)
    rawService:publishToAll('alliance_hero_lease_record_remove', aid, uid)
end

local function publishAllianceBuffOpen(aid, allianceBuff)
    local buff = allianceBuff2Net(allianceBuff)
    rawService:publishToAll('alliance_buff_open', aid, buff)
end

local function publishAllianceBuffClosed(aid, buffId)
    rawService:publishToAll('alliance_buff_closed', aid, buffId)
end

local function publishAllianceBuffUpdate(aid, allianceBuffList)
    local buffList = allianceBuffList2net(allianceBuffList)
    rawService:publishToAll('alliance_buff_update', aid, buffList)
end

--
-- base
--
function M.start(mapStub_)
    rawService = cluster.createService('alliance', impl)
    dbLoadAlliances()
    M.timer = timer.setInterval(timerUpdate, 1000)
    mapStub = mapStub_
    -- sync to ms
    local als = {}
    for _, v in pairs(alliances) do
        local al = alliance2SimpleNet(v)
        al.memberList = memberList2SimpleNet(v.memberList)
        al.scienceList = simpleScienceList2Net(v.scienceList)
        table.insert(als, al)
    end
    mapStub.cast_alliance_sync_all(als)
end

function M.onSave()
    M.queueJob(function()
        dbSaveAlliances()
    end)
end

function M.onLogin(uid)
    for _, v in pairs(alliances) do
        if v.memberList[uid] then
            v:memberOnline(uid)
            publishMemberOnline(v.id, uid)
            break
        end
    end
end

function M.onLogout(uid)
    for _, v in pairs(alliances) do
        if v.memberList[uid] then
            local offlineTime = timer.getTimestampCache()
            v:memberOffline(uid, offlineTime)
            publishMemberOffline(v.id, uid, offlineTime)
            break
        end
    end
end

function M.findAllianceByAid(aid)
    for _, v in pairs(alliances) do
        if v.id == aid then
            return v
        end
    end
end

function M.AddAllianceInvite(aid, invitedUid)
    local uList = allianceInviteList[invitedUid]
    if uList == nil then
        allianceInviteList[invitedUid] = {}
        uList = allianceInviteList[invitedUid]
        uList.list = {}
    end
    for _, v in pairs(uList.list) do
        if v == aid then
            return
        end
    end
    table.insert(uList.list, 1, aid)
    uList.list[ALLIANCE_INVITE_MAX+1] = nil
    uList.isDirty = true
    uList.createTime = timer.getTimestampCache()

    --public to fs
    publishInvitedUpdate(aid, invitedUid)
end

function M.removeAllianceInvite(aid, uid)
    local uList = allianceInviteList[uid]
    if uList and uList.list then
        for k, v in pairs(uList.list) do
            if v == aid then
                table.remove(uList.list, k)
                uList.isDirty = true
                uList.createTime = timer.getTimestampCache()
                break
            end
        end
        --public to fs
        publishInvitedRemove(aid, uid)
    end
end

function M.deleteAllianceInvite(uid)
    if allianceInviteList[uid] then
        allianceInviteList[uid] = nil
        dbDeleteAllianceInvite(uid)
    end
    --public to fs
    publishInvitedDelete(uid)
end

function M.removeAllianceApply(uid)
    for _, v in pairs(alliances) do
        if v.disbandTime == 0 then
            if v:removeApply(uid) then
                publishApplyRemove(v.id, uid)
            end
        end
    end
end

function M.collectHeroHireCostAll(aid, uid, bDelete)
    local collectSilver = 0
    local todayRentCount = 0
    local newRentCount = 0
    local timeSilver = 0
    if allianceHeroLeases[aid] then
        local ulease = allianceHeroLeases[aid][uid]
        if ulease and ulease.list then
            local heroIds = {}
            local now = timer.getTimestampCache()
            for heroId, leaseInfo in pairs(ulease.list) do
                local subTime = math.floor((leaseInfo.hookBeginTime - now)/3600)
                local heroInfo = leaseInfo.heroInfo
                collectSilver = collectSilver + leaseInfo.incomeRentSilver
                todayRentCount = todayRentCount + leaseInfo.todayRentCount
                newRentCount = newRentCount + leaseInfo.newRentCount
                if subTime > 0 and heroInfo then
                    local addSilver = math.floor(alliance_hero_hook * heroInfo.level * heroInfo.star * heroInfo.tpl.quality * subTime)
                    if addSilver > 0 then
                        timeSilver = timeSilver + addSilver
                    end
                end
                table.insert(heroIds, { heroId = heroId })
            end
            if bDelete then
                allianceHeroLeases[aid][uid] = nil
                publishHireHeroRemove(aid, uid, heroIds)
            end
        end
    end

    return collectSilver, todayRentCount, newRentCount, timeSilver
end

function M.collectHeroHireCostOne(aid, uid, heroId)
    local collectSilver = 0
    local todayRentCount = 0
    local newRentCount = 0
    local timeSilver = 0
    if allianceHeroLeases[aid] then
        local ulease = allianceHeroLeases[aid][uid]
        if ulease and ulease.list then
            local heroIds = {}
            local now = timer.getTimestampCache()
            local leaseInfo = ulease.list[heroId]
            if leaseInfo then
                local subTime = math.floor((leaseInfo.hookBeginTime - now)/3600)
                local heroInfo = leaseInfo.heroInfo
                collectSilver = collectSilver + leaseInfo.incomeRentSilver
                todayRentCount = todayRentCount + leaseInfo.todayRentCount
                newRentCount = newRentCount + leaseInfo.newRentCount
                if subTime > 0 and heroInfo then
                    local addSilver = math.floor(alliance_hero_hook * heroInfo.level * heroInfo.star * heroInfo.tpl.quality * subTime)
                    if addSilver > 0 then
                        timeSilver = timeSilver + addSilver
                    end
                end
            end
        end
    end

    return collectSilver, todayRentCount, newRentCount, timeSilver
end

function M.addAllianceRecord(aid, messageType, param1)
    local arecord = allianceRecords[aid]
    if arecord == nil then
        arecord = {}
        allianceRecords[aid] = arecord
    end
    local id, content = t.getAllianceMessageContent(messageType)
    -- print('M.addAllianceRecord...', id, messageType, content)
    if id and id > 0 and content then
        local recordInfo = {}
        recordInfo.id = id
        recordInfo.type = messageType
        recordInfo.content = content
        recordInfo.param1 = param1
        recordInfo.createTime = timer.getTimestampCache()
        -- print('M.addAllianceRecord...aid, recordInfo', aid, utils.serialize(recordInfo))

        table.insert(arecord, 1, recordInfo)
        while #arecord > ALLIANCE_RECORD_MAX do
            table.remove(arecord)
        end
        --
        publishAllianceRecordUpdate(aid, recordInfo)
    end
end

function M.addAllianceHeroleaseRecord(aid, uid, leaseRecordInfo)
    local urecord = allianceHeroleaseRecords[uid]
    if urecord == nil then
        urecord = {}
        allianceHeroleaseRecords[uid] = urecord
    end
    table.insert(urecord, 1, leaseRecordInfo)
    while #urecord > HERO_LEASE_RECORD_MAX do
        table.remove(urecord)
    end
    publishAllianceHeroleaseRecordAdd(aid, uid, leaseRecordInfo)
end

function M.removeAllianceHeroleaseRecord(aid, uid, bPublish)
    local urecord = allianceHeroleaseRecords[uid]
    if urecord then
        allianceHeroleaseRecords[uid] = nil
        publishAllianceHeroleaseRecordRemove(aid, uid)
    end
end

function M.getAllianceCount(allianceLevel)
    --print('getAllianceCount allianceLevel = ', allianceLevel)
    local count = 0
    for _, info in pairs(alliances) do
        if info.disbandTime == 0 and info.level >= allianceLevel then
            count = count + 1
        end
    end
    return count
end

function M.onAllianceOwnOccupyCity(allianceOwnCity, aid)
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return p.ErrorCode.ALLIANCE_NOT_EXIST
    end

    --mail
    local city_name = t.mapCity[allianceOwnCity.cityId].remark
    -- print("city_name -------------- ", city_name, allianceCity.cityId, type(allianceCity.cityId))
    local params = {}
    params.params1 = city_name
    for _, v in pairs(al.memberList) do
        local mail = pubMail.newMailInfo()
        mail.uid = v.uid
        mail.otherUid = 0
        mail.type = p.MailType.SYSTEM
        mail.subType = p.MailSubType.SYSTEM_ALLIANCE_OWN_CITY_OCCUPY
        mail.createTime = timer.getTimestampCache()
        mail.title, mail.content = pubMail.getTitleContentBySubType(mail.subType)
        mail.attachment = '{}'
        mail.params = utils.serialize(params)
        mail.isLang = true
        mail.isRead = false
        mail.isDraw = true
        dataService.appendMailInfo(mail)
    end

    --rawService:publishToAll('alliance_own_city', allianceOwnCity)
end

function M.onAllianceLoseOccupyCity(allianceLoseCity, aid)
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return p.ErrorCode.ALLIANCE_NOT_EXIST
    end

    --mail
    local city_name = t.mapCity[allianceLoseCity.cityId].remark
    local params = {}
    params.params1 = city_name
    for _, v in pairs(al.memberList) do
        local mail = pubMail.newMailInfo()
        mail.uid = v.uid
        mail.otherUid = 0
        mail.type = p.MailType.SYSTEM
        mail.subType = p.MailSubType.SYSTEM_ALLIANCE_LOSE_CITY_OCCUPY
        mail.createTime = timer.getTimestampCache()
        mail.title, mail.content = pubMail.getTitleContentBySubType(mail.subType)
        -- mail.attachment = '{}'
        mail.params = utils.serialize(params)
        mail.isLang = true
        mail.isRead = false
        mail.isDraw = true
        dataService.appendMailInfo(mail)
    end

    --rawService:publishToAll('alliance_lose_city', allianceLoseCity)
end

function M.onAllianceOwnCity(allianceOwnCity, aid)
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return p.ErrorCode.ALLIANCE_NOT_EXIST
    end
    al.castleCount = al.castleCount + 1
    al.isDirty = true;
    --mail
    local city_name = t.mapCity[allianceOwnCity.cityId].remark
    local params = {}
    params.params1 = city_name
    for _, v in pairs(al.memberList) do
        local mail = pubMail.newMailInfo()
        mail.uid = v.uid
        mail.otherUid = 0
        mail.type = p.MailType.SYSTEM
        mail.subType = p.MailSubType.SYSTEM_ALLIANCE_OWN_CITY
        mail.createTime = timer.getTimestampCache()
        mail.title, mail.content = pubMail.getTitleContentBySubType(mail.subType)
        mail.attachment = '{}'
        mail.params = utils.serialize(params)
        mail.isLang = true
        mail.isRead = false
        mail.isDraw = true
        mail.reportId = 0
        dataService.appendMailInfo(mail)
    end
    local city = allAllianceCity[aid]
    -- print("###onAllianceOwnCity, AllianceOwnCity", utils.serialize(city))
    -- if city ~= nil then
    --     table.insert(city,allianceOwnCity.cityId)
    -- else
    --     city = {}
    --     table.insert(city,allianceOwnCity.cityId)
    --     allAllianceCity[aid] = city
    -- end
    -- print("###onAllianceOwnCity, AllianceOwnCity", utils.serialize(city))
    M.CheckIsAddCityBuff(allianceOwnCity.allianceId, allianceOwnCity.cityId)
    rawService:publishToAll('alliance_own_city', allianceOwnCity)
end

function M.onAllianceLoseCity(allianceLoseCity, aid)
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return p.ErrorCode.ALLIANCE_NOT_EXIST
    end
    al.castleCount = al.castleCount - 1   
    al.isDirty = true;
    --mail
    local city_name = t.mapCity[allianceLoseCity.cityId].remark
    local params = {}
    params.params1 = city_name
    for _, v in pairs(al.memberList) do
        local mail = pubMail.newMailInfo()
        mail.uid = v.uid
        mail.otherUid = 0
        mail.type = p.MailType.SYSTEM
        mail.subType = p.MailSubType.SYSTEM_ALLIANCE_LOSE_CITY
        mail.createTime = timer.getTimestampCache()
        mail.title, mail.content = pubMail.getTitleContentBySubType(mail.subType)
        mail.attachment = '{}'
        mail.params = utils.serialize(params)
        mail.isLang = true
        mail.isRead = false
        mail.isDraw = true
        dataService.appendMailInfo(mail)
    end
    local city = allAllianceCity[allianceLoseCity.allianceId]
    -- if city ~= nil then
    --     for k,v in pairs(city) do
    --         if v == allianceLoseCity.cityId then
    --             table.remove(city,k)
    --             break
    --         end
    --     end
    -- end
    -- print("###MonAllianceLoseCity, AllianceOwnCity", utils.serialize(city))
    M.CheckIsCancelCityBuff(allianceLoseCity.allianceId, allianceLoseCity.cityId)
    rawService:publishToAll('alliance_lose_city', allianceLoseCity)
end

function M.onAllianceLoseCityBuff(allianceLoseCityId, aid)
    local allianceLoseCity = {}
    allianceLoseCity.cityId = allianceLoseCityId
    allianceLoseCity.allianceId = aid
    local city = uselessBuffCity[aid]
    if city ~= nil then
        table.insert(city,allianceLoseCityId)
    else
        city = {}
        table.insert(city,allianceLoseCityId)
        uselessBuffCity[aid] = city
    end
    alliances[aid].isDirty = true
    -- print("###M.onAllianceLoseCityBuff, allianceLoseCity", utils.serialize(city))
    rawService:publishToAll('alliance_lose_city_buff', allianceLoseCity)
end

function M.onAllianceAddCityBuff(addBuffCityId, aid)
    local allianceAddCity = {} 
    allianceAddCity.cityId = addBuffCityId
    allianceAddCity.allianceId = aid
    local city = uselessBuffCity[aid]
    -- print("###M.onAllianceAddCityBuff", utils.serialize(city))
    if city ~= nil then
        table.insert(city,addBuffCityId)
    else
        city = {}
        table.insert(city,addBuffCityId)
        uselessBuffCity[aid] = city
    end

    local city = uselessBuffCity[allianceAddCity.allianceId]
    if city ~= nil then
        for k,v in pairs(city) do
            if v == allianceAddCity.cityId then
                table.remove(city,k)
                break
            end
        end
    end
    alliances[aid].isDirty = true
    -- print("###M.onAllianceAddCityBuff, allianceLoseCity", utils.serialize(city))
    rawService:publishToAll('alliance_add_city_buff', allianceAddCity)
end

function M.getAllianceData(aid)
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return nil
    end

    local uid = 0
    local nickname = ''
    local headId = 0
    local userLevel = 0
    local vipLevel = 0
    local m = al.memberList[al.leaderId]
    if m then
        uid = m.uid
        nickname = m.nickname
        headId = m.headId
        userLevel = m.userLevel
        vipLevel = m.vipLevel
    end

    local data = {
        allianceName = al.name,
        allianceNickname = al.nickname,
        uid = uid,
        nickname = nickname,
        headId = headId,
        userLevel = userLevel,
        vipLevel = vipLevel,
    }

    return data
end

function M.CheckIsAddCityBuff(aid, cityId)
    -- 先检测是否需要增加一个上一级城市的Buff
    local cityType = M.getMapUnitType(cityId)
    local nextType = p.MapUnitType.UNKNOWN
    local cityLimit = 0
    if cityType == p.MapUnitType.COUNTY then
        nextType = p.MapUnitType.PREFECTURE
        cityLimit = t.configure["cityNumLimit"].prefecture
    elseif cityType == p.MapUnitType.PREFECTURE then
        nextType = p.MapUnitType.CHOW
        cityLimit = t.configure["cityNumLimit"].chow
    elseif cityType == p.MapUnitType.UNKNOWN then
        return
    end
    local cityNum = M.getCityCountByType(aid,cityType)
    local nextCityNum = M.getCityCountByType(aid,nextType)
    local nextLoseBuffCityNum = M.getLoseBuffCityCountByType(aid, nextType)
    local cities = uselessBuffCity[aid]
    if not cities then
        return
    end
    local canHaveCityNum = math.floor(cityNum / cityLimit)
    -- print("###cityNum, nextCityNum, nextLoseBuffCityNum, canHaveCityNum", cityNum, nextCityNum, nextLoseBuffCityNum, canHaveCityNum)
    if canHaveCityNum > (nextCityNum - nextLoseBuffCityNum) then
        local addBuffCityId = 0
        for _, v in ipairs(cities) do
            local mapCityTpl = t.mapCity[v]
            if mapCityTpl then
                -- 再根据地图原件ID去找地图原件类型
                local mapUnitTpl = t.mapUnit[mapCityTpl.unitId]
                if mapUnitTpl.unitType == nextType then
                    addBuffCityId = v
                    break
                end
            end
        end
        -- if cityType == p.MapUnitType.COUNTY then
        --     -- 如果是县城，则还需要检查郡城的数量是否足够
        --     M.CheckIsAddCityBuff(aid,addBuffCityId)
        -- end
        --存储每个失效联盟的CityID
        M.onAllianceAddCityBuff(addBuffCityId,aid)
    end
end

function M.CheckIsCancelCityBuff(aid,cityId)
    -- 先检测是否需要去掉一个上级城市的Buff
    local cityType = M.getMapUnitType(cityId)
    local nextType = p.MapUnitType.UNKNOWN
    local cityLimit = 0
    if cityType == p.MapUnitType.COUNTY then
        nextType = p.MapUnitType.PREFECTURE
        cityLimit = t.configure["cityNumLimit"].prefecture
    elseif cityType == p.MapUnitType.PREFECTURE then
        nextType = p.MapUnitType.CHOW
        cityLimit = t.configure["cityNumLimit"].chow
    elseif cityType == p.MapUnitType.UNKNOWN then
        return
    end
    local cityNum = M.getCityCountByType(aid,cityType)
    local nextCityNum = M.getCityCountByType(aid,nextType)
    local nextLoseBuffCityNum = M.getLoseBuffCityCountByType(aid, nextType)
    local cities = allAllianceCity[aid]
    local canHaveCityNum = math.floor(cityNum / cityLimit)
    -- print("###cityNum, nextCityNum, nextLoseBuffCityNum, canHaveCityNum", cityNum, nextCityNum, nextLoseBuffCityNum, canHaveCityNum)
    if (nextCityNum - nextLoseBuffCityNum)  > canHaveCityNum then
        -- 随机去掉一个上级城市的Buff
        local cancelBuffCityId = 0
        for _, v in ipairs(cities) do
            local mapCityTpl = t.mapCity[v]
            if mapCityTpl then
                -- 再根据地图原件ID去找地图原件类型
                local mapUnitTpl = t.mapUnit[mapCityTpl.unitId]
                if mapUnitTpl.unitType == nextType then
                    cancelBuffCityId = v
                    break
                end
            end
        end
        -- M.allianceLoseCity(aid, cancelBuffCityId)
        -- if cityType == p.MapUnitType.COUNTY then
        --     -- 如果是县城，则还需要检查郡城的数量是否足够
        --     M.CheckIsCancelCityBuff(aid,cancelBuffCityId)
        -- end
        -- 存储每个联盟失效的cityId
        M.onAllianceLoseCityBuff(cancelBuffCityId,aid)
        -- allianceStub.queueJob(function()
        --     allianceStub.add_cancel_buff_city(aid, cancelBuffCityId)
        -- end)
    end
    return 
end

function M.getMapUnitType(cityId)
    local mapCityTpl = t.mapCity[cityId]
    if not mapCityTpl then
        return p.MapUnitType.UNKNOWN
    end
    local mapUnitTpl = t.mapUnit[mapCityTpl.unitId]
    if not mapUnitTpl then
        return p.MapUnitType.UNKNOWN
    end
    return mapUnitTpl.unitType
end

function M.getCityCountByType(aid,type)
    -- 根据类型获得名城数量(县城, 郡城, 州城)
    local count = 0;
    if not allAllianceCity then
        return 0
    end
    local cities = allAllianceCity[aid]
    if not cities then
        return 0
    end
    for _, v in ipairs(cities) do
        -- 根据名城ID找到地图原件ID
        local mapCityTpl = t.mapCity[v]
        while mapCityTpl do
            -- 再根据地图原件ID去找地图原件类型
            local mapUnitTpl = t.mapUnit[mapCityTpl.unitId]
            if mapUnitTpl.unitType == type then
                count = count + 1
            end
            break
        end
    end
    return count
end

-- 获得buff失效的同类型的名城数量
function M.getLoseBuffCityCountByType(aid, type)
    -- 根据类型获得名城数量(县城, 郡城, 州城)
    local count = 0;
    if not uselessBuffCity then
        return 0
    end
    local cities = uselessBuffCity[aid]
    if not cities then
        return 0
    end
    for _, v in ipairs(cities) do
        -- 根据名城ID找到地图原件ID
        local mapCityTpl = t.mapCity[v]
        while mapCityTpl do
            -- 再根据地图原件ID去找地图原件类型
            local mapUnitTpl = t.mapUnit[mapCityTpl.unitId]
            if mapUnitTpl.unitType == type then
                count = count + 1
            end
            break
        end
    end
    return count
end

--
-- alliance service api implement
--
function impl.fetch_alliances()
    -- print('impl.fetch_alliances....')
    local als = {}
    for _, v in pairs(alliances) do
        if v.disbandTime == 0 then
            local al = alliance2Net(v)
            al.applyList = applyList2Net(v.applyList)
            al.memberList = memberList2Net(v.memberList)
            al.scienceList = scienceList2Net(v.scienceList)
            al.allianceBuffList = allianceBuffList2net(v.allianceBuffList)
            table.insert(als, al)
        end
    end
    -- print('impl.fetch_alliances....list', utils.serialize(als))
    return als
end

function impl.fetch_alliance_invited()
    local list = {}
    for uid, v in pairs(allianceInviteList) do
        if list[uid] == nil and #v.list > 0 then
            list[uid] = {}
        end
        for _, aid in pairs(v.list) do
            table.insert(list[uid], aid)
        end
    end
    -- print('impl.fetch_alliance_invited...', utils.serialize(list))
    return list
end

function impl.fetch_alliance_helps()
    -- <key=aid,table<key=helpId,helpInfo>>
    local aHelps = {}
    for aid, helpList in pairs(allianceHelps) do
        if aHelps[aid] == nil then
            aHelps[aid] = {}
        end
        for _, v in pairs(helpList) do
            local help = {}
            help.uid = v.uid
            help.helpId = v.helpId
            help.buildingId = v.buildingId
            help.times = v.times
            help.accept = v.accept
            help.max = v.max
            help.reduceTime = v.reduceTime
            help.cdId = v.cdId
            help.cdTime = v.cdTime
            help.helpUserList = v.helpUserList
            help.helpDesc = v.helpDesc
            aHelps[aid][help.helpId] = help
        end
    end
    return aHelps
end

function impl.fetch_alliance_hero_leases()
    --<aid, <uid, <heroId, leaseInfo>>>
    local retLeases = {}
    for aid, alease in pairs(allianceHeroLeases) do
        if retLeases[aid] == nil then
            retLeases[aid] = {}
        end
        for uid, ulease in pairs(alease) do
            if retLeases[aid][uid] == nil then
                retLeases[aid][uid] = {}
            end
            local leaseList = {}
            for _, leaseInfo in pairs(ulease.list) do
                local info = leaseInfo2Net(leaseInfo)
                table.insert(leaseList, info)
            end
            if #leaseList > 0 then
                retLeases[aid][uid] = leaseList
            else
                retLeases[aid][uid] = nil
            end
        end
    end

    return retLeases
end

function impl.fetch_alliance_records()
    local records = {}
    for aid, recordList in pairs(allianceRecords) do
        if records[aid] == nil then
            records[aid] = {}
        end
        for k, v in pairs(recordList) do
            local recordInfo = {}
            recordInfo.type = v.type
            recordInfo.content = v.content
            recordInfo.param1 = v.param1
            recordInfo.createTime = v.createTime

            records[aid][k] = recordInfo
        end
    end
    return records
end

function impl.fetch_alliance_hero_lease_records()
    local records = {}
    for uid, v in pairs(allianceHeroleaseRecords) do
        records[uid] = {
            employId = v.employId,
            heroId = v.heroId,
            uid = v.uid,
            nickname = v.nickname,
            headId = v.headId,
            userLevel = v.userLevel,
            vipLevel = v.vipLevel,
            rentSilver = v.rentSilver,
            createTime = v.createTime,
            rentCount = v.rentCount,
            todayRentCount = v.todayRentCount,
            newRentCount = v.newRentCount,
        }
    end
    return records
end

function impl.fetch_alliance_city()
    if not mapStub then
        return
    end
    allAllianceCity = mapStub.allAllianceCity
    -- print("###allianceService", utils.serialize(allAllianceCity))
    return allAllianceCity
end

function impl.fetch_alliance_lose_buff_city()
    return uselessBuffCity
end

function impl.alliance_mail(aid, fromUid, fromNickname, fromHeadId, fromLangType, content)
    -- print('impl.alliance_mail ...aid, fromUid, fromNickname, fromHeadId, fromLangType, content', aid, fromUid, fromNickname, fromHeadId, fromLangType, content)
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return p.ErrorCode.ALLIANCE_NOT_EXIST
    end
    local fromMenber = al.memberList[fromUid]
    if fromMenber == nil then
        return p.ErrorCode.ALLIANCE_NOT_MEMBER
    end
    --是否有发送权限
    if not fromMenber:haveRankPermit(rankPermitType.MAIL) then
        return p.ErrorCode.ALLIANCE_WITHOUT_PERMIT
    end
    --发送次数是否足够
    local maxCount = fromMenber:haveSendMailMax()
    -- print('impl.alliance_mail....sendMailCount, sendMailMax', fromMenber.sendMailCount, maxCount)
    if fromMenber.sendMailCount >= maxCount then
        return p.ErrorCode.ALLIANCE_SEND_MAIL_MAX
    end
    fromMenber.sendMailCount = fromMenber.sendMailCount + 1
    fromMenber.isDirty = true
    fromMenber.isSync = false
    publishMemberUpdate(aid, fromMenber)

    -- create mail
    M.queueJob(function()
        -- create mail
        local params = {
            allianceName = al.name,
            allianceNickname = al.nickname,
            bannerId = al.bannerId,
            alliesLevel = al.level,
            leaderName = al.leaderName,
            alliesCount = al.alliesCount,
            alliesMax = al.alliesMax,
            alliesPower = al.power,
        }
        for _, v in pairs(al.memberList) do
            --联盟全体邮件屏蔽给自己发邮件
            if fromUid ~= v.uid then
                local mail = pubMail.newMailInfo()
                mail.uid = v.uid
                mail.otherUid = fromUid
                mail.type = p.MailType.PRIVATE
                mail.subType = p.MailSubType.PRIVATE_ALLIANCE_MEMBER
                mail.createTime = timer.getTimestampCache()
                -- mail.title = pubMail.getTitleContentBySubType(mail.subType)
                --标题是对方的名字
                mail.title = fromMenber.nickname
                mail.content = content
                mail.attachment = ''
                mail.params = utils.serialize(params)
                mail.isLang = true
                mail.isRead = false
                mail.isDraw = true
                dataService.appendMailInfo(mail)
            end
        end
    end)
    return p.ErrorCode.SUCCESS
end

function impl.checkName(isCheckName, name)
    -- print('impl.checkName ...isCheckName, name',isCheckName, name)
    if isRepeatName(name) then
        -- print('impl.checkName ...', name, ' is repeat')
        return p.ErrorCode.ALLIANCE_NAME_REPETE
    end
    if not isValidName(name) then
        return p.ErrorCode.ALLIANCE_NAME_INVALID
    end
    return p.ErrorCode.SUCCESS
end

function impl.create(allianceName, allianceNickname, bannerId, uid, userNickname, headId, userLevel, vipLevel, power, castleLevel, x, y, marketIsOpen)
    -- print('impl.create...allianceName, allianceNickname, bannerId, uid, userNickname, headId, userLevel, vipLevel', allianceName, allianceNickname, bannerId, uid, userNickname, headId, userLevel, vipLevel)
    rawService:disableAutoReply()
    local session = rawService:session()
    local ret = p.ErrorCode.SUCCESS
    repeat
        --check name
        if not isValidName(allianceName) then
            rawService:reply(session, p.ErrorCode.ALLIANCE_NAME_INVALID)
            break
        end
        if isRepeatName(allianceName) then
            rawService:reply(session, p.ErrorCode.ALLIANCE_NAME_REPETE)
            break
        end

        --crate
        local al = allianceInfo.info:new()
        al.name = allianceName
        al.nickname = allianceNickname
        al.level = 1
        al.exp = 0
        al.bannerId = bannerId
        al.leaderId = uid
        al.leaderName = userNickname
        al.leaderHeadId = headId
        al.alliesMax = t.allianceLevel[1].alliesMax
        al.towerMax = t.allianceLevel[1].towerMax
        al.castleMax = t.allianceLevel[1].castleMax
        al.createTime = timer.getTimestampCache()
        al.isDirty = true

        M.queueJob(function()
            --遍历所有联盟删除该玩家的申请
            M.removeAllianceApply(uid)
            --删除该玩家的所有邀请信息
            M.deleteAllianceInvite(uid)

            al.id = dbAddAlliance(al)
            -- print('alliance impl.create ... aid', al.id)
            alliances[al.id] = al
            --sync to cs service
            allianceDataSyncToService(uid, al.id, al.name, al.nickname, al.bannerId)
            -- update to fs
            publishAllianceUpdate(al)

            local m = allianceInfo.member:new()
            m.uid = uid
            m.nickname = userNickname
            m.headId = headId
            m.userLevel = userLevel
            m.vipLevel = vipLevel
            m.rankLevel = 1
            m.activeWeek = 0
            m.contribution = 0
            m.sendMailCount = 0
            m.totalPower = power
            m.castleLevel = castleLevel
            m.x = x
            m.y = y
            m.marketIsOpen = marketIsOpen
            m.lastOnlineTime = 0
            m.joinTime = timer.getTimestampCache()
            m.isDirty = true
            m.isSync = true
            al:addMember(m)
            -- update to fs
            publishMemberUpdate(al.id, m, 1)

            --return
            rawService:reply(session, ret)

            --sync to ms
            local tempAllies = alliance2SimpleNet(al)
            tempAllies.memberList = memberList2Net(al.memberList)
            tempAllies.scienceList = simpleScienceList2Net(al.scienceList)
            mapStub.cast_alliance_create(tempAllies)

            achievementService.onAllianceUpdate(0, al.level)
        end)
    until true
end

function impl.apply_request(aid, uid, nickname, headId, userLevel, vipLevel)
    -- print('impl.apply_request...aid, uid, nickname, headId, userLevel, vipLevel', aid, uid, nickname, headId, userLevel, vipLevel)
    local ret = p.ErrorCode.SUCCESS
    repeat
        --1.联盟是否存在
        local al = alliances[aid]
        if al == nil or al.disbandTime ~= 0 then
            ret = p.ErrorCode.ALLIANCE_NOT_EXIST
            break
        end
        --2.检查申请玩家是否有联盟
        if isJoinAlliance(uid) then
            ret = p.ErrorCode.ALLIANCE_JOINED
            break
        end
        --3.检查该玩家是否在申请列表里
        local apply = al:findApply(uid)
        if apply then
            --不发tips
            break
        end
        --4.检查联盟人数是否已满
        if al.alliesCount >= al.alliesMax then
            ret = p.ErrorCode.ALLIANCE_FULL
            break
        end
        --add
        local now = timer.getTimestampCache()
        al:addApply(uid, nickname, headId, userLevel, vipLevel, now)
        --public
        publishApplyUpdate(aid, uid, nickname, headId, userLevel, vipLevel, now)
    until true

    return ret
end

function impl.apply_accept(aid, uid, isAccept, applyUid)
    -- print('imp.apply_accept..aid, uid, isAccept, applyUid', aid, uid, isAccept, applyUid)
    rawService:disableAutoReply()
    local session = rawService:session()
    local ret = p.ErrorCode.SUCCESS
    repeat
        --1.联盟是否存在
        local al = alliances[aid]
        if al == nil or al.disbandTime ~= 0 then
            rawService:reply(session, p.ErrorCode.ALLIANCE_NOT_EXIST)
            break
        end
        --2.检查申请玩家是否有联盟
        if isJoinAlliance(applyUid) then
            rawService:reply(session, p.ErrorCode.ALLIANCE_JOINED)
            break
        end
        --3.检查联盟人数是否已满
        if al.alliesCount >= al.alliesMax then
            rawService:reply(session, p.ErrorCode.ALLIANCE_FULL)
            break
        end
        --4.是否有权限
        local myMember = al.memberList[uid]
        if myMember == nil then
            rawService:reply(session, p.ErrorCode.ALLIANCE_WITHOUT_PERMIT)
            break
        end
        if not myMember:haveRankPermit(rankPermitType.ALLIANCE_ACCEPT_JOIN) then
            rawService:reply(session, p.ErrorCode.ALLIANCE_WITHOUT_PERMIT)
            break
        end
        --申请列表是否有该玩家
        local apply = al:findApply(applyUid)
        if apply == nil then
            rawService:reply(session, p.ErrorCode.ALLIANCE_NOT_FOUND_USER)
            break
        end
        al:removeApply(applyUid)
        publishApplyRemove(aid, applyUid)

        --add
        if isAccept then
            --遍历所有联盟删除该玩家的申请
            M.removeAllianceApply(applyUid)
            --sync to cs service
            allianceDataSyncToService(applyUid, aid, al.name, al.nickname, al.bannerId)

            M.queueJob(function()
                --删除该玩家的所有邀请信息
                M.deleteAllianceInvite(applyUid)

                local userInfo = dbLoadUserInfo(applyUid)
                local m = allianceInfo.member:new()
                m.uid = applyUid
                m.nickname = userInfo.nickname
                m.headId = userInfo.headId
                m.userLevel = userInfo.userLevel
                m.vipLevel = userInfo.vipLevel
                m.rankLevel = 4
                m.activeWeek = 0
                m.contribution = 0
                m.sendMailCount = 0
                m.totalPower = userInfo.totalPower
                m.castleLevel = userInfo.castleLevel
                m.x = userInfo.x
                m.y = userInfo.y
                m.marketIsOpen = userInfo.marketIsOpen
                m.lastOnlineTime = userInfo.lastLoginTimestamp
                if loginService.findUser(applyUid) then
                    m.lastOnlineTime = 0
                end
                m.joinTime = timer.getTimestampCache()
                m.isDirty = true
                m.isSync = true

                al:addMember(m)
                publishMemberUpdate(aid, m, 2)

                --联盟日记
                local messageType = p.AllianceMessageType.ALLIANCE_JOIN
                local param1 = userInfo.nickname
                M.addAllianceRecord(aid, messageType, param1)

                rawService:reply(session, ret)

                --sync to ms
                mapStub.cast_alliance_add_member(aid, applyUid)
            end)
        else
            rawService:reply(session, ret)
        end
    until true
end

function impl.invite(aid, uid, invitedUids)
    --1.邀请者是否有联盟
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return
    end
    local myMember = al.memberList[uid]
    if myMember == nil then
        return
    end
    --2.检查联盟人数是否已满
    if al.alliesCount >= al.alliesMax then
        return
    end
    --3.邀请者是否有权限
    if not myMember:haveRankPermit(rankPermitType.ALLIANCE_INVITE) then
        return
    end
    --4.被邀请者是否有联盟
    M.queueJob(function()
        local now = timer.getTimestampCache()
        for _, invitedUid in pairs(invitedUids) do
            if not isJoinAlliance(invitedUid) then
                M.AddAllianceInvite(aid, invitedUid)

                -- --sync to ms
                -- mapStub.cast_alliance_record_invited(invitedUid)
            end
        end
    end)
end

function impl.invite_accept(aid, uid, userNickname, headId, userLevel, vipLevel, isAccept, totalPower, castleLevel, x, y, marketIsOpen)
    -- print('impl.invite_accept...aid, uid, userNickname, headId, userLevel, vipLevel, isAccept', aid, uid, userNickname, headId, userLevel, vipLevel, isAccept)
    rawService:disableAutoReply()
    local session = rawService:session()
    local ret = p.ErrorCode.SUCCESS
    repeat
        --1.联盟是否存在
        local al = alliances[aid]
        if al == nil or al.disbandTime ~= 0 then
            rawService:reply(session, p.ErrorCode.ALLIANCE_NOT_EXIST)
            break
        end
        --2.检查玩家是否有联盟
        if isJoinAlliance(uid) then
            rawService:reply(session, p.ErrorCode.ALLIANCE_JOINED)
            break
        end
        --3.检查联盟人数是否已满
        if al.alliesCount >= al.alliesMax then
            rawService:reply(session, p.ErrorCode.ALLIANCE_FULL)
            break
        end
        M.queueJob(function()
            if isAccept then
                --遍历所有联盟删除该玩家的申请
                M.removeAllianceApply(uid)
                --删除该玩家的所有邀请信息
                M.deleteAllianceInvite(uid)

                --sync to cs service
                allianceDataSyncToService(uid, aid, al.name, al.nickname, al.bannerId)

                -- local userInfo = dbLoadUserInfo(uid)
                local m = allianceInfo.member:new()
                m.uid = uid
                m.nickname = userNickname
                m.headId = headId
                m.userLevel = userLevel
                m.vipLevel = vipLevel
                m.rankLevel = 4
                m.activeWeek = 0
                m.contribution = 0
                m.sendMailCount = 0
                m.totalPower = totalPower
                m.castleLevel = castleLevel
                m.x = x
                m.y = y
                m.marketIsOpen = marketIsOpen
                m.lastOnlineTime = 0
                m.joinTime = timer.getTimestampCache()
                m.isDirty = true
                m.isSync = true

                al:addMember(m)
                publishMemberUpdate(aid, m, 3)

                --联盟日记
                local messageType = p.AllianceMessageType.ALLIANCE_JOIN
                local param1 = userNickname
                M.addAllianceRecord(aid, messageType, param1)

                --sync to ms
                mapStub.cast_alliance_add_member(aid, uid)
            else
                M.removeAllianceInvite(aid, uid)
                --sync to ms邀请列表 ????
            end
            rawService:reply(session, ret)
        end)
    until true
end

function impl.quit(aid, uid)
    -- print('impl.quit...aid, uid', aid, uid)
    local ret = p.ErrorCode.SUCCESS
    repeat
        --联盟是否存在
        local al = alliances[aid]
        if al == nil or al.disbandTime ~= 0 then
            ret = p.ErrorCode.ALLIANCE_NOT_EXIST
            break
        end
        --是否是联盟成员
        local myself = al.memberList[uid]
        if myself == nil then
            ret = p.ErrorCode.ALLIANCE_NOT_MEMBER
            break
        end
        --盟主不可以退盟
        if uid == al.leaderId then
            ret = p.ErrorCode.ALLIANCE_LEADER_TRANSFER
            break
        end
        local param1 = myself.nickname
        --sync to cs service
        allianceDataSyncToService(uid, 0, '', '', 0)

        M.queueJob(function()
            --武将租借收益
            local collectSilver, todayRentCount, newRentCount, timeSilver = M.collectHeroHireCostAll(aid, uid, true)
            if collectSilver > 0 or timeSilver > 0 then
                publishCollectHeroHireCostUpdate(aid, uid, collectSilver, todayRentCount, newRentCount, timeSilver)
            end

            local db = dbo.open(0)
            dbdeleteAllianceHeroLeases(db, uid)
            --
            local allianceCdTimestamp = timer.getTimestampCache() + allianceCdInterval
            al:removeMember(uid)
            publishMemberDelete(aid, uid, allianceCdTimestamp, false)
            dbDeleteAllianceMember(db, aid, uid)

            --联盟日记
            local messageType = p.AllianceMessageType.ALLIANCE_QUIT
            M.addAllianceRecord(aid, messageType, param1)
            --出租记录
            M.removeAllianceHeroleaseRecord(aid, uid)

            -- need sync to ms
            mapStub.cast_alliance_remove_member(aid, uid, allianceCdTimestamp)
        end)
    until true

    return ret
end

function impl.kick(aid, uid, kickUid)
    -- print('impl.kick...aid, uid, kickUid', aid, uid, kickUid)
    local ret = p.ErrorCode.SUCCESS
    repeat
        --联盟是否存在
        local al = alliances[aid]
        if al == nil or al.disbandTime ~= 0 then
            ret = p.ErrorCode.ALLIANCE_NOT_EXIST
            break
        end
        --是否有权限
        local myself = al.memberList[uid]
        if myself == nil or not myself:haveRankPermit(rankPermitType.KICK) then
            ret = p.ErrorCode.ALLIANCE_WITHOUT_PERMIT
            break
        end
        --被踢人是否是联盟成员
        local kickMember = al.memberList[kickUid]
        if kickMember == nil then
            ret = p.ErrorCode.ALLIANCE_NOT_MEMBER
            break
        end
        --职位高低
        local rankDiff = kickMember.rankLevel - myself.rankLevel
        if rankDiff < 1 then
            ret = p.ErrorCode.ALLIANCE_WITHOUT_PERMIT
            break
        end
        local param1 = myself.nickname .. ',' .. kickMember.nickname
        --sync to cs service
        allianceDataSyncToService(kickUid, 0, '', '', 0)

        M.queueJob(function()
            --武将租借收益
            local drops = {}
            local collectSilver, todayRentCount, newRentCount, timeSilver = M.collectHeroHireCostAll(aid, kickUid, true)
            local isDraw = true
            if collectSilver > 0 or timeSilver > 0 then
                isDraw = false
                table.insert(drops, { tplId = p.SpecialPropIdType.SILVER, count = collectSilver + timeSilver, cond = {} })
            end
            local attachments = utils.serialize(drops)

            local db = dbo.open(0)
            dbdeleteAllianceHeroLeases(db, kickUid)
            --
            local allianceCdTimestamp = timer.getTimestampCache() + allianceCdInterval
            al:removeMember(kickUid)
            publishMemberDelete(aid, kickUid, allianceCdTimestamp, true)
            dbDeleteAllianceMember(db, aid, kickUid)
            --保存联盟CD,防止玩家不在线被踢
            -- if not loginService.findUser(kickUid) then
                dbSaveAllianceCdTimestamp(db, kickUid, allianceCdTimestamp)
            -- end

            --联盟日记
            local messageType = p.AllianceMessageType.ALLIANCE_KICK
            M.addAllianceRecord(aid, messageType, param1)
            --出租记录
            M.removeAllianceHeroleaseRecord(aid, uid)

            --mail
            local params = {
                allianceName = al.name,
                allianceNickname = al.nickname,
                bannerId = al.bannerId,
                alliesLevel = al.level,
                leaderName = al.leaderName,
                alliesCount = al.alliesCount,
                alliesMax = al.alliesMax,
                alliesPower = al.power,
            }
            params.params1 = ''
            local mail = pubMail.newMailInfo()
            mail.uid = kickUid
            mail.otherUid = 0
            mail.type = p.MailType.SYSTEM
            mail.subType = p.MailSubType.SYSTEM_ALLIANCE_KICK
            mail.createTime = timer.getTimestampCache()
            mail.title, mail.content = pubMail.getTitleContentBySubType(mail.subType)
            mail.isLang = true
            mail.isDraw = isDraw
            mail.attachment = attachments
            mail.params = utils.serialize(params)
            dataService.appendMailInfo(mail)

            -- need sync to ms
            mapStub.cast_alliance_remove_member(aid, kickUid, allianceCdTimestamp)
        end)
    until true

    return ret
end

function impl.change_banner(aid, uid, bannerId)
    -- print('impl.change_banner...aid, uid, bannerId', aid, uid, bannerId)
    --联盟是否存在
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return p.ErrorCode.ALLIANCE_NOT_EXIST
    end
    --是否有权限
    local myself = al.memberList[uid]
    if myself == nil or not myself:haveRankPermit(rankPermitType.CHANGE_BANNER) then
        return p.ErrorCode.ALLIANCE_WITHOUT_PERMIT
    end
    --换联盟旗帜CD
    local now = timer.getTimestampCache()
    if now < al.changeBannerCdTime then
        return p.ErrorCode.FAIL
    end

    --change
    al.bannerId = bannerId
    al.changeBannerCdTime = now + changeBannerCdInterval
    al.isDirty = true
    al.isSync = false

    --sync to cs service
    for _, v in pairs(al.memberList) do
        allianceDataSyncToService(v.uid, aid, al.name, al.nickname, al.bannerId)
    end

    --public to fs
    publishAllianceUpdate(al)

    -- need sync to ms
    local simple = alliance2SimpleNet(al)
    mapStub.cast_alliance_update(simple)

    return p.ErrorCode.SUCCESS
end

function impl.change_announcement(aid, uid, announcement)
    -- print('impl.change_announcement...aid, uid, announcement', aid, uid, announcement)
    --联盟是否存在
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return p.ErrorCode.ALLIANCE_NOT_EXIST
    end
    --是否有权限
    local myself = al.memberList[uid]
    if myself == nil or not myself:haveRankPermit(rankPermitType.CHANGE_ANNOUNCEMENT) then
        return p.ErrorCode.ALLIANCE_WITHOUT_PERMIT
    end

    --change
    al.announcement = announcement
    al.isDirty = true
    al.isSync = false

    --public to fs
    publishAllianceUpdate(al)

    return p.ErrorCode.SUCCESS
end

function impl.rankUp(aid, uid, upUid)
    -- print('impl.rankUp...aid, uid, upUid', aid, uid, upUid)
    --联盟是否存在
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return p.ErrorCode.ALLIANCE_NOT_EXIST
    end
    --是否有权限
    local myself = al.memberList[uid]
    if myself == nil or not myself:haveRankPermit(rankPermitType.RANK_UP) then
        return p.ErrorCode.ALLIANCE_WITHOUT_PERMIT
    end
    --升职人是否是联盟成员
    local upMember = al.memberList[upUid]
    if upMember == nil then
        return p.ErrorCode.ALLIANCE_NOT_MEMBER
    end
    --职位高低, 副盟主不能再提升职位
    local rankDiff = upMember.rankLevel - myself.rankLevel
    if rankDiff < 1 or upMember.rankLevel == 2 then
        return p.ErrorCode.ALLIANCE_NOT_RANK_UP
    end

    --change
    upMember.rankLevel = upMember.rankLevel - 1
    upMember.isDirty = true
    upMember.isSync = false

    --联盟日记
    local messageType = p.AllianceMessageType.RANK_UP
    local param1 = myself.nickname .. ',' .. upMember.nickname .. ',' .. tostring(upMember.rankLevel)
    M.addAllianceRecord(aid, messageType, param1)

    --public to fs
    publishMemberUpdate(al.id, upMember)

    return p.ErrorCode.SUCCESS
end

function impl.rankDown(aid, uid, downUid)
    -- print('impl.rankDown...aid, uid, downUid', aid, uid, downUid)
    --联盟是否存在
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return p.ErrorCode.ALLIANCE_NOT_EXIST
    end
    --是否有权限
    local myself = al.memberList[uid]
    if myself == nil or not myself:haveRankPermit(rankPermitType.RANK_UP) then
        return p.ErrorCode.ALLIANCE_WITHOUT_PERMIT
    end
    --升职人是否是联盟成员
    local downMember = al.memberList[downUid]
    if downMember == nil then
        return p.ErrorCode.ALLIANCE_NOT_MEMBER
    end
    --职位高低, 成员不能再降低职位
    local rankDiff = downMember.rankLevel - myself.rankLevel
    if rankDiff < 1 or downMember.rankLevel == 4 then
        return p.ErrorCode.ALLIANCE_NOT_RANK_DOWN
    end

    --change
    downMember.rankLevel = downMember.rankLevel + 1
    downMember.isDirty = true
    downMember.isSync = false

    --联盟日记
    local messageType = p.AllianceMessageType.RANK_DOWN
    local param1 = myself.nickname .. ',' .. downMember.nickname .. ',' .. tostring(downMember.rankLevel)
    M.addAllianceRecord(aid, messageType, param1)

    --public to fs
    publishMemberUpdate(al.id, downMember)

    return p.ErrorCode.SUCCESS
end

function impl.leader_transfer(aid, uid, transferUid)
    -- print('impl.leader_transfer...aid, uid, transferUid', aid, uid, transferUid)
    --联盟是否存在
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return p.ErrorCode.ALLIANCE_NOT_EXIST
    end
    if al.leaderId ~= uid then
        return p.ErrorCode.ALLIANCE_WITHOUT_PERMIT
    end
    --是否有权限
    local myself = al.memberList[uid]
    if myself == nil or not myself:haveRankPermit(rankPermitType.TRANSFER) then
        return p.ErrorCode.ALLIANCE_WITHOUT_PERMIT
    end
    --
    local transferMember = al.memberList[transferUid]
    if transferMember == nil then
        return p.ErrorCode.ALLIANCE_NOT_MEMBER
    end
    --
    if myself.rankLevel ~= 1 or transferMember.rankLevel ~= 2 then
        return p.ErrorCode.ALLIANCE_WITHOUT_PERMIT
    end

    --change
    transferMember.rankLevel = 1
    transferMember.isDirty = true
    transferMember.isSync = false

    myself.rankLevel = 2
    myself.isDirty = true
    myself.isSync = false

    local oldLeaderName = al.leaderName
    al.leaderId = transferMember.uid
    al.leaderName = transferMember.nickname
    al.leaderHeadId = transferMember.headId
    al.isDirty = true
    al.isSync = false

    --public to fs
    publishAllianceUpdate(al)
    publishMemberUpdate(al.id, myself)
    publishMemberUpdate(al.id, transferMember)

    --联盟日记
    local messageType = p.AllianceMessageType.LEADER_TRANSFER
    local param1 = myself.nickname .. ',' .. transferMember.nickname
    M.addAllianceRecord(aid, messageType, param1)

    -- need sync to ms
    local simple = alliance2SimpleNet(al)
    mapStub.cast_alliance_update(simple)

    local params = {
        allianceName = al.name,
        allianceNickname = al.nickname,
        bannerId = al.bannerId,
        alliesLevel = al.level,
        leaderName = al.leaderName,
        alliesCount = al.alliesCount,
        alliesMax = al.alliesMax,
        alliesPower = al.power,
    }
    params.params1 = oldLeaderName .. ',' .. al.leaderName
    for _, v in pairs(al.memberList) do
        local mail = pubMail.newMailInfo()
        mail.uid = v.uid
        mail.otherUid = 0
        mail.type = p.MailType.SYSTEM
        mail.subType = p.MailSubType.SYSTEM_ALLIANCE_LEADER_TRANSFER
        mail.createTime = timer.getTimestampCache()
        mail.title, mail.content = pubMail.getTitleContentBySubType(mail.subType)
        mail.attachment = utils.serialize({})
        mail.params = utils.serialize(params)
        mail.isLang = true
        mail.isRead = false
        mail.isDraw = true
        dataService.appendMailInfo(mail)
    end

    return p.ErrorCode.SUCCESS
end

function impl.leader_replace(aid, uid)
    -- print('impl.leader_replace...aid, uid', aid, uid)
    --联盟是否存在
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return p.ErrorCode.ALLIANCE_NOT_EXIST
    end
    if al.leaderId == uid then
        return p.ErrorCode.ALLIANCE_WITHOUT_PERMIT
    end
    --是否有权限
    local myself = al.memberList[uid]
    if myself == nil or not myself:haveRankPermit(rankPermitType.REPLACE_LEADER) then
        return p.ErrorCode.ALLIANCE_WITHOUT_PERMIT
    end
    --
    local leaderMember = al.memberList[al.leaderId]
    if leaderMember == nil or leaderMember.rankLevel ~= 1 then
        return p.ErrorCode.ALLIANCE_WITHOUT_PERMIT
    end
    --比自己职位高 有7天内上线的
    local now = timer.getTimestampCache()
    for _, v in pairs(al.memberList) do
        if v.rankLevel < myself.rankLevel then
            if v.lastOnlineTime + replaceInterval < now then
                return p.ErrorCode.FAIL
            end
        end
    end

    -- replace
    leaderMember.rankLevel = myself.rankLevel
    leaderMember.isDirty = true
    leaderMember.isSync = false

    myself.rankLevel = 1
    myself.isDirty = true
    myself.isSync = false

    local oldLeaderName = al.leaderName
    al.leaderId = myself.uid
    al.leaderName = myself.nickname
    al.leaderHeadId = myself.headId
    al.isDirty = true
    al.isSync = false

    --public to fs
    publishAllianceUpdate(al)
    publishMemberUpdate(al.id, myself)
    publishMemberUpdate(al.id, leaderMember)

    -- need sync to ms
    local simple = alliance2SimpleNet(al)
    mapStub.cast_alliance_update(simple)
    
    local params = {
        allianceName = al.name,
        allianceNickname = al.nickname,
        bannerId = al.bannerId,
        alliesLevel = al.level,
        leaderName = al.leaderName,
        alliesCount = al.alliesCount,
        alliesMax = al.alliesMax,
        alliesPower = al.power,
    }
    params.params1 = oldLeaderName .. ',' .. al.leaderName
    for _, v in pairs(al.memberList) do
        local mail = pubMail.newMailInfo()
        mail.uid = v.uid
        mail.otherUid = 0
        mail.type = p.MailType.SYSTEM
        mail.subType = p.MailSubType.SYSTEM_ALLIANCE_LEADER_REPLACE
        mail.createTime = timer.getTimestampCache()
        mail.title, mail.content = pubMail.getTitleContentBySubType(mail.subType)
        mail.attachment = utils.serialize({})
        mail.params = utils.serialize(params)
        mail.isLang = true
        mail.isRead = false
        mail.isDraw = true
        dataService.appendMailInfo(mail)
    end

    return p.ErrorCode.SUCCESS
end

function impl.science_donate(aid, uid, groupId, scienceExp, activeCount)
    -- print('impl.science_donate...aid, uid, groupId, scienceExp, activeCount', aid, uid, groupId, scienceExp, activeCount)
    --联盟是否存在
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return p.ErrorCode.ALLIANCE_NOT_EXIST
    end
    --是否是联盟成员
    local myself = al.memberList[uid]
    if myself == nil then
        return p.ErrorCode.ALLIANCE_NOT_MEMBER
    end
    --科技是否存在
    local groupTpl = t.allianceScience[groupId]
    if groupTpl == nil then
        return p.ErrorCode.FAIL
    end
    --
    local science = al:findScience(groupId)
    if science == nil then
        science = allianceInfo.science:new({ groupId = groupId, level = 1 })
        al:addScience(science)
    end
    -- print('impl.science_donate1111...groupId, level, exp, tplId', groupId, science.level, science.exp, science.tplId)
    --科技是否满级
    local maxLevel = #groupTpl
    if science.level >= maxLevel then
        return p.ErrorCode.ALLIANCE_SCIENCE_LEVEL_FULL
    end
    --科技是否开启
    local tplLevel = groupTpl[science.level]
    if tplLevel == nil or al.level < tplLevel.allianceLevel then
        return p.ErrorCode.ALLIANCE_SCIENCE_NOT_OPEN
    end

    -- --科技贡献值  跟联盟积分冲突，删除
    -- myself.contribution = myself.contribution + allianceScienceDonate

    --成员活跃值
    myself.activeWeek = myself.activeWeek + activeCount
    myself.isDirty = true
    myself.isSync = false

    --科技经验
    local oldScienceLevel = science.level
    local openLevel = tplLevel.allianceLevel
    local totalScienceExp = science.exp + scienceExp
    local levelup = false
    local allianceUp = false
    local oldAlLevel = al.level
    -- print('impl.science_donate#####11111...openLevel, allianceLevel, totalScienceExp, maxScienceExp, scienceLevel, scienceMaxLevel', openLevel, al.level, totalScienceExp, tplLevel.exp, science.level, maxLevel)
    while tplLevel ~= nil
        and science.level < maxLevel
        and totalScienceExp >= tplLevel.exp
        and al.level >= openLevel do

        totalScienceExp = totalScienceExp - tplLevel.exp
        science.level = science.level + 1
        science.exp = 0
        levelup = true
        --科技升级获得联盟经验
        local allianceExp = tplLevel.allianceExp
        if allianceExp > 0 then
            local alUp = al:AddAllianceExp(allianceExp)
            if alUp then
                allianceUp = true
            end
        end
        --开启需要的联盟等级
        local tplLevel = groupTpl[science.level]
        if tplLevel ~= nil then
            openLevel = tplLevel.allianceLevel
        end
        -- print('impl.science_donate#####2222...openLevel, allianceLevel, totalScienceExp, maxScienceExp, scienceLevel, scienceMaxLevel', openLevel, al.level, totalScienceExp, tplLevel.exp, science.level, maxLevel)
    end
    if levelup then
        if al.level >= openLevel then
            science.exp = totalScienceExp
        end
    else
        science.exp = science.exp + scienceExp
    end
    if science.level >= maxLevel then
        science.level = maxlevel
        science.exp = 0
    end
    if groupTpl[science.level] then
        science.tplId = groupTpl[science.level].id
    end
    -- print('impl.science_donate22222...groupId, level, exp, tplId', groupId, science.level, science.exp, science.tplId)
    --联盟活跃值
    al.toatlActive = al.toatlActive + activeCount
    al.isDirty = true
    al.isSync = false

    --public to fs
    local up = science.level - oldScienceLevel
    publishScienceUpdate(al.id, science, up)
    publishAllianceUpdate(al, allianceUp)
    publishMemberUpdate(al.id, myself) 

    --联盟日记
    if levelup then
        local messageType = p.AllianceMessageType.SCIENCE_UPGRADE
        local param1 = tostring(science.tplId) .. ',' .. tostring(science.level)
        M.addAllianceRecord(aid, messageType, param1)
        local scienceList2Net = simpleScienceList2Net(al.scienceList)
        mapStub.cast_alliance_science_update(aid, scienceList2Net)
    end
    if allianceUp then
        local messageType = p.AllianceMessageType.ALLIANCE_UPGRADE
        local param1 = tostring(al.level)
        M.addAllianceRecord(aid, messageType, param1)

        --mail
        local params = {
            allianceName = al.name,
            allianceNickname = al.nickname,
            bannerId = al.bannerId,
            alliesLevel = al.level,
            leaderName = al.leaderName,
            alliesCount = al.alliesCount,
            alliesMax = al.alliesMax,
            alliesPower = al.power,
        }
        params.params1 = tostring(al.level)
        for _, v in pairs(al.memberList) do
            local mail = pubMail.newMailInfo()
            mail.uid = v.uid
            mail.otherUid = 0
            mail.type = p.MailType.SYSTEM
            mail.subType = p.MailSubType.SYSTEM_ALLIANCE_LEVEL_UP
            mail.createTime = timer.getTimestampCache()
            mail.title, mail.content = pubMail.getTitleContentBySubType(mail.subType)
            mail.attachment = '{}'
            mail.params = utils.serialize(params)
            mail.isLang = true
            mail.isRead = false
            mail.isDraw = true
            dataService.appendMailInfo(mail)
        end

        achievementService.onAllianceUpdate(oldAlLevel, al.level)
    end

    -- need sync to ms
    local simple = alliance2SimpleNet(al)
    mapStub.cast_alliance_update(simple)

    return p.ErrorCode.SUCCESS
end

function impl.help_request(aid, uid, helpInfo)
    -- print('impl.help_request...aid, uid, helpInfo', aid, uid, utils.serialize(helpInfo))
    --联盟是否存在
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return p.ErrorCode.ALLIANCE_NOT_EXIST
    end
    --是否是联盟成员
    local myself = al.memberList[uid]
    if myself == nil then
        return p.ErrorCode.ALLIANCE_NOT_MEMBER
    end
    --
    if helpInfo == nil then
        return p.ErrorCode.ALLIANCE_HELP_REQUEST_INVALID
    end
    --
    local aHelps = allianceHelps[aid]
    if aHelps == nil then
        aHelps = {}
        allianceHelps[aid] = aHelps
    end
    local help = {}
    help.uid = helpInfo.uid
    help.helpId = genAllianceHelpId()
    help.buildingId = helpInfo.buildingId
    help.times = 0
    help.accept = 0
    help.max = helpInfo.max
    help.reduceTime = helpInfo.reduceTime
    help.cdId = helpInfo.cdId
    help.cdTime = helpInfo.cdTime
    help.helpUserList = {}
    help.helpDesc = helpInfo.helpDesc
    help.isDirty = true
    aHelps[help.helpId] = help

    -- print('impl.help_request...aid, helps', aid, utils.serialize(aHelps))

    -- update to fs
    publishHelpAdd(aid, help)
    return p.ErrorCode.SUCCESS
end

function impl.help_one(aid, uid, nickname, helpId)
    -- print('impl.help_one...aid, uid, nickname, helpId', aid, uid, nickname, helpId)
    --联盟是否存在
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return p.ErrorCode.ALLIANCE_NOT_EXIST
    end
    --是否是联盟成员
    local myself = al.memberList[uid]
    if myself == nil then
        return p.ErrorCode.ALLIANCE_NOT_MEMBER
    end
    --是否可以帮助
    local aHelps = allianceHelps[aid]
    if aHelps == nil then
        return p.ErrorCode.ALLIANCE_HELP_NOT_FOUND
    end
    local help = aHelps[helpId]
    if help == nil
        or help.max <= help.times
        or help.cdTime <= timer.getTimestampCache() then
        return p.ErrorCode.ALLIANCE_HELP_NOT_FOUND
    end
    --是否重复帮助
    if help.helpUserList[uid] then
        return p.ErrorCode.ALLIANCE_HELP_NOT_FOUND
    end

    help.isDirty = true
    help.cdTime = help.cdTime - help.reduceTime
    help.times = help.times + 1
    help.helpUserList[uid] = 1

    --增加活跃度
    myself.activeWeek = myself.activeWeek + helpActiveAdd
    myself.isDirty = true
    myself.isSync = false
    al.toatlActive = al.toatlActive + helpActiveAdd
    al.isDirty = true
    al.isSync = false

    -- publish to fs
    publishHelpOne(aid, uid, help)
    publishAllianceUpdate(al)
    publishMemberUpdate(al.id, myself)

    return p.ErrorCode.SUCCESS
end

function impl.help_all(aid, uid, nickname, helpIds)
    -- print('impl.help_all...aid, uid, nickname, helpIds', aid, uid, nickname, utils.serialize(helpIds))
    --联盟是否存在
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return p.ErrorCode.ALLIANCE_NOT_EXIST
    end
    --是否是联盟成员
    local myself = al.memberList[uid]
    if myself == nil then
        return p.ErrorCode.ALLIANCE_NOT_MEMBER
    end
    --是否可以帮助
    local aHelps = allianceHelps[aid]
    if aHelps == nil then
        return p.ErrorCode.ALLIANCE_HELP_NOT_FOUND
    end
    if helpIds == nil then
        return p.ErrorCode.ALLIANCE_HELP_NOT_FOUND
    end

    local helpCount = 0
    local helps = {}
    for _, v in pairs(helpIds) do
        local isGave = false
        local help = aHelps[v.helpId]
        if help == nil
            or help.max <= help.times
            or help.cdTime <= timer.getTimestampCache() then
            isGave = true
        end
        if help and help.helpUserList[uid] then
            isGave = true
        end
        if not isGave then
            help.isDirty = true
            help.cdTime = help.cdTime - help.reduceTime
            help.times = help.times + 1
            help.helpUserList[uid] = 1

            table.insert(helps, help)
            helpCount = helpCount + 1
        end
    end
    if helpCount <= 0 then
        return p.ErrorCode.ALLIANCE_HELP_NOT_FOUND
    end
    --增加活跃度
    local activeCount = helpCount * helpActiveAdd
    myself.activeWeek = myself.activeWeek + activeCount
    myself.isDirty = true
    myself.isSync = false
    al.toatlActive = al.toatlActive + activeCount
    al.isDirty = true
    al.isSync = false

    -- publish to fs
    publishHelpAll(aid, uid, helps)
    publishAllianceUpdate(al)
    publishMemberUpdate(al.id, myself)

    return p.ErrorCode.SUCCESS, helpCount
end

function impl.help_accept(aid, uid, helpId)
    -- print('impl.help_accept...aid, uid, helpId', aid, uid, helpId)
    --联盟是否存在
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return
    end
    --是否可以帮助
    local aHelps = allianceHelps[aid]
    if aHelps == nil then
        return
    end
    local help = aHelps[helpId]
    if help == nil
        or help.accept == help.times
        or help.cdTime <= timer.getTimestampCache() then
        return
    end
    help.isDirty = true
    help.accept = help.times

    -- publish to fs
    publishHelpAccept(aid, uid, help)
end

function impl.help_close(aid, uid, helpIds)
    -- print('impl.help_close...aid, uid, helpIds', aid, uid, utils.serialize(helpIds))
    --联盟是否存在
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return
    end
    --
    local aHelps = allianceHelps[aid]
    if aHelps == nil then
        return
    end
    --
    if helpIds == nil or not next(helpIds) then
        return
    end
    for _, helpId in pairs(helpIds) do
        local help = aHelps[helpId]
        local bUpdate = true
        if help == nil or help.uid ~= uid then
            -- print('impl.help_close ... not fund help, aid, uid, helpId', aid, uid, helpId)
            bUpdate = false
        end
        if bUpdate then
            help.isDirty = true
            help.cdTime = 0
            help.accept = help.max
        end
    end

    publishHelpClose(aid, uid, helpIds)
end

function impl.user_hire_hero(aid, uid, nickname, heroInfo)
    -- print('impl.user_hire_hero....aid, uid, nickname, heroInfo', aid, uid, nickname, heroInfo)
    rawService:disableAutoReply()
    local session = rawService:session()
    local ret = p.ErrorCode.SUCCESS
    repeat
        --联盟是否存在
        local al = alliances[aid]
        if al == nil or al.disbandTime ~= 0 then
            ret = p.ErrorCode.ALLIANCE_NOT_EXIST
            break
        end
        --是否是联盟成员
        local myself = al.memberList[uid]
        if myself == nil then
            ret = p.ErrorCode.ALLIANCE_NOT_MEMBER
            break
        end
        --
        if heroInfo == nil then
            ret = p.ErrorCode.HERO_NOT_EXIST
            break
        end
        -- print('impl.user_hire_hero.....11111..', utils.serialize(allianceHeroLeases[aid]))
        local alease = allianceHeroLeases[aid]
        if alease == nil then
            alease = {}
            allianceHeroLeases[aid] = alease
        end
        if alease[uid] == nil then
            alease[uid] = {}
            alease[uid].incomeCount = 0
            alease[uid].list = {}
        end
        --已出租
        if alease[uid].list[heroInfo.id] then
            ret = p.ErrorCode.HERO_HAD_HIRE
            break
        end
        --出租武将数量上限
        local count = 0
        for _, v in pairs(alease[uid].list) do
            count = count + 1
        end
        
        if count >= HERO_LEASE_MAX then
            print('=========impl.user_hire_hero...count, alease[uid].list', count, utils.serialize(alease[uid].list))
            ret = p.ErrorCode.HERO_HAD_HIRE
            break
        end

        local leaseInfo = {}
        leaseInfo.incomeRentSilver = 0
        leaseInfo.incomeHookSilver = 0
        leaseInfo.hookSilver = 0
        local now = timer.getTimestampCache()
        leaseInfo.canRecall = true
        leaseInfo.leaseBeginTime = now
        leaseInfo.hookBeginTime = now
        local info = pubHero.newHeroInfo(heroInfo)
        leaseInfo.heroInfo = info
        --价格=100*等级*星级*品阶
        local rentSilver = alliance_hero_lease.base * info.level * info.star * info.tpl.quality
        leaseInfo.rentSilver = rentSilver
        leaseInfo.rentCount = 0
        leaseInfo.todayRentCount = 0
        leaseInfo.newRentCount = 0
        alease[uid].isDirty = true
        alease[uid].list[info.id] = leaseInfo
        -- print('impl.user_hire_hero.....22222..', utils.serialize(allianceHeroLeases[aid]))

        -- update to fs
        publishHireHeroUpdate(aid, uid, leaseInfo)
    until true
    rawService:reply(session, ret)
end

function impl.user_recall_hero(aid, uid, heroId)
    -- print('impl.user_recall_hero....aid, uid, heroId', aid, uid, heroId)
    rawService:disableAutoReply()
    local session = rawService:session()
    local ret = p.ErrorCode.SUCCESS
    local collectSilver = 0
    local todayRentCount = 0
    local newRentCount = 0
    local timeSilver = 0
    repeat
        --联盟是否存在
        local al = alliances[aid]
        if al == nil or al.disbandTime ~= 0 then
            ret = p.ErrorCode.ALLIANCE_NOT_EXIST
            break
        end
        --是否是联盟成员
        local myself = al.memberList[uid]
        if myself == nil then
            ret = p.ErrorCode.ALLIANCE_NOT_MEMBER
            break
        end
        --检查武将租借列表
        -- print('impl.user_recall_hero.....11111..', utils.serialize(allianceHeroLeases[aid]))
        local alease = allianceHeroLeases[aid]
        if alease == nil then
            ret = p.ErrorCode.HERO_NOT_EXIST
            break
        end
        if alease[uid] == nil or alease[uid].list == nil then
            ret = p.ErrorCode.HERO_NOT_EXIST
            break
        end
        if alease[uid].list[heroId] == nil then
            ret = p.ErrorCode.HERO_NOT_EXIST
            break
        end
        --召回有时间限制
        local endTimestamp = alease[uid].list[heroId].leaseBeginTime + userRecalHeroInterval
        local now = timer.getTimestampCache()
        if now < endTimestamp then
            ret = p.ErrorCode.RECAL_HERO_TIME_NOT_OVER
            break
        end
        --收益
        collectSilver, todayRentCount, newRentCount, timeSilver = M.collectHeroHireCostOne(aid, uid, heroId)
        alease[uid].isDirty = true
        alease[uid].list[heroId] = nil
        -- print('impl.user_recall_hero.....22222..', utils.serialize(allianceHeroLeases[aid]))

        -- update to fs
        local heroIds = {}
        table.insert(heroIds, { heroId = heroId })
        publishHireHeroRemove(aid, uid, heroIds)
        if collectSilver > 0 or timeSilver > 0 then
            publishCollectHeroHireCostUpdate(aid, uid, collectSilver, todayRentCount, newRentCount, timeSilver)
        end

        --出租记录
        M.removeAllianceHeroleaseRecord(aid, uid, true)
    until true

    rawService:reply(session, ret, collectSilver + timeSilver)
end

function impl.user_employ_hero(aid, uid, ownerUid, heroId)
    -- print('impl.user_employ_hero...aid, uid, ownerUid, heroId', aid, uid, ownerUid, heroId)
    rawService:disableAutoReply()
    local session = rawService:session()
    local ret = p.ErrorCode.SUCCESS
    repeat
        --联盟是否存在
        local al = alliances[aid]
        if al == nil or al.disbandTime ~= 0 then
            ret = p.ErrorCode.ALLIANCE_NOT_EXIST
            break
        end
        --是否是联盟成员
        local myself = al.memberList[uid]
        if myself == nil then
            ret = p.ErrorCode.ALLIANCE_NOT_MEMBER
            break
        end
        local ownerMember = al.memberList[ownerUid]
        if ownerMember == nil then
            ret = p.ErrorCode.ALLIANCE_NOT_MEMBER
            break
        end
        --检查武将租借列表
        if allianceHeroLeases[aid] == nil then
            ret = p.ErrorCode.HERO_NOT_EXIST
            break
        end
        local ulease = allianceHeroLeases[aid][ownerUid]
        if ulease == nil or ulease.list == nil then
            ret = p.ErrorCode.HERO_NOT_EXIST
            break
        end
        local leaseInfo = ulease.list[heroId]
        if leaseInfo == nil then
            ret = p.ErrorCode.HERO_NOT_EXIST
            break
        end
        local heroInfo = leaseInfo.heroInfo
        if heroInfo == nil or next(heroInfo) == nil then
            ret = p.ErrorCode.HERO_NOT_EXIST
            break
        end
        --收益
        local incomeCount = ulease.incomeCount or 0
        local employSilver = math.floor(leaseInfo.rentSilver * alliance_hero_lease.coe)
        if incomeCount < alliance_hero_lease.count then
            leaseInfo.incomeRentSilver = leaseInfo.incomeRentSilver + employSilver
            leaseInfo.rentCount = leaseInfo.rentCount + 1
            leaseInfo.todayRentCount = leaseInfo.todayRentCount + 1
            leaseInfo.newRentCount = leaseInfo.newRentCount + 1

            ulease.incomeCount = incomeCount + 1
            ulease.isDirty = true
            --public to fs
            publishHireHeroUpdate(aid, ownerUid, leaseInfo)
        end
        --出租记录
        local employId = genAllianceHelpId()
        local leaseRecordInfo = {
            employId = employId,
            heroId = heroId,
            uid = uid,
            nickname = myself.nickname,
            headId = myself.headId,
            userLevel = myself.userLevel,
            vipLevel = myself.vipLevel,
            rentSilver = employSilver,
            createTime = timer.getTimestampCache(),
        }
        M.addAllianceHeroleaseRecord(aid, ownerUid, leaseRecordInfo)

        -- update to fs
        publishEmployHeroUpdate(aid, uid, ownerUid, ownerMember.nickname, heroInfo)
    until true

    rawService:reply(session, ret)
end

function impl.collect_hero_hire_cost(aid, uid)
    -- print('impl.collect_hero_hire_cost...aid, uid', aid, uid)
    local ret = p.ErrorCode.SUCCESS
    local collectSilver = 0
    local todayRentCount = 0
    local newRentCount = 0
    repeat
        --联盟是否存在
        local al = alliances[aid]
        if al == nil or al.disbandTime ~= 0 then
            ret = p.ErrorCode.ALLIANCE_NOT_EXIST
            break
        end
        --是否是联盟成员
        local myself = al.memberList[uid]
        if myself == nil then
            ret =  p.ErrorCode.ALLIANCE_NOT_MEMBER
            break
        end
        --检查武将租借列表
        if allianceHeroLeases[aid] == nil then
            ret =  p.ErrorCode.HERO_NOT_EXIST
            break
        end
        local ulease = allianceHeroLeases[aid][uid]
        if ulease == nil or ulease.list == nil then
            ret =  p.ErrorCode.HERO_NOT_EXIST
            break
        end
        --收益
        for _, leaseInfo in pairs(ulease.list) do
            if leaseInfo.incomeRentSilver > 0 then
                collectSilver = collectSilver + leaseInfo.incomeRentSilver
                todayRentCount = leaseInfo.todayRentCount
                newRentCount = leaseInfo.newRentCount
                leaseInfo.incomeRentSilver = 0
                leaseInfo.newRentCount = 0
                ulease.isDirty = true
            end
        end
        if collectSilver > 0 then
            --update to fs
            publishCollectHeroHireCostUpdate(aid, uid, collectSilver, todayRentCount, newRentCount, 0)
            --出租记录
            M.removeAllianceHeroleaseRecord(aid, uid, true)
        end
    until true

    return ret, collectSilver
end

function impl.member_update(aid, uid, nickname, headId, userLevel, vipLevel, totalPower, castleLevel, x, y, marketIsOpen)
    --联盟是否存在
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return
    end
    --是否是联盟成员
    local myself = al.memberList[uid]
    if myself == nil then
        return
    end

    myself.nickname = nickname
    myself.headId = headId
    myself.userLevel = userLevel
    myself.vipLevel = vipLevel
    myself.totalPower = totalPower
    myself.castleLevel = castleLevel
    myself.x = x
    myself.y = y
    myself.marketIsOpen = marketIsOpen
    myself.isDirty = true
    myself.isSync = false

    local bUpdateAlliance = false
    if uid == al.leaderId then
        al.leaderName = nickname
        al.isDirty = true
        al.isSync = false
        bUpdateAlliance = true
    end

    --update to fs
    publishMemberUpdate(al.id, myself)
    if bUpdateAlliance then
        publishAllianceUpdate(al)
        -- need sync to ms
        local simple = alliance2SimpleNet(al)
        mapStub.cast_alliance_update(simple)
    end
end

function impl.alliance_buff_open(aid, uid, buffId)
    -- print('impl.alliance_buff_open...aid, uid, buffId', aid, uid, buffId)
    --联盟是否存在
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return p.ErrorCode.ALLIANCE_NOT_EXIST
    end
    --是否有权限
    local myself = al.memberList[uid]
    if myself == nil or not myself:haveRankPermit(rankPermitType.BUFF) then
        return p.ErrorCode.ALLIANCE_WITHOUT_PERMIT
    end
    --buff是否存在
    local tpl = t.allianceBuff[buffId]
    if tpl == nil then
        return p.ErrorCode.ALLIANCE_BUFF_NOT_EXIST
    end
    --是否可以激活buff
    if al.level < tpl.allianceLevel then
        return p.ErrorCode.ALLIANCE_BUFF_CANNOT_OPEN
    end
    --是否还有其他已经激活的buff
    local now = timer.getTimestampCache()
    for _, v in pairs(al.allianceBuffList) do
        if now < v.endTimestamp then
            return p.ErrorCode.ALLIANCE_BUFF_CANNOT_OPENMULTI
        end
    end
    --
    local allianceBuff = al:findAllianceBuff(buffId)
    if allianceBuff == nil then
        allianceBuff = allianceInfo.newAllianceBuff({ buffId = buffId })
        al:addAllianceBuff(allianceBuff)
    end
    --激活次数是否已达上限
    if allianceBuff.useCount >= tpl.useMax then
        return p.ErrorCode.ALLIANCE_BUFF_USE_MAX
    end
    --
    allianceBuff.createTimestamp = now
    allianceBuff.endTimestamp = now + allianceBuff.tpl.spanTime
    allianceBuff.useCount = allianceBuff.useCount + 1
    al.isDirty = true
    al.isSync = false

    --update to fs
    publishAllianceBuffOpen(aid, allianceBuff)

    --联盟日记
    local messageType = p.AllianceMessageType.OPEN_ALLIANCE_BUFF
    local param1 = allianceBuff.tpl.buffName
    M.addAllianceRecord(aid, messageType, param1)

    --sync to ms
    local tempBuff = allianceBuff2SimpleNet(allianceBuff)
    mapStub.cast_alliance_buff_open(aid, tempBuff)

    return p.ErrorCode.SUCCESS
end

function impl.alliance_buff_closed(aid, uid, buffId)
    -- print('impl.alliance_buff_closed...aid, uid, buffId', aid, uid, buffId)
    --联盟是否存在
    local al = alliances[aid]
    if al == nil or al.disbandTime ~= 0 then
        return p.ErrorCode.ALLIANCE_NOT_EXIST
    end
    --是否有权限
    local myself = al.memberList[uid]
    if myself == nil or not myself:haveRankPermit(rankPermitType.BUFF) then
        return p.ErrorCode.ALLIANCE_WITHOUT_PERMIT
    end
    --
    local allianceBuff = al:findAllianceBuff(buffId)
    if allianceBuff then
        allianceBuff.createTimestamp = 0
        allianceBuff.endTimestamp = 0
        al.isDirty = true
        al.isSync = false
    else
        return p.ErrorCode.ALLIANCE_BUFF_NOT_EXIST
    end

    --update to fs
    publishAllianceBuffClosed(aid, buffId)
    --sync to ms
    mapStub.cast_alliance_buff_closed(aid, buffId)

    return p.ErrorCode.SUCCESS
end

--
-- timer task
--
local function allianceActiveRewordSend()
    for _, al in pairs(alliances) do
        if al.disbandTime == 0 then
            local sortList = {}
            for _, v in pairs(al.memberList) do
                table.insert(sortList, { uid = v.uid, activeWeek = v.activeWeek, joinTime = v.joinTime, nickname = v.nickname })
            end
            --先活跃度排名  先活跃度从大到小排列，相同再按入盟时间排序
            table.sort(sortList, function(left, right)
                if left.activeWeek == right.activeWeek then
                    return left.joinTime < right.joinTime
                else
                    return left.activeWeek > right.activeWeek
                end
            end)
            --邮件奖励 前3名
            local params = {
                allianceName = al.name,
                allianceNickname = al.nickname,
                bannerId = al.bannerId,
                alliesLevel = al.level,
                leaderName = al.leaderName,
                alliesCount = al.alliesCount,
                alliesMax = al.alliesMax,
                alliesPower = al.power,
            }
            --个人活跃度奖励
            --allianceActiveReword={{[5]=100,[10]=5000,[6]=100000},{[5]=50,[10]=3000,[6]=50000},{[5]=30,[10]=2000,[6]=300000}}
            local rankDetail = {}
            for k, v in ipairs(sortList) do
                if k <= 3 then
                    local drops = {}
                    local rewrd = allianceActiveReword[k]
                    if rewrd then
                        for tplId, count in pairs(rewrd) do
                            table.insert(drops, { tplId = tplId, count = count })
                        end
                    end

                    -- params.rank = k
                    table.insert(rankDetail, { nickname = v.nickname, activeWeek = v.activeWeek, rank = k, drops = drops })

                    params.params1 = tostring(k)
                    local mail = pubMail.newMailInfo()
                    mail.uid = v.uid
                    mail.otherUid = 0
                    mail.type = p.MailType.SYSTEM
                    mail.subType = p.MailSubType.SYSTEM_ALLIANCE_REWARD
                    mail.createTime = timer.getTimestampCache()
                    mail.title, mail.content = pubMail.getTitleContentBySubType(mail.subType)
                    mail.attachment = utils.serialize(drops)
                    mail.params = utils.serialize(params)
                    mail.isLang = true
                    mail.isRead = false
                    mail.isDraw = false
                    dataService.appendMailInfo(mail)
                else
                    break
                end
            end
            --联盟活跃度奖励详细说明
            -- params.rank = nil
            -- params.rankDetail = rankDetail

            local detailParams1 = nil
            for i=1,3 do
                local d = rankDetail[i]
                if d then
                    if detailParams1 == nil then
                        detailParams1 = d.nickname
                    else
                        detailParams1 = detailParams1 .. ',' .. d.nickname
                    end
                else
                    detailParams1 = detailParams1 .. ','
                end
            end

            params.params1 = detailParams1
            for _, v in pairs(al.memberList) do
                local mail = pubMail.newMailInfo()
                mail.uid = v.uid
                mail.otherUid = 0
                mail.type = p.MailType.SYSTEM
                mail.subType = p.MailSubType.SYSTEM_ALLIANCE_REWARD_DETAIL
                mail.createTime = timer.getTimestampCache()
                mail.title, mail.content = pubMail.getTitleContentBySubType(mail.subType)
                mail.attachment = utils.serialize({})
                mail.params = utils.serialize(params)
                mail.isLang = true
                mail.isRead = false
                mail.isDraw = true
                dataService.appendMailInfo(mail)
            end
        end
    end
    --活跃度清零
    for _, al in pairs(alliances) do
        if al.disbandTime == 0 then
            for _, v in pairs(al.memberList) do
                if v.activeWeek > 0 then
                    v.activeWeek = 0
                    v.isDirty = true
                    v.isSync = false
                    publishMemberUpdate(al.id, v)
                end
            end
        end
        if al.toatlActive > 0 then
            al.toatlActive = 0
            al.isDirty = true
            al.isSync = false
            publishAllianceUpdate(al)
        end
    end
end

timerUpdate = function ()
    local now = timer.getTimestampCache()
    if allianceHeroLeaseData.last5RefreshTime == 0 then
        allianceHeroLeaseData.last5RefreshTime = now
        allianceHeroLeaseData.isDirty = true
    end
    if timer.isTsShouldRefreshAtHour(allianceHeroLeaseData.last5RefreshTime, 5) then
        -- print('allianceHeroLeaseData update...last5RefreshTime', allianceHeroLeaseData.last5RefreshTime)
        allianceHeroLeaseData.last5RefreshTime = timer.getTimestampCache()
        allianceHeroLeaseData.isDirty = true
        --武将租借
        for aid, alease in pairs(allianceHeroLeases) do
            for uid, ulease in pairs(alease) do
                ulease.incomeCount = 0
                ulease.todayRentCount = 0
                ulease.isDirty = true
            end
        end

        for _, al in pairs(alliances) do
            for _, v in pairs(al.memberList) do
                if v.sendMailCount > 0 then
                    v.sendMailCount = 0
                    v.isSync = false
                    v.isDirty = true
                    publishMemberUpdate(al.id, v)
                end
            end
            --联盟buff
            for _, buff in pairs(al.allianceBuffList) do
                buff.useCount = 0
                buff.isSync = false
                buff.isDirty = true
            end
            al.isSync = false
            al.isDirty = true
            publishAllianceBuffUpdate(al.id, al.allianceBuffList)
        end
        --活跃度  每周1凌晨5点活跃度清零并结算
        local s = os.date('%w', now)
        if s == '1' then
            allianceActiveRewordSend()
        end
    end

    if now % 300 == 0 then
        -- print('alliance timerUpdate 5 minutes  job ...')
        M.onSave()

        for _, al in pairs(alliances) do
            if not al.isSync then
                al.isSync = true
                publishAllianceUpdate(al)
            end
        end
    end

    -- -- test
    -- if now % 30 == 0 then
    --     allianceActiveRewordSend()
    -- end
end

return M