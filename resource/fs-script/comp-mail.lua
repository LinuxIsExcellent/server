local agent = ...
local p = require('protocol')
local t = require('tploader')
local dbo = require('dbo')
local timer = require('timer')
local utils = require('utils')
local trie = require('trie')
local misc = require('libs/misc')
local pubMail = require('mail')
local chatStub = require('stub/chat')
local dataStub = require('stub/data')

local user = agent.user
local bag = agent.bag
local property

local mail = {
    mailParentIds = {},     --map<mailSubTypeName, id>
    mailTypeMaxCount = {},  --map<mailType, count>

    mails = {},             --map[mailType][mailId] = { lastCreateTime=0, ms={mailInfo,...} }
    mailDeletes = {},       --map<mailId, mailInfo>
}

-- init
local SUB_MAIL_MAX_COUNT = 20
mail.mailParentIds['private'] = {}
mail.mailParentIds['be_scout'] = 0

mail.mailTypeMaxCount[p.MailType.SYSTEM] = 30
mail.mailTypeMaxCount[p.MailType.REPORT] = 30
mail.mailTypeMaxCount[p.MailType.SCOUT] = 20
mail.mailTypeMaxCount[p.MailType.GATHER] = 10
mail.mailTypeMaxCount[p.MailType.PRIVATE] = 10


function mail.onInit()
    mail.dbLoad()
end

function mail.onAllCompInit()
    mail.sendMailUpdate()
end

function mail.onReady()
    --test
    if user.newPlayer then
        --[[
        local mailType = p.MailType.SYSTEM
        local mailSubType = p.MailSubType.SYSTEM_BAG_FULL
        local title, content = agent.mail.getTitleContentBySubType(mailSubType)
        local drops = {}
        table.insert(drops, { tplId = 2000001, count = 10, cond = {} })
        local attachments = utils.serialize(drops)
        local param = utils.serialize({})
        local now = timer.getTimestampCache()
        mail.saveMailRaw(user.uid, 0, mailType, mailSubType, title, content, now, true, false, attachments, param, '')


        --邮件发送
        local drops = {}
        table.insert(drops, { tplId = 2, count = 10, cond = {} })
        local mailType = p.MailType.SYSTEM
        local mailSubType = p.MailSubType.SYSTEM_ARENA_MAX_RECORD
        local title, content = agent.mail.getTitleContentBySubType(mailSubType)
        local attachments = utils.serialize(drops)
        local timestamp = timer.getTimestampCache()
        agent.mail.saveMailRaw(user.uid, 0, mailType, mailSubType, title, content, timestamp, true, false, attachments, '', '')

        local drops = {}
        local mailType = p.MailType.SYSTEM
        local mailSubType = p.MailSubType.SYSTEM_OPERATION
        local title = '自定义邮件1'
        local content = '你写我发，就这么简单1'
        local attachments = utils.serialize(drops)
        local timestamp = timer.getTimestampCache()
        agent.mail.saveMailRaw(user.uid, 0, mailType, mailSubType, title, content, timestamp, false, false, attachments, '', '')

        local drops = {}
        local mailType = p.MailType.SYSTEM
        local mailSubType = p.MailSubType.SYSTEM_OPERATION
        local title = '自定义邮件2'
        local content = '你写我发，就这么简单2'
        local attachments = ''
        local timestamp = timer.getTimestampCache()
        agent.mail.saveMailRaw(user.uid, 0, mailType, mailSubType, title, content, timestamp, false, false, attachments, '', '')
        --]]
    end
end

function mail.onClose()
    mail.dbSave()
end

function mail.onSave()
    mail.dbSave()
end

function mail.dbLoad()
    local db = dbo.open(0)
    --print("--------------Mail *** dbLoad ---------- ")
    local rs = db:executePrepare('SELECT id, parentId, uid, otherUid, type, subType, title, content, isLang, isRead, isDraw, createTime, attachment, params, reportId from s_mail where uid = ? and deleteType = 0 order by parentId, id', user.uid)
    if rs.ok then
        for _, row in ipairs(rs) do
            local info = pubMail.newMailInfo(row)
            mail.insertMail(info, false)
            --print('dbLoad .... new... ', info.id, info.uid, info.otherUid, info.title, info.type, info.subType, info.isRead, info.createTime, info.reportId)
        end
        -- check outdated mail
        mail.clearupMail(true)
    end
end

function mail.dbSave()
    local list = {}
    for _, mails in pairs(mail.mails) do
        for _, mail in pairs(mails) do
            for _, m in pairs(mail.ms) do
                if m.isDirty then
                    m.isDirty = false
                    table.insert(list, m)
                    -- print('mail.dbSave .... insert is dirty... ', m.id, m.title, m.type, m.subType, m.isRead, m.createTime, m.deleteType)
                end
            end
        end
    end
    for _, v in pairs(mail.mailDeletes) do
        if v.isDirty then
            v.isDirty = false
            table.insert(list, v)
        else
            --清除已删除邮件
            mail.mailDeletes[v.id] = nil
        end
    end

    dataStub.appendMailUpdateList(list)
end

function mail.getTitleContentBySubType(mailSubType)
    return pubMail.getTitleContentBySubType(mailSubType)
end

