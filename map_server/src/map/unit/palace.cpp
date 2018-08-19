#include "palace.h"
#include <algorithm>
#include "catapult.h"
#include "../armylist.h"
#include "../troop.h"
#include "../alliance.h"
#include "../agent.h"
#include "../mapProxy.h"
#include "../tpl/mapcitytpl.h"
#include "../qo/commandsuccessivekingsave.h"
#include "../qo/commandsuccessivekingsload.h"
#include <model/tpl/templateloader.h>
#include <model/tpl/army.h>
#include <model/tpl/configure.h>
#include <base/logger.h>

namespace ms
{
    namespace map
    {
        using namespace std;
        using namespace model;
        using namespace model::tpl;
        using namespace info;
        using namespace base;

        Capital::Capital(int id, const MapUnitCapitalTpl* tpl, const Point& bornPoint)
            : FamousCity(id, tpl, bornPoint), m_tpl(tpl)
        {           
           int cityId = m_bornPoint.x * 1200 + m_bornPoint.y;
            m_cityTpl = tpl::m_tploader->FindMapCity(cityId);
        }

        Capital::~Capital()
        {
            //SAFE_DELETE(m_dragon);
        }

        bool Capital::CanOccupy(Troop* troop)
        {
            if (isNotStart())
            {
                return false;
            }
            return allianceId() == 0 || allianceId() != troop->allianceId();
        }

        void Capital::Init()
        {
            if (m_state != model::PalaceState::NOSTART && m_state != model::PalaceState::NPC)  //如果不等于未开启和NPC就不走重载NPC
            {
                return;
            }
            if (m_tpl) {
                int armyGroup = m_cityTpl->armyGroup;
                int armyCount = m_cityTpl->armyCount;
                int armyNum = m_cityTpl->armyNum;
                m_armyListsTotal = 0;
                m_armyLists.clear();
                if (m_cityTpl) {
                    if (m_cityTpl->turretAtkPower > 0) {
                        m_turretAtkPower = m_cityTpl->turretAtkPower;
                    }

                    if (!m_cityTpl->traps.empty()) {
                        for (auto& trap :  m_cityTpl->traps) {
                            m_trapSet.AddTrap(trap.type,  model::ArmyState::NORMAL,  trap.count);
                        }
                    }
                    for (int i = 0; i < armyNum; ++i)
                    {
                        ArmyList armyList = tpl::m_tploader->GetRandomArmyList(armyGroup, armyCount);
                        m_armyLists.push_back(armyList);
                        //初始化的时候记录一下NPC总数
                        //armyListsTotal
                        m_armyListsTotal += armyList.ArmyCount(ArmyState::NORMAL) + armyList.ArmyCount(ArmyState::DIE);   
                    }
                }
                //SetDirty();
            }
            NoticeUnitUpdate();
        }

        int64_t Capital::startLeftTime() const
        {
            int time = m_startTimestamp - g_dispatcher->GetTimestampCache();
            return time > 0 ? time : 0;
        }

        int64_t Capital::endLeftTime() const
        {
            int time = m_endTimestamp - g_dispatcher->GetTimestampCache();
            return time > 0 ? time : 0;
        }

        int64_t Capital::chooseLeftTime() const
        {
            int time = m_endTimestamp + g_tploader->configure().palaceWar.chooseKingTime - g_dispatcher->GetTimestampCache();
            return time > 0 ? time : 0;
        }


        int64_t Capital::NpcRecoverLeftTime() const
        {
            int time = m_nextNPCRecoverTimestamp - g_dispatcher->GetTimestampCache();
            return time > 0 ? time : 0;
        }

        int64_t Capital::cpRecoverLeftTime() const
        {
            int time = m_nextCpRecoverTimestamp - g_dispatcher->GetTimestampCache();
            return time > 0 ? time : 0;
        }
       
        void Capital::SetOccupier(Agent* agent)
        {
            m_occupier = agent;
            SetDirty();
            NoticeUnitUpdate();
            //Debug();
        }


