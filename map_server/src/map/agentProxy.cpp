#include "agentProxy.h"
#include "mapMgr.h"
#include "mapProxy.h"
#include "unit/unit.h"
#include <model/rpc/map.h>
#include <model/protocol.h>
#include <base/cluster/message.h>
#include <base/data.h>
#include <base/3rd/rapidjson/document.h>
#include <base/3rd/rapidjson/writer.h>
#include <base/3rd/rapidjson/stringbuffer.h>
#include <lua/llimits.h>
#include "unit/castle.h"
#include "unit/resnode.h"
#include "troop.h"
#include "unit/monster.h"
#include "unit/camp.h"
#include "unit/famouscity.h"
#include "unit/catapult.h"
#include "unit/palace.h"
#include "unit/worldboss.h"
#include <model/metadata.h>
#include <base/lua/parser.h>
#include "luawrite.h"
#include "msgqueue/msgrecord.h"
#include "info/agentinfo.h"
#include "alliance.h"
#include "tpl/mapunittpl.h"
#include "tpl/mapcitytpl.h"
#include "tpl/scouttypetpl.h"
#include "qo/commandagentsave.h"
#include "qo/commandunitsave.h"
#include <model/tpl/templateloader.h>
#include <model/tpl/configure.h>
#include <model/tpl/item.h>
#include <model/tpl/army.h>
#include <model/tpl/hero.h>
#include <algorithm>
#include <tuple>
#include <base/logger.h>
#include "base/event.h"
#include "battle/battlemgr.h"
#include "trapset.h"


namespace ms
{
    namespace map
    {
        using namespace std;
        using namespace base;
        using namespace base::cluster;
        using namespace model::rpc;
        using namespace model;
        using namespace battle;
        using namespace msgqueue;
        using namespace info;
        using namespace model::tpl;



        void AgentProxy::HandleMessage(base::cluster::MessageIn& msgin)
        {
//             cout << "HandleMessage code = " << msgin.code() << endl;
            model::rpc::MapCode code = static_cast<model::rpc::MapCode>(msgin.code());
            switch (code) {
                case model::rpc::MapCode::QUIT: {
                    m_agent.Disconnect();
                };
                break;
                case model::rpc::MapCode::LUA_CALL:
                    OnLuaCall(msgin);
                    break;
                case model::rpc::MapCode::LUA_CAST:
                    OnLuaCast(msgin);
                    break;
                case model::rpc::MapCode::FORWARD:
                    OnLuaForward(msgin);
                    break;
                default:
                    return;
            }
        }



        void AgentProxy::OnLuaCall(base::cluster::MessageIn& msgin)
        {

            string func = msgin.ReadString();
//             cout << "LUA_CALL func = " << func << endl;
            base::DataTable table;      //参数列表
            try {
                base::readPkt(msgin, table);
            } catch (const string& err) {
                LOG_ERROR("LUA_CALL err = %s", err.c_str());
                SendMessage((uint16_t)model::rpc::MapCode::LUA_CALL, 32, msgin.session(), false);
                return;
            }
            if (func == "initCastle") {
                m_agent.m_uid = table.Get(1)->ToInteger();
                m_agent.m_nickname = table.Get(2)->ToString();
                m_agent.m_headId = table.Get(3)->ToInteger();
                int level = table.Get(4)->ToInteger();
                m_agent.m_registerTimestamp = table.Get(5)->ToInteger();
                Castle* castle = nullptr;
                if (m_agent.isLocalPlayer()) {
                    castle = g_mapMgr->CreateNewCastle(m_agent, level);
                }

                if (castle) {
                    bool needSync = false;
                    if (!m_agent.m_castle) {
                        //新账号
                        needSync = true;    //ms数据没有入库前需要该字段
                        m_agent.m_castle = castle;
                        m_agent.m_castle->CheckIsNovice();
                    }

                    // 点将台数据
                    DataTable teams;
                    for (auto& armyList : m_agent.m_arrArmyList) {
                        if (armyList.team() ==  0) {
                            continue;
                        }
                        if (armyList.IsOut()) {
                            if (Troop* troop = m_agent.FindTroop(armyList.troopId())) {
                                teams.Set(armyList.team(), (int)troop->type());
                            } else {
                                armyList.setTroopId(0);
                            }
                        }
                    }

                    SendMessage((uint16_t)model::rpc::MapCode::LUA_CALL, 32, msgin.session(), true, castle->x(), castle->y(), castle->burnEndTimestamp(), needSync,  teams);
                    g_mapMgr->OnAgentInitFinished(m_agent.m_uid);
                } else {
                    SendMessage((uint16_t)model::rpc::MapCode::LUA_CALL, 32, msgin.session(), false);
                    return;
                }
                m_agent.SetDirty();
                SendMapPersonalInfoUpdate();
                m_agent.SendRelatedTroops();
                m_agent.SendRelatedUnits();
                SendAllianceReinforcements();
            } else if (func == "march")
            {
                //cout << "March" << endl;
                model::MarchErrorNo errorNo = MarchErrorNo::OTHER;
                if (m_agent.m_castle) {
                    int index = 0;
                    MapTroopType type = (MapTroopType)table.Get(++index)->ToInteger();
                    int toX = table.Get(++index)->ToInteger();
                    int toY = table.Get(++index)->ToInteger();
                    int team = table.Get(++index)->ToInteger();

                    if (team > 0 && team <= 5) {
                        auto& armyList = m_agent.m_arrArmyList.at(team - 1);
                        if (armyList.IsOut()) {
                            if (Troop* troop = m_agent.FindTroop(armyList.troopId())) {
                                if (troop->type() ==  MapTroopType::CAMP_TEMP || troop->type() == MapTroopType::CAMP_FIXED || troop->type() == MapTroopType::SIEGE_CITY || troop->type() == MapTroopType::ASSIGN ) {
                                    errorNo = troop->ReMarch(type, {toX, toY});
                                }
                            }
                        } else {
                            if (armyList.GetArmyTotal() > 0) {
                                errorNo = g_mapMgr->March(m_agent, type, m_agent.m_castle->pos(), {toX, toY}, team);
                            }
                        }
                    } else if (team == 0) {
                        if (type == MapTroopType::SCOUT) {
                            errorNo = g_mapMgr->March(m_agent, type, m_agent.m_castle->pos(), {toX, toY}, team);
                        } else if (type ==  MapTroopType::CAMP_FIXED_DISMANTLE) {
                            // 拆除行营
                            Unit* unit = g_mapMgr->FindUnit( {toX, toY});
                            if (unit) {
                                if (auto campFixed = unit->ToCampFixed()) {
                                    if (campFixed->uid() ==  m_agent.uid() /*&& campFixed->troopId() ==  0*/) {
                                        campFixed->RemoveSelf();
                                        errorNo = MarchErrorNo::SUCCESS;
                                    }
                                }
                            }
                        }
                    }
                    if (errorNo != MarchErrorNo::SUCCESS) {
                        cout << "March error = " << (int)errorNo << " team=" << team <<  " type= " << (int)type << endl;
                    }
                }

                SendMessage((uint16_t)model::rpc::MapCode::LUA_CALL, 32, msgin.session(), errorNo == MarchErrorNo::SUCCESS);
                SendMapMarchResponse(errorNo);
            } else if (func == "randomTeleport") {
                if (m_agent.m_castle) {
                    bool canTeleport = true;
                    if (!m_agent.m_troops.empty()) {
                        SendNoticeMessage(ErrorCode::TELEPORT_ARMY_OUT, 1);
                        canTeleport = false;
                    }

                    if (canTeleport) {
                        int count = 0;
                        while (true) {
                            if (++count > 1000) break;
                            Point pos = g_mapMgr->GetRandomPoint();
                            if (g_mapMgr->Teleport(m_agent.m_castle, pos.x, pos.y)) {
                                //cout <<  "randomTeleport  x = " << pos.x << "  y = " <<  pos.y <<  endl;
                                SendMapPersonalInfoUpdate();
                                SendNoticeMessage(ErrorCode::TELEPORT_SUCCESS, 1);
                                SendMessage((uint16_t)model::rpc::MapCode::LUA_CALL, 32, msgin.session(), true, pos.x, pos.y);
                                return;
                            }
                        }
                    }
                }
                SendMessage((uint16_t)model::rpc::MapCode::LUA_CALL, 32, msgin.session(), false);
            } else if (func == "advancedTeleport") {
                //cout << "advancedTeleport troop.size = " << m_troops.size() << endl;
                if (m_agent.m_castle) {
                    bool canTeleport = true;
//                         SendNoticeMessage(ErrorCode::TELEPORT_ARMY_OUT, 1);
//                         canTeleport = false;
                    m_agent.ImmediateReturnAllTroop();

                    if (canTeleport) {
                        int toX = table.Get(1)->ToInteger();
                        int toY = table.Get(2)->ToInteger();
                        
                        int i = 10;
                        do {
                            if (g_mapMgr->Teleport(m_agent.m_castle, toX, toY)) {
                                //cout <<  "advancedTeleport  x = " << toX << "  y = " <<  toY <<  endl;
                                SendMapPersonalInfoUpdate();
                                SendNoticeMessage(ErrorCode::TELEPORT_SUCCESS, 1);
                                //--test
                                //m_castle->Burn();
                                SendMessage((uint16_t)model::rpc::MapCode::LUA_CALL, 32, msgin.session(), true, m_agent.m_castle->burnEndTimestamp());
                                return;
                            } else {

                            }
                        } while (--i > 0);
                    }
                }
                SendMessage((uint16_t)model::rpc::MapCode::LUA_CALL, 32, msgin.session(), false);
            } else if (func == "getPlayerInfo") {
                int64_t uid = table.Get(1)->ToInteger();
                if (Agent* agt = g_mapMgr->FindAgentByUID(uid)) {
                    DataTable info;
                    info.Set("uid", agt->uid());
                    info.Set("nickname", agt->nickname());
                    info.Set("headId", agt->headId());
                    info.Set("langType", (int)agt->langType());
                    info.Set("level", agt->level());
                    info.Set("aid", agt->allianceId());
                    info.Set("allianceLevel", agt->allianceId());
                    info.Set("allianceName", agt->allianceName());
                    info.Set("allianceNickname", agt->allianceNickname());
                    info.Set("vipLevel", agt->vipLevel());
                    info.Set("allianceCdTimestamp", agt->allianceCdTimestamp());
                    info.Set("totalPower", agt->totalPower());
                    SendMessage((uint16_t)model::rpc::MapCode::LUA_CALL, 32, msgin.session(), true, info);
                } else {
                    SendMessage((uint16_t)model::rpc::MapCode::LUA_CALL, 32, msgin.session(), false);
                }
            }  else if (func == "wallOutfire") {
                if (m_agent.m_castle && m_agent.m_castle->OutFire()) {
                    SendMessage((uint16_t)model::rpc::MapCode::LUA_CALL, 32, msgin.session(), true);
                } else {
                    SendMessage((uint16_t)model::rpc::MapCode::LUA_CALL, 32, msgin.session(), false);
                }
            }   else if (func == "wallRepair") {
                bool isGoldRepair = table.Get(1)->ToBoolean();
                if (m_agent.m_castle && m_agent.m_castle->Repair(isGoldRepair)) {
                    SendMessage((uint16_t)model::rpc::MapCode::LUA_CALL, 32, msgin.session(), true);
                } else {
                    SendMessage((uint16_t)model::rpc::MapCode::LUA_CALL, 32, msgin.session(), false);
                }
            }   else if (func == "kingChangeServerName") {
                const string& name = table.Get(1)->ToString();
                cout << "name = " << name << endl;
                bool result = false;
                if (m_agent.isKing()) {
                    result = g_mapMgr->ChangeServerName(name);
                } else {
                    cout << "kingChangeServerName is not king" << endl;
                }
                SendMessage((uint16_t)model::rpc::MapCode::LUA_CALL, 32, msgin.session(), result);
            }   else if (func == "kingChangeResNodeRate") {
                int id = table.Get(1)->ToInteger();
                cout << "ResNodeRate is : " << id << endl;
                bool result = false;
                if (m_agent.isKing()) {
                    result = g_mapMgr->ChangeResNodeRate(id);
                } else {
                    cout << "kingChangeResNodeRate is not king" << endl;
                }
                SendMessage((uint16_t)model::rpc::MapCode::LUA_CALL, 32, msgin.session(), result);
            } else if (func == "recallAllianceTroop") {
                int troopId = table.Get(1)->ToInteger();
                auto troop = g_mapMgr->FindTroop(troopId);
                bool ret = false;
                if (troop)
                {
                    if (m_agent.m_castle->RemoveReachReinforcement(troop))
                    {
                        troop->GoHome();
                        ret = true;
                    }
                    /*SendAllianceReinforcements();*/
                }
                SendMessage((uint16_t)model::rpc::MapCode::LUA_CALL, 32, msgin.session(), ret);
            } else if (func == "transport") {
                int64_t toUid = table.Get(1)->ToInteger();
                Agent* toAgent = g_mapMgr->FindAgentByUID(toUid);
                do
                {
                    // 如果同盟不相等
                    if (toAgent && toAgent->allianceId() == m_agent.allianceId())
                        break;
                    SendMessage((uint16_t)model::rpc::MapCode::LUA_CALL, 32, msgin.session(), false);
                    return;
                }while(0);
                DataTable& resourceParams = table.Get(2)->ToTable();
                bool result = g_mapMgr->OnTransport(&m_agent, toAgent, model::MapTroopType::TRANSPORT, m_agent.m_castle->pos(), toAgent->castle()->pos(), resourceParams);
                SendMessage((uint16_t)model::rpc::MapCode::LUA_CALL, 32, msgin.session(), result);
            }

        }

