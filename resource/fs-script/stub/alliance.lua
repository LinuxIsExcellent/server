local cluster = require('cluster')
local timer = require('timer')
local utils = require('utils')
local allianceInfo = require('alliance')
local pubHero = require('hero')
local p = require('protocol')
local t = require('tploader')
local misc = require('libs/misc')
local loginStub = require('stub/login')
local event = require('libs/event')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()

local ALLIANCE_RECORD_MAX = 20         -- 联盟记录数量上限
local HERO_LEASE_RECORD_MAX = 10    --出租记录数量
local ALLIANCE_INVITE_MAX = 20

local allianceStub = {
    rawStub,
    --联盟帮助
    allianceHelps = {}, -- [aid][helpId] = helpInfo
    allianceInviteList = {}, --[uid]={aid,....}
    allianceHeroLeases = {}, --[aid][uid][heroId] = leaseInfo
    allianceRecords = {},   -- 联盟记录[aid] = {recordInfo, ...}
    allianceHeroleaseRecords = {},   --租借记录[uid] = {leaseRecordInfo, ...}
    allAllianceCity = {}, --[aid]={cityId, ...}
    uselessBuffCity = {},  --[aid]={cityId, ...}  上一级名城数量不足，导致下一级失去一个名城属性的加成
}

local alliances = {} --所有联盟信息 <aid, <allianceInfo>>

