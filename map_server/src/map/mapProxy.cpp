#include "mapProxy.h"
#include "agent.h"
#include "agentProxy.h"
#include <base/utils/utils_string.h>
#include <base/utils/file.h>
#include <base/cluster/message.h>
#include <base/cluster/nodemonitor.h>
#include <base/event/dispatcher.h>
#include <base/memory/memorypoolmgr.h>
#include <base/framework.h>
#include <model/rpc/map.h>
#include <model/protocol.h>
#include <model/tpl/templateloader.h>
#include <model/tpl/configure.h>
#include <model/tpl/alliance.h>
#include "tpl/templateloader.h"
#include "tpl/maptrooptpl.h"
#include "tpl/mapcitytpl.h"
#include "tpl/resourcetpl.h"
#include "tpl/monstertpl.h"
#include "unit/castle.h"
#include "unit/resnode.h"
#include "troop.h"
#include "unit/monster.h"
#include "unit/camp.h"
#include "unit/tree.h"
#include "unit/famouscity.h"
#include "unit/capital.h"
#include "unit/palace.h"
#include "unit/catapult.h"
#include "unit/worldboss.h"
#include <fstream>
#include <base/utils/utils_time.h>
#include <base/lua/luamessage.h>
#include "alliance.h"
#include "qo/commandagentload.h"
#include "qo/commandagentsave.h"
#include "qo/commandunitload.h"
#include "qo/commandunitsave.h"
#include "qo/commandtroopload.h"
#include "qo/commandfighthistoryload.h"
#include "qo/commandsuccessivekingsload.h"
#include "qo/commandserverinfoload.h"
#include "qo/commandserverinfosave.h"
#include "qo/commanddictload.h"
#include "qo/commanddictsave.h"
#include "qo/commandtransportrecordload.h"
#include "qo/dataservice.h"
#include "qo/commandstatsave.h"
#include "luawrite.h"
#include "activitymgr.h"
#include <base/logger.h>
#include <base/dbo/dbpool.h>
#include <base/utils/utils_time.h>
#include <base/3rd/rapidjson/filestream.h>
#include <base/3rd/rapidjson/document.h>
#include <base/3rd/rapidjson/writer.h>
#include <base/3rd/rapidjson/stringbuffer.h>
#include "bitmap_image.hpp"

namespace ms
{
    namespace map
    {
        using namespace std;
        using namespace tpl;
        using namespace model;
        using namespace model::tpl;
        using namespace model::rpc;
        using namespace base::cluster;
        using namespace base::lua;
        using namespace qo;
        using namespace info;

