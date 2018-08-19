#include "clientSession.h"
#include <iomanip>


MySession* g_session = nullptr;

MySession::MySession(UserClient* client) : UserSession(client)
{
    g_session = this;
}
MySession::~MySession()
{
    g_session = nullptr;
}

void MySession::SendTemplateCheck()
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::TEMPLATE_CHECK, 30, client()->mempool());
    pktout.WriteVarInteger(0);
    Send(pktout);
}

void MySession::SendLogin(std::string username)
{
    static int num = 0;
    base::gateway::PacketOut pktout((uint16_t)model::CS::LOGIN, 20, client()->mempool());
    pktout.WriteString("test");
    pktout.WriteString(username);
    pktout.WriteString("token");
    pktout.WriteString("idfa");
    pktout.WriteString("channel");
    pktout.WriteString("1.0.0");
    pktout.WriteVarInteger(1);
    pktout.WriteBoolean(false);
    Send(pktout);
}

void MySession::sendMapSwitch(int kingdomid)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::MAP_SWITCH, 20, client()->mempool());
    pktout << kingdomid;
    Send(pktout);
}

void MySession::SendPingReply(int sequence)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::PING_REPLY, 30, client()->mempool());
    pktout << sequence;
    Send(pktout);
}

void MySession::BuildingCreate(int gridId, int tplId, bool buildNow)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::BUILDING_CREATE, 20, client()->mempool());
    pktout.WriteVarInteger(gridId);
    pktout.WriteVarInteger(tplId);
    pktout.WriteBoolean(buildNow);
    Send(pktout);
}

void MySession::BuildingUpgrade(int gridId, bool upgradeNow)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::BUILDING_UPGRADE, 20, client()->mempool());
    pktout.WriteVarInteger(gridId);
    pktout.WriteBoolean(upgradeNow);
    Send(pktout);
}

void MySession::BuildingTrain(int gridId, int queueIndex, int armyId, int count, bool trainNow)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::BUILDING_TRAIN, 20, client()->mempool());
    pktout.WriteVarInteger(gridId);
    pktout.WriteVarInteger(queueIndex);
    pktout.WriteVarInteger(armyId);
    pktout.WriteVarInteger(count);
    pktout.WriteBoolean(trainNow);
    Send(pktout);
}

void MySession::BuildingTrainCancle(int gridId, int queueIndex)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::BUILDING_TRAIN_CANCEL, 20, client()->mempool());
    pktout.WriteVarInteger(gridId);
    pktout.WriteVarInteger(queueIndex);
    Send(pktout);
}

void MySession::BuildTechnology(int gridId, int tplId, bool trainNow)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::BUILDING_TECHNOLOGY_RESEARCH, 20, client()->mempool());
    pktout.WriteVarInteger(gridId);
    pktout.WriteVarInteger(tplId);
    pktout.WriteBoolean(trainNow);
    Send(pktout);
}

void MySession::BuildTechnologyCancle(int gridId, int tplId)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::BUILDING_TECHNOLOGY_RESEARCH_CANCEL, 20, client()->mempool());
    pktout.WriteVarInteger(gridId);
    pktout.WriteVarInteger(tplId);
    Send(pktout);
}



void MySession::BuildingTrainQueue(int gridId)
{
//     base::gateway::PacketOut pktout((uint16_t)model::CS::BUILDING_TRAIN_QUEUES, 20, client()->mempool());
//     pktout.WriteVarInteger(gridId);
//     Send(pktout);
}

void MySession::BuildingCollect(int gridId, int index)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::BUILDING_COLLECT, 20, client()->mempool());
    pktout.WriteVarInteger(gridId);
    pktout.WriteVarInteger(index);
    Send(pktout);
}

void MySession::BuildingUpgradeCancel(int gridId)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::BUILDING_UPGRADE_CANCEL, 20, client()->mempool());
    pktout.WriteVarInteger(gridId);
    Send(pktout);
}

void MySession::BuildingDemolish(int gridId)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::BUILDING_DEMOLISH, 20, client()->mempool());
    pktout.WriteVarInteger(gridId);
    Send(pktout);
}

void MySession::BuildingDemolishCancel(int gridId)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::BUILDING_DEMOLISH_CANCEL, 20, client()->mempool());
    pktout.WriteVarInteger(gridId);
    Send(pktout);
}


/*
void MySession::BuildingMove(int from, int to)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::BUILDING_MOVE, 20, client()->mempool());
    pktout.WriteVarInteger(from);
    pktout.WriteVarInteger(to);
    Send(pktout);
}

void MySession::BuildingOpenBuilder2()
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::BUILDING_OPEN_BUILDER2, 20, client()->mempool());
    Send(pktout);
}

void MySession::BuildingOpenForest(int frestId)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::BUILDING_OPEN_FOREST, 20, client()->mempool());
    pktout.WriteVarInteger(frestId);
    Send(pktout);
}
*/
void MySession::SkillReset()
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::SKILL_RESET, 20, client()->mempool());
    Send(pktout);
}
void MySession::SkillAddPoint(int tplId)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::SKILL_ADD_POINT, 20, client()->mempool());
    pktout.WriteVarInteger(tplId);
    Send(pktout);
}
void MySession::QuestDraw(int questId)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::QUEST_DRAW, 20, client()->mempool());
    pktout.WriteVarInteger(questId);
    Send(pktout);
}