        int Capital::ArmyCount()
        {
            return m_armyListsTotal;
        }

        int Capital::NormalArmyCout()
        {
            int armyListsTotal = 0;
            for (std::list<ArmyList>::iterator it = m_armyLists.begin(); it != m_armyLists.end(); ++it)
            {
                armyListsTotal += (*it).ArmyCount(ArmyState::NORMAL);
            }
            return armyListsTotal;
        }
        /*
        void Palace::AddDragonHp(int num)
        {
            //cout << "Palace::AddDragonHp = " << num << endl;
            if (m_dragonHp >= m_dragonTpl->hp) {
                return;
            }
            m_dragonHp += num;
            if (m_dragonHp > m_dragonTpl->hp) {
                m_dragonHp = m_dragonTpl->hp;
            }
            SetDirty();
            NoticeUnitUpdate();
        }

        void Palace::RemoveDragonHp(int num)
        {
            if (m_dragonHp <= 0) {
                return;
            }
            m_dragonHp -= num;
            if (m_dragonHp <= 0) {
                m_dragonHp = 0;
                m_maintainer.ClearByTag(DRAGON_RECOVER);
            }
            SetDirty();
            NoticeUnitUpdate();
        }

        void Palace::ResetDragonArmyList()
        {
            SAFE_DELETE(m_dragon);
            m_dragon = new ArmyList();
            m_dragon->AddArmy(m_dragonTpl->id, model::ArmyState::NORMAL, 1);
        }
        */  
       void Capital::Garrison(Troop* troop)
       {
            if (troop) {
                if (troop->allianceId() !=  m_occupyallianceId) {
                    LOG_DEBUG("Garrison  alliance not the same");
                    return;
                }

                m_troops.push_back(troop);
                m_troopIds.push_back(troop->id());

                UpdateDefTroop();
                SetDirty();
            }
       }
       
        void Capital::Occupy(Troop* troop)
        {
            ClearAllDefenders();
            if (troop->allianceId() == 0) {
                troop->GoHome();
            } else {
                m_occupierId = troop->allianceId();
                m_occupyallianceId = troop->allianceId(); 
                m_allianceId = troop->allianceId();
                vector<Troop*> troops;
                vector<Troop*> removeList;
                troops.push_back(troop);
                /*for (auto it = troop->rallyTroops().begin(); it != troop->rallyTroops().end(); ++it) {
                    Troop* tp = it->second;
                    if (tp->armyList().IsAllDie()) {
                        removeList.push_back(tp);
                    } else {
                        troops.push_back(tp);
                    }
                }*/
                /*for (Troop* tp : removeList) {
                    tp->GoHome();
                }*/
                if (m_troops.empty()) {
                    SetOccupier(&troop->agent());
                }
                m_troops.push_back(troop);
                m_troopIds.push_back(troop->id());
                UpdateDefTroop();
                int64_t occupyTime = g_tploader->configure().palaceWar.occupyTime;
                m_endTimestamp = g_dispatcher->GetTimestampCache() + occupyTime;
                m_maintainer.ClearByTag(END_WAR);
                m_maintainer.Add(g_dispatcher->quicktimer().SetTimeoutWithLinker(std::bind(&Capital::EndWar, this), occupyTime * 1000), END_WAR);
                //Debug();
                //记录
                if (m_occupier) {
                    DataTable dt;
                    dt.Set(1, m_occupier->headId());
                    dt.Set(2, m_occupier->allianceName());
                    dt.Set(3, m_occupier->nickname());
                    string param = dt.Serialize();
                    AddPalaceWarRecord(PalaceWarRecordType::OCCUPY_PALACE, param);
                }
                //通知魔法塔占领者联盟变化
                for (Catapult * cp : g_mapMgr->catapults()) {
                    cp->OnPalaceOccupierIdChanged();
                }
                SetState(model::PalaceState::OCCUPY);
                NoticeUnitUpdate();
                SetDirty();
            }
        }

