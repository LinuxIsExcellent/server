local cluster = require('cluster')
local p = require('protocol')

local loginStub = {
    rawStub,
    onlineCount = 0,
    autoSaveInterval = 300,
    csShutdown = false
}

local globalUserDict = {}
local agentDict = {}

local subscriber = {
    user_online = function(uid, mbid, onlineCount)
        globalUserDict[uid] = {
            uid = uid,
            mbid = mbid
        }
        loginStub.onlineCount = onlineCount
        loginStub.updateAutoSaveInterval()
    end,

    user_offline = function(uid, onlineCount)
        globalUserDict[uid] = nil
        loginStub.onlineCount = onlineCount
        loginStub.updateAutoSaveInterval()
    end,

    cs_shutdown = function()
        --print('### cs_shutdown ###')
        loginStub.csShutdown = true
    end,
}

local rawStub

function loginStub.findUser(uid)
    return globalUserDict[uid]
end

function loginStub.connectService()
    rawStub = cluster.connectService('login@cs')
    loginStub.rawStub = rawStub
    rawStub:setSubscriber(subscriber)
end

function loginStub.addUser(rawAgent, agent)
    agentDict[rawAgent] = agent
end

function loginStub.removeUser(rawAgent)
    local agent = agentDict[rawAgent]
    if agent ~= nil then
        if agent.user ~= nil and agent.user.uid ~= 0 then
            rawStub:cast_logout(agent.user.uid)
        end
        agentDict[rawAgent] = nil
    end
end

function loginStub.updateAutoSaveInterval()
    if loginStub.onlineCount > 1000 then
        loginStub.autoSaveInterval = 600
    elseif loginStub.onlineCount > 500 then
        loginStub.autoSaveInterval = 300
    elseif loginStub.onlineCount > 200 then
        loginStub.autoSaveInterval = 180
    else
        loginStub.autoSaveInterval = 90
    end
end

function loginStub.allianceUpdateAlliance(uid, allianceUp)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.updateAlliance(allianceUp)
        return
        end
    end
end

function loginStub.allianceMemberArmy(uid)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.army.sendArmyInfosUpdate()
            return
        end
    end
end
function loginStub.allianceUpdateMember(uid, member, joinType)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.updateMember(member, joinType)
            return
        end
    end
end
function loginStub.allianceDeleteMember(uid, memberUid, allianceCdTime, isKick)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.deleteMember(memberUid, allianceCdTime, isKick)
            return
        end
    end
end
function loginStub.allianceUpdateApply(uid, updates, deletes)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.updateApply(updates, deletes)
            return
        end
    end
end
function loginStub.allianceUpdateInvited(aid, uid)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.updateInvited(aid)
            return
        end
    end
end
function loginStub.allianceRemoveInvited(aid, uid)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.removeInvited(aid)
            return
        end
    end
end
function loginStub.allianceDisband(uid)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.disband()
            return
        end
    end
end
function loginStub.allianceUpdateScience(uid, science, uplevel)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.updateScience(science, uplevel)
            return
        end
    end
end

function loginStub.allianceBlock(uid, block)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.sendBlock(block)
            return
        end
    end
end

function loginStub.allianceUnblock(uid, unblockUid)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.sendUnblock(unblockUid)
            return
        end
    end
end

function loginStub.allianceMessageUpdate(aid, uid, message)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.allianceMessageUpdate(aid, message)
            return
        end
    end
end
function loginStub.allianceUpdateHelp(uid, help)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.updateHelp(help)
            return
        end
    end
end
function loginStub.allianceUpdateHelpAdd(uid, help)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.updateHelpAdd(help)
            return
        end
    end
end
function loginStub.allianceUpdateHelpOne(uid, fromUid, help)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.updateHelpOne(fromUid, help)
            return
        end
    end
end
function loginStub.allianceUpdateHelpAll(uid, fromUid, helps)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.updateHelpAll(fromUid, helps)
            return
        end
    end
end
function loginStub.allianceUpdateHelpAccept(uid, help)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.updateHelpAccept(help)
            return
        end
    end
end
function loginStub.allianceUpdateHelpClose(uid, helpIds)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.updateHelpClose(helpIds)
            return
        end
    end
end

