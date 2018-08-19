#include "msgrecord.h"
#include <base/event/dispatcher.h>
#include <base/cluster/message.h>
#include <base/data.h>
#include <base/logger.h>
#include <base/3rd/rapidjson/document.h>
#include <base/3rd/rapidjson/writer.h>
#include <base/3rd/rapidjson/stringbuffer.h>
#include <base/lua/parser.h>
#include <model/tpl/item.h>
#include <model/tpl/templateloader.h>
#include <model/tpl/configure.h>
#include <model/rpc/map.h>
#include "../luawrite.h"
#include "../agent.h"
#include "../agentProxy.h"
#include "../mapMgr.h"
#include "../unit/castle.h"
#include "../unit/famouscity.h"
#include "../info/agentinfo.h"
#include  "../tpl/templateloader.h"
#include "../tpl/npcarmytpl.h"
#include "../tpl/mapcitytpl.h"

namespace ms
{
    namespace map
    {
        namespace msgqueue
        {
            using namespace std;
            using namespace info;
            using namespace base;
            using namespace model;
            using namespace model::tpl;

            MsgRecord::MsgRecord(int id, int64_t uid, model::MessageQueueType type)
                : m_id(id), m_uid(uid), m_type(type), m_sendTime(0), m_createTime(g_dispatcher->GetTimestampCache())
            {
            }

            void MsgRecord::Send(Agent* agent)
            {
                m_sendTime = g_dispatcher->GetTimestampCache();
            }

            const bool MsgRecord::ShouldResend() const
            {
                return (g_dispatcher->GetTimestampCache() - m_sendTime >= 5);
            }

            inline static void SerializeDrops(rapidjson::Writer<rapidjson::StringBuffer>& writer, const std::vector<model::tpl::DropItem>& drops)
            {
                //[<tplid, count>]
                writer.StartArray();
                for (size_t i = 0; i < drops.size(); ++i) {
                    const model::tpl::DropItem& drop = drops[i];
                    writer.StartArray();
                    writer.Int(drop.tpl.id);
                    writer.Int(drop.count);
                    writer.EndArray();
                }
                writer.EndArray();
            }

            inline static void DeserializeDrops(rapidjson::GenericValue<rapidjson::UTF8< char >>& gv, std::vector<model::tpl::DropItem>& drops)
            {
                //[<tplid, count>]
                if (gv.IsArray()) {
                    for (size_t i = 0; i < gv.Size(); ++i) {
                        int tplid = gv[i][0u].GetInt();
                        int count = gv[i][1].GetInt();
                        if (const ItemTpl* tpl = g_tploader->FindItem(tplid)) {
                            drops.emplace_back(*tpl, count);
                        }
                    }
                }
            }

            /*======  MsgMarch  ======*/

