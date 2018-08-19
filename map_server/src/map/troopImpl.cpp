#include "troop.h"
#include "troopImpl.h"
#include "agent.h"
#include "map.h"
#include "mapProxy.h"
#include "unit/unit.h"
#include "unit/castle.h"
#include "unit/monster.h"
#include "unit/camp.h"
#include "unit/resnode.h"
#include "unit/famouscity.h"
#include "unit/catapult.h"
#include "battle/battlemgr.h"
#include "model/tpl/templateloader.h"
#include "model/tpl/configure.h"
#include "tpl/templateloader.h"
#include "tpl/npcarmytpl.h"
#include "tpl/scouttypetpl.h"
#include "tpl/mapcitytpl.h"
#include "alliance.h"

namespace ms
{
    namespace map
    {
        using namespace std;
        using namespace base;
        using namespace model;
        using namespace model::tpl;
        using namespace battle;

        void TroopImpl::OnAttackFailed(Unit* unit)
        {
            m_troop->GoHome();
        }

        //防守失败
        void TroopImpl::OnDefenceFailed(Unit* unit)
        {
            m_troop->GoHome();
        }

        void TroopImpl::OnArriveCamp()
        {
            MapTroopType troopType = m_troop->parentType();
            if (troopType == MapTroopType::CAMP_FIXED || troopType == MapTroopType::ASSIGN) {
                m_troop->ResetParent();
                m_troop->SetState(model::MapTroopState::REACH);

                do {
                    if (Unit* to = g_mapMgr->FindUnit(m_troop->to())) {
                        auto campFixed = to->ToCampFixed();
                        if (campFixed) {
                            if (campFixed->uid() == m_troop->uid()) {
                                m_troop->SetType(troopType);
                                // 7.  行营出发的部队，在返回时服务器即会判定原行营点的状态，如不是自己的行营则会直接回城
                                // （如自己手贱，拆除了原有的，重新在原坐标点建了新的行营，则返回的部队直接回城）
                                if (campFixed->troopId() == 0 /*&& campFixed->bindTroopId() ==  m_troop->id()*/) {
                                    if (Occupy(campFixed)) {
                                        m_troop->agent().OnTroopArriveCampFixed(m_troop);
                                        break;
                                    }
                                }
                            } else {
                                m_troop->SetType(MapTroopType::CAMP_FIXED_OCCUPY);
                                if (campFixed->CanAttack(m_troop)) {
                                    //8.    行营出发的部队返回行营途中，行营坐标点如发生了以下变化（敌方驻扎、敌方行营）时会发生战斗
                                    if (BattleMgr::instance().GenerateBattle(m_troop, to)) {
                                        if (!to->IsDelete()) {
                                            to->NoticeUnitUpdate();   //可能被delete, CAMP_FIXED, CAMP_TEMP
                                        }
                                        break;
                                    }
                                }
                            }
                        } else {
                            //行营被摧毁 且 已有其它Unit
                            auto campTemp = to->ToCampTemp();
                            if (campTemp && campTemp->CanAttack(m_troop)) {
                                m_troop->SetType(MapTroopType::CAMP_TEMP);
                                //8.    行营出发的部队返回行营途中，行营坐标点如发生了以下变化（敌方驻扎、敌方行营）时会发生战斗
                                if (BattleMgr::instance().GenerateBattle(m_troop, to)) {
                                    if (!to->IsDelete()) {
                                        to->NoticeUnitUpdate();   //可能被delete, CAMP_FIXED, CAMP_TEMP
                                    }
                                    break;
                                }
                            }
                        }
                    } else {
//                         //行营被摧毁 但 无其它Unit
//                         m_troop->SetType(MapTroopType::CAMP_TEMP);
//                         if (CampingTemp()) {
//                             break;
//                         }
                    }
                    m_troop->GoHome();
                } while (0);
            }
        }
        
        void TroopImpl::OnArriveCastle()
        {
            if ( Unit* to = g_mapMgr->FindUnit ( m_troop->to() ) ) {
                if ( Castle* castle = to->ToCastle() ) {
                    if ( castle->uid() == m_troop->uid() ) {
                        m_troop->agent().OnTroopArriveCastle ( m_troop );
                    }
                }
            }
            m_troop->Remove();
        }
        
        void TroopImpl::OnArriveHome()
        {
            //cout << "Troop::OnArriveHome()" << endl;
            if (m_troop->parentType() == MapTroopType::CAMP_FIXED || m_troop->parentType() == MapTroopType::ASSIGN) {
                    OnArriveCamp();
            } else {
                    OnArriveCastle();
            }
        }

        void TroopImpl::OnBack()
        {
            //cout << "Troop::OnBack()" << endl;
             if (!m_troop->IsReach()) {
                return;
            }

            Unit* reachUnit = g_mapMgr->FindUnit(m_troop->to());
            if (!reachUnit) {
                return;
            }

            reachUnit->OnTroopBack(m_troop);
            reachUnit = nullptr;    //单元有可能已经被delete 比如Camp
        }

        void TroopImpl::OnLeave()
        {
            //cout << "Troop::OnLeave" << endl;
            if (!m_troop->IsReach()) {
                return;
            }

            Unit* reachUnit = g_mapMgr->FindUnit(m_troop->to());
            if (!reachUnit) {
                return;
            }
            
            reachUnit->OnTroopLeave(m_troop);
            reachUnit = nullptr;    //单元有可能已经被delete 比如Camp
        }

        void TroopImpl::OnReachInvalid()
        {
            m_troop->agent().msgQueue().AppendMsgTroopReachInvalid(m_troop->type());

//             //send mail
//             DataTable dt;
//             dt.Set("x", m_troop->to().x);
//             dt.Set("y", m_troop->to().y);
//             dt.Set("type", (int)m_troop->type());
//             g_mapMgr->SendMail(m_troop->uid(), 0, MailType::REPORT, MailSubType::MARCH_FAIL, "", dt.Serialize(), true);
        }

        void TroopImpl::OnMarchBack()
        {

        }

        void TroopImpl::FinishDeserialize()
        {
            if (m_troop->IsGoHome()) {
                m_troop->ResetLinker();
            } else if (m_troop->IsMarching()) {
                m_troop->ResetLinker();
            }
        }

        CampTemp* TroopImpl::CampingTemp(bool isSetup)
        {
            CampTemp* unit = g_mapMgr->CreateCampTemp(m_troop->to().x, m_troop->to().y, m_troop);
            if (unit) {
                //cout << "camp x, y = " << camp->x() << ", " << camp->y() << endl;
                m_troop->SetBeginTimestamp(0);
                m_troop->SetEndTimestamp(0);
                m_troop->NoticeTroopUpdate(true);
                if (!isSetup) {
//                     DataTable dt;
//                     dt.Set("x", m_troop->to().x);
//                     dt.Set("y", m_troop->to().y);
//                     dt.Set("allianceNickname", m_troop->allianceNickname());
//                     dt.Set("nickname", m_troop->nickname());
//                     dt.Set("headId", m_troop->headId());
//                     dt.Set("armyTotal", m_troop->GetArmyTotal());
//                     g_mapMgr->SendMail(m_troop->uid(), 0, MailType::REPORT, MailSubType::MAP_CAMP_SUCCESS, "", dt.Serialize(), true);
                }
                return unit;
            }
            return nullptr;
        }

        bool TroopImpl::CampingFixed()
        {
            if (m_troop->agent().MaxMarchingQueue() >  m_troop->agent().CampFixedCount()) {
                Point pos = m_troop->to();
                if (g_mapMgr->CreateCampFixed(pos.x, pos.y, m_troop)) {
                    m_troop->ResetParent();
                    m_troop->SetBeginTimestamp(0);
                    m_troop->SetEndTimestamp(0);
                    m_troop->NoticeTroopUpdate(true);
//                 DataTable dt;
//                 dt.Set("x", pos.x);
//                 dt.Set("y", pos.y);
//                 dt.Set("allianceNickname", m_troop->allianceNickname());
//                 dt.Set("nickname", m_troop->nickname());
//                 dt.Set("headId", m_troop->headId());
//                 dt.Set("armyTotal", m_troop->GetArmyTotal());
//                 g_mapMgr->SendMail(m_troop->uid(), 0, MailType::REPORT, MailSubType::MAP_CAMP_SUCCESS, "", dt.Serialize(), true);
                    return true;
                }
            }
            return false;
        }

        bool TroopImpl::Occupy(CampFixed* campFixed)
        {
            bool isOccupy = false;
            if (campFixed) {
                do {
                    if (campFixed->uid() !=  m_troop->uid()) {
                        if (m_troop->agent().MaxMarchingQueue() <=  m_troop->agent().CampFixedCount()) {
                            // 8.   行营出发的部队返回行营途中，行营坐标点如发生了以下变化（敌方驻扎、敌方行营）时会发生战斗，
                            // 除此之外都会自动转线回城，如有敌方行营，会发生战斗，战斗胜利后会判定己方行营上限有没达到，
                            // 没达到则占领敌方行营进入驻扎，反之摧毁，自己驻扎

                            campFixed->RemoveSelf();
                            if (CampingTemp()) {
                                isOccupy = true;
                            }
                            break;
                        }
                    }

                    campFixed->Occupy(m_troop);
                    campFixed->NoticeUnitUpdate();
                    m_troop->SetType(MapTroopType::CAMP_FIXED);
                    m_troop->ResetParent();
                    m_troop->SetBeginTimestamp(0);
                    m_troop->SetEndTimestamp(0);
                    m_troop->NoticeTroopUpdate(true);
                    isOccupy = true;
                } while (0);
            }
            return isOccupy;
        }

        bool DefaultTroopImpl::OnReachProc(Unit* to) {
            //m_troop->GoHome();
            return true;
        }
        
        void TroopToRes::Gathering(ResNode* res)
        {
            if (!m_troop->Gather(res)) {
                m_troop->GoHome();
            }
        }

        void TroopToRes::ReCalculateGatherTime()
        {
            if (!m_troop->IsReach()) {
                return;
            }

            Unit* reachUnit = g_mapMgr->FindUnit(m_troop->to());
            if (!reachUnit) {
                return;
            }

            if (ResNode* res = reachUnit->ToResNode()) {
                int time = res->ReCalculateGatherTime();
                if (time == 0) {
                    m_troop->GoHome();
                    return;
                }
                int64_t now = g_dispatcher->GetTimestampCache();
                m_troop->SetEndTimestamp(now +time);
                m_troop->SetDirty();
                m_troop->ResetLinker();
                m_troop->NoticeTroopUpdate();
            }
        }

        void TroopToRes::OnAttackWin(Unit* unit)
        {
            if (ResNode* res = unit->ToResNode()) {
                res->armyList().ClearAll();
                Gathering(res);
            } else {
                m_troop->GoHome();
            }
        }

        void TroopToRes::OnDefenceWin(Unit* unit)
        {
            if (unit) {
                ReCalculateGatherTime();
            }
        }

