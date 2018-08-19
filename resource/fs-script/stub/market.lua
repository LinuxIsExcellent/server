local cluster = require('cluster')
local p = require('protocol')

local rawStub

local marketStub = {
	rawStub,
}





function marketStub.connectService()
    rawStub = cluster.connectService('market@cs')
    marketStub.rawStub = rawStub
end




return marketStub