        int Capital::NpcBeAttackByCatapult(float catapultAttack)
        {
            int kills = 0;
            int hpSum = 0;
            ArmyList* armyList = DefArmyList();
            if (!armyList)
            {
                return kills;
            }
            for (auto it2 = armyList->armies().begin(); it2 != armyList->armies().end(); ++it2) {
                ArmyInfo& info = it2->second.armyInfo();
                int normal = info.count(model::ArmyState::NORMAL);
                if (normal > 0) {
                    hpSum += 1 * normal;
                }
            }

            ArmyList dieList;
            //Troop* tp = m_troop;
            for (auto it2 = armyList->armies().begin(); it2 != armyList->armies().end(); ++it2) {
                ArmyInfo& info = it2->second.armyInfo();
                    int normal = info.count(model::ArmyState::NORMAL);
                    if (normal > 0) {
                        int hp = 1 * normal;
                        int attack = catapultAttack * hp / hpSum;
                        int hurt = ceil((float)pow(attack, 2) / (attack + 1 ));
                        // todo by zds
                        // int hurt = ceil((float)pow(attack, 2) / (attack + tpl->jobDefense ));
                        // hurt 可能会大于 hp
                        int loss = 0;
                        if (hurt >= hp) {
                            loss = normal;
                        } else {
                            loss = ceil((float)normal * hurt / hp);
                        }
                        info.Remove(model::ArmyState::NORMAL, loss);
                        if (loss > 0) {
                            info.Add(model::ArmyState::DIE, loss);
                        }
                        kills += loss;
                    }
            }

            if (m_troop) 
            {
                if (m_troop->armyList()->IsAllDie()) {
                    //切换部队
                    SwitchTroop();
                }
            }
            
            NoticeUnitUpdate();
            return kills;
        }
        
       
        
        int Capital::PlayersBeAttackByCatapult(float catapultAttack)
        {
            
            //cout << "Palace::PlayersBeAttackByCatapult catapultAttack = " << catapultAttack << endl;
            int kills = 0;
            
            int64_t hpSum = 0;
            //1 计算总hp值 下一步根据hp的占比计算所受到的攻击
            if (!m_troop)
            {
                return kills;
            }
            Troop* tp = m_troop;   //最上层的部队
            for (auto it2 = tp->armyList()->armies().begin(); it2 != tp->armyList()->armies().end(); ++it2) {
                ArmyInfo& info = it2->second.armyInfo();
                int normal = info.count(model::ArmyState::NORMAL);
                if (normal > 0) {
                    hpSum += 1 * normal;
                }
            }
            
            //2 计算兵种所受到伤害 并死亡
        
            ArmyList dieList;
            //Troop* tp = m_troop;
            for (auto it2 = tp->armyList()->armies().begin(); it2 != tp->armyList()->armies().end(); ++it2) {
                ArmyInfo& info = it2->second.armyInfo();
                int normal = info.count(model::ArmyState::NORMAL);
                if (normal > 0) {
                    int hp = 1 * normal;
                    int attack = catapultAttack * hp / hpSum;
                    int hurt = ceil((float)pow(attack, 2) / (attack + 1 ));
                    // todo by zds
                    // int hurt = ceil((float)pow(attack, 2) / (attack + tpl->jobDefense ));
                    // hurt 可能会大于 hp
                    int loss = 0;
                    if (hurt >= hp) {
                        loss = normal;
                    } else {
                        loss = ceil((float)normal * hurt / hp);
                    }
                    info.Remove(model::ArmyState::NORMAL, loss);
                    if (loss > 0) {
                        info.Add(model::ArmyState::DIE, loss);
                        //dieList.AddArmy(info.tplid(), model::ArmyState::DIE, loss);
                    }
                    kills += loss;
                    //击杀记录
                    //AddRecordByCatapultAttack(tp->uid(), info.tplid(), loss, wound);
                }
            }
            tp->agent().OnBeAttackedByCatapult(dieList, false);
            tp->NoticeTroopUpdate();
            //tp->armyList().ClearAllDie();
            

            if (m_troop) 
            {
                if (m_troop->armyList()->IsAllDie()) {
                    //切换部队
                    SwitchTroop();
                }
            }
            NoticeUnitUpdate();
            return kills;

        }