        bool TroopToRes::OnReachProc(Unit* to)
        {
//             cout << "TroopToRes::OnReachProc " << endl;
            bool isInValid = true;
            if (to) {
                bool fight = false;
                if (ResNode* res = to->ToResNode()) {
                    if (res->CanOccupy(m_troop)) {
                        fight = true;
                    }
                }

                if (fight) {
                    if (BattleMgr::instance().GenerateBattle(m_troop, to)) {
                        isInValid = false;
                        if (!to->IsDelete()) {
                            to->NoticeUnitUpdate();   //可能被delete, CAMP_FIXED, CAMP_TEMP
                        }
                    }
                }
            }
            return isInValid;
        }

        void TroopToRes::FinishDeserialize()
        {
            TroopImpl::FinishDeserialize();
            if (m_troop->IsReach()) {
                if (Unit* unit = g_mapMgr->FindUnit(m_troop->to())) {
                    m_troop->SetReachTplId(unit->tpl().id);
                    if (ResNode* res = unit->ToResNode()) {
                        res->Occupy(m_troop);
                        m_troop->ResetLinker();
                    }
                }
            }
        }

        void TroopToMonster::OnAttackWin(Unit* unit)
        {
//             if (Monster* monster = unit->ToMonster()) {
//
//             }
            m_troop->GoHome();
        }

        bool TroopToMonster::OnReachProc(Unit* to)
        {
            bool isInValid = true;
            if (to) {
                bool fight = false;
                if (/*Monster* monster = */to->ToMonster()) {
                    fight = true;
                }

                if (fight) {
                    if (BattleMgr::instance().GenerateBattle(m_troop, to)) {
                        isInValid = false;
                        if (!to->IsDelete()) {
                            to->NoticeUnitUpdate();   //可能被delete, CAMP_FIXED, CAMP_TEMP
                        }
                    }
                }
            }
            return isInValid;
        }

        void TroopToMonster::OnReachInvalid()
        {
            TroopImpl::OnReachInvalid();
            m_troop->agent().msgQueue().AppendMsgAttackMonsterInvalid();
        }


        void TroopToWorldBoss::OnAttackWin(Unit* unit)
        {
//             if (Monster* monster = unit->ToMonster()) {
//
//             }
            m_troop->GoHome();
        }

        bool TroopToWorldBoss::OnReachProc(Unit* to)
        {
            bool isInValid = true;
            if (to) {
                bool fight = false;
                if (/*Monster* monster = */to->ToWorldBoss()) {
                    fight = true;
                }

                if (fight) {
                    if (BattleMgr::instance().GenerateBattle(m_troop, to)) {
                        isInValid = false;
                        if (!to->IsDelete()) {
                            to->NoticeUnitUpdate();   //可能被delete, CAMP_FIXED, CAMP_TEMP
                        }
                    }
                }
            }
            return isInValid;
        }

        void TroopToWorldBoss::OnReachInvalid()
        {
            TroopImpl::OnReachInvalid();
            m_troop->agent().msgQueue().AppendMsgAttackMonsterInvalid();
        }



        void TroopToCity::OnDefenceFailed(Unit* unit)
        {
//             if (m_troop->GetArmyTotal() <=  0) {
                m_troop->GoHome();
//             }
        }

        void TroopToCity::OnAttackWin(Unit* unit)
        {
            if (/*auto city = */unit->ToFamousCity()) {
                if (!m_troop->isOccupyCity()) {
                    m_troop->GoHome();
                }
            } else {
                m_troop->GoHome();
            }
        }

        bool TroopToCity::OnReachProc(Unit* to)
        {
            bool isInValid = true;
            if (m_troop->allianceId() > 0) {
                if (auto city = to->ToFamousCity()) {
                    //占领权一致，就是自己的城
                    if (city->occupyAllianceId() ==  m_troop->allianceId()) {
                        isInValid = OnReachMyCity(city);
                    } else {
                        isInValid = OnReachOtherCity(city);
                    }
                }
            }
            return isInValid;
        }
        
        bool TroopToCity::OnReachMyCity(FamousCity* city)
        {
            bool isInValid = true;
            if (m_troop->type() ==  MapTroopType::SIEGE_CITY) {
                if (city->CanGarrison(m_troop)) {
                    city->Garrison(m_troop);
                    city->NoticeUnitUpdate();
                    isInValid = false;
                }
            } else if (m_troop->type() ==  MapTroopType::PATROL_CITY) {
                ProcPatrolEvent(city);
                isInValid = false;
            }
            return isInValid;
        }

        bool TroopToCity::OnReachOtherCity(FamousCity* city)
        {
            bool isInValid = true;
            bool fight = false;
            if (city->CanOccupy(m_troop)) {
                fight = true;
            }

            if (fight) {
                if (BattleMgr::instance().GenerateBattle(m_troop, city)) {
                    isInValid = false;
                    if (!city->IsDelete()) {
                        city->NoticeUnitUpdate();   //可能被delete, CAMP_FIXED, CAMP_TEMP
                    }
                }
            }
            return isInValid;
        }

        void TroopToCity::ProcPatrolEvent(FamousCity* city)
        {
            BattleMgr::instance().PatrolCity(m_troop, city);
            m_troop->GoHome();
        }

        void TroopToCastle::OnDefenceFailed(Unit* unit)
        {
        }

        void TroopToCastle::OnAttackWin(Unit* unit)
        {
            if (/*auto castle = */unit->ToCastle()) {
            } else if (auto campFixed = unit->ToCampFixed()) {
                campFixed->RemoveSelf();
            }
            m_troop->GoHome();
        }

        bool TroopToCastle::OnReachAllianceCastle(Castle* castle)
        {
            bool isInValid = true;
            if (m_troop->type() ==  MapTroopType::SIEGE_CASTLE) {
                if (castle->CanGarrison(m_troop)) {
                    castle->Garrison(m_troop);
                    castle->NoticeUnitUpdate();
                    isInValid = false;
                }
            } 
            return isInValid;
        }

        bool TroopToCastle::OnReachProc(Unit* to)
        {
            bool isInValid = true;
            if (to) {
                bool fight = false;
                if (Castle* castle = to->ToCastle()) {
                     if (castle->allianceId() == m_troop->allianceId() && m_troop->allianceId() > 0) {
                        isInValid = OnReachAllianceCastle(castle);
                     }
                     else {
                        if (!castle->IsProtected() && castle->CanAttack(m_troop)) {
                            fight = true;
                        }
                     }
                } else if (auto camp = to->ToCampTemp()) {
                    if (camp->CanAttack(m_troop)) {
                        fight = true;
                    }
                } else if (auto camp = to->ToCampFixed()) {
                    if (camp->CanAttack(m_troop)) {
                        if (camp->troopId() !=  0) {
                            fight = true;
                        } else {
                            fight = false;
                            isInValid = false;
                            OnAttackWin(camp);
                        }
                    }
                } else if (auto res = to->ToResNode()) {
                    if (res->troop() && res->CanOccupy(m_troop)) {
                        fight = true;
                    }
                }

                if (fight) {
                    if (BattleMgr::instance().GenerateBattle(m_troop, to)) {
                        isInValid = false;
                        if (!to->IsDelete()) {
                            to->NoticeUnitUpdate();   //可能被delete, CAMP_FIXED, CAMP_TEMP
                        }
                    }
                }
            }
            return isInValid;
        }

        void TroopToCastle::FinishDeserialize()
        {
            TroopImpl::FinishDeserialize();
            if (m_troop->IsReach()) {
                if (Unit* unit = g_mapMgr->FindUnit(m_troop->to())) {
                    m_troop->SetReachTplId(unit->tpl().id);
                    if (/*Castle* cst =*/ unit->ToCastle()) {
                        m_troop->GoHome();
                    }
                }
            }
        }

        void TroopToCampTemp::OnAttackWin(Unit* unit)
        {
            if (m_troop->type() ==  MapTroopType::CAMP_TEMP) {
                if (!CampingTemp()) {
                    m_troop->GoHome();
                }
            } else if (m_troop->type() ==  MapTroopType::CAMP_TEMP_ATTACK) {
                m_troop->GoHome();
            }
        }

        bool TroopToCampTemp::OnReachProc(Unit* to)
        {
            if (m_troop->type() ==  MapTroopType::CAMP_TEMP) {
                bool isInValid = true;
                if (to ==  nullptr ||  to->IsTree()) {
                    auto camp = CampingTemp();
                    if (camp) {
                        isInValid = false;
                        m_campTemp = camp;
                    } else {
                            if (m_campTemp) {
                                if (BattleMgr::instance().GenerateBattle(m_troop, m_campTemp)) {
                                isInValid = false;
                            }
                        }
                    }
                }
                return isInValid;
            } else if (m_troop->type() ==  MapTroopType::CAMP_TEMP_ATTACK) {
                bool isInValid = true;
                if (to) {
                    bool fight = false;
                    if (auto camp = to->ToCampTemp()) {
                        if (camp->CanAttack(m_troop)) {
                            fight = true;
                        }
                    }

                    if (fight) {
                        if (BattleMgr::instance().GenerateBattle(m_troop, to)) {
                            isInValid = false;
                            if (!to->IsDelete()) {
                                to->NoticeUnitUpdate();   //可能被delete, CAMP_FIXED, CAMP_TEMP
                            }
                        }
                    }
                } else {
                    if (CampingTemp()) {
                        isInValid = false;
                    }
                }
                return isInValid;
            }
            return true;
        }

        void TroopToCampTemp::FinishDeserialize()
        {
            TroopImpl::FinishDeserialize();
            if (m_troop->IsReach()) {
                if (!CampingTemp(true)) {
                    m_troop->GoHome();
                }
            }
        }

//         void TroopToCampFixed::RemoveParentCamp()
//         {
//             if ( m_troop->parentType() == MapTroopType::CAMP_FIXED ) {
//                 //拆除源行营
//                 if ( Unit* unit = g_mapMgr->FindUnit ( m_troop->source() ) ) {
//                     if ( auto campFixed = unit->ToCampFixed() ) {
//                         if ( campFixed->troop() == m_troop ) {
//                             campFixed->RemoveSelf();
//                         }
//                     }
//                 }
//                 //重置
//                 m_troop->ResetParent();
//             }
//         }

        void TroopToCampFixed::OnAttackFailed(Unit* unit)
        {
            if (m_troop->type() ==  MapTroopType::CAMP_FIXED) {
                OnCreateCampFixedFailed();
            }
            m_troop->GoHome();
        }


        void TroopToCampFixed::OnAttackWin(Unit* unit)
        {
            do {
                if (m_troop->type() ==  MapTroopType::CAMP_FIXED) {
                    if (unit) {
                        if (/*auto campTemp = */unit->ToCampTemp()) {
                            if (CampingFixed()) {
                                break;
                            } else {
                                OnCreateCampFixedFailed();
                            }
                        } else if (auto campFixed = unit->ToCampFixed()) {
                            OnCreateCampFixedFailed();
                            if (Occupy(campFixed)) {
                                break;
                            }
                        }
                    }
                } else if (m_troop->type() ==  MapTroopType::CAMP_FIXED_ATTACK) {
                    if (unit) {
                        if (auto campFixed = unit->ToCampFixed()) {
                            if (campFixed->campFixedDurable() <= 0)
                            {
                                campFixed->RemoveSelf();
                            }
                        }
                    }
                } else if (m_troop->type() ==  MapTroopType::CAMP_FIXED_OCCUPY) {
                    if (unit) {
                        if (auto campFixed = unit->ToCampFixed()) {
                            if (campFixed->campFixedDurable() <= 0)
                            {
                                campFixed->RemoveSelf();
                            }
                        }
                    }
                }
                m_troop->GoHome();
            } while (0);
        }

