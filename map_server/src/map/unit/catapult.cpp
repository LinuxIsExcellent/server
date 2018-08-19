#include <algorithm>
#include "../troop.h"
#include "../agent.h"
#include "palace.h"
#include "catapult.h"
#include "../tpl/mapcitytpl.h"
#include <base/event/dispatcher.h>
#include <model/tpl/templateloader.h>
#include <model/tpl/configure.h>
#include <model/tpl/army.h>
#include <base/logger.h>

namespace ms
{
    namespace map
    {
        using namespace std;
        using namespace info;
        using namespace base;
        using namespace model;
        using namespace model::tpl;

        Catapult::Catapult(int id, const MapUnitCatapultTpl* tpl, const Point& bornPoint)
            : Unit(id, tpl), m_tpl(tpl), m_bornPoint(bornPoint)
        {
            int cityId = m_bornPoint.x * 1200 + m_bornPoint.y;
            m_cityTpl = tpl::m_tploader->FindMapCity(cityId);
        }

        Catapult::~Catapult()
        {

        }

        void Catapult::Init()
        {
            m_occupierId = 0;
            Refresh();

            NoticeUnitUpdate();

        }
     
                
        void Catapult::Refresh()
        {   
            if (m_occupierId == 0) {
                if (m_cityTpl)
                {
                    m_armyListsTotal = 0;
                    m_armyLists.clear();
                    for (int i = 0; i < m_cityTpl->armyNum; ++i) {
                        ArmyList armyList = tpl::m_tploader->GetRandomArmyList(m_cityTpl->armyGroup, m_cityTpl->armyCount);
                        m_armyLists.push_back(armyList);
                        //初始化的时候记录一下NPC总数
                        //armyListsTotal
                        m_armyListsTotal += armyList.ArmyCount(ArmyState::NORMAL) + armyList.ArmyCount(ArmyState::DIE);   
                    }
                }
                //cout << "armyGroup = " << m_cityTpl->armyGroup << "armyCount = " <<  m_cityTpl->armyCount << endl;
                NoticeUnitUpdate();
            }
        }
        int Catapult::GetDropId()
        {
            return m_cityTpl->dropId;
        }

        int Catapult::troopId() const
        {
            if (m_troop == nullptr) {
                return 0;
            }
            return m_troop->id();
        }
        
        void Catapult::ClearAllDefenders()
        {
            while(!m_troops.empty())
            {
                Troop* tp = m_troops.front();
                tp->GoHome();
            }
        }
       
