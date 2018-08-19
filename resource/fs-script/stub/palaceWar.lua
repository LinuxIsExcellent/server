local cluster = require('cluster')
local p = require('protocol')

local rawStub
local titles = {}
local gifts = {}
local palaceWarStub = {
    rawStub,
    titles = titles,
    gifts = gifts,
}
local kingUid = 0

local function titlesDebug()
    --print('titlesDebug')
    for _, v in pairs(titles) do
        print("titlesDebug : tplid, uid, headId, nickname, lastCancelTimestamp", v.tplid, v.uid, v.headId, v.nickname, v.lastCancelTimestamp)
    end
end

local function giftsDebug()
    for k, v in pairs(gifts) do
        print("giftsDebug : tplid, uidSize", k, #v)
    end
end

local subscriber = {
    onTitlesUpdate = function(list)
        -- print('onTitlesUpdate')
        for _, v in pairs(list) do
            titles[v.tplid] = v
            if v.tplid == p.TitleType.KING then
                kingUid = v.uid
            end
        end
        titlesDebug()
    end,
    onGiftsUpdate = function(list)
        -- print('onGiftsUpdate')
        for k, v in pairs(list) do
            gifts[k] = v
        end
        giftsDebug()
    end,
}

function palaceWarStub.connectService()
    rawStub = cluster.connectService('palaceWar@cs')
    palaceWarStub.rawStub = rawStub
    rawStub:setSubscriber(subscriber)

    palaceWarStub.fetchAllTitles()
    palaceWarStub.fetchAllGifts()
end

function palaceWarStub.fetchAllTitles()
    --print('palaceWarStub.fetchAllTitles begin')
    local ret, list = rawStub:call_fetch_all_titles()
    if not ret or list == nil then
        print('palaceWarStub.fetchAllTitles ... error : ret, list', ret, list)
        return
    end
    for _, v in pairs(list) do
        titles[v.tplid] = v
        if v.tplid == p.TitleType.KING then
            kingUid = v.uid
        end
    end
    titlesDebug()
    --print('palaceWarStub.fetchAllTitles end')
end

function palaceWarStub.fetchAllGifts()
    --print('palaceWarStub.fetchAllGifts begin')
    local ret, list = rawStub:call_fetch_all_gifts()
    if not ret or list == nil then
        print('palaceWarStub.fetchAllGifts ... error : ret, list', ret, list)
        return
    end
    for k, v in pairs(list) do
        gifts[k] = v
    end
    giftsDebug()
    --print('palaceWarStub.fetchAllGifts end')
end

function palaceWarStub.isKing(uid)
    return kingUid == uid
end
 
function palaceWarStub.findTitleByUid(uid)
    for k, v in pairs(titles) do
        if v.uid == uid then
            return k
        end
    end
    return 0
end

return palaceWarStub