local subscriber = {
    --
    alliance_update = function (allies, allianceUp)
        -- print('alliance subscriber alliance_update...allies, allianceUp', utils.serialize(allies), allianceUp)
        if allies then
            local al = alliances[allies.id]
            --add alliance
            if al == nil then
                alliances[allies.id] = allianceInfo.info:new(allies)
                return
            end
            --update alliance
            al.id = allies.id
            al.name = allies.name
            al.nickname = allies.nickname
            al.level = allies.level
            al.exp = allies.exp
            al.bannerId = allies.bannerId
            al.power = allies.power
            al.leaderId = allies.leaderId
            al.leaderName = allies.leaderName
            al.leaderHeadId = allies.leaderHeadId

            al.alliesCount = allies.alliesCount
            al.alliesMax = allies.alliesMax
            al.towerCount = allies.towerCount
            al.towerMax = allies.towerMax
            al.castleCount = allies.castleCount
            al.castleMax = allies.castleMax

            al.toatlActive = allies.toatlActive
            al.announcement = allies.announcement
            al.openRecruitment = allies.openRecruitment

            al.changeBannerCdTime = allies.changeBannerCdTime

            -- update to client
            allianceStub.updateAlliance(al.id, allianceUp)
        end
    end,

    member_update = function(aid, member, joinType)
        -- print('alliance subscriber member_update...aid, member', aid, utils.serialize(member))
        if member == nil then
            return
        end
        local al = alliances[aid]
        if al == nil then
            return
        end
        local m = al.memberList[member.uid]
        if m == nil then
            --add member
            m = allianceInfo.member:new(member)
            al:addMember(m)

            allianceStub.updateMember(aid, m, joinType)
            --联盟成员数量有变化
            allianceStub.updateAlliance(al.id, false)
        else
            --update member
            m.name = member.name
            m.nickname = member.nickname
            m.headId = member.headId
            m.userLevel = member.userLevel
            m.vipLevel = member.vipLevel
            m.rankLevel = member.rankLevel
            m.activeWeek = member.activeWeek
            m.contribution = member.contribution
            m.sendMailCount = member.sendMailCount
            m.totalPower = member.totalPower
            m.castleLevel = member.castleLevel
            m.x = member.x
            m.y = member.y
            m.marketIsOpen = member.marketIsOpen
            m.lastOnlineTime = member.lastOnlineTime
            m.joinTime = member.joinTime

            allianceStub.updateMember(aid, m, joinType)
        end
    end,

    member_delete = function(aid, uid, allianceCdTimestamp, isKick)
        -- print('alliance subscriber member_delete...aid, uid, isKick', aid, uid, isKick)
        if aid == 0 or uid == 0 then
            return
        end
        local al = alliances[aid]
        if al == nil then
            return
        end
        al:removeMember(uid)

        -- update to client
        allianceStub.deleteMember(aid, uid, allianceCdTimestamp, isKick)
    end,

    member_online = function(aid, uid)
        -- print('alliance subscriber member_online...aid, uid', aid, uid)
        if aid == 0 or uid == 0 then
            return
        end
        local al = alliances[aid]
        if al == nil then
            return
        end
        local m = al.memberList[uid]
        if m ~= nil then
            m.lastOnlineTime = 0
            -- update to client
            allianceStub.updateMember(aid, m)
        end
    end,

    member_offline = function(aid, uid, offlineTime)
        -- print('alliance subscriber member_offline...aid, uid, offlineTime', aid, uid, offlineTime)
        if aid == 0 or uid == 0 then
            return
        end
        local al = alliances[aid]
        if al == nil then
            return
        end
        local m = al.memberList[uid]
        if m ~= nil then
            m.lastOnlineTime = offlineTime
            -- update to client
            allianceStub.updateMember(aid, m)
        end
    end,

    apply_update = function(aid, uid, nickname, headId, userLevel, vipLevel, time)
        -- print('alliance subscriber apply_update...aid, uid, nickname, headId, userLevel, vipLevel, time', aid, uid, nickname, headId, userLevel, vipLevel, time)
        if aid == 0 or uid == 0 then
            return
        end
        local al = alliances[aid]
        if al == nil or al.disbandTime ~= 0 then
            return
        end
        local deletes = al:addApply(uid, nickname, headId, userLevel, vipLevel, time)
        -- update to client
        local update = {}
        table.insert(update, { uid = uid, nickname = nickname, headId = headId, userLevel = userLevel, vipLevel = vipLevel })
        allianceStub.updateApply(aid, update, deletes)
    end,

    apply_remove = function(aid, uid)
        -- print('alliance subscriber apply_remove...aid, uid', aid, uid)
        if aid == 0 or uid == 0 then
            return
        end
        local al = alliances[aid]
        if al == nil or al.disbandTime ~= 0 then
            return
        end
        al:removeApply(uid)

        -- update to client
        local deletes = {}
        table.insert(deletes, { uid = uid })
        allianceStub.updateApply(aid, {}, deletes)
    end,

    invited_update = function(aid, uid)
        -- print('alliance subscriber invited_update...aid, uid', aid, uid)
        if aid == 0 or uid == 0 then
            return
        end
        local al = alliances[aid]
        if al == nil or al.disbandTime ~= 0 then
            return
        end

        local uList = allianceStub.allianceInviteList[uid]
        -- print('==================invited_update 111 uList', utils.serialize(uList))
        if uList == nil then
            allianceStub.allianceInviteList[uid] = {}
            uList = allianceStub.allianceInviteList[uid]
        end
        for _, v in pairs(uList) do
            if v == aid then
                return
            end
        end
        table.insert(uList, 1, aid)
        local oldAid = uList[ALLIANCE_INVITE_MAX + 1] or 0
        uList[ALLIANCE_INVITE_MAX + 1] = nil
        -- print('==================invited_update 222 uList', utils.serialize(uList))

        allianceStub.updateInvited(aid, uid)
        if oldAid > 0 then
            allianceStub.removeInvited(oldAid, uid)
        end
    end,

    invited_remove = function (aid, uid)
        -- print('alliance subscriber invited_remove...aid, uid', aid, uid)
        if aid == 0 or uid == 0 then
            return
        end
        local al = alliances[aid]
        if al == nil or al.disbandTime ~= 0 then
            return
        end
        local uList = allianceStub.allianceInviteList[uid]
        -- print('==================invited_remove 111 uList', utils.serialize(uList))
        local aidList = {}
        if uList then
            for k, v in pairs(uList) do
                if v == aid then
                    table.remove(uList, k)
                    break
                end
            end
            allianceStub.removeInvited(aid, uid)
        end
        -- print('==================invited_remove 222 uList', utils.serialize(uList))
    end,

    invited_delete = function(uid)
        -- print('alliance subscriber invited_delete...uid', uid)
        allianceStub.allianceInviteList[uid] = nil
    end,

    science_update = function (aid, science, uplevel)
        -- print('alliance subscriber science_update...aid, science, uplevel', aid, utils.serialize(science), uplevel)
        if aid == 0 or science == nil then
            return
        end
        local al = alliances[aid]
        if al == nil then
            return
        end
        local s = al:findScience(science.groupId)
        if s == nil then
            s = allianceInfo.science:new(science)
            al:addScience(s)
        else
            s.tplId = science.tplId
            s.level = science.level
            s.exp = science.exp
        end
        allianceStub.updateScience(aid, s, uplevel)
    end,

    help_add = function(aid, help)
        -- print('alliance subscriber help_add', aid, help)
        if aid == 0 or help == nil then
            return
        end
        local a = alliances[aid]
        if a == nil or a.disbandTime ~= 0 then
            return
        end
        local aHelps = allianceStub.allianceHelps[aid]
        if aHelps == nil then
            aHelps = {}
            allianceStub.allianceHelps[aid] = aHelps
        end
        aHelps[help.helpId] = help

        -- update to client
        allianceStub.updateHelpAdd(aid, help)
    end,

    help_one = function(aid, fromUid, help)
        -- print('alliance subscriber help_one...aid, fromUid, help', aid, fromUid, help)
        if aid == 0 or fromUid == 0 or help == nil then
            return
        end
        local a = alliances[aid]
        if a == nil or a.disbandTime ~= 0 then
            return
        end
        local aHelps = allianceStub.allianceHelps[aid]
        if aHelps == nil then
            return
        end
        local h = aHelps[help.helpId]
        if h ~= nil then
            aHelps[help.helpId] = help
        end

        -- update to client
        allianceStub.updateHelpOne(aid, fromUid, help)
    end,

    help_all = function(aid, fromUid, helps)
        -- print('alliance subscriber help_all..aid, fromUid, helps', aid, fromUid, helps)
        if aid == 0 or fromUid == 0 or helps == nil then
            return
        end
        local a = alliances[aid]
        if a == nil or a.disbandTime ~= 0 then
            return
        end
        local aHelps = allianceStub.allianceHelps[aid]
        if aHelps == nil then
            return
        end
        for _, v in pairs(helps) do
            local h = aHelps[v.helpId]
            if h ~= nil then
                aHelps[v.helpId] = v
            end
        end

        -- update to client
        allianceStub.updateHelpAll(aid, fromUid, helps)
    end,

    help_accept = function(aid, uid, help)
        -- print('alliance subscriber help_accept...aid, help', aid, help)
        if aid == 0 or help == nil then
            return
        end
        local a = alliances[aid]
        if a == nil or a.disbandTime ~= 0 then
            return
        end
        local aHelps = allianceStub.allianceHelps[aid]
        if aHelps == nil then
            return
        end
        local h = aHelps[help.helpId]
        if h ~= nil then
            aHelps[help.helpId] = help
        end

        -- update to client
        allianceStub.updateHelpAccept(aid, uid, help)
    end,

    help_close = function(aid, uid, helpIds)
        -- print('alliance subscriber help_close...aid, uid, helpIds', aid, uid, utils.serialize(helpIds))
        if aid == 0 or uid == 0 or helpIds == nil then
            return
        end
        local a = alliances[aid]
        if a == nil or a.disbandTime ~= 0 then
            return
        end
        local aHelps = allianceStub.allianceHelps[aid]
        if aHelps == nil then
            return
        end
        for _,helpId in pairs(helpIds) do
            aHelps[helpId] = nil
        end

        -- update to client
        allianceStub.updateHelpClose(aid, helpIds)
    end,

    hire_hero_update = function(aid, uid, leaseInfo)
        -- print('alliance subscriber hire_hero_update...aid, uid, leaseInfo', aid, uid, utils.serialize(leaseInfo))
        if leaseInfo == nil then
            return
        end
        local al = alliances[aid]
        if al == nil then
            return
        end
        local m = al.memberList[uid]
        if m == nil then
            return
        end
        --
        -- print('alliance subscriber hire_hero_update...allianceHeroLeases111', utils.serialize(allianceStub.allianceHeroLeases[aid]))
        local leases = allianceStub.allianceHeroLeases[aid]
        if leases == nil then
            leases = {}
            allianceStub.allianceHeroLeases[aid] = leases
        end
        if leases[uid] == nil then
            leases[uid] = {}
        end
        local heroId = leaseInfo.heroInfo.id
        if leases[uid][heroId] then
            leases[uid][heroId].incomeRentSilver = leaseInfo.incomeRentSilver
            leases[uid][heroId].incomeHookSilver = leaseInfo.incomeHookSilver
            leases[uid][heroId].rentSilver = leaseInfo.rentSilver
            leases[uid][heroId].canRecall = leaseInfo.canRecall
            leases[uid][heroId].leaseBeginTime = leaseInfo.leaseBeginTime
            leases[uid][heroId].hookBeginTime = leaseInfo.hookBeginTime
            leases[uid][heroId].rentCount = leaseInfo.rentCount
            leases[uid][heroId].todayRentCount = leaseInfo.todayRentCount
            leases[uid][heroId].newRentCount = leaseInfo.newRentCount
        else
            local info = {}
            info.incomeRentSilver = leaseInfo.incomeRentSilver
            info.incomeHookSilver = leaseInfo.incomeHookSilver
            info.rentSilver = leaseInfo.rentSilver
            info.canRecall = leaseInfo.canRecall
            info.leaseBeginTime = leaseInfo.leaseBeginTime
            info.hookBeginTime = leaseInfo.hookBeginTime
            info.rentCount = leaseInfo.rentCount
            info.todayRentCount = leaseInfo.todayRentCount
            info.newRentCount = leaseInfo.newRentCount

            local tempInfo = pubHero.newHeroInfo(leaseInfo.heroInfo)
            info.heroInfo = tempInfo
            leases[uid][heroId] = info

            --update to client
            local updates = {}
            local power = tempInfo:getSingleHeroPower()
            local rentSilver = leaseInfo.rentSilver
            local rentTime = leaseInfo.leaseBeginTime
            local rentCount = leaseInfo.rentCount
            local todayRentCount = leaseInfo.todayRentCount
            local newRentCount = leaseInfo.newRentCount

            table.insert(updates, {
                ownerUid = uid,
                ownerNickname = m.nickname,
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
            allianceStub.hireHeroUpdate(aid, uid, updates)
        end
        -- print('alliance subscriber hire_hero_update...allianceHeroLeases222', utils.serialize(allianceStub.allianceHeroLeases[aid]))
    end,

    hire_hero_remove = function(aid, uid, heroIds)
        -- print('alliance subscriber hire_hero_remove...aid, uid, heroIds', aid, uid, utils.serialize(heroIds))
        local al = alliances[aid]
        if al == nil then
            return
        end
        local m = al.memberList[uid]
        if m == nil then
            return
        end
        -- print('alliance subscriber hire_hero_remove...allianceHeroLeases111', utils.serialize(allianceStub.allianceHeroLeases[aid]))
        local leases = allianceStub.allianceHeroLeases[aid]
        if leases == nil then
            return
        end
        if leases[uid] == nil then
            return
        end
        for _, v in pairs(heroIds) do
            leases[uid][v.heroId] = nil
        end
        -- print('alliance subscriber hire_hero_remove...allianceHeroLeases222', utils.serialize(allianceStub.allianceHeroLeases[aid]))

        --update to client
        allianceStub.deleteHireHero(aid, uid, heroIds)
    end,

    employ_hero_update = function(aid, uid, ownerUid, ownerNickname, heroInfo)
        -- print('alliance subscriber employ_hero_update...aid, uid, ownerUid, ownerNickname, heroInfo', aid, uid, ownerUid, ownerNickname, utils.serialize(heroInfo))
        if heroInfo == nil then
            return
        end
        local al = alliances[aid]
        if al == nil then
            return
        end
        local myself = al.memberList[uid]
        if myself == nil then
            return
        end
        local leases = allianceStub.allianceHeroLeases[aid]
        if leases == nil then
            return
        end
        if leases[ownerUid] == nil then
            return
        end
        local heroId = heroInfo.id
        if leases[ownerUid][heroId] == nil then
            return
        end

        --update to client
        allianceStub.employHeroUpdate(aid, uid, ownerUid, ownerNickname, heroInfo)
    end,

    collect_hero_hire_cost = function(aid, uid, collectSilver, todayRentCount, newRentCount, timeSilver)
        -- print('alliance subscriber collect_hero_hire_cost...aid, uid, collectSilver', aid, uid, collectSilver)
        local al = alliances[aid]
        if al == nil then
            return
        end
        local m = al.memberList[uid]
        if m == nil then
            return
        end
        --
        if allianceStub.allianceHeroLeases[aid] then
            local ulease = allianceStub.allianceHeroLeases[aid][uid]
            if ulease then
                for _, v in pairs(ulease) do
                    v.incomeRentSilver = 0
                    v.newRentCount = 0
                end
            end
        end
        if collectSilver > 0 then
            allianceStub.collectHeroHireCostUpdate(aid, uid, collectSilver, todayRentCount, newRentCount, timeSilver)
        end
    end,

    alliance_record_update = function(aid, recordInfo)
        -- print('alliance subscriber alliance_record_update...aid, recordInfo', aid, recordInfo)
        if recordInfo == nil then
            return
        end
        local arecord = allianceStub.allianceRecords[aid]
        if arecord == nil then
            arecord = {}
            allianceStub.allianceRecords[aid] = arecord
        end
        table.insert(arecord, 1, recordInfo)
        while #arecord > ALLIANCE_RECORD_MAX do
            table.remove(arecord)
        end
        --update to client
        allianceStub.allianceRecordUpdate(aid, recordInfo)
    end,

    alliance_hero_lease_record_Add = function(aid, uid, leaseRecordInfo)
        -- print('alliance subscriber alliance_hero_lease_record_update...uid, leaseRecordInfo', uid, leaseRecordInfo)
        if leaseRecordInfo == nil then
            return
        end
        local arecord = allianceStub.allianceHeroleaseRecords[uid]
        if arecord == nil then
            arecord = {}
            allianceStub.allianceHeroleaseRecords[uid] = arecord
        end
        table.insert(arecord, 1, leaseRecordInfo)

        while #arecord > HERO_LEASE_RECORD_MAX do
            table.remove(arecord)
        end

        --update to client
        allianceStub.allianceHeroLeaseRecordAdd(aid, uid, leaseRecordInfo)
    end,

    alliance_hero_lease_record_remove = function(aid, uid)
        -- print('alliance subscriber alliance_hero_lease_record_remove...uid', uid)
        local arecord = allianceStub.allianceHeroleaseRecords[uid]
        if arecord then
            local deleteEmployIds = {}
            for _, v in pairs(arecord) do
                table.insert(deleteEmployIds, { employId = v. employId })
            end
            allianceStub.allianceHeroleaseRecords[uid] = nil
            --update to client
            allianceStub.allianceHeroLeaseRecordRemove(aid, uid, deleteEmployIds)
        end
    end,

    alliance_buff_open = function(aid, allianceBuff)
        -- print('alliance subscriber alliance_buff_open...aid, allianceBuff', aid, allianceBuff)
        if allianceBuff == nil then
            return
        end
        local al = alliances[aid]
        if al == nil then
            return
        end
        local buff = al:findAllianceBuff(allianceBuff.buffId)
        if buff == nil then
            buff = allianceInfo.newAllianceBuff(allianceBuff)
            al:addAllianceBuff(buff)
        else
            buff.createTimestamp = allianceBuff.createTimestamp
            buff.endTimestamp = allianceBuff.endTimestamp
            buff.useCount = allianceBuff.useCount
        end

        --update to client
        allianceStub.allianceBuffOpen(aid, allianceBuff)
    end,

    alliance_buff_closed = function(aid, buffId)
        -- print('alliance subscriber alliance_buff_closed...aid, buffId', aid, buffId)
        local al = alliances[aid]
        if al == nil then
            return
        end
        local buff = al:findAllianceBuff(buffId)
        if buff then
            buff.createTimestamp = 0
            buff.endTimestamp = 0
        end

        --update to client
        allianceStub.allianceBuffClosed(aid, buffId)
    end,

    alliance_buff_update = function(aid, allianceBuffList)
        -- print('alliance subscriber alliance_buff_update...aid, allianceBuffList', aid, allianceBuffList)
        local al = alliances[aid]
        if al == nil then
            return
        end
        if allianceBuffList == nil then
            return
        end
        for _, v in pairs(allianceBuffList) do
            local buff = al:findAllianceBuff(v.buffId)
            if buff then
                buff.createTimestamp = v.createTimestamp
                buff.endTimestamp = v.endTimestamp
                buff.useCount = v.useCount
            end
        end
        --update to client
        allianceStub.allianceBuffUpdate(aid)
    end,

    alliance_own_city = function(alCity)
        local city = allianceStub.allAllianceCity[alCity.allianceId]
        print("###MyAllianceCities", utils.serialize(city))
        if city ~= nil then
            table.insert(city,alCity.cityId)
        else
            city = {}
            table.insert(city,alCity.cityId)
            allianceStub.allAllianceCity[alCity.allianceId] = city
        end
        allianceStub.allianceOwnCity(alCity.allianceId, alCity.cityId)
        allianceStub.updateAlliance(alCity.allianceId, false)
        allianceStub.updateAllianceMemeberArmy(alCity.allianceId)
    end,

    alliance_lose_city = function(alCity)
        print("###MyAllianceCities", utils.serialize(city))
        local city = allianceStub.allAllianceCity[alCity.allianceId]
        if city ~= nil then
            for k,v in pairs(city) do
                if v == alCity.cityId then
                    table.remove(city,k)
                    break
                end
            end
        end
        allianceStub.allianceLoseCity(alCity.allianceId, alCity.cityId)
        allianceStub.updateAlliance(alCity.allianceId, false)
        allianceStub.updateAllianceMemeberArmy(alCity.allianceId)
    end,

     alliance_lose_city_buff = function(alCity)
        local city = allianceStub.uselessBuffCity[alCity.allianceId]
        print("###MyAllianceLoseBuffCities", utils.serialize(city))
        if city ~= nil then
            table.insert(city,alCity.cityId)
        else
            city = {}
            table.insert(city,alCity.cityId)
            allianceStub.uselessBuffCity[alCity.allianceId] = city
        end
        allianceStub.allianceLoseCity(alCity.allianceId, alCity.cityId)
        allianceStub.updateAlliance(alCity.allianceId, false)
        allianceStub.updateAllianceMemeberArmy(alCity.allianceId)
    end,

    alliance_add_city_buff = function(alCity)
        print("###MyAllianceLoseBuffCities", utils.serialize(city))
        local city = allianceStub.uselessBuffCity[alCity.allianceId]
        if city ~= nil then
            for k,v in pairs(city) do
                if v == alCity.cityId then
                    table.remove(city,k)
                    break
                end
            end
        end
        allianceStub.allianceOwnCity(alCity.allianceId, alCity.cityId)
        allianceStub.updateAlliance(alCity.allianceId, false)
        allianceStub.updateAllianceMemeberArmy(alCity.allianceId)
    end,
}

local rawStub = nil
local msg = nil
function allianceStub.connectService()
    rawStub, msg = cluster.connectService('alliance@cs')
    if not rawStub then
        print('allianceStub connectService alliance error,', msg)
        return
    end
    allianceStub.rawStub = rawStub
    rawStub:setSubscriber(subscriber)

    --fetch data
    allianceStub.fetch_alliances()
    allianceStub.fetch_alliance_invited()
    allianceStub.fetch_alliance_helps()
    allianceStub.fetch_alliance_hero_leases()
    allianceStub.fetch_alliance_records()
    allianceStub.fetch_alliance_hero_lease_records()
    allianceStub.fetch_alliance_city()
    allianceStub.fetch_alliance_lose_buff_city()
end


function allianceStub.queueErrorFun()
    print("queueErorFun ---------------")
end

function allianceStub.queueJob(threadFun)
    jobExecutor:queue(threadFun, allianceStub.queueErrorFun)
end


--
-- call/cast cs
--
function allianceStub.fetch_alliances()
    local bRet, alls = rawStub:call_fetch_alliances()
    -- print('allianceStub.call_fetch_alliances...bRet, alls', bRet, utils.serialize(alls))
    if bRet and alls then
        for _, v in pairs(alls) do
            local al = {}
            local base = {
                id = v.id,
                name = v.name,
                nickname = v.nickname,
                level = v.level,
                exp = v.exp,
                bannerId = v.bannerId,
                leaderId = v.leaderId,
                power = 0,
                leaderName = v.leaderName,
                leaderHeadId = v.leaderHeadId,
                alliesCount = 0,
                alliesMax = v.alliesMax,
                towerCount = v.towerCount,
                towerMax = v.towerMax,
                castleCount = v.castleCount,
                toatlActive = 0,
                announcement = v.announcement,
                openRecruitment = v.openRecruitment,
                createTime = v.createTime,
                disbandTime = v.disbandTime,
                changeBannerCdTime = v.changeBannerCdTime,
            }
            al = allianceInfo.info:new(base)
            al.memberList = {}
            al.applyList = {}
            al.inviteList = {}
            al.scienceList = {}
            al.allianceBuffList = {}
            alliances[al.id] = al
            if v.memberList then
                for _, m in pairs(v.memberList) do
                    -- print('call_fetch_alliances member ...', m.uid, m.nickname, m.rankLevel)
                    local member = allianceInfo.member:new(m)
                    al:addMember(member)
                end
            end
            if v.applyList then
                for _, apply in pairs(v.applyList) do
                    -- print('call_fetch_alliances applyList ....uid, nickname, headId, userLevel, vipLevel, time', apply.uid, apply.nickname, apply.headId, apply.userLevel, apply.vipLevel, apply.time)
                    al:addApply(apply.uid, apply.nickname, apply.headId, apply.userLevel, apply.vipLevel, apply.time)
                end
            end
            if v.scienceList then
                for _, science in pairs(v.scienceList) do
                    local s = allianceInfo.science:new(science)
                    if s then
                        al:addScience(s)
                    end
                end
            end
            if v.allianceBuffList then
                for _, v in pairs(v.allianceBuffList) do
                    local buffInfo = allianceInfo.newAllianceBuff(v)
                    if buffInfo then
                        al:addAllianceBuff(buffInfo)
                    end
                end
            end
        end
    end

    -- for _, v in pairs(alliances) do
    --     print('allianceStub.call_fetch_alliances...', utils.serialize(v))
    -- end
end

function allianceStub.fetch_alliance_invited()
    -- print('allianceStub.fetch_alliance_invited ...')
    local ret, list = rawStub:call_fetch_alliance_invited()

    -- print('allianceStub.call_fetch_alliance_invited ... ret, list', ret, utils.serialize(list))
    if ret and list then
        allianceStub.allianceInviteList = list
    end
end

function allianceStub.fetch_alliance_helps()
    -- print('allianceStub.fetch_alliance_helps ...')
    local ret, helps = rawStub:call_fetch_alliance_helps()

    -- print('allianceStub.call_fetch_alliance_helps ... ret, helps', ret, helps)
    if ret and helps then
        allianceStub.allianceHelps = helps
    end
end

function allianceStub.fetch_alliance_hero_leases()
    -- print('allianceStub.fetch_alliance_hero_leases ...')
    local ret, leases = rawStub:call_fetch_alliance_hero_leases()

    -- print('allianceStub.fetch_alliance_hero_leases ... ret, leases', ret, leases)
    if ret and leases then
        for aid, heroLeaseList in pairs(leases) do
            local heroLease = allianceStub.allianceHeroLeases[aid]
            if heroLease == nil then
                heroLease = {}
                allianceStub.allianceHeroLeases[aid] = heroLease
            end
            for uid, list in pairs(heroLeaseList) do
                if heroLease[uid] == nil then
                    heroLease[uid] = {}
                end
                for _, v in pairs(list) do
                    local leaseInfo = {}
                    leaseInfo.incomeRentSilver = v.incomeRentSilver
                    leaseInfo.incomeHookSilver = v.incomeHookSilver
                    leaseInfo.rentSilver = v.rentSilver
                    leaseInfo.canRecall = v.canRecall
                    leaseInfo.leaseBeginTime = v.leaseBeginTime
                    leaseInfo.hookBeginTime = v.hookBeginTime
                    leaseInfo.rentCount = v.rentCount
                    leaseInfo.todayRentCount = v.todayRentCount
                    leaseInfo.newRentCount = v.newRentCount

                    local info = pubHero.newHeroInfo(v.heroInfo)
                    leaseInfo.heroInfo = info
                    heroLease[uid][info.id] = leaseInfo
                end
            end
        end
    end
    -- print('=====fetch_alliance_hero_leases', utils.serialize(allianceStub.allianceHeroLeases))
end

function allianceStub.fetch_alliance_records()
    local ret, records = rawStub:call_fetch_alliance_records()
    -- print('allianceStub.fetch_alliance_records ... ret, records', ret, records)
    if ret and records then
        allianceStub.allianceRecords = records
    end
end

function allianceStub.fetch_alliance_hero_lease_records()
    local ret, records = rawStub:call_fetch_alliance_hero_lease_records()
    -- print('allianceStub.fetch_alliance_hero_lease_records ... ret, records', ret, records)
    if ret and records then
        allianceStub.allianceHeroleaseRecords = records
    end
end

function allianceStub.fetch_alliance_city()
    local ret, allianceCity = rawStub:call_fetch_alliance_city()
    -- print('allianceStub.fetch_alliance_city ... ret, allianceCity', ret, utils.serialize(allianceCity))
    if ret and allianceCity then
        allianceStub.allAllianceCity = allianceCity
    end
end

function allianceStub.fetch_alliance_lose_buff_city()
    local ret, uselessBuffCity = rawStub:call_fetch_alliance_lose_buff_city()
    -- print('allianceStub.fetch_alliance_lose_buff_city ... ret, uselessBuffCity', ret, utils.serialize(uselessBuffCity))
    if ret and uselessBuffCity then
        allianceStub.uselessBuffCity = uselessBuffCity
    end
end

--
function allianceStub.call_alliance_mail(aid, fromUid, fromNickname, fromHeadId, fromLangType, content)
    -- print('allianceStub.call_alliance_mail...aid, fromUid, fromNickname, fromHeadId, fromLangType, content', aid, fromUid, fromNickname, fromHeadId, fromLangType, content)
    local bRet, ret = rawStub:call_alliance_mail(aid, fromUid, fromNickname, fromHeadId, fromLangType, content)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret
end

function allianceStub.call_create(allianceName, allianceNickname, bannerId, uid, userNickname, headId, userLevel, vipLevel, power, castleLevel, x, y, marketIsOpen)
    local bRet, ret = rawStub:call_create(allianceName, allianceNickname, bannerId, uid, userNickname, headId, userLevel, vipLevel, power, castleLevel, x, y, marketIsOpen)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret
end

function allianceStub.call_apply_request(aid, uid, nickname, headId, userLevel, vipLevel)
    -- print('allianceStub.call_apply_request...aid, uid, nickname, headId, userLevel, vipLevel', aid, uid, nickname, headId, userLevel, vipLevel)
    local bRet, ret = rawStub:call_apply_request(aid, uid, nickname, headId, userLevel, vipLevel)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret
end

function allianceStub.call_apply_accept(aid, uid, isAccept, applyUid)
    -- print('allianceStub.call_apply_accept...aid, uid, isAccept, applyUid', aid, uid, isAccept, applyUid)
    local bRet, ret = rawStub:call_apply_accept(aid, uid, isAccept, applyUid)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret
end

function allianceStub.call_checkName(isCheckName, name)
    -- print('allianceStub.call_checkName...isCheckName, name', isCheckName, name)
    local bRet, ret = rawStub:call_checkName(isCheckName, name)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret
end

function allianceStub.cast_invite(aid, uid, invitedUids)
    rawStub:cast_invite(aid, uid, invitedUids)
end

function allianceStub.call_invite_accept(aid, uid, userNickname, headId, userLevel, vipLevel, isAccept, totalPower, castleLevel, x, y, marketIsOpen)
    -- print('allianceStub.call_invite_accept..aid, uid, userNickname, headId, userLevel, vipLevel, isAccept', aid, uid, userNickname, headId, userLevel, vipLevel, isAccept)
    local bRet, ret = rawStub:call_invite_accept(aid, uid, userNickname, headId, userLevel, vipLevel, isAccept, totalPower, castleLevel, x, y, marketIsOpen)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret
end

function allianceStub.call_quit(aid, uid)
    -- print('allianceStub.call_quit...aid, uid', aid, uid)
    local bRet, ret = rawStub:call_quit(aid, uid)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret
end

function allianceStub.call_kick(aid, uid, kickUid)
    -- print('allianceStub.call_kick...aid, uid, kickUid', aid, uid, kickUid)
    local bRet, ret = rawStub:call_kick(aid, uid, kickUid)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret
end

function allianceStub.call_change_banner(aid, uid, bannerId)
    -- print('allianceStub.call_change_banner...aid, uid, bannerId', aid, uid, bannerId)
    local bRet, ret = rawStub:call_change_banner(aid, uid, bannerId)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret
end

function allianceStub.call_change_announcement(aid, uid, announcement)
    -- print('allianceStub.call_change_announcement...aid, uid, announcement', aid, uid, announcement)
    local bRet, ret = rawStub:call_change_announcement(aid, uid, announcement)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret
end

function allianceStub.call_rankUp(aid, uid, upUid)
    -- print('allianceStub.call_rankUp...aid, uid, upUid', aid, uid, upUid)
    local bRet, ret = rawStub:call_rankUp(aid, uid, upUid)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret
end

function allianceStub.call_rankDown(aid, uid, downUid)
    -- print('allianceStub.call_rankDown...aid, uid, downUid', aid, uid, downUid)
    local bRet, ret = rawStub:call_rankDown(aid, uid, downUid)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret
end

function allianceStub.call_leader_transfer(aid, uid, transferUid)
    -- print('allianceStub.call_leader_transfer...aid, uid, transferUid', aid, uid, transferUid)
    local bRet, ret = rawStub:call_leader_transfer(aid, uid, transferUid)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret
end

function allianceStub.call_leader_replace(aid, uid)
    -- print('allianceStub.call_leader_replace...aid, uid', aid, uid)
    local bRet, ret = rawStub:call_leader_replace(aid, uid)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret
end

function allianceStub.call_science_donate(aid, uid, groupId, scienceExp, activeCount)
    -- print('allianceStub.call_science_donate...aid, uid, groupId, scienceExp, activeCount', aid, uid, groupId, scienceExp, activeCount)
    local bRet, ret = rawStub:call_science_donate(aid, uid, groupId, scienceExp, activeCount)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret
end

function allianceStub.call_help_request(aid, uid, helpInfo)
    -- print('allianceStub.call_help_request...aid, uid, helpInfo', aid, uid, helpInfo)
    local bRet, ret = rawStub:call_help_request(aid, uid, helpInfo)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret
end

function allianceStub.call_help_one(aid, uid, nickname, helpId)
    -- print('allianceStub.call_help_one ...aid, uid, nickname, helpId', aid, uid, nickname, helpId)
    local bRet, result = rawStub:call_help_one(aid, uid, nickname, helpId)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return result
end

function allianceStub.call_help_all(aid, uid, nickname, helpIds)
    -- print('allianceStub.call_help_all ...aid, uid, nickname, helpIds', aid, uid, nickname, helpIds)
    local bRet, result, count = rawStub:call_help_all(aid, uid, nickname, helpIds)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return result, count
end

function allianceStub.cast_help_accept(aid, uid, helpId)
    -- print('allianceStub.cast_help_accept ...aid, uid, helpId', aid, uid, helpId)
    rawStub:cast_help_accept(aid, uid, helpId)
end

function allianceStub.cast_help_close(aid, uid, helpIds)
    -- print('allianceStub.cast_help_close ...aid, helpId, helpIds', aid, uid, helpIds)
    rawStub:cast_help_close(aid, uid, helpIds)
end

function allianceStub.call_user_hire_hero(aid, uid, nickname, heroInfo)
    -- print('allianceStub.call_user_hire_hero...aid, uid, nickname, heroInfo', aid, uid, nickname, heroInfo)
    local bRet, ret = rawStub:call_user_hire_hero(aid, uid, nickname, heroInfo)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret
end

function allianceStub.call_user_recall_hero(aid, uid, heroId)
    -- print('allianceStub.call_user_recall_hero...aid, uid, heroId', aid, uid, heroId)
    local bRet, ret, collectSilver = rawStub:call_user_recall_hero(aid, uid, heroId)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret, collectSilver
end

function allianceStub.call_user_employ_hero(aid, uid, ownerUid, heroId)
    -- print('allianceStub.call_user_employ_hero...aid, uid, ownerUid, heroId', aid, uid, ownerUid, heroId)
    local bRet, ret = rawStub:call_user_employ_hero(aid, uid, ownerUid, heroId)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret
end

function allianceStub.call_collect_hero_hire_cost(aid, uid)
    -- print('allianceStub.call_collect_hero_hire_cost...aid, uid', aid, uid)
    local bRet, ret, collectSilver = rawStub:call_collect_hero_hire_cost(aid, uid)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret, collectSilver
end

function allianceStub.cast_member_update(aid, uid, nickname, headId, userLevel, vipLevel, power, castleLevel, x, y, marketIsOpen)
    -- print('allianceStub.cast_member_update....aid, uid, nickname, headId, userLevel, vipLevel', aid, uid, nickname, headId, userLevel, vipLevel)
    rawStub:cast_member_update(aid, uid, nickname, headId, userLevel, vipLevel, power, castleLevel, x, y, marketIsOpen)
end

function allianceStub.call_alliance_buff_open(aid, uid, buffId)
    -- print('allianceStub.call_alliance_buff_open...aid, uid, buffId', aid, uid, buffId)
    local bRet, ret = rawStub:call_alliance_buff_open(aid, uid, buffId)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret
end

function allianceStub.call_alliance_buff_closed(aid, uid, buffId)
    -- print('allianceStub.call_alliance_buff_closed...aid, uid, buffId', aid, uid, buffId)
    local bRet, ret = rawStub:call_alliance_buff_closed(aid, uid, buffId)
    if not bRet then
        return p.ErrorCode.UNKNOWN
    end
    return ret
end


--
-- local data
--
function allianceStub.member2Net(member)
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

function allianceStub.science2Net(science)
    local s = {
        groupId = science.groupId,
        tplId = science.tplId,
        level = science.level,
        exp = science.exp,
    }
    return s
end

function allianceStub.findMyAlliance(uid)
    -- print('allianceStub.findMyAlliance .... uid', uid)
    for _, v in pairs(alliances) do
        if v.disbandTime == 0 then
            for _, m in pairs(v.memberList) do
                if m.uid == uid then
                    -- print('allianceStub.findMyAlliance find aid, uid', v.id, m.uid)
                    return v
                end
            end
        end
    end
    return nil
end

function allianceStub.findAllianceByAid(aid)
    local info = alliances[aid]
    if info and info.disbandTime == 0 then
        return info
    end
    return nil
end

function allianceStub.getAllianceCount(allianceLevel)
    local count = 0
    for _, info in pairs(alliances) do
        if info.disbandTime == 0 and info.level >= allianceLevel then
            count = count + 1
        end
    end
    return count
end

function allianceStub.searchByName(allianceName, flag)
    if allianceName == nil then
        allianceName = ''
    end
    local list = {}
    if flag > 0 then
        local min = (flag - 1) * 50
        local max = flag * 50
        --按活跃排序
        local sortList = {}
        for _, v in pairs(alliances) do
            if v.disbandTime == 0 then
                table.insert(sortList, { power = v.power, aid = v.id })
            end
        end
        table.sort(sortList, function(infoLeft, infoRight)
            return infoLeft.power > infoRight.power
        end)
        --联盟列表
        for k, temp in ipairs(sortList) do
            if k > min and k <= max then
                local v = alliances[temp.aid]
                if v then
                    table.insert(list, {
                        aid = v.id,
                        name = v.name,
                        nickname = v.nickname,
                        bannerId = v.bannerId,
                        level = v.level,
                        alliesCount = v.alliesCount,
                        alliesMax = v.alliesMax,
                        toatlActive = v.toatlActive,
                        power = v.power,
                        })
                end
            end
            if k > max then
                break
            end
        end
    else
        --具体联盟
        for _, v in pairs(alliances) do
            if string.find(v.name, allianceName) and v.disbandTime == 0 then
            --if v.name == allianceName  and v.disbandTime == 0 then
                table.insert(list, {
                    aid = v.id,
                    name = v.name,
                    nickname = v.nickname,
                    bannerId = v.bannerId,
                    level = v.level,
                    alliesCount = v.alliesCount,
                    alliesMax = v.alliesMax,
                    toatlActive = v.toatlActive,
                    power = v.power,
                    })
            end
        end
    end
    return list
end

function allianceStub.searchById(aid)
    -- print('allianceStub.searchById ...', aid)
    local list = {}
    for _, v in pairs(alliances) do
        if v.id == aid and v.disbandTime == 0  then
            table.insert(list, v)
            table.insert(list, {
                aid = v.id,
                name = v.name,
                nickname = v.nickname,
                bannerId = v.bannerId,
                level = v.level,
                alliesCount = v.alliesCount,
                alliesMax = v.alliesMax,
                toatlActive = v.toatlActive,
                power = v.power
                })
            return list
        end
    end
    return nil
end

function allianceStub.isAllianceExist(aid)
    if alliances[aid] and alliances[aid].disbandTime == 0 then
        return true
    end
    return false
end

function allianceStub.isAllianceFull(aid)
    local al = alliances[aid]
    if al and al.disbandTime == 0 then
        return al.alliesCount >= al.alliesMax
    end
    return true
end

function allianceStub.isJoinAlliance(uid)
    for _, v in pairs(alliances) do
        if v.disbandTime == 0 then
            if v.memberList[uid] then
                return true
            end
        end
    end
    return false
end

function allianceStub.isApplyListHasUser(aid, uid)
    local info = alliances[aid]
    if info and info.disbandTime == 0 then
        for _, v in pairs(info.applyList) do
            if v.uid == uid then
                return true
            end
        end
    end
    return false
end

function allianceStub.isHeroLeased(aid, uid, heroId)
    if allianceStub.allianceHeroLeases[aid] then
        local ulease = allianceHeroLeases[aid][uid]
        if ulease and ulease[heroId] then
            return true
        end
    end
    return false
end

function allianceStub.getHeroLeaseCount(aid, uid)
    local count = 0
    if allianceStub.allianceHeroLeases[aid] then
        local ulease = allianceHeroLeases[aid][uid]
        for _, v in pairs(ulease) do
            count = count + 1
        end
    end
    return count
end

function allianceStub.getHeroLeaseIncome(aid, uid)
    local totalSilver = 0
    if allianceStub.allianceHeroLeases[aid] then
        local ulease = allianceStub.allianceHeroLeases[aid][uid]
        if ulease then
            for _, v in pairs(ulease) do
                totalSilver = totalSilver + v.incomeRentSilver
            end
        end
    end
    return totalSilver
end

function allianceStub.getHeroLeaseBeginTime(aid, uid)
    local leaseBeginTime = 0
    if allianceStub.allianceHeroLeases[aid] then
        local ulease = allianceStub.allianceHeroLeases[aid][uid]
        if ulease then
            for _, v in pairs(ulease) do
                leaseBeginTime =  v.leaseBeginTime or 0
            end
        end
    end
    return leaseBeginTime
end

function allianceStub.isCanOpenAllianceBuff(aid)
    local open = true
    local al = alliances[aid]
    if al and al.disbandTime == 0 then
        local now = timer.getTimestampCache()
        for _, v in pairs(al.allianceBuffList) do
            if now < v.endTimestamp then
                open = false
                break
            end
        end
    end
    return open
end

function allianceStub.isAllianceInvited(aid, uid)
    local info = alliances[aid]
    if info == nil or  info.disbandTime ~= 0 then
        return false
    end
    local uList = allianceStub.allianceInviteList[uid]
    if uList then
        for _, v in pairs(uList) do
            if v == aid then
                return true
            end
        end
    end
    return false
end

--
-- update to client
--
function allianceStub.updateAlliance(aid, allianceUp)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end
    for _, v in pairs(a.memberList) do
        loginStub.allianceUpdateAlliance(v.uid, allianceUp)
    end
end

function allianceStub.updateAllianceMemeberArmy(aid)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end
    for _, v in pairs(a.memberList) do
        loginStub.allianceMemberArmy(v.uid)
    end
end

function allianceStub.updateMember(aid, member, joinType)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end
    --
    local m = allianceStub.member2Net(member)
    -- print('allianceStub.updateMember...', utils.serialize(m))

    for _, v in pairs(a.memberList) do
        loginStub.allianceUpdateMember(v.uid, m, joinType)
    end
end

function allianceStub.deleteMember(aid, uid, allianceCdTimestamp, isKick)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end
    for _, v in pairs(a.memberList) do
        loginStub.allianceDeleteMember(v.uid, uid, allianceCdTimestamp, isKick)
    end
    --通知自己
    loginStub.allianceDeleteMember(uid, uid, allianceCdTimestamp, isKick)
end

function allianceStub.updateApply(aid, update, deletes)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end
    for _, v in pairs(a.memberList) do
        loginStub.allianceUpdateApply(v.uid, update, deletes)
    end
end

function allianceStub.updateInvited(aid, uid)
    print('allianceStub.updateInvited...aid, uid', aid, uid)
    loginStub.allianceUpdateInvited(aid, uid)
end

function allianceStub.removeInvited(aid, uid)
    print('allianceStub.removeInvited...aid, uid', aid, uid)
    loginStub.allianceRemoveInvited(aid, uid)
end

function allianceStub.updateScience(aid, science, uplevel)
    -- print('allianceStub.updateScience...aid, update, uplevel', aid, science, uplevel)
    local a = alliances[aid]
    if a == nil then
        return
    end
    --
    local s = allianceStub.science2Net(science)
    -- print('allianceStub.updateScience...', utils.serialize(s))

    for _, v in pairs(a.memberList) do
        loginStub.allianceUpdateScience(v.uid, s, uplevel)
    end
end

function allianceStub.updateHelpAdd(aid, help)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end
    for _, v in pairs(a.memberList) do
        loginStub.allianceUpdateHelpAdd(v.uid, help)
    end
end

function allianceStub.updateHelpOne(aid, fromUid, help)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end

    for _, v in pairs(a.memberList) do
        loginStub.allianceUpdateHelpOne(v.uid, fromUid, help)
    end
end

function allianceStub.updateHelpAll(aid, fromUid, helps)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end

    for _, v in pairs(a.memberList) do
        loginStub.allianceUpdateHelpAll(v.uid, fromUid, helps)
    end
end

function allianceStub.updateHelpAccept(aid, uid, help)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end

    for _, v in pairs(a.memberList) do
        if v.uid == uid then
            loginStub.allianceUpdateHelpAccept(v.uid, help)
            return
        end
    end
end

function allianceStub.updateHelpClose(aid, helpIds)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end

    for _, v in pairs(a.memberList) do
        loginStub.allianceUpdateHelpClose(v.uid, helpIds)
    end
end

function allianceStub.hireHeroUpdate(aid, ownerUid, ownerNickname, update)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end

    for _, v in pairs(a.memberList) do
        loginStub.allianceHireHeroUpdate(v.uid, ownerUid, ownerNickname, update)
    end
end

function allianceStub.deleteHireHero(aid, ownerUid, heroIds)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end
    for _, v in pairs(a.memberList) do
        loginStub.allianceDeleteHireHero(v.uid, ownerUid, heroIds)
    end
end

function allianceStub.employHeroUpdate(aid, uid, ownerUid, ownerNickname, heroInfo)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end
    loginStub.allianceEmployHeroUpdate(uid, ownerUid, ownerNickname, heroInfo)
end

function allianceStub.collectHeroHireCostUpdate(aid, uid, collectSilver, todayRentCount, newRentCount, timeSilver)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end
    loginStub.allianceCollectHeroHireCostUpdate(uid, collectSilver, todayRentCount, newRentCount, timeSilver)
end

function allianceStub.allianceRecordUpdate(aid, recordInfo)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end
    for _, v in pairs(a.memberList) do
        loginStub.allianceRecordUpdate(v.uid, recordInfo)
    end
end

function allianceStub.allianceHeroLeaseRecordAdd(aid, uid, leaseRecordInfo)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end
    loginStub.allianceHeroLeaseRecordAdd(uid, leaseRecordInfo, employIds)
end

function allianceStub.allianceHeroLeaseRecordRemove(aid, uid, employIds)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end
    loginStub.allianceHeroLeaseRecordRemove(uid, employIds)
end

function allianceStub.allianceBuffOpen(aid, allianceBuff)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end

    for _, v in pairs(a.memberList) do
        loginStub.allianceBuffOpen(v.uid, allianceBuff)
    end
end

function allianceStub.allianceBuffClosed(aid, buffId)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end

    for _, v in pairs(a.memberList) do
        loginStub.allianceBuffClosed(v.uid, buffId)
    end
end

function allianceStub.allianceBuffUpdate(aid)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end

    for _, v in pairs(a.memberList) do
        loginStub.allianceBuffUpdate(v.uid)
    end
end

function allianceStub.allianceOwnCity(aid, cityId)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end

    for _, v in pairs(a.memberList) do
        loginStub.allianceOwnCity(v.uid, cityId)
    end
end

function allianceStub.allianceLoseCity(aid, cityId)
    local a = alliances[aid]
    if a == nil or a.disbandTime ~= 0 then
        return
    end

    for _, v in pairs(a.memberList) do
        loginStub.allianceLoseCity(v.uid, cityId)
    end
end

return allianceStub