            std::string MsgMarch::Serialize()
            {
                string jsonString;
                try {
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartObject();

                    writer.Key("troopType");
                    writer.Int((int)m_troopType);

                    writer.Key("pos");
                    writer.StartArray();
                    writer.Int(m_toPos.x);
                    writer.Int(m_toPos.y);
                    writer.EndArray();

                    writer.Key("armyList");
                    m_armyList.Serialize(writer);

                    writer.EndObject();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgMarch json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgMarch::Deserialize(const string& data)
            {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    m_troopType = static_cast<model::MapTroopType>(doc["troopType"].GetInt());
                    if (doc["pos"].IsArray()) {
                        m_toPos.x = doc["pos"][0u].GetInt();
                        m_toPos.y = doc["pos"][1].GetInt();
                    }

                    m_armyList.Desrialize(doc["armyList"]);
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgMarch fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgMarch::Send(Agent* agent)
            {
                //cout << "MsgMarch::Send" << endl;
                MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, pos, armyList;
                msg.Set("troopType", (int)m_troopType);
                pos.Set(1, m_toPos.x);
                pos.Set(2, m_toPos.y);
                msg.Set("pos", pos);
                m_armyList.SetDataTable(armyList);
                msg.Set("armyList", armyList);
                msg.Set("troopId", m_troopId);

                msgout << 3;    //argc
                lua_write(msgout, "onMarch");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgMarch::SetData(model::MapTroopType troopType, const Point& toPos, const ArmyList* armyList, const int troopId)
            {
                m_troopType = troopType;
                m_toPos = toPos;
                if (armyList) {
                    m_armyList = *armyList;
                }
                m_troopId = troopId;
            }

            /*======  MsgMarchBack  ======*/

            std::string MsgMarchBack::Serialize()
            {
                string jsonString;
                try {
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartObject();

                    writer.Key("troopType");
                    writer.Int((int)m_troopType);

                    writer.Key("pos");
                    writer.StartArray();
                    writer.Int(m_toPos.x);
                    writer.Int(m_toPos.y);
                    writer.EndArray();

                    writer.Key("food");
                    writer.Int(m_food);
                    writer.Key("wood");
                    writer.Int(m_wood);
                    writer.Key("iron");
                    writer.Int(m_iron);
                    writer.Key("stone");
                    writer.Int(m_stone);

                    writer.Key("armyList");
                    m_armyList.Serialize(writer);

                    writer.Key("isArriveCastle");
                    writer.Bool(m_isArriveCastle);

                    writer.EndObject();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgMarchBack json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgMarchBack::Deserialize(const string& data)
            {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    m_troopType = static_cast<model::MapTroopType>(doc["troopType"].GetInt());
                    if (doc["pos"].IsArray()) {
                        m_toPos.x = doc["pos"][0u].GetInt();
                        m_toPos.y = doc["pos"][1].GetInt();
                    }

                    m_food = doc["food"].GetInt();
                    m_wood = doc["wood"].GetInt();
                    m_iron = doc["iron"].GetInt();
                    m_stone = doc["stone"].GetInt();

                    m_armyList.Desrialize(doc["armyList"]);

                    m_isArriveCastle = doc["isArriveCastle"].GetBool();
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgMarchBack fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgMarchBack::Send(Agent* agent)
            {
                MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, pos, armyList;
                msg.Set("isArriveCastle", m_isArriveCastle);
                msg.Set("troopType", (int)m_troopType);
                pos.Set(1, m_toPos.x);
                pos.Set(2, m_toPos.y);
                msg.Set("pos", pos);
                msg.Set("food", m_food);
                msg.Set("wood", m_wood);
                msg.Set("iron", m_iron);
                msg.Set("stone", m_stone);
                m_armyList.SetDataTable(armyList);
                msg.Set("armyList", armyList);

                msgout << 3;    //argc
                lua_write(msgout, "onMarchBack");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgMarchBack::SetData(bool isArriveCastle,model::MapTroopType troopType, const Point& toPos, int food, int wood, int iron, int stone, const ArmyList* armyList)
            {
                m_isArriveCastle = isArriveCastle;
                m_troopType = troopType;
                m_toPos = toPos;
                m_food = food;
                m_wood = wood;
                m_iron = iron;
                m_stone = stone;
                if (armyList) {
                    m_armyList = *armyList;
                }
            }

            /*======  MsgTroopReachInvalid  ======*/
            string MsgTroopReachInvalid::Serialize()
            {
                string jsonString;
                try {
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();

                    writer.Int((int)m_type);

                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgTroopReachInvalid json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgTroopReachInvalid::Deserialize(const string& data)
            {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray() && doc.Size() == 1) {
                        m_type = (MapTroopType)doc[0u].GetInt();
                    }

                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgTroopReachInvalid fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgTroopReachInvalid::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg;
                msg.Set("type", (int)m_type);

                msgout << 3;    //argc
                lua_write(msgout, "onTroopReachInvalid");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgTroopReachInvalid::SetData(MapTroopType type)
            {
                m_type = type;
            }

            /*======  MsgAttackMonsterInvalid  ======*/

            string MsgAttackMonsterInvalid::Serialize()
            {
                string jsonString;
                try {
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();
                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgAttackMonsterInvalid json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgAttackMonsterInvalid::Deserialize(const string& data)
            {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray()) {
                    }

                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgAttackMonsterInvalid fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgAttackMonsterInvalid::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg;

                msgout << 3;    //argc
                lua_write(msgout, "onAttackMonsterInvalid");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgAttackMonsterInvalid::SetData()
            {
            }

            /*======  MsgCreateCampFixedFailed  ======*/

            string MsgCreateCampFixedFailed::Serialize()
            {
                string jsonString;
                try {
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();
                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgCreateCampFixedFailed json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgCreateCampFixedFailed::Deserialize(const string& data)
            {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray()) {
                    }

                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgCreateCampFixedFailed fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgCreateCampFixedFailed::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg;

                msgout << 3;    //argc
                lua_write(msgout, "onCreateCampFixedFailed");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgCreateCampFixedFailed::SetData()
            {
            }

            /*======  MsgGatherResource  ======*/

            std::string MsgGatherResource::Serialize()
            {
                string jsonString;
                try {
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartObject();

                    writer.Key("pos");
                    writer.StartArray();
                    writer.Int(m_pos.x);
                    writer.Int(m_pos.y);
                    writer.EndArray();

                    writer.Key("resTplId");
                    writer.Int(m_resTplId);

                    writer.Key("drops");
                   SerializeDrops(writer, m_drops);

                    writer.Key("gatherRemain");
                    writer.Int(m_gatherRemain);

                    writer.Key("timestamp");
                    writer.Int64(m_timestamp);

                    writer.EndObject();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgGatherResource json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgGatherResource::Deserialize(const string& data)
            {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsObject()) {
                        if (doc["pos"].IsArray()) {
                            m_pos.x = doc["pos"][0u].GetInt();
                            m_pos.y = doc["pos"][1].GetInt();
                        }
                        m_resTplId = doc["resTplId"].GetInt();
                        if (doc["drops"].IsArray()) {
                            DeserializeDrops(doc["drops"],  m_drops);
                        }
                        m_gatherRemain = doc["gatherRemain"].GetInt();
                        m_timestamp = doc["timestamp"].GetInt64();
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgGatherResource fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgGatherResource::Send(Agent* agent)
            {
                MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, pos, drops;
                pos.Set(1, m_pos.x);
                pos.Set(2, m_pos.y);
                msg.Set("pos", pos);
                msg.Set("resTplId", m_resTplId);
                SetDropsTable(drops, m_drops);
                msg.Set("drops", drops);
                msg.Set("gatherRemain", m_gatherRemain);
                msg.Set("timestamp", m_timestamp);

                msgout << 3;    //argc
                lua_write(msgout, "onGatherResource");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgGatherResource::SetData(const Point& toPos, int resTplId, const std::vector<model::tpl::DropItem>& dropItems, int gatherRemain, int64_t timestamp)
            {
                m_pos = toPos;
                m_resTplId = resTplId;
                std::copy(dropItems.begin(), dropItems.end(), std::back_inserter(m_drops));
                m_gatherRemain = gatherRemain;
                m_timestamp = timestamp;
            }

            /*======  MsgAttackResource  ======*/

            std::string MsgAttackResource::Serialize()
            {
                string jsonString;
                try {
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartObject();

                    writer.Key("winner");
                    writer.Int((int)m_winner);
                    writer.Key("myAttackType");
                    writer.Int((int)m_myAttackType);

                    //attacker
                    writer.Key("attacker");
                    m_attacker.Serialize(writer);
                    //defender
                    writer.Key("defender");
                    m_defender.Serialize(writer);

                    writer.Key("reportId");
                    writer.Int(m_reportId);

                    writer.Key("timestamp");
                    writer.Int64(m_timestamp);
                    writer.Key("unitId");
                    writer.Int(m_unitId);
                    writer.Key("troopId");
                    writer.Int(m_troopId);

                    writer.Key("pos");
                    writer.StartArray();
                    writer.Int(m_unitPos.x);
                    writer.Int(m_unitPos.y);
                    writer.EndArray();

                    writer.Key("tplId");
                    writer.Int(m_tplId);

                    writer.Key("reportId");
                    writer.Int(m_reportId);

                    // reportInfo
                    writer.Key("reportInfo");
                    m_reportInfo.Serialize(writer);

                    writer.Key("drops");
                    SerializeDrops(writer, m_drops);

                    writer.EndObject();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgAttackResource json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgAttackResource::Deserialize(const string& data)
            {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsObject()) {
                        m_winner = static_cast<model::AttackType>(doc["winner"].GetInt());
                        m_myAttackType = static_cast<model::AttackType>(doc["myAttackType"].GetInt());

                        m_attacker.Deserialize(doc["attacker"]);
                        m_defender.Deserialize(doc["defender"]);

                        m_reportId = doc["reportId"].GetInt();

                        m_timestamp = doc["timestamp"].GetInt64();
                        m_unitId = doc["unitId"].GetInt();
                        m_troopId = doc["troopId"].GetInt();

                        if (doc["pos"].IsArray()) {
                            m_unitPos.x = doc["pos"][0u].GetInt();
                            m_unitPos.y = doc["pos"][1].GetInt();
                        }
                        
                        m_tplId = doc["tplId"].GetInt();
                        m_reportId = doc["reportId"].GetInt();

                        m_reportInfo.Deserialize(doc["reportInfo"]);

                        if (doc["drops"].IsArray()) {
                            DeserializeDrops(doc["drops"],  m_drops);
                        }
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgAttackResource fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgAttackResource::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, attacker, defender, pos, drops, reportInfo;
                msg.Set("winner", (int)m_winner);
                msg.Set("battleType", (int)BattleType::GATHER);
                msg.Set("myAttackType", (int)m_myAttackType);
                //attacker
                m_attacker.SetDataTable(attacker);
                m_defender.SetDataTable(defender);

                msg.Set("attacker", attacker);
                msg.Set("defender", defender);

                msg.Set("timestamp", m_timestamp);

                pos.Set(1, m_unitPos.x);
                pos.Set(2, m_unitPos.y);
                msg.Set("pos", pos);

                msg.Set("tplId", m_tplId);
               
                SetDropsTable(drops, m_drops);
                msg.Set("drops", drops);

                msg.Set("reportId", m_reportId);

                m_reportInfo.SetDataTable(reportInfo);
                msg.Set("reportInfo", reportInfo);



                msgout << 6; //argc
                lua_write(msgout, "onAttackResource");
                lua_write(msgout, m_id);
                lua_write(msgout, m_unitId);
                lua_write(msgout, m_troopId);
                lua_write(msgout, msg);
                lua_write(msgout, m_reportId);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgAttackResource::SetData(model::AttackType winner, model::AttackType myAttackType, 
                const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int reportId, 
                const Point& unitPos,  int tplId, const std::vector<model::tpl::DropItem>& dropItems, int64_t timestamp, 
                int unitId, int troopId, const MsgReportInfo& reportInfo)
            {
                m_winner = winner;
                m_myAttackType = myAttackType;
                m_attacker = attacker;
                m_defender = defender;
                m_timestamp = timestamp;
                m_unitId = unitId;
                m_troopId = troopId;
                m_unitPos = unitPos;    
                m_tplId = tplId;    
                m_reportId = reportId; 
                m_reportInfo = reportInfo;
                std::copy(dropItems.begin(), dropItems.end(), std::back_inserter(m_drops));
            }

            /*======  MsgAttackMonster  ======*/

            std::string MsgAttackMonster::Serialize()
            {
                string jsonString;
                try {
                    // [ winner, myAttackType, [x, y], monsterTplId, [<tplid, count>], [<tplid, <state, count>>], timestamp, unitId, troopId, monsterHpBegin, monsterHpEnd ]
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartObject();

                    writer.Key("winner");
                    writer.Int((int)m_winner);
                    writer.Key("myAttackType");
                    writer.Int((int)m_myAttackType);

                    //attacker
                    writer.Key("attacker");
                    m_attacker.Serialize(writer);
                    //defender
                    writer.Key("defender");
                    m_defender.Serialize(writer);

                    writer.Key("pos");
                    writer.StartArray();
                    writer.Int(m_unitPos.x);
                    writer.Int(m_unitPos.y);
                    writer.EndArray();

                    writer.Key("tplId");
                    writer.Int(m_tplId);

                    writer.Key("drops");
                    SerializeDrops(writer,  m_drops);

                    writer.Key("armyList");
                    m_armyList.Serialize(writer);

                    writer.Key("timestamp");
                    writer.Int64(m_timestamp);
                    writer.Key("unitId");
                    writer.Int(m_unitId);
                    writer.Key("troopId");
                    writer.Int(m_troopId);                    
                    writer.Key("monsterHpBegin");
                    writer.Int(m_monsterHpBegin);                    
                    writer.Key("monsterHpEnd");
                    writer.Int(m_monsterHpEnd);
                    writer.Key("isCaptive");
                    writer.Int(m_isCaptive);
                    writer.Key("reportId");
                    writer.Int(m_reportId);
                    // reportInfo
                    writer.Key("reportInfo");
                    m_reportInfo.Serialize(writer);


                    writer.EndObject();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgAttackMonster json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgAttackMonster::Deserialize(const string& data)
            {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsObject()) {
                        m_winner = static_cast<model::AttackType>(doc["winner"].GetInt());
                        m_myAttackType = static_cast<model::AttackType>(doc["myAttackType"].GetInt());

                        m_attacker.Deserialize(doc["attacker"]);
                        m_defender.Deserialize(doc["defender"]);
                        //m_armyList.Deserialize(doc["armyList"]);

                        m_timestamp = doc["timestamp"].GetInt64();
                        m_unitId = doc["unitId"].GetInt();
                        m_troopId = doc["troopId"].GetInt();

                        if (doc["pos"].IsArray()) {
                            m_unitPos.x = doc["pos"][0u].GetInt();
                            m_unitPos.y = doc["pos"][1].GetInt();
                        }
                        
                        m_tplId = doc["tplId"].GetInt();

                        if (doc["drops"].IsArray()) {
                            DeserializeDrops(doc["drops"],  m_drops);
                        }

                        m_monsterHpBegin = doc["monsterHpBegin"].GetInt();
                        m_monsterHpEnd = doc["monsterHpEnd"].GetInt();
                        m_isCaptive = doc["isCaptive"].GetInt();
                        m_reportId = doc["reportId"].GetInt();
                        m_reportInfo.Deserialize(doc["reportInfo"]);
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgAttackMonster fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgAttackMonster::Send(Agent* agent)
            {
                MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, pos, drops, armyList, attacker, defender, reportInfo;
                msg.Set("winner", (int)m_winner);
                msg.Set("myAttackType", (int)m_myAttackType);
                msg.Set("battleType", (int)BattleType::MONSTER);
                pos.Set(1, m_unitPos.x);
                pos.Set(2, m_unitPos.y);
                msg.Set("pos", pos);
                msg.Set("tplId", m_tplId);
                msg.Set("monsterHpBegin", m_monsterHpBegin);
                msg.Set("monsterHpEnd", m_monsterHpEnd);
                msg.Set("isCaptive", m_isCaptive);
                SetDropsTable(drops, m_drops);
                msg.Set("drops", drops);

                m_attacker.SetDataTable(attacker);
                m_defender.SetDataTable(defender);
                m_armyList.SetDataTable(armyList);

                msg.Set("attacker", attacker);
                msg.Set("defender", defender);
                msg.Set("armyList", armyList);

                msg.Set("timestamp", m_timestamp);

                m_reportInfo.SetDataTable(reportInfo);
                msg.Set("reportInfo", reportInfo);

                msgout << 6;    //argc
                lua_write(msgout, "onAttackMonster");
                lua_write(msgout, m_id);
                lua_write(msgout, m_unitId);
                lua_write(msgout, m_troopId);
                lua_write(msgout, msg);
                lua_write(msgout, m_reportId);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgAttackMonster::SetData(model::AttackType winner, model::AttackType myAttackType, 
                const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, 
                const Point& unitPos, int tplId, int monsterHpBegin, int monsterHpEnd, 
                const std::vector<model::tpl::DropItem>& dropItems, const ArmyList& armyList, bool isCaptive, 
                int64_t timestamp, int unitId, int troopId, int reportId, const MsgReportInfo& reportInfo)
            {
                m_winner = winner;
                m_myAttackType = myAttackType;
                m_unitPos = unitPos;
                m_tplId = tplId;
                std::copy(dropItems.begin(), dropItems.end(), std::back_inserter(m_drops));
                m_attacker = attacker;
                m_defender = defender;
                m_armyList = armyList;
                m_timestamp = timestamp;
                m_unitId = unitId;
                m_troopId = troopId;
                m_monsterHpBegin = monsterHpBegin;
                m_monsterHpEnd = monsterHpEnd;
                m_isCaptive = isCaptive;
                m_reportId = reportId;
                m_reportInfo = reportInfo;
            }


            /*======  MsgAttackCity  ======*/

            std::string MsgAttackCity::Serialize()
            {
                string jsonString;
                try {
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartObject();

                    writer.Key("winner");
                    writer.Int((int)m_winner);
                    writer.Key("myAttackType");
                    writer.Int((int)m_myAttackType);

                    //attacker
                    writer.Key("attacker");
                    m_attacker.Serialize(writer);
                    //defender
                    writer.Key("defender");
                    m_defender.Serialize(writer);

                    writer.Key("reportId");
                    writer.Int(m_reportId);

                    writer.Key("unitPos");
                    writer.StartArray();
                    writer.Int(m_unitPos.x);
                    writer.Int(m_unitPos.y);
                    writer.EndArray();

                    writer.Key("tplId");
                    writer.Int(m_tplId);

                    writer.Key("drops");
                    SerializeDrops(writer,  m_drops);

                    writer.Key("timestamp");
                    writer.Int64(m_timestamp);
                    writer.Key("unitId");
                    writer.Int(m_unitId);
                    writer.Key("troopId");
                    writer.Int(m_troopId);

                    // reportInfo
                    writer.Key("reportInfo");
                    m_reportInfo.Serialize(writer);

                    writer.EndObject();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgAttackCity json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgAttackCity::Deserialize(const string& data)
            {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsObject()) {
                        m_winner = static_cast<model::AttackType>(doc["winner"].GetInt());
                        m_myAttackType = static_cast<model::AttackType>(doc["myAttackType"].GetInt());

                        m_attacker.Deserialize(doc["attacker"]);
                        m_defender.Deserialize(doc["defender"]);

                        m_reportId = doc["reportId"].GetInt();

                        if (doc["unitPos"].IsArray()) {
                            m_unitPos.x = doc["unitPos"][0u].GetInt();
                            m_unitPos.y = doc["unitPos"][1].GetInt();
                        }

                        m_tplId = doc["tplId"].GetInt();

                        if (doc["drops"].IsArray()) {
                            DeserializeDrops(doc["drops"],  m_drops);
                        }

                        m_timestamp = doc["timestamp"].GetInt64();
                        m_unitId = doc["unitId"].GetInt();
                        m_troopId = doc["troopId"].GetInt();
                        m_reportInfo.Deserialize(doc["reportInfo"]);
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgAttackCity fail: %s\n", ex.what());
                    return false;
                }
            }

            //MsgAttackCity
            void MsgAttackCity::Send(Agent* agent)
            {
                MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, pos, drops, attacker, defender, reportInfo;
                msg.Set("winner", (int)m_winner);
                msg.Set("myAttackType", (int)m_myAttackType);
                msg.Set("battleType", (int)BattleType::SIEGE_CITY);

                m_attacker.SetDataTable(attacker);
                m_defender.SetDataTable(defender);

                msg.Set("attacker", attacker);
                msg.Set("defender", defender);

                pos.Set(1, m_unitPos.x);
                pos.Set(2, m_unitPos.y);
                msg.Set("pos", pos);

                msg.Set("tplId", m_tplId);

                SetDropsTable(drops, m_drops);
                msg.Set("drops", drops);
                msg.Set("timestamp", m_timestamp);

                m_reportInfo.SetDataTable(reportInfo);
                msg.Set("reportInfo", reportInfo);

                msgout << 6;    //argc
                lua_write(msgout, "onAttackCity");
                lua_write(msgout, m_id);
                lua_write(msgout, m_unitId);
                lua_write(msgout, m_troopId);
                lua_write(msgout, msg);
                lua_write(msgout, m_reportId);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgAttackCity::SetData(model::AttackType winner, model::AttackType myAttackType, 
                const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int reportId, const Point& unitPos, 
                int tplId, const std::vector<model::tpl::DropItem>& dropItems, int64_t timestamp, int unitId, int troopId, const MsgReportInfo& reportInfo)
            {
                m_winner = winner;
                m_myAttackType = myAttackType;
                m_attacker = attacker;
                m_defender = defender;
                m_reportId = reportId;
                m_unitPos = unitPos;
                m_tplId = tplId;
                std::copy(dropItems.begin(), dropItems.end(), std::back_inserter(m_drops));
                m_timestamp = timestamp;
                m_unitId = unitId;
                m_troopId = troopId;
                m_reportInfo = reportInfo;
            }
        
            /*======  MsgAttackCaslte  ======*/
            std::string MsgAttackCastle::Serialize()
            {
                string jsonString;
                try {
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartObject();

                    writer.Key("winner");
                    writer.Int((int)m_winner);
                    writer.Key("myAttackType");
                    writer.Int((int)m_myAttackType);

                    //attacker
                    writer.Key("attacker");
                    m_attacker.Serialize(writer);
                    //defender
                    writer.Key("defender");
                    m_defender.Serialize(writer);

                    writer.Key("reportId");
                    writer.Int(m_reportId);

                    writer.Key("unitPos");
                    writer.StartArray();
                    writer.Int(m_unitPos.x);
                    writer.Int(m_unitPos.y);
                    writer.EndArray();

                    writer.Key("tplId");
                    writer.Int(m_tplId);

                    writer.Key("armyHurt");
                    writer.Int(m_armyHurt);
                    writer.Key("food");
                    writer.Int(m_food);
                    writer.Key("wood");
                    writer.Int(m_wood);
                    writer.Key("iron");
                    writer.Int(m_iron);
                    writer.Key("stone");
                    writer.Int(m_stone);

                    writer.Key("timestamp");
                    writer.Int64(m_timestamp);

                    writer.Key("foodRm");
                    writer.Int(m_foodRemove);
                    writer.Key("woodRm");
                    writer.Int(m_woodRemove);
                    writer.Key("ironRm");
                    writer.Int(m_ironRemove);
                    writer.Key("stoneRm");
                    writer.Int(m_stoneRemove);

                    //collectInfos
                    writer.Key("collectInfos");
                    writer.StartArray();
                    for (const info::CollectInfo & info : m_collectInfos) {
                        writer.StartArray();
                        writer.Int(info.gridId);
                        writer.Int64(info.timestamp);
                        writer.EndArray();
                    }
                    writer.EndArray();

                    writer.Key("burnEndTs");
                    writer.Int64(m_burnEndTimestamp);

                    writer.Key("unitId");
                    writer.Int(m_unitId);
                    writer.Key("troopId");
                    writer.Int(m_troopId);

                    //reportInfo
                    writer.Key("reportInfo");
                    m_reportInfo.Serialize(writer);

                    writer.EndObject();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgAttackCastle json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgAttackCastle::Deserialize(const string& data)
            {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    m_winner = static_cast<model::AttackType>(doc["winner"].GetInt());
                    m_myAttackType = static_cast<model::AttackType>(doc["myAttackType"].GetInt());

                    m_attacker.Deserialize(doc["attacker"]);
                    m_defender.Deserialize(doc["defender"]);

                    m_reportId = doc["reportId"].GetInt();

                    if (doc["unitPos"].IsArray()) {
                        m_unitPos.x = doc["unitPos"][0u].GetInt();
                        m_unitPos.y = doc["unitPos"][1].GetInt();
                    }

                    m_tplId = doc["tplId"].GetInt();

                    m_armyHurt = doc["armyHurt"].GetInt();
                    m_food = doc["food"].GetInt();
                    m_wood = doc["wood"].GetInt();
                    m_iron = doc["iron"].GetInt();
                    m_stone = doc["stone"].GetInt();
                    m_timestamp = doc["timestamp"].GetInt64();

                    m_foodRemove = doc["foodRm"].GetInt();
                    m_woodRemove = doc["woodRm"].GetInt();
                    m_ironRemove = doc["ironRm"].GetInt();
                    m_stoneRemove = doc["stoneRm"].GetInt();

                    if (doc["collectInfos"].IsArray()) {
                        for (size_t i = 0; i < doc["collectInfos"].Size(); ++i) {
                            info::CollectInfo ci;
                            ci.gridId = doc["collectInfos"][i][0u].GetInt();
                            ci.timestamp = doc["collectInfos"][i][1].GetInt64();
                            m_collectInfos.push_back(ci);
                        }
                    }

                    m_burnEndTimestamp = doc["burnEndTs"].GetInt64();

                    m_unitId = doc["unitId"].GetInt();
                    m_troopId = doc["troopId"].GetInt();
                    m_reportInfo.Deserialize(doc["reportInfo"]);
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgAttackCastle fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgAttackCastle::Send(Agent* agent)
            {
                MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, attacker, defender,  collectInfos, pos, reportInfo;
                msg.Set("winner", (int)m_winner);
                msg.Set("myAttackType", (int)m_myAttackType);
                msg.Set("battleType", (int)BattleType::SIEGE_CASTLE);

                m_attacker.SetDataTable(attacker);
                m_defender.SetDataTable(defender);
                msg.Set("attacker", attacker);
                msg.Set("defender", defender);

                pos.Set(1, m_unitPos.x);
                pos.Set(2, m_unitPos.y);
                msg.Set("pos", pos);

                msg.Set("tplId", m_tplId);

                msg.Set("timestamp", m_timestamp);
                msg.Set("armyHurt", m_armyHurt);

                msg.Set("food", m_food);
                msg.Set("wood", m_wood);
                msg.Set("iron", m_iron);
                msg.Set("stone", m_stone);

                msg.Set("foodRm", m_foodRemove);
                msg.Set("woodRm", m_woodRemove);
                msg.Set("ironRm", m_ironRemove);
                msg.Set("stoneRm", m_stoneRemove);

                for (const info::CollectInfo & info : m_collectInfos) {
                    collectInfos.Set(info.gridId, info.timestamp);
                }
                msg.Set("collectInfos", collectInfos);
                msg.Set("burnEndTs",  m_burnEndTimestamp);

                m_reportInfo.SetDataTable(reportInfo);
                msg.Set("reportInfo", reportInfo);

                msgout << 6;    //argc
                lua_write(msgout, "onAttackCastle");
                lua_write(msgout, m_id);
                lua_write(msgout, m_unitId);
                lua_write(msgout, m_troopId);
                lua_write(msgout, msg);
                lua_write(msgout, m_reportId);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgAttackCastle::SetData(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int reportId, const Point& unitPos,  int tplId,
                                          int armyHurt,  int food, int wood, int iron, int stone, int64_t timestamp, int foodRemove, int woodRemove, int ironRemove, int stoneRemove, const std::vector<info::CollectInfo>& collectInfos,
                                          int64_t burnEndTs,int unitId, int troopId, const MsgReportInfo& reportInfo)
            {
                m_winner = winner;
                m_myAttackType = myAttackType;
                m_unitPos = unitPos;
                m_tplId = tplId;
                m_attacker = attacker;
                m_defender = defender;
                m_reportId = reportId;
                m_armyHurt = armyHurt;
                m_food = food;
                m_wood = wood;
                m_iron = iron;
                m_stone = stone;
                m_timestamp = timestamp;
                m_foodRemove = foodRemove;
                m_woodRemove = woodRemove;
                m_ironRemove = ironRemove;
                m_stoneRemove = stoneRemove;
                m_collectInfos = collectInfos;
                m_burnEndTimestamp = burnEndTs;
                m_unitId = unitId;
                m_troopId = troopId;
                m_reportInfo = reportInfo;
            }

             /*======  MsgBeatCaslte  ======*/
            std::string MsgBeatCastle::Serialize()
            {
                string jsonString;
                try {
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartObject();

                    writer.Key("winner");
                    writer.Int((int)m_winner);
                    writer.Key("myAttackType");
                    writer.Int((int)m_myAttackType);

                     //attacker
                    writer.Key("attacker");
                    m_attacker.Serialize(writer);
                    //defender
                    writer.Key("defender");
                    m_defender.Serialize(writer);

                    writer.Key("food");
                    writer.Int(m_food);
                    writer.Key("wood");
                    writer.Int(m_wood);
                    writer.Key("iron");
                    writer.Int(m_iron);
                    writer.Key("stone");
                    writer.Int(m_stone);

                    writer.Key("timestamp");
                    writer.Int64(m_timestamp);

                    writer.Key("foodRm");
                    writer.Int(m_foodRemove);
                    writer.Key("woodRm");
                    writer.Int(m_woodRemove);
                    writer.Key("ironRm");
                    writer.Int(m_ironRemove);
                    writer.Key("stoneRm");
                    writer.Int(m_stoneRemove);

                    //collectInfos
                    writer.Key("collectInfos");
                    writer.StartArray();
                    for (const info::CollectInfo & info : m_collectInfos) {
                        writer.StartArray();
                        writer.Int(info.gridId);
                        writer.Int64(info.timestamp);
                        writer.EndArray();
                    }
                    writer.EndArray();

                    writer.Key("burnEndTs");
                    writer.Int64(m_burnEndTimestamp);

                    writer.Key("unitId");
                    writer.Int(m_unitId);
                    writer.Key("troopId");
                    writer.Int(m_troopId);

                    writer.EndObject();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgBeatCaslte json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgBeatCastle::Deserialize(const string& data)
            {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    m_winner = static_cast<model::AttackType>(doc["winner"].GetInt());
                    m_myAttackType = static_cast<model::AttackType>(doc["myAttackType"].GetInt());

                    m_attacker.Deserialize(doc["attacker"]);
                    m_defender.Deserialize(doc["defender"]);

                    m_food = doc["food"].GetInt();
                    m_wood = doc["wood"].GetInt();
                    m_iron = doc["iron"].GetInt();
                    m_stone = doc["stone"].GetInt();
                    m_timestamp = doc["timestamp"].GetInt64();

                    m_foodRemove = doc["foodRm"].GetInt();
                    m_woodRemove = doc["woodRm"].GetInt();
                    m_ironRemove = doc["ironRm"].GetInt();
                    m_stoneRemove = doc["stoneRm"].GetInt();

                    if (doc["collectInfos"].IsArray()) {
                        for (size_t i = 0; i < doc["collectInfos"].Size(); ++i) {
                            info::CollectInfo ci;
                            ci.gridId = doc["collectInfos"][i][0u].GetInt();
                            ci.timestamp = doc["collectInfos"][i][1].GetInt();
                            m_collectInfos.push_back(ci);
                        }
                    }

                    m_burnEndTimestamp = doc["burnEndTs"].GetInt64();

                    m_unitId = doc["unitId"].GetInt();
                    m_troopId = doc["troopId"].GetInt();

                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgBeatCaslte fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgBeatCastle::Send(Agent* agent)
            {
                MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, attacker, defender,  collectInfos, pos;
                msg.Set("winner", (int)m_winner);
                msg.Set("myAttackType", (int)m_myAttackType);
                msg.Set("battleType", (int)BattleType::SIEGE_CASTLE);

                m_attacker.SetDataTable(attacker);
                m_defender.SetDataTable(defender);
                msg.Set("attacker", attacker);
                msg.Set("defender", defender);

                msg.Set("timestamp", m_timestamp);

                msg.Set("food", m_food);
                msg.Set("wood", m_wood);
                msg.Set("iron", m_iron);
                msg.Set("stone", m_stone);
                msg.Set("foodRm", m_foodRemove);
                msg.Set("woodRm", m_woodRemove);
                msg.Set("ironRm", m_ironRemove);
                msg.Set("stoneRm", m_stoneRemove);

                for (const info::CollectInfo & info : m_collectInfos) {
                    collectInfos.Set(info.gridId, info.timestamp);
                }
                msg.Set("collectInfos", collectInfos);
                msg.Set("burnEndTs",  m_burnEndTimestamp);

                msgout << 6;    //argc
                lua_write(msgout, "onBeatCastle");
                lua_write(msgout, m_id);
                lua_write(msgout, m_unitId);
                lua_write(msgout, m_troopId);
                lua_write(msgout, msg);
                lua_write(msgout, "");
                agent->proxy()->SendMessage(msgout);
            }

            void MsgBeatCastle::SetData(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender,int food, int wood, int iron, int stone, int64_t timestamp, int foodRemove, 
                int woodRemove, int ironRemove, int stoneRemove, const std::vector<info::CollectInfo>& collectInfos, int64_t burnEndTs, int unitId, int troopId)
            {
                m_winner = winner;
                m_myAttackType = myAttackType;
                m_food = food;
                m_wood = wood;
                m_iron = iron;
                m_stone = stone;
                m_timestamp = timestamp;
                m_foodRemove = foodRemove;
                m_woodRemove = woodRemove;
                m_ironRemove = ironRemove;
                m_stoneRemove = stoneRemove;
                m_collectInfos = collectInfos;
                m_burnEndTimestamp = burnEndTs;
                m_unitId = unitId;
                m_troopId = troopId;
                m_attacker = attacker;
                m_defender = defender;
            }

            /*======  MsgCamp  ======*/

            std::string MsgCamp::Serialize()
            {
                string jsonString;
                try {
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartObject();

                    writer.Key("winner");
                    writer.Int((int)m_winner);
                    writer.Key("myAttackType");
                    writer.Int((int)m_myAttackType);

                    //attacker
                    writer.Key("attacker");
                    m_attacker.Serialize(writer);
                    //defender
                    writer.Key("defender");
                    m_defender.Serialize(writer);

                    writer.Key("reportId");
                    writer.Int(m_reportId);

                    writer.Key("unitPos");
                    writer.StartArray();
                    writer.Int(m_unitPos.x);
                    writer.Int(m_unitPos.y);
                    writer.EndArray();

                    writer.Key("tplId");
                    writer.Int(m_tplId);

                    writer.Key("timestamp");
                    writer.Int64(m_timestamp);
                    writer.Key("unitId");
                    writer.Int(m_unitId);
                    writer.Key("troopId");
                    writer.Int(m_troopId);

                    // reportInfo
                    writer.Key("reportInfo");
                    m_reportInfo.Serialize(writer);

                    writer.EndObject();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgCamp json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgCamp::Deserialize(const string& data)
            {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsObject()) {
                        m_winner = static_cast<model::AttackType>(doc["winner"].GetInt());
                        m_myAttackType = static_cast<model::AttackType>(doc["myAttackType"].GetInt());

                        m_attacker.Deserialize(doc["attacker"]);
                        m_defender.Deserialize(doc["defender"]);

                        m_reportId = doc["reportId"].GetInt();

                        if (doc["unitPos"].IsArray()) {
                            m_unitPos.x = doc["unitPos"][0u].GetInt();
                            m_unitPos.y = doc["unitPos"][1].GetInt();
                        }

                        m_tplId = doc["tplId"].GetInt();

                        m_timestamp = doc["timestamp"].GetInt64();
                        m_unitId = doc["unitId"].GetInt();
                        m_troopId = doc["troopId"].GetInt();
                        m_reportInfo.Deserialize(doc["reportInfo"]);
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgCamp fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgCamp::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, attacker, defender, pos, reportInfo;
                msg.Set("winner", (int)m_winner);
                msg.Set("myAttackType", (int)m_myAttackType);
                msg.Set("battleType", battleType());

                //attacker
                m_attacker.SetDataTable(attacker);
                m_defender.SetDataTable(defender);
                msg.Set("attacker", attacker);
                msg.Set("defender", defender);

                pos.Set(1, m_unitPos.x);
                pos.Set(2, m_unitPos.y);
                msg.Set("pos", pos);

                msg.Set("tplId", m_tplId);

                msg.Set("timestamp", m_timestamp);

                m_reportInfo.SetDataTable(reportInfo);

                msg.Set("reportInfo", reportInfo);

                msgout << 6;    //argc
                lua_write(msgout, EventName());
                lua_write(msgout, m_id);
                lua_write(msgout, m_unitId);
                lua_write(msgout, m_troopId);
                lua_write(msgout, msg);
                lua_write(msgout, m_reportId);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgCamp::SetData(model::AttackType winner, model::AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int reportId, const Point& unitPos, int tplId, int64_t timestamp, int unitId, int troopId, const MsgReportInfo& reportInfo)
            {
                m_winner = winner;
                m_myAttackType = myAttackType;
                m_unitPos = unitPos;
                m_tplId = tplId;
                m_attacker = attacker;
                m_defender = defender;
                m_reportId = reportId;
                m_timestamp = timestamp;
                m_unitId = unitId;
                m_troopId = troopId;
                m_reportInfo = reportInfo;
            }

            int MsgAttackCampFixed::battleType() {
                return (int)BattleType::CAMP_FIXED;
            }

            int MsgOccupyCampFixed::battleType() {
                return (int)BattleType::CAMP_FIXED;
            }

            int MsgAttackCampTemp::battleType() {
                return (int)BattleType::CAMP_TEMP;
            }

            /*======  MsgAttackPalace  ======*/
            std::string MsgAttackPalace::Serialize()
            {
                string jsonString;
                try {
                    // [ winner, myAttackType, attacker, defender, timestamp, unitId, troopId ]
                    // attacker, defender = [attackType, uid, nickname, allianceName, headId, [x, y], turretLevel, turretKills, [[uid, nickname, [<tplid, <state, count>>]]]
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();
                    writer.Int((int)m_winner);
                    writer.Int((int)m_myAttackType);
                    //attacker
                    m_attacker.Serialize(writer);

                    //defender
                    m_defender.Serialize(writer);

                    writer.Int64(m_timestamp);
                    writer.Int64(m_unitId);
                    writer.Int64(m_troopId);

                    //dragon
                    writer.StartArray();
                    writer.Int(m_dragon.beginHp);
                    writer.Int(m_dragon.endHp);
                    writer.Int(m_dragon.maxHp);
                    writer.EndArray();

                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgAttackPalace json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgAttackPalace::Deserialize(const string& data)
            {
                try {
                    // [ winner, myAttackType, attacker, defender, timestamp, unitId, troopId ]
                    // attacker, defender = [attackType, uid, nickname, allianceName, headId, [x, y], turretLevel, turretKills, [[uid, nickname, [<tplid, <state, count>>]]]
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray() && doc.Size() == 8) {
                        m_winner = static_cast<model::AttackType>(doc[0u].GetInt());
                        m_myAttackType = static_cast<model::AttackType>(doc[1].GetInt());

                        m_attacker.Deserialize(doc[2]);
                        m_defender.Deserialize(doc[3]);

                        m_timestamp = doc[4].GetInt64();
                        m_unitId = doc[5].GetInt();
                        m_troopId = doc[6].GetInt();

                        if (doc[7].IsArray()) {
                            auto& doc7 = doc[7];
                            m_dragon.beginHp = doc7[0u].GetInt();
                            m_dragon.endHp = doc7[1].GetInt();
                            m_dragon.maxHp = doc7[2].GetInt();
                        }
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgAttackPalace fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgAttackPalace::Send(Agent* agent)
            {
//                 ms::map::msgqueue::MsgRecord::Send(agent);
// 
//                 base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
//                 DataTable msg, attacker, defender, dragon;
//                 msg.Set("winner", (int)m_winner);
//                 msg.Set("battleType", (int)BattleType::PALACE);
//                 msg.Set("myAttackType", (int)m_myAttackType);
//                 //attacker
//                 m_attacker.SetDataTable(attacker);
//                 m_defender.SetDataTable(defender);
//                 m_dragon.SetDataTable(dragon);
// 
//                 msg.Set("attacker", attacker);
//                 msg.Set("defender", defender);
//                 msg.Set("dragon", dragon);
//                 msg.Set("timestamp", m_timestamp);
// 
//                 msgout << 5;    //argc
//                 lua_write(msgout, "onAttackPalace");
//                 lua_write(msgout, m_id);
//                 lua_write(msgout, m_unitId);
//                 lua_write(msgout, m_troopId);
//                 lua_write(msgout, msg);
//                 agent->proxy()->SendMessage(msgout);
            }

            void MsgAttackPalace::SetData(AttackType winner, AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int64_t timestamp, int unitId, int troopId, const MsgPvpDragonInfo& dragon)
            {
                m_winner = winner;
                m_myAttackType = myAttackType;
                m_attacker = attacker;
                m_defender = defender;
                m_timestamp = timestamp;
                m_unitId = unitId;
                m_troopId = troopId;
                m_dragon = dragon;
            }

            /*======  MsgAttackNeutralCastle  ======*/
            std::string MsgAttackNeutralCastle::Serialize()
            {
                string jsonString;
                try {
                    // [ winner, myAttackType, attacker, defender, timestamp, unitId, troopId ]
                    // attacker, defender = [attackType, uid, nickname, allianceName, headId, [x, y], turretLevel, turretKills, [[uid, nickname, [<tplid, <state, count>>]]]
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();
                    writer.Int((int)m_winner);
                    writer.Int((int)m_myAttackType);
                    //attacker
                    m_attacker.Serialize(writer);

                    //defender
                    m_defender.Serialize(writer);

                    writer.Int64(m_timestamp);
                    writer.Int64(m_unitId);
                    writer.Int64(m_troopId);

                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgAttackNeutralCastle json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgAttackNeutralCastle::Deserialize(const string& data)
            {
                try {
                    // [ winner, myAttackType, attacker, defender, timestamp, unitId, troopId ]
                    // attacker, defender = [attackType, uid, nickname, allianceName, headId, [x, y], turretLevel, turretKills, [[uid, nickname, [<tplid, <state, count>>]]]
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray() && doc.Size() == 7) {
                        m_winner = static_cast<model::AttackType>(doc[0u].GetInt());
                        m_myAttackType = static_cast<model::AttackType>(doc[1].GetInt());
                        m_attacker.Deserialize(doc[2]);
                        m_defender.Deserialize(doc[3]);

                        m_timestamp = doc[4].GetInt64();
                        m_unitId = doc[5].GetInt();
                        m_troopId = doc[6].GetInt();
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgAttackNeutralCastle fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgAttackNeutralCastle::Send(Agent* agent)
            {
//                 ms::map::msgqueue::MsgRecord::Send(agent);
// 
//                 base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
//                 DataTable msg, attacker, defender;
//                 msg.Set("winner", (int)m_winner);
//                 msg.Set("battleType", (int)BattleType::NEUTRAL_CASTLE);
//                 msg.Set("myAttackType", (int)m_myAttackType);
//                 //attacker
//                 m_attacker.SetDataTable(attacker);
//                 m_defender.SetDataTable(defender);
// 
//                 msg.Set("attacker", attacker);
//                 msg.Set("defender", defender);
//                 msg.Set("timestamp", m_timestamp);
// 
//                 msgout << 5;    //argc
//                 lua_write(msgout, "onAttackNeutralCastle");
//                 lua_write(msgout, m_id);
//                 lua_write(msgout, m_unitId);
//                 lua_write(msgout, m_troopId);
//                 lua_write(msgout, msg);
//                 agent->proxy()->SendMessage(msgout);
            }

            void MsgAttackNeutralCastle::SetData(AttackType winner, AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int64_t timestamp, int unitId, int troopId)
            {
                m_winner = winner;
                m_myAttackType = myAttackType;
                m_attacker = attacker;
                m_defender = defender;
                m_timestamp = timestamp;
                m_unitId = unitId;
                m_troopId = troopId;
            }

            /*======  MsgAttackCatapult  ======*/
            string MsgAttackCatapult::Serialize()
            {
                string jsonString;
                try {
                    // [ winner, myAttackType, attacker, defender, timestamp, unitId, troopId ]
                    // attacker, defender = [attackType, uid, nickname, allianceName, headId, [x, y], turretLevel, turretKills, [[uid, nickname, [<tplid, <state, count>>]]]
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();
                    writer.Int((int)m_winner);
                    writer.Int((int)m_myAttackType);
                    //attacker
                    m_attacker.Serialize(writer);

                    //defender
                    m_defender.Serialize(writer);

                    writer.Key("drops");
                    SerializeDrops(writer,  m_dropItems);

                    writer.Int64(m_timestamp);
                    writer.Int64(m_unitId);
                    writer.Int64(m_troopId);
                    writer.Int64(m_reportId);
                    // reportInfo
                    m_reportInfo.Serialize(writer);

                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgAttackNeutralCastle json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgAttackCatapult::Deserialize(const string& data)
            {
                try {
                    // [ winner, myAttackType, attacker, defender, timestamp, unitId, troopId ]
                    // attacker, defender = [attackType, uid, nickname, allianceName, headId, [x, y], turretLevel, turretKills, [[uid, nickname, [<tplid, <state, count>>]]]
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray() && doc.Size() == 10) {
                        m_winner = static_cast<model::AttackType>(doc[0u].GetInt());
                        m_myAttackType = static_cast<model::AttackType>(doc[1].GetInt());
                        //attacker
                        m_attacker.Deserialize(doc[2]);
                        //defender
                        m_defender.Deserialize(doc[3]);
                        //dropItems
                        if (doc[4].IsArray()) {
                            DeserializeDrops(doc["drops"],  m_dropItems);
                        }

                        m_timestamp = doc[5].GetInt64();
                        m_unitId = doc[6].GetInt();
                        m_troopId = doc[7].GetInt();
                        m_reportId = doc[8].GetInt();
                        // reportInfo
                        m_reportInfo.Deserialize(doc[9]);
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgAttackNeutralCastle fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgAttackCatapult::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, attacker, defender, drops, reportInfo;
                msg.Set("winner", (int)m_winner);
                msg.Set("battleType", (int)BattleType::CATAPULT);
                msg.Set("myAttackType", (int)m_myAttackType);
                //attacker
                m_attacker.SetDataTable(attacker);
                m_defender.SetDataTable(defender);
                SetDropsTable(drops, m_dropItems);

                msg.Set("attacker", attacker);
                msg.Set("defender", defender);
                msg.Set("timestamp", m_timestamp);
                m_reportInfo.SetDataTable(reportInfo);
                msg.Set("reportInfo", reportInfo);
                msg.Set("drops", drops);

                msgout << 6;    //argc
                lua_write(msgout, "onAttackCatapult");
                lua_write(msgout, m_id);
                lua_write(msgout, m_unitId);
                lua_write(msgout, m_troopId);
                lua_write(msgout, msg);
                lua_write(msgout, m_reportId);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgAttackCatapult::SetData(AttackType winner, AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, const std::vector<model::tpl::DropItem>& dropItems, int reportId, int64_t timestamp, int unitId, int troopId, const MsgReportInfo& reportInfo)
            {
                m_winner = winner;
                m_myAttackType = myAttackType;
                m_attacker = attacker;
                m_defender = defender;
                m_timestamp = timestamp;
                m_unitId = unitId;
                m_troopId = troopId;
                m_reportId = reportId;
                m_reportInfo = reportInfo;
                std::copy(dropItems.begin(), dropItems.end(), std::back_inserter(m_dropItems));
            }

            /*======  MsgAttackGoblinCamp  ======*/
            string MsgAttackGoblinCamp::Serialize()
            {
                string jsonString;
                try {
                    // [ winner, myAttackType, attacker, defender, timestamp, unitId, troopId ]
                    // attacker, defender = [attackType, uid, nickname, allianceName, headId, [x, y], turretLevel, turretKills, [[uid, nickname, [<tplid, <state, count>>]]]
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();
                    writer.Int((int)m_winner);
                    writer.Int((int)m_myAttackType);
                    //attacker
                    m_attacker.Serialize(writer);

                    //defender
                    m_defender.Serialize(writer);

                    writer.Int64(m_timestamp);
                    writer.Int64(m_unitId);
                    writer.Int64(m_troopId);

                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgAttackNeutralCastle json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgAttackGoblinCamp::Deserialize(const string& data)
            {
                try {
                    // [ winner, myAttackType, attacker, defender, timestamp, unitId, troopId ]
                    // attacker, defender = [attackType, uid, nickname, allianceName, headId, [x, y], turretLevel, turretKills, [[uid, nickname, [<tplid, <state, count>>]]]
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray() && doc.Size() == 7) {
                        m_winner = static_cast<model::AttackType>(doc[0u].GetInt());
                        m_myAttackType = static_cast<model::AttackType>(doc[1].GetInt());
                        //attacker
                        m_attacker.Deserialize(doc[2]);
                        //defender
                        m_defender.Deserialize(doc[3]);

                        m_timestamp = doc[4].GetInt64();
                        m_unitId = doc[5].GetInt();
                        m_troopId = doc[6].GetInt();
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgAttackNeutralCastle fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgAttackGoblinCamp::Send(Agent* agent)
            {
//                 ms::map::msgqueue::MsgRecord::Send(agent);
// 
//                 base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
//                 DataTable msg, attacker, defender;
//                 msg.Set("winner", (int)m_winner);
//                 msg.Set("battleType", (int)BattleType::CATAPULT);
//                 msg.Set("myAttackType", (int)m_myAttackType);
//                 //attacker
//                 m_attacker.SetDataTable(attacker);
//                 m_defender.SetDataTable(defender);
// 
//                 msg.Set("attacker", attacker);
//                 msg.Set("defender", defender);
//                 msg.Set("timestamp", m_timestamp);
// 
//                 msgout << 5;    //argc
//                 lua_write(msgout, "onAttackGoblinCamp");
//                 lua_write(msgout, m_id);
//                 lua_write(msgout, m_unitId);
//                 lua_write(msgout, m_troopId);
//                 lua_write(msgout, msg);
//                 agent->proxy()->SendMessage(msgout);
            }

            void MsgAttackGoblinCamp::SetData(AttackType winner, AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int64_t timestamp, int unitId, int troopId)
            {
                m_winner = winner;
                m_myAttackType = myAttackType;
                m_attacker = attacker;
                m_defender = defender;
                m_timestamp = timestamp;
                m_unitId = unitId;
                m_troopId = troopId;
            }


            /*======  MsgScout  ======*/

            std::string MsgScout::Serialize()
            {
                string jsonString;
                try {
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);

                    writer.StartObject();
                    writer.Key("winner");
                    writer.Int((int)m_winner);
                    writer.Key("myAttackType");
                    writer.Int((int)m_myAttackType);

                    writer.Key("scoutTypes");
                    writer.StartArray();
                    for (size_t i = 0; i < m_scoutTypes.size(); ++i) {
                        writer.Int((int)m_scoutTypes[i]);
                    }
                    writer.EndArray();

                    //attacker
                    writer.Key("attacker");
                    writer.StartObject();
                    writer.Key("uid");
                    writer.Int64(m_attacker.uid);
                    writer.Key("nickname");
                    writer.String(m_attacker.nickname.c_str());
                    writer.Key("allianceNickname");
                    writer.String(m_attacker.allianceNickname.c_str());
                    writer.Key("headId");
                    writer.Int64(m_attacker.headId);
                    writer.Key("castlePos");
                    writer.StartArray();
                    writer.Int(m_attacker.castlePos.x);
                    writer.Int(m_attacker.castlePos.y);
                    writer.EndArray();
                    writer.EndObject();

                    //defender
                    writer.Key("defender");
                    writer.StartObject();

                    writer.Key("uid");
                    writer.Int64(m_defender.uid);
                    writer.Key("nickname");
                    writer.String(m_defender.nickname.c_str());
                    writer.Key("allianceNickname");
                    writer.String(m_defender.allianceNickname.c_str());
                    writer.Key("headId");
                    writer.Int64(m_defender.headId);
                    writer.Key("lordLevel");
                    writer.Int(m_defender.lordLevel);

                    writer.Key("castlePos");
                    writer.StartArray();
                    writer.Int(m_defender.castlePos.x);
                    writer.Int(m_defender.castlePos.y);
                    writer.EndArray();

                    writer.Key("targetPos");
                    writer.StartArray();
                    writer.Int(m_defender.targetPos.x);
                    writer.Int(m_defender.targetPos.y);
                    writer.EndArray();

                    writer.Key("targetTplId");
                    writer.Int(m_defender.targetTplId);

                    writer.Key("result");
                    string result = m_defender.result.Serialize();
                    writer.String(result.c_str(), result.size());

                    writer.EndObject();

                    writer.Key("timestamp");
                    writer.Int64(m_timestamp);

                    writer.EndObject();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgScout json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgScout::Deserialize(const string& data)
            {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsObject()) {
                        m_winner = static_cast<model::AttackType>(doc["winner"].GetInt());
                        m_myAttackType = static_cast<model::AttackType>(doc["myAttackType"].GetInt());
                        //scoutTypes
                        if (doc["scoutTypes"].IsArray()) {
                            for (size_t i = 0; i < doc["scoutTypes"].Size(); ++i) {
                                m_scoutTypes.push_back(static_cast<model::WATCHTOWER_SCOUT_TYPE>(doc["scoutTypes"][i].GetInt()));
                            }
                        }

                        //attacker
                        if (doc["attacker"].IsObject()) {
                            auto& attackerDoc = doc["attacker"];
                            m_attacker.uid = attackerDoc["uid"].GetInt64();
                            m_attacker.nickname = attackerDoc["nickname"].GetString();
                            m_attacker.allianceNickname = attackerDoc["allianceNickname"].GetString();
                            m_attacker.headId = attackerDoc["headId"].GetInt64();
                            if (attackerDoc["castlePos"].IsArray()) {
                                m_attacker.castlePos.x = attackerDoc["castlePos"][0u].GetInt();
                                m_attacker.castlePos.y = attackerDoc["castlePos"][1].GetInt();
                            }
                        }

                        //defender
                        if (doc["defender"].IsObject()) {
                            auto& defenderDoc = doc["defender"];
                            m_defender.uid = defenderDoc["uid"].GetInt64();
                            m_defender.nickname = defenderDoc["nickname"].GetString();
                            m_defender.allianceNickname = defenderDoc["allianceNickname"].GetString();
                            m_defender.headId = defenderDoc["headId"].GetInt64();
                            m_defender.lordLevel = defenderDoc["lordLevel"].GetInt();

                            if (defenderDoc["castlePos"].IsArray()) {
                                m_defender.castlePos.x = defenderDoc["castlePos"][0u].GetInt();
                                m_defender.castlePos.y = defenderDoc["castlePos"][1].GetInt();
                            }

                            if (defenderDoc["targetPos"].IsArray()) {
                                m_defender.targetPos.x = defenderDoc["targetPos"][0u].GetInt();
                                m_defender.targetPos.y = defenderDoc["targetPos"][1].GetInt();
                            }

                            m_defender.targetTplId = defenderDoc["targetTplId"].GetInt();

                            string result = defenderDoc["result"].GetString();
                            m_defender.result = base::lua::Parser::ParseAsDataTable(result);
                        }

                        m_timestamp = doc["timestamp"].GetInt64();
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgScout fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgScout::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, scoutTypes, attacker, defender;
                msg.Set("winner", (int)m_winner);
                msg.Set("myAttackType", (int)m_myAttackType);
                for (size_t i = 0; i < m_scoutTypes.size(); ++i) {
                    scoutTypes.Set(i + 1, (int)m_scoutTypes[i]);
                }
                msg.Set("scoutTypes", scoutTypes);
                m_attacker.SetDataTable(attacker);
                m_defender.SetDataTable(defender,  m_myAttackType ==  m_winner);
                msg.Set("attacker", attacker);
                msg.Set("defender", defender);
                msg.Set("timestamp", m_timestamp);

                msgout << 3;    //argc
                lua_write(msgout, "onScout");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgScout::SetData(AttackType winner, AttackType myAttackType, const std::vector< WATCHTOWER_SCOUT_TYPE >& scoutTypes, const MsgPlayerInfo& attacker, const MsgScoutDefenderInfo& defender, int64_t timestamp)
            {
                m_winner = winner;
                m_myAttackType = myAttackType;
                m_scoutTypes = scoutTypes;
                m_attacker = attacker;
                m_defender = defender;
                m_timestamp = timestamp;
            }

            /*======  MsgBuffRemove  ======*/
            std::string MsgBuffRemove::Serialize()
            {
                string jsonString;
                try {
                    // [ type ]
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();

                    writer.Int((int)m_type);

                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgBuffRemove json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgBuffRemove::Deserialize(const string& data)
            {
                try {
                    // [ type ]
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray() && doc.Size() == 1) {
                        m_type = static_cast<model::BuffType>(doc[0u].GetInt());
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgBuffRemove fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgBuffRemove::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg;
                msg.Set("type", (int)m_type);

                msgout << 3;    //argc
                lua_write(msgout, "onBuffRemove");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgBuffRemove::SetData(BuffType type)
            {
                m_type = type;
            }

            /*======  MsgResourceHelp  ======*/
            std::string MsgResourceHelp::Serialize()
            {
                string jsonString;
                try {
                    // player = [attackType, uid, nickname, allianceName, headId, [x, y]]
                    // [<tplid, count>], timestamp
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();

                    //player
                    writer.StartArray();
                    writer.Int64(m_player.uid);
                    writer.String(m_player.nickname.c_str());
                    writer.Int64(m_player.headId);
                    writer.String(m_player.allianceName.c_str());
                    writer.String(m_player.allianceNickname.c_str());
                    writer.Int(m_player.allianceBannerId);
                    writer.StartArray();
                    writer.Int(m_player.castlePos.x);
                    writer.Int(m_player.castlePos.y);
                    writer.EndArray();
                    writer.EndArray();

                    //drops
                    SerializeDrops(writer, m_drops);
                    writer.Int64(m_timestamp);
                    writer.Int((int)m_myAttackType);

                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgResourceHelp json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgResourceHelp::Deserialize(const string& data)
            {
                try {
                    // player = [attackType, uid, nickname, allianceName, headId, [x, y]]
                    // [<tplid, count>], timestamp
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray() && doc.Size() == 4) {
                        //player
                        if (doc[0u].IsArray()) {
                            auto& doc0 = doc[0u];
                            m_player.uid = doc0[0u].GetInt64();
                            m_player.nickname = doc0[1].GetString();
                            m_player.headId = doc0[2].GetInt64();
                            m_player.allianceName = doc0[3].GetString();
                            m_player.allianceNickname = doc0[4].GetString();
                            m_player.allianceBannerId = doc0[5].GetInt();
                            if (doc0[6].IsArray()) {
                                m_player.castlePos.x = doc0[6][0u].GetInt();
                                m_player.castlePos.y = doc0[6][1].GetInt();
                            }
                        }

                        DeserializeDrops(doc[1], m_drops);
                        m_timestamp = doc[2].GetInt64();
                        m_myAttackType = (AttackType)doc[3].GetInt();
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgResourceHelp fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgResourceHelp::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, player, drops;
                m_player.SetDataTable(player);
                msg.Set("player", player);
                SetDropsTable(drops, m_drops);
                msg.Set("drops", drops);
                msg.Set("myAttackType", (int)m_myAttackType);
                msg.Set("timestamp", m_timestamp);

                msgout << 3;    //argc
                lua_write(msgout, "onResourceHelp");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgResourceHelp::SetData(const MsgPlayerInfo& player, int food, int wood, int iron, int stone, AttackType myAttackType, int64_t timestamp)
            {
                m_player = player;
                PushResourceToDrops(m_drops, food, wood, iron, stone, 0);
                m_myAttackType = myAttackType;
                m_timestamp = timestamp;
            }

            /*======  MsgReinforcements  ======*/
            string MsgReinforcements::Serialize()
            {
                string jsonString;
                try {
                    // player = [uid, nickname, allianceName, headId, [x, y]]
                    // timestamp
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();

                    //player
                    writer.StartArray();
                    writer.Int64(m_player.uid);
                    writer.String(m_player.nickname.c_str());
                    writer.Int64(m_player.headId);
                    writer.String(m_player.allianceName.c_str());
                    writer.String(m_player.allianceNickname.c_str());
                    writer.Int(m_player.allianceBannerId);
                    writer.StartArray();
                    writer.Int(m_player.castlePos.x);
                    writer.Int(m_player.castlePos.y);
                    writer.EndArray();
                    writer.EndArray();

                    writer.Int((int)m_myAttackType);
                    m_armyList.Serialize(writer);
                    writer.Int64(m_timestamp);

                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgReinforcements json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgReinforcements::Deserialize(const string& data)
            {
                try {
                    // player = [uid, nickname, allianceName, headId, [x, y]]
                    // timestamp
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray() && doc.Size() == 4) {
                        //player
                        if (doc[0u].IsArray()) {
                            auto& doc0 = doc[0u];
                            m_player.uid = doc0[0u].GetInt64();
                            m_player.nickname = doc0[1].GetString();
                            m_player.headId = doc0[2].GetInt64();
                            m_player.allianceName = doc0[3].GetString();
                            m_player.allianceNickname = doc0[4].GetString();
                            m_player.allianceBannerId = doc0[5].GetInt();
                            if (doc0[6].IsArray()) {
                                m_player.castlePos.x = doc0[6][0u].GetInt();
                                m_player.castlePos.y = doc0[6][1].GetInt();
                            }
                        }

                        m_myAttackType = (AttackType)doc[1].GetInt();
                        m_armyList.Desrialize(doc[2]);
                        m_timestamp = doc[3].GetInt64();
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgReinforcements fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgReinforcements::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, player, armyList;
                m_player.SetDataTable(player);
                msg.Set("player", player);
                m_armyList.SetDataTable(armyList);
                msg.Set("armyList", armyList);
                msg.Set("myAttackType", (int)m_myAttackType);
                msg.Set("timestamp", m_timestamp);

                msgout << 3;    //argc
                lua_write(msgout, "onReinforcements");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgReinforcements::SetData(const msgqueue::MsgPlayerInfo& player, model::AttackType myAttackType, const ArmyList& armyList, int64_t timestamp)
            {
                m_player = player;
                m_armyList = armyList;
                m_myAttackType = myAttackType;
                m_timestamp = timestamp;
            }


            /*======  MsgNeutralCastleNoticeMail  ======*/

            std::string MsgNeutralCastleNoticeMail::Serialize()
            {
                string jsonString;
                try {
                    // [ mailSubType, timestamp ]
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();

                    writer.Int((int)m_mailSubType);
                    writer.Int64(m_timestamp);
                    //allianceInfo
                    writer.StartArray();
                    writer.String(m_alliance.allianceName.c_str());
                    writer.String(m_alliance.allianceNickname.c_str());
                    writer.Int(m_alliance.bannerId);
                    writer.EndArray();
                    writer.String(m_param1.c_str());

                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgNeutralCastleNoticeMail json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgNeutralCastleNoticeMail::Deserialize(const string& data)
            {
                try {
                    // [ mailSubType, timestamp ]
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray() && doc.Size() == 4) {
                        m_mailSubType = static_cast<model::MailSubType>(doc[0u].GetInt());
                        m_timestamp = doc[1].GetInt64();
                        auto& doc2 = doc[2];
                        if (doc2.IsArray() && doc2.Size() == 3) {
                            m_alliance.allianceName = doc2[0u].GetString();
                            m_alliance.allianceNickname = doc2[1].GetString();
                            m_alliance.bannerId = doc2[2].GetInt();
                        }
                        m_param1 = doc[3].GetString();
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgNeutralCastleNoticeMail fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgNeutralCastleNoticeMail::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg;
                msg.Set("mailSubType", (int)m_mailSubType);
                msg.Set("allianceName", m_alliance.allianceName);
                msg.Set("allianceNickname", m_alliance.allianceNickname);
                msg.Set("bannerId", m_alliance.bannerId);
                msg.Set("timestamp", m_timestamp);
                msg.Set("param1", m_param1);

                msgout << 3;    //argc
                lua_write(msgout, "onNeutralCastleNoticeMail");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgNeutralCastleNoticeMail::SetData(MailSubType mailSubType, const MsgAllianceInfo& alliance, int64_t timestamp, const std::string& param1)
            {
                m_mailSubType = mailSubType;
                m_alliance = alliance;
                m_timestamp = timestamp;
                m_param1 = param1;
            }


            /*======  MsgMonsterSiege  ======*/

            std::string MsgMonsterSiege::Serialize()
            {
                string jsonString;
                try {
                    // [ level, [<tplid, count>], foodRemove, woodRemove, ironRemove, stoneRemove, collectInfos, timestamp ]
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();

                    writer.Int(m_level);

                    //drops
                    SerializeDrops(writer, m_drops);

                    writer.Int(m_foodRemove);
                    writer.Int(m_woodRemove);
                    writer.Int(m_ironRemove);
                    writer.Int(m_stoneRemove);

                    //collectInfos
                    writer.StartArray();
                    for (const info::CollectInfo & info : m_collectInfos) {
                        writer.StartArray();
                        writer.Int(info.gridId);
                        writer.Int64(info.timestamp);
                        writer.EndArray();
                    }
                    writer.EndArray();
                    writer.Int64(m_timestamp);

                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgMonsterSiege json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgMonsterSiege::Deserialize(const string& data)
            {
                try {
                    // [ level, [<tplid, count>], foodRemove, woodRemove, ironRemove, stoneRemove, collectInfos, timestamp ]
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray() && doc.Size() == 8) {
                        m_level = doc[0u].GetInt();
                        //drops
                        DeserializeDrops(doc[1], m_drops);
                        m_foodRemove = doc[2].GetInt();
                        m_woodRemove = doc[3].GetInt();
                        m_ironRemove = doc[4].GetInt();
                        m_stoneRemove = doc[5].GetInt();

                        auto& doc6 = doc[6];
                        if (doc6.IsArray()) {
                            for (size_t i = 0; i < doc6.Size(); ++i) {
                                info::CollectInfo ci;
                                ci.gridId = doc6[i][0u].GetInt();
                                ci.timestamp = doc6[i][1].GetInt();
                                m_collectInfos.push_back(ci);
                            }
                        }
                        m_timestamp = doc[7].GetInt64();
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgNeutralCastleNoticeMail fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgMonsterSiege::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, collectInfos, drops;
                msg.Set("level", m_level);
                SetDropsTable(drops, m_drops);
                msg.Set("drops", drops);
                msg.Set("timestamp", m_timestamp);
                for (const info::CollectInfo & info : m_collectInfos) {
                    collectInfos.Set(info.gridId, info.timestamp);
                }

                msgout << 8;    //argc
                lua_write(msgout, "onMonsterSiege");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                lua_write(msgout, m_foodRemove);
                lua_write(msgout, m_woodRemove);
                lua_write(msgout, m_ironRemove);
                lua_write(msgout, m_stoneRemove);
                lua_write(msgout, collectInfos);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgMonsterSiege::SetData(int level, int food, int wood, int iron, int stone, int foodRemove, int woodRemove, int ironRemove, int stoneRemove, const std::vector< info::CollectInfo >& collectInfos, int64_t timestamp)
            {
                m_level = level;
                PushResourceToDrops(m_drops, food, wood, iron, stone, 0);
                m_foodRemove = foodRemove;
                m_woodRemove = woodRemove;
                m_ironRemove = ironRemove;
                m_stoneRemove = stoneRemove;
                m_collectInfos = collectInfos;
                m_timestamp = timestamp;
            }

            /*======  MsgMonsterSiegeResourceGetBack  ======*/
            std::string MsgMonsterSiegeResourceGetBack::Serialize()
            {
                string jsonString;
                try {
                    // [ level, [<tplid, count>], timestamp ]
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();

                    writer.Int(m_level);

                    //drops
                    SerializeDrops(writer, m_drops);

                    writer.Int64(m_timestamp);

                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgMonsterSiegeResourceGetBack json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgMonsterSiegeResourceGetBack::Deserialize(const string& data)
            {
                try {
                    // [ level, [<tplid, count>], timestamp ]
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray() && doc.Size() == 3) {
                        m_level = doc[0u].GetInt();
                        //drops
                        DeserializeDrops(doc[1], m_drops);
                        m_timestamp = doc[2].GetInt64();
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgMonsterSiegeResourceGetBack fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgMonsterSiegeResourceGetBack::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, drops;
                msg.Set("level", m_level);
                SetDropsTable(drops, m_drops);
                msg.Set("drops", drops);
                msg.Set("timestamp", m_timestamp);

                msgout << 3;    //argc
                lua_write(msgout, "onMonsterSiegeResourceGetBack");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgMonsterSiegeResourceGetBack::SetData(int level, int food, int wood, int iron, int stone, int64_t timestamp)
            {
                m_level = level;
                PushResourceToDrops(m_drops, food, wood, iron, stone, 0);
                MergeDrops(m_drops, g_tploader->configure().monsterSiege.extraDrops);
                m_timestamp = timestamp;
            }

            /*======  MsgBeAttackedByCatapult  ======*/
            string MsgBeAttackedByCatapult::Serialize()
            {
                string jsonString;
                try {
                    // [ [<tplid, <state, count>>] ]
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();

                    m_dieList.Serialize(writer);
                    writer.Bool(m_isCaptive);

                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgBeAttackedByCatapult json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgBeAttackedByCatapult::Deserialize(const string& data)
            {
                try {
                    // [ [<tplid, <state, count>>] ]
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray() && doc.Size() == 2) {
                        m_dieList.Desrialize(doc[0u]);
                        m_isCaptive = doc[1].GetBool();
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgBeAttackedByCatapult fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgBeAttackedByCatapult::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, dieList;
                m_dieList.SetDataTable(dieList);
                msg.Set("dieList", dieList);
                msg.Set("isCaptive", m_isCaptive);

                msgout << 3;    //argc
                lua_write(msgout, "onBeAttackedByCatapult");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgBeAttackedByCatapult::SetData(const ArmyList& dieList, bool isCaptive)
            {
                m_dieList = dieList;
                m_isCaptive = isCaptive;
            }

            /*======  MsgMonsterSiegeResourceGetBack  ======*/
            string MsgCastleRebuild::Serialize()
            {
                string jsonString;
                try {
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartObject();

                    writer.Key("castlePos");
                    writer.StartArray();
                    writer.Int(m_castlePos.x);
                    writer.Int(m_castlePos.y);
                    writer.EndArray();

                    writer.Key("timestamp");
                    writer.Int64(m_timestamp);

                    writer.EndObject();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgCastleRebuild json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgCastleRebuild::Deserialize(const string& data)
            {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());

                    if (doc.IsObject()) {
                        if (doc["castlePos"].IsArray()) {
                            m_castlePos.x = doc["castlePos"][0u].GetInt();
                            m_castlePos.y = doc["castlePos"][1].GetInt();
                        }
                        m_timestamp = doc["timestamp"].GetInt64();
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgCastleRebuild fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgCastleRebuild::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, pos;
                msg.Set("timestamp", m_timestamp);

                pos.Set(1, m_castlePos.x);
                pos.Set(2, m_castlePos.y);
                msg.Set("castlePos", pos);

                msgout << 3;    //argc
                lua_write(msgout, "onCastleRebuild");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgCastleRebuild::SetData(Point castlePos, int64_t timestamp)
            {
                m_castlePos = castlePos;
                m_timestamp = timestamp;
            }

            /*======  MsgKillDragonRank  ======*/
            string MsgKillDragonRank::Serialize()
            {
                string jsonString;
                try {
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();

                    writer.Int(m_rank);
                    writer.Int64(m_timestamp);

                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgKillDragonRank json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgKillDragonRank::Deserialize(const string& data)
            {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray() && doc.Size() == 2) {
                        m_rank = doc[0].GetInt();
                        m_timestamp = doc[1].GetInt64();
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgKillDragonRank fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgKillDragonRank::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg;
                msg.Set("rank", m_rank);
                msg.Set("timestamp", m_timestamp);

                msgout << 3;    //argc
                lua_write(msgout, "onKillDragonRank");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgKillDragonRank::SetData(int rank, int64_t timestamp)
            {
                m_rank = rank;
                m_timestamp = timestamp;
            }

            /*======  MsgKillDragonLastAttack  ======*/
            string MsgKillDragonLastAttack::Serialize()
            {
                string jsonString;
                try {
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();

                    writer.Int64(m_timestamp);

                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgKillDragonLastAttack json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgKillDragonLastAttack::Deserialize(const string& data)
            {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray() && doc.Size() == 1) {
                        m_timestamp = doc[0].GetInt64();
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgKillDragonLastAttack fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgKillDragonLastAttack::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg;
                msg.Set("timestamp", m_timestamp);

                msgout << 3;    //argc
                lua_write(msgout, "onKillDragonLastAttack");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgKillDragonLastAttack::SetData(int64_t timestamp)
            {
                m_timestamp = timestamp;
            }


            /*======  MsgExploreMysteriousCity  ======*/
            string MsgExploreMysteriousCity::Serialize()
            {
                string jsonString;
                try {
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();

                    SerializeDrops(writer, m_drops);

                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgExploreMysteriousCity json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgExploreMysteriousCity::Deserialize(const string& data)
            {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray() && doc.Size() == 1) {
                        DeserializeDrops(doc[0u], m_drops);
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgExploreMysteriousCity fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgExploreMysteriousCity::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, dropsTable;
                SetDropsTable(dropsTable, m_drops);
                msg.Set("drops", dropsTable);
                msg.Set("timestamp", m_createTime);

                msgout << 3;    //argc
                lua_write(msgout, "onExploreMysteriousCity");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgExploreMysteriousCity::SetData(const vector< DropItem >& drops)
            {
                std::copy(drops.begin(), drops.end(), std::back_inserter(m_drops));
            }


            //****************************************
            void MsgPvpDetail::SetDataTable(DataTable& table) const
            {
                DataTable armyListTable, trapsTable;
//                 table.Set("uid", uid);
//                 table.Set("nickname", nickname);
//                 table.Set("headId", headId);
                armyList.SetDataTable(armyListTable);
                table.Set("armyList", armyListTable);
                trapSet.SetDataTable(trapsTable);
                table.Set("trapSet",  trapsTable);
            }

            void MsgPvpDetail::SetData(Troop& tp)
            {
//                 uid = tp.uid();
//                 nickname = tp.nickname();
//                 headId = tp.headId();

                if (tp.armyList()) {
                    armyList = *tp.armyList();
                }
            }

            void MsgPlayerInfo::SetDataTable(DataTable& table)
            {
                DataTable pos;
                table.Set("uid", uid);
                table.Set("headId", headId);
                table.Set("nickname", nickname);
                table.Set("allianceName", allianceName);
                table.Set("allianceNickname", allianceNickname);
                table.Set("bannerId", allianceBannerId);
                pos.Set(1, castlePos.x);
                pos.Set(2, castlePos.y);
                table.Set("castlePos", pos);
                table.Set("tplId", tplId);
                table.Set("lordLevel", lordLevel);
            }

            void MsgPlayerInfo::SetData(const Agent& agent)
            {
                uid = agent.uid();
                headId = agent.headId();
                nickname = agent.nickname();
                allianceId = agent.allianceId();
                allianceName = agent.allianceName();
                allianceNickname = agent.allianceNickname();
                allianceBannerId = agent.allianceBannerId();
                if (agent.castle()) {
                    castlePos = agent.castle()->pos();
                }
                lordLevel = agent.level();
            }

            void MsgPlayerInfo::SetUnitData(const Unit* unit)
            {
                uid = 0;
                if (unit) {
//                     headId = unit->tpl().headId;
//                     nickname = unit->tpl().name;
//                     lordLevel = unit->tpl().level;
                    auto npcArmyTpl = tpl::m_tploader->FindNpcArmyTpl(unit->tpl().armyGroup);
                    if (npcArmyTpl) {
                        nickname = npcArmyTpl->name;
                        lordLevel = npcArmyTpl->level;
                        headId = npcArmyTpl->headId;
                    }
                    castlePos = unit->pos();
                    tplId = unit->tpl().id;
                }
            }

            void MsgPvpPlayerInfo::SetDataTable(DataTable& table)
            {
                MsgPlayerInfo::SetDataTable(table);
                DataTable detailsTable;
                detail.SetDataTable(detailsTable);
                table.Set("detail", detailsTable);
            }

            void MsgPvpPlayerInfo::Serialize(rapidjson::Writer< rapidjson::StringBuffer >& writer)
            {
                writer.StartObject();

                writer.Key("uid");
                writer.Int64(uid);
                writer.Key("nickname");
                writer.String(nickname.c_str());
                writer.Key("allianceNickname");
                writer.String(allianceNickname.c_str());
                writer.Key("headId");
                writer.Int64(headId);
                writer.Key("castlePos");
                writer.StartArray();
                writer.Int(castlePos.x);
                writer.Int(castlePos.y);
                writer.EndArray();
                writer.Key("tplId");
                writer.Int(tplId);

                writer.Key("detail");
                writer.StartObject();
                writer.Key("armyList");
                detail.armyList.Serialize(writer);
                writer.Key("trapSet");
                detail.trapSet.Serialize(writer);
                writer.EndObject();

                writer.EndObject();
            }

            void MsgPvpPlayerInfo::Deserialize(rapidjson::GenericValue< rapidjson::UTF8< char > >& gv)
            {
                if (gv.IsObject()) {
                    uid = gv["uid"].GetInt64();
                    nickname = gv["nickname"].GetString();
                    allianceNickname = gv["allianceNickname"].GetString();
                    headId = gv["headId"].GetInt64();
                    if (gv["castlePos"].IsArray()) {
                        castlePos.x = gv["castlePos"][0u].GetInt();
                        castlePos.y = gv["castlePos"][1].GetInt();
                    }
                    tplId = gv["tplId"].GetInt();

                    detail.armyList.Desrialize(gv["detail"]["armyList"]);
                    detail.trapSet.Desrialize(gv["detail"]["trapSet"]);
                }
            }

            void MsgReportInfo::Serialize(rapidjson::Writer< rapidjson::StringBuffer >& writer)
            {
                writer.StartObject();

                writer.Key("attackerBeginArmyCount");
                writer.Int64(attackerBeginArmyCount);
                writer.Key("attackerEndArmyCount");
                writer.Int64(attackerEndArmyCount);

                writer.Key("defenderBeginArmyCount");
                writer.Int64(defenderBeginArmyCount);
                writer.Key("defenderEndArmyCount");
                writer.Int64(defenderEndArmyCount);

                writer.Key("posName");
                writer.String(posName.c_str());

                writer.Key("isLang");
                writer.Int64(isLang);

                writer.Key("level");
                writer.Int64(level);

                writer.EndObject();
            }

            void MsgReportInfo::Deserialize(rapidjson::GenericValue< rapidjson::UTF8< char > >& gv)
            {
                if (gv.IsObject()) {
                    attackerBeginArmyCount = gv["attackerBeginArmyCount"].GetInt64();
                    attackerEndArmyCount = gv["attackerEndArmyCount"].GetInt64();
                    defenderBeginArmyCount = gv["defenderBeginArmyCount"].GetInt64();
                    defenderEndArmyCount = gv["defenderEndArmyCount"].GetInt64();
                    posName = gv["posName"].GetString();
                    isLang = gv["isLang"].GetInt64();
                    level = gv["level"].GetInt64();
                }
            }

            void MsgReportInfo::SetDataTable(base::DataTable& table)
            {
                table.Set("attackerBeginArmyCount", attackerBeginArmyCount);
                table.Set("attackerEndArmyCount", attackerEndArmyCount);
                table.Set("defenderBeginArmyCount", defenderBeginArmyCount);
                table.Set("defenderEndArmyCount", defenderEndArmyCount);
                table.Set("posName", posName);
                table.Set("isLang", isLang);
                table.Set("level", level);
            }

            void MsgPvpDragonInfo::SetDataTable(DataTable& table)
            {
                table.Set("beginHp", beginHp);
                table.Set("endHp", endHp);
                table.Set("maxHp", maxHp);
            }

            void MsgScoutDefenderInfo::SetDataTable(base::DataTable& table, bool isWin)
            {
                ms::map::msgqueue::MsgPlayerInfo::SetDataTable(table);

                DataTable pos;
                pos.Set(1, targetPos.x);
                pos.Set(2, targetPos.y);
                table.Set("targetPos", pos);

                table.Set("targetTplId",  targetTplId);

                if (isWin) {
                    table.Set("result",  result);
                }
            }

            void MsgWorldBossInfo::SetDataTable(DataTable& table)
            {
                table.Set("tplId", tplId);
                table.Set("beginArmyCount", beginArmyCount);
                table.Set("endArmyCount", endArmyCount);
                table.Set("maxArmyCount", maxArmyCount);
            }

            /*======  MsgCityDefenseUpdate  ======*/

            std::string MsgCityDefenseUpdate::Serialize()
            {
                string jsonString;
                try {
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartObject();

                    writer.Key("cityDefense");
                    writer.Int(m_cityDefense);

                    writer.EndObject();

                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgCityDefenseUpdate json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgCityDefenseUpdate::Deserialize(const string& data)
            {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    m_cityDefense = doc["cityDefense"].GetInt();
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgCityDefenseUpdate fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgCityDefenseUpdate::Send(Agent* agent)
            {
                MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());

                msgout << 3;    //argc
                lua_write(msgout, "onCityDefenseUpdate");
                lua_write(msgout, m_id);
                lua_write(msgout, m_cityDefense);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgCityDefenseUpdate::SetData(int cityDefense)
            {
                m_cityDefense = cityDefense;
            }

            void MsgCityInfo::SetData(FamousCity* city)
            {
                targetPos = city->pos();
                targetTplId = city->tpl().id;
                if (city->cityTpl()) {
                    cityId = city->cityTpl()->cityId;
                    cityName = city->cityTpl()->name;
                }

                if (city->troop()) {
                    allianceId = city->troop()->allianceId();
                    allianceName = city->troop()->allianceName();
                    allianceNickname = city->troop()->allianceNickname();
                    allianceBannerId = city->troop()->allianceBannerId();
                }

                troopCount = city->troops().size();
                for (auto troop :  city->troops()) {
                    if (troop) {
                        armyTotalCount +=  troop->GetArmyTotal();
                    }
                }
            }

            void MsgCityInfo::SetDataTable(base::DataTable& table)
            {
                DataTable pos;
                pos.Set(1, targetPos.x);
                pos.Set(2, targetPos.y);
                table.Set("targetPos", pos);

                table.Set("targetTplId",  targetTplId);
                table.Set("cityId",  cityId);
                table.Set("cityName",  cityName);

                table.Set("allianceName", allianceName);
                table.Set("allianceNickname", allianceNickname);
                table.Set("bannerId", allianceBannerId);

                table.Set("troopCount", troopCount);
                table.Set("armyTotalCount", armyTotalCount);
            }

            void MsgCityInfo::Serialize(rapidjson::Writer< rapidjson::StringBuffer >& writer)
            {
                writer.StartObject();

                writer.Key("targetPos");
                writer.StartArray();
                writer.Int(targetPos.x);
                writer.Int(targetPos.y);
                writer.EndArray();

                writer.Key("targetTplId");
                writer.Int(targetTplId);
                writer.Key("cityId");
                writer.Int(cityId);
                writer.Key("cityName");
                writer.String(cityName.c_str());


                writer.Key("allianceName");
                writer.String(allianceName.c_str());
                writer.Key("allianceNickname");
                writer.String(allianceNickname.c_str());
                writer.Key("bannerId");
                writer.Int(allianceBannerId);

                writer.Key("troopCount");
                writer.Int(troopCount);
                writer.Key("armyTotalCount");
                writer.Int(armyTotalCount);

                writer.EndObject();
            }

            void MsgCityInfo::Deserialize(rapidjson::GenericValue< rapidjson::UTF8< char > >& gv)
            {
                if (gv.IsObject()) {
                    targetPos.x = gv["targetPos"][0].GetInt();
                    targetPos.y = gv["targetPos"][1].GetInt();

                    targetTplId = gv["targetTplId"].GetInt();
                    cityId = gv["cityId"].GetInt();
                    cityName = gv["cityName"].GetString();

                    allianceName = gv["allianceName"].GetString();
                    allianceNickname = gv["allianceNickname"].GetString();
                    allianceBannerId = gv["bannerId"].GetInt();

                    troopCount = gv["troopCount"].GetInt();
                    armyTotalCount = gv["armyTotalCount"].GetInt();
                }
            }

            void MsgCityPatrolEvent::SetDataTable(DataTable& table) const
            {
                DataTable dtDrops, dtRemoves;
                table.Set("id",  id);
                SetDropsTable(dtDrops, drops);
                table.Set("drops", dtDrops);

                SetDropsTable(dtRemoves, removes);
                table.Set("removes", dtRemoves);

                if (armyList.size() > 0) {
                    DataTable armyListTable;
                    armyList.SetDataTable(armyListTable, 2);
                    table.Set("armyList", armyListTable);

//                     int dieCount = armyList.ArmyCount(ArmyState::DIE);
//                     table.Set("dieCount", dieCount);
                }
            }

            /*======  MsgCityPatrol  ======*/

            std::string MsgCityPatrol::Serialize()
            {
                string jsonString;
                try {
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartObject();

                    writer.Key("cityInfo");
                    m_cityInfo.Serialize(writer);

                    writer.Key("events");
                    writer.StartArray();
                    for (auto &event :  m_events) {
                        writer.StartObject();

                        writer.Key("id");
                        writer.Int(event.id);

                        writer.Key("drops");
                        SerializeDrops(writer, event.drops);

                        writer.Key("removes");
                        SerializeDrops(writer, event.removes);

                        writer.Key("armyList");
                        event.armyList.Serialize(writer);

                        writer.EndObject();
                    }
                    writer.EndArray();

                    writer.EndObject();

                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgCityPatrol json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgCityPatrol::Deserialize(const string& data)
            {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());

                    m_cityInfo.Deserialize(doc["cityInfo"]);

                    auto& events = doc["events"];
                    for (size_t i = 0; i < events.Size(); ++i) {
                        MsgCityPatrolEvent msgEvent;
                        msgEvent.id = events[i]["id"].GetInt();

                        auto& drops = events[i]["drops"];
                        DeserializeDrops(drops, msgEvent.drops);

                        auto& removes = events[i]["removes"];
                        DeserializeDrops(removes, msgEvent.removes);

                        auto& armyList = events[i]["armyList"];
                        msgEvent.armyList.Desrialize(armyList);

                        m_events.push_back(msgEvent);
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgCityPatrol fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgCityPatrol::Send(Agent* agent)
            {
                MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, dtEvents, dtCityInfo;

                for (int i = 0; i < (int)m_events.size(); ++i) {
                    DataTable dtEvent;
                    m_events[i].SetDataTable(dtEvent);
                    dtEvents.Set(i+1, dtEvent);
                }

                msg.Set("events", dtEvents);

                m_cityInfo.SetDataTable(dtCityInfo);
                msg.Set("cityInfo", dtCityInfo);

                msg.Set("timestamp", m_timestamp);

                msgout << 3;    //argc
                lua_write(msgout, "onCityPatrol");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgCityPatrol::SetData(const MsgCityInfo& cityInfo, const std::vector<MsgCityPatrolEvent>& events, int64_t timestamp)
            {
                m_cityInfo = cityInfo;

                for (auto &event :  events) {
                    MsgCityPatrolEvent msgEvent;
                    msgEvent.id = event.id;
                    auto& drops = event.drops;
                    std::copy(drops.begin(), drops.end(), std::back_inserter(msgEvent.drops));

                    auto& removes = event.removes;
                    std::copy(removes.begin(), removes.end(), std::back_inserter(msgEvent.removes));

                    msgEvent.armyList = event.armyList;

                    m_events.emplace_back(msgEvent);
                }

                m_timestamp = timestamp;
            }


            /*======  MsgAttackWorldBoss  ======*/
            string MsgAttackWorldBoss::Serialize()
            {
                string jsonString;
                try {
                    // [ winner, myAttackType, attacker, defender, timestamp, unitId, troopId ]
                    // attacker, defender = [attackType, uid, nickname, allianceName, headId, [x, y], turretLevel, turretKills, [[uid, nickname, [<tplid, <state, count>>]]]
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();
                    writer.Int((int)m_winner);
                    writer.Int((int)m_myAttackType);
                    //attacker
                    m_attacker.Serialize(writer);

                    //defender
                    m_defender.Serialize(writer);

                    writer.Int64(m_timestamp);
                    writer.Int64(m_unitId);
                    writer.Int64(m_troopId);
                    writer.Int64(m_reportId);
                    // reportInfo
                    m_reportInfo.Serialize(writer);

                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgAttackWorldBoss json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgAttackWorldBoss::Deserialize(const string& data)
            {
                try {
                    // [ winner, myAttackType, attacker, defender, timestamp, unitId, troopId ]
                    // attacker, defender = [attackType, uid, nickname, allianceName, headId, [x, y], turretLevel, turretKills, [[uid, nickname, [<tplid, <state, count>>]]]
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray() && doc.Size() == 9) {
                        m_winner = static_cast<model::AttackType>(doc[0u].GetInt());
                        m_myAttackType = static_cast<model::AttackType>(doc[1].GetInt());
                        //attacker
                        m_attacker.Deserialize(doc[2]);
                        //defender
                        m_defender.Deserialize(doc[3]);

                        m_timestamp = doc[4].GetInt64();
                        m_unitId = doc[5].GetInt();
                        m_troopId = doc[6].GetInt();
                        m_reportId = doc[7].GetInt();
                        m_reportInfo.Deserialize(doc[8]);
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgAttackWorldBoss fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgAttackWorldBoss::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, attacker, defender, reportInfo;
                msg.Set("winner", (int)m_winner);
                msg.Set("battleType", (int)BattleType::WORLDBOSS);
                msg.Set("myAttackType", (int)m_myAttackType);
                //attacker
                m_attacker.SetDataTable(attacker);
                m_defender.SetDataTable(defender);

                msg.Set("attacker", attacker);
                msg.Set("defender", defender);
                msg.Set("timestamp", m_timestamp);

                m_reportInfo.SetDataTable(reportInfo);

                msg.Set("reportInfo", reportInfo);

                msgout << 6;    //argc
                lua_write(msgout, "onAttackWorldBoss");
                lua_write(msgout, m_id);
                lua_write(msgout, m_unitId);
                lua_write(msgout, m_troopId);
                lua_write(msgout, msg);
                lua_write(msgout, m_reportId);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgAttackWorldBoss::SetData(AttackType winner, AttackType myAttackType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int reportId, int64_t timestamp, int unitId, int troopId, const MsgReportInfo& reportInfo)
            {
                m_winner = winner;
                m_myAttackType = myAttackType;
                m_attacker = attacker;
                m_defender = defender;
                m_timestamp = timestamp;
                m_unitId = unitId;
                m_troopId = troopId;
                m_reportId = reportId;
                m_reportInfo = reportInfo;
            }


            /*======  MsgAttackWorldBossEnd  ======*/
            string MsgAttackWorldBossEnd::Serialize()
            {
                string jsonString;
                try {
                    // [ winner, myAttackType, attacker, defender, timestamp, unitId, troopId ]
                    // attacker, defender = [attackType, uid, nickname, allianceName, headId, [x, y], turretLevel, turretKills, [[uid, nickname, [<tplid, <state, count>>]]]
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();
                    //attacker
                    m_attacker.Serialize(writer);

                    //defender
                    m_defender.Serialize(writer);

                    //worldBoss
                    writer.StartArray();
                    writer.Int(m_worldboss.tplId);
                    writer.Int(m_worldboss.beginArmyCount);
                    writer.Int(m_worldboss.endArmyCount);
                    writer.Int(m_worldboss.maxArmyCount);
                    writer.EndArray();

                    writer.Int64(m_timestamp);
                    writer.Int64(m_unitId);
                    writer.Int64(m_troopId);

                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgAttackWorldBossEnd json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgAttackWorldBossEnd::Deserialize(const string& data)
            {
                try {
                    // [ winner, myAttackType, attacker, defender, timestamp, unitId, troopId ]
                    // attacker, defender = [attackType, uid, nickname, allianceName, headId, [x, y], turretLevel, turretKills, [[uid, nickname, [<tplid, <state, count>>]]]
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray() && doc.Size() == 6) {
                        //attacker
                        m_attacker.Deserialize(doc[0u]);
                        //defender
                        m_defender.Deserialize(doc[1]);
                        //worldBoss
                        auto& doc2 = doc[2];
                        if (doc2.IsArray()) {
                            m_worldboss.tplId = doc2[0u].GetInt();
                            m_worldboss.beginArmyCount = doc2[1].GetInt();
                            m_worldboss.endArmyCount = doc2[2].GetInt();
                            m_worldboss.maxArmyCount = doc2[3].GetInt();
                        }
                        m_timestamp = doc[3].GetInt64();
                        m_unitId = doc[4].GetInt();
                        m_troopId = doc[5].GetInt();
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgAttackWorldBossEnd fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgAttackWorldBossEnd::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, attacker, defender, wolrdboss;
                msg.Set("battleType", (int)BattleType::WORLDBOSS);
                //attacker
                m_attacker.SetDataTable(attacker);
                m_defender.SetDataTable(defender);
                m_worldboss.SetDataTable(wolrdboss);
                msg.Set("attacker", attacker);
                msg.Set("defender", defender);
                msg.Set("wolrdboss", wolrdboss);
                msg.Set("timestamp", m_timestamp);
                msgout << 5;    //argc
                lua_write(msgout, "onAttackWorldBossEnd");
                lua_write(msgout, m_id);
                lua_write(msgout, m_unitId);
                lua_write(msgout, m_troopId);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgAttackWorldBossEnd::SetData(const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, const MsgWorldBossInfo& worldBossInfo, const int64_t timestamp, int unitId, int troopId)
            {
                m_attacker = attacker;
                m_defender = defender;
                m_worldboss = worldBossInfo;
                m_timestamp = timestamp;
                m_unitId = unitId;
                m_troopId = troopId;
            }

            /*======  MsgAttackCityEnd  ======*/
            string MsgAttackCityEnd::Serialize()
            {
                string jsonString;
                try {
                    // [ winner, myAttackType, attacker, defender, timestamp, unitId, troopId ]
                    // attacker, defender = [attackType, uid, nickname, allianceName, headId, [x, y], turretLevel, turretKills, [[uid, nickname, [<tplid, <state, count>>]]]
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();
                    //attacker
                    m_attacker.Serialize(writer);

                    //defender
                    m_defender.Serialize(writer);

                    //worldBoss
                    writer.StartArray();
                    writer.Int(m_worldboss.tplId);
                    writer.Int(m_worldboss.beginArmyCount);
                    writer.Int(m_worldboss.endArmyCount);
                    writer.Int(m_worldboss.maxArmyCount);
                    writer.EndArray();

                    writer.Int64(m_timestamp);
                    writer.Int64(m_unitId);
                    writer.Int64(m_troopId);

                    writer.Key("drops");
                    SerializeDrops(writer,  m_dropItems);

                    writer.Key("armyList");
                    m_armyList.Serialize(writer);

                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgAttackCityEnd json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgAttackCityEnd::Deserialize(const string& data)
            {
                try {
                    // [ winner, myAttackType, attacker, defender, timestamp, unitId, troopId ]
                    // attacker, defender = [attackType, uid, nickname, allianceName, headId, [x, y], turretLevel, turretKills, [[uid, nickname, [<tplid, <state, count>>]]]
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray() && doc.Size() == 7) {
                        //attacker
                        m_attacker.Deserialize(doc[0u]);
                        //defender
                        m_defender.Deserialize(doc[1]);
                        //worldBoss
                        auto& doc2 = doc[2];
                        if (doc2.IsArray()) {
                            m_worldboss.tplId = doc2[0u].GetInt();
                            m_worldboss.beginArmyCount = doc2[1].GetInt();
                            m_worldboss.endArmyCount = doc2[2].GetInt();
                            m_worldboss.maxArmyCount = doc2[3].GetInt();
                        }
                        m_timestamp = doc[3].GetInt64();
                        m_unitId = doc[4].GetInt();
                        m_troopId = doc[5].GetInt();
                        if (doc[6].IsArray()) {
                            DeserializeDrops(doc["drops"],  m_dropItems);
                        }
                        m_armyList.Desrialize(doc["armyList"]);
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgAttackCityEnd fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgAttackCityEnd::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, attacker, defender, wolrdboss, armylist, drops;
                msg.Set("battleType", (int)BattleType::WORLDBOSS);
                //attacker
                m_attacker.SetDataTable(attacker);
                m_defender.SetDataTable(defender);
                m_worldboss.SetDataTable(wolrdboss);
                m_armyList.SetDataTable(armylist);
                SetDropsTable(drops, m_dropItems);

                msg.Set("attacker", attacker);
                msg.Set("defender", defender);
                msg.Set("wolrdboss", wolrdboss);
                msg.Set("armylist", armylist);
                msg.Set("drops", drops);
                msg.Set("timestamp", m_timestamp);
                msgout << 5;    //argc
                lua_write(msgout, "onAttackCityEnd");
                lua_write(msgout, m_id);
                lua_write(msgout, m_unitId);
                lua_write(msgout, m_troopId);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgAttackCityEnd::SetData(const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, const MsgWorldBossInfo& worldBossInfo, const std::vector<model::tpl::DropItem>& dropItems,  const ArmyList& armyList, const int64_t timestamp, int unitId, int troopId)
            {
                m_attacker = attacker;
                m_defender = defender;
                m_worldboss = worldBossInfo;
                m_timestamp = timestamp;
                m_unitId = unitId;
                m_troopId = troopId;
                std::copy(dropItems.begin(), dropItems.end(), std::back_inserter(m_dropItems));
                m_armyList = armyList;
            }


            /*======  MsgAttackCatapultEnd  ======*/
            string MsgAttackCatapultEnd::Serialize()
            {
                string jsonString;
                try {
                    // [ winner, myAttackType, attacker, defender, timestamp, unitId, troopId ]
                    // attacker, defender = [attackType, uid, nickname, allianceName, headId, [x, y], turretLevel, turretKills, [[uid, nickname, [<tplid, <state, count>>]]]
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();
                    //attacker
                    m_attacker.Serialize(writer);

                    //defender
                    m_defender.Serialize(writer);

                    //worldBoss
                    writer.StartArray();
                    writer.Int(m_worldboss.tplId);
                    writer.Int(m_worldboss.beginArmyCount);
                    writer.Int(m_worldboss.endArmyCount);
                    writer.Int(m_worldboss.maxArmyCount);
                    writer.EndArray();

                    writer.Int64(m_timestamp);
                    writer.Int64(m_unitId);
                    writer.Int64(m_troopId);

                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgAttackCatapultEnd json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgAttackCatapultEnd::Deserialize(const string& data)
            {
                try {
                    // [ winner, myAttackType, attacker, defender, timestamp, unitId, troopId ]
                    // attacker, defender = [attackType, uid, nickname, allianceName, headId, [x, y], turretLevel, turretKills, [[uid, nickname, [<tplid, <state, count>>]]]
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray() && doc.Size() == 6) {
                        //attacker
                        m_attacker.Deserialize(doc[0u]);
                        //defender
                        m_defender.Deserialize(doc[1]);
                        //worldBoss
                        auto& doc2 = doc[2];
                        if (doc2.IsArray()) {
                            m_worldboss.tplId = doc2[0u].GetInt();
                            m_worldboss.beginArmyCount = doc2[1].GetInt();
                            m_worldboss.endArmyCount = doc2[2].GetInt();
                            m_worldboss.maxArmyCount = doc2[3].GetInt();
                        }
                        m_timestamp = doc[3].GetInt64();
                        m_unitId = doc[4].GetInt();
                        m_troopId = doc[5].GetInt();
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgAttackCatapultEnd fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgAttackCatapultEnd::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, attacker, defender, wolrdboss;
                msg.Set("battleType", (int)BattleType::WORLDBOSS);
                //attacker
                m_attacker.SetDataTable(attacker);
                m_defender.SetDataTable(defender);
                m_worldboss.SetDataTable(wolrdboss);
                msg.Set("attacker", attacker);
                msg.Set("defender", defender);
                msg.Set("wolrdboss", wolrdboss);
                msg.Set("timestamp", m_timestamp);
                msgout << 5;    //argc
                lua_write(msgout, "onAttackCatapultEnd");
                lua_write(msgout, m_id);
                lua_write(msgout, m_unitId);
                lua_write(msgout, m_troopId);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgAttackCatapultEnd::SetData(const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, const MsgWorldBossInfo& worldBossInfo, const int64_t timestamp, int unitId, int troopId)
            {
                m_attacker = attacker;
                m_defender = defender;
                m_worldboss = worldBossInfo;
                m_timestamp = timestamp;
                m_unitId = unitId;
                m_troopId = troopId;
            }


            /*======  MsgTransport  ======*/
            string MsgTransport::Serialize()
            {
                string jsonString;
                try {
                    // [ winner, myAttackType, attacker, defender, timestamp, unitId, troopId ]
                    // attacker, defender = [attackType, uid, nickname, allianceName, headId, [x, y], turretLevel, turretKills, [[uid, nickname, [<tplid, <state, count>>]]]
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();
                    //attacker
                    m_attacker.Serialize(writer);

                    //defender
                    m_defender.Serialize(writer);
                    //resouce
                    writer.Int64(m_food);
                    writer.Int64(m_wood);
                    writer.Int64(m_iron);
                    writer.Int64(m_stone);

                    writer.Bool(m_isSuccess);
                    writer.Int64(m_timestamp);

                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgTransport json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgTransport::Deserialize(const string& data)
            {
                try {
                    // [ winner, myAttackType, attacker, defender, timestamp, unitId, troopId ]
                    // attacker, defender = [attackType, uid, nickname, allianceName, headId, [x, y], turretLevel, turretKills, [[uid, nickname, [<tplid, <state, count>>]]]
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray() && doc.Size() == 9) {
                        //attacker
                        m_attacker.Deserialize(doc[0u]);
                        //defender
                        m_defender.Deserialize(doc[1]);
                        
                        m_myTransportType = static_cast<model::TransportType>(doc[2].GetInt64());
                        m_food = doc[3].GetInt64();
                        m_wood = doc[4].GetInt64();
                        m_iron = doc[5].GetInt64();
                        m_stone = doc[6].GetInt64();

                        m_isSuccess = doc[7].GetBool();
                        m_timestamp = doc[8].GetInt64();
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgTransport fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgTransport::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, attacker, defender, wolrdboss;
                //attacker
                m_attacker.SetDataTable(attacker);
                m_defender.SetDataTable(defender);
                msg.Set("myTransportType", (int)m_myTransportType);
                msg.Set("attacker", attacker);
                msg.Set("defender", defender);
                msg.Set("food", m_food);
                msg.Set("wood", m_wood);
                msg.Set("iron", m_iron);
                msg.Set("stone", m_stone);
                msg.Set("isSuccess", m_isSuccess);
                msg.Set("timestamp", m_timestamp);

                msgout << 3;    //argc
                lua_write(msgout, "onTransport");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgTransport::SetData(model::TransportType myTransportType, const MsgPvpPlayerInfo& attacker, const MsgPvpPlayerInfo& defender, int food, int wood, int iron, int stone, bool isSuccess, const int64_t timestamp)
            {
                m_attacker = attacker;
                m_defender = defender;
                m_food = food;
                m_wood = wood;
                m_iron = iron;
                m_stone = stone;
                m_isSuccess = isSuccess;
                m_timestamp = timestamp;
                m_myTransportType = myTransportType;
            }

            /*======  MsgCompensate  ======*/
             string MsgCompensate::Serialize()
            {
                string jsonString;
                try {
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartArray();

                    //drops
                    SerializeDrops(writer, m_drops);
                    writer.Int64(m_timestamp);

                    writer.EndArray();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgCompensate json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgCompensate::Deserialize(const string& data)
            {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    if (doc.IsArray() && doc.Size() == 2) {
                        DeserializeDrops(doc[1], m_drops);
                        m_timestamp = doc[2].GetInt64();
                    }
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgCompensate fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgCompensate::Send(Agent* agent)
            {
                ms::map::msgqueue::MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg, drops;
                SetDropsTable(drops, m_drops);
                msg.Set("drops", drops);
                msg.Set("timestamp", m_timestamp);

                msgout << 3;    //argc
                lua_write(msgout, "onCompensate");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgCompensate::SetData(int food, int wood, int stone, int iron, const int64_t timestamp)
            {
                PushResourceToDrops(m_drops, food, wood, iron, stone, 0);
                m_timestamp = timestamp;
            }


            /*======  MsgCityDefenerFill  ======*/

            std::string MsgCityDefenerFill::Serialize()
            {
                string jsonString;
                try {
                    rapidjson::StringBuffer jsonbuffer;
                    rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                    writer.StartObject();
                    writer.Key("armyList");
                    m_armyList.Serialize(writer);
                    writer.Key("timestemp");
                    writer.Int64(m_timestemp);
                    writer.EndObject();
                    jsonString = jsonbuffer.GetString();
                } catch (exception& ex) {
                    LOG_ERROR("save MsgCityDefenerFill json fail: %s\n", ex.what());
                }
                return jsonString;
            }

            bool MsgCityDefenerFill::Deserialize(const string& data)
            {
                try {
                    rapidjson::Document doc;
                    doc.Parse<0>(data.c_str());
                    m_armyList.Desrialize(doc["armyList"]);
                    m_timestemp = doc["timestemp"].GetInt64();
                    return true;
                } catch (exception& ex) {
                    LOG_ERROR("json string to MsgCityDefenerFill fail: %s\n", ex.what());
                    return false;
                }
            }

            void MsgCityDefenerFill::Send(Agent* agent)
            {
                //cout << "MsgMarch::Send" << endl;
                MsgRecord::Send(agent);

                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::EVENT, 64, g_mapMgr->mempool());
                DataTable msg,armyList;

                m_armyList.SetDataTable(armyList);
                msg.Set("armyList", armyList);
                msg.Set("timestemp", m_timestemp);

                msgout << 3;    //argc
                lua_write(msgout, "onCityDefenerFill");
                lua_write(msgout, m_id);
                lua_write(msgout, msg);
                agent->proxy()->SendMessage(msgout);
            }

            void MsgCityDefenerFill::SetData(const ArmyList* armyList,const int64_t timestemp)
            {
                if (armyList) {
                    m_armyList = *armyList;
                }
                 m_timestemp = timestemp;
            }
        }
    }
}