        void MapMailboxEventHandler::OnMessageReceive(base::cluster::MessageIn& msgin) {
                // cout << "OnMessageReceive = " << msgin.code() << endl;
                if (!g_mapMgr->m_isSetupFinish) {
                    return;
                }
                model::rpc::MapCode code = static_cast<model::rpc::MapCode>(msgin.code());
                if ((int)code == 0 && (msgin.session() == 0 || msgin.session() > 10)) {
                    LuaMessageReader reader = LuaMessageReader(msgin);
                    string func = msgin.ReadString();
                    //cout << "func = " << func << endl;
                    DataTable dt = reader.ReadAllArguments();
                    //cout << dt.Count() << endl;
                    //dt.DumpInfo();
                    if (func == "onCSBootstrapFinish") {
                        g_mapMgr->m_csMbid = msgin.from();
                        //cout << "mbid = " << g_mapMgr->m_csMbid << endl;
                        g_mapMgr->proxy()->SendcsBootstrapFinishResponse();
                        if (!g_mapMgr->CheckTotalPower()) {
                            g_mapMgr->proxy()->SendcsMapGlobalData();
                        }
                        g_mapMgr->proxy()->SendcsAllianceCity();
                    }else if (func == "onKingChanged") {
                        //TODO 活动状态判定
                        int64_t uid = dt.Get(1)->ToInteger();
                        g_mapMgr->ChangeKing(uid);
                    } else if (func == "onTitlesUpdate") {
                        const DataTable& list = dt.Get(1)->ToTable();
                        list.ForeachAll([](const DataValue & k, const DataValue & v) {
                            int64_t uid = k.ToInteger();
                            if (Agent* agt = g_mapMgr->FindAgentByUID(uid)) {
                                agt->UpdateTitleId(v.ToInteger());
                            }
                            return false;
                        });
                    }/* else if (func == "crossTeleportGetUidResponse") {
                        int session = dt.Get(1)->ToInteger();
                        int64_t uid = dt.Get(2)->ToInteger();
                        g_mapMgr->m_crossTeleportMgr->onRecvCrossTeleportResponse(session, uid);
                    }*/ else if (func == "activitiesUpdate") {
                        g_mapMgr->m_activityMgr->HandleMessage(func, dt);
                    } else if (func == "report_sync")
                    {
                        // cout << "the report max is " << dt.Get(1)->ToInteger() << endl;
                        g_mapMgr->SetMaxReportId(dt.Get(1)->ToInteger());
                    }
                    else {
                        g_mapMgr->m_alliance->HandleMessage(func, dt);
                    }
                    return;
                }
                switch (code) {
                    case MapCode::ENTER: {
                        int64_t uid = msgin.ReadVarInteger<int64_t>();
                        g_mapMgr->Enter(uid, msgin.from());
                    }
                    break;
                    case MapCode::QUIT: {
                        int64_t uid = msgin.ReadVarInteger<int64_t>();
                        g_mapMgr->Quit(uid);
                    }
                    break;
                    case MapCode::LUA_CALL:
                    case MapCode::LUA_CAST:
                    case MapCode::FORWARD: {
//                         cout << "case MapCode::code " << (int)code << endl;
                        int64_t uid = msgin.ReadVarInteger<int64_t>();
                        if (g_mapMgr->isLocalUid(uid)) {
                            // if local uid
                            Agent* agent = g_mapMgr->FindAgentByUID(uid);
                            if (agent != nullptr) {
                                agent->proxy()->HandleMessage(msgin);
                            } else {
                                if (Agent* agent = g_mapMgr->FindAgentWaitInit(uid)) {
                                    agent->proxy()->HandleMessage(msgin);
                                }
                            }
                        } else {
                            Agent* agent = g_mapMgr->FindVisitorByUID(uid);
                            if (agent == nullptr) {
                                agent = new Agent(uid);
                                agent->Connect(msgin.from());
                                g_mapMgr->VisitorJoin(agent);
                            }
                            if (agent != nullptr) {
                                agent->proxy()->HandleMessage(msgin);
                            }
                        }
                    }
                    break;
//                     case MapCode::CROSS_TELEPORT: {
//                         g_mapMgr->m_crossTeleportMgr->OnRecvCrossTeleport(msgin);
//                     }
//                     break;
                    default: {
                        cout << "default code = " << (int)code << endl;
                    }
                    break;
                }
            }

        MapProxy::MapProxy(Map& map) : m_map(map)
        {
            m_mailboxEventHandler = new MapMailboxEventHandler();
            m_mailbox = base::cluster::Mailbox::Create(*m_mailboxEventHandler, m_map.name().c_str(), true);
        }

        MapProxy::~MapProxy()
        {
            delete m_mailbox;
            delete m_mailboxEventHandler;
            m_mempool = nullptr; // do not delete
        }

        void MapProxy::SetUp()
        {
            m_mempool = base::memory::g_memory_pool_mgr->Aquire("map", 64, 256);
        }

        void MapProxy::SendToCS(const function< void (base::lua::LuaMessageWriter&) >& fun)
        {
            if (m_map.m_csMbid) {
                MSGOUT(0, 64);
                LuaMessageWriter lmw = LuaMessageWriter(msgout);
                fun(lmw);
                msgout.SetSession(0u);
                m_mailbox->Cast(m_map.m_csMbid, msgout);
            }
        }


        void MapProxy::SendMail(int64_t toUid, int64_t otherUid, MailType type, MailSubType subType, const string& attachment, const string& params, bool isDraw)
        {
            //cout << "Map::SendMail" << endl;
            SendToCS([&](base::lua::LuaMessageWriter & lmw) {
                lmw.WriteArgumentCount(7);
                lmw.WriteString("sendMail");
                lmw.WriteVarInteger(toUid);
                lmw.WriteVarInteger(otherUid);
                lmw.WriteVarInteger((int)type);
                lmw.WriteVarInteger((int)subType);
                lmw.WriteString(attachment);
                lmw.WriteString(params);
                lmw.WriteBoolean(isDraw);
            });
        }

