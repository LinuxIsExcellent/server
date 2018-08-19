local agent = ...
local p = require('protocol')
local dbo = require('dbo')
local misc = require('libs/misc')
local utils = require('utils')
local dataStub = require('stub/data')
local user = agent.user

local dict = {
    data = {},
    isDirty = {},
}

function dict.get(k)
    return misc.deserialize(dict.data[k])
end

--withoutKey可不传 默认为false
function dict.set(k, v, withoutKey)
    local v = utils.serialize(v, withoutKey)
    if dict.data[k] ~= v then
        dict.data[k] = v
        dict.isDirty[k] = true
    end
end

function dict.dbLoad()
    local db = dbo.open(0)
    local rs = db:executePrepare('SELECT k, v FROM s_dict WHERE uid = ? or  k = "ui.switch"', user.uid)
    if rs.ok and rs.rowsCount > 0 then
        for _, row in ipairs(rs) do
            dict.data[row.k] = row.v
            --print('dict.dbLoad uid, k =', user.uid, row.k)
        end
    end
end

function dict.dbSave(timingSave)
    local db = dbo.open(0)
    local list = {}
    for k, v in pairs(dict.data) do
        if dict.isDirty[k] then
            dict.isDirty[k] = false
            table.insert(list, { uid = user.uid, k = k, v = v })
            --print('dict.dbSave uid, k =', user.uid, k)
        end
    end

    dataStub.appendDataList(p.DataClassify.DICT, list)
end

function dict.dbSaveByKey(key)
    dict.isDirty[key] = false
    local dataRow = { uid = user.uid, k = key, v = dict.data[key] }
    dataStub.appendData(p.DataClassify.DICT, dataRow)
    --print('dict.dbSave uid, k =', user.uid, key)
end

function dict.onInit()
    dict.dbLoad()
end

function dict.onClose()
    dict.dbSave()
end

function dict.onSave()
    dict.dbSave()
end

function dict.onCrossTeleport()
end

function dict.getCrossTeleportInfo()
    return dict.data
end

return dict



