#ifndef CLIENTSESSION_H
#define CLIENTSESSION_H

#include <base/modulebase.h>
#include <base/gateway/userclient.h>
#include <base/gateway/usersession.h>
#include <base/memory/memorypoolmgr.h>
#include <base/utils/utils_string.h>
#include <base/utils/crypto.h>
#include <base/3rd/lua/llex.h>
#include <model/protocol.h>



using namespace std;
using namespace base::gateway;

class MySession : public base::gateway::UserSession
{
    public:
        MySession(UserClient* client) ;
        virtual ~MySession();

        void SendTemplateCheck() ;

        void SendLogin(std::string username);

        void sendMapSwitch(int kingdomid) ;

        void SendPingReply(int sequence) ;

        void BuildingCreate(int gridId, int tplId, bool buildNow);

        void BuildingUpgrade(int gridId, bool upgradeNow) ;
        
        void BuildingTrain(int gridId, int queueIndex,  int armyId, int count, bool trainNow);
        
        void BuildingTrainCancle(int gridId, int queueIndex);
        
        void BuildingTrainQueue(int gridId);
        
        void BuildingCollect(int gridId, int index);

        void BuildingUpgradeCancel(int gridId) ;

        void BuildingMove(int from, int to) ;

        void BuildingDemolish(int gridId) ;

        void BuildingDemolishCancel(int gridId) ;

        void BuildingOpenBuilder2() ;

        void BuildingOpenForest(int frestId) ;
        
        void BuildTechnology(int gridId, int tplId, bool trainNow);
        
        void BuildTechnologyCancle(int gridId, int tplId);

        void SkillReset() ;
        void SkillAddPoint(int tplId) ;

        void QuestDraw(int questId);

        void MailView(int mailType, int64_t mailId);
        void MailDelete(int mailType, int64_t mailId);
        void MailDraw(int mailType, int64_t mailId, int64_t parentId);
        void MailSendPrivate(int64_t uid, std::string content);
        void MailSendAlliance(std::string content);
        void MailShare(int mailType, int64_t mailId, int64_t parentId);
        void MailShareView(int64_t uid, int64_t mailId);

        void MapJoin(int k, int x, int y);
        void MapLeave();
        void MapView(int x, int y);
        void MapMarch(int t, int x, int y);
        void MapReMarch(int troopId, int t, int x, int y);

        // chat
        void ChatKingdom(std::string content);
        void ChatAlliance(std::string content);
        void ChatBlock(int64_t aid);
        void ChatUnblock(int64_t aid);

        // range
        void RangeFetch(int type);

        //misc
        void Sign();
        void DrawOnline();
        void Turnplate();
        void TurnplateRecords();

        //quest target
        void QuestTarget(int target);
        //Seven Target
        void SevenTarget(int id);

        //alliance gift
        void GiftDraw(int64_t gid);
        
        //Skill
        void SkillAdd(int64_t tplId, bool isFull);

        //daily task
        void DailyDraw(int id);
        
        //hero
        void HeroDraw(int drawType, bool isTen, int64_t bid);
        
        //army team
        void TeamSave(int team, int troopType, int x, int y);
        void TeamRemove(int team);
        
        //bag puton/takeoff
        void BagSynthe(int64_t bid);
        void BagSell(int64_t bid, int count);
        void HeroPutOn(int type, int64_t bid, int heroId, int slot);
        void HeroTakeOff(int type, int heroId, int slot);
        
        void HeroStarUpgrage(int heroId, int64_t bid);
        void HeroSkillUpgrage(int heroId, int slot, int64_t bid);
        void HeroEquipUpgrage(int heroId, int slot, int64_t bid);
        void HeroEquipSuccinct(int heroId, int slot, int locate);
        void HeroEquipInlayOn(int heroId, int slot, int locate, int64_t bid);
        void HeroEquipInlayOff(int heroId, int slot, int locate);

        //battle
        void BattleStart(int teamId);
        void BattleOver(int battleId);
        void HeroBattleStart();
        void HeroBattleOver(int battleId);

        //scenario copy
        void ScenarioMmopup(int chapterId, int sectionId, int count);
        void ScenarioDrew(int chapterId, int target);
        void ScenarioFight(int chapterId, int sectionId);
        void ScenarioAnswer(int chapterId, int sectionId, int count);

        void TakeTax(bool isGoldTax);
        
        // alliance
        void AillanceHireHero(int heroId);
        void AillanceRecallHero(int heroId);
        void AillanceEmployHero(int64_t ownerUid, int heroId);

        virtual void OnUserClientReceivePacket(PacketIn& pktin)  ;

        virtual void OnUserClientClose() {


        }
    private:
        std::string m_localization;
};
extern MySession* g_session;
#endif // CLIENTSESSION_H