function mail.getMailParentId(mailSubType, otherUid)
    local parentId = 0
    if mailSubType == p.MailSubType.PRIVATE_MAIL or mailSubType == p.MailSubType.PRIVATE_ALLIANCE_MEMBER then
        if otherUid ~= 0 then
            parentId = mail.mailParentIds['private'][otherUid] or 0
        end
    elseif mailSubType == p.MailSubType.SCOUT_DEFENSE_SUCCESS or mailSubType == p.MailSubType.SCOUT_DEFENSE_FAIL then
        parentId = mail.mailParentIds['be_scout'] or 0
    end
    -- print('mail.getMailParentId type, parentId', mailSubType, parentId)
    return parentId
end

function mail.setMailParentId(mailSubType, pid, otherUid)
    if mailSubType == p.MailSubType.PRIVATE_MAIL or mailSubType == p.MailSubType.PRIVATE_ALLIANCE_MEMBER then
        mail.mailParentIds['private'][otherUid] = pid
    elseif mailSubType == p.MailSubType.SCOUT_DEFENSE_SUCCESS or mailSubType == p.MailSubType.SCOUT_DEFENSE_FAIL then
        mail.mailParentIds['be_scout'] = pid
    end
    -- print('mail.setMailParentId type, parentId', mailSubType, pid)
end

function mail.saveMailRaw(uid, otherUid, mailType, mailSubType, title, content, createTime, isLang, isDraw, attachment, params, reportId)
    -- check
    if type(uid) ~= 'number' or type(otherUid) ~= 'number' or type(mailType) ~= 'number' or type(mailSubType) ~= 'number' or type(title) ~= 'string' or type(content) ~= 'string' 
        or type(createTime) ~= 'number' or type(isLang) ~= 'boolean' or type(isDraw) ~= 'boolean' or type(attachment) ~= 'string' or type(params) ~= 'string' or type(reportId) ~= 'number' then
        -- print(debug.traceback())
        utils.log('mail.saveMailRaw wrong types : ' .. type(uid) .. type(otherUid) .. type(mailType) .. type(mailSubType) .. type(title) .. type(content) .. type(createTime) .. type(isLang) .. type(isDraw) .. type(attachment) .. type(params) .. type(reportId))
        return
    end
    if uid > 0 then
        -- new info
        local m = pubMail.newMailInfo()
        m.uid = uid
        m.otherUid = otherUid
        m.type = mailType
        m.subType = mailSubType
        m.title = title
        m.content = content
        m.isLang = isLang
        m.isDraw = isDraw
        m.createTime = createTime
        m.attachment = attachment
        m.params = params
        m.reportId = reportId
        local parentId = mail.getMailParentId(mailSubType, otherUid) or 0
        m.parentId = parentId
        -- save
        dataStub.appendMailInfo(m)
    end
end

function mail.reloadMailById(mid)
    agent.queueJob(function()
        -- print('mail.reloadMailById ......', user.uid, mid)
        local db = dbo.open(0)
        local rs = db:executePrepare('SELECT id, parentId, uid, otherUid, type, subType, title, content, isLang, isRead, isDraw, createTime, attachment, params, reportId from s_mail where uid = ? and id = ? and deleteType = 0 order by parentId, id', user.uid, mid)
        if rs.ok then
            for _, row in ipairs(rs) do
                local info = pubMail.newMailInfo(row)
                mail.insertMail(info, true)
                -- print('dbLoadById .... new... ', info.id, info.uid, info.otherUid, info.title, info.type, info.subType, info.isLang, info.isRead, info.createTime)
                mail.sendMailUpdate()
            end
        end
    end)
end

function mail.insertMail(m, bClearup)
    if m == nil then
        print('insertMail.....error. mail is nil')
        return
    end

    local mails = mail.mails[m.type]
    if mails == nil then
        mail.mails[m.type] = {}
        mails = mail.mails[m.type]
    end

    local mailTab = {}
    if m.parentId == 0 then
        m.parentId = mail.getMailParentId(m.subType, m.otherUid)
    end

    if m.parentId == 0 then
        mailTab.lastCreateTime = m.createTime
        mailTab.ms = {}
        table.insert(mailTab.ms, m)

        m.parentId = m.id
        mails[m.parentId] = mailTab
        mail.setMailParentId(m.subType, m.id, m.otherUid)
    else
        mailTab = mails[m.parentId]
        if mailTab == nil then
            mailTab = {}
            mailTab.lastCreateTime = 0
            mailTab.ms = {}
            mails[m.parentId] = mailTab
            mail.setMailParentId(m.subType, m.parentId, m.otherUid)
        end
        table.insert(mailTab.ms, m)
        if mailTab.lastCreateTime < m.createTime then
            mailTab.lastCreateTime = m.createTime
        end
    end
    --清除超出数量的邮件
    if bClearup then
        mail.clearupMail()
    end
end

