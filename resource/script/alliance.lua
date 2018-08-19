local p = require('protocol')
local t = require('tploader')
local utils = require('utils')
local misc = require('libs/misc')


local MAX_RECORD = 30

local alliance = {
    info = {
        id = 0,                 -- ID
        name = '',              -- 名称
        nickname = '',          -- 简称
        level = 1,              -- 等级
        exp = 0,                -- 经验
        bannerId = 1,           -- 联盟旗帜ID
        power = 0,              -- 联盟战力

        leaderId = 0,           -- 盟主ID
        leaderName = '',        -- 盟主昵称
        leaderHeadId = 0,       --

        alliesCount = 0,        -- 成员数
        alliesMax = 0,          -- 成员总数
        towerCount = 0,         -- 防御塔数
        towerMax = 0,           -- 防御塔总数
        castleCount = 0,        --占领城池数
        castleMax = 0,          --占领城池上限
        toatlActive = 0,        -- 联盟总活跃
        announcement = '',      -- 联盟公告
        openRecruitment = true, -- 公开招募(自动加入还是手动加入)
        createTime = 0,         --创建时间
        disbandTime = 0,        -- 解散时间

        changeBannerCdTime = 0, --换联盟旗帜CD

        applyList = {},         -- 申请加入列表 {uid, nickname, headId, userLevel, vipLevel, time}
        inviteList = {},        -- 被邀请人员列表 {uid, nickname, headId, inviteUid, time}
        memberList = {},        -- 成员列表<uid, <member>>
        scienceList = {},       -- 联盟科技列表<groupId, <science>>
        allianceBuffList = {},  -- 联盟增益buff列表<buffId, <allianceBuff>>

        isDirty = false,
        isSync = false,
    },
    -- 成员
    member = {
        uid = 0,
        nickname = '',
        headId = 0,
        userLevel = 0,
        vipLevel = 0,
        rankLevel = 4,         -- 联盟阶级
        activeWeek = 0,       -- 个人周活跃值
        contribution = 0,     -- 联盟贡献
        sendMailCount = 0,    -- 已发送全体邮件数量
        totalPower = 0,            -- 成员战力
        castleLevel = 0,        -- 玩家城池等级
        x = 0,                  
        y = 0,                -- 城池坐标
        marketIsOpen = false,      -- 市场是否开启


        --
        lastOnlineTime = 0,     --上次在线时间,0表示在线
        joinTime = 0,           --加入联盟时间

        --
        isDirty = false,
        isSync = false,
    },
    -- 科技
    science = {
        groupId = 0,    --科技组ID
        tplId = 0,      --科技ID()
        level = 1,      --科技等级
        exp = 0,        --科技经验

        isDirty = false,
        isSync = false,
    },
    --联盟增益buff
    allianceBuff = {
        tpl = nil,
        buffId = 0,
        type = 0,           --AllianceBuffType
        useCount = 0,       --已激活使用次数(每天定时更新)
        createTimestamp = 0,--开始激活时间戳
        endTimestamp = 0,   --激活到期时间戳

        isDirty = false,
        isSync = false,
    },
}

function alliance.info:new(o)
    o = o or {}
    if o.applyList == nil then o.applyList = {} end
    if o.inviteList == nil then o.inviteList = {} end
    if o.memberList == nil then o.memberList = {} end
    if o.scienceList == nil then
        o.scienceList = {}
        for k,v in pairs(t.allianceScience) do
            local science = alliance.science:new({ groupId = k, tplId = v[1].id, level = 1 })
            if not o.scienceList[science.groupId] then
                o.scienceList[science.groupId] = science
            end
            o.isDirty = true
            o.isSync = false  
        end
    end
    if o.allianceBuffList == nil then o.allianceBuffList = {} end

    setmetatable(o, self)
    self.__index = self
    return o
end

function alliance.member:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end

function alliance.science:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end

function alliance.allianceBuff:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end

function alliance.newAllianceBuff(o)
    if o == nil then
        o = {}
    end
    local tpl = t.allianceBuff[o.buffId]
    if tpl == nil then
        return nil
    end
    local tempBuff = {}
    tempBuff.buffId = o.buffId
    tempBuff.tpl = tpl
    tempBuff.type = tpl.type
    tempBuff.useCount = o.useCount or 0
    tempBuff.createTimestamp = o.createTimestamp or 0
    tempBuff.endTimestamp = o.endTimestamp or 0
    local buffInfo = alliance.allianceBuff:new(tempBuff)

    buffInfo.isDirty = true
    buffInfo.isSync = false

    return buffInfo
end

--基本数据
function alliance.info:isAllianceLevelFull()
    return self.level >= #t.allianceLevel
end

function alliance.info:AddAllianceExp(allianceExp)
    -- print('alliance.info:AddAllianceExp...addExp, allianceExp, allianceLevel', allianceExp, self.exp, self.level)
    local levelup = false
    if allianceExp > 0 then
        local tpl = t.allianceLevel[self.level]
        local totalExp = self.exp + allianceExp
        while tpl ~= nil and totalExp >= tpl.exp and not self:isAllianceLevelFull() do
            totalExp = totalExp - tpl.exp
            self.level = self.level + 1
            self.exp = 0
            levelup = true
            tpl = t.allianceLevel[self.level]
        end
        if levelup then
            self.exp = totalExp
        else
            if self:isAllianceLevelFull() then
                self.exp = 0
            else
                self.exp = self.exp + allianceExp
            end
        end
        self.isDirty = true
        self.isSync = false
    end
    -- print('alliance.info:AddAllianceExp...levelup, allianceExp, allianceLevel', levelup, self.exp, self.level)

    return levelup
