local M = {}
local p = require('protocol')
local cluster = require('cluster')
local dbo = require('dbo')
local utils = require('utils')
local timer = require('timer')
local framework = require('framework')
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()

local loginService = require('loginService')
local daySeconds = 24 * 60 * 60

local rawService
local impl = {}

local saving = false
local closing = false
local waiting = false
local dataCount = 0
local dataQueue = {}

for k,v in pairs(p.DataClassify) do
    dataQueue[v] = {}
end

local TopClasify = {
    MAIL = p.DataClassify.MAIL,
}

-- framework node shutdown
framework.beforeShutdown(function(cb)
    M.queueJob(function()
        print('framework.beforeShutdown data')
        closing = true
        M.saveInternal()
        cb()
    end, cb)
end)

local function getDataTable(classify)
    local tableName = nil
    local updateFields = nil
    if classify == p.DataClassify.MAIL then
        tableName = 's_mail'
    elseif classify == p.DataClassify.MAIL_UPDATE then
        tableName = 's_mail'
        updateFields = { 'isRead', 'isDraw', 'deleteType', 'createTime', 'deleteTime' }
    elseif classify == p.DataClassify.USER_UPDATE then
        tableName = 's_user'
        updateFields = { 'username', 'nickname', 'headId', 'camp', 'level', 'exp', 'silver', 'gold', 'food', 'wood', 'iron', 'stone', 'stamina', 'skillPoint', 'staminaBuyCount', 'lastStaminaRecoverTime', 'goldCharged', 'langType', 'attackWins', 'attackLosses', 'defenseWins', 'defenseLosses', 
        'scoutCount', 'kills', 'losses', 'heals', 'lordPower', 'troopPower', 'buildingPower', 'sciencePower', 'trapPower', 'heroPower', 'totalPower', 'captives', 'lastLoginTimestamp', 'castleLevel', 'allianceId', 'allianceName', 
        'allianceNickname', 'allianceBannerId', 'x', 'y', 'isJoinRank', 'babelMaxLayer', 'babelTimestamp', 'bronzeMaxScore', 'bronzeTodayScore', 'bronzeTodayTimestamp' , 'isSetName' , 'storyId' ,'staminaPropUseCount' , 'energy' , 'energyBuyCount' , 'lastEnergyRecoverTime','vipLevel'}
    elseif classify == p.DataClassify.DICT then
        tableName = 's_dict'
        updateFields = { 'v' }
    elseif classify == p.DataClassify.BAG then
        tableName = 's_bag'
        updateFields = { 'itemId', 'count', 'data', 'ext1', 'ext2' }
    elseif classify == p.DataClassify.HERO then
        tableName = 's_hero'
        updateFields = { 'heroId', 'exp', 'level', 'star', 'soulLevel', 'physical', 'physicalRecoveryTimeStamp', 'slotNum', 'isLock', 'skill', 'soul' }
    elseif classify == p.DataClassify.QUEST then
        tableName = 's_quest'
        updateFields = { 'total', 'progress', 'createTime', 'finishTime', 'drawTime' }
    elseif classify == p.DataClassify.SEVEN_TARGET then
        tableName = 's_seven_target'
        updateFields = { 'total', 'progress', 'createTime', 'finishTime', 'drawTime' }
    elseif classify == p.DataClassify.ARENA_RECORD then
        tableName = 's_arena_record'
        updateFields = { 'id', 'uid', 'nickname', 'level', 'headId', 'allianceName', 'allianceNickname', 'bannerId', 'toUid', 'toNickname', 'toLevel', 'toHeadId', 'toAllianceName', 'toAllianceNickname', 'toBannerId', 'isWin', 'isAttacker', 'changRank', 'createTime', 'battleData', 'canRevenge', 'toPower' }
    elseif classify == p.DataClassify.TRANSPORT_RECORD then
        tableName = 's_transport_record'
        updateFields = { 'id', 'uid', 'headId', 'nickName', 'toUid', 'toHeadId', 'toNickName', 'transportType', 'carry', 'arriveType', 'arriveTime'}
    else
        print('wrong data classify = ', classify)
    end
    return tableName, updateFields
