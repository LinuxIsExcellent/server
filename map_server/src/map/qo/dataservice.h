#ifndef MAP_QO_DATASERVICE_H
#define MAP_QO_DATASERVICE_H

#include <functional>
#include <unordered_map>
#include <queue>
#include <vector>
#include "../mapMgr.h"
#include "../info/dbinfo.h"

namespace ms
{
    namespace map
    {

        class Agent;
        class Unit;
        struct BattleInfo;
        struct MonsterSiegeRecord;

        namespace msgqueue
        {
            class MsgQueue;
            class MsgRecord;
        }

        namespace qo
        {
            using namespace info;

            class DataService
            {
            public:
                static DataService* Create();
                static void Destroy();

                DbInfo* CreateDbInfo(msgqueue::MsgRecord* msg);
                DbInfo* CreateDbInfo(const Agent* agent);
                DbInfo* CreateDbInfo(Unit* unit);
                DbInfo* CreateDbInfo(const Troop* troop);
                DbInfo* CreateDbInfo(const BattleInfo* btInfo);
//                 DbInfo* CreateDbInfo(const MonsterSiegeRecord* record);

                void AppendData(const DbInfo* dataRow);
                void AppendData(DataClassify classify, const std::vector<const DbInfo*>& dataRows);
                void AppendMsgQueueData(const msgqueue::MsgQueue& msgQueue);

                void On5MinuteSave();
                void OnCleanup(const std::function<void()>& cb);

            private:
                DataService();
                virtual ~DataService();

                bool Setup();

                void Save();
                void SaveInternal();
                void OnSaveInternalFinished();

                void SaveAgentList(const std::vector<const DbInfo*>& toSaveList);
                void SaveUnitList(const std::vector<const DbInfo*>& toSaveList);
                void SaveTroopList(const std::vector<const DbInfo*>& toSaveList);
                void SaveMsgQueueList(const std::vector<const DbInfo*>& toSaveList);
                void SaveFightHistoryList(const std::vector<const DbInfo*>& toSaveList);
                void SaveMonsterSiegeList(const std::vector<const DbInfo*>& toSaveList);

                void AppendAgentData();
                void AppendUnitData();
                void AppendTroopData();
                void AppendFightHistoryData();

                bool m_saving = false;
                bool m_closing = false;
                size_t m_dataCount = 0u;
                std::unordered_map<int, std::queue<const DbInfo*> > m_dataQueue; // <DataClassify, quere>

                base::AutoObserver m_autoObserver;
                std::function<void()> m_cleanupCb;
            };

            extern DataService* g_dataService;
        }
    }
}

#endif // MAP_QO_DATASERVICE_H