        void AgentProxy::OnLuaCast(base::cluster::MessageIn& msgin)
        {
            string func = msgin.ReadString();

            base::DataTable table;  //参数列表
            try {
                base::readPkt(msgin, table);
            } catch (const string& err) {
                cout << err << endl;
                return;
            }

            if (func == "onOffline") {
                g_mapMgr->ViewerLeave(&m_agent);
                m_agent.Disconnect();
                int mid = m_agent.m_uid >> 32;
                if (mid != g_mapMgr->localServiceId()) {
                    LOG_DEBUG("orange delete this? mid = %d, m_uid = %ld, localServiceId = %d", mid, m_agent.m_uid, g_mapMgr->localServiceId());
                    g_mapMgr->VisitorLeave(m_agent.m_uid);
                    delete this;
                }
            } else if (func == "onCastleLevelUp") {
                int level = table.Get(1)->ToInteger();
                if (m_agent.m_castle) {
                    g_mapMgr->OnCastleLevelUp(m_agent.m_castle, level);
                }
            } else if (func == "syncPlayerInfo") {
                m_agent.m_nickname = table.Get(1)->ToString();
                m_agent.m_headId = table.Get(2)->ToInteger();
                m_agent.m_level = table.Get(3)->ToInteger();
                m_agent.m_lastLoginTimestamp = table.Get(4)->ToInteger();
                m_agent.m_langType = (model::LangType)table.Get(5)->ToInteger();
                m_agent.m_lordPower = table.Get(6)->ToInteger();
                m_agent.m_troopPower = table.Get(7)->ToInteger();
                m_agent.m_buildingPower = table.Get(8)->ToInteger();
                m_agent.m_sciencePower = table.Get(9)->ToInteger();
                m_agent.m_trapPower = table.Get(10)->ToInteger();
                m_agent.m_heroPower = table.Get(11)->ToInteger();
                m_agent.m_totalPower = table.Get(12)->ToInteger();
                m_agent.m_exp = table.Get(13)->ToInteger();
                m_agent.m_allianceCdTimestamp = table.Get(14)->ToInteger();

                if (m_agent.m_castle) {
                    m_agent.m_castle->NoticeUnitUpdate();
                }
                m_agent.SetDirty();
            } else if (func == "syncAllResource") {
                m_agent.m_food = table.Get(1)->ToInteger();
                m_agent.m_wood = table.Get(2)->ToInteger();
                m_agent.m_iron = table.Get(3)->ToInteger();
                m_agent.m_stone = table.Get(4)->ToInteger();
                m_agent.m_gold = table.Get(5)->ToInteger();
                m_agent.SetDirty();
            } else if (func == "syncLordInfo") {
                m_agent.m_attackWins = table.Get(1)->ToInteger();
                m_agent.m_attackLosses = table.Get(2)->ToInteger();
                m_agent.m_defenseWins = table.Get(3)->ToInteger();
                m_agent.m_defenseLosses = table.Get(4)->ToInteger();
                m_agent.m_scoutCount = table.Get(5)->ToInteger();
                m_agent.m_kills = table.Get(6)->ToInteger();
                m_agent.m_losses = table.Get(7)->ToInteger();
                m_agent.m_heals = table.Get(8)->ToInteger();
                m_agent.SetDirty();
                //cout << "DEBUG AGENT LORDINFO attackWins = " << m_attackWins << " attackLosses = " << m_attackLosses << " defenseWins = " << m_defenseWins << " defenseLosses = " << m_defenseLosses << " scoutCount = " << m_scoutCount << " kills = " << m_kills << " losses = " << m_losses << " heals = " << m_heals << endl;
            } else if (func == "recall") {
                int troopId = table.Get(1)->ToInteger();
                //cout << "recall troopId = " << troopId << endl;
                if (Troop* troop = m_agent.FindTroop(troopId)) {
                    troop->Recall();
                }
            } else if (func == "speedUp") {
                int troopId = table.Get(1)->ToInteger();
                float speedUp = table.Get(2)->ToDouble();
                // cout << "speedUp troopId = " << troopId << " speedUp = " << speedUp << endl;
                if (Troop* troop = m_agent.FindTroop(troopId)) {
                    // 运输不能加速
                    if (troop->type() == MapTroopType::TRANSPORT)
                        return;
                    troop->SpeedUp(speedUp);
                }
            } else if (func == "confirmMsg") {
                int msgId =  table.Get(1)->ToInteger();
                m_agent.m_msgQueue.ConfirmMsg(msgId);
            } else if (func == "syncCastleTeam") {
                int castleDefenderId = table.Get(1)->ToInteger();
                m_agent.m_castleDefenderId = castleDefenderId; 
            } else if (func == "syncTeam") {
                const DataTable& teams = table.Get(1)->ToTable();
                // teams.DumpInfo();   
                teams.ForeachAll([&](const DataValue & k, const DataValue & v) {
                    int teamId = k.ToInteger();
                    if (teamId < 1 || teamId > 5) {
                        return false;
                    }
                    auto& armyList = m_agent.m_arrArmyList[teamId - 1];
                    Troop* troop = nullptr;
                    if (armyList.IsOut()) {
                        bool canSync = false;
                        troop = m_agent.FindTroop(armyList.troopId());
                        if (troop) {
                            if ((troop->type() ==  MapTroopType::CAMP_FIXED || troop->type() ==  MapTroopType::CAMP_TEMP || troop->type() ==  MapTroopType::SIEGE_CITY) && troop->IsReach()) {
                                canSync = true;
                            }
                        } else  {
                            // 以防行军数据丢失
                            armyList.setTroopId(0);
                            canSync = true;
                        }

                        if (!canSync) {
                            return false;
                        }
                    }
                    armyList.ClearAll();
                    armyList.SetTeam(teamId);

                    const DataTable& tableArmyList = v.ToTable();
                    tableArmyList.ForeachMap([&](const DataValue & k2, const DataValue & v2) {
                        int heroId = k2.ToInteger();
                        const DataTable& tableArmy = v2.ToTable();
                        int armyType = tableArmy.Get("armyType")->ToInteger();
                        int armyLevel = tableArmy.Get("level")->ToInteger();
                        int count = tableArmy.Get("count")->ToInteger();
                        int position = tableArmy.Get("position")->ToInteger();

                        auto it = m_agent.m_heroes.find(heroId);
                        if (it != m_agent.m_heroes.end()) {
                            HeroInfo& heroInfo = *it->second;
                            auto armyGroup = armyList.AddArmyGroup(armyType, armyLevel, heroInfo, position);
                            if (armyGroup) {
                                armyGroup->armyInfo().Add(ArmyState::NORMAL, count);
                            }
                        }
                        return false;
                    });
                    m_agent.m_castle->NoticeUnitUpdate();
                    armyList.OnPropertyUpdate(m_agent.m_property);

                    if (troop) {
                        troop->NoticeTroopUpdate();
                    }
                    return false;
                });
            }  else if (func == "syncHero") {
                const DataTable& heroes = table.Get(1)->ToTable();
                //heroes.DumpInfo();
                heroes.ForeachAll([&](const DataValue & k, const DataValue & v) {
                    const DataTable& tableHero = v.ToTable();
                    int heroId =tableHero.Get("id")->ToInteger();
                    int level = tableHero.Get("level")->ToInteger();
                    int star = tableHero.Get("star")->ToInteger();
                    int physical = tableHero.Get("physical")->ToInteger();     //武将体力，也就是武将生命值

                    std::vector<HeroSkill> skill;
                    std::vector<HeroEquip> equipment;

                    const auto& skillValue = tableHero.Get("skill");
                    if (skillValue->IsTable()) {
                        const DataTable& tableSkill  = skillValue->ToTable();
                        tableSkill.ForeachAll([&](const DataValue & k, const DataValue & vSkill) {
                            if (vSkill.IsTable()) {
                                auto& skillTable = vSkill.ToTable();
                                int tplId = skillTable.Get("tplId")->ToInteger();
                                int level = skillTable.Get("level")->ToInteger();
                                skill.emplace_back(tplId, level);
                            }
                            return false;
                        });
                    } else {
                        cout << "skillValue->IsTable() false heroid " << heroId << endl;
                    }

                    std::shared_ptr<HeroInfo> heroInfo;
                    auto it = m_agent.m_heroes.find(heroId);
                    if (it !=  m_agent.m_heroes.end()) {
                        heroInfo = it->second;
                        heroInfo->SetLevel(level);
                        heroInfo->SetStar(star);
                        heroInfo->SetPhysical(physical);
                    } else {
                        heroInfo =  std::make_shared<HeroInfo>(heroId, level, star);
                        m_agent.m_heroes.emplace(heroId, heroInfo);
                        g_mapMgr->OnAddHero(heroId);
                    }

                    heroInfo->SetSkill(skill);

                    const auto& additionListValue = tableHero.Get("additionList");
                    if (additionListValue->IsTable()) {
                        const DataTable& tableAdditionList  = additionListValue->ToTable();
                        tableAdditionList.ForeachAll([&](const DataValue & k, const DataValue & vAttr) {
                            if (vAttr.IsTable()) {
                                model::AttributeType attrType = static_cast<model::AttributeType>(k.ToInteger());
                                auto& attrTable = vAttr.ToTable();
                                // 5018和5019为int型
                                if (attrType == model::AttributeType::HERO_FIGHT || attrType == model::AttributeType::HERO_SINGLE_ENERGY)
                                {
                                    int base = attrTable.Get((int)model::AttributeAdditionType::BASE)->ToInteger();
                                    int pct = attrTable.Get((int)model::AttributeAdditionType::PCT)->ToInteger();
                                    int ext = attrTable.Get((int)model::AttributeAdditionType::EXT)->ToInteger();
                                    heroInfo->SetProp(attrType, (float)base, (float)pct, (float)ext);

                                    // std::cout << "attrType = " << (int)attrType << std::endl;

                                    // std::cout << "base = " <<  base << " pct = " << pct << " ext = " <<  ext << std::endl;

                                }
                                else
                                {
                                    float base = attrTable.Get((int)model::AttributeAdditionType::BASE)->ToDouble();
                                    float pct = attrTable.Get((int)model::AttributeAdditionType::PCT)->ToDouble();
                                    float ext = attrTable.Get((int)model::AttributeAdditionType::EXT)->ToDouble();
                                    heroInfo->SetProp(attrType, base, pct, ext);
                                    // std::cout << "attrType = " << (int)attrType << std::endl;

                                    // std::cout << "base = " <<  base << " pct = " << pct << " ext = " <<  ext << std::endl;
                                }
                            }
                            return false;
                        });
                    }

                    // 更新点将台数据
                    for (auto& armyList : m_agent.m_arrArmyList) {
                        if (armyList.team() < 1) {
                            continue;
                        }
                        ArmyGroup* armyGroup = armyList.GetArmyGroup(heroId);
                        if (armyGroup) {
                            armyGroup->updateHeroInfo(*heroInfo);
                            break;
                        }
                    }
                    return false;
                });
            } else if (func == "syncArmy") {
                m_agent.m_trapSets.ClearAll();
                m_agent.m_armySet.ClearAll();

                const DataTable& armies = table.Get(1)->ToTable();
                armies.ForeachAll([&](const DataValue & k, const DataValue & v) {
                    auto& armyV = v.ToTable();
                    int armyType = armyV.Get("armyType")->ToInteger();
                    int armyLevel =  armyV.Get("level")->ToInteger();

                    auto& statesV = armyV.Get("states")->ToTable();
                    statesV.ForeachAll([&](const DataValue & k, const DataValue & v) {
                        model::ArmyState state = static_cast<model::ArmyState>(k.ToInteger());
                        int count = v.ToInteger();

                        if (armyType == (int)model::ArmysType::ROLLING_LOGS ||  armyType == (int)model::ArmysType::GRIND_STONE || armyType == (int)model::ArmysType::CHEVAL_DE_FRISE || armyType == (int)model::ArmysType::KEROSENE) {
                            m_agent.m_trapSets.AddTrap(armyType, state,  count);
                        } else {
                            m_agent.m_armySet.AddArmy(armyType,  armyLevel,  state,  count);
                        }
                        return false;
                    });
                    return false;
                });
            } else if (func == "syncCollectInfo") {
                // 城里采集信息
                m_agent.m_collectInfos.clear();
                const DataTable& infos = table.Get(1)->ToTable();
                infos.ForeachMap([&](const DataValue & k, const DataValue & v) {
                    CollectInfo info;
                    const DataTable& collectTable = v.ToTable();
                    info.gridId = k.ToInteger();
                    info.type = (ResourceType)collectTable.Get("type")->ToInteger();
                    info.output = collectTable.Get("output")->ToInteger();
                    info.capacity = collectTable.Get("capacity")->ToInteger();
                    info.timestamp = collectTable.Get("timestamp")->ToInteger();
                    m_agent.m_collectInfos.push_back(info);
                    return false;
                });
                m_agent.SetDirty();
            } else if (func == "syncWatchtower") {
                m_agent.m_watchtowerLevel = table.Get(1)->ToInteger();
                m_agent.SetDirty();
            }  else if (func == "syncTurretInfo") {
                m_agent.m_turretLevel = table.Get(1)->ToInteger();
                m_agent.SetDirty();
            } else if (func == "syncProperty") {
                const DataTable& list = table.Get(1)->ToTable();
                m_agent.m_property.ReadDataTable(list, m_agent);
                
                for (auto it = m_agent.m_troops.begin(); it != m_agent.m_troops.end(); ++it) {
                    it->second->OnPropertyChanged();
                }
                m_agent.SetDirty();
            }  else if (func == "syncBuffList") { // 各种增益
                int64_t now = g_dispatcher->GetTimestampCache();
                m_agent.m_cityBuffs.clear();
                const DataTable& buffsTable = table.Get(1)->ToTable();
                buffsTable.ForeachVector([&](int k, const DataValue & v) {
                    BuffInfo info;
                    const DataTable& buffTable = v.ToTable();
                    int type = buffTable.Get("type")->ToInteger();
                    info.type = static_cast<model::BuffType>(type);
                    info.endTimestamp = buffTable.Get("endTimestamp")->ToInteger();
                    info.param1 = buffTable.Get("param1")->ToInteger();
                    info.attr = buffTable.Get("attr")->ToTable();
                    m_agent.m_cityBuffs.emplace(type, info);
                    //handle buff
                    if (now < info.endTimestamp) {
                        switch ((BuffType)type) {
                            case BuffType::PEACE_SHIELD: {
                                if (m_agent.m_castle) {
                                    m_agent.m_castle->CheckPeaceShield();
//                                     m_castle->PeaceShield(info.endTimestamp - now);
                                }
                            }
                            break;
                            default:
                                break;
                        }
                    }
                    return false;
                });

                m_agent.m_skillBuffs.clear();
                const DataTable& skillBuffsTable = table.Get(2)->ToTable();
                skillBuffsTable.ForeachVector([&](int k, const DataValue & v) {
                    BuffInfo info;
                    const DataTable& buffTable = v.ToTable();
                    int type = buffTable.Get("type")->ToInteger();
                    info.endTimestamp = buffTable.Get("endTimestamp")->ToInteger();
                    info.param1 = buffTable.Get("param1")->ToInteger();
                    info.attr = buffTable.Get("attr")->ToTable();
                    m_agent.m_skillBuffs.emplace(type, info);
                    return false;
                });
                m_agent.SetDirty();
            } else if (func == "syncTechnologies") {
                m_agent.m_technologies.clear();
                const DataTable& list = table.Get(1)->ToTable();
                list.ForeachAll([this](const DataValue & k, const DataValue & v) {
                    m_agent.m_technologies.emplace(k.ToInteger(), v.ToInteger());
                    return false;
                });
                m_agent.SetDirty();
            } else if (func == "syncVip") {
                m_agent.m_vip.level = table.Get(1)->ToInteger();
                m_agent.SetDirty();
            } else if (func == "syncEmployHero") {
            
            } else if (func == "queryPlayerInfo") {
                // 查看玩家信息
                std::vector<RankInfo> vecRank;
                int64_t uid = table.Get(1)->ToInteger();
//                 cout << "queryPlayerInfo...uid" << uid << endl;
                if (Agent* agent = g_mapMgr->FindAgentByUID(uid)) {
                    const DataTable& infos = table.Get(2)->ToTable();
                    infos.ForeachAll([&](const DataValue & k, const DataValue & v) {
                        RankInfo info;
                        info.type = (RangeType)k.ToInteger();
                        info.rank = v.ToInteger();
//                         cout << "queryPlayerInfo...type=" << (int)info.type << ", rank=" << info.rank << endl;
                        vecRank.push_back(info);
                        return false;
                    });
                    SendMapQueryPlayerInfoResponse(agent, vecRank);
                }
            } else if (func == "updateTroops") {
                for (auto value :  m_agent.m_troops) {
                    auto troop = value.second;
                    if (troop) {
                        troop->NoticeTroopUpdate();
                    }
                }
            } else if (func == "updateMyUnits") {
                for (auto unit :  m_agent.m_campFixed) {
                    if (unit) {
                        unit->NoticeUnitUpdate();
                    }
                }
                for (auto unit :  m_agent.m_campTemp) {
                    if (unit) {
                        unit->NoticeUnitUpdate();
                    }
                }
                for (auto unit :  m_agent.m_relatedUnit) {
                    if (unit) {
                        unit->NoticeUnitUpdate();
                    }
                }
            } else if (func == "sendCanMarchMonsterLevelUpdate") 
            {
                    SendCanMarchMonsterLevelUpdate();
            }



        }