        void Capital::OnPlaceWarStart()
        {
            SetState(model::PalaceState::NPC);
            //刷NPC
            Init();
            int64_t recoverTime = g_tploader->configure().palaceWar.recoverTime;
            m_nextNPCRecoverTimestamp = g_dispatcher->GetTimestampCache() + recoverTime;
            m_maintainer.Add(g_dispatcher->quicktimer().SetIntervalWithLinker(std::bind(&Capital::NPCRecover, this), recoverTime * 1000), NPC_RECOVER);
            //清理联盟信息
            m_occupierId = 0;
            m_occupyallianceId = 0;
            m_allianceId = 0;
            //魔法塔恢复
            recoverTime = g_tploader->configure().catapult.recoverTime;
            m_nextCpRecoverTimestamp = g_dispatcher->GetTimestampCache() + recoverTime;
            m_maintainer.Add(g_dispatcher->quicktimer().SetIntervalWithLinker(std::bind(&Capital::CatapultsRecover, this), recoverTime * 1000), CATAPULT_RECOVER);
            NoticeUnitUpdate();
            SetDirty();
        }
        
        
        
        void Capital::OnPlaceWarEnd()
        {
            //m_state = model::CastleState::PROTECTED;
            m_startTimestamp = g_dispatcher->GetTimestampCache() + g_tploader->configure().palaceWar.peaceTime;
            ClearAllDefenders();
            m_occupier = nullptr;
            m_nextNPCRecoverTimestamp = 0;
            m_nextCpRecoverTimestamp = 0;
            m_armyListsTotal = 0;
            m_armyLists.clear();

            //m_recordByCatapultAttack.clear();
            //m_attackDragonRecords.clear();
            m_maintainer.ClearAll();
            NoticeUnitUpdate();
            SetDirty();
        }
        
        
        void Capital::OnKingChanged()
        {
            if (g_mapMgr->king()) {
                //在完成王城争霸结束之后CS还会发来一个消息清楚国王数据
                SetState(model::PalaceState::PROTECTED);
                AddSuccessiveKingRecord(*(g_mapMgr->king()));
            }
            NoticeUnitUpdate();
        }
       
        void Capital::OnTroopLoadFinished(Troop* troop)
        {
            if (troop->IsReach()) {
                m_defenders.emplace(troop->uid(), troop);
            }
        }

