#include <algorithm>
#include "camp.h"
#include <base/logger.h>
#include "../agent.h"
#include <model/tpl/templateloader.h>
#include <model/tpl/configure.h>

namespace ms
{
    namespace map
    {
        CampTemp::CampTemp(int id, const MapUnitCampTempTpl* tpl, Troop* troop)
            : Unit(id, tpl), m_troop(troop)
        {
            if(m_troop) {
                auto m_agent = &m_troop->agent();
                if (m_agent) {
                    m_agent->AddCampTemp(this);
                }
            }
        }

        CampTemp::~CampTemp()
        {

        }

        int64_t CampTemp::allianceId() const
        {
            if (m_troop) {
                return m_troop->allianceId();
            }
            return 0;
        }

        bool CampTemp::RemoveSelf()
        {
            if (m_troop) {
                auto m_agent = &m_troop->agent();
                if (m_agent) {
                    m_agent->RemoveCampTemp(this);
                }
            }
            return Unit::RemoveSelf();
        }

        void CampTemp::OnTroopLeave(Troop* troop)
        {
            //std::cout << "CampTemp::OnTroopLeave" << std::endl;
            if (troop->id() == m_troop->id()) {
                RemoveSelf();
            }
        }
        
        
         CampFixed::CampFixed(int id, const MapUnitCampFixedTpl* tpl, Troop* troop)
            : Unit(id, tpl), m_troop(troop)
        {
            if(m_troop) {
                m_troopId = m_troop->id();
                m_bindTroopId = m_troopId;
                m_uid = m_troop->uid();
                m_agent = &m_troop->agent();
                if (m_agent) {
                    m_agent->AddCampFixed(this);
                }
                m_troops.push_back(troop);
                m_troopIds.push_back(troop->id());
            }
            m_campFixedDurableMax = g_tploader->configure().cortressDurable.durable;
            m_campFixedDurable = m_campFixedDurableMax;
        }

        CampFixed::~CampFixed()
        {

        }

        const std::string CampFixed::nickname() const
        {
            if (!m_agent) {
                return "";
            }
            return m_agent->nickname();
        }

        int64_t CampFixed::allianceId() const
        {
            if (m_agent) {
                return m_agent->allianceId();
            }
            return 0;
        }

        void CampFixed::Occupy(Troop* troop)
        {
            LOG_DEBUG("CampFixed::Occupy : %d ", troop->id());
            if (troop->allianceId() !=  allianceId()) {
                std::list<Troop*> troops = m_troops;
                for (auto troop :  troops) {
                    if (troop) {
                        troop->GoHome();
                    }
                }

                m_troopId = troop->id();
                m_bindTroopId = m_troopId;
            } 

            if (troop) {
                m_troop = troop;
                m_troops.push_back(troop);
                m_troopIds.push_back(troop->id());
                if (m_uid !=  troop->uid()) {
                    if (m_agent) {
                        m_agent->RemoveCampFixed(this);
                    }
                    m_uid = troop->uid();
                    m_agent = &m_troop->agent();
                    if (m_agent) {
                        m_agent->AddCampFixed(this);
                    }
                }
                troop->SetCurrentPos(m_pos);
            }
            UpdateDefTroop();
            NoticeUnitUpdate();
            SetDirty();
        }

        bool CampFixed::CanGarrison(Troop* troop)
        {
            //是否能驻守多只部队
            return true;
        }

        void CampFixed::Garrison(Troop* troop)
        {
            if (troop) {
                LOG_DEBUG("CampFixed::Garrison ");
                if (troop->allianceId() !=  allianceId()) {
                    LOG_DEBUG("Garrison  alliance not the same");
                    return;
                }

                m_troops.push_back(troop);
                m_troopIds.push_back(troop->id());

                UpdateDefTroop();
                SetDirty();
            }
        }

        void CampFixed::UpdateDefTroop()
        {
            Troop* defTroop = nullptr;
            if (!m_troops.empty()) {
                defTroop = m_troops.back();
            }

            if (m_troop) {
                m_troop->agent().RemoveCampFixed(this);
            }

            m_troop = defTroop;

            if (m_troop) {
                m_troop->agent().AddCampFixed(this);
            }
        }

        bool CampFixed::SwitchTroop()
        { 
            //m_troop.army_List().IsAllDie() 
            //m_troops.pop_back();

            //现在是弹出队列尾的处理 
            Troop* defTroop = nullptr;
            bool ret = false;

            if (!m_troops.empty()) {
                auto troop = m_troops.back();
                troop->GoHome();
                if (!m_troops.empty()) {
                    defTroop = m_troops.back();
                    ret = true;
                }
            }

            if (m_troop) {
                m_troop->agent().RemoveCampFixed(this);
            }

            m_troop = defTroop;

            if (m_troop) {
                m_troop->agent().AddCampFixed(this);
            }

            return ret;
        }

        bool CampFixed::RemoveSelf()
        {
            if (m_agent) {
                // 驻军全部回家
                while(!m_troops.empty())
                {
                    Troop* tp = m_troops.front();
                    tp->GoHome();
                }
                m_agent->RemoveCampFixed(this);
            }
            return Unit::RemoveSelf();
        }

