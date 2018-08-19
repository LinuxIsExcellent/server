#include "dataservice.h"
#include <base/dbo/connection.h>
#include <base/logger.h>
#include "../agent.h"
#include "../unit/unit.h"
#include "../alliance.h"
#include "../unit/castle.h"

namespace ms
{
    namespace map
    {
        namespace qo
        {
            using namespace std;
            using namespace base::dbo;
            using namespace info;

            DataService* g_dataService = nullptr;

            DataService* DataService::Create()
            {
                if (g_dataService == nullptr) {
                    g_dataService = new DataService();
                    g_dataService->Setup();
                }
                return g_dataService;
            }

            void DataService::Destroy()
            {
                SAFE_DELETE(g_dataService);
            }

            DataService::DataService()
            {
            }

            DataService::~DataService()
            {
                for (auto& it: m_dataQueue) {
                    std::queue<const DbInfo*>& queue = it.second;
                    while (!queue.empty()) {
                        const DbInfo* info = queue.front();
                        SAFE_DELETE(info);
                        queue.pop();
                    }
                }
            }

            bool DataService::Setup()
            {
                m_dataCount = 0u;

                int end = (int)DataClassify::END;
                for (int i = 1; i < end; ++i) {
                    m_dataQueue.emplace(i, std::queue<const DbInfo*>());
                }

                return true;
            }

            DbInfo* DataService::CreateDbInfo(msgqueue::MsgRecord* msg)
            {
                DbMsgQueueInfo* info = new DbMsgQueueInfo;
                info->uid = msg->uid();
                info->mid = msg->id();
                info->type = msg->type();
                info->data = msg->Serialize();
                info->create_time = msg->createTime();
                return info;
            }

            DbInfo* DataService::CreateDbInfo(const Agent* agent)
            {
                DbAgentInfo* info = new DbAgentInfo;
                info->uid = agent->uid();
                info->data = agent->Serialize();
                return info;
            }

            DbInfo* DataService::CreateDbInfo(Unit* unit)
            {
                DbUnitInfo* info = new DbUnitInfo;
                info->id = unit->id();
                info->tplid = unit->tpl().id;
                info->x = unit->x();
                info->y = unit->y();
                if(Castle* castle = unit->ToCastle()) {
                    info->uid = castle->uid();
                }
                info->data = unit->Serialize();
                 return info;
            }

            DbInfo* DataService::CreateDbInfo(const Troop* troop)
            {
                DbTroopInfo* info = new DbTroopInfo;
                info->id = troop->id();
                info->uid = troop->uid();
                info->data = troop->Serialize();
                info->is_delete = troop->IsRemove();
                info->create_time = troop->createTime();
                return info;
            }

            DbInfo* DataService::CreateDbInfo(const BattleInfo* btInfo)
            {
                DbFightHistoryInfo* info = new DbFightHistoryInfo;
                info->id = btInfo->id;
                info->redUid = btInfo->red.uid;
                info->redNickname = btInfo->red.nickname;
                info->redAllianceId = btInfo->red.allianceId;
                info->redAllianceNickname = btInfo->red.allianceNickname;
                info->blueUid = btInfo->blue.uid;
                info->blueNickname = btInfo->blue.nickname;
                info->blueAllianceId = btInfo->blue.allianceId;
                info->blueAllianceNickname = btInfo->blue.allianceNickname;
                info->isRedDelete = btInfo->isRedDelete;
                info->isBlueDelete = btInfo->isBlueDelete;
                info->winTeam = btInfo->winTeam;
                info->battleType = btInfo->battleType;
                info->battleTime = btInfo->battleTime;
                return info;
            }

//             DbInfo* DataService::CreateDbInfo(const MonsterSiegeRecord* record)
//             {
//                 DbMonsterSiegeInfo* info = new DbMonsterSiegeInfo;
//                 info->uid = record->uid;
//                 info->level = record->level;
//                 info->food = record->food;
//                 info->wood = record->wood;
//                 info->iron = record->iron;
//                 info->stone = record->stone;
//                 info->timestamp = record->timestamp;
//                 return info;
//             }

            void DataService::Save()
            {
                if (m_dataCount == 0u) {
                    m_saving = false;
                    return;
                }
                SaveInternal();
            }