        void Catapult::AttackPalace()
        {
            //cout << "Catapult::AttackPalace" << endl;
            Capital& plc = g_mapMgr->capital();
            if (m_troops.size() == 0 || m_occupierId == plc.occupierId()) {
                return;
            }

            //calculate hurt
            
            if (!plc.NormalArmyCout() == 0 && plc.occupierId() == 0) { 
                //attack dragon
                // const CatapultConf::AttackDragon& conf = catapultConf.attackDragon;
                float attack = 100;//conf.basePercent + armyCountLevel() * conf.correction * (1 + conf.gainCoe * catapultCount());
                int kills = plc.NpcBeAttackByCatapult(attack);
                //cout << "dragonHP hurt = " << hurt << endl;
                if (kills > 0) {
                    m_lastKills = kills;
                    m_kills += kills;

                    //m_kills += hurt;
                    m_lastKillTimestamp = g_dispatcher->GetTimestampCache();
                    NoticeUnitUpdate();
                    //记录
                    if (m_occupier) {
                        DataTable dt;
                        dt.Set(1, m_occupier->headId());
                        dt.Set(2, m_occupier->allianceName());
                        dt.Set(3, m_occupier->nickname());
                        dt.Set(4, 0);
                        dt.Set(5, "");
                        dt.Set(6, "");
                        dt.Set(7, kills);
                        string plcname = "";
                        //第一次王城争霸就发空，后面的就发玩家改了的名字。 通过国王记录来判断
                        if (plc.successiveKings().size() != 0)
                        {
                            plcname = g_mapMgr->serverName();
                        }
                        dt.Set(8,plcname);
                        string param = dt.Serialize();
                        AddPalaceWarRecord(PalaceWarRecordType::CATAPULT_ATTACK_NPC, param);
                    }
                    SetDirty();
                }
            
            }
            else if (m_occupierId != plc.occupierId()) {
                //attack player
                // const CatapultConf::AttackPlayer& conf = catapultConf.attackPlayer;
                float attack =  2000;//conf.baseAttack + armyCountLevel() * conf.correction * (1 + conf.gainCoe * catapultCount());
                int kills = plc.PlayersBeAttackByCatapult(attack);
                if (kills > 0) {
                    m_lastKills = kills;
                    m_kills += kills;
                    m_lastKillTimestamp = g_dispatcher->GetTimestampCache();
                    NoticeUnitUpdate();
                    //记录
                    if (m_occupier) {
                        DataTable dt;
                        dt.Set(1, m_occupier->headId());
                        dt.Set(2, m_occupier->allianceName());
                        dt.Set(3, m_occupier->nickname());
                        dt.Set(4, plc.occupier()->headId());
                        dt.Set(5, plc.occupier()->allianceName());
                        dt.Set(6, plc.occupier()->nickname());
                        dt.Set(7, kills);
                        string plcname = "";
                        //第一次王城争霸就发空，后面的就发玩家改了的名字。 通过国王记录来判断
                        if (plc.successiveKings().size() != 0)
                        {
                            plcname = g_mapMgr->serverName();
                        }
                        dt.Set(8,plcname);
                        string param = dt.Serialize();
                        AddPalaceWarRecord(PalaceWarRecordType::CATAPULT_ATTACK_PLAYER, param);
                    }
                    SetDirty();
                }
            }
            NoticeUnitUpdate();
            
        }
            
/*        const std::string& Catapult::nickname()
        {
            if (m_troop) {
                return m_troop->nickname();
            }
            return "";
        }*/

        void Catapult::OnPlaceWarStart()
        {
            SetState(model::PalaceState::NPC);
            m_occupierId = 0;
            Refresh();
            NoticeUnitUpdate();
            SetDirty();
        }

        void Catapult::OnPlaceWarEnd()
        {
            SetState(model::PalaceState::PROTECTED);
            ClearAllDefenders();
            m_occupierId = g_mapMgr->capital().occupierId();
            m_occupier = nullptr;
            m_armyListsTotal = 0;
            m_armyLists.clear();
            ResetKillInfos();
            NoticeUnitUpdate();
            m_maintainer.ClearAll();
            SetDirty();
        }

        void Catapult::OnTroopLoadFinished(Troop* troop)
        {
            if (troop->IsReach()) {
                m_defenders.emplace(troop->uid(), troop);
            }
        }

        void Catapult::OnPalaceOccupierIdChanged()
        {
            ResetKillInfos();
            NoticeUnitUpdate();
        }

        void Catapult::OnSelfOccupierIdChanged()
        {
            ResetKillInfos();
        }

        void Catapult::ResetKillInfos()
        {
            m_lastKills = 0;
            m_kills = 0;
            m_lastKillTimestamp = 0;
            SetDirty();
        }

        int Catapult::Power()
        {
            if (m_troop)
            {
                // 先就用兵数的十分之一
                return m_troop->armyList()->ArmyCount(ArmyState::NORMAL) * 2;
                // return m_troop->armyList()->Power();
            }
            return 0;
        }

        const int Catapult::CurrentArmyTotal()
        {
            int armyListsTotal = 0;
            for (std::list<ArmyList>::iterator it = m_armyLists.begin(); it != m_armyLists.end(); ++it)
            {
                armyListsTotal += (*it).ArmyCount(ArmyState::NORMAL);
            }
            return armyListsTotal;
        }