        void TroopToCampFixed::OnDefenceFailed ( Unit* unit )
        {
            if ( m_troop->parentType() == MapTroopType::UNKNOWN ) {
                //行营未被派出去，GoHome
                m_troop->GoHome();
            } else {
                //行营被派出去，ResetParent, GoHome
                m_troop->ResetParent();
                m_troop->GoHome();
            }
        }
        
        bool TroopToCampFixed::OnReachProc(Unit* to)
        {
            if (m_troop->type() ==  MapTroopType::CAMP_FIXED) {
                bool isInValid = true;
                if (to ==  nullptr || to->IsTree()) {
                    if (CampingFixed()) {
                        isInValid = false;
                    }
                } else {
                    bool fight = false;
                    if (auto camp = to->ToCampTemp()) {
                        if (camp->CanAttack(m_troop)) {
                            fight = true;
                        }
                    } else if (auto camp = to->ToCampFixed()) {
                        if (camp->CanAttack(m_troop)) {
                            if (camp->troopId() !=  0) {
                                fight = true;
                            } else {
                                fight = false;
                                isInValid = false;
                                OnAttackWin(camp);
                            }
                        } else if (camp->uid() ==  m_troop->uid() && camp->troopId() ==  0) {
                            fight = false;
                            isInValid = false;
                            // 是自己的行营，直接占领
                            if (Occupy(camp)) {
                                OnCreateCampFixedFailed();
                                isInValid = false;
                            }
                        }
                    }

                    if (fight) {
                        if (BattleMgr::instance().GenerateBattle(m_troop, to)) {
                            isInValid = false;
                            if (!to->IsDelete()) {
                                to->NoticeUnitUpdate();   //可能被delete, CAMP_FIXED, CAMP_TEMP
                            }
                        }
                    }
                }
                return isInValid;
            } else if (m_troop->type() ==  MapTroopType::CAMP_FIXED_ATTACK) {
                bool isInValid = true;
                if (to) {
                    bool fight = false;
                    if (auto camp = to->ToCampTemp()) {
                        if (camp->CanAttack(m_troop)) {
                            fight = true;
                        }
                    } else if (auto camp = to->ToCampFixed()) {
                        if (camp->CanAttack(m_troop)) {
                            fight = true;
                        }
                    }

                    if (fight) {
                        if (BattleMgr::instance().GenerateBattle(m_troop, to)) {
                            isInValid = false;
                            if (!to->IsDelete()) {
                                to->NoticeUnitUpdate();   //可能被delete, CAMP_FIXED, CAMP_TEMP
                            }
                        }
                    }
                }
                return isInValid;
            } else if (m_troop->type() ==  MapTroopType::CAMP_FIXED_OCCUPY || m_troop->type() ==  MapTroopType::ASSIGN) {
                bool isInValid = true;
                if (to) {
                    bool fight = false;
                    if (auto camp = to->ToCampTemp()) {
                        if (camp->CanAttack(m_troop)) {
                            fight = true;
                        }
                    } else if (auto camp = to->ToCampFixed()) {
                        if (camp->CanAttack(m_troop)) {
                            fight = true;
                        } else if (camp->allianceId() == m_troop->allianceId() && (m_troop->allianceId() != 0 || (m_troop->allianceId() == 0 && camp->uid() == m_troop->uid()))) {
                            if (camp->CanGarrison(m_troop)) {
                                camp->Garrison(m_troop);
                                fight = false;
                                isInValid = false;
                                camp->NoticeUnitUpdate();
                            } 
                        }
                    }

                    if (fight) {
                        if (BattleMgr::instance().GenerateBattle(m_troop, to)) {
                            isInValid = false;
                            if (!to->IsDelete()) {
                                to->NoticeUnitUpdate();   //可能被delete, CAMP_FIXED, CAMP_TEMP
                            }
                        }
                    }
                }
                return isInValid;
            }
            return true;
        }

        void TroopToCampFixed::OnReachInvalid()
        {
            TroopImpl::OnReachInvalid();
            if (m_troop->type() ==  MapTroopType::CAMP_FIXED) {
                OnCreateCampFixedFailed();
            }
        }

        void TroopToCampFixed::OnCreateCampFixedFailed()
        {
//             auto & campFixedConsumed = g_tploader->configure().troopConfig.campFixedConfig;
//             m_troop->AddResource(ResourceType::FOOD,  campFixedConsumed.food);
//             m_troop->AddResource(ResourceType::WOOD,  campFixedConsumed.wood);
//             m_troop->AddResource(ResourceType::IRON,  campFixedConsumed.iron);
//             m_troop->AddResource(ResourceType::STONE,  campFixedConsumed.stone);
            m_troop->agent().msgQueue().AppendMsgCreateCampFixedFailed();
        }