function loginStub.allianceUpdateLeaderTransfer(uid, newLeader, oldLeader, isReplace)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.updateLeaderTransfer(newLeader, oldLeader, isReplace)
            return
        end
    end
end

function loginStub.allianceUpdateAddAllianceGift(uid, giftInfo)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.updateAddAllianceGift(giftInfo)
            return
        end
    end
end

function loginStub.allianceHireHeroUpdate(uid, ownerUid, updates)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.hireHeroUpdate(ownerUid, updates)
            return
        end
    end
end

function loginStub.allianceDeleteHireHero(uid, ownerUid, heroIds)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.deleteHireHero(ownerUid, heroIds)
            return
        end
    end
end

function loginStub.allianceEmployHeroUpdate(uid, ownerUid, ownerNickname, heroInfo)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.employHeroUpdate(ownerUid, ownerNickname, heroInfo)
            return
        end
    end
end

function loginStub.allianceCollectHeroHireCostUpdate(uid, collectSilver, todayRentCount, newRentCount, timeSilver)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.collectHeroHireCostUpdate(collectSilver, todayRentCount, newRentCount, timeSilver)
            return
        end
    end
end

function loginStub.allianceRecordUpdate(uid, record)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.allianceRecordUpdate(record)
            return
        end
    end
end

function loginStub.allianceHeroLeaseRecordAdd(uid, leaseRecord)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.allianceHeroLeaseRecordAdd(leaseRecord)
            return
        end
    end
end

function loginStub.allianceHeroLeaseRecordRemove(uid, employIds)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.allianceHeroLeaseRecordRemove(employIds)
            return
        end
    end
end

function loginStub.allianceBuffOpen(uid, allianceBuff)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.allianceBuffOpen(allianceBuff)
            return
        end
    end
end

function loginStub.allianceBuffClosed(uid, buffId)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.allianceBuffClosed(buffId)
            return
        end
    end
end

function loginStub.allianceBuffUpdate(uid)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.alliance.allianceBuffUpdate()
            return
        end
    end
end

function loginStub.arenaRankUpdate(uid, rank)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.arena.arenaRankUpdate(rank)
            return
        end
    end
end

function loginStub.arenaChangeOpponent(uid, list)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.arena.arenaChangeOpponent(list)
            return
        end
    end
end

--broad
function loginStub.broadKingdomChat(chat)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted then
            v.chat.sendKingdomChat(chat)
        end
    end
end

function loginStub.broadAllianceChat(aid, chat)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.alliance.info ~= nil and v.alliance.info.id == aid then
            v.chat.sendAllianceChat(chat)
        end
    end
end

function loginStub.broadActivityUpdate()
    for _, v in pairs(agentDict) do
        if v.isInitCompleted then
            v.activity.sendActivityRecordUpdate()
        end
    end
end

function loginStub.broadMarquee(marquee)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted then
            v.gms.sendMarquee(marquee)
        end
    end
end

function loginStub.broadNoticeUpdate()
    for _, v in pairs(agentDict) do
        if v.isInitCompleted then
            v.gms.sendNoticeUpdate()
        end
    end
end

function loginStub.broadMailBatchUpdate()
    for _, v in pairs(agentDict) do
        if v.isInitCompleted then
            v.gms.pickMailBatches()
        end
    end
end

function loginStub.broadOnDistributeRewardsFinished()
    for _, v in pairs(agentDict) do
        if v.isInitCompleted then
            v.tower.onDistributeRewardsFinished()
        end
    end
end

function loginStub.broadChargeTplUpdate()
    for _, v in pairs(agentDict) do
        if v.isInitCompleted then
            v.charge.sendChargeOptions()
        end
    end
end

function loginStub.broadAchievementUpdate(achievementId)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted then
            v.achievement.achievementUpdate(achievementId, false)
        end
    end
end

function loginStub.allianceOwnCity(uid, cityId)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.property.onAllianceOwnCity(cityId)
        end
    end
end

function loginStub.allianceLoseCity(uid, cityId)
    for _, v in pairs(agentDict) do
        if v.isInitCompleted and v.user.uid == uid then
            v.property.onAllianceLoseCity(cityId)
        end
    end
end

return loginStub
