#include <algorithm>
#include "castle.h"
#include "../agent.h"
#include "../agentProxy.h"
#include "../troop.h"
#include "../info/agentinfo.h"
#include <model/tpl/templateloader.h>
#include <model/tpl/configure.h>
#include <base/logger.h>
#include <base/3rd/rapidjson/document.h>
#include <base/3rd/rapidjson/stringbuffer.h>
#include <base/3rd/rapidjson/writer.h>



namespace ms
{
    namespace map
    {
        using namespace std;
        using namespace base;
        using namespace model;
        using namespace info;
        using namespace model::tpl;

        Castle::Castle(int id, const MapUnitCastleTpl* tpl, Agent* agent, int level)
            : Unit(id, tpl), m_agent(agent), m_level(level)
        {
        
        }

        Castle::~Castle()
        {
        }

        int64_t Castle::uid() const
        {
            return m_agent->uid();
        }

        int64_t Castle::allianceId() const
        {
            return m_agent->allianceId();
        }

        const string& Castle::allianceNickname() const
        {
            return m_agent->allianceNickname();
        }

        const std::string& Castle::nickname() const
        {
            return m_agent->nickname();
        }

        int Castle::allianceBannerId() const
        {
            return m_agent->allianceBannerId();
        }

        ArmyList* Castle::DefArmyList()
        {
            return m_agent->defArmyList();
        }

        int Castle::getDefTeam() const
        {
            return m_agent->getDefTeam();
        }

        std::array<ArmyList, 5>& Castle::armyListArray() {
            return m_agent->armyListArray();
        }

        void Castle::AddCityDefense(int add)
        {
            m_cityDefense += add;
            if (m_cityDefense > cityDefenseMax()) {
                m_cityDefense = cityDefenseMax();
            }
            if (m_cityDefense < 0) {
                m_cityDefense = 0;
            }
            SetDirty();
            m_agent->msgQueue().AppendMsgCityDefenseUpdate(m_cityDefense);
        }

        int Castle::loseCityDefense() const
        {
            int lose = m_agent->property().cityDefense - m_cityDefense;
            return lose > 0 ? lose : 0;
        }

        int Castle::cityDefenseMax() const
        {
            return m_agent->property().cityDefense;
        }

        int Castle::burnLeftTime() const
        {
            int64_t now = g_dispatcher->GetTimestampCache();
            return m_burnEndTimestamp > now ? m_burnEndTimestamp - now : 0;
        }

        int Castle::silverRepairLeftTime() const
        {
            int64_t now = g_dispatcher->GetTimestampCache();
            return m_silverRepairEndTimestamp > now ? m_silverRepairEndTimestamp - now : 0;
        }

        int Castle::goldRepairLeftTime() const
        {
            int64_t now = g_dispatcher->GetTimestampCache();
            return m_goldRepairEndTimestamp > now ? m_goldRepairEndTimestamp - now : 0;
        }

        int Castle::titleId() const
        {
            return m_agent->titleId();
        }

        bool Castle::CanAttack(Troop *troop) {
            if (troop) {
                return troop->allianceId() == 0 || allianceId() != troop->allianceId();
            }
            return false;
        }

        int Castle::IsPeaceShield() const
        {
            int time = 0;
            //城市buff
            for (auto it = m_agent->cityBuffs().begin(); it != m_agent->cityBuffs().end(); ++it) {
                const BuffInfo& buff = it->second;
                if (it->first == (int)model::BuffType::PEACE_SHIELD) {
                    time = buff.endTimestamp - g_dispatcher->GetTimestampCache();
                    break;
                }
            }

            auto alBuff = m_agent->GetValidAllianceBuff(model::AllianceBuffType::PEACE_SHIELD);
            if (alBuff) {
                int time2 = alBuff->endTimestamp - g_dispatcher->GetTimestampCache();
                if (time < time2) {
                    time = time2;
                }
            }
            
            if (time < 0) {
                time = 0;
            }
            return time;
        }