void MySession::MailView(int mailType, int64_t mailId)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::MAIL_VIEW, 20, client()->mempool());
    pktout.WriteVarInteger(mailType);
    pktout.WriteVarInteger(mailId);
    Send(pktout);
}
void MySession::MailDelete(int mailType, int64_t mailId)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::MAIL_DELETE, 20, client()->mempool());
    pktout.WriteVarInteger(mailType);
    pktout.WriteVarInteger(1);
    pktout.WriteVarInteger(mailId);
    Send(pktout);
}
void MySession::MailDraw(int mailType, int64_t mailId, int64_t parentId)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::MAIL_DRAW_ATTACHMENT, 20, client()->mempool());
    pktout.WriteVarInteger(mailType);
    pktout.WriteVarInteger(1);
    pktout.WriteVarInteger(mailId);
    pktout.WriteVarInteger(parentId);
    Send(pktout);
}
void MySession::MailSendPrivate(int64_t uid, string content)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::MAIL_PRIVATE_SEND, 20, client()->mempool());
    pktout.WriteVarInteger(uid);
    pktout.WriteString(content);
    Send(pktout);
}
void MySession::MailSendAlliance(string content)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::MAIL_ALLIANCE_SEND, 20, client()->mempool());
    pktout.WriteString(content);
    Send(pktout);
}
void MySession::MailShare(int mailType, int64_t mailId, int64_t parentId)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::MAIL_ALLIANCE_SHARE, 20, client()->mempool());
    pktout.WriteVarInteger(mailType);
    pktout.WriteVarInteger(mailId);
    pktout.WriteVarInteger(parentId);
    Send(pktout);
}
void MySession::MailShareView(int64_t uid, int64_t mailId)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::MAIL_ALLIANCE_SHARE_VIEW, 20, client()->mempool());
    pktout.WriteVarInteger(uid);
    pktout.WriteVarInteger(mailId);
    Send(pktout);
}

void MySession::MapJoin(int k, int x, int y)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::MAP_JOIN, 20, client()->mempool());
    pktout.WriteVarInteger(k);
    pktout.WriteVarInteger(x);
    pktout.WriteVarInteger(y);
    pktout.WriteVarInteger(2);
    Send(pktout);
}
void MySession::MapLeave()
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::MAP_LEAVE, 20, client()->mempool());
    Send(pktout);
}
void MySession::MapView(int x, int y)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::MAP_VIEW, 20, client()->mempool());
    pktout.WriteVarInteger(x);
    pktout.WriteVarInteger(y);
    pktout.WriteVarInteger(1);
    Send(pktout);
}
void MySession::MapMarch(int t, int x, int y)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::MAP_MARCH, 20, client()->mempool());
    pktout.WriteVarInteger(t);
    pktout.WriteVarInteger(x);
    pktout.WriteVarInteger(y);
    pktout.WriteVarInteger(2);
    pktout.WriteVarInteger(1);
    pktout.WriteVarInteger(1300001);
    pktout.WriteVarInteger(1);
    pktout.WriteVarInteger(1);
    pktout.WriteVarInteger(10);
    pktout.WriteVarInteger(1);
    Send(pktout);
}

void MySession::MapReMarch(int troopId, int t, int x, int y)
{
//      base::gateway::PacketOut pktout((uint16_t)model::CS::MAP_CAMP_MARCH, 20, client()->mempool());
//     pktout.WriteVarInteger(troopId);
//      pktout.WriteVarInteger(t);
//     pktout.WriteVarInteger(x);
//     pktout.WriteVarInteger(y);
//     Send(pktout);   
}

void MySession::ChatKingdom(string content)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::CHAT_SEND, 20, client()->mempool());
    pktout.WriteVarInteger(1);
    pktout.WriteString(content);
    Send(pktout);
}
void MySession::ChatAlliance(string content)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::CHAT_SEND, 20, client()->mempool());
    pktout.WriteVarInteger(2);
    pktout.WriteString(content);
    Send(pktout);
}
void MySession::ChatBlock(int64_t aid)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::CHAT_BLOCK, 20, client()->mempool());
    pktout.WriteVarInteger(aid);
    Send(pktout);
}
void MySession::ChatUnblock(int64_t aid)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::CHAT_UNBLOCK, 20, client()->mempool());
    pktout.WriteVarInteger(aid);
    Send(pktout);
}

void MySession::RangeFetch(int type)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::RANGE_FETCH, 20, client()->mempool());
    pktout.WriteVarInteger(0);
    pktout.WriteVarInteger(type);
    Send(pktout);
}

void MySession::Sign()
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::MISC_SIGN, 20, client()->mempool());
    Send(pktout);
}
void MySession::DrawOnline()
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::MISC_DRAW_ONLINE, 20, client()->mempool());
    Send(pktout);
}
void MySession::Turnplate()
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::MISC_TURNPLATE_DRAW, 20, client()->mempool());
    Send(pktout);
}
void MySession::TurnplateRecords()
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::MISC_TURNPLATE_RECORDS_FETCH, 20, client()->mempool());
    Send(pktout);
}

void MySession::SkillAdd(int64_t tplId, bool isFull)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::SKILL_ADD_POINT, 20, client()->mempool());
    pktout.WriteVarInteger(tplId);
    pktout.WriteBoolean(isFull);
    Send(pktout);
}

void MySession::HeroDraw(int drawType, bool isTen, int64_t bid)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::HERO_DRAW, 20, client()->mempool());
    pktout.WriteVarInteger(drawType);
    pktout.WriteBoolean(isTen);
    pktout.WriteVarInteger<int64_t>(bid);
    Send(pktout);
}

void MySession::TeamSave(int team, int troopType, int x, int y)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::SAVE_TEAM_INFO, 20, client()->mempool());
    pktout.WriteVarInteger(1);
    pktout.WriteVarInteger(1300001);
    pktout.WriteVarInteger(1);
    pktout.WriteVarInteger(100);
    pktout.WriteVarInteger(1);
    
    pktout.WriteVarInteger(team);
    pktout.WriteVarInteger(troopType);
    pktout.WriteVarInteger(x);
    pktout.WriteVarInteger(y);
    
    Send(pktout);
}

void MySession::TeamRemove(int team)
{
}

void MySession::BagSynthe(int64_t bid)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::BAG_SYNTHESIZE, 20, client()->mempool());
    pktout.WriteVarInteger<int64_t>(bid);
    Send(pktout);
}

void MySession::BagSell(int64_t bid, int count)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::BAG_SELL, 20, client()->mempool());
    pktout.WriteVarInteger<int64_t>(bid);
    pktout.WriteVarInteger(count);
    Send(pktout);
}


