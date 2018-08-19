local agent = ...
local p = require('protocol')
local t = require('tploader')
local dbo = require('dbo')
local trie = require('trie')
local utils = require('utils')
local misc = require('libs/misc')
local mapService
local user = agent.user
local alliance = agent.alliance
local propertyList = agent.property.list 
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()
local marketStub = require('stub/market')
local dataStub = require('stub/data')


local market = {
	
}

--运输记录
local recordInfo = {
    id = 0,                 --id
    uid = 0,                --玩家UID
    headId = 0,             --玩家头像
    nickname = '',          --玩家昵称

    toUid = 0,               --对手UID
    toHeadId = 0,             --对手头像
    toNickname = '',          --对手昵称

    transportType = 0,       --运输类型(0:传送， 1:接受)
    carry = '',                --运输的资源
    arriveType = 0,            --到达类型(0:失败， 1:到达   , 2:运输中)
    arriveTime = 0,            --到达时间
    isSync = false,
    isDirty = false,        
}

local RECORD_MAX = 30
local marketRecord = {}  --map<id, recordInfo>


function recordInfo:new(o)
    o = o or {}
    o.isSync = false
    o.isDirty = false
    setmetatable(o, self)
    self.__index = self
    return o
end


function market.onInit()
    market.dbLoad()
end

function market.onAllCompInit()
    market.sendMarketRecord()
end

function market.onReady()
    mapService = agent.map.mapService
end

function market.onClose()
    market.dbSave()
end

function market.onSave()
    market.dbSave()
end

function market.dbLoad()
    local db = dbo.open(0)
    local rs = db:executePrepare('SELECT id, uid, headId, nickName, toUid, toHeadId, toNickName, transportType, carry, arriveType, arriveTime from s_transport_record where uid = ? order by id desc limit ?', user.uid, RECORD_MAX)
    if rs.ok then
        for _, row in ipairs(rs) do
            local info = recordInfo:new(row)
            -- print("###market.dbLoad...", utils.serialize(info))
            table.insert(marketRecord, info)
        end
    end
    -- print("###market.dbLoad...", utils.serialize(marketRecord))
end

function market.dbSave()
end