        void MapProxy::SendcsMapGlobalData()
        {
            DataTable dtGlobal;
            dtGlobal.Set("castleCellCnt", m_map.globalData().castleCnt * 4);
            dtGlobal.Set("killNpcCnt", m_map.globalData().killNpcCnt);
            //dtGlobal.Set("occupyResCnt", m_map.globalData().occupyResCnt);
            dtGlobal.Set("occupyRes", m_map.globalData().occupyRes);
            dtGlobal.Set("occupyCapitalCnt", m_map.globalData().occupyCapitalCnt);
            dtGlobal.Set("occupyChowCnt", m_map.globalData().occupyChowCnt);
            dtGlobal.Set("occupyPrefectureCnt", m_map.globalData().occupyPrefectureCnt);
            dtGlobal.Set("occupyCountyCnt", m_map.globalData().occupyCountyCnt);

            dtGlobal.Set("totalPower", m_map.globalData().totalPower);
            dtGlobal.Set("gatherCnt", m_map.globalData().gatherCnt);
            dtGlobal.Set("heroCnt", m_map.globalData().heroCnt);

            SendToCS([&](base::lua::LuaMessageWriter & lmw) {
                lmw.WriteArgumentCount(2);
                lmw.WriteString("globalData");
                lmw.WriteValue(dtGlobal);
            });
        }

        void MapProxy::SendcsAllianceCity()
        {
            DataTable dtAllAllianceCity;
            for (auto alCity : m_map.m_allianceCity) {
                auto alId = alCity.first;
                auto cityList = alCity.second;
                DataTable dtOwnCity;
                int index = 0;
                for (auto city : cityList) {
                    if (city && city->cityTpl()) {
                        dtOwnCity.Set(++index, city->cityTpl()->cityId);
                    }
                }
                dtAllAllianceCity.Set(alId, dtOwnCity);
            }
            SendToCS([&](base::lua::LuaMessageWriter & lmw) {
                lmw.WriteArgumentCount(2);
                lmw.WriteString("allAllianceCity");
                lmw.WriteValue(dtAllAllianceCity);
            });
        }

        void MapProxy::SetMapGlobalData(base::DataTable& dt)
        {
            if (DataValue* dv = dt.Get("castleCnt")) {
                m_map.m_globalData.castleCnt = dv->ToInteger();
            }
            if (DataValue* dv = dt.Get("killNpcCnt")) {
                m_map.m_globalData.killNpcCnt = dv->ToInteger();
            }
            if (DataValue* dv = dt.Get("occupyResCnt")) {
                m_map.m_globalData.occupyResCnt = dv->ToInteger();
            }
            if (DataValue* dv = dt.Get("occupyRes")) {
                m_map.m_globalData.occupyRes = dv->ToTable();
            }
            if (DataValue* dv = dt.Get("occupyCapitalCnt")) {
                m_map.m_globalData.occupyCapitalCnt = dv->ToInteger();
            }
            if (DataValue* dv = dt.Get("occupyChowCnt")) {
                m_map.m_globalData.occupyChowCnt = dv->ToInteger();
            }
            if (DataValue* dv = dt.Get("occupyPrefectureCnt")) {
                m_map.m_globalData.occupyPrefectureCnt = dv->ToInteger();
            }
            if (DataValue* dv = dt.Get("occupyCountyCnt")) {
                m_map.m_globalData.occupyCountyCnt = dv->ToInteger();
            }

            if (DataValue* dv = dt.Get("totalPower")) {
                m_map.m_globalData.totalPower = dv->ToInteger();
            }
            if (DataValue* dv = dt.Get("gatherCnt")) {
                m_map.m_globalData.gatherCnt = dv->ToInteger();
            }
            if (DataValue* dv = dt.Get("heroCnt")) {
                m_map.m_globalData.heroCnt = dv->ToTable();
            }

            if (DataValue* dv = dt.Get("monsterDist")) {
                m_map.m_globalData.monsterDist = dv->ToTable();
            }
        }

        void MapProxy::SendcsAddAllianceRecord(int64_t allianceId, model::AllianceMessageType type, std::string param)
        {
            DataTable dtRecord;
            dtRecord.Set("allianceId", allianceId);
            dtRecord.Set("type", (int)type);
            dtRecord.Set("param", param);

            SendToCS([&](base::lua::LuaMessageWriter & lmw) {
                lmw.WriteArgumentCount(2);
                lmw.WriteString("addAllianceRecord");
                lmw.WriteValue(dtRecord);
            });
        }

