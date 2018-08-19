local p = require('protocol')
local cluster = require('cluster')
local utils = require('utils')

local dataStub = {
    rawStub,
}

local subscriber = {
}

local rawStub

function dataStub.connectService()
    rawStub = cluster.connectService('data@cs')
    dataStub.rawStub = rawStub
    rawStub:setSubscriber(subscriber)
end

function dataStub.appendData(classify, dataRow)
    rawStub:cast_appendData(classify, dataRow)
end

function dataStub.appendDataList(classify, dataRowList)
    if #dataRowList > 0 then
        rawStub:cast_appendDataList(classify, dataRowList)
    end
end

function dataStub.appendMailInfo(info)
    if info.id == 0 then
        info.id = utils.createItemId()
    end
    local classify = p.DataClassify.MAIL
    local dataRow = {  id = info.id, parentId = info.parentId, uid = info.uid, otherUid = info.otherUid, type = info.type, 
    subType = info.subType, title = info.title, content = info.content, attachment = info.attachment, params = info.params, 
    reportId = info.reportId, isLang = info.isLang, isRead = info.isRead, isDraw = info.isDraw, deleteType = info.deleteType, 
    createTime = info.createTime, deleteTime = info.deleteTime }
    rawStub:cast_appendData(classify, dataRow)
end

function dataStub.appendMailUpdateList(infoList)
    if #infoList == 0 then
        return
    end
    local classify = p.DataClassify.MAIL_UPDATE
    local list = {}
    for k,info in pairs(infoList) do
        local dataRow = {  id = info.id, parentId = info.parentId, uid = info.uid, otherUid = info.otherUid, 
        type = info.type, subType = info.subType, title = info.title, content = info.content, attachment = info.attachment, 
        params = info.params, reportId = info.reportId, isLang = info.isLang, isRead = info.isRead, 
        isDraw = info.isDraw, deleteType = info.deleteType, createTime = info.createTime, deleteTime = info.deleteTime }
        table.insert(list, dataRow)
    end
    rawStub:cast_appendDataList(classify, list)
end

return dataStub