        void Capital::InitWar()
        {
            cout << "Palace::InitWar" << endl;

            // Init();
            // cout << "g_mapMgr->openServerTime() = " << g_mapMgr->openServerTime() << endl;
            // cout << "=========================m_occupierId" << m_occupierId << endl;
            const PalaceWarConf& conf = g_tploader->configure().palaceWar;
            if ( isNotStart() ) {
                //第一次王城争霸
                int64_t now = g_dispatcher->GetTimestampCache();
                if (m_endTimestamp == 0) {
                    int64_t pwt = g_mapMgr->palaceWarTime();
                    if (pwt != 0 && pwt > now) {
                        m_startTimestamp = pwt;
                    } else {
                        int64_t ost = g_mapMgr->openServerTime();
                        if (ost == 0) {
                            ost = now;
                        }
                        m_startTimestamp = ost + conf.firstStartTime;
                        // cout << "===================firstStartTime = " << conf.firstStartTime << endl;
                        // cout << "===================startTimestamp =  " << m_startTimestamp << endl;
                        // cout << "===================now   = " << g_dispatcher->GetTimestampCache() << endl;
                    }
                    SetDirty();
                    //首次争霸战前备战物资
                    ResetPrepareNoticeTimer();
                }
                //Debug();
                if (startLeftTime() > 0) {
                    // cout << "**************************begin StartWar: startLeftTime  = " << startLeftTime() << endl;
                    m_maintainer.Add(g_dispatcher->quicktimer().SetTimeoutWithLinker(std::bind(&Capital::StartWar, this), startLeftTime() * 1000), START_WAR);
                } else {
                    // cout << "**************************begin StartWar now " << startLeftTime() << endl;
                    m_maintainer.Add(g_dispatcher->quicktimer().SetTimeoutWithLinker(std::bind(&Capital::StartWar, this), 1 * 1000), START_WAR);
                }
                //cout << "startLeftTime = " << startLeftTime() << endl;
            } else {
                //争霸状态
                //龙战斗用信息
                //ResetDragonArmyList();
                //龙恢复血量的定时器
                m_nextNPCRecoverTimestamp = g_dispatcher->GetTimestampCache() + conf.recoverTime;
                m_maintainer.Add(g_dispatcher->quicktimer().SetIntervalWithLinker(std::bind(&Capital::NPCRecover, this), conf.recoverTime * 1000), NPC_RECOVER);
                //魔法塔恢复
                int64_t cpRecoverTime = g_tploader->configure().catapult.recoverTime;
                m_nextCpRecoverTimestamp = g_dispatcher->GetTimestampCache() + cpRecoverTime;
                m_maintainer.Add(g_dispatcher->quicktimer().SetIntervalWithLinker(std::bind(&Capital::CatapultsRecover, this), cpRecoverTime * 1000), CATAPULT_RECOVER);
                for (Catapult * cp : g_mapMgr->catapults()) {
                    cp->Refresh();
                }
                //结束定时器
                if (m_occupierId != 0) {
                    m_maintainer.Add(g_dispatcher->quicktimer().SetTimeoutWithLinker(std::bind(&Capital::EndWar, this), endLeftTime() * 1000), END_WAR);
                }
            }

            NoticeUnitUpdate();
            //m_maintainer.Add(g_dispatcher->quicktimer().SetTimeoutWithLinker(std::bind(&Capital::Debug, this), 5 * 1000), DEBUG_DATA);
        }

        void Capital::StartWar()
        {
            // cout << "Palace::StartWar" << endl;
            g_mapMgr->OnPalaceWarStart();
            
            OnPlaceWarStart();
            for (Catapult * cp : g_mapMgr->catapults()) {
                cp->OnPlaceWarStart();
            }

            //Debug();
        }

        void Capital::EndWar()
        {
            SetState(model::PalaceState::CHOOSE);
            cout << "Palace::EndWar" << endl;
            for (Catapult * cp : g_mapMgr->catapults()) {
                cp->OnPlaceWarEnd();
            }
            OnPlaceWarEnd();

            //通知CS结束
            g_mapMgr->proxy()->SendcsPalaceWarEnd();

            InitWar();
        }

        
        void Capital::NPCRecover()
        {
            //cout << "Palace::DragonRecover" << endl;
            if (m_occupierId == 0)
            {
                Init();
                int64_t recoverTime = g_tploader->configure().palaceWar.recoverTime;
                m_nextNPCRecoverTimestamp = g_dispatcher->GetTimestampCache() + recoverTime;
                NoticeUnitUpdate();
                //Debug();
                SetDirty();
            }
        }
        
       
        void Capital::CatapultsRecover()
        {
            cout << "Palace::CatapultsRecover" << endl;
            const CatapultConf& conf = g_tploader->configure().catapult;
            m_nextCpRecoverTimestamp = g_dispatcher->GetTimestampCache() + conf.recoverTime;
            for (Catapult * cp : g_mapMgr->catapults()) {
                cp->Refresh();
            }
            SetDirty();
        }
        
        
        void Capital::Debug()
        {
            m_maintainer.Add(g_dispatcher->quicktimer().SetTimeoutWithLinker(std::bind(&Capital::Debug, this), 5 * 1000), DEBUG_DATA);
        }
            
        
        void Capital::AddSuccessiveKingRecord(const Agent& king)
        {
            SuccessiveKing sk;
            sk.headId = king.headId();
            sk.allianceNickname = king.allianceName();
            sk.nickname = king.nickname();
            if (m_successiveKings.empty()) {
                sk.n = 1;
            } else {
                SuccessiveKing& f = m_successiveKings.front();
                sk.n = f.n + 1;
            }
            sk.timestamp = g_dispatcher->GetTimestampCache();
            m_successiveKings.push_front(sk);
            if (m_successiveKings.size() > 100) {
                m_successiveKings.pop_back();
            }
            m_runner.PushCommand(new qo::CommandSuccessiveKingSave(sk));
        }
        