function mail.clearupMail(bLoad)
    local nowTime = timer.getTimestampCache()
    local removeList = {}
    for mailType, maxCount in pairs(mail.mailTypeMaxCount) do
        local mails = mail.mails[mailType]
        if mails then
            --对未删掉的邮件按时间从大到小排序
            local tempMails = {}
            for mailId, v in pairs(mails) do
                for _, m in pairs(v.ms) do
                    if m.deleteTime == 0 then
                        table.insert(tempMails, { lastCreateTime = v.lastCreateTime, id = mailId })
                        break
                    end
                end
            end
            table.sort(tempMails, function(lhs,rhs) return lhs.lastCreateTime > rhs.lastCreateTime end)

            --检测邮件数量上限
            local count = 0
            for _, v in pairs(tempMails) do
                count = count + 1
                local ms = mails[v.id].ms or {}
                if count <= maxCount then
                    --按时间从大到小排序,对合并的邮件做删除操作
                    table.sort(ms, function(lhs, rhs) return tonumber(lhs.createTime) > tonumber(rhs.createTime) end)
                    local subCount = 0
                    local deleteList = {}
                    for k, m in ipairs(ms) do
                        subCount = subCount + 1
                        if subCount > SUB_MAIL_MAX_COUNT then
                            m.isSync = true
                            m.isDirty = true
                            m.deleteType = p.MailDeleteType.BEYOND_DELETE
                            m.deleteTime = nowTime
                            --
                            table.insert(deleteList, k)
                            mail.mailDeletes[m.id] = m
                        end
                    end
                    for _, k in pairs(deleteList) do
                        ms[k] = nil
                    end
                else
                    --删掉
                    for _, m in pairs(ms) do
                        m.isSync = true
                        m.isDirty = true
                        m.deleteType = p.MailDeleteType.BEYOND_DELETE
                        m.deleteTime = nowTime
                        --
                        mail.mailDeletes[m.id] = m
                    end
                    if not bLoad then
                        table.insert(removeList, { mailId = v.id })
                    end
                    mails[v.id] = nil
                end
            end
        end
    end
    --通知客户端
    if next(removeList) then
        agent.sendPktout(p.SC_MAIL_DELETE_RESPONSE, '@@1=b,2=[mailId=i]', true, removeList)
    end
end

function mail.checkCanDrawAttachment(list)
    local addValue = 0
    for _, v in pairs(list) do
        local tpl = t.item[v.tplId]
        if tpl then
            local bCheck = true
            --不进背包的不算
            if tpl.type == p.ItemType.PROP and tpl.subType == p.ItemPropType.SPECIAL_ITEMS then
                bCheck = false
            end
            if bCheck then
                --是否可以叠加
                if bag.checkCanAccumulateByCond(v.cond) then
                    addValue = addValue + 1
                else
                    addValue = addValue + v.count
                end
                if bag.checkBagIsFullByAdd(addValue) then
                    return false
                end
            end
        end
    end
    return true
end