function market.reloadRecordById(recordId)
    agent.queueJob(function()
        -- print('arena.reloadRecordById ......uid, recordId', recordId)
        print('arena.reloadRecordById ......record', utils.serialize(marketRecord))
        local db = dbo.open(0)
        local rs = db:executePrepare('SELECT id, uid, headId, nickName, toUid, toHeadId, toNickName, transportType, carry, arriveType, arriveTime from s_transport_record where uid = ? and id = ?', user.uid, recordId)
        if rs.ok then
            local info = {}
            for _, row in ipairs(rs) do
                info = recordInfo:new(row)
            end
            local isOldRecord = false
            for _, v in ipairs(marketRecord) do
                if v.id == info.id then
                    v.arriveType = info.arriveType
                    v.arriveTime = info.arriveTime
                    v.isSync = false
                    isOldRecord = true
                end
            end
            if not isOldRecord then
                table.insert(marketRecord, 1, info)
                while #marketRecord > RECORD_MAX do
                    -- print("remove , ..  ", utils.serialize(marketRecord[#marketRecord]))
                    table.remove(marketRecord)
                    -- print("after  remove , ..  ", utils.serialize(marketRecord[#marketRecord]))
                end
            end
        end
        market.sendMarketRecord()
    end)

end

-- 同一个盟友只能在同一时间运输一次
function market.checkMemberOneTransport(uid)
    for _, v in ipairs(marketRecord) do
        if v.toUid == uid and  v.transportType == p.TransportType.TRANSMIT and p.TransportArriveType.TRANSPORTING == v.arriveType then
            return true            
        end
    end
    return false
end

function market.cs_transport(toUid, list, session)
    local function TransportResponse(result)
        print("###p.CS_TRANSPORT......result ", result)
        agent.sendPktout(p.SC_TRANSPORT_RESPONSE, result)
    end
    local tempList = {}       --税收之前的数量
    local tempList1 = {}     --税收之后的数量
    local totalCount = 0
    if toUid == user.uid then
        print("###p.CS_TRANSPORT... cannot transport yourself!!!", toUid)
        agent.sendNoticeMessage(p.ErrorCode.TRANSPORT_FAIL, '', 1)
        TransportResponse(false)
        return
    end
    for _,v in pairs(list) do
        local tplId, count = v.tplId, v.count
        print("tplId, count", tplId, count)
        -- 1.判断资源是否足够
        if not user.isResourceEnough(tplId, count) then
            print("###p.CS_TRANSPORT... resource is not enough, tplId, count  ", tplId, count)
            agent.sendNoticeMessage(p.ErrorCode.RESOURCE_IS_NOT_ENOUGH, '', 1)
            TransportResponse(false)
            return
        end
        tempList[tplId] = count
        tempList1[tplId] = math.floor(count * (1 - propertyList.transportTaxPct)) 
        totalCount = totalCount + count
    end
    -- 1.判断是否同盟玩家，市场是否开启
    if not alliance.info.memberList[toUid] or not alliance.info.memberList[toUid].marketIsOpen then
        print("###p.CS_TRANSPORT... market is not open or is not my alliance")
        agent.sendNoticeMessage(p.ErrorCode.TRANSPORT_FAIL, '', 1)
        TransportResponse(false)
        return
    end
    -- 2.判断运输数量是否超过限制
    if totalCount > propertyList.transportNum then
        print("###p.CS_TRANSPORT... transportCount is exceed", propertyList.transportNum)
        agent.sendNoticeMessage(p.ErrorCode.TRANSPORT_IS_EXCEED, '', 1)
        TransportResponse(false)
        return
    end
    -- 3.判断运输数量是否为0
    if totalCount <= 0 then
        print("###p.CS_TRANSPORT... transportCount is zero, totalCount", totalCount)
        TransportResponse(false)
        return
    end

    -- 4.判断是否已经向改玩家运输
    if market.checkMemberOneTransport(toUid) then
        print("###p.CS_TRANSPORT... can not transport member at the same time ", toUid)
        agent.sendNoticeMessage(p.ErrorCode.TRANSPORT_MEMBER_ONCE_LIMIT, '', 1)
        TransportResponse(false)
        return
    end
    -- 5.开始运输
    agent.queueJob(function()
        local ok = mapService:call("transport", toUid, tempList1)
        if ok then
            -- 6.扣资源
            local tempList1 = {}
            for k,v in pairs(tempList) do
                -- 扣资源
                user.removeResource(k, v, p.ResourceConsumeType.TRANSPORT, 0, true)
            end
            TransportResponse(true)
        else
            agent.sendNoticeMessage(p.ErrorCode.TRANSPORT_FAIL, '', 1)
            TransportResponse(false)
        end
    end)
end

function market.sendMarketRecord()
    if #marketRecord > 0 then
        local tempList = {}
        for _, v in ipairs(marketRecord) do
            if not v.isSync then
                print("###market.sendMarketRecord", utils.serialize(v))
                local resourceTable = misc.deserialize(v.carry)
                table.insert(tempList, { id=v.id, uid=v.uid, headId=v.headId, nickName=v.nickName, toUid=v.toUid, toHeadId=v.toHeadId, toNickName=v.toNickName, transportType=v.transportType, food=resourceTable.food,wood=resourceTable.wood,iron=resourceTable.iron,stone=resourceTable.stone,arriveType=v.arriveType, arriveTime=v.arriveTime })
                v.isSync = true
            end
        end
        -- print('p.CS_MARKET_TRANSPORT_RECORD...data', data)
        agent.sendPktout(p.SC_MARKET_TRANSPORT_RECORD_UPDATE, '@@1=[id=i,uid=i,headId=i,nickName=s,toUid=i,toHeadId=i,toNickName=s,transportType=i,food=i,wood=i,iron=i,stone=i,arriveType=i,arriveTime=i]', tempList)
    end
end

return market