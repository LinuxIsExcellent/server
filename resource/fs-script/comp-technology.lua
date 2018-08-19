local agent = ...
local p = require('protocol')
local t = require('tploader')
local timer = require('timer')
local utils = require('utils')
local event = require('libs/event')
local logStub = require('stub/log')

local user = agent.user
local dict = agent.dict

local sanRecoverTime = 0

local pktHandlers = {}

local technology = {
    --events
    techTreeList = {}, --map<type, techTreeInfo> 
    evtDoResearch = event.new(), --(tplId)
    evtResearchFinished = event.new(), --(tplid)
    evtAddLevel = event.new(), --(tplId, level)
    evTechnologyUpdate = event.new(),    --(tplId, level)
}

local techGroupInfo = {
    groupId = 0,
    tplId = 0,
    level = 0,
    sync = false,
}

local techTreeInfo = {
    type = 0,
    techGroupList = {}, --map<groupId,techGroupInfo>
    sync = false,
}

function techGroupInfo:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end

function techTreeInfo:new(o)
    o = o or {}
    if o.techGroupList == nil then o.techGroupList = {} end
    setmetatable(o, self)
    self.__index = self
    return o
end


function technology.onInit()
    agent.registerHandlers(pktHandlers)
    technology.dbLoad()
    if user.newPlayer then
        
    end
end

function technology.onAllCompInit()
    technology.sendTechnologyInfosUpdate()
end

function technology.onClose()
    technology.dbSave()
end

function technology.onSave()
    technology.dbSave()
end

function technology.onTimerUpdate(timerIndex)

end

function technology.dbLoad()
	technology.techTreeList = dict.get("technology.techTreeList") or {}
end

function technology.dbSave()
	dict.set("technology.techTreeList",technology.techTreeList) 
end

function technology.checkUnlockTechnologyTree(level)
    local tpl = t.techTreeUnlock
    local type = 0
    for _,v in pairs(tpl) do
        if level >= v.unlockLevel then
            type = v.type
        end
    end
    if type ~= 0 then
        technology.unlockTechnologyTree(type)
        print('type',type)    
    end
end

function technology.unlockTechnologyTree(type)
    local treeList = technology.techTreeList
    local tree = t.techTreeList[type] 
    if not treeList[type] then
        local techGroupList = {}
        for _,v in pairs(tree) do
            local grouplist = v.grouplist
            for _,group in pairs(grouplist) do
                local tplId = technology.getTechIdByGroupAndLevel(group,0)
                 techGroupList[group] = techGroupInfo:new({
                    groupId = group, 
                    tplId = tplId,
                    level = 0,
                    sync = false,
                    })
            end   
        end 

        technology.techTreeList[type] = techTreeInfo:new({
                type = type,
                techGroupList = techGroupList,
                sync = false,
                })

        technology.sendTechnologyInfosUpdate()
    end   
end

 function technology.getLevelCount()
    -- local count = 0
    -- for treeInfo
    --  for group, level in pairs(technology.techGroupList) do
    --     count = count + level
    --  end
    --  return count
 end

function technology.getLevelByGroup(type,groupId)
    local treeInfo = technology.techTreeList[type]
    local level = 0
    if treeInfo then
        local groupInfo = treeInfo.techGroupList[groupId]
        if groupInfo then
           level = groupInfo.level
        end
    end
    return level
end

function technology.getTechIdByGroup(type,groupId)
    local treeInfo = technology.techTreeList[type]
    local tplId = 0
    if treeInfo then
        local groupInfo = treeInfo.techGroupList[groupId]
        if groupInfo then
           tplId = groupInfo.tplId
        end
    end
    return tplId
end

function technology.getNextLevelTechIdByGroup(type,groupId)
    local level = technology.getLevelByGroup(type,groupId)
    local group = t.techGroup[groupId]
    local nextLevelId = 0
    for _,v in pairs(group) do
        if v.level == level then
            nextLevelId = v.nextLevel
        end
    end
    return nextLevelId
end

function technology.getMaxLevelByGroup(type,groupId)
    local level = technology.getLevelByGroup(type,groupId)
    local group = t.techGroup[groupId]
    local maxLevel = 0
    for _,v in pairs(group) do
        if v.level == level then
            maxLevel = v.maxLevel
        end
    end
    return maxLevel
end

function technology.getTechIdByGroupAndLevel(groupId,level)
    local group = t.techGroup[groupId]
    local id = 0
    --print('groupId..',groupId)
    if group then
        for _,v in pairs(group) do
            if v.level == level then
                id = v.id
            end
        end
    end
    return id
end

function technology.getTypeByGroup(groupId)
    --print('groupId..',groupId)
    local type = 0
    local group = t.techGroup[groupId]
    if group then
        for _,v in pairs(group) do
            type = v.type
        end
    end
    return type
end

function technology.addLevel(type,groupId) 
    local treeInfo = technology.techTreeList[type]
    local ok = false
    if treeInfo then
        local groupInfo = treeInfo.techGroupList[groupId]
        if not groupInfo then
            local tplId = technology.getTechIdByGroupAndLevel(groupId,0)  
            technology.techTreeList[type].techGroupList[groupId] = techGroupInfo:new({
            groupId = groupId,
            tplId = tplId,
            level = 0,
            sync = false,
            })
           ok = true
        else
            local level = groupInfo.level
            local maxLevel = technology.getMaxLevelByGroup(type,groupId)
            if level < maxLevel then
                level = level + 1
                local tplId = technology.getTechIdByGroupAndLevel(groupId,level)
                technology.techTreeList[type].techGroupList[groupId] = techGroupInfo:new({
                    groupId = groupId,
                    tplId = tplId,
                    level = level,
                    sync = false,
                })
            end
            ok = true
        end
    end
    if ok then
        technology.sendTechnologyInfosUpdate() 
        technology.evTechnologyUpdate:trigger()  
        --technology.evtAddLevel:trigger(groupId, level)
        return true
    end     
    return false
end

function technology.sendTechnologyInfosUpdate(updateList)
    local list =  technology.techTreeList
    --print("###technology.sendTechnologyInfosUpdate",utils.serialize(list))
    local updateListTemp = {}
    for k,v in pairs(list) do
        local techGroupInfoList = v.techGroupList
        --print("###technology.  techGroupInfoList",utils.serialize(techGroupInfoList))
        local temp = {}
         if techGroupInfoList then
            for groupId, techGroupInfo in pairs(techGroupInfoList) do
                if techGroupInfo then
                    table.insert(temp, {groupId = techGroupInfo.groupId,tplId = techGroupInfo.tplId,level = techGroupInfo.level})
                end
            end
            table.insert(updateListTemp,{type = k,techGroupList = temp})
        end   
    end
    -- print("###technology.sendTechnologyInfosUpdate",utils.serialize(updateListTemp))
    agent.sendPktout(p.SC_TECHNOLOGY_INFOS_UPDATE, '@@1=[type=i,techGroupList=[groupId=i,tplId=i,level=i]]', updateListTemp)
end

function technology.sendTechnologyTreeUpdate()
    local list =  technology.techTreeList
    local temp = {}
    for _,v in pairs(list) do
        table.insert(temp, {type = v.type})
    end
    --print('technology.sendTechnologyInfosUpdate...san, list', technology.san, utils.serialize(temp))
    agent.sendPktout(p.SC_TECHNOLOGY_TREE_UPDATE, '@@1=[type=i]', temp)
end
return technology