        void CampFixed::OnTroopLeave(Troop* troop)
        {
            // if (m_troop && troop == m_troop) {
            //     m_troop = nullptr;
            //     m_troopId = 0;
            //     NoticeUnitUpdate();
            //     SetDirty();
            // }

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
        
        void CampFixed::OnTroopBack(Troop* troop)
        {
            if (m_troop && troop == m_troop) {
                m_troop = nullptr;
                m_troopId = 0;
                m_bindTroopId = 0;
                NoticeUnitUpdate();
                SetDirty();
            }
        }
        
        void CampFixed::OnTroopReach(Troop* troop)
        {
        }

        void CampFixed::AddCampFixedDurable(int add)
        {
            m_campFixedDurable += add;
            if (m_campFixedDurable >=  m_campFixedDurableMax) {
                m_campFixedDurableRecover = 0;
                m_campFixedDurableRecoverTime = 0;
                m_updateSeconds = 0;
                m_campFixedDurable = m_campFixedDurableMax;
            }

            if (m_campFixedDurable < 0) {
                m_campFixedDurable = 0;
            }
            SetDirty();
        }

        void CampFixed::OnIntervalUpdate(size_t updateSeconds)
        {
             if (m_campFixedDurableRecover > 0) {
                if (m_campFixedDurableRecoverTime > 0) {
                    ++m_updateSeconds;

                    if (m_updateSeconds % 60 == 0) {
                        m_campFixedDurableRecoverTime += 60;
                        AddCampFixedDurable(m_campFixedDurableRecover);
                        NoticeUnitUpdate();
                        SetDirty();
                    }
                }
            }
        }


        void CampFixed::StartRecoverCampFixedDurable()
        {
            int64_t now = g_dispatcher->GetTimestampCache();
            m_campFixedDurableRecoverTime = now;
            UpdateCampFixedDurable();
            SetDirty();
        }

        void CampFixed::UpdateCampFixedDurable()
        {
            m_campFixedDurableRecover = 100;
            SetDirty();
        }


        std::string CampFixed::Serialize()
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

                writer.Key("troopId");
                writer.StartArray();
                for (auto troop :  m_troops) {
                    if (troop) {
                        writer.Int(troop->id());
                    }
                }
                writer.EndArray();

                writer.Key("bindTroopId");
                writer.Int(bindTroopId());

                writer.Key("uid");
                writer.Int64(uid());

                writer.Key("campFixedDurable");
                writer.Int64(m_campFixedDurable);

                writer.Key("campFixedDurableRecoverTime");
                writer.Int64(m_campFixedDurableRecoverTime);

                writer.EndObject();
                jsonString = jsonbuffer.GetString();
            } catch (std::exception& ex) {
                LOG_ERROR("save CampFixed json fail: %s\n", ex.what());
            }
            return jsonString;
        }

        bool CampFixed::Deserialize(std::string data)
        {
            try {
                rapidjson::Document doc;
                doc.Parse<0>(data.c_str());
                if (doc.IsObject()) {

                    if (doc["pos"].IsArray()) {
                        auto& temp = doc["pos"];
                        m_pos.x = temp[0u].GetInt();
                        m_pos.y = temp[1].GetInt();
                    }

                    if (doc["troopId"].IsArray()) {
                        auto& temp = doc["troopId"];

                        for (int i = 0; i < (int)temp.Size(); ++i) {
                            int troopId = temp[i].GetInt();
                            m_troopIds.push_back(troopId);
                        }
                    }

                    m_bindTroopId = doc.HasMember("bindTroopId")  ? doc["bindTroopId"].GetInt() : 0;
                    m_uid = doc["uid"].GetInt64();
                    m_campFixedDurable = doc.HasMember("campFixedDurable")  ? doc["campFixedDurable"].GetInt64() : 0;
                    m_campFixedDurableRecoverTime = doc.HasMember("campFixedDurableRecoverTime")  ? doc["campFixedDurableRecoverTime"].GetInt64() : 0;    
                }
                return true;
            } catch (std::exception& ex) {
                LOG_ERROR("json string to CampFixed fail: %s\n", ex.what());
                return false;
            }
        }

        void CampFixed::FinishDeserialize()
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

            //m_troop = g_mapMgr->FindTroop(m_troopId);
            m_agent = g_mapMgr->FindAgentByUID(m_uid);
            if (!m_agent) {
                LOG_ERROR("CampFixed::FinishDeserialize can not find agent uid = %d", m_uid);
            } else {
                m_agent->AddCampFixed(this);
            }

            // 恢复耐久值
            if (m_campFixedDurableRecoverTime > 0) {
                int64_t now = g_dispatcher->GetTimestampCache();
                int intervalSecond = now - m_campFixedDurableRecoverTime;
                if (intervalSecond > 0) {
                    UpdateCampFixedDurable();
                    AddCampFixedDurable(intervalSecond/ 60 * m_campFixedDurableRecover);
                    if (m_campFixedDurableRecoverTime > 0) {
                        m_updateSeconds = intervalSecond % 60;
                        m_campFixedDurableRecoverTime = now - m_updateSeconds;
                    }
                }
            }
        }
    }
}