--邮件列表微设计
function mail.setMailMicroDesign(mailType, subType, mailInfo)
    if type(mailInfo) ~= 'table'then
        return 0, {}
    end
    local function getPos(pos)
        local x = ''
        local y = ''
        if pos then
            if pos.x then
                x = tostring(pos.x)
            end
            if pos.y then
                y = tostring(pos.y)
            end
        end

        return x .. ',' .. y
    end
    local function getPosByTable(pos)
        local x = ''
        local y = ''
        if pos then
            if pos[2] then
                y = tostring(pos[2])
            end
            if pos[1] then
                x = tostring(pos[1])
            end
        end

        return x .. ',' .. y
    end
    --图标
    local toUid = 0
    local toNickname = ''
    local toHeadId = pubMail.getIconBySubType(mailInfo.subType)
    --摘要
    local summary = pubMail.getSummaryBySubType(mailInfo.subType)
    --摘要所需参数
    local paramsTemp = ''
    local params = misc.deserialize(mailInfo.params) or {}
    if mailType == p.MailType.SYSTEM then
        if subType == p.MailSubType.SYSTEM_REWARD then
        elseif subType == p.MailSubType.SYSTEM_MAINTENANCE then
        elseif subType == p.MailSubType.SYSTEM_OPERATION then
        elseif subType == p.MailSubType.SYSTEM_BAG_FULL then
        elseif subType == p.MailSubType.SYSTEM_ARENA_MAX_RECORD then
        elseif subType == p.MailSubType.SYSTEM_ALLIANCE_INVITE
            or subType == p.MailSubType.SYSTEM_ALLIANCE_KICK
            or subType == p.MailSubType.SYSTEM_ALLIANCE_DISBAND
            or subType == p.MailSubType.SYSTEM_ALLIANCE_LEADER_TRANSFER
            or subType == p.MailSubType.SYSTEM_ALLIANCE_LEADER_REPLACE
            or subType == p.MailSubType.SYSTEM_ALLIANCE_TELEPORT_INVITE
            or subType == p.MailSubType.SYSTEM_ALLIANCE_REWARD
            or subType == p.MailSubType.SYSTEM_ALLIANCE_LEVEL_UP
            or subType == p.MailSubType.SYSTEM_ALLIANCE_REWARD_DETAIL 
            or subType == p.MailSubType.SYSTEM_ALLIANCE_OWN_CITY 
            or subType == p.MailSubType.SYSTEM_ALLIANCE_LOSE_CITY 
            or subType == p.MailSubType.SYSTEM_ALLIANCE_OWN_CITY_OCCUPY 
            or subType == p.MailSubType.SYSTEM_ALLIANCE_LOSE_CITY_OCCUPY 
            or subType == p.MailSubType.SYSTEM_ATTACK_CASTLE_SUCCESS 
            or subType == p.MailSubType.SYSTEM_PALACE_WAR_GIFT 
            or subType == p.MailSubType.SYSTEM_WORLDBOSS_HURT_REWARD
            or subType == p.MailSubType.SYSTEM_WORLDBOSS_KILL_REWARD
            or subType == p.MailSubType.SYSTEM_WORLDBOSS_ALLIANCE_REWARD
            or subType == p.MailSubType.SYSTEM_CITY_HURT_REWARD
            or subType == p.MailSubType.SYSTEM_CITY_KILL_REWARD
            or subType == p.MailSubType.SYSTEM_CITY_ALLIANCE_REWARD
            or subType == p.MailSubType.SYSTEM_CATAPULT_HURT_REWARD
            or subType == p.MailSubType.SYSTEM_CATAPULT_KILL_REWARD
            or subType == p.MailSubType.SYSTEM_CATAPULT_ALLIANCE_REWARD
            or subType == p.MailSubType.SYSTEM_DEFENSE_CASTLE_FAIL
            or subType == p.MailSubType.SYSTEM_BRONZESPARROWTOWER_REWARD then
                toHeadId = tostring(params.bannerId or 0)
                paramsTemp = params.allianceName or ''
        end
        if not mailInfo.isLang then
            summary = misc.substring(mailInfo.content) or ''
        end
    elseif mailType == p.MailType.REPORT then
        --守
        local isLang = 0
        if subType == p.MailSubType.REPORT_DEFENSE_CASTLE_SUCCESS
            or subType == p.MailSubType.REPORT_DEFENSE_CASTLE_FAIL
            or subType == p.MailSubType.REPORT_DEFENSE_CAMP_FIXED_SUCCESS
            or subType == p.MailSubType.REPORT_DEFENSE_CAMP_FIXED_FAIL
            or subType == p.MailSubType.REPORT_DEFENSE_CAMP_TEMP_SUCCESS
            or subType == p.MailSubType.REPORT_DEFENSE_CAMP_TEMP_FAIL
            or subType == p.MailSubType.REPORT_DEFENSE_RESOURCE_SUCCESS
            or subType == p.MailSubType.REPORT_DEFENSE_RESOURCE_FAIL
            or subType == p.MailSubType.REPORT_DEFENSE_FAMOUS_CITY_SUCCESS
            or subType == p.MailSubType.REPORT_DEFENSE_FAMOUS_CITY_FAIL  then
            if params.attacker ~= nil then
                local attackerNickname = params.attacker.nickname or ''
                local defenderNickname = params.defender.nickname or ''
                local attackerAllianceName = params.attacker.allianceName or ''
                local defenderAllianceName = params.defender.allianceName or ''
                local posName = params.reportInfo.posName
                local isLang = tostring(params.reportInfo.isLang)
                local pos = getPosByTable(params.pos)
                local level = tostring(params.reportInfo.level)
                local attackerBeginArmyCount = tostring(params.reportInfo.attackerBeginArmyCount)
                local attackerEndArmyCount = tostring(params.reportInfo.attackerEndArmyCount)
                local defenderBeginArmyCount = tostring(params.reportInfo.defenderBeginArmyCount)
                local defenderEndArmyCount = tostring(params.reportInfo.defenderEndArmyCount)
                local attackerHeroId = 0
                local attackerHeroLevel = 0
                local defenderHeroId = 0
                local defenderHeroLevel = 0
                -- print("params.attacker.detail.armyList.list", utils.serialize(params.attacker.detail.armyList.list))
                local temp = 0
                for k, v in pairs(params.attacker.detail.armyList.list) do
                    -- print("###params.attacker.detail.armyList.list", utils.serialize(v))
                    temp = v.hero.level
                    if temp <= v.hero.level then
                        attackerHeroId = v.hero.tplId
                        attackerHeroLevel = v.hero.level
                    end 
                end
                temp = 0
                for k, v in pairs(params.defender.detail.armyList.list) do
                    -- print("###params.defender.detail.armyList.list", utils.serialize(v))
                    temp = v.hero.level
                    if temp <= v.hero.level then
                        defenderHeroId = v.hero.tplId
                        defenderHeroLevel = v.hero.level
                    end 
                end
                paramsTemp = attackerNickname .. ',' .. defenderNickname .. ',' .. attackerAllianceName .. ',' .. defenderAllianceName .. ',' .. posName .. ',' .. isLang
                paramsTemp = paramsTemp .. ',' .. pos .. ',' .. level .. ',' .. attackerBeginArmyCount .. ',' .. attackerEndArmyCount .. ',' .. defenderBeginArmyCount .. ',' .. defenderEndArmyCount
                paramsTemp = paramsTemp .. ',' .. attackerHeroId .. ',' .. attackerHeroLevel .. ',' .. defenderHeroId .. ',' .. defenderHeroLevel
            end
        --攻
        elseif subType == p.MailSubType.REPORT_ATTACK_CASTLE_SUCCESS
            or subType == p.MailSubType.REPORT_ATTACK_CASTLE_FAIL
            or subType == p.MailSubType.REPORT_ATTACK_CAMP_FIXED_SUCCESS
            or subType == p.MailSubType.REPORT_ATTACK_CAMP_FIXED_FAIL
            or subType == p.MailSubType.REPORT_ATTACK_CAMP_TEMP_SUCCESS
            or subType == p.MailSubType.REPORT_ATTACK_CAMP_TEMP_FAIL
            or subType == p.MailSubType.REPORT_ATTACK_RESOURCE_SUCCESS
            or subType == p.MailSubType.REPORT_ATTACK_RESOURCE_FAIL
            or subType == p.MailSubType.REPORT_GATHER_BATTLE_SUCCESS
            or subType == p.MailSubType.REPORT_GATHER_BATTLE_FAIL
            or subType == p.MailSubType.REPORT_ATTACK_FAMOUS_CITY_SUCCESS
            or subType == p.MailSubType.REPORT_ATTACK_FAMOUS_CITY_FAIL
            or subType == p.MailSubType.REPORT_OCCUPY_FAMOUS_CITY_SUCCESS
            or subType == p.MailSubType.REPORT_OCCUPY_FAMOUS_CITY_FAIL
            or subType == p.MailSubType.REPORT_OCCUPY_CAMP_FIXED_SUCCESS
            or subType == p.MailSubType.REPORT_OCCUPY_CAMP_FIXED_FAIL 
            or subType == p.MailSubType.REPORT_ATTACK_MONSTER_SUCCESS 
            or subType == p.MailSubType.REPORT_ATTACK_MONSTER_FAIL then
            if params.defender ~= nil then
                local attackerNickname = params.attacker.nickname or ''
                local defenderNickname = params.defender.nickname or ''
                local attackerAllianceName = params.attacker.allianceName or ''
                local defenderAllianceName = params.defender.allianceName or ''
                local posName = params.reportInfo.posName
                local isLang = tostring(params.reportInfo.isLang)
                local pos = getPosByTable(params.pos)
                local level = tostring(params.reportInfo.level)
                local attackerBeginArmyCount = tostring(params.reportInfo.attackerBeginArmyCount)
                local attackerEndArmyCount = tostring(params.reportInfo.attackerEndArmyCount)
                local defenderBeginArmyCount = tostring(params.reportInfo.defenderBeginArmyCount)
                local defenderEndArmyCount = tostring(params.reportInfo.defenderEndArmyCount)
                local attackerHeroId = 0
                local attackerHeroLevel = 0
                local defenderHeroId = 0
                local defenderHeroLevel = 0
                -- print("params.attacker.detail.armyList.list", utils.serialize(params.attacker.detail.armyList.list))
                local temp = 0
                for k, v in pairs(params.attacker.detail.armyList.list) do
                    -- print("###params.attacker.detail.armyList.list", utils.serialize(v))
                    temp = v.hero.level
                    if temp <= v.hero.level then
                        attackerHeroId = v.hero.tplId
                        attackerHeroLevel = v.hero.level
                    end 
                end
                temp = 0
                for k, v in pairs(params.defender.detail.armyList.list) do
                    -- print("###params.defender.detail.armyList.list", utils.serialize(v))
                    temp = v.hero.level
                    if temp <= v.hero.level then
                        defenderHeroId = v.hero.tplId
                        defenderHeroLevel = v.hero.level
                    end 
                end
                paramsTemp = attackerNickname .. ',' .. defenderNickname .. ',' .. attackerAllianceName .. ',' .. defenderAllianceName .. ',' .. posName .. ',' .. isLang
                paramsTemp = paramsTemp .. ',' .. pos .. ',' .. level .. ',' .. attackerBeginArmyCount .. ',' .. attackerEndArmyCount .. ',' .. defenderBeginArmyCount .. ',' .. defenderEndArmyCount
                paramsTemp = paramsTemp .. ',' .. attackerHeroId .. ',' .. attackerHeroLevel .. ',' .. defenderHeroId .. ',' .. defenderHeroLevel
            end
        elseif subType == p.MailSubType.REPORT_PATROL_MAIL then
            --名城名称
            if params.cityInfo ~= nil then
                paramsTemp = params.cityInfo.cityName or ''
            end
        end
    elseif mailType == p.MailType.SCOUT then
        if subType == p.MailSubType.SCOUT_DEFENSE_SUCCESS
            or subType == p.MailSubType.SCOUT_DEFENSE_FAIL then
            if params.attacker ~= nil then
                paramsTemp = params.attacker.nickname or ''
            end
        elseif subType == p.MailSubType.SCOUT_ATTACK_CASTLE_FAIL
            or subType == p.MailSubType.SCOUT_ATTACK_CASTLE_SUCCESS
            or subType == p.MailSubType.SCOUT_ATTACK_FAMOUS_CITY_SUCCESS
            or subType == p.MailSubType.SCOUT_ATTACK_CAMP_FIXED_SUCCESS
            or subType == p.MailSubType.SCOUT_ATTACK_CAMP_TEMP_SUCCESS
            or subType == p.MailSubType.SCOUT_ATTACK_RESOURCE_SUCCESS then
            if params.defender ~= nil then
                paramsTemp = params.defender.nickname or ''
            end
            if subType == p.MailSubType.SCOUT_ATTACK_RESOURCE_SUCCESS then
                local tplId = params.defender.targetTplId
                local tpl = t.mapUnit[tplId]
                -- print("mailType == p.MailType.SCOUT tplId ", tplId)
                local reseName = ''
                if tpl ~= nil then
                    reseName = tpl.name or ''
                    print("mailType == p.MailType.SCOUT reseName ", reseName)
                end
                paramsTemp = paramsTemp .. ',' .. reseName
            end
        end
    elseif mailType == p.MailType.GATHER then
        if subType == p.MailSubType.GATHER_FOOD
            or subType == p.MailSubType.GATHER_WOOD
            or subType == p.MailSubType.GATHER_STONE
            or subType == p.MailSubType.GATHER_IRON then
            --坐标+资源+数量
            local count = 0
            if params.drops ~= nil then
                for _, v in ipairs(params.drops) do
                    count = v.count
                    break
                end
            end
            paramsTemp = getPos(params.pos)
            paramsTemp = paramsTemp .. ',' .. tostring(count)
        end
    elseif mailType == p.MailType.PRIVATE then
        --私人邮件 显示对方的信息
        if subType == p.MailSubType.PRIVATE_ALLIANCE_MEMBER then
            toUid = mailInfo.uid or 0
            toHeadId = tostring(params.fromHeadId or 0)
            toNickname = params.fromNickname or ''
        else
            if user.uid == params.fromUid then
                --我发给对方
                toUid = params.toUid or 0
                toHeadId = tostring(params.toHeadId or 0)
                toNickname = params.toNickname or ''
            elseif user.uid == params.toUid then
                --对方发给我
                toUid = params.fromUid or 0
                toHeadId = tostring(params.fromHeadId or 0)
                toNickname = params.fromNickname or ''
            end
        end
        -- paramsTemp = toUid .. ',' .. toNickname
        summary = misc.substring(mailInfo.content) or ''
    end
    return toUid, toNickname, toHeadId, summary, paramsTemp