        void Catapult::AddPalaceWarRecord(PalaceWarRecordType type, const string& param)
        {
            //cout << "Catapult::AddPalaceWarRecord type = " << (int)type << " param = " << param << endl;
            PalaceWarRecord pwr;
            pwr.type = type;
            pwr.param = param;
            pwr.timestamp = g_dispatcher->GetTimestampCache();
            m_palaceWarRecords.push_front(pwr);
            if (m_palaceWarRecords.size() > 100) {
                m_palaceWarRecords.pop_back();
            }
        }

        void Catapult::RepatriateAllDefenders()
        {
            ClearAllDefenders();
            NoticeUnitUpdate();
        }

        void Catapult::Occupy(Troop* troop)
        {
            LOG_DEBUG("CampFixed::Occupy : %d ", troop->id());
            if (troop->allianceId() !=  m_occupierId) {
                std::list<Troop*> troops = m_troops;
                ClearAllDefenders();
                m_troopId = troop->id();
            } 

            if (troop) {
                m_troop = troop;
                m_troops.push_back(troop);
                m_troopIds.push_back(troop->id());
                //在这里清理一下NPC部队
                m_armyLists.clear();
                m_uid = troop->uid();
                m_occupier = &m_troop->agent();
            }
            m_maintainer.ClearByTag(ATTACK_PALACE);
            m_maintainer.Add(g_dispatcher->quicktimer().SetIntervalWithLinker(std::bind(&Catapult::AttackPalace, this), g_tploader->configure().catapult.attackTime * 1000), ATTACK_PALACE);
            m_occupierId = troop->allianceId();
            SetOccupier(m_occupier);
            m_isOccupy = true;
            SetState(model::PalaceState::OCCUPY);
            UpdateDefTroop();
            //增加箭塔占领记录
            base::DataTable dt;
            dt.Set(1, m_occupier->headId());
            dt.Set(2, m_occupier->allianceName());
            dt.Set(3, m_occupier->nickname());
            string param = dt.Serialize();
            AddPalaceWarRecord(PalaceWarRecordType::OCCUPY_CATAPULT, param);
            NoticeUnitUpdate();
            SetDirty();
        }



        void Catapult::ClearNpc()
        {
            
        }


        bool Catapult::CanGarrison(Troop* troop) {
            //是否能驻守多只部队
            return true;
        }

        void Catapult::Garrison(Troop* troop) {
             if (troop) {
                LOG_DEBUG("Catapult::Garrison ");
                if (troop->allianceId() !=  m_occupierId) {
                    LOG_DEBUG("Catapult  alliance not the same");
                    return;
                }

                m_troops.push_back(troop);
                m_troopIds.push_back(troop->id());

                UpdateDefTroop();
                SetDirty();
            }
        }

        void Catapult::UpdateDefTroop() {
            Troop* defTroop = nullptr;
            if (!m_troops.empty()) {
                defTroop = m_troops.back();
            }

            if (m_troop) {
                m_troop->agent().RemoveRelatedUnit(this);
            }

            m_troop = defTroop;

            if (m_troop) {
                m_troop->agent().AddRelatedUnit(this);
            }
        }

        bool Catapult::SwitchTroop() {
            //现在是弹出队列尾的处理 
            //1.如果NPC队列不为空则先走NPC队列的
            bool ret = false;
            if (!m_armyLists.empty()) {
                //取出最上层的军队
                m_armyLists.pop_back();
                if (!m_armyLists.empty()) {
                    ret = true;
                    return true;
                }
            }
            //2.再处理玩家首军
            Troop* defTroop = nullptr;
            if (!m_troops.empty()) {
                auto troop = m_troops.back();
                troop->GoHome();
                if (!m_troops.empty()) {
                    defTroop = m_troops.back();
                    ret = true;
                }
            }

            if (m_troop) {
                m_troop->agent().RemoveRelatedUnit(this);
            }

            m_troop = defTroop;

            if (m_troop) {
                m_troop->agent().AddRelatedUnit(this);
            }

            return ret;
        }