        void AgentProxy::OnLuaForward(base::cluster::MessageIn& msgin)
        {
            model::CS code = static_cast<model::CS>(msgin.ReadVarInteger<int>());
            //cout << "OnLuaForward code = "  << (int)code << endl;
            switch (code) {
                case model::CS::MAP_JOIN: {
                    /*int k = */msgin.ReadVarInteger<int>();
                    int x = msgin.ReadVarInteger<int>();
                    int y = msgin.ReadVarInteger<int>();
                    g_mapMgr->ViewerJoin(&m_agent, x, y);
                }
                break;
                case model::CS::MAP_LEAVE: {
//                     cout << "MAP_LEAVE" << endl;
                    g_mapMgr->ViewerLeave(&m_agent);
                    if (!m_agent.isLocalPlayer()) {
                        g_mapMgr->VisitorLeave(m_agent.m_uid);
                        m_agent.Disconnect();
                        delete &m_agent;
                        return;
                    }
                }
                break;
                case model::CS::MAP_VIEW: {
                    int x = msgin.ReadVarInteger<int>();
                    int y = msgin.ReadVarInteger<int>();
                    // cout << "MAP_VIEW " << x << " " << y  << endl;
                    g_mapMgr->ViewerMove(&m_agent, x, y);
                }
                break;
                case model::CS::MAP_GO_HOME: {
                    int troopId = msgin.ReadVarInteger<int>();
                    bool isMine = false;
                    Troop* troop = m_agent.FindTroop(troopId);
                    if (troop) {
                        isMine = true;
                        if (troop->IsReach()) {
                            troop->GoHome();
                        }
                    }

                    if (!isMine) {
                        if (m_agent.m_alliance && m_agent.m_alliance->info.leaderId ==  m_agent.uid()) {
                            troop = g_mapMgr->FindTroop(troopId);
                            if (troop) {
                                if (troop->Repatriate()) {
                                    std::string param(troop->nickname());
                                    auto unit = g_mapMgr->FindUnit(troop->from());
                                    if (unit) {
                                        auto city = unit->ToFamousCity();
                                        if (city) {
                                            param.append(",");
                                            param.append(city->cityTpl()->name);
                                        }
                                    }
                                    g_mapMgr->m_mapProxy->SendcsAddAllianceRecord(m_agent.m_alliance->info.id, model::AllianceMessageType::TROOP_BE_REPATRIATED, param);
                                }
                            }
                        }
                    }
                }
                break;
                case model::CS::MAP_ALLIANCE_INVITE_LIST: {
                        int index = msgin.ReadVarInteger<int>();
                        int count = msgin.ReadVarInteger<int>();
                        cout << " index = " << index << " count = " << count << endl;

                        std::vector<Agent*> list;
                        int i = 0;
                        int64_t time = g_dispatcher->GetTimestampCache() - 24 * 3600;
                        for (auto it = g_mapMgr->agents().begin(); it != g_mapMgr->agents().end(); ++it) {
                            Agent* agt = it->second;
                            //cout << "aid = " << agt->allianceId() << " ltype = " << (int)agt->m_langType << " ts = " << agt->m_lastLoginTimestamp << " time = " << time << " hehe = " << HadInvited(agt->uid()) << endl;
                            if (agt->allianceId() == 0 && agt->m_lastLoginTimestamp > time && !m_agent.HadInvited(agt->uid())) {
                                if (++i > index) {
                                    if (count > 0) {
                                        list.push_back(agt);
                                        --count;
                                    } else {
                                        break;
                                    }
                                }
                            }
                        }
                        SendMapAllianceInviteListResponse(list);
                }
                break;
                case model::CS::MAP_ALLIANCE_INVITE_LIST_BY_NAME: {
                        string str = msgin.ReadString();
                        cout << "MAP_ALLIANCE_INVITE_LIST_BY_NAME str = " << str << endl;
                        if (str.size() < 3 || str.size() > 20) {
                            return;
                        }
                        transform(str.begin(), str.end(), str.begin(), ::toupper);
                        std::vector<Agent*> list;
                        for (auto it = g_mapMgr->agents().begin(); it != g_mapMgr->agents().end(); ++it) {
                            Agent* agt = it->second;
                            if (!agt->m_alliance && !m_agent.HadInvited(agt->uid())) {
                                string nickname = agt->nickname();
                                transform(nickname.begin(), nickname.end(), nickname.begin(), ::toupper);
                                if (nickname.find(str) != string::npos) {
                                    list.push_back(agt);
                                }
                            }
                        }
                        SendMapAllianceInviteListResponse(list);
                }
                break;
                case model::CS::MAP_GET_TROOP_EXTRA_INFO: {
                    std::list<Troop*> troops;
                    int len = msgin.ReadVarInteger<int>(); // max 100
                    for (int i = 0; i < len; ++i) {
                        int troopId = msgin.ReadVarInteger<int>();
                        if (Troop* tp = g_mapMgr->FindTroop(troopId)) {
                            troops.push_back(tp);
                        }   
                    }
                    SendGetTroopExtraInfoResponse(troops);
                }
                break;
                case model::CS::KING_GET_KINGDOM_SET_INFOS:
                {
                    SendKingGetKingdomSetInfosResponse();
                }
                break;
                case model::CS::PALACE_WAR_SUCCESSIVE_KINGS: {
                            SendPalaceWarSuccessiveKingsResponse();
                        }
                break;
                case model::CS::PALACE_WAR_RECORDS: {
                    int x = msgin.ReadVarInteger<int>();
                    int y = msgin.ReadVarInteger<int>();
                    if (Unit* unit = g_mapMgr->FindUnit( {x, y})) {
                        if (const Capital* plc = unit->ToCapital()) {
                            SendPalaceWarRecordsResponse(x, y, plc->palaceWarRecords());
                        } else if (const Catapult* cp = unit->ToCatapult()) {
                            SendPalaceWarRecordsResponse(x, y, cp->palaceWarRecords());
                        }
                    }
                }
                break;
                case model::CS::GET_ALLIANCE_CITIES: {
                    SendAllianceCitiesInfoUpdate();
                }
                case model::CS::MAP_SEARCH: {
                    int type = msgin.ReadVarInteger<int>();
                    int level = msgin.ReadVarInteger<int>();
                    // cout << "type" << type << endl;
                    // cout << "level"<< level << endl;
                    if (m_agent.m_castle)
                    {
                        g_mapMgr->SearchMap(&m_agent, m_agent.m_castle->x(), m_agent.m_castle->y(), type,level);
                    }
                }
                break;
                default:
                    return;
            }
        }


