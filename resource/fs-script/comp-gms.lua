local agent = ...
local t = require('tploader')
local p = require('protocol')
local timer = require('timer')
local utils = require('utils')
local misc = require('libs/misc')
local gmsStub = require('stub/gms')
local user = agent.user
local dict = agent.dict

local pktHandlers = {}

local gms = {
    picked_mail_batches = {},
}

function gms.langIsMatch(tpl, lang)
    if tpl == nil then
        return true
    end
    if tpl == '' then
        return true
    else
        for _, v in ipairs(tpl) do
            if v == lang then
                return true
            end
        end
    end

    return false
end

function gms.versionIsMatch(tpl, version)
    if tpl == nil then
        return true
    end
    local function greater(a1, b1, c1, d1, a2, b2, c2, d2)
        if a1 > a2 then
            return true
        elseif a1 == a2 then
            if b1 > b2 then
                return true
            elseif b1 == b2 then
                if c1 > c2 then
                    return true
                elseif c1 == c2 then
                    if d1 > d2 then
                        return true
                    else
                        return false
                    end
                end
            else
                return false
            end
        else
            return false
        end
    end

    if tpl == '' then
        return true
    else
        local vs = misc.stringSplitInt(version, '.')
        local major, minor, revision, build = vs[1], vs[2], vs[3], vs[4] or 0
        if not major or not minor or not revision or not build then
            return false
        end
        if tpl.opt == '>' then
            return greater(major, minor, revision, build, tpl.major, tpl.minor, tpl.revision, tpl.build)
        elseif tpl.opt == '=' then
            return major == tpl.major and minor == tpl.minor and revision == tpl.revision and build == tpl.build
        elseif tpl.opt == '<' then
            return greater(tpl.major, tpl.minor, tpl.revision, tpl.build, major, minor, revision, build)
        end
    end

    return false
end

function gms.channelIsMatch(tpl, channel)
    if tpl == nil then
        return true
    end
    if tpl == '' then
        return true
    else
        for _, v in ipairs(tpl) do
            if v == channel then
                return true
            end
        end
    end

    return false
end

function gms.levelIsMatch(tpl, level)
    if tpl == nil then
        return true
    end
    if tpl == '' then
        return true
    else
        if tpl.opt == '>' then
            return level > tpl.level
        elseif tpl.opt == '=' then
            return level == tpl.level
        elseif tpl.opt == '<' then
            return level < tpl.level
        end
    end

    return false
end

function gms.loginTimeIsMatch(tpl, loginLogs)
    if tpl == nil then
        return true
    end
    if tpl == '' then
        return true
    else
        for _, v in ipairs(loginLogs) do
            if not (v.logout_time < tpl.startTime or v.login_time > tpl.endTime) then
                return true
            end
        end
    end

    return false
end

function gms.onInit()
    agent.registerHandlers(pktHandlers)
    gms.dbLoad()
end

function gms.onAllCompInit()
    gms.sendNoticeUpdate()
    gms.pickMailBatches()
end

function gms.onClose()
    gms.onSave()
end

function gms.onSave()
    gms.dbSave()
end

function gms.dbLoad()
    gms.picked_mail_batches = dict.get('gms.picked_mail_batches') or {}
end

function gms.dbSave()
    local now = timer.getTimestampCache()
    local picked_mail_batches = {}
    for k, v in pairs(gms.picked_mail_batches) do
        if now - v < 60 * 24 * 3600 then
            picked_mail_batches[k] = v
        end
    end
    dict.set('gms.picked_mail_batches', picked_mail_batches)
end

function gms.sendMarquee(marquee)
    -- match 
    -- user.info.langType
    if not gms.langIsMatch(marquee.matchLang_, user.info.langType) then
        --print('lang not match')
        return
    end
    -- user.info.channel
    if not gms.channelIsMatch(marquee.matchChannel_, user.info.channel) then
        --print('channel not match')
        return
    end
    -- user.version
    if not gms.versionIsMatch(marquee.matchVersion_, user.version) then
        --print('version not match', marquee.matchVersion, user.version)
        return
    end
    -- user.info.level
    if not gms.levelIsMatch(marquee.matchLevel_, user.info.level) then
        --print('level not match')
        return
    end

    local msg = marquee[user.getLangKey('msg_')]
    agent.sendPktout(p.SC_GMS_SEND_MARQUEE,  '@@1=s',  msg)
    -- print('gms.sendMarquee uid, nickname, msg = ', user.uid, user.info.nickname, msg)
end

function gms.sendNoticeUpdate()
    local now = timer.getTimestampCache()
    local list = {}
    for _, v in pairs(gmsStub.notices) do
        if v.expireTime > now then
            local title = v[user.getLangKey('title_')]
            local content = v[user.getLangKey('content_')]
            if title and content then
                table.insert(list, { id = v.id, priority = v.priority, title = title,  content = content, publishTime = v.publishTime })
            else
                --utils.log('gms.sendNoticeUpdate title=nil, key=' .. user.getLangKey('title_'))
            end
        end
    end
    if next(list) ~= nil then
        agent.sendPktout(p.SC_GMS_NOTICE_UPDATE,  '@@1=[id=i,priority=i,title=s,content=s,publishTime=i]',  list)
    end
end

function gms.pickMailBatches()
    local now = timer.getTimestampCache()
    local logs = agent.misc.data.loginLogs
    for _, v in pairs(gmsStub.mailBatches) do
        repeat
            if not v.active then
                break
            end
            if v.openTime > now or now > v.closeTime then
                break
            end

            if gms.picked_mail_batches[v.id] then
                break
            end

            -- match
            -- user.info.langType
            if not gms.langIsMatch(v.matchLang_, user.info.langType) then
                --print('lang not match')
                break
            end
            -- user.info.channel
            if not gms.channelIsMatch(v.matchChannel_, user.info.channel) then
                --print('channel not match')
                break
            end
            -- user.version
            if not gms.versionIsMatch(v.matchVersion_, user.version) then
                --print('version not match')
                break
            end
            -- user.info.level
            if not gms.levelIsMatch(v.matchLevel_, user.info.level) then
                --print('level not match')
                break
            end
            -- user.loginLogs
            if not gms.loginTimeIsMatch(v.matchLoginTime_, logs) then
                --print('loginTime not match', v.matchLoginTime)
                break
            end

            gms.picked_mail_batches[v.id] = now

            local dropList = {}
            for tplId, count in pairs(v.dropList) do
                table.insert(dropList, t.createDropItem(tplId, count))
            end
            local mailSubType = p.MailSubType.SYSTEM_OPERATION
            local title = v[user.getLangKey('title_')]
            local content = v[user.getLangKey('content_')]
            local attachments = utils.serialize(dropList)
            local param = utils.serialize({ })
            local createTime = v.mailTime
            agent.mail.saveMailRaw(user.uid, 0, p.MailType.SYSTEM, mailSubType, title, content, createTime, false, false, attachments, param, 0)
            -- print('gms.pickMailBatches uid, nickname, msg = ', user.uid, user.info.nickname, title)
        until true
    end
end


return gms