            void DataService::SaveInternal()
            {
                //std::cout << "dataCount = " << m_dataCount << std::endl;
                if (m_dataCount == 0u) {
                    m_saving = false;
                    return;
                }
                m_saving = true;

                DataClassify classify = DataClassify::END;
                std::vector<const DbInfo*> toSaveList;

                // batch save
                for (auto& it: m_dataQueue) {
                    size_t max = 1;
                    std::queue<const DbInfo*>& queue = it.second;
                    if (queue.size() > 200) {
                        max = 200;
                    } else if (queue.size() > 50) {
                        max = 50;
                    }
                    if (queue.size() > 0 && m_closing) {
                        max = 200;
                    }
                    if (max > 1) {
                        classify = static_cast<DataClassify>(it.first);
                        do {
                            --m_dataCount;
                            toSaveList.push_back(queue.front());
                            queue.pop();
                        } while (toSaveList.size() < max && queue.size() > 0);
                        break;
                    }
                }
                // single save
                if (toSaveList.empty()) {
                    for (auto& it: m_dataQueue) {
                        std::queue<const DbInfo*>& queue = it.second;
                        if (queue.size() > 0) {
                            classify = static_cast<DataClassify>(it.first);
                            --m_dataCount;
                            toSaveList.push_back(queue.front());
                            queue.pop();
                            break;
                        }
                    }
                }

                // do saving
                if (toSaveList.size() > 0) {
                    std::string table = "save ";
                    switch (classify) {
                        case DataClassify::AGENT: {
                            table.append("agent");
                            SaveAgentList(toSaveList);
                        }
                        break;
                        case DataClassify::UNIT: {
                            table.append("unit");
                            SaveUnitList(toSaveList);
                        }
                        break;
                        case DataClassify::TROOP: {
                            table.append("troop");
                            SaveTroopList(toSaveList);
                        }
                        break;
                        case DataClassify::MSGQUEUE: {
                            table.append("msgqueue");
                            SaveMsgQueueList(toSaveList);
                        }
                        break;
                        case DataClassify::FIGHT_HISTORY: {
                            table.append("fightHistory");
                            SaveFightHistoryList(toSaveList);
                        }
                        break;
                        case DataClassify::MONSTER_SIEGE: {
                            table.append("monsterSiege");
                            SaveMonsterSiegeList(toSaveList);
                        }
                        break;
                        default:
                            LOG_ERROR("wrong data classify=%d\n", (int)classify);
                            break;
                    };

                    for (const DbInfo* v : toSaveList) {
                        SAFE_DELETE(v);
                    }

                    if (toSaveList.size() > 1u) {
                        table.append(" size=");
                        //cout << table.c_str() << toSaveList.size() << ", dataCount=" << m_dataCount << endl;
                    }
                }
            }

            void DataService::OnSaveInternalFinished()
            {
                m_saving = m_dataCount > 0u;
                if (m_closing) {
                    printf("DataService::OnSaveInternalFinished dataCount=%ld when closing\n", m_dataCount);
                    if (m_saving) {
                        SaveInternal();
                    } else {
                        m_cleanupCb();
                    }
                } else if (m_saving) {
                    Save();
                }
            }

            void DataService::AppendData(const DbInfo* dataRow)
            {
                if (!dataRow) {
                    return;
                }
                int classify = (int)dataRow->classify();
                auto it = m_dataQueue.find(classify);
                if (it != m_dataQueue.end()) {
                    it->second.push(dataRow);
                    ++m_dataCount;

                    if (!m_saving) {
                        m_saving = true;
                        Save();
                    }
                }
            }

            void DataService::AppendData(DataClassify classify, const std::vector< const DbInfo* >& dataRows)
            {
                if (dataRows.empty()) {
                    return;
                }

                auto it = m_dataQueue.find((int)classify);
                if (it != m_dataQueue.end()) {
                    for (const DbInfo* row : dataRows) {
                        if (row->classify() == classify) {
                            it->second.push(row);
                            ++m_dataCount;
                        } else {
                            LOG_ERROR("DataService::AppendData wrong classify.\n");
                        }
                    }

                    if (!m_saving) {
                        m_saving = true;
                        Save();
                    }
                }
            }

            void DataService::AppendAgentData()
            {
                vector<const DbInfo*> infos;
                for (auto it = g_mapMgr->agents().begin(); it != g_mapMgr->agents().end(); ++it) {
                    Agent* agent = it->second;
                    if (agent && agent->IsDirty()) {
                        agent->SetClean();
                        infos.push_back(CreateDbInfo(agent));
                    }
                }

                AppendData(DataClassify::AGENT, infos);
            }