        bool Castle::IsAntiScout() const
        {
            for (auto it = m_agent->cityBuffs().begin(); it != m_agent->cityBuffs().end(); ++it) {
                const BuffInfo& buff = it->second;
                if (it->first == (int)model::BuffType::ANTI_SCOUT && buff.endTimestamp > g_dispatcher->GetTimestampCache()) {
                    return true;
                }
            }

            if (m_agent->GetValidAllianceBuff(model::AllianceBuffType::ANTI_SCOUT)) {
                return true;
            }

            return false;
        }

        bool Castle::IsNoPlunder() const
        {
//             for (auto it = m_agent->skillBuffs().begin(); it != m_agent->skillBuffs().end(); ++it) {
//                 const BuffInfo& buff = it->second;
//                 if (it->first == (int)model::BuffType::NO_PLUNDER && buff.endTimestamp > g_dispatcher->GetTimestampCache()) {
//                     return true;
//                 }
//             }
            return false;
        }

        bool Castle::IsNcProtecting() const
        {
            // cout << "IsNcProtecting " << g_dispatcher->GetTimestampCache()  << " " << m_ncProtectEndTime << endl;
            return g_dispatcher->GetTimestampCache() < m_ncProtectEndTime;
        }

        void Castle::Burn()
        {
            //cout << "BURN" << endl;
            m_maintainer.ClearByTag(TAG_BURN_END);
            auto &wallsValue = g_tploader->configure().wallsValue;
            if (m_state == CastleState::BURN) {
                ++m_burnTimes;
                m_burnEndTimestamp += wallsValue.burnincreasetime;
            } else {
                m_state = CastleState::BURN;
                m_burnBeginTimestamp = g_dispatcher->GetTimestampCache();
                m_burnEndTimestamp = m_burnBeginTimestamp + wallsValue.burnbasetime;
                m_maintainer.Add(g_dispatcher->quicktimer().SetIntervalWithLinker(std::bind(&Castle::OnBurning, this), wallsValue.span * 1000), TAG_BURNING);
            }
            SetDirty();
            m_maintainer.Add(g_dispatcher->quicktimer().SetIntervalWithLinker(std::bind(&Castle::OnBurnEnd, this), burnLeftTime() * 1000), TAG_BURN_END);
            NoticeUnitUpdate();
            m_agent->proxy()->SendMapPersonalInfoUpdate();
        }

        bool Castle::OutFire()
        {
            //cout << "OutFire" << endl;
            if (IsBurning()) {
                OnBurnEnd();
                return true;
            }
            return false;
        }

        bool Castle::CanGarrison(Troop* troop) 
        {
            //是否能驻守多只部队
            return true;
        }

        void Castle::Garrison(Troop* troop)
        {
            if (troop) {
                LOG_DEBUG("Castle::Garrison ");
                if (troop->allianceId() !=  allianceId()) {
                    LOG_DEBUG("Castle Garrison  alliance not the same");
                    return;
                }
                RemoveMarchReinforcement(troop);
                // 如果增援部队达到增援部队数量上限
                if ((int)m_reachReinforcementTroopIds.size() >= m_agent->allianceReinforcementNum())
                {
                    troop->GoHome();
                    return;
                }
                m_troops.push_back(troop);
                m_troopIds.push_back(troop->id());
                AddReachReinforcement(troop);

                UpdateDefTroop();
                // UpdateCityDefRecover();
                SetDirty();
            }
        }

        void Castle::BackTroop(Troop* troop)
        {   
            if (std::find(m_troops.begin(),  m_troops.end(),  troop) !=  m_troops.end()) {
                troop->GoHome();
            }
            //cout << "RemoveReachReinforcement uid = " << uid << endl;
            // m_reinforcements.erase(uid);
        }

        void Castle::UpdateDefTroop()
        {            
            //现在是取队列尾的处理 
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

        bool Castle::SwitchTroop()
        { 
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
                m_troop->agent().RemoveRelatedUnit(this);
            }

            m_troop = defTroop;

            if (m_troop) {
                m_troop->agent().AddRelatedUnit(this);
            }

            return ret;
        }

        bool Castle::IsLastTroop()
        {
            bool ret = false;
            if (m_troops.size() == 1 || m_troops.empty()) {
                ret = true;
            }
            
            return ret;
        }

