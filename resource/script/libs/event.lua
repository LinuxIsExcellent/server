-- 事件
local M = {}

local mt = {}
mt.__index = mt

local weak = {__mode = 'v'}

local function makeCallback(t, cb, observer)
    local ret = {t, cb, observer}
    --setmetatable(ret, weak)
    return ret
end

local function makeSelfCallback(t, s, cb, observer)
    local ret = {t, cb, observer, s}
    --setmetatable(ret, weak)
    return ret
end

function mt:attach(node, cb)
    assert(type(cb) == 'function')
    table.insert(self._handlers, makeCallback('ever', cb, node:getObserver()))
end

function mt:attachOnce(node, cb)
    assert(type(cb) == 'function')
    table.insert(self._handlers, makeCallback('once', cb, node:getObserver()))
end

function mt:attachRaw(cb)
    assert(type(cb) == 'function')
    table.insert(self._handlers, makeCallback('ever', cb))
end

function mt:attachRawOnce(cb)
    assert(type(cb) == 'function')
    table.insert(self._handlers, makeCallback('once', cb))
end

function mt:attachSelf(s, cb)
    assert(type(cb) == 'function')
    table.insert(self._handlers, makeSelfCallback('ever', s, cb))
end

function mt:trigger(...)
    local cbs = self._handlers
    local i = 1
    while i <= #cbs do
        local v = cbs[i]
        local cb = v[2]
        local observer = v[3]
        local s = v[4]
        local bk = false
        if observer == nil or observer:isExist() then
            if s == nil then
                bk = cb(...)
            else
                bk = cb(s, ...)
            end
        end
        if v[1] == 'once' or (observer ~= nil and not observer:isExist()) then
            table.remove(cbs, i)
        else
            i = i + 1
        end
        if bk then
            break
        end
    end
end

function M.new()
    local obj = {
        _handlers = {}
    }
    setmetatable(obj, mt)
    return obj
end

return M
