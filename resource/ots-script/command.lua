local p = require('protocol')
local timer = require('timer')

function commandHandle(str, s)
    print(s)
    if str == "@orange1" then
        local pktout = s:newPktout(p.CS_LOGIN, 16)
        pktout:write("test", "orange1", "token", "",  "channel", "1.1.20", 1, false)
        s:send(pktout)
    elseif str == "@orange2" then
        local pktout = s:newPktout(p.CS_LOGIN, 16)
        pktout:write("test", "orange2", "token", "", "channel", "1.0.0", 1, false)
        s:send(pktout)
    elseif str == "@orange233" then
        local pktout = s:newPktout(p.CS_LOGIN, 16)
        pktout:write("bk", "orange233", "token", "", "channel", "1.0.0", 1, false)
        s:send(pktout)
    elseif str == "@cross1" then
        local pktout = s:newPktout(p.CS_LOGIN, 16)
        pktout:write("tips", "cross1", "dddd", "1.0.0")
        s:send(pktout)
    elseif str == "@battle" then
        local pktout = s:newPktout(p.CS_MAP_MARCH, 16)
        pktout:write(1, 1, 1)
        pktout:write(1)
        pktout:write(1001001, 1000)
        s:send(pktout)
    elseif str == "@test" then
        print("orange test")
    elseif str == "@camp1" then
        local pktout = s:newPktout(p.CS_MAP_MARCH, 16)
        pktout:write(5, 4, 4)
        pktout:write(1)
        pktout:write(1001001, 1000)
        s:send(pktout)
    elseif str == "@camp2" then
        local pktout = s:newPktout(p.CS_MAP_MARCH, 16)
        pktout:write(5, 4, 4)
        pktout:write(2)
        pktout:write(1001001, 1000)
        pktout:write(1002001, 1000)
        s:send(pktout)
    elseif str == "@camp3" then
        local pktout = s:newPktout(p.CS_MAP_MARCH, 16)
        pktout:write(5, 23, 951)
        pktout:write(1)
        pktout:write(1001001, 1)
        pktout:write(0, 0, 0)
        pktout:write(true)
        s:send(pktout)
    elseif str == "@monster1" then
        local pktout = s:newPktout(p.CS_MAP_MARCH, 16)
        pktout:write(4, 22, 22)
        pktout:write(1)
        pktout:write(1001001, 2000)
        s:send(pktout)
    elseif str == "@monster2" then
        local pktout = s:newPktout(p.CS_MAP_MARCH, 16)
        pktout:write(4, 33, 33)
        pktout:write(1)
        pktout:write(1001001, 10)
        s:send(pktout)
    elseif str == "@packs" then
        timer.setInterval(function()
            local pktout = s:newPktout(p.CS_BUILDING_TRAIN, 16)
            pktout:write(1, 1, 1, true)
            s:send(pktout)
        end, 100)
    elseif str == "@testCall" then
        timer.setInterval(function()
            local pktout = s:newPktout(9999, 16)
            s:send(pktout)
        end, 100)
    else
        print("unhandle command")
    end
end