        void Castle::OnBurning()
        {
            // 间隔时间减少的城防值=城防值*{初始减少百分比+（燃烧时被打次数*系数1）%}+初始减少基数+燃烧时被打次数*系数2
            auto &wallsValue = g_tploader->configure().wallsValue;
            int reduce = wallsValue.reduce;
            if (m_cityDefense > reduce) {
                AddCityDefense(-reduce);
            } else {
                g_mapMgr->RebuildCastle(this);
                return;
            }
            SetDirty();
            m_agent->proxy()->SendMapPersonalInfoUpdate();
        }

        void Castle::OnBurnEnd()
        {
            //cout << "OnBurnEnd" << endl;
            m_maintainer.ClearByTag(TAG_BURN_END);
            m_maintainer.ClearByTag(TAG_BURNING);
            if (m_state == CastleState::PROTECTED_AND_BURN) {
                m_state = CastleState::PROTECTED;
            } else {
                m_state = CastleState::NORMAL;
            }
            m_burnTimes = 0;
            m_burnBeginTimestamp = 0;
            m_burnEndTimestamp = 0;
            SetDirty();
            m_agent->proxy()->SendMapPersonalInfoUpdate();
            NoticeUnitUpdate();
        }

        void Castle::PeaceShield(int time)
        {
            //cout << "PeaceShield time = " << time << endl;
            m_maintainer.ClearByTag(TAG_PEACE_END);
            m_maintainer.Add(g_dispatcher->quicktimer().SetTimeoutWithLinker(std::bind(&Castle::OnPeaceEnd, this), time * 1000), TAG_PEACE_END);
            if (m_state != CastleState::PROTECTED && m_state != CastleState::PROTECTED_AND_BURN) {
                if (m_state == CastleState::BURN) {
                    m_state = CastleState::PROTECTED_AND_BURN;
                } else {
                    m_state = CastleState::PROTECTED;
                }
                SetDirty();
                NoticeUnitUpdate();
                m_agent->proxy()->SendMapPersonalInfoUpdate();
            }
        }

        void Castle::RemovePeaceShield()
        {
            OnPeaceEnd();
        }

        void Castle::OnPeaceEnd()
        {
            //cout << "Castle::OnPeaceEnd" << endl;
            m_maintainer.ClearByTag(TAG_PEACE_END);
            if (IsProtected()) {
                if (m_state == CastleState::PROTECTED_AND_BURN) {
                    m_state = CastleState::BURN;
                } else {
                    if (m_state == CastleState::NOVICE) {
                        m_noviceTimestamp = g_dispatcher->GetTimestampCache();
                    }
                    m_state = CastleState::NORMAL;
                }
                SetDirty();
                NoticeUnitUpdate();
                m_agent->proxy()->SendMapPersonalInfoUpdate();
            }
        }

        bool Castle::Repair(bool isGoldRepair)
        {
            //cout << "Repair" << endl;
            int64_t now = g_dispatcher->GetTimestampCache();
            auto& wallsRecovery = g_tploader->configure().wallsRecovery;
            if (!isGoldRepair) {
                if (now >= m_silverRepairEndTimestamp) {
                   AddCityDefense(cityDefenseMax() * wallsRecovery.monneycover + wallsRecovery.monneycoverbase);
                    m_silverRepairEndTimestamp = now + wallsRecovery.monneycd;
                    SetDirty();
                    m_agent->proxy()->SendMapPersonalInfoUpdate();
                    return true;
                }
            } else {
                if (now >= m_goldRepairEndTimestamp) {
                    int max = cityDefenseMax();
                    AddCityDefense(max);
                    m_goldRepairEndTimestamp = now + wallsRecovery.goldcd;
                    SetDirty();
                    m_agent->proxy()->SendMapPersonalInfoUpdate();
                    return true;
                }
            }

            return false;
        }

        void Castle::OnRebuild()
        {
            ClearAllReinforcement();
            m_maintainer.ClearAll();

            m_state = CastleState::NORMAL;
            auto& wallsValue = g_tploader->configure().wallsValue;
            m_cityDefense = 0;
            AddCityDefense(cityDefenseMax() * wallsValue.recovery);
            m_burnTimes = 0;
            m_burnBeginTimestamp = 0;
            m_burnEndTimestamp = 0;
            SetDirty();
//             m_agent->ReCalculateNcProperty();
            m_agent->SetCastlePos(pos());
            m_agent->proxy()->SendMapPersonalInfoUpdate();
            m_agent->OnCastleRebuild();
        }