end

function mail.sendMailUpdate()
    local updateMails = {}
    for mailType, maxCount in pairs(mail.mailTypeMaxCount) do
        local mails = mail.mails[mailType]
        if mails then
            for k, v in pairs(mails) do
                local syncMail = { id = k }
                local needSync = false
                local isRead = true
                for _, m in ipairs(v.ms) do
                    if not m.isSync then
                        if syncMail.title == nil or syncMail.title == '' then
                            syncMail.type = m.type
                            syncMail.subType = m.subType
                            syncMail.title = m.title
                            syncMail.isLang = m.isLang
                            syncMail.isDraw = m.isDraw
                            syncMail.createTime = m.createTime
                            syncMail.attachmentSize = 0
                            local tabAttachment= misc.deserialize(m.attachment)
                            if type(tabAttachment) == "table" then
                                syncMail.attachmentSize = utils.countTable(tabAttachment)
                            end
                            --邮件列表微设计
                            syncMail.uid, syncMail.nickname, syncMail.headId, syncMail.summary, syncMail.params = mail.setMailMicroDesign(syncMail.type , syncMail.subType, m)
                            -- print('--------------11111 isLang, subType, uid, nickname, headId, summary, params-----------', syncMail.isLang, syncMail.subType, syncMail.uid, syncMail.nickname, syncMail.headId, syncMail.summary, syncMail.params)
                        end
                        if syncMail.createTime < m.createTime then
                            syncMail.createTime = m.createTime
                            syncMail.title = m.title
                            syncMail.isLang = m.isLang
                            syncMail.uid, syncMail.nickname, syncMail.headId, syncMail.summary, syncMail.params = mail.setMailMicroDesign(syncMail.type , syncMail.subType, m)
                            -- print('--------------22222 isLang, subType, uid, nickname, headId, summary, params-----------', syncMail.isLang, syncMail.subType, syncMail.uid, syncMail.nickname, syncMail.headId, syncMail.summary, syncMail.params)
                        end
                        if not m.isRead then
                            isRead = false
                        end
                        if m.subType == p.MailSubType.PRIVATE_MAIL then
                            syncMail.subType = m.subType
                        end
                        m.isSync = true
                        needSync = true
                    end
                end
                if needSync then
                    syncMail.isRead = isRead
                    table.insert(updateMails, { mailId = syncMail.id, type = syncMail.type, subType = syncMail.subType, title = syncMail.title, summary = syncMail.summary, isLang = syncMail.isLang, isRead = syncMail.isRead, isDraw = syncMail.isDraw, createTime = syncMail.createTime, attachmentSize = syncMail.attachmentSize, uid = syncMail.uid, nickname = syncMail.nickname, headId = syncMail.headId, params1 = syncMail.params })
                end
            end
        end
    end
    --print('mail.sendMailUpdate....', utils.serialize(updateMails))
    local ok = agent.sendPktout(p.SC_MAIL_UPDATE, '@@1=[mailId=i,type=i,subType=i,title=s,summary=s,isLang=b,isRead=b,isDraw=b,createTime=i,attachmentSize=i,uid=i,nickname=s,headId=s,params1=s]', updateMails)
    if ok == false then
        local err = 'sendMailUpdate uid=' .. user.uid .. '\n'
        for _, v in pairs(updateMails) do
            --print('sendMailUpdate .... dump... ', v.mailId, v.title, v.type, v.subType, v.isRead, v.createTime, v.attachmentSize,v.headId)
            local mailId = v.mailId or 'nil'
            local type = v.type or 'nil'
            local subType = v.subType or 'nil'
            local isRead = v.isRead or 'nil'
            local isDraw = v.isDraw or 'nil'
            local createTime = v.createTime or 'nil'
            local attachmentSize = v.attachmentSize or 'nil'
            local headId = v.headId or 'nil'
            err = err .. 'mailId=' .. mailId .. ',type=' .. type .. ',subType=' .. subType .. ',createTime=' .. createTime .. ',attachmentSize=' .. attachmentSize .. ',headId=' .. headId .. '\n'
        end
        utils.log(err)
    end