end

function M.start()
    rawService = cluster.createService('data', impl)
end

function M.queueJob(threadFun, errorFun)
    jobExecutor:queue(threadFun, errorFun)
end

function M.queueErrorFun()
    saving = false
end

function M.save()
    if dataCount == 0 or waiting then
        return
    end
    M.queueJob(function()
        M.saveInternal()
    end, M.queueErrorFun)
end


function M.saveInternal()
    --print('dataCount =', dataCount)

    if dataCount == 0 or waiting then
        saving = false
        return
    end

    saving = true
    local clasify = 0
    local toSaveList = {}

    -- ============    top clasify    ===========
    if not next(toSaveList) then
        for k,v in pairs(TopClasify) do
            local max = 1
            local queue = dataQueue[v]
            local count = #queue
            if count >= 50 then
                max = 50
            elseif count >= 10 then
                max = 10
            end
            if count > 0 and closing then
                max = 200
            end

            if max > 1 then
                clasify = v
                repeat
                    dataCount = dataCount - 1
                    local data = table.remove(queue, 1)
                    table.insert(toSaveList, data)
                until #toSaveList >= max or #queue == 0
                break
            end
        end

        if not next(toSaveList) then
            for k,v in pairs(TopClasify) do
                local queue = dataQueue[v]
                if #queue > 0 then
                    clasify = v
                    dataCount = dataCount - 1
                    local data = table.remove(queue, 1)
                    table.insert(toSaveList, data)
                    break
                end
            end
        end
    end

    -- ============    other clasify    ===========
    -- save 50 or 200 per time
    if not next(toSaveList) then
        for k,v in pairs(p.DataClassify) do
            local max = 1
            local queue = dataQueue[v]
            local count = #queue
            if count >= 200 then
                max = 200
            elseif count >= 50 then
                max = 50
            end
            if count > 0 and closing then
                max = 200
            end

            if max > 1 then
                clasify = v
                repeat
                    dataCount = dataCount - 1
                    local data = table.remove(queue, 1)
                    table.insert(toSaveList, data)
                until #toSaveList >= max or #queue == 0
                break
            end
        end
    end

    -- save 10 per time
    if not next(toSaveList) then
        for k,v in pairs(p.DataClassify) do
            local max = 1
            local queue = dataQueue[v]
            local count = #queue
            if count >= 10 then
                max = 10
                clasify = v
                repeat
                    dataCount = dataCount - 1
                    local data = table.remove(queue, 1)
                    table.insert(toSaveList, data)
                until #toSaveList >= max or #queue == 0
                break
            end
        end
    end
    -- save single
    if not next(toSaveList) then
        for k,v in pairs(p.DataClassify) do
            local queue = dataQueue[v]
            if #queue > 0 then
                clasify = v
                dataCount = dataCount - 1
                local data = table.remove(queue, 1)
                table.insert(toSaveList, data)
                break
            end
        end
    end

    local saveCount = #toSaveList
    if saveCount > 0 then
        local tableName, updateFields = getDataTable(clasify)
        -- print('----------tableName, clasify, updateFields', tableName, clasify, utils.serialize(updateFields))
        -- print('----------tableName, clasify, toSaveList', tableName, clasify, utils.serialize(toSaveList))
        if tableName then
            -- if saveCount > 1 then
            --     print('save data =', tableName, saveCount, dataCount)
            -- end
            local db = dbo.open(0)
            if not updateFields then
                local rs = db:insertBatch(tableName, toSaveList)
                if rs.ok and rs.affectedRows > 0 then
                    M.handleNotify(clasify, toSaveList)
                end
                if not rs.ok then
                    print(debug.traceback())
                    utils.log('fail data: ' .. utils.toJson(toSaveList))
                end
                if rs.warningCount > 0 then
                    utils.log('warning data: ' .. utils.toJson(toSaveList))
                end
            else
                local rs = db:insertBatch(tableName, toSaveList, updateFields)
                if rs.ok and rs.affectedRows > 0 then
                    M.handleNotify(clasify, toSaveList)
                end
                if not rs.ok then
                    print(debug.traceback())
                    utils.log('fail data: ' .. utils.toJson(toSaveList))
                end
                if rs.warningCount > 0 then
                    utils.log('warning data: ' .. utils.toJson(toSaveList))
                end
            end
        end
    end

    saving = dataCount > 0

    if closing then
        M.saveInternal()
    elseif saving then
        M.save()
    end
