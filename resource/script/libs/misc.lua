local utils = require('utils')

local misc = {}

function misc.serialize(o, withoutKey)
    local str = ""
    local t = type(o)
    if t == "number" then
        str = str .. o
    elseif t == "string" then
        str = string.format('%q', o)
    elseif t == "boolean" then
        str = str .. tostring(o)
    elseif t == "table" then
        str = "{"
        for k, v in pairs(o) do
            if withoutKey ~= true then
                str = str .. "[" .. misc.serialize(k, withoutKey) .. "]" .. "=" 
            end
            str = str .. misc.serialize(v, withoutKey) .. ","
        end
        --metatable
        local metatable = getmetatable(o)
        if metatable ~= nil and type(metatable.__index) == "table" then
            for k, v in pairs(metatable) do
                if k ~= "__index" then
                    str = str .. "[" .. misc.serialize(k) .. "]" .. "=" .. misc.serialize(v) .. ","
                end
            end
        end
        str = str .. "}"
    elseif t == "nil" then
        str = nil
    else
        local msg = "error #MISC SERIALIZE WARNING wrong type = " .. t
        utils.log(msg)
        error(msg)
    end
    return str
end

function misc.deserialize(str, throwError)
    if throwError == nil then
        throwError = true
    end

    local t = type(str)
    if t == "number" or t == "string" or t == "boolean" then
        --注：此处字符串类型有可能最后返回nil 如str = "aa" => func = {return aa}
        str = tostring(str)
    elseif t == "table" then
        return str
    elseif t == "nil" then
        --print("#MISC DESERIALIZE WARNING", str)
        return nil
    else
        local msg = "error #MISC DESERIALIZE WARNING wrong type = " .. t
        utils.log(msg)
        if throwError then
            error(msg)
        else
            return nil
        end
    end
    local func = load("return " .. str)
    if func == nil then
        local msg = "error #MISC DESERIALIZE WARNING format error str = " .. str
        utils.log(msg)
        if throwError then
            error(msg)
        end
        return nil
    end
    return func()
end

function misc.substring(str, maxLen)
    if type(str) ~= "string" then
        print("#misc.substring wrong type of str = ", type(str))
        return str
    end
    if maxLen ~= nil and type(maxLen) ~= "number" then
        print("#misc.substring wrong type of maxLen = ", type(maxLen))
        return str
    end
    local pos = str:find('\n')
    if pos then
        str = str:sub(1, pos)
    end
    maxLen = maxLen or 60

    local len = string.len(str)
    if len > maxLen then
        local dropping = string.byte(str, maxLen+1)
        if not dropping then return str end
        if dropping >= 128 and dropping < 192 then
            return misc.substring(str, maxLen-1)
        end
        return string.sub(str, 1, maxLen)
    end

    return str
end

function misc.decodeURI(s)
    if s == nil then
        s = ''
    end
    s = string.gsub(s, '%%(%x%x)', function(h) return string.char(tonumber(h, 16)) end)
    return s
end

function misc.encodeURI(s)
    if s == nil then
        s = ''
    end
    s = string.gsub(s, "([^%w%.%- ])", function(c) return string.format("%%%02X", string.byte(c)) end)
    return string.gsub(s, " ", "+")
end

function misc.stringSplit(str, delimiter)
    local result = {}
    if not str or str == '' or not delimiter then
        return result
    end

    for v in (str .. delimiter):gmatch('(.-)' .. delimiter) do
        table.insert(result, v)
    end
    return result
end

function misc.stringSplitInt(str, delimiter)
    local result = {}
    if not str or str == '' or not delimiter then
        return result
    end

    str = str .. delimiter
    local start = 1
    local len = delimiter:len()
    while true do
        local pos = str:find(delimiter, start, true)
        if not pos then
            break
        end
        local v = str:sub(start, pos - 1)
        local int = tonumber(v)
        if int then
            table.insert(result, int)
        end
        start = pos + len
    end
    return result
end

function misc.inArray(target, array)
    if type(array) ~= 'table' then
        return false
    end
    for i,v in ipairs(array) do
        if v == target then
            return true
        end
    end

    return false
end

function misc.strTrim(str)
  str = string.gsub(str, "^[ \t\n\r]+", "")
  return string.gsub(str, "[ \t\n\r]+$", "")
end

return misc