        void AgentProxy::SendMapUnitUpdate(const std::vector< Unit* >& units, const std::vector<int>& removeList, bool isOnViewMove)
        {
            if (m_agent.m_mbid) {
                std::vector<Unit*> unitsSt;
                for (Unit * unit : units) {
                    bool isSent = m_agent.m_unitIdCache.emplace(unit->id()).second == false;
                    if (isOnViewMove) {
                        if (!isSent) {
                            unitsSt.push_back(unit);
                        }
                    } else {
                        unitsSt.push_back(unit);
                    }
                }

                if (unitsSt.empty() && removeList.empty()) {
                    return;
                }

                //cout << "agent SendMapUnitUpdate size = " << unitsSt.size() << " remove size = " << removeList.size() << endl;
                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, 64, g_mapMgr->mempool());
                msgout << (int)model::SC::MAP_UNIT_UPDATE;
                msgout << g_mapMgr->localServiceId();
                msgout << unitsSt.size();

                auto outArmyList = [&](ArmyList* armylist) {
                    if (armylist) {
                        auto& armies = armylist->armies();
                        msgout << armylist->ActiveSize();
                        for (auto & value : armies) {
                            auto& armyGroup = value.second;
                            int normalCnt =  armyGroup.armyInfo().count(ArmyState::NORMAL);
                            if (normalCnt > 0) {
                                msgout << armyGroup.heroInfo().tplId() << armyGroup.armyInfo().tplid();
                                msgout << normalCnt << armyGroup.position();
                            }
                        }
                    } else {
                        msgout << 0; //armylist.size();
                    }
                };

                auto outCastleArmyList = [&](ArmyList* armylist) {
                    if (armylist) {
                        auto& armies = armylist->armies();
                        msgout << armylist->size();
                        for (auto & value : armies) {
                            auto& armyGroup = value.second;
                            int normalCnt =  armyGroup.armyInfo().count(ArmyState::NORMAL);
                            msgout << armyGroup.heroInfo().tplId() << armyGroup.armyInfo().tplid();
                            msgout << normalCnt << armyGroup.position();
                        }
                    } else {
                        msgout << 0; //armylist.size();
                    }
                };

                auto outAlliance = [&](int64_t allianceId) {
                    if (allianceId > 0) {
                        string allianceName = "";
                        int allianceBannerId = 0;
                        if (AllianceSimple* as = g_mapMgr->alliance().FindAllianceSimple(allianceId)) {
                            allianceName = as->info.name;
                            //LOG_DEBUG("outAlliance find --------  %d %s ", allianceId, allianceName.c_str());
                            allianceBannerId = as->info.bannerId;
                        }
                        //LOG_DEBUG("orange delete this? mid = %d, m_uid = %ld, localServiceId = %d", mid, m_uid, g_mapMgr->localServiceId());
                        //LOG_DEBUG("outAlliance  %d %s ", allianceId, allianceName.c_str());
                        msgout << allianceId << allianceName << allianceBannerId;
                    } else {
                        msgout << allianceId << "" << 0;
                    }
                };

                for (Unit * unit : unitsSt) {
                    msgout << unit->id() << unit->x() << unit->y() << unit->tpl().id;
                    // int level = m_tploader->FindMapUnit(unit->tpl().id)->level;
                    // cout << "id = " << unit->id() << " type = " << (int)unit->type() << " x = " << unit->x() << " y = " << unit->y()  << " tplId =" << unit->tpl().id <<  " level = " << level << endl;

                    ArmyList* armylist = nullptr;
                    switch (unit->type()) {
                        case MapUnitType::CASTLE: {
                            //cout << "CASTLE:" << endl;
                            Castle* castle = unit->ToCastle();
                            msgout << castle->uid() << 0 << castle->nickname() << castle->level() << (int)castle->state();
                            //cout << "allianceNickname=" << castle->allianceNickname() << " allianceBannerId=" << castle->allianceBannerId() << " nickname=" << castle->nickname() << endl;
                            msgout << 0 << 0 << (float)0 << 0 << 0;
                            //cout << "titleId=" << castle->titleId() << endl;
                            outArmyList(armylist);
                            msgout << castle->cityDefense() <<  castle->cityDefenseMax();
                            outAlliance(castle->allianceId());
                            msgout << castle->troops().size();
                            for (auto troop :  castle->troops()) {
                                if (troop) {
                                    msgout << troop->uid() <<  troop->id();
                                    cout << "###uid, troopId" << castle->uid() << "," << troop->id() << endl;
                                }
                            }
                            msgout << 0;
                            outAlliance(0);
                            msgout << 0;
                            int size = 0;
                            for (int i = 0; i < (int)castle->armyListArray().size(); ++i)
                            {
                                armylist = const_cast<ArmyList*>(&castle->armyListArray()[i]);
                                // 外出以及没有兵力的都不显示
                                if (armylist->size() > 0 && !armylist->IsOut())
                                {
                                    size++;
                                }
                            }
                            msgout << size;
                            for (int i = 0; i < (int)castle->armyListArray().size(); ++i)
                            {
                                // outArmyList(castle->DefArmyList());
                                armylist = &castle->armyListArray()[i];
                                // cout << "#########################################" << endl;
                                // cout << "index" << i << endl;
                                if (armylist->size() > 0 && !armylist->IsOut())
                                {
                                    outCastleArmyList(armylist);
                                }
                            }
                        }
                        break;
                        case MapUnitType::FARM_FOOD:
                        case MapUnitType::FARM_WOOD:
                        case MapUnitType::MINE_IRON:
                        case MapUnitType::MINE_STONE: {
                            ResNode* res = unit->ToResNode();
                            msgout << res->uid() << res->noviceuid() <<  "" << 0 << 0;
                            msgout << res->leftGather() << res->troopId() << res->troopGatherSpeed() << res->canGather() << res->Capacity() - res->canGather();
//                             cout << "resNode : " << res->leftGather() << " " << res->troopId() << " " << res->gatherSpeed() << " " << res->canGather() << " hasGather = " << res->hasGather() + res->hadGather() << endl;
                            armylist = &res->armyList();
                            outArmyList(armylist);
                            msgout << 0 <<  0;
                            outAlliance(res->allianceId());
                            if (res->troopId() != 0)
                            {
                                msgout << 1;
                                msgout << res->uid() <<  res->troopId();
                            }
                            else
                            {
                                msgout << 0;
                            }
                            
                            msgout << 0;
                            outAlliance(0);
                            msgout << 0;
                            msgout << 0;
                        }
                        break;
                        case MapUnitType::MONSTER: {
                            Monster* monster = unit->ToMonster();
                            monster->tpl().name;
                            msgout << 0 << 0 <<  "野怪" << 0 << 0;
                            msgout << 0 << 0 << (float)0 << 0 << 0;
                            armylist = &monster->armyList();
                            outArmyList(armylist);
                            msgout << 0 <<  0;
                            outAlliance(0);
                            msgout << 0;
                            msgout << 0;
                            outAlliance(0);
                            msgout << 0;
                            msgout << 0;
                        }
                        break;
                        case MapUnitType::CAPITAL:
                        {
                            Capital* capital = unit->ToCapital();
                            int leftTime = 0;
                            int chooseLeftTime = 0;
                            int headId = 0;
                            string nickname = "";
                            //第一次王城争霸就发空，后面的就发玩家改了的名字。 通过国王记录来判断
                            if (capital->successiveKings().size() != 0)
                            {
                                nickname = g_mapMgr->serverName();
                            }
                            if (capital->isNotStart()) {
                                leftTime = capital->startLeftTime();
                                if (const Agent* king = g_mapMgr->king()) {
                                    headId = king->headId();
                                } else {
                                    chooseLeftTime = capital->chooseLeftTime();
                                }
                            } else {
                                if (capital->occupierId() == 0) {
                                    leftTime = capital->NpcRecoverLeftTime();
                                } else {
                                    leftTime = capital->endLeftTime();
                                    if (const Agent* agt = capital->occupier()) {
                                        headId = agt->headId();
                                    }
                                }
                            }
                            msgout << headId << 0 << nickname << 0 << (int)capital->state() << 0 << capital->troopId() << float(0) << leftTime << 0;
                            armylist = capital->DefArmyList();
                            cout << g_mapMgr->serverName().c_str() << endl;
                            outArmyList(armylist);
                            //armylist->Debug();
                            msgout << capital->CurrentArmyTotal() << capital->ArmyListsTotal();
                            outAlliance(capital->occupierId());
                            //msgout << 0 << "" << 0;
                            msgout << capital->troops().size();
                            for (auto troop :  capital->troops()) {
                                if (troop) {
                                    msgout << troop->uid() <<  troop->id();
                                }
                            }
                            msgout << chooseLeftTime;
                            outAlliance(capital->occupierId()); 
                            //msgout << 0 << "" << 0;
                            msgout << 0;
                            msgout << 0;
                        }
                        break;
                        case MapUnitType::CATAPULT:
                        {
                            Catapult* cp = unit->ToCatapult();
                            //cout << "Catapult :";
                            int leftTime = 0;
                            int headId = 0;
                            // int param1 = 0;
                            string nickname;
                            nickname = cp->CityTpl().name;
                            string allianceName;
                            int allianceBannerId = 0;
                            if (AllianceSimple* as = g_mapMgr->alliance().FindAllianceSimple(cp->occupierId())) {
                                allianceName = as->info.nickname + as->info.name;
                                allianceBannerId = as->info.bannerId;
                            }
                            const Capital& plc = g_mapMgr->capital();
                            if (cp->isNotStart()) {
                                leftTime = plc.startLeftTime();
                                if (const Agent* king = g_mapMgr->king()) {
                                    headId = king->headId();
                                } else {
                                    // param1 = plc.chooseLeftTime();
                                }
                            } else {
                                if (cp->occupierId() == 0) {
                                    leftTime = plc.cpRecoverLeftTime();
                                } else {
                                    leftTime = plc.endLeftTime();
                                    if (const Agent* agt = cp->occupier()) {
                                        headId = agt->headId();
                                        //nickname = agt->nickname();
                                    }
                                }
                                // param1 = cp->defenders().size();
                            }
                            msgout << headId << 0 << nickname << 0 << (int)cp->state() << cp->lastKillTimestamp() << cp->troopId() << float(0) << leftTime << cp->kills();
                            armylist = cp->DefArmyList();
                            outArmyList(armylist);
                            msgout << cp->CurrentArmyTotal() << cp->ArmyListsTotal();
                            //outAlliance(cp.occupierId());
                            msgout << cp->occupierId() << allianceName << allianceBannerId;
                            msgout << cp->troops().size();
                            for (auto troop :  cp->troops()) {
                                if (troop) {
                                    msgout << troop->uid() <<  troop->id();
                                }
                            };
                            msgout << cp->Power();

                            //outAlliance(cp->occupierId());
                            msgout << cp->occupierId() << allianceName << allianceBannerId;
                            msgout << 0;
                            msgout << 0;
                        }
                        break;
                        case MapUnitType::WORLD_BOSS: {
                            WorldBoss* worldboss = unit->ToWorldBoss();
                            // worldboss->tpl().ToWorldBoss().name;
                            // Monster* monster = unit->ToMonster();
                            msgout << 0 << 0 <<  worldboss->tpl().name << 0 << 0;
                            msgout << worldboss->CurrentArmyTotal() << 0 << (float)0 << 0 << 0;
                            outArmyList(0);
                            msgout << 0 <<  0;
                            outAlliance(0);
                            msgout << 0;
                            msgout << 0;
                            outAlliance(0);
                            msgout << 0;
                            msgout << 0;
                        }
                        break;
                        case MapUnitType::CHOW:
                        case MapUnitType::PREFECTURE:
                        case MapUnitType::COUNTY:  {
                            FamousCity* city = unit->ToFamousCity();
                            msgout << city->uid() << 0 <<  city->nickname() << 0 << (int)city->state();
                            msgout << 0 << city->troopId() << (float)city->cityDefenseRecover() << 0 << 0;
                            armylist = city->DefArmyList();
                            outArmyList(armylist);
                            msgout << city->cityDefense() <<  city->cityDefenseMax();
                            outAlliance(city->allianceId());
                            msgout << city->troops().size();
                            for (auto troop :  city->troops()) {
                                if (troop) {
                                    msgout << troop->uid() <<  troop->id();
                                }
                            }
                            msgout << city->occupyLeftTime();
                            outAlliance(city->occupyAllianceId());
                            msgout << (int)city->npcRefreshTime();
                            msgout << 0;
                        }
                        break;
                        case MapUnitType::CAMP_TEMP: {
                            auto camp = unit->ToCampTemp();
                            msgout << camp->uid() <<  0 <<  camp->nickname() << 0 << 0;
                            msgout << 0 << camp->troopId() << (float)0 << 0 << 0;
                            outArmyList(armylist);
                            msgout << 0 <<  0;
                            outAlliance(camp->allianceId());
                            msgout << 0;
                            msgout << 0;
                            outAlliance(0);
                            msgout << 0;
                            msgout << 0;
                        }
                        break;
                        case MapUnitType::CAMP_FIXED: {
                            auto camp = unit->ToCampFixed();
                            msgout << camp->uid() <<  0 <<  camp->nickname() << 0 << 0;
                            msgout << 0 << camp->troopId() << (float)0 << 0 << 0;
                            outArmyList(armylist);
                            msgout << camp->campFixedDurable() <<  0;
                            outAlliance(camp->allianceId());
                            msgout << camp->troops().size();
                            for (auto troop :  camp->troops()) {
                                if (troop) {
                                    msgout << troop->uid() <<  troop->id();
                                }
                            }
                            msgout << 0;
                            outAlliance(0);
                            msgout << 0;
                            msgout << 0;
                        }
                        break;
                        default:
                            msgout << 0 <<  0 <<  "" << 0 << 0;
                            msgout << 0 << 0 << (float)0 << 0 << 0;
                            outArmyList(armylist);
                            msgout << 0 <<  0;
                            outAlliance(0);
                            msgout << 0;
                            msgout << 0;
                            outAlliance(0);
                            msgout << 0;
                            msgout << 0;
                            break;
                    }
                }

                msgout << removeList.size();
                // cout << "unit removeList begin" << endl;
                for (int unid : removeList) {
                    msgout << unid;
                    // cout << unid << ", ";
                }
                // cout <<  endl;
                // cout << "unit removeList end" << endl;
                g_mapMgr->proxy()->mbox()->Cast(m_agent.m_mbid, msgout);
            }
        }