void MySession::HeroPutOn(int type, int64_t bid, int heroId, int slot)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::HERO_PUTON, 20, client()->mempool());
    pktout.WriteVarInteger(type);
    pktout.WriteVarInteger<int64_t>(bid);
    pktout.WriteVarInteger(heroId);
    pktout.WriteVarInteger(slot);
    Send(pktout);
}

void MySession::HeroTakeOff(int type, int heroId, int slot)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::HERO_TAKEOFF, 20, client()->mempool());
    pktout.WriteVarInteger(type);
    pktout.WriteVarInteger(heroId);
    pktout.WriteVarInteger(slot);
    Send(pktout);
}

void MySession::HeroStarUpgrage(int heroId, int64_t bid)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::HERO_STAR_UPGRADE, 20, client()->mempool());
    pktout.WriteVarInteger(heroId);
    pktout.WriteVarInteger<int64_t>(bid);
    Send(pktout);
}

void MySession::HeroSkillUpgrage(int heroId, int slot, int64_t bid)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::HERO_SKILL_UPGRADE, 20, client()->mempool());
    pktout.WriteVarInteger(heroId);
    pktout.WriteVarInteger(slot);
    pktout.WriteVarInteger(1);
    
    pktout.WriteVarInteger<int64_t>(bid);
    pktout.WriteVarInteger(1);
    Send(pktout);
}

void MySession::HeroEquipUpgrage(int heroId, int slot, int64_t bid)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::HERO_EQUIP_UPGRADE, 20, client()->mempool());
    pktout.WriteVarInteger(heroId);
    pktout.WriteVarInteger(slot);
    pktout.WriteVarInteger(1);
    
    pktout.WriteVarInteger<int64_t>(bid);
    pktout.WriteVarInteger(1);
    Send(pktout);
}

void MySession::HeroEquipSuccinct(int heroId, int slot, int locate)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::HERO_EQUIP_SUCCINCT, 20, client()->mempool());
    pktout.WriteVarInteger(heroId);
    pktout.WriteVarInteger(slot);
    pktout.WriteVarInteger(locate);
    Send(pktout);
}

void MySession::HeroEquipInlayOn(int heroId, int slot, int locate, int64_t bid)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::HERO_EQUIP_INLAY_ON, 20, client()->mempool());
    pktout.WriteVarInteger(heroId);
    pktout.WriteVarInteger(slot);
    pktout.WriteVarInteger(locate);
    pktout.WriteVarInteger<int64_t>(bid);
    Send(pktout);
}

void MySession::HeroEquipInlayOff(int heroId, int slot, int locate)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::HERO_EQUIP_INLAY_OFF, 20, client()->mempool());
    pktout.WriteVarInteger(heroId);
    pktout.WriteVarInteger(slot);
    pktout.WriteVarInteger(locate);
    Send(pktout);
}

void MySession::BattleStart(int teamId)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::BATTLE_START, 20, client()->mempool());
    pktout.WriteVarInteger(0);
    pktout.WriteVarInteger(teamId);
    Send(pktout);
}

void MySession::BattleOver(int battleId)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::BATTLE_OVER, 20, client()->mempool());
    pktout.WriteVarInteger(battleId);
    pktout.WriteVarInteger(1);
    pktout.WriteVarInteger(0);
    pktout.WriteVarInteger(0);
    Send(pktout);
}

void MySession::HeroBattleStart()
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::HERO_BATTLE_START, 20, client()->mempool());
    pktout.WriteVarInteger(0);
    Send(pktout);
}

void MySession::HeroBattleOver(int battleId)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::HERO_BATTLE_OVER, 20, client()->mempool());
    pktout.WriteVarInteger(battleId);
    Send(pktout);
}

void MySession::ScenarioMmopup(int chapterId, int sectionId, int count)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::SCENARIO_MOPUP, 20, client()->mempool());
    pktout.WriteVarInteger(chapterId);
    pktout.WriteVarInteger(sectionId);
    pktout.WriteVarInteger(count);
    Send(pktout);
}

void MySession::ScenarioDrew(int chapterId, int target)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::SCENARIO_STAR_DRAW, 20, client()->mempool());
    pktout.WriteVarInteger(chapterId);
    pktout.WriteVarInteger(target);
    Send(pktout);
}

void MySession::ScenarioFight(int chapterId, int sectionId)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::SCENARIO_FIGHT, 20, client()->mempool());
    pktout.WriteVarInteger(chapterId);
    pktout.WriteVarInteger(sectionId);
    pktout.WriteVarInteger(3);
    
    pktout.WriteVarInteger(1300001);
    pktout.WriteVarInteger(1);
    pktout.WriteVarInteger(100);
    pktout.WriteVarInteger(1);
    
    pktout.WriteVarInteger(1300002);
    pktout.WriteVarInteger(2);
    pktout.WriteVarInteger(200);
    pktout.WriteVarInteger(2);
    
    pktout.WriteVarInteger(1320005);
    pktout.WriteVarInteger(3);
    pktout.WriteVarInteger(300);
    pktout.WriteVarInteger(3);
    Send(pktout); 
}

void MySession::ScenarioAnswer(int chapterId, int sectionId, int count)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::SCENARIO_ANSWER, 20, client()->mempool());
    pktout.WriteVarInteger(chapterId);
    pktout.WriteVarInteger(sectionId);
    pktout.WriteVarInteger(count);
    Send(pktout);
}

void MySession::TakeTax(bool isGoldTax)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::TAKE_TAX, 20, client()->mempool());
    pktout.WriteBoolean(isGoldTax);
    Send(pktout);
}

void MySession::AillanceHireHero(int heroId)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::ALLIANCE_USER_HIRE_HERO, 20, client()->mempool());
    pktout.WriteVarInteger(heroId);
    Send(pktout);
}

void MySession::AillanceRecallHero(int heroId)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::ALLIANCE_USER_RECALL_HERO, 20, client()->mempool());
    pktout.WriteVarInteger(heroId);
    Send(pktout);
}

