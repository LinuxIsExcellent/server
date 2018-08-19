local agent = ...
local p = require('protocol')
local t = require('tploader')
local event = require('libs/event')
local utils = require('utils')

local user = agent.user
local bag = agent.bag
local cdlist = agent.cdlist
local dict = agent.dict

local pktHandlers = {}

local buff = {
    cityList = {},

    --events
    evtBuffAdd = event.new(), --()
    evtBuffRemove = event.new(), --(type)
}

local buffInfo = {
    type = 0,
    cdId = 0,
    param1 = 0,
    attr = {},
    sync = false,
}

function buffInfo:new(o)
    o = o or {}
    if o.attr == nil then o.attr = {} end
    setmetatable(o, self)
    self.__index = self
    return o
end

function buffInfo:cityCb(isInit)
    self.cdId = 0
    self.param1 = 0
    self.attr = {}
    self.sync = false
    buff.evtBuffRemove:trigger(self.type)
    if isInit ~= true then
        buff.sendCityBuffListUpdate()
        agent.map.syncBuffList()
    end
end

function buff.addCityBuff(type, time, param1, attr)
    -- print('buff.addCityBuff...type, time, param1, attr', type, time, param1, utils.serialize(attr))
    if param1 == nil then
        param1 = 0
    end
    if attr == nil or attr == '' then
        attr = {}
    end
    local info = buff.cityList[type]
    if info and cdlist.getCD(info.cdId) then
        if param1 == info.param1 then
            cdlist.addEndTime(info.cdId, time)
            cdlist.sendCDListUpdate()
        elseif param1 > info.param1 then
            info.cdId = cdlist.addCD(type, time)
            info.param1 = param1
        else
            return
        end
        info.sync = false
    else
        local info = buffInfo:new({ type = type, param1 = param1, attr = attr })
        info.cdId = cdlist.addCD(type, time, info.cityCb, info)
        buff.cityList[type] = info
    end
    buff.sendCityBuffListUpdate()
    buff.evtBuffAdd:trigger()
    agent.map.syncBuffList()
end


function buff.remove(type)
    local info = buff.cityList[type]
    if info then
        info.cdId = 0
        info.param1 = 0
        inf.attr = {}
        info.sync = false
        buff.evtBuffRemove:trigger(type)
        buff.sendCityBuffListUpdate()
        agent.map.syncBuffList()
    end
end

function buff.onInit()
    agent.registerHandlers(pktHandlers)
    buff.dbLoad()
end

function buff.onAllCompInit()
    for _, info in pairs(buff.cityList) do
        local cd = cdlist.getCD(info.cdId)
        if not cd or cd:isFinish() then
            info:cityCb(true)
        else
            cd:setCb(info.cityCb, info)
        end
    end
    buff.sendCityBuffListUpdate()
end

function buff.onClose()
    buff.dbSave()
end

function buff.onSave()
    buff.dbSave()
end

function buff.dbLoad()
    local cityList = dict.get("buff.cityList")
    if cityList then
        for _, v in pairs(cityList) do
            local info = buffInfo:new({ type = v.type, cdId = v.cdId, param1 = v.param1, attr = v.attr })
            buff.cityList[v.type] = info
        end
    end
end

function buff.dbSave()
    local cityList = {}
    for k, v in pairs(buff.cityList) do
        if v.cdId > 0 then
            local temp = {}
            temp.type = v.type
            temp.cdId = v.cdId
            temp.param1 = v.param1
            temp.attr = v.attr
            table.insert(cityList, temp)
        end
    end
    dict.set("buff.cityList", cityList)
end

function buff.sendCityBuffListUpdate()
    local cityList = {}
    for type, info in pairs(buff.cityList) do
        if not info.sync then
            local attr = utils.serialize(info.attr)
            table.insert(cityList, { type = info.type, cdId = info.cdId, param1 = info.param1, attr = attr })
            info.sync = true
        end
    end
    -- print('buff.sendCityBuffListUpdate...cityList', utils.serialize(cityList))
    if next(cityList) ~= nil then
        agent.sendPktout(p.SC_BUFF_LIST_UPDATE, '@@1=[type=i,cdId=i,param1=i,attr=s]', cityList)
    end
end

return buff