            void DataService::AppendUnitData()
            {
                vector<const DbInfo*> infos;
                g_mapMgr->UnitsForeach([&](Unit* unit){
                    if( unit->IsDirty() && (
                        unit->IsResNode() || unit->IsFamousCity() || unit->IsCapital() || unit->IsCastle() ||  unit->IsCampFixed() ||  
                        unit->IsTree() || unit->IsCatapult() || unit->IsMonster() || unit->IsMonster() || unit->IsCatapult() ) )
                    {
                        unit->SetClean();
                        infos.push_back(CreateDbInfo(unit));
                    }
                });

                AppendData(DataClassify::UNIT, infos);
            }

            void DataService::AppendTroopData()
            {
                vector<const DbInfo*> infos;
                for (auto it = g_mapMgr->m_troops.begin(); it != g_mapMgr->m_troops.end(); ++it) {
                    if (Troop* troop = it->second) {
                        if (troop->IsDirty() && troop->uid() > 0) {
                            troop->SetClean();
                            infos.push_back(CreateDbInfo(troop));
                        }
                    }
                }
                std::vector<Troop*> removeList;
                int64_t dt = g_dispatcher->GetTimestampCache() - 3600 * 6;
                for (auto it = g_mapMgr->m_troopsRemoved.begin(); it != g_mapMgr->m_troopsRemoved.end(); ++it) {
                    if (Troop* troop = it->second) {
                        if (troop->IsDirty() && troop->uid() > 0) {
                            troop->SetClean();
                            infos.push_back(CreateDbInfo(troop));
                        }
                        if (troop->removeTime() != 0 && troop->removeTime() < dt) {
                            removeList.push_back(troop);
                        }
                    }
                }
                for (size_t i = 0; i < removeList.size(); ++i) {
                    g_mapMgr->m_troopsRemoved.erase(removeList[i]->id());
                    SAFE_DELETE(removeList[i]);
                }

                AppendData(DataClassify::TROOP, infos);
            }

            void DataService::AppendFightHistoryData()
            {
                vector<const DbInfo*> infos;
                for (auto it = g_mapMgr->m_battleInfoList.begin(); it != g_mapMgr->m_battleInfoList.end(); ++it) {
                    BattleInfo* info = it->second;
                    if (info->isDirty) {
                        info->isDirty = false;
                        infos.push_back(CreateDbInfo(info));
                    }
                }

                AppendData(DataClassify::FIGHT_HISTORY, infos);
            }

//             void DataService::AppendMonsterSiegeData()
//             {
//                 vector<const DbInfo*> infos;
//                 for (auto it = g_mapMgr->monsterSiege().m_records.begin(); it != g_mapMgr->monsterSiege().m_records.end(); ++it) {
//                     MonsterSiegeRecord* record = &it->second;
//                     if (record->isDirty) {
//                         record->isDirty = false;
//                         infos.push_back(CreateDbInfo(record));
//                     }
//                 }
//                 AppendData(DataClassify::MONSTER_SIEGE, infos);
//             }
//
            void DataService::AppendMsgQueueData(const msgqueue::MsgQueue& msgQueue)
            {
                vector<const DbInfo*> infos;
                for (auto it = msgQueue.m_sent_records.begin(); it != msgQueue.m_sent_records.end(); ++it) {
                    if (msgqueue::MsgRecord* msg = *it) {
                        DbInfo* info = CreateDbInfo(msg);
                        infos.push_back(info);
                    }
                }
                for (auto it = msgQueue.m_waiting_records.begin(); it != msgQueue.m_waiting_records.end(); ++it) {
                    if (msgqueue::MsgRecord* msg = *it) {
                        DbInfo* info = CreateDbInfo(msg);
                        infos.push_back(info);
                    }
                }

                AppendData(DataClassify::MSGQUEUE, infos);
            }