        bool TroopToScout::Scout(Unit* unit)
        {
            if (unit) {
                AttackType winner = AttackType::ATTACK;
                std::vector< tpl::ScoutTypeTpl > scoutTypesTpl;
                //attack info
                Agent& attAgent = m_troop->agent();
                map::tpl::m_tploader->GetScoutType(/*attAgent.watchtowerLevel()*/100,  scoutTypesTpl, true);
                // map::tpl::m_tploader->GetScoutType(100,  scoutTypesTpl);
                if (scoutTypesTpl.empty()) {
                    return false;
                }

                std::vector< WATCHTOWER_SCOUT_TYPE > effectiveScoutTypes;
                msgqueue::MsgPlayerInfo attacker;
                msgqueue::MsgScoutDefenderInfo defender;
                attacker.SetData(attAgent);
                base::DataTable& tableResult = defender.result;         // 侦查结果

                //defender info
                bool defenderIsFalse = false;
                Agent* defAgent = nullptr;

                // 得到上下浮动20%的值
                auto getRoughValue = [&](int& value) {
                    int max = value * (1 + 0.2);    
                    int min = value * (1 - 0.2);    
                    value = framework.random().GenRandomNum(min, max + 1);
                };

                // 侦查玩家城池
                auto scoutCastle = [&](Castle* castle) {
                    if (castle->IsAntiScout()) {
                        winner = AttackType::DEFENSE;
                    }
                    defAgent = &castle->agent();
                    defenderIsFalse = defAgent->IsFalseArmy();
                    if (winner == AttackType::ATTACK) {
                        Agent& agent = castle->agent();
                        for (auto tpl : scoutTypesTpl) {
                            bool isValid = true;
                            base::DataTable table;
                            auto type = tpl.type;
                            switch (type) {
                                // 101
                                case WATCHTOWER_SCOUT_TYPE::PLAYER_ALLIANCE: {
                                    table.Set("allianceName",  agent.allianceName());
                                }
                                break;
                                // 102
                                case WATCHTOWER_SCOUT_TYPE::PLAYER_NAME_LV: {
                                    table.Set("nickname",  agent.nickname());
                                    table.Set("headId",  agent.headId());
                                    table.Set("level",  agent.level());
                                }
                                break;
                                // 103
                                case WATCHTOWER_SCOUT_TYPE::PLAYER_RESOURCE_COUNT: {
                                    int foodOutput = 0;
                                    int woodOutput = 0;
                                    int ironOutput = 0;
                                    int stoneOutput = 0;
                                    agent.GetCollectOutput(foodOutput, woodOutput, ironOutput, stoneOutput);
                                    table.Set("food", agent.food());
                                    table.Set("wood", agent.wood());
                                    table.Set("iron", agent.iron());
                                    table.Set("stone", agent.stone());
                                    table.Set("foodOutput", foodOutput);
                                    table.Set("woodOutput", woodOutput);
                                    table.Set("ironOutput", ironOutput);
                                    table.Set("stoneOutput", stoneOutput);
                                }
                                break;
                                // 104
                                case WATCHTOWER_SCOUT_TYPE::PLAYER_GLOBAL_BUFF: {
                                    DataTable buffsTable;
                                    unordered_map<int, info::BuffInfo> buffs = defAgent->GetValidBuffs(1, true);
                                    int i = 0;
                                    for (auto it = buffs.begin(); it != buffs.end(); ++it) {
                                        const info::BuffInfo& info = it->second;
                                        DataTable buffTable;
                                        buffTable.Set("type", it->first);
                                        buffTable.Set("param1", info.param1);
                                        buffTable.Set("attr", info.attr);
                                        buffsTable.Set(++i, buffTable);
                                    }
                                    table.Set("cityBuff ", buffsTable);
                                }
                                break;
                                // 105
                                case WATCHTOWER_SCOUT_TYPE::WALL_DEFENSE_ARMY_COUNT: {
                                    int count = 0;
                                    if(ArmyList* armyList = defAgent->TeamArmyList(defAgent->getDefTeam()))
                                    {
                                        count = armyList->ArmyCount(ArmyState::NORMAL);
                                    }
                                    getRoughValue(count);
                                    table.Set("armyCount", count);
                                }
                                break;
                                // 106
                                case WATCHTOWER_SCOUT_TYPE::WALL_DEFENSE_HERO_NAME: {
                                    DataTable Table;
                                    if(ArmyList* armyList = defAgent->TeamArmyList(defAgent->getDefTeam()))
                                    {
                                        int i = 0;
                                        for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                            base::DataTable subTable;
                                            HeroInfo& heroInfo = it->second.heroInfo();
                                            subTable.Set("heroId", heroInfo.tplId());
                                            Table.Set(++i, subTable);
                                        }
                                    }
                                    table.Set("heroNames", Table);
                                }
                                break;
                                // 107
                                case WATCHTOWER_SCOUT_TYPE::WALL_DEFENSE_HERO_LEVEL: {
                                    DataTable Table;
                                    if(ArmyList* armyList = defAgent->TeamArmyList(defAgent->getDefTeam()))
                                    {
                                        int i = 0;
                                        for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                            base::DataTable subTable;
                                            HeroInfo& heroInfo = it->second.heroInfo();
                                            subTable.Set("level", heroInfo.level());
                                            Table.Set(++i, subTable);
                                        }
                                    }
                                    table.Set("heroLevels", Table);
                                }
                                break;
                                // 108
                                case WATCHTOWER_SCOUT_TYPE::WALL_DEFENSE_ARMY_TYPE_LEVEL: {
                                    DataTable Table;
                                    if(ArmyList* armyList = defAgent->TeamArmyList(defAgent->getDefTeam()))
                                    {
                                        int i = 0;
                                        for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                            DataTable subTable;
                                            ArmyInfo& armyInfo = it->second.armyInfo();
                                            subTable.Set("type", armyInfo.type());
                                            subTable.Set("level", armyInfo.level());
                                            Table.Set(++i, subTable);
                                        }
                                    }
                                    table.Set("armyTypeLevels", Table);
                                }
                                break;
                                // 109
                                case WATCHTOWER_SCOUT_TYPE::WALL_DEFENSE_ROUGH_COUNT_DURABLE: {
                                    int armySetCount = 0;
                                    int castleDurable = 0;
                                    int castleDurableMax = 0;
                                    castleDurable = defAgent->castle()->cityDefense();
                                    castleDurableMax = defAgent->castle()->cityDefenseMax();
                                    if(ArmyInfo* armyInfo = defAgent->armySet().GetArmy((int)model::ArmysType::REDIF))
                                    {
                                        armySetCount = armySetCount + armyInfo->count(model::ArmyState::NORMAL);
                                    }
                                    getRoughValue(armySetCount);
                                    getRoughValue(castleDurable);
                                    table.Set("castleDurable", castleDurable);
                                    table.Set("castleDurableMax", castleDurableMax);
                                    table.Set("armySetCount", armySetCount);
                                }
                                break;
                                // 110
                                case WATCHTOWER_SCOUT_TYPE::WALL_DEFENSE_ALL_TEAM_ARMY_ROUGH_COUNT: {
                                    int armyCount = 0;
                                    int count = 0;
                                    for (auto troop :  defAgent->castle()->troops()) {
                                        armyCount = armyCount + troop->armyList()->ArmyCount(ArmyState::NORMAL);
                                        ++count;
                                    }
                                    // 玩家自己的
                                    for (int j = 0; j < 5; ++j)
                                    {
                                        auto armyList = defAgent->armyListArray()[j];
                                        if (armyList.team() != 0 && !armyList.IsOut() && !armyList.IsAllDie())
                                        {
                                            armyCount = armyCount + armyList.ArmyCount(ArmyState::NORMAL);
                                            ++count;
                                        }
                                    }
                                    getRoughValue(armyCount);
                                    table.Set("teamArmyRoughCount", armyCount);
                                    table.Set("teamCount", count);
                                }
                                break;
                                // 111
                                case WATCHTOWER_SCOUT_TYPE::WALL_DEFENSE_TEAM_ARMY_ROUGH_COUNT: {
                                    DataTable Table;
                                    int i = 0;
                                    // 同盟增援的
                                    for (auto troop :  defAgent->castle()->troops()) {
                                        base::DataTable subTable;
                                        int armyCount = troop->armyList()->ArmyCount(ArmyState::NORMAL);
                                        getRoughValue(armyCount);
                                        subTable.Set("ArmyCount", armyCount);
                                        Table.Set(++i, subTable);
                                    }
                                    // 玩家自己的
                                    for (int j = 0; j < 5; ++j)
                                    {
                                        auto armyList = defAgent->armyListArray()[j];
                                        if (armyList.team() != 0 && !armyList.IsOut() && !armyList.IsAllDie())
                                        {
                                            base::DataTable subTable;
                                            int armyCount = armyList.ArmyCount(ArmyState::NORMAL);
                                            getRoughValue(armyCount);
                                            subTable.Set("ArmyCount", armyCount);
                                            Table.Set(++i, subTable);
                                        }
                                    }
                                    table.Set("allTeamArmyRoughCount", Table);
                                }
                                break;
                                // 112
                                case WATCHTOWER_SCOUT_TYPE::WALL_DEFENSE_TYPE_COUNT: {
                                    int rollingLogsCount = 0;
                                    int grindStone = 0;
                                    int chevalDeFrise = 0;
                                    int kerosene = 0;
                                    if(TrapInfo* trapInfo = defAgent->trapSet().GetTrap((int)model::ArmysType::ROLLING_LOGS))
                                    {
                                        rollingLogsCount = trapInfo->count(model::ArmyState::NORMAL);
                                    }
                                    if(TrapInfo* trapInfo = defAgent->trapSet().GetTrap((int)model::ArmysType::GRIND_STONE))
                                    {
                                        grindStone = trapInfo->count(model::ArmyState::NORMAL);
                                    }
                                    if(TrapInfo* trapInfo = defAgent->trapSet().GetTrap((int)model::ArmysType::CHEVAL_DE_FRISE))
                                    {
                                        chevalDeFrise = trapInfo->count(model::ArmyState::NORMAL);
                                    }
                                    if(TrapInfo* trapInfo = defAgent->trapSet().GetTrap((int)model::ArmysType::KEROSENE))
                                    {
                                        kerosene = trapInfo->count(model::ArmyState::NORMAL);
                                    }
                                    getRoughValue(rollingLogsCount);
                                    getRoughValue(grindStone);
                                    getRoughValue(chevalDeFrise);
                                    getRoughValue(kerosene);
                                    DataTable Table;
                                    Table.Set((int)model::ArmysType::ROLLING_LOGS, rollingLogsCount);
                                    Table.Set((int)model::ArmysType::GRIND_STONE, grindStone);
                                    Table.Set((int)model::ArmysType::CHEVAL_DE_FRISE, chevalDeFrise);
                                    Table.Set((int)model::ArmysType::KEROSENE, kerosene);

                                    table.Set("trapSet", Table);
                                }
                                break;
                                // 113
                                case WATCHTOWER_SCOUT_TYPE::ALL_TEAM_HERO_NAME: {
                                    DataTable Table;
                                    int i = 0;
                                    for (auto troop :  defAgent->castle()->troops()) {
                                        DataTable subTable;
                                        ArmyList* armyList = troop->armyList();
                                        int j = 0;
                                        for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                            base::DataTable subSubTable;
                                            HeroInfo& heroInfo = it->second.heroInfo();
                                            subSubTable.Set("heroId", heroInfo.tplId());
                                            subTable.Set(++j, subSubTable);
                                        }
                                        Table.Set(++i, subTable);
                                    }
                                    // 玩家自己的
                                    for (int k = 0; k < 5; ++k)
                                    {
                                        auto armyList = defAgent->armyListArray()[k];
                                        if (armyList.team() != 0 && !armyList.IsOut() && !armyList.IsAllDie())
                                        {
                                            DataTable subTable;
                                            int j = 0;
                                            for (auto it = armyList.armies().begin(); it != armyList.armies().end(); ++it) {
                                                base::DataTable subSubTable;
                                                HeroInfo& heroInfo = it->second.heroInfo();
                                                subSubTable.Set("heroId", heroInfo.tplId());
                                                subTable.Set(++j, subSubTable);
                                            }
                                            Table.Set(++i, subTable);
                                        }
                                    }

                                    table.Set("allTeamNames", Table);
                                }
                                break;
                                // 114
                                case WATCHTOWER_SCOUT_TYPE::ALL_TEAM_HERO_LEVEL: {
                                    DataTable Table;
                                    int i = 0;
                                    for (auto troop :  defAgent->castle()->troops()) {
                                        DataTable subTable;
                                        ArmyList* armyList = troop->armyList();
                                        int j = 0;
                                        for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                            base::DataTable subSubTable;
                                            HeroInfo& heroInfo = it->second.heroInfo();
                                            subSubTable.Set("level", heroInfo.level());
                                            subTable.Set(++j, subSubTable);
                                        }
                                        Table.Set(++i, subTable);
                                    }
                                    // 玩家自己的
                                    for (int k = 0; k < 5; ++k)
                                    {
                                        auto armyList = defAgent->armyListArray()[k];
                                        if (armyList.team() != 0 && !armyList.IsOut() && !armyList.IsAllDie())
                                        {
                                            DataTable subTable;
                                            int j = 0;
                                            for (auto it = armyList.armies().begin(); it != armyList.armies().end(); ++it) {
                                                base::DataTable subSubTable;
                                                HeroInfo& heroInfo = it->second.heroInfo();
                                                subSubTable.Set("level", heroInfo.level());
                                                subTable.Set(++j, subSubTable);
                                            }
                                            Table.Set(++i, subTable);
                                        }
                                    }

                                    table.Set("allTeamLevels", Table);
                                }
                                break;
                                // 115
                                case WATCHTOWER_SCOUT_TYPE::ALL_TEAM_ARMY_TYPE_LEVEL: {
                                    DataTable Table;
                                    int i = 0;
                                    for (auto troop :  defAgent->castle()->troops()) {
                                        DataTable subTable;
                                        ArmyList* armyList = troop->armyList();
                                        int j = 0;
                                        for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                            base::DataTable subSubTable;
                                            ArmyInfo& armyInfo = it->second.armyInfo();
                                            subSubTable.Set("type", armyInfo.type());
                                            subSubTable.Set("level", armyInfo.level());
                                            subTable.Set(++j, subSubTable);
                                        }
                                        Table.Set(++i, subTable);
                                    }
                                    // 玩家自己的
                                    for (int k = 0; k < 5; ++k)
                                    {
                                        auto armyList = defAgent->armyListArray()[k];
                                        if (armyList.team() != 0 && !armyList.IsOut() && !armyList.IsAllDie())
                                        {
                                            DataTable subTable;
                                            int j = 0;
                                            for (auto it = armyList.armies().begin(); it != armyList.armies().end(); ++it) {
                                                base::DataTable subSubTable;
                                                ArmyInfo& armyInfo = it->second.armyInfo();
                                                subSubTable.Set("type", armyInfo.type());
                                                subSubTable.Set("level", armyInfo.level());
                                                subTable.Set(++j, subSubTable);
                                            }
                                            Table.Set(++i, subTable);
                                        }
                                    }

                                    table.Set("allTeamTypeLevels", Table);
                                }
                                break;
                                // 116
                                case WATCHTOWER_SCOUT_TYPE::ALL_TEAM_ARMY_COUNT: {
                                    int armyCount = 0;
                                    for (auto troop :  defAgent->castle()->troops()) {
                                        armyCount = armyCount + troop->armyList()->ArmyCount(ArmyState::NORMAL);
                                    }
                                    // 玩家自己的
                                    for (int j = 0; j < 5; ++j)
                                    {
                                        auto armyList = defAgent->armyListArray()[j];
                                        if (armyList.team() != 0 && !armyList.IsOut() && !armyList.IsAllDie())
                                        {
                                            DataTable subTable;
                                            armyCount = armyCount + armyList.ArmyCount(ArmyState::NORMAL);
                                        }
                                    }
                                    table.Set("allTeamArmyCount", armyCount);
                                }
                                break;
                                // 117
                                case WATCHTOWER_SCOUT_TYPE::WALL_DEFENSE_COUNT: {
                                    DataTable Table;
                                    int rollingLogsCount = 0;
                                    int grindStone = 0;
                                    int chevalDeFrise = 0;
                                    int kerosene = 0;
                                    if(TrapInfo* trapInfo = defAgent->trapSet().GetTrap((int)model::ArmysType::ROLLING_LOGS))
                                    {
                                        rollingLogsCount = trapInfo->count(model::ArmyState::NORMAL);
                                    }
                                    if(TrapInfo* trapInfo = defAgent->trapSet().GetTrap((int)model::ArmysType::GRIND_STONE))
                                    {
                                        grindStone = trapInfo->count(model::ArmyState::NORMAL);
                                    }
                                    if(TrapInfo* trapInfo = defAgent->trapSet().GetTrap((int)model::ArmysType::CHEVAL_DE_FRISE))
                                    {
                                        chevalDeFrise = trapInfo->count(model::ArmyState::NORMAL);
                                    }
                                    if(TrapInfo* trapInfo = defAgent->trapSet().GetTrap((int)model::ArmysType::KEROSENE))
                                    {
                                        kerosene = trapInfo->count(model::ArmyState::NORMAL);
                                    }
                                    Table.Set((int)model::ArmysType::ROLLING_LOGS, rollingLogsCount);
                                    Table.Set((int)model::ArmysType::GRIND_STONE, grindStone);
                                    Table.Set((int)model::ArmysType::CHEVAL_DE_FRISE, chevalDeFrise);
                                    Table.Set((int)model::ArmysType::KEROSENE, kerosene);
                                    table.Set("trapSet", Table);
                                }
                                break;
                                // 118
                                case WATCHTOWER_SCOUT_TYPE::TEAM_ARMY_COUNT: {
                                    int i = 0;
                                    base::DataTable Table;
                                    for (auto troop :  defAgent->castle()->troops()) {
                                        int j = 0;
                                        base::DataTable subTable;
                                        for (auto it = troop->armyList()->armies().begin(); it != troop->armyList()->armies().end(); ++it) {
                                            base::DataTable subSubTable;
                                            ArmyInfo& armyInfo = it->second.armyInfo();
                                            subSubTable.Set("count", armyInfo.count(ArmyState::NORMAL));
                                            subTable.Set(++j, subSubTable);
                                        }
                                        Table.Set(++i, subTable);
                                    }
                                    // 玩家自己的
                                    for (int j = 0; j < 5; ++j)
                                    {
                                        auto armyList = defAgent->armyListArray()[j];
                                        if (armyList.team() != 0 && !armyList.IsOut() && !armyList.IsAllDie())
                                        {
                                            int m = 0;
                                            base::DataTable subTable;
                                            for (auto it = armyList.armies().begin(); it != armyList.armies().end(); ++it) {
                                                base::DataTable subSubTable;
                                                ArmyInfo& armyInfo = it->second.armyInfo();
                                                subSubTable.Set("count", armyInfo.count(ArmyState::NORMAL));
                                                subTable.Set(++m, subSubTable);
                                            }
                                            Table.Set(++i, subTable);
                                        }
                                    }
                                    table.Set("teamArmyCount", Table);
                                }
                                break;
                                // 119
                                case WATCHTOWER_SCOUT_TYPE::CATAPULT_LEVEL: {
                                    table.Set("catapultLevel", defAgent->turretLevel());
                                }
                                break;
                                // 120
                                case WATCHTOWER_SCOUT_TYPE::TECHNOLOGY_INFO: {
                                    DataTable technologiesTables;
                                    int i = 0;
                                    for (auto troop :  defAgent->castle()->troops()) {
                                        DataTable technologiesTable;
                                        auto& technologies = troop->agent().technologies();
                                        for (auto it = technologies.begin(); it != technologies.end(); ++it) {
                                            technologiesTable.Set(it->first, it->second);
                                        }
                                        technologiesTables.Set(++i, technologiesTable);
                                    }
                                    // 玩家自己的
                                    for (int j = 0; j < 5; ++j)
                                    {
                                        auto armyList = defAgent->armyListArray()[j];
                                        if (armyList.team() != 0 && !armyList.IsOut() && !armyList.IsAllDie())
                                        {
                                            DataTable technologiesTable;
                                            auto& technologies = defAgent->technologies();
                                            for (auto it = technologies.begin(); it != technologies.end(); ++it) {
                                                technologiesTable.Set(it->first, it->second);
                                            }
                                            technologiesTables.Set(++i, technologiesTable);
                                        }
                                    }
                                    table.Set("allTechnologies", technologiesTables);
                                    }
                                break;
                                // 121
                                case WATCHTOWER_SCOUT_TYPE::ALL_HERO_SKILL_CONFIGURATION: {
                                    base::DataTable Table;
                                    int j = 0;
                                    for (auto troop : defAgent->castle()->troops()) {
                                        ArmyList* armyList = troop->armyList();
                                        base::DataTable subTable;
                                        int i = 0;
                                        for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                            base::DataTable subSubTable;
                                            HeroInfo& heroInfo = it->second.heroInfo();
                                            int m = 0;
                                            for (auto & skill : heroInfo.skill())
                                            {
                                                base::DataTable subSubSubTable;
                                                subSubSubTable.Set("id", skill.id);
                                                subSubSubTable.Set("level", skill.level);
                                                subSubTable.Set(++m, subSubSubTable);
                                            }
                                            subTable.Set(++i, subSubTable);
                                        }
                                        Table.Set(++j,subTable);
                                    }