        bool Castle::AddMarchReinforcement(Troop* troop)
        {
            //cout << "AddMarchReinforcement uid = " << troop->uid() << " tid = " << troop->id() << endl;
            // m_reinforcements.emplace(troop->uid(), troop);
            m_marchReinforcementTroops.push_back(troop);
            m_marchReinforcementTroopIds.push_back(troop->id());
            return true;
        }

        bool Castle::RemoveMarchReinforcement(Troop* troop)
        {
            //cout << "RemoveReachReinforcement uid = " << uid << endl;
            if ( std::find(m_marchReinforcementTroops.begin(),  m_marchReinforcementTroops.end(),  troop) !=  m_marchReinforcementTroops.end()) {
                m_marchReinforcementTroops.remove(troop);
                m_marchReinforcementTroopIds.remove(troop->id());
                return true;
            }
            return false;
        }

        bool Castle::AddReachReinforcement(Troop* troop)
        {
            m_reachReinforcementTroops.push_back(troop);
            m_reachReinforcementTroopIds.push_back(troop->id());
            return true;
        }

        bool Castle::RemoveReachReinforcement(Troop* troop)
        {
            //cout << "RemoveReachReinforcement uid = " << uid << endl;
            if ( std::find(m_reachReinforcementTroops.begin(),  m_reachReinforcementTroops.end(),  troop) !=  m_reachReinforcementTroops.end()) {
                m_reachReinforcementTroops.remove(troop);
                m_reachReinforcementTroopIds.remove(troop->id());
                return true;
            }
            return false;
        }

//
//         bool Castle::CanReinforce(int64_t uid, const ArmyList& armyList)
//         {
//             //cout << "Castle::CanReinforce" << endl;
//             for (auto it = m_reinforcements.begin(); it != m_reinforcements.end(); ++it) {
//                 if (uid == it->first) {
//                     //cout << "uid is exist uid = " << uid << endl;
//                     return false;
//                 }
//             }
//             if (reinforcementsTotal() + armyList.GetArmyTotal() > m_agent->property().reinforcementsMax) {
//                 cout << "over max " << "reinforcementsTotal = " << reinforcementsTotal() << " armyTotal = " << armyList.GetArmyTotal() << " Max = " << m_agent->property().reinforcementsMax << endl;
//                 return false;
//             }
//             return true;
//         }

        void Castle::OnLevelUp(int level)
        {
            if (m_level >= level) return;
            if (const MapUnitTpl* tpl = map::tpl::m_tploader->FindMapUnit(model::MapUnitType::CASTLE, level)) {
                SetLevel(level);
                SetTpl(tpl);
                if (m_state == CastleState::NOVICE && m_level >= g_tploader->configure().noviceProtected.level) {
                    OnPeaceEnd();
                }
                NoticeUnitUpdate();
            }
        }

        void Castle::OnTroopMarch(Troop* troop)
        {
            if (!troop->agent().isPlayer()) {
                m_ncProtectEndTime = g_dispatcher->GetTimestampCache() + g_tploader->configure().neutralCastle.playerProtectedTime;
                // cout << "uid = " << uid() << " m_ncProtectEndTime = " << m_ncProtectEndTime << endl;
                SetDirty();
                return;
            }

            switch (troop->type()) {
                case MapTroopType::REINFORCEMENTS: {
                    AddMarchReinforcement(troop);
                    SetDirty();
                }
                break;
                default:
                    break;
            }
        }

        void Castle::OnTroopReach(Troop* troop)
        {

        }

        void Castle::OnTroopLeave(Troop* troop)
        {
            if (troop->type() == MapTroopType::REINFORCEMENTS) {
                 RemoveReachReinforcement(troop);
            }
           
            if ( std::find(m_troops.begin(),  m_troops.end(),  troop) !=  m_troops.end()) {
                m_troops.remove(troop);
                m_troopIds.remove(troop->id());

                if (troop ==  m_troop) {
                    UpdateDefTroop();
                }
            }
            SetDirty();
            NoticeUnitUpdate();
        }