void MySession::AillanceEmployHero(int64_t ownerUid, int heroId)
{
    base::gateway::PacketOut pktout((uint16_t)model::CS::ALLIANCE_USER_EMPLOY_HERO, 20, client()->mempool());
    pktout.WriteVarInteger<int64_t>(ownerUid);
    pktout.WriteVarInteger(heroId);
    Send(pktout);
}




void MySession::OnUserClientReceivePacket(PacketIn& pktin)
{
//     model::SC code = static_cast<model::SC>(pktin.code());
//     switch (code) {
//         case model::SC::TEMPLATE_UPDATE: {
//             int progress = pktin.ReadVarInteger<int>();
//             int total = pktin.ReadVarInteger<int>();
//             cout << "Template Update:" << std::right << std::setw(2) << progress << "/" << total << " ";
//             if (total > 0) {
//                 string name = pktin.ReadString();
//                 int partProgress = pktin.ReadVarInteger<int>();
//                 int partTotal = pktin.ReadVarInteger<int>();
//                 string raw = pktin.ReadString();
//                 string plain = base::utils::zlib_uncompress(raw);
// 
//                 cout << "tpl:" << left << setw(22) << name <<
//                 "(" << partProgress << "/" << partTotal << ") hash=" << base::utils::sha1hex(plain) << endl;
//                 if (name == "localization.json") {
//                     m_localization += raw;
//                     if (partProgress == partTotal) {
// //                         cout << base::utils::zlib_uncompress(m_localization) << endl;
//                     }
//                 }
//             }
//         }
//         break;
//         case model::SC::LOGIN_RESPONSE: {
//             int result = pktin.ReadVarInteger<int>();
//             string reason = pktin.ReadString();
//             cout << "LoginResponse=" << result << " " << reason.c_str() << endl;
//         }
//         break;
//         case model::SC::LOGOUT: {
//             int kickType = pktin.ReadVarInteger<int>();
//             cout << "LOGOUT kickType=" << kickType << endl;
//         }
//         break;
//         case model::SC::PING: {
//             SendPingReply(pktin.ReadVarInteger<int>());
//         }
//         break;
//         case model::SC::PING_RESULT:  {
//             int delay = pktin.ReadVarInteger<int>();
//             cout << "network delay=" << delay << endl;
//         }
//         break;
//         case model::SC::READY: {
//             sendMapSwitch(0);
//             //BuildingCreate(25, 1220000, true);
//         }
//         break;
// 
//         case model::SC::BUILDING_UPDATE: {
//         }
//         break;
//         case model::SC::BUILDING_CREATE_RESPONSE: {
//             bool ok = pktin.ReadBoolean();
//             cout << "BuildingCreateResponse=" << (int)ok << endl;
//         }
//         break;
//         case model::SC::BUILDING_UPGRADE_RESPONSE: {
//             bool ok = pktin.ReadBoolean();
//             cout << "BuildingUpgradeResponse=" << (int)ok << endl;
//         }
//         break;
//         case model::SC::BUILDING_UPGRADE_CANCEL_RESPONSE: {
//             bool ok = pktin.ReadBoolean();
//             cout << "BuildingUpgreadCancelResponse=" << (int)ok << endl;
//         }
//         break;
//         case model::SC::BUILDING_TRAIN_RESPONSE: {
//             bool ok = pktin.ReadBoolean();
//             cout << "BUILDING_TRAIN_RESPONSE=" << (int)ok << endl;
//         }
//         break;
// //         case model::SC::BUILDING_TRAIN_QUEUES_RESPONSE: {
// //             int gridId = pktin.ReadVarInteger<int>();
// //             int trailQueues = pktin.ReadVarInteger<int>();
// //             cout << "BUILDING_TRAIN_QUEUES_RESPONSE, gridId = " << gridId <<" trailQueues = "<< trailQueues << endl;
// //             int len = pktin.ReadVarInteger<int>();
// //             for (int i=0; i<len; ++i){
// //                 int cdId = pktin.ReadVarInteger<int>();
// //                 int queueIndex = pktin.ReadVarInteger<int>();
// //                 int armyId = pktin.ReadVarInteger<int>();
// //                 int count = pktin.ReadVarInteger<int>();
// //                 cout << "BUILDING_TRAIN_QUEUES_RESPONSE, cdId = " << cdId <<" queueIndex = "<< queueIndex <<" armyId = "<< armyId << " count = "<< count << endl;
// //             }
// //         }
// //         break;
//         case model::SC::BUILDING_COLLECT_RESPONSE: {
//             int result = pktin.ReadVarInteger<int>();
//             int gridId = pktin.ReadVarInteger<int>();
//             cout << "BUILDING_COLLECT_RESPONSE, result = " << result << " gridId = "<< gridId << endl;
//             int len = pktin.ReadVarInteger<int>();
//             for (int i=0; i<len; ++i){
//                 int param1 = pktin.ReadVarInteger<int>();
//                 int param2 = pktin.ReadVarInteger<int>();
//                 cout << "BUILDING_COLLECT_RESPONSE, param1 = " << param1 << " param2 = "<< param2 << endl;
//             }
//         }
//         break;
//         /*
//         case model::SC::BUILDING_MOVE_RESPONSE: {
//             bool ok = pktin.ReadBoolean();
//             cout << "BuildingMoveResponse=" << (int)ok << endl;
//         }
//         break;
//         case model::SC::BUILDING_DEMOLISH_RESPONSE: {
//             bool ok = pktin.ReadBoolean();
//             cout << "BuildingDemolishResponse=" << (int)ok << endl;
//         }
//         break;
//         case model::SC::BUILDING_DEMOLISH_CANCEL_RESPONSE: {
//             bool ok = pktin.ReadBoolean();
//             cout << "BuildingDemolishCancelResponse=" << (int)ok << endl;
//         }
//         break;
//         case model::SC::BUILDING_OPEN_BUILDER2_RESPONSE: {
//             bool ok = pktin.ReadBoolean();
//             cout << "BuildingOpemBuilder2Response=" << (int)ok << endl;
//         }
//         break;
//         case model::SC::BUILDING_OPEN_FOREST_RESPONSE: {
//             bool ok = pktin.ReadBoolean();
//             cout << "BuildingOpemForestResponse=" << (int)ok << endl;
//         }
//         break;
//         */
//         case model::SC::SKILL_UPDATE: {
//         }
//         break;
//         case model::SC::SKILL_RESET_RESPONSE: {
//             bool ok = pktin.ReadBoolean();
//             cout << "SkillResetResponse=" << (int)ok << endl;
//         }
//         break;
//         case model::SC::SKILL_ADD_POINT_RESPONSE: {
//             bool ok = pktin.ReadBoolean();
//             cout << "SkillAddPointResponse=" << (int)ok << endl;
//         }
//         break;
//         case model::SC::QUEST_UPDATE: {
//             int len = pktin.ReadVarInteger<int>();
//             cout << "QuestUpdate ..... update size:" << len << endl;
//             for (int i = 0; i < len; ++i) {
//                 int questId = pktin.ReadVarInteger<int>();
//                 int progress = pktin.ReadVarInteger<int>();
//                 cout << "QuestUpdate id:" << questId << "  progress:" << progress << endl;
//             }
//             len = pktin.ReadVarInteger<int>();
//             cout << "QuestUpdate ..... draw size:" << len << endl;
//             for (int i = 0; i < len; ++i) {
//                 int questId = pktin.ReadVarInteger<int>();
//                 cout << "QuestUpdate draw id:" << questId << endl;
//             }
//         }
//         break;
//         case model::SC::MAIL_UPDATE: {
//             int size = pktin.size();
//             int len = pktin.ReadVarInteger<int>();
//             cout << "MailUpdate ..... update size:" << len << "pkt size:" << size << endl;
//             for (int i = 0; i < len; ++i) {
//                 int mailId = pktin.ReadVarInteger<int>();
//                 string title = pktin.ReadString();
//                 int type = pktin.ReadVarInteger<int>();
//                 int subType = pktin.ReadVarInteger<int>();
//                 bool isRead = pktin.ReadBoolean();
//                 bool isDraw = pktin.ReadBoolean();
//                 int createTime = pktin.ReadVarInteger<int>();
//                 int attachmentSize = pktin.ReadVarInteger<int>();
//                 long headId = pktin.ReadVarInteger<int>();
//                 string params = pktin.ReadString();
//                 printf("MailUpdate ...... i %u, mailid %u, title %s, type %u, subType %u, isRead %s, isDraw %s, createTime %u, attachmentSize %u, params %s\n",
//                        i, mailId, title.c_str(), type, subType, (isRead ? "true" : "false"), (isDraw ? "true" : "false"), createTime, attachmentSize, params.c_str());
//             }
//         }
//         break;
// //         case model::SC::MAIL_DETAIL_UPDATE: {
// //             int len = pktin.ReadVarInteger<int>();
// //             cout << "MailDetailUpdate ...... update size:" << len << endl;
// //             for (int i = 0; i < len; ++i) {
// //                 int mailId = pktin.ReadVarInteger<int64_t>();
// //                 int mailParentId = pktin.ReadVarInteger<int64_t>();
// //                 string title = pktin.ReadString();
// //                 string content = pktin.ReadString();
// //                 int type = pktin.ReadVarInteger<int>();
// //                 int subType = pktin.ReadVarInteger<int>();
// //                 string params = pktin.ReadString();
// //                 bool isRead= pktin.ReadBoolean();
// //                 bool isDraw = pktin.ReadBoolean();
// //                 int createTime = pktin.ReadVarInteger<int>();
// //                 string attachments = pktin.ReadString();
// //                 printf("MailDetailUpdate...... i %u, mailid %u, parentId %u, title %s, content %s, type %u, subType %u, params %s, isDraw %s, createTime %u, attachments %s\n",
// //                        i, mailId, mailParentId, title.c_str(), content.c_str(), type, subType, params.c_str(), (isDraw ? "true" : "false"), createTime, attachments.c_str());
// //             }
// //         }
// //         break;
//         case model::SC::MAIL_DELETE_RESPONSE: {
//             int mailId = pktin.ReadVarInteger<int>();
//             bool isOk = pktin.ReadBoolean();
//             printf("MailDeleteResponse ......  mailId %u, isOk %s\n", mailId, (isOk ? "true" : "false"));
//         }
//         break;
//         case model::SC::MAIL_DRAW_ATTACHMENT_RESPONSE: {
//             int mailId = pktin.ReadVarInteger<int>();
//             bool isOk = pktin.ReadBoolean();
//             printf("MailDrawResponse ......  mailId %u, isOk %s\n", mailId, (isOk ? "true" : "false"));
//         }
//         break;
//         case model::SC::MAIL_ALLIANCE_SHARE_VIEW_REPONSE: {
//             int len = pktin.ReadVarInteger<int>();
//             cout << "MailAllianceShareViewReponse ...... size:" << len << endl;
//             for (int i = 0; i < len; ++i) {
//                 int mailId = pktin.ReadVarInteger<int>();
//                 string title = pktin.ReadString();
//                 string content = pktin.ReadString();
//                 int type = pktin.ReadVarInteger<int>();
//                 int subType = pktin.ReadVarInteger<int>();
//                 string params = pktin.ReadString();
//                 int createTime = pktin.ReadVarInteger<int>();
//                 string attachment = pktin.ReadString();
//                 printf("MailAllianceShareViewReponse...... i %u, mailid %u, title %s, content %s, type %u, subType %u, params %s, createTime %u, attachment %s\n",
//                        i, mailId, title.c_str(), content.c_str(), type, subType, params.c_str(), createTime, attachment.c_str());
//             }
//         }
//         break;
//         // alliance
//         case model::SC::ALLIANCE_INFO_UPDATE: {
//             int64_t aid = pktin.ReadVarInteger<int64_t>();
//             string name = pktin.ReadString();
//             string nickname = pktin.ReadString();
//             int64_t leaderId = pktin.ReadVarInteger<int64_t>();
//             string leaderName = pktin.ReadString();
//             int64_t leaderHeadId = pktin.ReadVarInteger<int64_t>();
//             int bannerId = pktin.ReadVarInteger<int>();
//             int alliesCount = pktin.ReadVarInteger<int>();
//             int alliesMax = pktin.ReadVarInteger<int>();
//             int power = pktin.ReadVarInteger<int>();
//             int language = pktin.ReadVarInteger<int>();
//             string slogan = pktin.ReadString();
//             string rankTitle = pktin.ReadString();
//             bool openRecruitment = pktin.ReadBoolean();
//             string bannerList = pktin.ReadString();
//             //
//             printf("AllianceInfoUpdate ...... aid %ld, name %s, nickname %s, leaderId %ld, leaderName %s, bannerId %d, alliesCount %d, alliesMax %d, power %d, language %d, slogan %s, rankTitle %s, bannerList %s, openRecruitment %s\n",
//                    aid, name.c_str(), nickname.c_str(), leaderId, leaderName.c_str(), bannerId, alliesCount, alliesMax, power, language, slogan.c_str(), rankTitle.c_str(), bannerList.c_str(), (openRecruitment ? "true" : "false"));
//         }
//         break;
//         case model::SC::ALLIANCE_MEMBER_INFO_UPDATE: {
//             int len = pktin.ReadVarInteger<int>();
//             printf("AllianceMemberInfoUpdate ...... member size %d\n", len);
//             for (int i = 0; i < len; ++i) {
//                 int64_t uid = pktin.ReadVarInteger<int64_t>();
//                 string nickname = pktin.ReadString();
//                 int64_t headId = pktin.ReadVarInteger<int64_t>();
//                 int totalPower = pktin.ReadVarInteger<int>();
//                 int rankLevel = pktin.ReadVarInteger<int>();
//                 int expToday = pktin.ReadVarInteger<int>();
//                 int expWeek = pktin.ReadVarInteger<int>();
//                 int expAll = pktin.ReadVarInteger<int>();
//                 int honorToday = pktin.ReadVarInteger<int>();
//                 int honorWeek = pktin.ReadVarInteger<int>();
//                 int honorAll = pktin.ReadVarInteger<int>();
//                 int lastOnlineTime = pktin.ReadVarInteger<int>();
//                 printf("AllianceMemberInfoUpdate ......member index %d, uid %ld, nickname %s, headId %ld, totalPower %d, rankLevel %d, lastOnlineTime %d\n",
//                        i, uid, nickname.c_str(), headId, totalPower, rankLevel, lastOnlineTime);
//             }
//             len = pktin.ReadVarInteger<int>();
//             printf("AllianceMemberInfoUpdate ...... delete size %d\n", len);
//             for (int i = 0; i < len; ++i) {
//                 int uid = pktin.ReadVarInteger<int>();
//                 printf("AllianceMemberInfoUpdate ...... delete index %d, uid\n", i, uid);
//             }
// 
//         }
//         break;
//         case model::SC::ALLIANCE_CHECK_NAME_RESPONSE: {
//             int result = pktin.ReadVarInteger<int>();
//             printf("AllianceCheckNameResponse ......  result %d\n", result);
//         }
//         break;
//         case model::SC::ALLIANCE_CREATE_RESPONSE: {
//             int result = pktin.ReadVarInteger<int>();
//             printf("AllianceCreateResponse ......  result %d\n", result);
//         }
//         break;
//         case model::SC::ALLIANCE_KICK_MEMBER_RESPONSE: {
//             int result = pktin.ReadVarInteger<int>();
//             printf("AllianceKickMemberResponse ......  result %d\n", result);
//         }
//         break;
//         case model::SC::ALLIANCE_APPLY_ACCEPT_RESPONSE: {
//             int result = pktin.ReadVarInteger<int>();
//             printf("AllianceApplyAccetpResponse ......  result %d\n", result);
//         }
//         break;
//         case model::SC::ALLIANCE_SEARCH_RESPONSE: {
//             int len = pktin.ReadVarInteger<int>();
//             printf("AllianceSearchResponse ......  len %d\n", len);
//             for (int i = 0; i < len; ++i) {
//                 int aid = pktin.ReadVarInteger<int>();
//                 string name = pktin.ReadString();
//                 string nickname = pktin.ReadString();
//                 int leaderId = pktin.ReadVarInteger<int>();
//                 string leaderName = pktin.ReadString();
//                 int leaderHeadId = pktin.ReadVarInteger<int>();
//                 int bannerId = pktin.ReadVarInteger<int>();
//                 int alliesCount = pktin.ReadVarInteger<int>();
//                 int alliesMax = pktin.ReadVarInteger<int>();
//                 int power = pktin.ReadVarInteger<int>();
//                 int language = pktin.ReadVarInteger<int>();
//                 string slogan = pktin.ReadString();
//                 string rankTitle = pktin.ReadString();
//                 printf("AllianceSearchResponse index %d ...... aid %d, name %s, nickname %s, leaderId %d, leaderName %s, bannerId %d, alliesCount %d, alliesMax %d, power %d, language %d, slogan %s, rankTitle %s\n",
//                        i, aid, name.c_str(), nickname.c_str(), leaderId, leaderName.c_str(), bannerId, alliesCount, alliesMax, power, language, slogan.c_str(), rankTitle.c_str());
//             }
//         }
//         break;
//         case model::SC::ALLIANCE_APPLY_RESPONSE: {
//             int result = pktin.ReadVarInteger<int>();
//             printf("AllianceApplyResponse ......  result %d\n", result);
//         }
//         break;
//         case model::SC::ALLIANCE_HELP_REQUEST_RESPONSE: {
//             int result = pktin.ReadVarInteger<int>();
//             printf("AllianceHelpRequestResponse ......  result %d\n", result);
//         }
//         break;
//         case model::SC::ALLIANCE_HELP_UPDATE: {
//             int updateLen = pktin.ReadVarInteger<int>();
//             printf("AllianceHelpUpdate ......  updateLen %d\n", updateLen);
//             for (int i = 0; i < updateLen; ++i) {
//                 int helpId = pktin.ReadVarInteger<int>();
//                 int buildingId = pktin.ReadVarInteger<int>();
//                 int times = pktin.ReadVarInteger<int>();
//                 int max = pktin.ReadVarInteger<int>();
//                 int uid = pktin.ReadVarInteger<int>();
//                 string nickname = pktin.ReadString();
//                 int headId = pktin.ReadVarInteger<int>();
//                 string info = pktin.ReadString();
//                 printf("AllianceHelpUpdate ...... UPDATE index %d: helpId%d, buildingId %d, times %d, max %d, uid %d, nickname %s, headId %d, info %s\n", i, helpId, buildingId, times, max, uid, nickname.c_str(), headId, info.c_str());
//             }
//             int deleteLen = pktin.ReadVarInteger<int>();
//             printf("AllianceHelpUpdate ......  deleteLen %d\n", deleteLen);
//             for (int i = 0; i < deleteLen; ++i) {
//                 int helpId = pktin.ReadVarInteger<int>();
//                 printf("AllianceHelpUpdate ...... DELETE  index %d: helpId %d\n", i, helpId);
//             }
// 
//         }
//         break;
//         // chat
//         case model::SC::CHAT_RECV: {
//             printf("CHAT_RECV ...... \n");
//             //*
//             int type = pktin.ReadVarInteger<int>();
//             int subType = pktin.ReadVarInteger<int>();
//             int64_t from_uid = pktin.ReadVarInteger<int64_t>();
//             string from_nickname = pktin.ReadString();
//             int vip_level = pktin.ReadVarInteger<int>();
//             int64_t head_id = pktin.ReadVarInteger<int64_t>();
//             int language = pktin.ReadVarInteger<int>();
// 	    string allianceNickname = pktin.ReadString();
//             int time = pktin.ReadVarInteger<int>();
//             string content = pktin.ReadString();
//             string params =  pktin.ReadString();
//             printf("CHAT_RECV ......  type %u, subType %u, from_uid %lu, from_nickname %s, vip_level %u, head_id %lu, language %u, allianceNickname %s, time %u, content %s, params %s\n", type, subType, from_uid, from_nickname.c_str(), vip_level, head_id, language,allianceNickname.c_str(), time, content.c_str(), params.c_str());
//             //*/
//         }
//         break;
//         case model::SC::CHAT_BLOCK_RESPONSE: {
//             int result = pktin.ReadVarInteger<int>();
//             printf("ChatlockResponse ......  result %d\n", result);
//         }
//         break;
//         case model::SC::CHAT_UNBLOCK_RESPONSE: {
//             int result = pktin.ReadVarInteger<int>();
//             printf("ChatUnblockResponse ......  result %d\n", result);
//         }
//         break;
//         case model::SC::CHAT_BLOCK_UPDATE: {
//             int len = pktin.ReadVarInteger<int>();
//             printf("ChatBlockUpdate ......  update len %d\n", len);
//             for (int i = 0; i < len; ++i) {
//                 int uid = pktin.ReadVarInteger<int>();
//                 string userNickname = pktin.ReadString();
//                 int headId = pktin.ReadVarInteger<int>();
//                 printf("ChatBlockUpdate ...... uid %d, userNickname %s, headId %d\n",
//                        uid, userNickname.c_str(), headId);
//             }
//             len = pktin.ReadVarInteger<int>();
//             printf("ChatBlockUpdate ......  delete len %d\n", len);
//             for (int i = 0; i < len; ++i) {
//                 int uid = pktin.ReadVarInteger<int>();
//                 printf("ChatBlockUpdate ......delete uid %d\n", uid);
//             }
//         }
//         break;
//         case model::SC::RANGE_FETCH_RESPONSE: {
//             int type = pktin.ReadVarInteger<int>();
//             int vsn = pktin.ReadVarInteger<int>();
//             int selfRank = pktin.ReadVarInteger<int>();
//             string data = pktin.ReadString();
//             printf("RangeFetchResponse ....... type %d, vsn %d, selfRank %d, data %s\n", type, vsn, selfRank, data.c_str());
//         }
//         break;
//         case model::SC::TIME_SYNC: {
//             int time = pktin.ReadVarInteger<int>();
//             int zone = pktin.ReadVarInteger<int>();
//             int isdst = pktin.ReadVarInteger<int>();
//             printf("TimeSync  ....... time %d, zone %d, isdst %d\n", time, zone, isdst);
//         }
//         break;
// 
//         /******* misc  *******/
//         case model::SC::MISC_SIGN_UPDATE: {
//             int signToday = pktin.ReadBoolean() ? 1 : 0;
//             int signCount = pktin.ReadVarInteger<int>();
//             printf("MISC_SIGN_UPDATE  ....... signToday %d, signCount %d\n", time, signToday, signCount);
//         }
//         break;
//         case model::SC::MISC_SIGN_RESPONSE: {
//             int ok = pktin.ReadBoolean() ? 1 : 0;
//             int tplId = pktin.ReadVarInteger<int>();
//             int count = pktin.ReadVarInteger<int>();
//             printf("MISC_SIGN_RESPONSE  ....... ok %d, tplId %d, count %d\n", ok, tplId, count);
//         }
//         break;
//         case model::SC::MISC_ONLINE_UPDATE: {
//             int step = pktin.ReadVarInteger<int>();
//             int leftSeconds = pktin.ReadVarInteger<int>();
//             int tplId = pktin.ReadVarInteger<int>();
//             int count = pktin.ReadVarInteger<int>();
//             printf("MISC_ONLINE_UPDATE  ....... step %d, leftSeconds %d, tplId %d, count %d\n", step, leftSeconds, tplId, count);
//         }
//         break;
//         case model::SC::MISC_DRAW_ONLINE_RESPONSE: {
//             int ok = pktin.ReadBoolean() ? 1 : 0;
//             int tplId = pktin.ReadVarInteger<int>();
//             int count = pktin.ReadVarInteger<int>();
//             printf("MISC_DRAW_ONLINE_RESPONSE  ....... ok %d, tplId %d, count %d\n", ok, tplId, count);
//         }
//         break;
// 
//         case model::SC::MISC_TURNPLATE_DATA_UPDATE: {
//             int freeCount = pktin.ReadVarInteger<int>();
//             int coinCount = pktin.ReadVarInteger<int>();
// 	    int consume = pktin.ReadVarInteger<int>();
// 	    int mul = pktin.ReadVarInteger<int>();
//             printf("MISC_DICE_UPDATE  .......  freeCount %d, coinCount %d, consume %d, mul %d\n", freeCount,coinCount,consume,mul);
//         }
//         break;
//         case model::SC::MISC_TURNPLATE_DRAW_RESPONSE: {
//             int ok = pktin.ReadBoolean() ? 1 : 0;
//             int gridId = pktin.ReadVarInteger<int>();
//             int type = pktin.ReadVarInteger<int>();
//             int tplId = pktin.ReadVarInteger<int>();
//             int count = pktin.ReadVarInteger<int>();
//             printf("MISC_DICE_RESPONSE  ....... ok %d, gridId %d, type %d, tplId %d, count %d\n", ok, gridId, type, tplId, count);
//         }
//         break;
//         case model::SC::HERO_DRAW_RESPONSE: {
//             int ok = pktin.ReadBoolean() ? 1 : 0;
//             int len = pktin.ReadVarInteger<int>();
//             printf("HERO_DRAW_RESPONSE ......  len %d\n", len);
//             for (int i = 0; i < len; ++i) {
//                 int id = pktin.ReadVarInteger<int>();
//                 int count = pktin.ReadVarInteger<int>();
//                 printf("HERO_DRAW_RESPONSE ...... id %d,  count %d\n",id, count);
//             }
//         }
//         break;
//         case model::SC::SAVE_TEAM_INFO_RESPONSE: {
//             int ok = pktin.ReadBoolean() ? 1 : 0;
//             printf("SAVE_TEAM_INFO_RESPONSE ......  ok %d\n", ok);
//         }
//         break;
//        case model::SC::BAG_SYNTHESIZE_RESPONSE: {
//             int ok = pktin.ReadBoolean() ? 1 : 0;
//             printf("BAG_SYNTHESIZE_RESPONSE ......  ok %d\n", ok);
//         }
//         break;
//        case model::SC::HERO_PUTON_RESPONSE: {
//             int ok = pktin.ReadBoolean() ? 1 : 0;
//             printf("HERO_PUTON_RESPONSE ......  ok %d\n", ok);
//         }
//         break;
//        case model::SC::HERO_TAKEOFF_RESPONSE: {
//             int ok = pktin.ReadBoolean() ? 1 : 0;
//             printf("HERO_TAKEOFF_RESPONSE ......  ok %d\n", ok);
//         }
//         break;        
//        case model::SC::HERO_STAR_UPGRADE_RESPONSE: {
//             int ok = pktin.ReadVarInteger<int>();
//             printf("HERO_STAR_UPGRADE_RESPONSE ......  ok %d\n", ok);
//         }
//         break;
//         case model::SC::HERO_SKILL_UPGRADE_RESPONSE: {
//             int ok = pktin.ReadVarInteger<int>();
//             printf("HERO_SKILL_UPGRADE_RESPONSE ......  ok %d\n", ok);
//         }
//         break;
//         case model::SC::HERO_EQUIP_UPGRADE_RESPONSE: {
//             int ok = pktin.ReadVarInteger<int>();
//             printf("HERO_EQUIP_UPGRADE_RESPONSE ......  ok %d\n", ok);
//         }
//         break;
//         case model::SC::HERO_EQUIP_INLAY_ON_RESPONSE: {
//             int ok = pktin.ReadVarInteger<int>();
//             printf("HERO_EQUIP_INLAY_ON_RESPONSE ......  ok %d\n", ok);
//         }
//         break;
//         case model::SC::HERO_EQUIP_INLAY_OFF_RESPONSE: {
//             int ok = pktin.ReadVarInteger<int>();
//             printf("HERO_EQUIP_INLAY_OFF_RESPONSE ......  ok %d\n", ok);
//         }
//         break;
//         case model::SC::HERO_EQUIP_SUCCINCT_RESPONSE: {
//             int ok = pktin.ReadVarInteger<int>();
//             printf("HERO_EQUIP_SUCCINCT_RESPONSE ......  ok %d\n", ok);
//         }
//         break;  
//        case model::SC::BAG_SELL_RESPONSE: {
//             int ok = pktin.ReadBoolean() ? 1 : 0;
//             int silver = pktin.ReadVarInteger<int>();
//             printf("BAG_SELL_RESPONSE ......  ok %d, silver %d\n", ok, silver);
//         }
//         break;
//         case model::SC::SCENARIO_MOPUP_RESPONSE: {
//             int ok = pktin.ReadVarInteger<int>();
//             int len = pktin.ReadVarInteger<int>();
//             printf("SCENARIO_MOPUP_RESPONSE ......  ok %d, len %d\n", ok, len);
//         }
//         break;
//         case model::SC::SCENARIO_STAR_DRAW_RESPONSE: {
//             int ok = pktin.ReadVarInteger<int>();
//             int len = pktin.ReadVarInteger<int>();
//             printf("SCENARIO_STAR_DRAW_RESPONSE ......  ok %d, len %d\n", ok, len);
//         }
//         break;
//         case model::SC::SCENARIO_FIGHT_RESPONSE: {
//             int ok = pktin.ReadVarInteger<int>();
//             int battleId = pktin.ReadVarInteger<int>();
//             printf("SCENARIO_FIGHT_RESPONSE ......  ok %d, battleId %d\n", ok, battleId);
//         }
//         break;
//         case model::SC::MAP_PERSONAL_INFO_UPDATE: {
//             printf("MAP_PERSONAL_INFO_UPDATE ......  ok\n");
//         }
//         break;
//         default: {
//             cout << "receive unhandled packet=" << pktin.code() << endl;
//         }
//         break;
//     }
}