        /*
        void Palace::RepatriateAllDefenders()
        {
            ClearAllDefenders();
            NoticeUnitUpdate();
        }
        */
       
        void Capital::AddPalaceWarRecord(PalaceWarRecordType type, const string& param)
        {
            //cout << "Palace::AddPalaceWarRecord type = " << (int)type << " param = " << param << endl;
            PalaceWarRecord pwr;
            pwr.type = type;
            pwr.param = param;
            pwr.timestamp = g_dispatcher->GetTimestampCache();
            m_palaceWarRecords.push_front(pwr);
            if (m_palaceWarRecords.size() > 100) {
                m_palaceWarRecords.pop_back();
            }
        }
        
        /*
        void Palace::AddAttackDragonRecord(int64_t uid, int hurt)
        {
            if (PalaceWarAttackDragonRecord* record = FindAttackDragonRecord(uid)) {
                record->hurt += hurt;
                record->timestamp = g_dispatcher->GetTimestampCache();
            } else {
                PalaceWarAttackDragonRecord rd;
                rd.uid = uid;
                rd.hurt = hurt;
                rd.timestamp = g_dispatcher->GetTimestampCache();
                m_attackDragonRecords.emplace(uid, rd);
            }
        }
        */

        string Capital::Serialize()
        {
            std::string jsonString;
            try {
                rapidjson::StringBuffer jsonbuffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                writer.StartObject();

                writer.String("pos");
                writer.StartArray();
                writer.Int(m_pos.x);
                writer.Int(m_pos.y);
                writer.EndArray();

                writer.String("refreshTime");
                writer.Int64(m_refreshTime);

                writer.String("saveTimestamp"); // temp
                writer.Int64(g_dispatcher->GetTimestampCache());

                //writer.String("dragonHp");
                //writer.Int(m_dragonHp);
                writer.String("occupierId");
                writer.Int64(m_occupierId);
                writer.String("occupyallianceId");
                writer.Int64(m_occupyallianceId);
                writer.String("allianceId");
                writer.Int64(m_allianceId);

                writer.String("state");
                writer.Int((int)m_state);
                writer.String("occupier");
                writer.Int64(m_occupier ? m_occupier->uid() : 0);
                writer.String("startTimestamp");
                writer.Int64(m_startTimestamp);
                writer.String("endTimestamp");
                writer.Int64(m_endTimestamp);
                //writer.String("nextDgRecoverTimestamp");
                //writer.Int64(m_nextDgRecoverTimestamp);
                //writer.String("nextCpRecoverTimestamp");
                //writer.Int64(m_nextCpRecoverTimestamp);
                writer.Key("armyLists");
                writer.StartArray();
                for (auto it = m_armyLists.begin(); it != m_armyLists.end(); ++it)
                {
                    (*it).Serialize(writer);
                }
                writer.EndArray();

                writer.Key("troopId");
                writer.StartArray();
                for (auto troop :  m_troops) {
                    if (troop) {
                        writer.Int(troop->id());
                    }
                }
                writer.EndArray();
                writer.String("armyListsTotal");
                writer.Int64(m_armyListsTotal);
                writer.EndObject();

                
                jsonString = jsonbuffer.GetString();
            } catch (std::exception& ex) {
                LOG_ERROR("save Palace json fail: %s\n", ex.what());
            }

            return jsonString;
        }