        void Castle::OnTeleport()
        {
            ClearAllReinforcement();
//             m_agent->ReCalculateNcProperty();
            m_agent->SetCastlePos(pos());
            SetDirty();
            //cout << "reinforcements size = " << m_reinforcements.size() << endl;
        }

        void Castle::OnCityDefenseMaxChange(int oldV, int newV)
        {
            int v = newV - oldV;
            if (v > 0) {
                if (oldV == 0) {
                    if (m_cityDefense == 0) {
                        AddCityDefense(newV);
                        SetDirty();
                    }
                } else {
                    AddCityDefense(v);
                    SetDirty();
                }
                m_agent->proxy()->SendMapPersonalInfoUpdate();
            }
        }

        void Castle::CheckIsNovice()
        {
            //cout << "Castle::CheckIsNovice" << endl;
            if (m_noviceTimestamp == 0) {
                m_noviceTimestamp = m_agent->registerTimestamp() + g_tploader->configure().noviceProtected.time;
                int64_t time = m_noviceTimestamp - g_dispatcher->GetTimestampCache();
                if (time > 0 && m_state == CastleState::NORMAL) {
                    //cout << "time = " << time << endl;
                    m_state = CastleState::NOVICE;
                    m_maintainer.Add(g_dispatcher->quicktimer().SetTimeoutWithLinker(std::bind(&Castle::OnPeaceEnd, this), time * 1000), TAG_PEACE_END);
                    SetDirty();
                }
                NoticeUnitUpdate();
            }
        }

        void Castle::CheckPeaceShield()
        {
            int time = IsPeaceShield();
            if (time > 0) {
                PeaceShield(time);
            }
        }

        void Castle::ClearAllReinforcement()
        {
            for (auto troop : m_reachReinforcementTroops) {
                if (troop->IsReach()) {
                    troop->GoHome();
                } else if (troop->IsMarching()) {
                    // 现在需求改成，到达再撤回
                    // tp->Repatriate();
                }
            }
            m_reachReinforcementTroops.clear();
            m_reachReinforcementTroopIds.clear();
            m_marchReinforcementTroops.clear();
            m_marchReinforcementTroopIds.clear();
            m_agent->proxy()->SendAllianceReinforcements();
            SetDirty();
        }

        void Castle::CheckIsSendReinforceTroop()
        {
            {
                m_agent->proxy()->SendAllianceReinforcements();
            }   
        }

        std::string Castle::Serialize()
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

                writer.Key("troopId");
                writer.StartArray();
                for (auto troop :  m_troops) {
                    if (troop) {
                        writer.Int(troop->id());
                    }
                }
                writer.EndArray();

                writer.Key("reachReinforcementIds");
                writer.StartArray();
                for (auto troop: m_reachReinforcementTroops) {
                    if (troop) {
                        writer.Int(troop->id());
                    }
                }
                writer.EndArray();

                writer.Key("marchReinforcementIds");
                writer.StartArray();
                for (auto troop: m_marchReinforcementTroops) {
                    if (troop) {
                        writer.Int(troop->id());
                    }
                }
                writer.EndArray();

                writer.String("uid");
                writer.Int64(m_agent->uid());
                writer.String("level");
                writer.Int(m_level);
                writer.String("state");
                writer.Int((int)m_state);
                writer.String("cityDefense");
                writer.Int(m_cityDefense);
                writer.String("silverRepairEndTimestamp");
                writer.Int64(m_silverRepairEndTimestamp);
                writer.String("goldRepairEndTimestamp");
                writer.Int64(m_goldRepairEndTimestamp);
                writer.String("burnTimes");
                writer.Int(m_burnTimes);
                writer.String("burnBeginTimestamp");
                writer.Int64(m_burnBeginTimestamp);
                writer.String("burnEndTimestamp");
                writer.Int64(m_burnEndTimestamp);
                writer.String("noviceTimestamp");
                writer.Int64(m_noviceTimestamp);

