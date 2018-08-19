#ifndef MAP_MAP_PROXY_H
#define MAP_MAP_PROXY_H
#include <base/cluster/mailbox.h>
#include <base/command/runner.h>
#include <base/observer.h>
#include <base/bootable.h>
#include <lua/llimits.h>
#include "base/event.h"
#include <base/lua/luamessage.h>
#include <model/metadata.h>
#include "area.h"
#include <unordered_map>
#include <vector>
#include <queue>
#include <math.h>
#include "point.h"
#include <functional>
#include <map>
#include <tuple>
#include "troopevent.h"
#include "unitevent.h"
#include "interface.h"
#include "map.h"
#include "mapMgr.h"

namespace model
{
    namespace tpl
    {
        struct DropTpl;
    }
}

namespace base
{
    namespace lua
    {
        class LuaMessageWriter;
    }

    namespace cluster
    {
        struct NodeInfo;
    }
    class DataValue;
}

namespace engine
{
    struct WarReport;
}


namespace ms
{
    namespace map
    {
        class ActivityMgr;
        namespace info
        {
            class Kingdom;
        }

        namespace msgqueue
        {
            struct MsgPvpPlayerInfo;
        }

        namespace tpl
        {
            struct MapUnitTpl;
        }

        namespace qo
        {
            class CommandAgentLoad;
            class CommandAgentSave;
            class CommandAgentUpdate;
            class CommandUnitLoad;
            class CommandUnitSave;
            class CommandUnitUpdate;
            class CommandTroopLoad;
            class CommandTroopSave;
            class CommandTroopUpdate;
            class CommandFightHistoryLoad;
            class CommandFightHistorySave;
            class CommandFightHistoryUpdate;
            class CommandServerInfoLoad;
            class CommandPalaceWarLoad;
            class CommandTransportRecordLoad;
            class DataService;
        }
        using namespace base;
        class ArmyList;
        class Unit;
        class ResNode;
        class Castle;
        class Agent;
        class AgentProxy;
        class Troop;
        class ArmyInfo;
        class CampTemp;
        class CampFixed;
        class FamousCity;
        class MapMailboxEventHandler;
        class Alliance;
        struct TeamInfo;
        struct BattleInfo;
        class Capital;
        class Catapult;


        class MapMailboxEventHandler : public base::cluster::Mailbox::EventHandler
        {
        public:
            MapMailboxEventHandler() {
            }

            virtual ~MapMailboxEventHandler() {
            }

            virtual void OnMessageReceive(base::cluster::MessageIn& msgin);

        };

        class MapProxy
        {
        public:
            MapProxy(Map& map);
            ~MapProxy();

            void SetUp();

            base::memory::MemoryPool& mempool()
            {
                return *m_mempool;
            }

            base::cluster::Mailbox* mbox()
            {
                return m_mailbox;
            }

            void SendToCS(const std::function<void(base::lua::LuaMessageWriter&)>& fun);
            
            void SendMail(int64_t toUid, int64_t otherUid, model::MailType type, model::MailSubType subType, const std::string& attachment, const std::string& params, bool isDraw);
            
            void SendcsMapGlobalData();

            void SendcsAllianceCity();

            void SetMapGlobalData(base::DataTable& dt);

            void SendcsAddAllianceRecord(int64_t allianceId, model::AllianceMessageType type, std::string param);

            void SendcsAddTransportRecord(Troop* troop, int64_t uid, int32_t headId, std::string nickName, int64_t toUid, int32_t toHeadId, std::string toNickName, 
                model::TransportArriveType ArriveType, int64_t timestamp);

            void SendcsBootstrapFinishResponse();

            void SendcsPalaceWarStart();

            void SendcsPalaceWarEnd();

            void SendcsPalaceWarPrepare(int times, int dropId);

            template<typename T>
            void SendcsAppendPush(int64_t uid, model::PushClassify classify, const T& tag, int cdId, int64_t sendTime) {
                SendToCS([&](base::lua::LuaMessageWriter & lmw) {
                    base::DataTable pushInfo;
                    pushInfo.Set("uid", uid);
                    pushInfo.Set("classify", (int)classify);
                    pushInfo.Set("tag", tag);
                    pushInfo.Set("cdId", cdId);
                    pushInfo.Set("sendTime", sendTime);
                    lmw.WriteArgumentCount(2);
                    lmw.WriteString("appendPush");
                    lmw.WriteValue(pushInfo);
                });
            }

            void BroadNoticeMessage(int id, int type, const std::string& param = "");

            // todo 如果用了新战报系统，
            void AddReport(int64_t uid, int reportId, const std::string& reportData);

        private:
            Map& m_map;

            MapMailboxEventHandler* m_mailboxEventHandler = nullptr;
            base::cluster::Mailbox* m_mailbox = nullptr;
            base::memory::MemoryPool* m_mempool = nullptr;
        };
    }
}

#endif // MAP_MAP_PROXY_H

