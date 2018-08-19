local utils = require("utils")
local olibs = require("olibs")
local timer = require("timer")

print("hello world")

print("md5(orange) = ", utils.md5("orange"))

olibs.test(utils)

local function countTable(t)
    local count = 0
    for _, _ in pairs(t) do
        count = count + 1
    end
    return count
end

local t = {}
for i = 1, 1000 do
    table.insert(t, i)
end
t.a = 3
local meta = {
    m = 100
}
setmetatable(t, meta)

beginT = os.clock()
count = countTable(t)
endT = os.clock()

print("lua count =", count, endT - beginT) 

local beginT = os.clock()
local count = olibs.countTable(t)
local endT = os.clock()

print("olibs count =", count, endT - beginT) 