end

function M.handleNotify(classify, list)
    for k,v in pairs(list) do
        if classify == p.DataClassify.MAIL then
            loginService.castMailReload(v.uid, v.id)
        elseif classify == p.DataClassify.MAIL_UPDATE then
            --
        elseif classify == p.DataClassify.DICT then
            --
        elseif classify == p.DataClassify.BAG then
            --
        elseif classify == p.DataClassify.BLACKSMITH then
            --
        elseif classify == p.DataClassify.QUEST then
            --
        elseif classify == p.DataClassify.SEVEN_TARGET then
            --
        elseif classify == p.DataClassify.ARENA_RECORD then
            loginService.castArenaRecordReload(v.uid, v.id)
        elseif classify == p.DataClassify.TRANSPORT_RECORD then
            loginService.castTransportRecordReload(v.uid, v.id)
        end
    end
end

function M.isDataSaving(uid)
    for k,v in pairs(p.DataClassify) do
        local queue = dataQueue[v]
        for k2,v2 in pairs(queue) do
            if not v2.aid and v2.uid and v2.uid == uid then
                return true
            end
        end
    end
    return false
end

function M.setWaiting(yes)
    if yes then
        waiting = true
    else
        waiting = false
        M.save()
    end
end

function M.appendMailInfo(info)
    if info.id == 0 then
        info.id = utils.createItemId()
    end
    local classify = p.DataClassify.MAIL
    local dataRow = {  id = info.id, parentId = info.parentId, uid = info.uid, otherUid = info.otherUid, type = info.type, 
    subType = info.subType, title = info.title, content = info.content, attachment = info.attachment, params = info.params, 
    reportId = info.reportId, isLang = info.isLang, isRead = info.isRead, isDraw = info.isDraw, deleteType = info.deleteType, 
    createTime = info.createTime, deleteTime = info.deleteTime }
    -- local dataRow = {  id = info.id, parentId = info.parentId, uid = info.uid, otherUid = info.otherUid, type = info.type, subType = info.subType, title = info.title, content = info.content, attachment = info.attachment, params = info.params, battleData = info.battleData, isLang = info.isLang, isRead = info.isRead, isDraw = info.isDraw, deleteType = info.deleteType, createTime = info.createTime, deleteTime = info.deleteTime }
    impl.appendData(classify, dataRow)
end

function M.appendData(classify, dataRow)
    impl.appendData(classify, dataRow)
end

function M.appendDataList(classify, dataRowList)
    impl.appendDataList(classify, dataRowList)
end

--
-- data service api implement
--

function impl.appendData(classify, dataRow)
    -- print('impl.appendData ...', utils.serialize(dataRow))
    -- print('impl.appendData ...', utils.serialize(dataRow))
    local list = dataQueue[classify]
    if list and dataRow then
        table.insert(list, dataRow)
        dataCount = dataCount + 1

        if not saving then
            --print('start data saving ...')
            saving = true
            M.save()
        end
    end
end

function impl.appendDataList(classify, dataRowList)
    -- print('impl.appendDataList ...', classify,utils.serialize(dataRowList))
    local list = dataQueue[classify]
    if list and dataRowList then
        for k,dataRow in pairs(dataRowList) do
            table.insert(list, dataRow)
            dataCount = dataCount + 1
        end

        if dataCount > 0 and not saving then
            --print('start data saving ...')
            saving = true
            M.save()
        end
    end
end

return M