end

function mail.sendMailView(mailId, mailTab)
    print("###mail.sendMailView")
    local mailType = 0
    local subType = 0
    local viewMails = {}
    for _, v in pairs(mailTab.ms) do
        if v.deleteType == 0 or v.deleteType == nil then
            if mailType == 0 then
                mailType = v.type
            end
            if subType == 0 then
                subType = v.subType
            end

            local params1 = v.params

            if subType == p.MailSubType.SYSTEM_ALLIANCE_INVITE
                or subType == p.MailSubType.SYSTEM_ALLIANCE_KICK
                or subType == p.MailSubType.SYSTEM_ALLIANCE_DISBAND
                or subType == p.MailSubType.SYSTEM_ALLIANCE_LEADER_TRANSFER
                or subType == p.MailSubType.SYSTEM_ALLIANCE_LEADER_REPLACE
                or subType == p.MailSubType.SYSTEM_ALLIANCE_TELEPORT_INVITE
                or subType == p.MailSubType.SYSTEM_ALLIANCE_REWARD
                or subType == p.MailSubType.SYSTEM_ALLIANCE_LEVEL_UP
                or subType == p.MailSubType.SYSTEM_ALLIANCE_REWARD_DETAIL 
                or subType == p.MailSubType.SYSTEM_ALLIANCE_OWN_CITY
                or subType == p.MailSubType.SYSTEM_ALLIANCE_LOSE_CITY
                or subType == p.MailSubType.SYSTEM_ALLIANCE_OWN_CITY_OCCUPY
                or subType == p.MailSubType.SYSTEM_ALLIANCE_LOSE_CITY_OCCUPY 
                or subType == p.MailSubType.SYSTEM_ATTACK_CASTLE_SUCCESS
                or subType == p.MailSubType.SYSTEM_PALACE_WAR_GIFT
                or subType == p.MailSubType.SYSTEM_WORLDBOSS_HURT_REWARD
                or subType == p.MailSubType.SYSTEM_WORLDBOSS_KILL_REWARD
                or subType == p.MailSubType.SYSTEM_WORLDBOSS_ALLIANCE_REWARD
                or subType == p.MailSubType.SYSTEM_CITY_HURT_REWARD
                or subType == p.MailSubType.SYSTEM_CITY_KILL_REWARD
                or subType == p.MailSubType.SYSTEM_CITY_ALLIANCE_REWARD
                or subType == p.MailSubType.SYSTEM_CATAPULT_HURT_REWARD
                or subType == p.MailSubType.SYSTEM_CATAPULT_KILL_REWARD
                or subType == p.MailSubType.SYSTEM_CATAPULT_ALLIANCE_REWARD
                or subType == p.MailSubType.SYSTEM_DEFENSE_CASTLE_FAIL then

                local params = misc.deserialize(v.params)
                if params.params1 then
                    params1 = params.params1
                end
            end

            table.insert(viewMails, {
                mailId = v.id,
                title = v.title,
                content = v.content,
                isLang = v.isLang,
                isRead = v.isRead,
                isDraw = v.isDraw,
                createTime = v.createTime,
                attachment = v.attachment,
                params1 = params1,
                reportId = v.reportId,
                })
            v.isRead = true
            v.isSync = true
            v.isDirty = true
        end
    end
    -- print('p.SC_MAIL_VIEW_RESPONSE...mailId viewMails', mailId, utils.serialize(viewMails))
    agent.sendPktout(p.SC_MAIL_VIEW_RESPONSE, '@@1=i,2=i,3=i,4=[mailId=i,title=s,content=s,isLang=b,isRead=b,isDraw=b,createTime=i,attachment=s,params1=s,reportId=i]', mailType, subType, mailId, viewMails)
 end 

 function mail.cs_mail_view(mailType, mailId)
    local mails = mail.mails[mailType]
    if mails == nil then
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local mailTab = mails[mailId]
    if mailTab == nil then
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end

    mail.sendMailView(mailId, mailTab)
 end

 function mail.cs_mail_delete(mailType, mailIdList)
    local mails = mail.mails[mailType]
    if mails == nil then
        return
    end
    local mailList = {}
    local ok = false
    for _,mailId in pairs(mailIdList or {})do
        local mailTab = mails[mailId]
        local mailSubType = 0
        local otherUid = 0
        if mailTab ~= nil then
            for _, v in pairs(mailTab.ms) do
                mailSubType = v.subType
                otherUid = v.otherUid
                v.deleteType = 1
                v.deleteTime = timer.getTimestampCache()
                v.isDirty = true
                --
                mail.mailDeletes[v.id] = v
            end
            if mailSubType ~= 0 then
                mail.setMailParentId(mailSubType, 0, otherUid)
            end
            mails[mailId] = nil
            ok = true
            table.insert(mailList, { mailId = mailId })
        end
    end
    agent.sendPktout(p.SC_MAIL_DELETE_RESPONSE, '@@1=b,2=[mailId=i]', ok, mailList)
 end

 function mail.cs_mail_draw_attachment(mailType, mailIdList)
    local mails = mail.mails[mailType]
    if mails == nil then
        return
    end
    local mailList = {}
    local ok = false
    for _,mailId in pairs(mailIdList or {})do
        local mailTab = mails[mailId]
        if mailTab ~= nil then
            local drops = {}
            for _, v in pairs(mailTab.ms) do
                if v.id == mailId then
                    local isDraw = true
                    if not v.isDraw then
                        local attachment = misc.deserialize(v.attachment)
                        if type(attachment) == "table" then
                            for _, a in pairs(attachment) do
                                local isFind = false
                                for _, d in pairs(drops) do
                                    if d.tplId == a.tplId then
                                        d.count = d.count + a.count
                                        isFind = true
                                    end
                                end
                                if not isFind then
                                    table.insert(drops,a)
                                end
                            end
                        end
                    end
                    --检查背包是否已满
                    if not mail.checkCanDrawAttachment(drops)then
                        print('bag is full, cannot draw mail attachment...', utils.serialize(drops))
                        isDraw = false
                        drops = {}
                        agent.sendNoticeMessage(p.ErrorCode.BAG_FULL_CLEAR, '', 1)
                    end
                    v.isRead = true
                    v.isDraw = isDraw
                    v.isDirty = true
                    v.isSync = false
                    ok = isDraw
                    break
                end
            end
            -- pick item
            if next(drops) then
                local gainType = p.ResourceGainType.MAIL
                bag.pickDropItems(drops, gainType)
                table.insert(mailList, { mailId = mailId })
            end
        end
    end
    mail.sendMailUpdate()
    agent.sendPktout(p.SC_MAIL_DRAW_ATTACHMENT_RESPONSE, '@@1=b,2=[mailId=i]', ok, mailList)