                                    // 玩家自己的
                                    for (int k = 0; k < 5; ++k)
                                    {
                                        auto armyList = defAgent->armyListArray()[k];
                                        if (armyList.team() != 0 && !armyList.IsOut() && !armyList.IsAllDie())
                                        {
                                            base::DataTable subTable;
                                            int i = 0;
                                            for (auto it = armyList.armies().begin(); it != armyList.armies().end(); ++it) {
                                                base::DataTable subSubTable;
                                                HeroInfo& heroInfo = it->second.heroInfo();
                                                int m = 0;
                                                for (auto & skill : heroInfo.skill())
                                                {
                                                    base::DataTable subSubSubTable;
                                                    subSubSubTable.Set("id", skill.id);
                                                    subSubSubTable.Set("level", skill.level);
                                                    subSubTable.Set(++m, subSubSubTable);
                                                }
                                                subTable.Set(++i, subSubTable);
                                            }
                                            Table.Set(++j,subTable);
                                        }
                                    }
                                    table.Set("heroSkillConf", Table);
                                }
                                break;
                                // 122
                                case WATCHTOWER_SCOUT_TYPE::SCOUT_SPEED_UP: {
                                    // 增加侦察速度，在出征的时候判断
                                }
                                break;
                                // 123
                                case WATCHTOWER_SCOUT_TYPE::CASTLE_TEAM_PPLAYER_NHL_ALLIANCE_NAME: {
                                    DataTable castleTeamPplayerNhlAllianceName;
                                    int i = 0;
                                    for (auto troop :  defAgent->castle()->troops()) {
                                        DataTable subTable;
                                        subTable.Set("name", troop->agent().nickname());
                                        subTable.Set("headId", troop->agent().headId());
                                        subTable.Set("level", troop->agent().level());
                                        castleTeamPplayerNhlAllianceName.Set(++i, subTable);
                                    }
                                    // 玩家自己的
                                    for (int j = 0; j < 5; ++j)
                                    {
                                        auto armyList = defAgent->armyListArray()[j];
                                        if (armyList.team() != 0 && !armyList.IsOut() && !armyList.IsAllDie())
                                        {
                                            DataTable subTable;
                                            subTable.Set("name", defAgent->nickname());
                                            subTable.Set("headId", defAgent->headId());
                                            subTable.Set("level", defAgent->level());
                                            castleTeamPplayerNhlAllianceName.Set(++i, subTable);
                                        }
                                    }
                                    table.Set("castleTeamPplayerNhlAllianceName", castleTeamPplayerNhlAllianceName);
                                }
                                break;
                                default:
                                    isValid = false;
                                    break;
                            }
                            if (isValid) {
                                tableResult.Set(tpl.uiKeywords, table);
                                effectiveScoutTypes.push_back(type);
                            }
                        }
                    }
                };

                // 侦查名城
                auto scoutCity = [&](FamousCity* city) {
                    ArmyList* firstArmyList = nullptr;
                    std::list<ArmyList*> armyLists;
                    if (city->isOccupy())
                    {
                        firstArmyList = city->troop()->armyList();
                        for (auto tp : city->troops())
                        {
                            armyLists.push_back(tp->armyList());
                        }
                    }
                    else
                    {
                        firstArmyList = city->DefArmyList();
                    }
                    
                    for (auto tpl : scoutTypesTpl) {
                        bool isValid = true;
                        base::DataTable table;
                        auto type = tpl.type;
                        switch (type) {
                            // 301
                            case WATCHTOWER_SCOUT_TYPE::CITY_NAME: {
                                table.Set("cityName", city->cityTpl()->name);
                            }
                            break;
                            // 302
                            case WATCHTOWER_SCOUT_TYPE::CITY_PLAYER_ALLIANCE: {
                                if (!city->isOccupy()) {
                                    isValid = false;
                                    break;
                                }
                                if (auto as = g_mapMgr->alliance().FindAllianceSimple(city->allianceId()))
                                {
                                    table.Set("allianceName", as->info.name);
                                }
                            }
                            break;
                            // 303
                            case WATCHTOWER_SCOUT_TYPE::CITY_DEFENSE_TEAM_ONE_ARMY_ROUGH_COUNT: {
                                int armyCount = 0;
                                if (firstArmyList)
                                {
                                    armyCount = firstArmyList->ArmyCount(ArmyState::NORMAL);
                                }
                                getRoughValue(armyCount);
                                table.Set("oneArmyRoughCount", armyCount);
                            }
                            break;
                            // 304
                            case WATCHTOWER_SCOUT_TYPE::CITY_DEFENSE_TEAM_ONE_HERO_NAME: {
                                base::DataTable Table;
                                if (firstArmyList)
                                {
                                    int i = 0;
                                    for (auto it = firstArmyList->armies().begin(); it != firstArmyList->armies().end(); ++it) {
                                        base::DataTable subTable;
                                        HeroInfo& heroInfo = it->second.heroInfo();
                                        subTable.Set("heroId", heroInfo.tplId());
                                        Table.Set(++i, subTable);
                                    }
                                }
                                table.Set("oneTeamHeroName", Table);
                            }
                            break;
                            // 305
                            case WATCHTOWER_SCOUT_TYPE::CITY_DEFENSE_TEAM_ONE_HERO_LEVEL: {
                                base::DataTable Table;
                                if (firstArmyList)
                                {
                                    int i = 0;
                                    for (auto it = firstArmyList->armies().begin(); it != firstArmyList->armies().end(); ++it) {
                                        base::DataTable subTable;
                                        HeroInfo& heroInfo = it->second.heroInfo();
                                        subTable.Set("level", heroInfo.level());
                                        Table.Set(++i, subTable);
                                    }
                                }
                                table.Set("oneTeamHeroLevel", Table);
                            }
                            break;
                            // 306
                            case WATCHTOWER_SCOUT_TYPE::CITY_DEFENSE_TEAM_ONE_ARMY_TYPE_LEVEL: {
                                base::DataTable Table;
                                if (firstArmyList)
                                {
                                    int i = 0;
                                    for (auto it = firstArmyList->armies().begin(); it != firstArmyList->armies().end(); ++it) {
                                        base::DataTable subTable;
                                        ArmyInfo& armyInfo = it->second.armyInfo();
                                        subTable.Set("type", armyInfo.type());
                                        subTable.Set("level", armyInfo.level());
                                        Table.Set(++i, subTable);
                                    }
                                }
                                table.Set("oneTeamArmyLevelTypeTable", Table);
                            }
                            break;
                            // 307
                            case WATCHTOWER_SCOUT_TYPE::CITY_ALL_DEFENSE_TEAM_ARMY_ROUGH_COUNT: {
                                int armyCount = 0;
                                int count = 0;
                                if (city->isOccupy())
                                {
                                    for (auto armyList : armyLists)
                                    {
                                        armyCount = armyCount + armyList->ArmyCount(ArmyState::NORMAL);
                                        count++;
                                    }    
                                }
                                else
                                {
                                    for (auto armyList : city->armyLists())
                                    {
                                        armyCount = armyCount + armyList.ArmyCount(ArmyState::NORMAL);
                                        count++;
                                    }
                                }
                                getRoughValue(armyCount);
                                table.Set("cityAllDefenseTeamArmyRoughCount", armyCount);
                                table.Set("teamCount", count);
                            }
                            break;
                            // 308
                            case WATCHTOWER_SCOUT_TYPE::CITY_DEFENSE_TEAM_ARMY_ROUGH_COUNT: {
                                base::DataTable Table;
                                if (city->isOccupy())
                                {
                                    int i = 0;
                                    for (auto armyList : armyLists)
                                    {
                                        base::DataTable subTable;
                                        int armyCount = 0;
                                        armyCount = armyList->ArmyCount(ArmyState::NORMAL);
                                        getRoughValue(armyCount);
                                        subTable.Set("armyRoughCount",armyCount);
                                        Table.Set(++i,subTable);
                                    }
                                }
                                else
                                {
                                    int i = 0;
                                    for (auto armyList : city->armyLists())
                                    {
                                        base::DataTable subTable;
                                        int armyCount = 0;
                                        armyCount = armyList.ArmyCount(ArmyState::NORMAL);
                                        getRoughValue(armyCount);
                                        subTable.Set("armyRoughCount",armyCount);
                                        Table.Set(++i,subTable);
                                    }
                                }
                                table.Set("cityDefenseTeamArmyRoughCount", Table);
                            }
                            break;
                            // 309
                            case WATCHTOWER_SCOUT_TYPE::CITY_DEFENSE_TEAM_HERO_NAME: {
                                base::DataTable Table;
                                if (city->isOccupy())
                                {
                                    int j = 0;
                                    for (auto armyList : armyLists)
                                    {
                                        base::DataTable subTable;
                                        int i = 0;
                                        for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                            base::DataTable subSubTable;
                                            HeroInfo& heroInfo = it->second.heroInfo();
                                            subSubTable.Set("heroId", heroInfo.tplId());
                                            subTable.Set(++i, subSubTable);
                                        }
                                        Table.Set(++j,subTable);
                                    }
                                }
                                else
                                {
                                    int j = 0;
                                    for (auto armyList : city->armyLists())
                                    {
                                        base::DataTable subTable;
                                        int i = 0;
                                        for (auto it = armyList.armies().begin(); it != armyList.armies().end(); ++it) {
                                            base::DataTable subSubTable;
                                            HeroInfo& heroInfo = it->second.heroInfo();
                                            subSubTable.Set("heroId", heroInfo.tplId());
                                            subTable.Set(++i, subSubTable);
                                        }
                                        Table.Set(++j,subTable);
                                    }
                                }
                                
                                table.Set("allHeroNames", Table);
                            }
                            break;
                            // 310
                            case WATCHTOWER_SCOUT_TYPE::CITY_DEFENSE_TEAM_HERO_LEVEL: {
                                base::DataTable Table;
                                if (city->isOccupy())
                                {
                                    int j = 0;
                                    for (auto armyList : armyLists)
                                    {
                                        base::DataTable subTable;
                                        int i = 0;
                                        for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                            base::DataTable subSubTable;
                                            HeroInfo& heroInfo = it->second.heroInfo();
                                            subSubTable.Set("level", heroInfo.level());
                                            subTable.Set(++i, subSubTable);
                                        }
                                        Table.Set(++j,subTable);
                                    }
                                }
                                else
                                {
                                    int j = 0;
                                    for (auto armyList : city->armyLists())
                                    {
                                        base::DataTable subTable;
                                        int i = 0;
                                        for (auto it = armyList.armies().begin(); it != armyList.armies().end(); ++it) {
                                            base::DataTable subSubTable;
                                            HeroInfo& heroInfo = it->second.heroInfo();
                                            subSubTable.Set("level", heroInfo.level());
                                            subTable.Set(++i, subSubTable);
                                        }
                                        Table.Set(++j,subTable);
                                    }
                                }
                                
                                table.Set("allHeroLevels", Table);
                            }
                            break;
                            // 311
                            case WATCHTOWER_SCOUT_TYPE::CITY_DEFENSE_TEAM_ARMY_TYPE_LEVEL: {
                                base::DataTable Table;
                                if (city->isOccupy())
                                {
                                    int i = 0;
                                    for (auto armyList : armyLists)
                                    {
                                        int j = 0;
                                        base::DataTable subTable;
                                        for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                            base::DataTable subSubTable;
                                            ArmyInfo& armyInfo = it->second.armyInfo();
                                            subSubTable.Set("type", armyInfo.type());
                                            subSubTable.Set("level", armyInfo.level());
                                            subTable.Set(++j, subSubTable);
                                        }
                                        Table.Set(++i,subTable);
                                    }
                                }
                                else
                                {
                                    int i = 0;
                                    for (auto armyList : city->armyLists())
                                    {
                                        int j = 0;
                                        base::DataTable subTable;
                                        for (auto it = armyList.armies().begin(); it != armyList.armies().end(); ++it) {
                                            base::DataTable subSubTable;
                                            ArmyInfo& armyInfo = it->second.armyInfo();
                                            subSubTable.Set("type", armyInfo.type());
                                            subSubTable.Set("level", armyInfo.level());
                                            subTable.Set(++j, subSubTable);
                                        }
                                        Table.Set(++i,subTable);
                                    }
                                }
                                
                                table.Set("allArmyLevelTypes", Table);
                            }
                            break;
                            // 312
                            case WATCHTOWER_SCOUT_TYPE::CITY_DEFENSE_TEAM_ALLIANCE_TECH_INFO: {
                                // todo
                                // 地图上还没有联盟科技等级信息
                                if (!city->isOccupy())
                                {
                                    isValid = false;
                                    break;
                                }
                                base::DataTable Table;
                                int i = 0;
                                for (auto troop : city->troops())
                                {
                                    const AllianceSimple* alliance = troop->agent().alliance();
                                    if (alliance)
                                    {
                                        base::DataTable subTable;
                                        int j = 0;
                                        for (auto scienceInfo : alliance->sciences)
                                        {
                                            base::DataTable subSubTable;
                                            subSubTable.Set("groupId", scienceInfo.groupId);
                                            subSubTable.Set("tplId", scienceInfo.tplId);
                                            subSubTable.Set("level", scienceInfo.level);
                                            subTable.Set(++j, subSubTable);
                                        }
                                        Table.Set(++i, subTable);
                                    }
                                }
                                table.Set("allianceTechInfo", Table);
                            }
                            break;
                            // 313
                            case WATCHTOWER_SCOUT_TYPE::CITY_ALL_DEFENSE_TEAM_ARMY_COUNT: {
                                int armyCount = 0;
                                if (city->isOccupy())
                                {
                                   for (auto armyList : armyLists)
                                    {
                                        armyCount = armyCount + armyList->ArmyCount(ArmyState::NORMAL);
                                    } 
                                }
                                else
                                {
                                    for (auto armyList : city->armyLists())
                                    {
                                        armyCount = armyCount + armyList.ArmyCount(ArmyState::NORMAL);
                                    } 
                                }
                                
                                table.Set("allArmyCount", armyCount);
                            }
                            break;
                            // 314
                            case WATCHTOWER_SCOUT_TYPE::CITY_DEFENSE_TEAM_ARMY_COUNT: {
                                base::DataTable Table;
                                if (city->isOccupy())
                                {
                                    int i = 0;
                                    for (auto armyList : armyLists)
                                    {
                                        base::DataTable subTable;
                                        int armyCount = 0;
                                        armyCount = armyList->ArmyCount(ArmyState::NORMAL);
                                        subTable.Set("armyCount",armyCount);
                                        Table.Set(++i, subTable);
                                    } 
                                }
                                else
                                {
                                    int i = 0;
                                    for (auto armyList : city->armyLists())
                                    {
                                        base::DataTable subTable;
                                        int armyCount = 0;
                                        armyCount = armyList.ArmyCount(ArmyState::NORMAL);
                                        subTable.Set("ArmyCount",armyCount);
                                        Table.Set(++i, subTable);
                                    }
                                }
                                
                                table.Set("allArmyCounts", Table);
                            }
                            break;
                            // 315
                            case WATCHTOWER_SCOUT_TYPE::CITY_DEFENSE_TEAM_TECH_INFO: {
                                if (!city->isOccupy())
                                {
                                    isValid = false;
                                    break;
                                }
                                int i = 0;
                                DataTable technologiesTables;
                                for (auto tp : city->troops())
                                {
                                    DataTable technologiesTable;
                                    auto& technologies = tp->agent().technologies();
                                    for (auto it = technologies.begin(); it != technologies.end(); ++it) {
                                        technologiesTable.Set(it->first, it->second);
                                    }
                                    technologiesTables.Set(++i, technologiesTable);
                                } 
                                table.Set("allTechnologies", technologiesTables);   

                            }
                            break;
                            // 316
                            case WATCHTOWER_SCOUT_TYPE::CITY_DEFENSE_TEAM_HERO_SKILL_CONFIGURATION: {
                                base::DataTable Table;
                                if (city->isOccupy())
                                {
                                    int j = 0;
                                    for (auto armyList : armyLists)
                                    {
                                        base::DataTable subTable;
                                        int i = 0;
                                        for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                            base::DataTable subSubTable;
                                            HeroInfo& heroInfo = it->second.heroInfo();
                                            int m = 0;
                                            for (auto & skill : heroInfo.skill())
                                            {
                                                base::DataTable subSubSubTable;
                                                subSubSubTable.Set("id", skill.id);
                                                subSubSubTable.Set("level", skill.level);
                                                subSubTable.Set(++m, subSubSubTable);
                                            }
                                            subTable.Set(++i, subSubTable);
                                        }
                                        Table.Set(++j,subTable);
                                    }
                                }
                                else
                                {
                                    int j = 0;
                                    for (auto armyList : city->armyLists())
                                    {
                                        base::DataTable subTable;
                                        int i = 0;
                                        for (auto it = armyList.armies().begin(); it != armyList.armies().end(); ++it) {
                                            base::DataTable subSubTable;
                                            HeroInfo& heroInfo = it->second.heroInfo();
                                            int m = 0;
                                            for (auto & skill : heroInfo.skill())
                                            {
                                                base::DataTable subSubSubTable;
                                                subSubSubTable.Set("id", skill.id);
                                                subSubSubTable.Set("level", skill.level);
                                                subSubTable.Set(++m, subSubSubTable);
                                            }
                                            subTable.Set(++i, subSubTable);
                                        }
                                        Table.Set(++j,subTable);
                                    }
                                }
                                
                                table.Set("allHeroSkillConfiguration", Table);
                            }
                            break;
                            // 317
                            case WATCHTOWER_SCOUT_TYPE::CITY_TEAM_PPLAYER_NHL_ALLIANCE_NAME: {
                                base::DataTable Table;
                                if (city->isOccupy())
                                {
                                    int i = 0;
                                    for (auto tp : city->troops())
                                    {
                                        DataTable subTable;
                                        subTable.Set("name", tp->agent().nickname());
                                        subTable.Set("headId", tp->agent().headId());
                                        subTable.Set("level", tp->agent().level());
                                        Table.Set(++i, subTable);
                                    }
                                }
                                table.Set("cityTeamPplayerNhlAllianceName", Table);
                            }
                            break;
                            default:
                                isValid = false;
                                break;
                         }
                        if (isValid) {
                            tableResult.Set(tpl.uiKeywords , table);
                            effectiveScoutTypes.push_back(type);
                        }
                    }
                };

                // 侦查行营
                auto scoutCampFixed = [&](CampFixed* campFixed) {
                    if (campFixed->troop()) {
                        defAgent = &campFixed->troop()->agent();
                    }

                    for (auto tpl : scoutTypesTpl) {
                        bool isValid = true;
                        base::DataTable table;
                        auto type = tpl.type;
                        switch (type) {
                            // 201
                            case WATCHTOWER_SCOUT_TYPE::CAMPFIXED_NAME: {
                                table.Set("name", campFixed->tpl().name);
                            }
                            break;
                            // 202
                            case WATCHTOWER_SCOUT_TYPE::CAMPFIXED_PLAYER_ALLIANCE: {
                                table.Set("allianceName", defAgent->allianceName());
                            }
                            break;
                            // 203
                            case WATCHTOWER_SCOUT_TYPE::CAMPFIXED_PLAYER_NAME: {
                                table.Set("playerName", defAgent->nickname());
                            }
                            break;
                            // 204
                            case WATCHTOWER_SCOUT_TYPE::CAMPFIXED_DEFENSE_TEAM_ONE_ARMY_ROUGH_COUNT: {
                                if (!campFixed->troop())
                                {
                                    isValid = false;
                                    break;
                                }
                                int armyCount = campFixed->troop()->armyList()->ArmyCount(ArmyState::NORMAL);
                                getRoughValue(armyCount);
                                table.Set("oneArmyRoughCount", armyCount);
                            }
                            break;
                            // 205
                            case WATCHTOWER_SCOUT_TYPE::CAMPFIXED_DEFENSE_TEAM_ONE_HERO_NAME: {
                                if (!campFixed->troop())
                                {
                                    isValid = false;
                                    break;
                                }
                                base::DataTable Table;
                                if (ArmyList* armyList = campFixed->troop()->armyList())
                                {
                                    int i = 0;
                                    for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                        base::DataTable subTable;
                                        HeroInfo& heroInfo = it->second.heroInfo();
                                        subTable.Set("heroId", heroInfo.tplId());
                                        Table.Set(++i, subTable);
                                    }
                                }
                                table.Set("oneTeamHeroName", Table);
                            }
                            break;
                            // 206
                            case WATCHTOWER_SCOUT_TYPE::CAMPFIXED_DEFENSE_TEAM_ONE_HERO_LEVEL: {
                                if (!campFixed->troop())
                                {
                                    isValid = false;
                                    break;
                                }
                                base::DataTable Table;
                                if (ArmyList* armyList = campFixed->troop()->armyList())
                                {
                                    int i = 0;
                                    for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                        base::DataTable subTable;
                                        HeroInfo& heroInfo = it->second.heroInfo();
                                        subTable.Set("level", heroInfo.level());
                                        Table.Set(++i, subTable);
                                    }
                                }
                                table.Set("oneTeamHeroLevel", Table);
                            }
                            break;
                            // 207
                            case WATCHTOWER_SCOUT_TYPE::CAMPFIXED_DEFENSE_TEAM_ONE_ARMY_TYPE_LEVEL: {
                                if (!campFixed->troop())
                                {
                                    isValid = false;
                                    break;
                                }
                                base::DataTable Table;
                                if (ArmyList* armyList = campFixed->troop()->armyList())
                                {
                                    int i = 0;
                                    for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                        base::DataTable subTable;
                                        ArmyInfo& armyInfo = it->second.armyInfo();
                                        subTable.Set("level", armyInfo.level());
                                        subTable.Set("type", armyInfo.type());
                                        Table.Set(++i, subTable);
                                    }
                                }
                                table.Set("oneTeamArmyLevelTypeTable", Table);
                            }
                            break;
                            // 208
                            case WATCHTOWER_SCOUT_TYPE::CAMPFIXED_ALL_DEFENSE_TEAM_ARMY_ROUGH_COUNT: {
                                if (campFixed->troops().empty())
                                {
                                    isValid = false;
                                    break;
                                }
                                int armyCount = 0;
                                int count = 0;
                                for (auto troop : campFixed->troops())
                                {
                                    ArmyList* armyList = troop->armyList();
                                    armyCount = armyCount + armyList->ArmyCount(ArmyState::NORMAL);
                                    count++;
                                }    
                                getRoughValue(armyCount);
                                table.Set("allArmyRoughCount", armyCount);
                                table.Set("teamCount", count);
                            }
                            break;
                            // 209
                            case WATCHTOWER_SCOUT_TYPE::CAMPFIXED_DEFENSE_TEAM_ARMY_ROUGH_COUNT: {
                                if (campFixed->troops().empty())
                                {
                                    isValid = false;
                                    break;
                                }
                                base::DataTable Table;
                                int i = 0;
                                for (auto troop : campFixed->troops())
                                {
                                    ArmyList* armyList = troop->armyList();
                                    base::DataTable subTable;
                                    int armyCount = 0;
                                    armyCount = armyList->ArmyCount(ArmyState::NORMAL);
                                    getRoughValue(armyCount);
                                    subTable.Set("ArmyCount",armyCount);
                                    Table.Set(++i,subTable);
                                }    
                                table.Set("armyRoughCounts", Table);
                            }
                            break;
                            // 210
                            case WATCHTOWER_SCOUT_TYPE::CAMPFIXED_DEFENSE_TEAM_HERO_NAME: {
                                if (campFixed->troops().empty())
                                {
                                    isValid = false;
                                    break;
                                }
                                base::DataTable Table;
                                int j= 0;
                                for (auto troop : campFixed->troops())
                                {
                                    ArmyList* armyList = troop->armyList();
                                    base::DataTable subTable;
                                    int i = 0;
                                    for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                        base::DataTable subSubTable;
                                        HeroInfo& heroInfo = it->second.heroInfo();
                                        subSubTable.Set("heroId", heroInfo.tplId());
                                        subTable.Set(++i, subSubTable);
                                    }
                                    Table.Set(++j,subTable);
                                }    
                                table.Set("allHeroNames", Table);
                            }
                            break;
                            // 211
                            case WATCHTOWER_SCOUT_TYPE::CAMPFIXED_DEFENSE_TEAM_HERO_LEVEL: {
                                if (campFixed->troops().empty())
                                {
                                    isValid = false;
                                    break;
                                }
                                base::DataTable Table;
                                int j = 0;
                                for (auto troop : campFixed->troops())
                                {
                                    ArmyList* armyList = troop->armyList();
                                    base::DataTable subTable;
                                    int i = 0;
                                    for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                        base::DataTable subSubTable;
                                        HeroInfo& heroInfo = it->second.heroInfo();
                                        subSubTable.Set("level", heroInfo.level());
                                        subTable.Set(++i, subSubTable);
                                    }
                                    Table.Set(++j,subTable);
                                }    
                                table.Set("allHeroLevels", Table);
                            }
                            break;
                            // 212
                            case WATCHTOWER_SCOUT_TYPE::CAMPFIXED_DEFENSE_TEAM_ARMY_TYPE_LEVEL: {
                                if (campFixed->troops().empty())
                                {
                                    isValid = false;
                                    break;
                                }
                                base::DataTable Table;
                                int j = 0;
                                for (auto troop : campFixed->troops())
                                {
                                    ArmyList* armyList = troop->armyList();
                                    base::DataTable subTable;
                                    int i = 0;
                                    for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                        base::DataTable subSubTable;
                                        ArmyInfo& armyInfo = it->second.armyInfo();
                                        subSubTable.Set("type", armyInfo.type());
                                        subSubTable.Set("level", armyInfo.level());
                                        subTable.Set(++i, subSubTable);
                                    }
                                    Table.Set(++j,subTable);
                                }    
                                table.Set("allArmyLevelTypes", Table);
                            }
                            break;
                            // 213
                            case WATCHTOWER_SCOUT_TYPE::CAMPFIXED_DEFENSE_TEAM_ALLIANCE_TECH_INFO: {
                                // todo  
                                base::DataTable Table;
                                int i = 0;
                                for (auto troop : campFixed->troops())
                                {
                                    const AllianceSimple* alliance = troop->agent().alliance();
                                    if (alliance)
                                    {
                                        base::DataTable subTable;
                                        int j = 0;
                                        for (auto scienceInfo : alliance->sciences)
                                        {
                                            base::DataTable subSubTable;
                                            subSubTable.Set("groupId", scienceInfo.groupId);
                                            subSubTable.Set("tplId", scienceInfo.tplId);
                                            subSubTable.Set("level", scienceInfo.level);
                                            subTable.Set(++j, subSubTable);
                                        }
                                        Table.Set(++i, subTable);
                                    }
                                }
                                table.Set("allianceTechInfo", Table);                               
                            }
                            break;
                            // 214
                            case WATCHTOWER_SCOUT_TYPE::CAMPFIXED_ALL_DEFENSE_TEAM_ARMY_COUNT: {
                                if (campFixed->troops().empty())
                                {
                                    isValid = false;
                                    break;
                                }

                                int armyCount = 0;
                                for (auto troop : campFixed->troops())
                                {
                                    ArmyList* armyList = troop->armyList();
                                    armyCount = armyCount + armyList->ArmyCount(ArmyState::NORMAL);
                                }
                                table.Set("campfixedAllDefenseTeamArmyCount", armyCount);
                            }
                            break;
                            // 215
                            case WATCHTOWER_SCOUT_TYPE::CAMPFIXED_DEFENSE_TEAM_ARMY_COUNT: {
                                if (campFixed->troops().empty())
                                {
                                    isValid = false;
                                    break;
                                }
                                int i = 0;
                                base::DataTable Table;
                                for (auto troop : campFixed->troops())
                                {
                                    ArmyList* armyList = troop->armyList();
                                    base::DataTable subTable;
                                    int armyCount = 0;
                                    armyCount = armyList->ArmyCount(ArmyState::NORMAL);
                                    subTable.Set("ArmyCount",armyCount);
                                    Table.Set(++i, subTable);
                                }
                                table.Set("campfixedDefenseTeamArmyCount", Table);
                            }
                            break;
                            // 216
                            case WATCHTOWER_SCOUT_TYPE::CAMPFIXED_DEFENSE_TEAM_TECH_INFO: {
                                if (campFixed->troops().empty())
                                {
                                    isValid = false;
                                    break;
                                }
                                DataTable technologiesTables;
                                int i = 0;
                                for (auto troop : campFixed->troops())
                                {
                                    DataTable technologiesTable;
                                    auto& technologies = troop->agent().technologies();
                                    for (auto it = technologies.begin(); it != technologies.end(); ++it) {
                                        technologiesTable.Set(it->first, it->second);
                                    }
                                    technologiesTables.Set(++i, technologiesTable);
                                } 
                                table.Set("allTechnologies", technologiesTables);
                            }
                            break;
                            // 217
                            case WATCHTOWER_SCOUT_TYPE::CAMPFIXED_DEFENSE_TEAM_HERO_SKILL_CONFIGURATION: {
                                if (campFixed->troops().empty())
                                {
                                    isValid = false;
                                    break;
                                }
                                base::DataTable Table;
                                int j = 0;
                                for (auto troop : campFixed->troops())
                                {
                                    ArmyList* armyList = troop->armyList(); 
                                    base::DataTable subTable;
                                    int i = 0;
                                    for (auto it = armyList->armies().begin(); it != armyList->armies().end(); ++it) {
                                        base::DataTable subSubTable;
                                        HeroInfo& heroInfo = it->second.heroInfo();
                                        int m = 0;
                                        for (auto & skill : heroInfo.skill())
                                        {
                                            base::DataTable subSubSubTable;
                                            subSubSubTable.Set("id", skill.id);
                                            subSubSubTable.Set("level", skill.level);
                                            subSubTable.Set(++m, subSubSubTable);
                                        }
                                        subTable.Set(++i, subSubTable);
                                    }
                                    Table.Set(++j,subTable);
                                } 
                                table.Set("allHeroSkillConfiguration", Table);
                            }
                            break;
                            // 218
                            case WATCHTOWER_SCOUT_TYPE::CAMPFIXED_TEAM_PPLAYER_NHL_ALLIANCE_NAME: {
                                if (campFixed->troops().empty())
                                {
                                    isValid = false;
                                    break;
                                }
                                base::DataTable Table;
                                int i = 0;
                                for (auto troop : campFixed->troops())
                                {
                                    DataTable subTable;
                                    subTable.Set("name", troop->agent().nickname());
                                    subTable.Set("headId", troop->agent().headId());
                                    subTable.Set("level", troop->agent().level());
                                    Table.Set(++i, subTable);
                                } 
                                table.Set("campfixedTeamPplayerNhlAllianceName", Table);
                            }
                            break;
                            default:
                                isValid = false;
                                break;
                        }
                        if (isValid) {
                            tableResult.Set(tpl.uiKeywords, table);
                            effectiveScoutTypes.push_back(type);
                        }
                    }
                };

                if (Castle* castle = unit->ToCastle()) {
                    scoutCastle(castle);
                }  else if (CampFixed* campFixed = unit->ToCampFixed()) {
                    scoutCampFixed(campFixed);
                }  else if (FamousCity* city = unit->ToFamousCity()) {
                    scoutCity(city);
                } else {
                    return false;
                }


                defender.targetTplId = unit->tpl().id;
                defender.targetPos = unit->pos();

                if (defAgent) {
                    defender.SetData(*defAgent);
                    defender.castlePos = unit->pos();
                } else {
                    defender.SetUnitData(unit);
                }

                //msg
                attAgent.OnScout(winner, model::AttackType::ATTACK, effectiveScoutTypes, attacker, defender);
                // if (defAgent) {  //Mod侦查只能地方不能发现
                //     defAgent->OnScout(winner, model::AttackType::DEFENSE, effectiveScoutTypes, attacker, defender);
                // }
                return true;
            }
            return false;
        }

        bool TroopToScout::OnReachProc(Unit* to)
        {
            bool isInValid = true;
            if (to) {
                bool canScout = false;
                if (Castle* castle = to->ToCastle()) {
                    // 保护状态下，直接返回
                    if (castle->IsProtected())
                    {
                        AttackType winner = AttackType::DEFENSE;
                        std::vector< WATCHTOWER_SCOUT_TYPE > effectiveScoutTypes;
                        Agent& attAgent = m_troop->agent();
                        msgqueue::MsgPlayerInfo attacker;
                        msgqueue::MsgScoutDefenderInfo defender;
                        attacker.SetData(attAgent);
                        attAgent.OnScout(winner, model::AttackType::ATTACK, effectiveScoutTypes, attacker, defender);
                        return isInValid;
                    }
                    if (m_troop->allianceId() == 0 || castle->allianceId() != m_troop->allianceId()) {
                        canScout = true;
                    }
                } else if (/*CampFixed* camp = */to->ToCampFixed()) {
                    canScout = true;
//                     if (Troop* tp = camp->troop()) {
//                         if (m_troop->allianceId() == 0 || tp->allianceId() != m_troop->allianceId()) {
//                             canScout = true;
//                         }
//                     }
                } else if (/*FamousCity* city = */to->ToFamousCity()) {
                    canScout = true;
//                     if (Troop* tp = city->troop()) {
//                         if (m_troop->allianceId() == 0 || tp->allianceId() != m_troop->allianceId()) {
//                             canScout = true;
//                         }
//                     }
                }

                if (canScout) {
                    Scout(to);
                    m_troop->GoHome();
                    isInValid = false;
                }
            }
            return isInValid;
        }

        // -------------------- TroopToReinforcements
        bool TroopToReinforcements::OnReachProc(Unit* to)
        {
            bool isInValid = true;
            if (to) {
                if (Castle* castle = to->ToCastle()) {
                    if (castle->allianceId() == m_troop->allianceId() && m_troop->allianceId() > 0) {
                        castle->Garrison(m_troop);
                        castle->NoticeUnitUpdate();
                        isInValid = false;
                    }
                }
            }
            return isInValid;
        }

        void TroopToReinforcements::FinishDeserialize()
        {
            TroopImpl::FinishDeserialize();
            if (m_troop->IsReach()) {
                if (Unit* unit = g_mapMgr->FindUnit(m_troop->to())) {
                    m_troop->SetReachTplId(unit->tpl().id);
//                     if (Castle* cst = unit->ToCastle()) {
//                         if (m_troop->type() == model::MapTroopType::REINFORCEMENTS) {
//                             cst->AddReinforcement(m_troop);
//                         }
//                     }
                }
            }
        }

        void TroopToReinforcements::OnBack()
        {
            if (Unit* unit = g_mapMgr->FindUnit(m_troop->to())) {
                if (Castle* castle = unit->ToCastle()) {
                    castle->RemoveMarchReinforcement(m_troop);
                }
            }
        }

        // -------------------- TroopToCatapult
        void TroopToCatapult::OnDefenceFailed(Unit* unit)
        {
//             if (m_troop->GetArmyTotal() <=  0) {
                m_troop->GoHome();
//             }
        }

        void TroopToCatapult::OnAttackWin(Unit* unit)
        {
            if (unit->ToCatapult()) {
                // if (!m_troop->isOccupyCity()) {
                //     m_troop->GoHome();
                // }
            } else {
                m_troop->GoHome();
            }
        }

        bool TroopToCatapult::OnReachProc(Unit* to)
        {
            bool isInValid = true;
            //int troopallianceId =  m_troop->allianceId();
            //cout << "troopallianceId" << troopallianceId << endl;
            if (m_troop->allianceId() > 0) {
                if (auto catapult = to->ToCatapult()) {
                    //int catapultallianceId = catapult->allianceId();
                    //cout << "catapultallianceId" << catapultallianceId << endl;
                    if (catapult->allianceId() ==  m_troop->allianceId()) {
                        isInValid = OnReachAllianceCatapult(catapult);
                    } else {
                        isInValid = OnReachOtherCatapult(catapult);
                    }
                }
            }
            return isInValid;
        }
        
        bool TroopToCatapult::OnReachAllianceCatapult(Catapult* catapult)
        {
            bool isInValid = true;
            if (m_troop->type() ==  MapTroopType::SIEGE_CATAPULT) {
                if (catapult->CanGarrison(m_troop)) {
                    catapult->Garrison(m_troop);
                    catapult->NoticeUnitUpdate();
                    isInValid = false;
                }
            }

            return isInValid;
        }

        bool TroopToCatapult::OnReachOtherCatapult(Catapult* catapult)
        {
            bool isInValid = true;
            bool fight = true;

            if (fight) {
                if (BattleMgr::instance().GenerateBattle(m_troop, catapult)) {
                    isInValid = false;
                    if (!catapult->IsDelete()) {
                        catapult->NoticeUnitUpdate();   //可能被delete
                    }
                }
            }
            return isInValid;
        }

        void TroopToCatapult::OnReachInvalid()
        {
            TroopImpl::OnReachInvalid();
        }

        // 达到成功
        bool TroopToTransport::OnReachProc(Unit* to) {
            if (to)
            {
                Castle* toCastle = to->ToCastle();
                if (!toCastle)
                {
                    return true;
                }
                Agent& agent = m_troop->agent();
                Agent& toAgent = toCastle->agent();
                msgqueue::MsgPvpPlayerInfo m_attackerInfo;
                msgqueue::MsgPvpPlayerInfo m_defenderInfo;
                m_attackerInfo.SetData(agent);
                m_defenderInfo.SetData(toAgent);
                toAgent.msgQueue().AppendMsgTransport(model::TransportType::RECEIVE, m_attackerInfo, m_defenderInfo, 
                    m_troop->food(), m_troop->wood(), m_troop->iron(), m_troop->stone(), true);
                agent.msgQueue().AppendMsgTransport(model::TransportType::TRANSMIT, m_attackerInfo, m_defenderInfo, 
                    m_troop->food(), m_troop->wood(), m_troop->iron(), m_troop->stone(), true);
                int64_t now = g_dispatcher->GetTimestampCache();
                g_mapMgr->proxy()->SendcsAddTransportRecord(m_troop, agent.uid(), agent.headId(), agent.nickname(), toAgent.uid(), toAgent.headId(), toAgent.nickname(), model::TransportArriveType::SUCCESS, now);
                // 运输成功，把部队里面的资源清除
                m_troop->ClearResources();
                m_troop->GoHome();    
                return false;
            }
            else
            {
                // 如果目标为空，则走到达失败
                OnMarchBack();
            }
            return true;
            
        }

        void TroopToTransport::OnBack() {

        }

        // 达到失败
        void TroopToTransport::OnMarchBack()
        {
            if (Agent* toAgent = m_troop->transportAgent())
            {
                int64_t now = g_dispatcher->GetTimestampCache();
                Agent& agent = m_troop->agent();
                msgqueue::MsgPvpPlayerInfo m_attackerInfo;
                msgqueue::MsgPvpPlayerInfo m_defenderInfo;
                m_attackerInfo.SetData(agent);
                m_defenderInfo.SetData(*toAgent);
                toAgent->msgQueue().AppendMsgTransport(model::TransportType::RECEIVE, m_attackerInfo, m_defenderInfo, 
                    m_troop->food(), m_troop->wood(), m_troop->iron(), m_troop->stone(), false);
                agent.msgQueue().AppendMsgTransport(model::TransportType::TRANSMIT, m_attackerInfo, m_defenderInfo, 
                    m_troop->food(), m_troop->wood(), m_troop->iron(), m_troop->stone(), false);
                g_mapMgr->proxy()->SendcsAddTransportRecord(m_troop, agent.uid(), agent.headId(), agent.nickname(), toAgent->uid(), toAgent->headId(), toAgent->nickname(), model::TransportArriveType::FAIL, now);
            }
        }

    }
}