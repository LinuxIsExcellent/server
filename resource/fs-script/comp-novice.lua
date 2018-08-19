local agent = ...
local p = require('protocol')
local t = require('tploader')

local user = agent.user
local dict = agent.dict

local pktHandlers = {}

local novice = {
    flags = {},
}

function novice.dbLoad()
    local flags = dict.get("novice.flags") or {}
    for _, v in pairs(flags) do
        novice.flags[v] = v
    end
end

function novice.dbSave()
    local flags = {}
    for _, v in pairs(novice.flags) do
        table.insert(flags, v)
    end
    dict.set("novice.flags", flags)
end

function novice.onSave()
    novice.dbSave()
end

function novice.onInit()
    agent.registerHandlers(pktHandlers)

    novice.dbLoad()
    if not user.info.isSetName then
        return
    end
    novice.sendNoviceUpdate()
end

function novice.onClose()
    novice.dbSave()
end

function novice.onAllCompInit()
    user.evtSetName:attachRaw(novice.sendNoviceUpdate)
end

function novice.sendNoviceUpdate()
    local flags = {}
    for _, flagId in pairs(novice.flags) do
        table.insert(flags, { flagId = flagId })
        print('###novice.sendNoviceUpdate..flagId=', flagId)
    end
    print("###novice.sendNoviceUpdate")
    agent.sendPktout(p.SC_NOVICE_UPDATE, '@@1=[flagId=s]', flags)
end

function novice.cs_novice_set(flagId)
    -- print('p.CS_NOVICE_SET flagId=', flagId)
    if flagId ~= '' then
        -- novice.flags[flagId] = flagId
        table.insert(novice.flags, flagId)
    end
end

return novice