end

--成员列表
function alliance.info:findMember(uid)
    return self.memberList[uid]
end

function alliance.info:addMember(m)
    -- print("###alliance.info:addMember", utils.serialize(m))
    -- print(debug.traceback())
    -- print("self.memberList", utils.serialize(self.memberList))
    if m then
        -- 联盟战力为成员总战斗力相加   TODO by zds
        if self.memberList[m.uid] == nil then
            self.alliesCount = self.alliesCount + 1
            self.toatlActive = self.toatlActive + m.activeWeek
            self.power = self.power + m.totalPower
        end
        self.memberList[m.uid] = m
    else
        print('alliance.info:addMember ... m is nil')
    end
    -- print("self.memberList", utils.serialize(self))
end

function alliance.info:removeMember(uid)
    if self.memberList[uid] == nil then
        return
    end
    local m = self.memberList[uid]
    self.alliesCount = self.alliesCount - 1

    self.memberList[uid] = nil
    self.isDirty = true
    self.isSync = false
end

function alliance.info:memberOnline(uid)
    if self.memberList[uid] == nil then
        return
    end
    self.memberList[uid].lastOnlineTime = 0
    self.memberList[uid].isDirty = true
    self.memberList[uid].isSync = false
end

function alliance.info:memberOffline(uid)
    if self.memberList[uid] == nil then
        return
    end
    self.memberList[uid].lastOnlineTime = offlineTime
    self.memberList[uid].isDirty = true
    self.memberList[uid].isSync = false
end

function alliance.member:haveRankPermit(rp)
    local permit = t.allianceRankDetail[self.rankLevel].permit
    if permit == nil then
        return false
    end
    if permit[rp] == nil then
        return false
    end
    return true
end

function alliance.member:haveSendMailMax()
    local count = 0
    local permit = t.allianceRankDetail[self.rankLevel]
    if permit == nil then
        return count
    end
    count = permit.sendMailmax or 0
    return count
end

--申请列表
function alliance.info:findApply(uid)
    for _, v in pairs(self.applyList) do
        if v.uid == uid then
            return v
        end
    end
    return nil
end

function alliance.info:isApplyListHasUser(uid)
    for _, v in pairs(self.applyList) do
        if v.uid == uid then
            return true
        end
    end
    return false
end

function alliance.info:addApply(uid, nickname, headId, userLevel, vipLevel, time)
    local apply = self:findApply(uid)
    if apply ~= nil then
        apply.nickname = nickname
        apply.headId = headId
        apply.userLevel = userLevel
        apply.vipLevel = vipLevel
        apply.time = time
        return
    end
    table.insert(self.applyList, 1, { uid = uid, nickname = nickname, headId = headId, userLevel = userLevel, vipLevel = vipLevel, time = time })

    local deletes = {}
    while #self.applyList > 30 do
        table.insert(deletes, { uid = self.applyList[1].uid })
        table.remove(self.applyList, 1)
    end
    self.isDirty = true
    self.isSync = false

    return deletes
end

function alliance.info:removeApply(uid)
    for k, v in pairs(self.applyList) do
        if v.uid == uid then
            table.remove(self.applyList, k)
            self.isDirty = true
            self.isSync = false
            return true
        end
    end
    return false
end

--邀请列表
function alliance.info:findInvite(uid, inviteUid)
    for _, v in pairs(self.inviteList) do
        if v.uid == uid and v.inviteUid == inviteUid then
            return v
        end
    end
    return nil
end

function alliance.info:addInvite(uid, nickname, headId, inviteUid, time)
    local invite = self:findInvite(uid, inviteUid)
    if invite ~= nil then
        return
    end
    table.insert(self.inviteList, 1, { uid = uid, nickname = nickname, headId = headId, inviteUid = inviteUid, time = time })
    while #self.inviteList > 100 do
        table.remove(self.inviteList, 1)
    end
    self.isDirty = true
    self.isSync = false
end

function alliance.info:removeInvite(uid, inviteUid)
    for k, v in pairs(self.inviteList) do
        if v.uid == uid and v.inviteUid == inviteUid then
            table.remove(self.inviteList, k)
            self.isDirty = true
            self.isSync = false
            return true
        end
    end
    return false
end

--联盟科技列表
function alliance.info:findScience(groupId)
    return self.scienceList[groupId]
end

function alliance.info:addScience(science)
    if not self.scienceList[science.groupId] then
        self.scienceList[science.groupId] = science
    end
    self.isDirty = true
    self.isSync = false
end

--联盟增益buff
function alliance.info:findAllianceBuff(buffId)
    return self.allianceBuffList[buffId]
end

function alliance.info:addAllianceBuff(allianceBuff)
    if not self.allianceBuffList[allianceBuff.buffId] then
        self.allianceBuffList[allianceBuff.buffId] = allianceBuff
    end
    self.isDirty = true
    self.isSync = false
end

function alliance.info:resetAllianceBuff(buffId)
    local buff = self.allianceBuffList[buffId]
    if buff then
        buff.createTimestamp = 0
        buff.endTimestamp = 0

        self.isDirty = true
        self.isSync = false
        return true
    end
    return false
end

return alliance