end

function mail.cs_mail_alliance_send(content)
    local content = trie.filter(content)
    agent.queueJob(function()
        local ret = agent.alliance.sendAllianceMail(user.uid, user.info.nickname, user.info.headId, user.info.langType, content)
        -- print('p.CS_MAIL_ALLIANCE_SEND ... ret', ret)
        if ret ~= p.ErrorCode.SUCCESS then
            agent.sendNoticeMessage(ret, '', 1)
        end
        agent.sendPktout(p.SC_MAIL_ALLIANCE_SEND_RESPONSE, ret)
    end)
end

function mail.cs_mail_private_send(uid, content)
    -- print('p.CS_MAIL_PRIVATE_SEND .... to uid, content', uid, content)
    if uid == 0 or content == '' or content == nil then
        agent.sendPktout(p.SC_MAIL_PRIVATE_SEND_RESPONSE, 1)
        return
    end
    --check block
    local blockList = chatStub.blockListMap[uid]
    if blockList and blockList[user.uid] then
        agent.sendPktout(p.SC_MAIL_PRIVATE_SEND_RESPONSE, 2)
        return
    end
    -- check bad word
    content = trie.filter(content)

    agent.queueJob(function()
        local info = agent.map.call_getPlayerInfo(uid)
        local toHeadId = 0
        local toUserLv = 0
        local toNickname = ''
        local toAllianceName = ''
        local toAllianceNickname = ''
        if info ~= nil then
            toHeadId = info.headId
            toUserLv = info.level
            toNickname = info.nickname or toNickname
            toAllianceName = info.allianceName or toAllianceName
            toAllianceNickname = info.allianceNickname or toAllianceNickname
        end
        local allianceName = agent.alliance.allianceName()
        local allianceNickname = agent.alliance.allianceNickname()
        chatStub.cast_privateMail(user.uid, user.info.level, user.info.nickname, user.info.headId, user.info.langType, allianceName, uid, toUserLv, toNickname, toHeadId, toAllianceName, content)
        agent.sendPktout(p.SC_MAIL_PRIVATE_SEND_RESPONSE, 0)
    end)