        bool Capital::Deserialize(string data)
        {
            // cout << "==================palace data================" << endl;
            try {
                rapidjson::Document doc;
                doc.Parse<0>(data.c_str());
                if (doc.IsObject()) {

                    if (doc["pos"].IsArray()) {
                        auto& temp = doc["pos"];
                        m_pos.x = temp[0u].GetInt();
                        m_pos.y = temp[1].GetInt();
                    }
                    m_refreshTime = doc["refreshTime"].GetInt64();

                    int64_t saveTimestamp = doc["saveTimestamp"].GetInt64();
                    saveTimestamp = g_dispatcher->GetTimestampCache() - saveTimestamp;
                    if (saveTimestamp < 0) {
                        saveTimestamp = 0;
                    }

                    //m_dragonHp = doc["dragonHp"].GetInt();
                    m_occupierId = doc["occupierId"].GetInt64();
                    m_occupyallianceId = doc["occupyallianceId"].GetInt64();
                    m_allianceId = doc["allianceId"].GetInt64();
                    m_state = static_cast<model::PalaceState>(doc["state"].GetInt());
                    int64_t uid = doc["occupier"].GetInt64();
                    m_occupier = g_mapMgr->FindAgentByUID(uid);
                    
                    int64_t startTimestamp = doc["startTimestamp"].GetInt64();
                    m_startTimestamp = startTimestamp;
                    int64_t endTimestamp = doc["endTimestamp"].GetInt64();
                    if (endTimestamp != 0) m_endTimestamp = endTimestamp + saveTimestamp;

                    auto& armyLists = doc["armyLists"];
                    for (size_t i = 0; i < armyLists.Size(); ++i) {
                        ArmyList armyList;
                        armyList.Desrialize(armyLists[i]);
                        m_armyLists.push_back(armyList);
                    }
                    if (doc["troopId"].IsArray()) {
                        auto& temp = doc["troopId"];
                        for (int i = 0; i < (int)temp.Size(); ++i) {
                            int troopId = temp[i].GetInt();
                            m_troopIds.push_back(troopId);
                        }
                    }
                    m_armyListsTotal = doc["armyListsTotal"].GetInt64();
                }
                return true;
            } catch (std::exception& ex) {
                LOG_ERROR("json string to Palace fail: %s\n", ex.what());
                return false;
            }
        }

        void Capital::FinishDeserialize()
        {
            if (m_troopIds.size() > 0) {
                for (auto troopId :  m_troopIds) {
                  auto troop = g_mapMgr->FindTroop(troopId);
                  if (troop) {
                      m_troops.push_back(troop);
                  }
                }
                UpdateDefTroop();
            } else {
//                 if (!m_isOccupy) {
//                     Init();
//                 }
            }
            m_runner.PushCommand(new qo::CommandSuccessiveKingsLoad());
            SetDirty();
        }

        void Capital::ResetPrepareNoticeTimer()
        {
            // cout << "Palace::ResetPrepareNoticeTimer" << endl;
            int64_t leftTime = -1;
            PalaceWarPrepareConf c;
            int64_t now = g_dispatcher->GetTimestampCache();
            for (const PalaceWarPrepareConf & conf : g_tploader->configure().palaceWarPrepares) {
                int64_t t = m_startTimestamp - conf.time;
                if (now < t) {
                    leftTime = t - now;
                    c = conf;
                    break;
                }
            }
            if (leftTime > 0) {
                // cout << "leftTime = " << leftTime << " times = " << c.times << " dropId = " << c.dropId << endl;
                m_maintainer.Add(g_dispatcher->quicktimer().SetTimeoutWithLinker([ = ]() {
                    g_mapMgr->proxy()->SendcsPalaceWarPrepare(c.times, c.dropId);
                    ResetPrepareNoticeTimer();
                }, leftTime * 1000), PREPARE_NOTICE);
            }
        }

    }
}