        void AgentProxy::SendMapUnitRemove(const vector< int >& unids)
        {
            if (m_agent.m_mbid) {
                //cout << "agent SendMapUnitRemove" << endl;
                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, 64, g_mapMgr->mempool());
                msgout << (int)model::SC::MAP_UNIT_UPDATE;
                msgout << g_mapMgr->localServiceId();
                msgout << 0;
                msgout << unids.size();
                for (int unid : unids) {
                    msgout << unid;
                    m_agent.m_unitIdCache.erase(unid);
                    //cout << unid << ", ";
                }
                //cout << endl;
                g_mapMgr->proxy()->mbox()->Cast(m_agent.m_mbid, msgout);
            }
        }

        void AgentProxy::SendMapTroopUpdate(std::vector< Troop* >& troops, bool isOnViewMove)
        {
            if (m_agent.m_mbid && !troops.empty()) {
                std::vector<Troop*> troopsSt;
                for (Troop * tp : troops) {
                    bool isSent = m_agent.m_tidCache.emplace(tp->id()).second == false;
                    if (isOnViewMove) {
                        if (!isSent) {
                            troopsSt.push_back(tp);
                        }
                    } else {
                        troopsSt.push_back(tp);
                    }
                }

                if (troopsSt.empty()) {
                    return;
                }

                //cout << "Agent::SendMapTroopUpdate size = " << troopsSt.size() << endl;
                const int COUNT = 300;
                int n = troopsSt.size() / (COUNT + 1) + 1;
                for (int i = 0; i < n; ++i) {
                    base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, 64, g_mapMgr->mempool());
                    msgout << (int)model::SC::MAP_TROOP_UPDATE;
                    msgout << g_mapMgr->localServiceId();
                    int size = (i == n - 1) ? troopsSt.size() % COUNT : COUNT;
                    msgout << size;

                    auto outAlliance = [&](int64_t allianceId) {
                        if (allianceId > 0) {
                            string allianceName = "";
                            //int allianceBannerId = 0;
                            if (AllianceSimple* as = g_mapMgr->alliance().FindAllianceSimple(allianceId)) {
                                allianceName = as->info.name;
                                //allianceBannerId = as->info.bannerId;
                            }
                            msgout << allianceId << allianceName;
                        } else {
                            msgout << allianceId << "";
                        }
                    };

                    auto outPlayer = [&](int64_t uid) {
                        std::string toPlayerNickname = "";
                        int toPlayerHeadId = 0;
                        int toPlayerLevel = 0;
                        if (uid > 0) {
                            if (auto agent = g_mapMgr->FindAgentByUID(uid)) {
                                toPlayerNickname = agent->nickname();
                                toPlayerHeadId = agent->headId();
                                toPlayerLevel = agent->level();
                            }
                        }
                        msgout << toPlayerNickname << toPlayerHeadId << toPlayerLevel;
                    };

                    for (int j = 0; j < size; ++j) {
                        Troop* troop = troopsSt.at(i * COUNT + j);
                        Unit* toUnit = g_mapMgr->FindUnit(troop->to());
                        int toId = toUnit !=  nullptr  ?  toUnit->id() : 0;
                        int64_t toUid = toUnit !=  nullptr  ?  toUnit->uid() : 0;
                        int64_t toAllianceId  = toUnit !=  nullptr  ?  toUnit->allianceId() : 0;

                        msgout << troop->id() << (int)troop->type() << (int)troop->state() << troop->uid() << troop->from().x << troop->from().y << g_mapMgr->GetUnitTplid(troop->from()) << troop->to().x << troop->to().y << g_mapMgr->GetUnitTplid(troop->to()) << toId;
                        outAlliance(toAllianceId);
                        outPlayer(toUid);

                        // cout << "######troopInfo   " << troop->id() << " " << uid() << " " << (int)troop->type() << " " << (int)troop->state() << " " << troop->uid() << " " << troop->from().x << " " << troop->from().y << " " << g_mapMgr->GetUnitTplid(troop->from()) << " " << troop->to().x << " " << troop->to().y << " " << g_mapMgr->GetUnitTplid(troop->to()) << endl;
                        msgout << troop->allianceId() << troop->allianceName() << troop->nickname() << troop->headId() << troop->lordLevel() << troop->leftSecond() << troop->totalSecond() << troop->speed() << troop->initialSpeed() << troop->speedUp() << troop->currentPos().x << troop->currentPos().y;
                        // cout << troop->allianceId() << " " << troop->allianceNickname() << " " << troop->nickname() << " " << troop->headId() << " " << troop->lordLevel() << " leftSecond = " << troop->leftSecond() << " totalSecond = " << troop->totalSecond() << " " << troop->speed() << " " << troop->currentPos().x << " " << troop->currentPos().y << endl;

                        msgout << troop->teamId();
                        if (troop->armyList()) {
                            ArmyList& al = *troop->armyList();
                            msgout << troop->power();
                            msgout << al.size();
                            for (auto it = al.armies().begin(); it != al.armies().end(); ++it) {
                                //int tplid = it->first;
                                const ArmyGroup& armyGroup = it->second;
                                msgout << armyGroup.heroInfo().tplId();
                                msgout << armyGroup.heroInfo().level();
                                msgout << armyGroup.heroInfo().star();
                                msgout << armyGroup.armyInfo().tplid();
                                msgout << armyGroup.position();
                                msgout << 1;
                                msgout << (int)ArmyState::NORMAL << armyGroup.armyInfo().count(ArmyState::NORMAL);
                                // cout << "troop id is " << troop->id() << "===" << "ArmyList:" << armyGroup.heroInfo().tplId() << ", " << armyGroup.heroInfo().level()  << ", " << armyGroup.heroInfo().star()  << ", " << armyGroup.armyInfo().tplid() << endl;
                            }
                        } else {
                            msgout << 0;
                            msgout << 0;
                        }

                        int param1 = 0;
                        int param2 = 0;
                        if (troop->type() == MapTroopType::GATHER && toUnit) {
                            auto resNode = toUnit->ToResNode();
                            if (troop->IsReach()) {
                                if (resNode) {
                                    resNode->Gather();
                                }
                            }
                            param1 = troop->DropItemCount();
                            if (resNode) {
                                param2 = resNode->canGather() + resNode->hadGather();
                            }
                        } else if (troop->type() ==  MapTroopType::SCOUT ||  troop->type() ==  MapTroopType::SIEGE_CASTLE) {
                            auto castle = troop->agent().castle();
                            if (castle) {
                                param1 = castle->pos().x * 10000 + castle->pos().y;
                            }
                        }
                        msgout << param1 <<  param2;
                    }
                    msgout << 0;
                    g_mapMgr->proxy()->mbox()->Cast(m_agent.m_mbid, msgout);
                    /*cout << "$$$$$$$uid is " << uid() << endl;*/
                    m_agent.m_castle->CheckIsSendReinforceTroop();
                }
            }
        }

        void AgentProxy::SendMapTroopRemove(const vector< int >& tids)
        {
            if (m_agent.m_mbid) {
                //cout << "agent SendMapTroopRemove" << endl;
                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, 64, g_mapMgr->mempool());
                msgout << (int)model::SC::MAP_TROOP_UPDATE;
                msgout << g_mapMgr->localServiceId();
                msgout << 0;
                msgout << tids.size();
                // cout << "SendMapTroopRemove, uid, " << uid() << endl;
                for (int tid : tids) {
                    msgout << tid;
                    m_agent.m_tidCache.erase(tid);
                    // cout << "tid" << " = " << tid << endl;
                }
                //cout << endl;
                g_mapMgr->proxy()->mbox()->Cast(m_agent.m_mbid, msgout);
                m_agent.m_castle->CheckIsSendReinforceTroop();
            }
        }

        void AgentProxy::SendMessage(base::cluster::MessageOut& msgout)
        {
            if (m_agent.m_mbid) {
                g_mapMgr->proxy()->mbox()->Cast(m_agent.m_mbid, msgout);
            }
        }

        template<typename ...Args>
        void AgentProxy::SendMessage(uint16_t code, uint32_t approx_size, uint16_t session, Args ...args)
        {
            base::cluster::MessageOut msgout(code, approx_size, g_mapMgr->mempool());
            msgout.SetSession(session);
            int argc = sizeof...(args);
            msgout << argc;
            write_msgout(msgout, args...);
            SendMessage(msgout);
        }


        void AgentProxy::SendMapMarchResponse(model::MarchErrorNo errNo)
        {
            if (m_agent.m_mbid) {
                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, 64, g_mapMgr->mempool());
                msgout << (int)model::SC::MAP_MARCH_RESPONSE << (int)errNo;
                g_mapMgr->proxy()->mbox()->Cast(m_agent.m_mbid, msgout);
            }
        }

        void AgentProxy::SendNoticeMessage(model::ErrorCode id, int type, const string& notice)
        {
            if (m_agent.m_mbid) {
                //cout << "agent SendNoticeMessage" << endl;
                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, 64, g_mapMgr->mempool());
                msgout << (int)model::SC::NOTICE_MESSAGE << id << notice << type;
                g_mapMgr->proxy()->mbox()->Cast(m_agent.m_mbid, msgout);
            }
        }

        void AgentProxy::SendMapPersonalInfoUpdate()
        {
            if (m_agent.m_mbid && m_agent.m_castle) {
//                 cout << "agent SendMapPersonalInfoUpdate" << endl;
                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, 64, g_mapMgr->mempool());
                msgout << (int)model::SC::MAP_PERSONAL_INFO_UPDATE;
                int k = g_mapMgr->localServiceId();
                int blockId = g_mapMgr->GetBlockByPoint(m_agent.m_castle->pos());
                msgout << k << m_agent.m_castle->x() << m_agent.m_castle->y() << blockId << m_agent.m_castle->id() <<  m_agent.m_castle->cityDefense() << m_agent.m_castle->silverRepairLeftTime() <<  m_agent.m_castle->goldRepairLeftTime() <<
                       m_agent.m_castle->burnLeftTime() << m_agent.m_castle->burnTotalTime() << m_agent.m_castle->burnTimes() << (int)m_agent.m_castle->state();
//                 cout << "cityDefense = " << m_castle->cityDefense() << " cityDefenseMax = " << m_castle->cityDefenseMax() << endl;
                g_mapMgr->proxy()->mbox()->Cast(m_agent.m_mbid, msgout);
            }
        }

        void AgentProxy::SendAllianceReinforcements()
        {
            if (m_agent.m_mbid) {
                // cout << "agent SendAllianceReinforcements marchReinforcementsize reachReinforcementsize =   " << m_castle->marchReinforcements().size() << "," << m_castle->reachReinforcements().size() << endl;
                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, 64, g_mapMgr->mempool());
                msgout << (int)model::SC::MAP_SEND_REINFORCEMENTS;
                msgout << m_agent.m_castle->marchReinforcements().size() + m_agent.m_castle->reachReinforcements().size();
                // cout << "===###marchReinforcements=== uid, " << m_uid << endl; 
                // cout << "===###marchReinforcements=== size, " << m_castle->marchReinforcements().size() << endl; 
                for (auto id: m_agent.m_castle->marchReinforcements()) {
                    msgout << id;
                    cout << id << endl;
                }
                // cout << "===###reachReinforcements=== size, " << m_castle->reachReinforcements().size() << endl; 
                for (auto id: m_agent.m_castle->reachReinforcements()) {
                    msgout << id;
                    cout << id << endl;
                }
                g_mapMgr->proxy()->mbox()->Cast(m_agent.m_mbid, msgout);
            }
        }

        void AgentProxy::SendMapQueryPlayerInfoResponse(const Agent* agent, const std::vector<info::RankInfo>& vecRank)
        {
            if (m_agent.m_mbid && agent) {
//                 cout << "agent SendMapQueryPlayerInfoResponse" << endl;
                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, 64, g_mapMgr->mempool());
                msgout << (int)model::SC::MAP_QUERY_PLAYERINFO_RESPONSE;
                msgout << agent->m_uid << agent->m_nickname << agent->m_level << agent->m_exp << agent->m_headId << agent->m_totalPower
                       << agent->allianceId() << agent->allianceName() << agent->allianceName() << agent->allianceBannerId() << agent->allianceLevel()
                       << agent->vipLevel() << agent->titleId();
                msgout << (int)vecRank.size();
                for (const RankInfo & info : vecRank) {
                    msgout << (int)info.type;
                    msgout << info.rank;
//                     cout << "type=" << (int)info.type << ", rank=" << info.rank << endl;
                }
                g_mapMgr->proxy()->mbox()->Cast(m_agent.m_mbid, msgout);
            }
        }


        void AgentProxy::SendMapAllianceInviteListResponse(const std::vector< Agent* >& list)
        {
            if (m_agent.m_mbid) {
                // cout << "agent SendMapAllianceInviteList size = " << list.size() << endl;
                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, 64, g_mapMgr->mempool());
                msgout << (int)model::SC::MAP_ALLIANCE_INVITE_LIST_RESPONSE;
                msgout << list.size();
                for (Agent * agt : list) {
                    msgout << agt->uid() << agt->headId() << agt->nickname() << agt->level() << agt->vipLevel();
                    // cout << agt->uid() << " headId:" << agt->headId() << " nickname:" << agt->nickname() << " userLevel:" << agt->level() << " vipLevel:" << agt->vipLevel() << endl;
                }
                g_mapMgr->proxy()->mbox()->Cast(m_agent.m_mbid, msgout);
            }
        }

        void AgentProxy::SendAllianceCitiesInfoUpdate()
        {
            auto allianceCity = g_mapMgr->AllianceCity(m_agent.allianceId());
                if (!allianceCity) {
                    return;
            }
            if (m_agent.allianceId() > 0) {
                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, 64, g_mapMgr->mempool());
                msgout << (int)model::SC::GET_ALLIANCE_CITIES_RESPONSE;
                msgout << allianceCity->size();
                for (auto it = allianceCity->begin(); it != allianceCity->end(); ++it) {
                    msgout << (*it)->cityTpl()->cityId << (int)(*it)->type() << (*it)->cityTpl()->name << (*it)->cityDefense() << (*it)->cityDefenseMax() << (*it)->troops().size() << (*it)->x() << (*it)->y();
                    cout << "id , type, name, cityDefense, cityDefenseMax, size, x, y " << (*it)->cityTpl()->cityId << " " << (int)(*it)->type() << " " << (*it)->cityTpl()->name << " " << (*it)->cityDefense() << " " << (*it)->cityDefenseMax() << "" <<  (*it)->troops().size() << " " << (*it)->x() << " " << (*it)->y() << endl;
                }
                g_mapMgr->proxy()->mbox()->Cast(m_agent.m_mbid, msgout);
            }
        }

        void AgentProxy::SendMapGetNearestUnitResponse(const Unit* unit)
        {
            if (m_agent.m_mbid) {
                if (unit) {
                    //cout << "Agent::SendMapGetNearestUnitResponse x = " << unit->x() << " y = " << unit->y() << endl;
                    base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, 64, g_mapMgr->mempool());
                    msgout << (int)model::SC::MAP_GET_NEAREST_UNIT_RESPONSE;
                    msgout << unit->pos();
                    g_mapMgr->proxy()->mbox()->Cast(m_agent.m_mbid, msgout);
                } else {
                    cout << "Agent::SendMapGetNearestUnitResponse false" << endl;
                    base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, 64, g_mapMgr->mempool());
                    msgout << (int)model::SC::MAP_GET_NEAREST_UNIT_RESPONSE;
                    msgout << 0 << 0;
                    g_mapMgr->proxy()->mbox()->Cast(m_agent.m_mbid, msgout);
                }
            }
        }

        void AgentProxy::SendMapSearchResponse(int x, int y)
        {
            if (m_agent.m_mbid) {
                cout << "###SendMapSearchResponse, x, y = " << x << "," << y <<endl;
                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, 64, g_mapMgr->mempool());
                msgout << (int)model::SC::MAP_SEARCH_RESPONSE;
                msgout << x;
                msgout << y;
                g_mapMgr->proxy()->mbox()->Cast(m_agent.m_mbid, msgout);
            }
        }

        void AgentProxy::SendPalaceWarSuccessiveKingsResponse()
        {
            if (m_agent.m_mbid) {
                cout << "Agent::SendPalaceWarSuccessiveKingsResponse" << endl;
                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, 64, g_mapMgr->mempool());
                msgout << (int)model::SC::PALACE_WAR_SUCCESSIVE_KINGS_RESPONSE;
                const std::list<SuccessiveKing>& sks = g_mapMgr->capital().successiveKings();
                msgout << sks.size();
                for (auto it = sks.begin(); it != sks.end(); ++it) {
                    const SuccessiveKing& sk = *it;
                    msgout << sk.headId << sk.allianceNickname << sk.nickname << sk.n << sk.timestamp;
                    cout << "headId, allianceNickName, nickname, n, timestamp    " <<  sk.headId << sk.allianceNickname << sk.nickname << sk.n << sk.timestamp << endl;
                }
                g_mapMgr->proxy()->mbox()->Cast(m_agent.m_mbid, msgout);
            }
        }

        void AgentProxy::SendPalaceWarRecordsResponse(int x, int y, const list< PalaceWarRecord >& records)
        {
            if (m_agent.m_mbid) {
                cout << "Agent::SendPalaceWarRecordsResponse x = " << x << " y = " << y << " size = " << records.size() << endl;
                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, 64, g_mapMgr->mempool());
                msgout << (int)model::SC::PALACE_WAR_RECORDS_RESPONSE;
                msgout << x << y;
                msgout << records.size();
                for (auto it = records.begin(); it != records.end(); ++it) {
                    const PalaceWarRecord& pwr = *it;
                    msgout << (int)pwr.type << pwr.param << pwr.timestamp;
                    cout << "x, y, type, param, timestamp    " << x << y << (int)pwr.type << pwr.param << pwr.timestamp << endl;;
                }
                g_mapMgr->proxy()->mbox()->Cast(m_agent.m_mbid, msgout);
            }
        }
        
        void AgentProxy::SendKingGetKingdomSetInfosResponse()
        {
            if (m_agent.m_mbid && m_agent.isKing()) {
                //cout << "Agent::SendKingGetKingdomSetInfosResponse" << endl;
                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, 64, g_mapMgr->mempool());
                msgout << (int)model::SC::KING_GET_KINGDOM_SET_INFOS_RESPONSE;
                msgout << g_mapMgr->serverName() << g_mapMgr->m_canChangeServerName << g_mapMgr->m_resourceNodeRateId << g_mapMgr->m_lastChangeResIdTime;
                g_mapMgr->proxy()->mbox()->Cast(m_agent.m_mbid, msgout);
            }
        }

        void AgentProxy::SendGetTroopExtraInfoResponse(std::list<Troop*>& troops)
        {
            if (m_agent.m_mbid) {   

                // 得到上下浮动20%的值
                auto getRoughValue = [&](int& value) {
                    int max = value * (1 + 0.2);    
                    int min = value * (1 - 0.2);    
                    value = framework.random().GenRandomNum(min, max + 1);
                };

                //cout << "Agent::SendGetTroopExtraInfoResponse" << endl;
                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, 64, g_mapMgr->mempool());
                msgout << (int)model::SC::MAP_GET_TROOP_EXTRA_INFO_RESPONSE;
                
                std::vector< tpl::ScoutTypeTpl > scoutTypesTpl;
                map::tpl::m_tploader->GetScoutType(100,  scoutTypesTpl, false);
                msgout << scoutTypesTpl.size();
                for (auto tpl : scoutTypesTpl) {
                    msgout << tpl.type;
                }
                msgout << troops.size();
                for (auto troop : troops)
                {
                    msgout << troop->id();
                    base::DataTable paramTable;
                    for (auto tpl : scoutTypesTpl) {
                        bool isValid = true;
                        base::DataTable table;
                        auto type = tpl.type;
                        switch (type) {
                            // 501
                            case WATCHTOWER_SCOUT_TYPE::PLAY_HEAD_LEVEL: {
                                table.Set("headId", troop->agent().headId());
                                table.Set("level", troop->agent().level());
                                table.Set("name", troop->agent().nickname());
                                table.Set("allianceName", troop->agent().allianceName());
                                table.Set("x", troop->from().x);
                                table.Set("y", troop->from().y);
                            }
                            break;
                            // 502
                            case WATCHTOWER_SCOUT_TYPE::MARCH_REMAIN_TIME: {
                                table.Set("remainTime", troop->leftSecond());
                            }
                            break;
                            // 503
                            case WATCHTOWER_SCOUT_TYPE::MARCH_ARMY_ROUGH_COUNT: {
                                int count = 0;
                                if(ArmyList* armyList = troop->armyList())
                                {
                                    count = armyList->ArmyCount(ArmyState::NORMAL);
                                }
                                getRoughValue(count);
                                table.Set("marchArmyRoughCount", count);
                            }
                            break;
                            // 504
                            case WATCHTOWER_SCOUT_TYPE::MARCH_HERO_NAME: {
                                DataTable Table;
                                ArmyList* armyList = troop->armyList();
                                int i = 0;
                                for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                    base::DataTable subTable;
                                    HeroInfo& heroInfo = it->second.heroInfo();
                                    subTable.Set("heroId", heroInfo.tplId());
                                    Table.Set(++i, subTable);
                                }
                                table.Set("heroName", Table);
                            }
                            break;
                            // 505
                            case WATCHTOWER_SCOUT_TYPE::MARCH_HERO_LEVEL: {
                                DataTable Table;
                                ArmyList* armyList = troop->armyList();
                                int i = 0;
                                for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                    base::DataTable subTable;
                                    HeroInfo& heroInfo = it->second.heroInfo();
                                    subTable.Set("level", heroInfo.level());
                                    Table.Set(++i, subTable);
                                }
                                table.Set("heroLevel", Table);
                            }
                            break;
                            // 506
                            case WATCHTOWER_SCOUT_TYPE::MARCH_ARMY_TYPE_LEVEL: {
                                DataTable Table;
                                ArmyList* armyList = troop->armyList();
                                int i = 0;
                                for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                    base::DataTable subTable;
                                    ArmyInfo& armyInfo = it->second.armyInfo();
                                    subTable.Set("type", armyInfo.type());
                                    subTable.Set("level", armyInfo.level());
                                    Table.Set(++i, subTable);
                                }
                                table.Set("armyTypeLevel", Table);
                            }
                            break;
                            // 507
                            case WATCHTOWER_SCOUT_TYPE::MARCH_ARMY_COUNT: {
                                DataTable Table;
                                if(ArmyList* armyList = troop->armyList())
                                {
                                    int i = 0;
                                    for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                        base::DataTable subTable;
                                        ArmyInfo& armyInfo = it->second.armyInfo();
                                        subTable.Set("count", armyInfo.count(ArmyState::NORMAL));
                                        Table.Set(++i, subTable);
                                    }

                                }
                                table.Set("marchArmyCount", Table);
                            }
                            break;
                            // 508
                            case WATCHTOWER_SCOUT_TYPE::MARCH_TECH_INFO: {
                                DataTable technologiesTable;
                                auto& technologies = troop->agent().technologies();
                                for (auto it = technologies.begin(); it != technologies.end(); ++it) {
                                    technologiesTable.Set(it->first, it->second);
                                }
                                table.Set("technologies", technologiesTable);
                            }
                            break;
                            // 509
                            case WATCHTOWER_SCOUT_TYPE::MARCH_HERO_SKILL_CONFIGURATION: {
                                DataTable Table;
                                ArmyList* armyList = troop->armyList();
                                int i = 0;
                                for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                    base::DataTable subTable;
                                    HeroInfo& heroInfo = it->second.heroInfo();
                                    int m = 0;
                                    for (auto & skill : heroInfo.skill())
                                    {
                                        base::DataTable subSubTable;
                                        subSubTable.Set("id", skill.id);
                                        subSubTable.Set("level", skill.level);
                                        subTable.Set(++m, subSubTable);
                                    }
                                    Table.Set(++i, subTable);
                                }
                                table.Set("heroSkillConf", Table);
                            }
                            break;
                            /*// 510
                            case WATCHTOWER_SCOUT_TYPE::MARCK_ALL_ARMY_COUNT: {
                                DataTable Table;
                                ArmyList* armyList = troop->armyList();
                                int i = 0;
                                for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                    base::DataTable subTable;
                                    HeroInfo& heroInfo = it->second.heroInfo();
                                    int m = 0;
                                    for (auto & skill : heroInfo.skill())
                                    {
                                        base::DataTable subSubTable;
                                        subSubTable.Set("id", skill.id);
                                        subSubTable.Set("level", skill.level);
                                        subTable.Set(++m, subSubTable);
                                    }
                                    Table.Set(++i, subTable);
                                }
                                table.Set("heroSkillConf", Table);
                            }
                            break;*/
                            default:
                                    isValid = false;
                                    break;
                        }
                        if (isValid) {
                            paramTable.Set(tpl.uiKeywords, table);
                        }
                    }
                    /*paramTable.DumpInfo();*/
                    string param = paramTable.Serialize();
                    msgout << param;
                }
                g_mapMgr->proxy()->mbox()->Cast(m_agent.m_mbid, msgout);
            }
        }

        void AgentProxy::SendRecallAllianceTroop(bool result)
        {
            if (m_agent.m_mbid) {
                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, 64, g_mapMgr->mempool());
                msgout << (int)model::SC::MAP_RECALL_ALLIANCE_TROOP_RESPONSE << result;
                g_mapMgr->proxy()->mbox()->Cast(m_agent.m_mbid, msgout);
            }
        }

        void AgentProxy::SendCanMarchMonsterLevelUpdate()
        {
            int level = m_agent.m_monsterDefeatedLevel + 1;
            // cout << "-----------------Agent::SendCanMarchMonsterLevelUpdate------------" <<level<< endl;
            if (m_agent.m_mbid) {
                base::cluster::MessageOut msgout((uint16_t)model::rpc::MapCode::FORWARD, 64, g_mapMgr->mempool());
                msgout << (int)model::SC::CAN_MARCH_MONSTER_LEVEL_UPDATE << (int)level;
                g_mapMgr->proxy()->mbox()->Cast(m_agent.m_mbid, msgout);
            }
        }

    }

}