                writer.EndObject();
                jsonString = jsonbuffer.GetString();
            } catch (std::exception& ex) {
                LOG_ERROR("save Castle json fail: %s\n", ex.what());
            }
            return jsonString;
        }

        bool Castle::Deserialize(std::string data)
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

                    m_refreshTime = doc["refreshTime"].GetInt64();

                    if (doc["troopId"].IsArray()) {
                        auto& temp = doc["troopId"];

                        for (int i = 0; i < (int)temp.Size(); ++i) {
                            int troopId = temp[i].GetInt();
                            m_troopIds.push_back(troopId);
                        }
                    }

                    if (doc["reachReinforcementIds"].IsArray()) {
                        auto& temp = doc["reachReinforcementIds"];

                        for (int i = 0; i < (int)temp.Size(); ++i) {
                            int troopId = temp[i].GetInt();
                            m_reachReinforcementTroopIds.push_back(troopId);
                        }
                    }

                    if (doc["marchReinforcementIds"].IsArray()) {
                        auto& temp = doc["marchReinforcementIds"];

                        for (int i = 0; i < (int)temp.Size(); ++i) {
                            int troopId = temp[i].GetInt();
                            m_marchReinforcementTroopIds.push_back(troopId);
                        }
                    }


                    int64_t uid = doc["uid"].GetInt64();
                    m_agent = g_mapMgr->FindAgentByUID(uid);
                    if (!m_agent) {
                        LOG_ERROR("Castle::FinishDeserialize agent not found! uid = %ld\n", uid);
                    } else {
                        m_agent->SetCastlePos(pos());
                    }

                    m_level = doc["level"].GetInt();
                    m_state = static_cast<model::CastleState>(doc["state"].GetInt());
                    m_cityDefense = doc["cityDefense"].GetInt();
                    m_silverRepairEndTimestamp = doc["silverRepairEndTimestamp"].GetInt64();
                    m_goldRepairEndTimestamp = doc["goldRepairEndTimestamp"].GetInt64();
                    m_burnTimes = doc["burnTimes"].GetInt();
                    m_burnBeginTimestamp = doc["burnBeginTimestamp"].GetInt64();
                    m_burnEndTimestamp = doc["burnEndTimestamp"].GetInt64();
                    m_noviceTimestamp = doc["noviceTimestamp"].GetInt64();
                }
                return true;
            } catch (std::exception& ex) {
                LOG_ERROR("json string to Castle fail: %s\n", ex.what());
                return false;
            }
        }

        void Castle::FinishDeserialize()
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

            if (m_reachReinforcementTroopIds.size() > 0) {
                for (auto troopId : m_troopIds) {
                    auto troop = g_mapMgr->FindTroop(troopId);
                    if (troop) {
                        m_reachReinforcementTroops.push_back(troop);
                    }
                }
            }

            if (m_marchReinforcementTroopIds.size() > 0) {
                for (auto troopId : m_troopIds) {
                    auto troop = g_mapMgr->FindTroop(troopId);
                    if (troop) {
                        m_marchReinforcementTroops.push_back(troop);
                    }
                }
            }

            if (IsBurning()) {
                auto& wallsValue = g_tploader->configure().wallsValue;
                m_maintainer.Add(g_dispatcher->quicktimer().SetIntervalWithLinker(std::bind(&Castle::OnBurning, this), wallsValue.span * 1000), TAG_BURNING);
                m_maintainer.Add(g_dispatcher->quicktimer().SetIntervalWithLinker(std::bind(&Castle::OnBurnEnd, this), burnLeftTime() * 1000), TAG_BURN_END);
            }
            if (IsProtected()) {
                bool setTimer = false;
                if (m_state == CastleState::NOVICE) {
                    //新手保护
                    int time = m_noviceTimestamp - g_dispatcher->GetTimestampCache();
                    if (time > 0) {
                        m_maintainer.Add(g_dispatcher->quicktimer().SetTimeoutWithLinker(std::bind(&Castle::OnPeaceEnd, this), time * 1000), TAG_PEACE_END);
                        setTimer = true;
                    }
                } else {
                    //城市buff
                    int time = IsPeaceShield();
                    if (time > 0) {
                        m_maintainer.Add(g_dispatcher->quicktimer().SetTimeoutWithLinker(std::bind(&Castle::OnPeaceEnd, this), time * 1000), TAG_PEACE_END);
                        setTimer = true;
                    }
                }
                if (!setTimer) {
                    OnPeaceEnd();
                }
            }
        }

    }
}
