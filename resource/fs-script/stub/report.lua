local cluster = require('cluster')
local p = require('protocol')

local rawStub

local reportStub = {
	rawStub,
}





function reportStub.connectService()
    rawStub = cluster.connectService('report@cs')
    reportStub.rawStub = rawStub
end




return reportStub