        void MapProxy::SendcsAddTransportRecord(Troop* troop, int64_t uid, int32_t headId, std::string nickName, int64_t toUid, int32_t toHeadId, std::string toNickName, 
                model::TransportArriveType ArriveType, int64_t timestamp)
        {
            DataTable dtRecord;
            dtRecord.Set("transportId1", troop->transportRecordId1());
            dtRecord.Set("uid", uid);
            dtRecord.Set("headId", headId);
            dtRecord.Set("nickName", nickName);
            dtRecord.Set("transportId2", troop->transportRecordId2());
            dtRecord.Set("toUid", toUid);
            dtRecord.Set("toHeadId", toHeadId);
            dtRecord.Set("toNickName", toNickName);
            DataTable dtResource;
            dtResource.Set("food", troop->food());
            dtResource.Set("wood", troop->wood());
            dtResource.Set("iron", troop->iron());
            dtResource.Set("stone", troop->stone());
            dtRecord.Set("carry", dtResource);
            dtRecord.Set("ArriveType", (int)ArriveType);
            dtRecord.Set("arriveTime", timestamp);

            SendToCS([&](base::lua::LuaMessageWriter & lmw) {
                lmw.WriteArgumentCount(2);
                lmw.WriteString("addTransportRecord");
                lmw.WriteValue(dtRecord);
            });
        }

        void MapProxy::SendcsBootstrapFinishResponse()
        {
            SendToCS([&](base::lua::LuaMessageWriter & lmw) {
                int argc = 1;
                bool isNotStart = m_map.m_capital->isNotStart();
                //bool isProtected = false;
                if (isNotStart) {
                    argc = 4;
                } else {
                    argc = 2;
                }
                lmw.WriteArgumentCount(argc);
                lmw.WriteString("bootstrapFinishResponse");
                lmw.WriteBoolean(isNotStart);
                 if (isNotStart) {
                     lmw.WriteVarInteger(m_map.m_capital->occupierId());
                     lmw.WriteVarInteger(m_map.m_capital->chooseLeftTime());
                 }
            });
        }

        void MapProxy::SendcsPalaceWarStart()
        {
             SendToCS([&](base::lua::LuaMessageWriter & lmw) {
                lmw.WriteArgumentCount(1);
                lmw.WriteString("palaceWarStart");
            });
        }

        void MapProxy::SendcsPalaceWarEnd()
        {
            SendToCS([&](base::lua::LuaMessageWriter & lmw) {
                lmw.WriteArgumentCount(3);
                lmw.WriteString("palaceWarEnd");
                lmw.WriteVarInteger(m_map.m_capital->occupierId());
                lmw.WriteVarInteger(m_map.m_capital->chooseLeftTime());
            });
        }

        void MapProxy::SendcsPalaceWarPrepare(int times, int dropId)
        {
            SendToCS([&](base::lua::LuaMessageWriter & lmw) {
                lmw.WriteArgumentCount(4);
                lmw.WriteString("palaceWarPrepare");
                lmw.WriteVarInteger(times);
                lmw.WriteVarInteger(dropId);
                DataTable uidsTable;
                int i = 0;
                int64_t ts = 86400  * 10;
                int64_t now = g_dispatcher->GetTimestampCache();
                for (auto it = m_map.m_agents.begin(); it != m_map.m_agents.end(); ++it) {
                    Agent* agt = it->second;
                    if (now - agt->m_lastLoginTimestamp < ts) {
                        uidsTable.Set(++i, agt->uid());
                    }
                }
                lmw.WriteValue(uidsTable);
            });
        }

        void MapProxy::BroadNoticeMessage(int id, int type, const std::string& param)
        {
            for (auto it = m_map.m_agents.begin(); it != m_map.m_agents.end(); ++it) {
                Agent* agt = it->second;
                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, 64, mempool());
                msgout << (int)model::SC::NOTICE_MESSAGE;
                msgout << id << param << type;
                agt->proxy()->SendMessage(msgout);
            }
        }

        void MapProxy::AddReport(int64_t uid, int reportId, const std::string& reportData)
        {
            //cout << "Map::SendMail" << endl;
            DataTable dtAddReport;
            dtAddReport.Set("id", reportId);
            dtAddReport.Set("uid", uid);
            dtAddReport.Set("createTime", g_dispatcher->GetTimestampCache());
            dtAddReport.Set("data", reportData);

            SendToCS([&](base::lua::LuaMessageWriter & lmw) {
                lmw.WriteArgumentCount(2);
                lmw.WriteString("addReport");
                lmw.WriteValue(dtAddReport);
            });
        }
    }
}
