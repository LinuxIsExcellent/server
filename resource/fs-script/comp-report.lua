local agent = ...
local p = require('protocol')
local t = require('tploader')
local dbo = require('dbo')
local trie = require('trie')
local utils = require('utils')
local misc = require('libs/misc')
local mapService
local user = agent.user
local alliance = agent.alliance
local threadHelper = require('libs/threadHelper')
local jobExecutor = threadHelper.createJobExecutor()
local reportStub = require('stub/report')

--local pktHandlers = {}

local report = {
	
}


function report.onInit()
    --agent.registerHandlers(pktHandlers)
end

function report.onAllCompInit()

end

function report.onReady()
    mapService = agent.map.mapService
end

function report.onClose()
end

function report.cs_get_report(id)
    -- print("------- get-report ------ ", id)
    local reportData = {}	
    agent.queueJob(function()
        --local ok, reportData = reportStub.rawStub:call_getReportData(id)    
        local db = dbo.open(0)
        local rs = db:executePrepare('SELECT * from s_report where id = ? limit 1', id)

        if rs.ok then
            for _, row in ipairs(rs) do
                reportData.id = row.id
                reportData.uid = row.uid
                reportData.createTime = row.createTime
                reportData.data = row.data
            end
        end
        if reportData.data ~= nil then
            -- agent.replyPktout(session, p.SC_GET_REPORT_RESPONSE, '@@1=i,2=i,3=i,4=[uid=i,headId=i,nickName=s,soloHeroId=i],5=[trapId=i],6=[attackType=i,id=i,heroId=i,soldierId=i,hp=i,initPosition=i],7=[castId=i,buffId=i,targetIds=[targetId=i]],8=[allUnitRounds=[amryId=i,targetId=i,complexSkill=[skillId=i,castId=i,targetIds=s],skill=[skillId=i,castId=i,targetIds=s],attack=[targetId=i,changeHp=i],addRageVal=i,addBatVal=i]],9=[winUID=i,dropIds=[dropId=i]]', reportData.id, reportData.uid, reportData.createTime, data.initTeamDatas, list,data.heroAmryDatas,buffList,{},resultList) 
            agent.replyPktout(session, p.SC_GET_REPORT_RESPONSE, '@@1=i,2=i,3=i,4=s', reportData.id, reportData.uid, reportData.createTime, reportData.data)            
        end
    end)
end

return report

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                