        void Catapult::OnTroopLeave(Troop* troop) {
            if (std::find(m_troops.begin(),  m_troops.end(),  troop) !=  m_troops.end()) {
                m_troops.remove(troop);
                m_troopIds.remove(troop->id());

                if (troop ==  m_troop) {
                    UpdateDefTroop();
                }
                NoticeUnitUpdate();
                SetDirty();
            }
        }

        void Catapult::OnTroopBack(Troop* troop) { 

        }

        void Catapult::OnTroopReach(Troop* troop) { 

        }

        string Catapult::Serialize()
        {
            // cout << "save catapult data..................." << endl;
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

                writer.String("occupierId");
                writer.Int64(m_occupierId);

                writer.Key("troopId");
                writer.StartArray();
                for (auto troop :  m_troops) {
                    if (troop) {
                        writer.Int(troop->id());
                    }
                }
                writer.EndArray();

                writer.String("state");
                writer.Int((int)m_state);
                writer.String("occupier");
                writer.Int64(m_occupier ? m_occupier->uid() : 0);
                writer.String("lastKills");
                writer.Int(m_lastKills);
                writer.String("lastKillTimestamp");
                writer.Int64(m_lastKillTimestamp);
                writer.String("kills");
                writer.Int(m_kills);

                writer.Key("armyLists");
                writer.StartArray();
                for (auto it = m_armyLists.begin(); it != m_armyLists.end(); ++it)
                {
                    (*it).Serialize(writer);
                }
                writer.EndArray();
                writer.String("armyListsTotal");
                writer.Int64(m_armyListsTotal);
                writer.EndObject();
                jsonString = jsonbuffer.GetString();
            } catch (std::exception& ex) {
                LOG_ERROR("save Catapult json fail: %s\n", ex.what());
            }
            return jsonString;
        }

        bool Catapult::Deserialize(string data)
        {
            // cout << "==================catapult data================" << endl;
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

                    m_occupierId = doc["occupierId"].GetInt64();

                    if (doc["troopId"].IsArray()) {
                        auto& temp = doc["troopId"];

                        for (int i = 0; i < (int)temp.Size(); ++i) {
                            int troopId = temp[i].GetInt();
                            m_troopIds.push_back(troopId);
                        }
                    }
                    m_state = static_cast<model::PalaceState>(doc["state"].GetInt());
                    int64_t uid = doc["occupier"].GetInt64();
                    m_occupier = g_mapMgr->FindAgentByUID(uid);
                    m_lastKills = doc["lastKills"].GetInt();
                    m_lastKillTimestamp = doc["lastKillTimestamp"].GetInt64();
                    m_kills = doc["kills"].GetInt();
                    auto& armyLists = doc["armyLists"];
                    for (size_t i = 0; i < armyLists.Size(); ++i) {
                        ArmyList armyList;
                        armyList.Desrialize(armyLists[i]);
                        m_armyLists.push_back(armyList);
                    }
                    m_armyListsTotal = doc["armyListsTotal"].GetInt64();
                }
                return true;
            } catch (std::exception& ex) {
                LOG_ERROR("json string to Catapult fail: %s\n", ex.what());
                return false;
            }
        }

        void Catapult::FinishDeserialize()
        {   
            if (m_troopIds.size() > 0) {
                for (auto troopId :  m_troopIds) {
                  auto troop = g_mapMgr->FindTroop(troopId);
                  if (troop) {
                      m_troops.push_back(troop);
                  }
                }
                UpdateDefTroop();
            }
            if (!g_mapMgr->capital().isNotStart()) {
                //投放攻击王城的定时器
                m_maintainer.ClearByTag(ATTACK_PALACE);
                m_maintainer.Add(g_dispatcher->quicktimer().SetIntervalWithLinker(std::bind(&Catapult::AttackPalace, this), g_tploader->configure().catapult.attackTime * 1000), ATTACK_PALACE);
            }
        }


    }
}