            void DataService::SaveAgentList(const std::vector< const DbInfo* >& toSaveList)
            {
                string sql = "insert into s_agent (uid, data) values ";
                for (size_t i = 0; i < toSaveList.size(); ++i) {
                    if (i == 0) {
                        sql.append("(?,?)");
                    } else {
                        sql.append(",(?,?)");
                    }
                }
                sql.append(" on duplicate key update data=values(data)");

                PreparedStatement* pstmt = PreparedStatement::Create(2, sql.c_str());
                for (const DbInfo* v : toSaveList) {
                    const DbAgentInfo* info = static_cast<const DbAgentInfo*>(v);
                    pstmt->SetInt64(info->uid);
                    pstmt->SetString(info->data);
                }

                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        LOG_ERROR("DataService::SaveAgentList %s\n", rs.error_message());
                    }
                    OnSaveInternalFinished();
                }, m_autoObserver);
            }

            void DataService::SaveUnitList(const std::vector< const DbInfo* >& toSaveList)
            {
                string sql = "insert into s_unit (id, tplid, x, y, uid, data) values ";
                for (size_t i = 0; i < toSaveList.size(); ++i) {
                    if (i == 0) {
                        sql.append("(?, ?, ?, ?, ?, ?)");
                    } else {
                        sql.append(",(?, ?, ?, ?, ?, ?)");
                    }
                }
                sql.append(" on duplicate key update tplid=values(tplid), x=values(x), y=values(y), uid=values(uid), data=values(data)");

                // LOG_DEBUG("DataService::SaveUnitList...  %s\n", sql.c_str());

                PreparedStatement* pstmt = PreparedStatement::Create(2, sql.c_str());
                for (const DbInfo* v : toSaveList) {
                    const DbUnitInfo* info = static_cast<const DbUnitInfo*>(v);
                    pstmt->SetInt32(info->id);
                    pstmt->SetInt32(info->tplid);
                    pstmt->SetInt32(info->x);
                    pstmt->SetInt32(info->y);
                    pstmt->SetInt64(info->uid);
                    pstmt->SetString(info->data);
                }

                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        LOG_ERROR("DataService::SaveUnitList %s\n", rs.error_message());
                    }
                    OnSaveInternalFinished();
                }, m_autoObserver);
            }

            void DataService::SaveTroopList(const std::vector< const DbInfo* >& toSaveList)
            {
                string sql = "insert into s_troop (id, uid, data, is_delete, create_time) values ";
                for (size_t i = 0; i < toSaveList.size(); ++i) {
                    if (i == 0) {
                        sql.append("(?, ?, ?, ?, ?)");
                    } else {
                        sql.append(",(?, ?, ?, ?, ?)");
                    }
                }
                sql.append(" on duplicate key update data=values(data), uid=values(uid), is_delete=values(is_delete)");

                PreparedStatement* pstmt = PreparedStatement::Create(2, sql.c_str());
                for (const DbInfo* v : toSaveList) {
                    const DbTroopInfo* info = static_cast<const DbTroopInfo*>(v);
                    pstmt->SetInt32(info->id);
                    pstmt->SetInt64(info->uid);
                    pstmt->SetString(info->data);
                    pstmt->SetBoolean(info->is_delete);
                    pstmt->SetInt64(info->create_time);
                }

                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        LOG_ERROR("DataService::SaveTroopList %s\n", rs.error_message());
                    }
                    OnSaveInternalFinished();
                }, m_autoObserver);
            }

            void DataService::SaveMsgQueueList(const std::vector< const DbInfo* >& toSaveList)
            {
                string sql = "insert into s_msg_queue (uid, mid, type, data, create_time) values ";
                for (size_t i = 0; i < toSaveList.size(); ++i) {
                    if (i == 0) {
                        sql.append("(?, ?, ?, ?, ?)");
                    } else {
                        sql.append(",(?, ?, ?, ?, ?)");
                    }
                }
                sql.append(" on duplicate key update type=values(type), data=values(data)");

                PreparedStatement* pstmt = PreparedStatement::Create(2, sql.c_str());
                for (const DbInfo* v : toSaveList) {
                    const DbMsgQueueInfo* info = static_cast<const DbMsgQueueInfo*>(v);
                    pstmt->SetInt64(info->uid);
                    pstmt->SetInt32(info->mid);
                    pstmt->SetInt16((int16_t)info->type);
                    pstmt->SetString(info->data);
                    pstmt->SetInt64(info->create_time);
                }

                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        LOG_ERROR("DataService::SaveMsgQueueList %s\n", rs.error_message());
                    }
                    OnSaveInternalFinished();
                }, m_autoObserver);
            }

            void DataService::SaveFightHistoryList(const std::vector< const DbInfo* >& toSaveList)
            {
                string sql = "insert into s_battle_history (id, redUid, redNickname, redAllianceId, redAllianceNickname, blueUid, blueNickname, blueAllianceId, blueAllianceNickname, isRedDelete, isBlueDelete, winTeam, battleType, battleTime) values ";
                for (size_t i = 0; i < toSaveList.size(); ++i) {
                    if (i == 0) {
                        sql.append("(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
                    } else {
                        sql.append(",(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
                    }
                }
                sql.append(" on duplicate key update isRedDelete=values(isRedDelete), isBlueDelete=values(isBlueDelete)");

                PreparedStatement* pstmt = PreparedStatement::Create(2, sql.c_str());
                for (const DbInfo* v : toSaveList) {
                    const DbFightHistoryInfo* info = static_cast<const DbFightHistoryInfo*>(v);
                    pstmt->SetInt32(info->id);
                    pstmt->SetInt64(info->redUid);
                    pstmt->SetString(info->redNickname);
                    pstmt->SetInt64(info->redAllianceId);
                    pstmt->SetString(info->redAllianceNickname);
                    pstmt->SetInt64(info->blueUid);
                    pstmt->SetString(info->blueNickname);
                    pstmt->SetInt64(info->blueAllianceId);
                    pstmt->SetString(info->blueAllianceNickname);
                    pstmt->SetBoolean(info->isRedDelete);
                    pstmt->SetBoolean(info->isBlueDelete);
                    pstmt->SetInt32(info->winTeam);
                    pstmt->SetInt32(info->battleType);
                    pstmt->SetInt64(info->battleTime);
                }

                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        LOG_ERROR("DataService::SaveFightHistoryList %s\n", rs.error_message());
                    }
                    OnSaveInternalFinished();
                }, m_autoObserver);
            }

            void DataService::SaveMonsterSiegeList(const std::vector< const DbInfo* >& toSaveList)
            {
                string sql = "INSERT INTO s_monster_siege (uid, level, food, wood, iron, stone, timestamp) VALUES ";
                for (size_t i = 0; i < toSaveList.size(); ++i) {
                    if (i == 0) {
                        sql.append("(?, ?, ?, ?, ?, ?, ?)");
                    } else {
                        sql.append(", (?, ?, ?, ?, ?, ?, ?)");
                    }
                }
                sql.append(" ON DUPLICATE KEY UPDATE level=VALUES(level), food=VALUES(food), wood=VALUES(wood), iron=VALUES(iron), stone=VALUES(stone), timestamp=VALUES(timestamp)");

                PreparedStatement* pstmt = PreparedStatement::Create(2, sql.c_str());
                for (const DbInfo* v : toSaveList) {
                    const DbMonsterSiegeInfo* info = static_cast<const DbMonsterSiegeInfo*>(v);
                    pstmt->SetInt64(info->uid);
                    pstmt->SetInt32(info->level);
                    pstmt->SetInt32(info->food);
                    pstmt->SetInt32(info->wood);
                    pstmt->SetInt32(info->iron);
                    pstmt->SetInt32(info->stone);
                    pstmt->SetInt64(info->timestamp);
                }

                pstmt->Execute([this](ResultSet & rs) {
                    if (rs.HasError()) {
                        LOG_ERROR("DataService::SaveMonsterSiegeList %s\n", rs.error_message());
                    }
                    OnSaveInternalFinished();
                }, m_autoObserver);
            }

            void DataService::On5MinuteSave()
            {
                AppendAgentData();
                AppendUnitData();
                AppendTroopData();
                AppendFightHistoryData();
            }

            void DataService::OnCleanup(const std::function<void()>& cb)
            {
                cout << "DataService::OnCleanup" << endl;
                On5MinuteSave();
                // agent's msgQueue
                for (auto it = g_mapMgr->agents().begin(); it != g_mapMgr->agents().end(); ++it) {
                    Agent* agt = it->second;
                    if (!agt->msgQueue().Empty()) {
                        AppendMsgQueueData(agt->msgQueue());
                        LOG_DEBUG("DataService::OnCleanup uid=%ld, online=%d", agt->uid(), agt->mbid() ? 1 : 0);
                    }
                }

                m_cleanupCb = cb;
                m_closing = true;
                // it may happens that no data to save
                cout << "DataService::OnCleanup is saving = " << (m_saving ? "true" : "false") << endl;
                if (!m_saving) {
                    m_cleanupCb();
                }
            }

        }
    }
}