end

function mail.cs_mail_alliance_share(mailType, mailId)
    local mails = mail.mails[mailType]
    if mails == nil then
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    local mailTab = mails[mailId]
    if mailTab == nil then
        agent.sendNoticeMessage(p.ErrorCode.FAIL, '', 1)
        return
    end
    for _, v in pairs(mailTab.ms) do
        if v.deleteType == 0 and v.id == mailId and v.type == p.MailType.REPORT then
            -- share mail to alliance chat
            local mailParams = misc.deserialize(v.params)
            local chatParams = {uid = v.uid, mailId = mailId, subType = v.subType, param1 = v.title}
            --table.insert(chatParams, )
            agent.alliance.sendAllianceEventToChat(p.ChatSubType.ALLIANCE_SHARE_MAIL, utils.serialize(chatParams))
            agent.sendNoticeMessage(p.ErrorCode.MAIL_ALLIANCE_SHARE_SUCCESS, '', 1)
            break
        end
    end
end

function mail.cs_mail_alliance_share_view(uid, mailId)
    -- load from db
    agent.queueJob(function()
        -- print('p.CS_MAIL_ALLIANCE_SHARE_VIEW ......', uid, mailId)
        local viewMail = {}
        local db = dbo.open(0)
        local rs = db:executePrepare('SELECT id, parentId, uid, otherUid, type, subType, title, content, isLang, isRead, isDraw, createTime, attachment, params, reportId from s_mail where uid = ? and id = ? and deleteType = 0 order by parentId, id', uid, mailId)
        if rs.ok then
            for _, v in ipairs(rs) do
                print('v', utils.serialize(v))
                local m = pubMail.newMailInfo(v)
                local mailTab = {}
                if m.parentId == 0 then
                    mailTab.lastCreateTime = m.createTime
                    mailTab.ms = {}
                    m.parentId = m.id
                    table.insert(mailTab.ms, m)
                    mail.sendMailView(0, mailTab)
                end
                --print('view share mail Load from db ....  ', v.id, v.title, v.type, v.subType, v.createTime, v.params, v.attachment)
                -- table.insert(viewMail, {
                --     mailId = v.id,
                --     title = v.title,
                --     content = v.content,
                --     isLang = v.isLang,
                --     createTime = v.createTime,
                --     attachment = v.attachment,
                --     params1 = v.params,
                --     battleData = v.battleData,
                --     })
                break
            end
        end
        -- send to client
        --agent.sendPktout(p.SC_MAIL_ALLIANCE_SHARE_VIEW_REPONSE, '@@1=[mailId=i,title=s,content=s,isLang=b,createTime=i,attachment=s,params1=s,battleData=s]', viewMail)
    end)
end

return mail
