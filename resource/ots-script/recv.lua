local p = require('protocol')

local pktinHandler = {}

function recvPkt(pktin)
    local code = pktin:code()
    if pktinHandler[code] then
        --pktinHandler[code](pktin)
    else
        print("#lua unhandle code = ", code)
    end
end

pktinHandler[11] = function()
    print("player is ready")
end

pktinHandler[1001] = function(pktin)
    local size = pktin:read('i');
    print("size = ", size)
    for i = 1, size do
        local id, x, y, tplid, leftSecond, uid, alliance, nickname, level, state = pktin:read("iiiiiissii")
        print(id, x, y, tplid, leftSecond, uid, alliance, nickname, level, state)
        pktin:read("iifii")
    end
    print("size = ", size)
    local rsize = pktin:read('i')
    for i = 1, rsize do
        local id = pktin:read("i")
        print(id)
    end
    print("rsize = ", rsize)
end

pktinHandler[1003] = function(pktin)
    local k, x, y = pktin:read("iii")
    print("k x y", k, x, y)
end

pktinHandler[1002] = function(pktin)
    local size = pktin:read("i")
    print("pktin size = ", pktin:size())
    print("size", size)
    for i = 1, size do
        print("i = ", i)
        local id, type, state, uid, fx, fy, tx, ty = pktin:read("iiiiiiiiiiissiiiifff")
        print(id, type, state, uid, fx, fy, tx, ty)
        local armySize = pktin:read('i')
        print("armySize = ", armySize)
        for j = 1, armySize do
            pktin:read('i')
            local stateSize = pktin:read('i')
            print("stateSize = ", stateSize)
            for k = 1, stateSize do
                pktin:read('ii')
            end
        end
        local rallySize = pktin:read('i')
        print("rallySize = ", rallySize)
        for j = 1, rallySize do
            pktin:read('i')
        end
        pktin:read('iii')
        local toNickname = pktin:read('s')
        --print(toNickname)
        pktin:read('ii')
        local reinfomentsSize = pktin:read('i')
        print("reinfomentsSize = ", reinfomentsSize)
        for j = 1, reinfomentsSize do
            pktin:read('i')
        end
        pktin:read('iiiii')
    end
    print("####################################")

    local rsize = pktin:read("i")
    print("rsize", rsize)
    for i = 1, rsize do
        local id = pktin:read("i")
        print(id)
    end
    print("pktin size = ", pktin:size())
end

pktinHandler[351] = function(pktin)
    local size = pktin:read("i")
    print("size", size)
    for i = 1, size do
        local tplid, size2 = pktin:read("ii")
        print("tplid, size2", tplid, size2)
        for j = 1, size2 do
            local state, count = pktin:read("ii")
            print("state, count", state, count)
        end
    end
end

pktinHandler[p.SC_USER_UPDATE] = function(pktin)
    local uid, nickname, gold, food, wood, iron, mithril = pktin:read("isiiiii")
    print("uid, nickname, gold, food, wood, iron, mithril = ", uid, nickname, gold, food, wood, iron, mithril)
end

pktinHandler[p.SC_ATTR_PLUS_UPDATE] = function(pktin)
    local a1 = pktin:read("i")
    print("****************** property = ", a1)
end

pktinHandler[9999] = function(pktin)
    local ls1 = pktin:read('i')
    print('ls1 = ', ls1)
    for i = 1, ls1 do
        local k1 = pktin:read('i')
        local ls2 = pktin:read('i')
        print('k1 = ', k1, 'ls2 = ', ls2)
        for j = 1, ls2 do
            local k2 = pktin:read('i')
            local ls3 = pktin:read('i')
            print('k2 = ', k2, 'ls3 = ', ls3)
            for k = 1, ls3 do
                local k3, v3 = pktin:read('ii')
                print('kv3', k3, v3)
            end
        end
    end
end
