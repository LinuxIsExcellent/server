#include <algorithm>
#include "famouscity.h"
#include <base/logger.h>
#include "../tpl/mapcitytpl.h"
#include "../alliance.h"
#include <model/tpl/templateloader.h>
#include <model/tpl/alliance.h>
#include <model/tpl/configure.h>
#include "../agent.h"

namespace ms
{
    namespace map
    {
        using namespace model::tpl;

        FamousCity::FamousCity(int id, const MapUnitFamousCityTpl* tpl, const Point& bornPoint)
            : Unit(id, tpl), m_tpl(tpl), m_bornPoint(bornPoint), m_troop(nullptr)
        {
            if (m_tpl) {
                m_cityDefenseMax = m_tpl->cityDefense;
                m_cityDefense = m_tpl->cityDefense;
            }
            int cityId = m_bornPoint.x * 1200 + m_bornPoint.y;
            m_cityTpl = tpl::m_tploader->FindMapCity(cityId);
        }

        FamousCity::~FamousCity()
        {

        }
        
        void FamousCity::Init()
        {
//          int cityId = m_bornPoint.x * 1200 + m_bornPoint.y;
//          auto cityTpl = tpl::m_tploader->FindMapCity(cityId);
            if (m_tpl) {
                int armyGroup = m_tpl->armyGroup;
                int armyCount = m_tpl->armyCount;
                int armyNum = m_cityTpl->armyNum;
                m_armyListsTotal = 0;
                m_armyLists.clear();
                if (m_cityTpl) {
                    if (m_cityTpl->turretAtkPower > 0) {
                        m_turretAtkPower = m_cityTpl->turretAtkPower;
                    }

                    if (!m_cityTpl->traps.empty()) {
                        for (auto& trap :  m_cityTpl->traps) {
                            m_trapSet.AddTrap(trap.type, model::ArmyState::NORMAL,  trap.count);
                        }
                    }
                    m_trapSet.InitProp();

                    if (!m_isOccupy) {
                       
                        if (m_cityTpl->armyGroup > 0 && m_cityTpl->armyCount) {
                            armyGroup = m_cityTpl->armyGroup;
                            armyCount = m_cityTpl->armyCount;
                        }
                        for (int i = 0; i < armyNum; ++i)
                        {
                            ArmyList armyList = tpl::m_tploader->GetRandomArmyList(armyGroup, armyCount);
                            m_armyLists.push_back(armyList);
                            //初始化的时候记录一下NPC总数
                            //armyListsTotal
                            m_armyListsTotal += armyList.ArmyCount(ArmyState::NORMAL) + armyList.ArmyCount(ArmyState::DIE);   
                        }
                        //m_maintainer.ClearByTag(TAG_NPC_RESET); 第一次不需要clear
                        int64_t time = m_armyRecoveryTime - g_dispatcher->GetTimestampCache(); 
                        if (time > 0) {
                            m_maintainer.Add(g_dispatcher->quicktimer().SetTimeoutWithLinker(std::bind(&FamousCity::OnNpcReset, this), time * 1000), TAG_NPC_RESET);   
                        }
                    }

                    if (m_state == model::CastleState::OCCUPY) {
                        int64_t time = m_occupyLeftTime - g_dispatcher->GetTimestampCache(); 
                        if (time > 0) {
                            m_maintainer.Add(g_dispatcher->quicktimer().SetTimeoutWithLinker(std::bind(&FamousCity::OnOccupyEnd, this), time * 1000), TAG_OCCUPY_END); 
                        }      
                    }

                }
                //SetDirty();
            }
        }

  
        int FamousCity::GetDropId()
        {
            return m_cityTpl->dropId;
        }

        int FamousCity::troopId() const
        {
            if (m_troop == nullptr) {
                return 0;
            }
            return m_troop->id();
        }

        int64_t FamousCity::uid() const
        {
            if (m_troop == nullptr) {
                return 0;
            }
            return m_troop->uid();
        }

        int64_t FamousCity::allianceId() const
        {
            return m_allianceId;
        }

        const std::string FamousCity::nickname() const
        {
            if (!m_troop) {
                return "";
            }
            return m_troop->nickname();
        }

        void FamousCity::AddCityDefense(int add)
        {
            m_cityDefense += add;
            if (m_cityDefense >=  m_cityDefenseMax) {
                m_cityDefenseRecover = 0;
                m_cityDefenseRecoverTime = 0;
                m_updateSeconds = 0;
                m_cityDefense = m_cityDefenseMax;
            }

            if (m_cityDefense < 0) {
                m_cityDefense = 0;
            }
            SetDirty();
        }

         void FamousCity::Refresh()
        {
                 Init();
        }
        
        bool FamousCity::CanGarrison(Troop* troop)
        {
            //是否能驻守多只部队
            return true;
        }

        bool FamousCity::CanOccupy(Troop* troop)
        {
            return allianceId() == 0 || allianceId() != troop->allianceId();
        }


        //占领权改变
        void FamousCity::ChangeOccupy(int64_t old_allianceId , int64_t new_allianceId)
        {
            if (old_allianceId != 0) { 
                    auto old_alliance = g_mapMgr->alliance().FindAllianceSimple(old_allianceId);
                    old_alliance->OnAllianceLoseOccupyCity(this, new_allianceId);
                }

                if (new_allianceId != 0) {
                    auto new_alliance = g_mapMgr->alliance().FindAllianceSimple(new_allianceId);
                    new_alliance->OnAllianceOwnOccupyCity(this);
                }
        }

        void FamousCity::ChangeSovereign(int64_t old_allianceId , int64_t new_allianceId)
        {
            if (old_allianceId != 0) { 
                    auto old_alliance = g_mapMgr->alliance().FindAllianceSimple(old_allianceId);
                    old_alliance->OnAllianceLoseCity(this, new_allianceId);
                }

                if (new_allianceId != 0) {
                    auto new_alliance = g_mapMgr->alliance().FindAllianceSimple(new_allianceId);
                    new_alliance->OnAllianceOwnCity(this);
                }
        }

        void FamousCity::OnOccupyEnd()
        {
            std::cout << "FamousCity::OnOccupyEnd" << std::endl;
            if (m_state == model::CastleState::OCCUPY)
            {
                m_maintainer.ClearByTag(TAG_OCCUPY_END);
                m_occupyLeftTime = 0;
                m_state = model::CastleState::SOVEREIGN;

                //迁移原联盟玩家主城
                int wide = m_tpl->range.x;
                int height = m_tpl->range.y;
                g_mapMgr->MoveLoseCastle(m_bornPoint, wide, height, this, m_allianceId);  
                ChangeSovereign(m_allianceId, m_occupyallianceId);
    
                m_allianceId = m_occupyallianceId;
                NoticeUnitUpdate();
                SetDirty();
            }
        }

        void FamousCity::Occupy(Troop* troop)
        {
            if (troop) {
                LOG_DEBUG("FamousCity::Occupy new %d, origin %d, state %d", troop->allianceId(), allianceId(), (int)m_state);
                if (troop->allianceId() !=  allianceId()) {
                    std::list<Troop*> troops = m_troops;
                    ClearAllDefenders();

                    if (m_state == model::CastleState::NORMAL)
                    {
                        //第一次占领直接变成主权
                        SetState(model::CastleState::SOVEREIGN);

                        ChangeSovereign(0, troop->allianceId());
                        m_allianceId = troop->allianceId();
                        m_occupyallianceId = troop->allianceId(); 
                    } else {
                        //重置占领剩余时间，重置定时器, 修改名城状态
                        //m_occupyLeftTime = g_tploader->configure().cityConfig.occupation; //测试阶段为2分钟
                        m_occupyLeftTime = g_dispatcher->GetTimestampCache() + g_tploader->configure().cityConfig.occupation;
                        int64_t time = m_occupyLeftTime - g_dispatcher->GetTimestampCache(); 
                        SetState(model::CastleState::OCCUPY);
                        m_maintainer.ClearByTag(TAG_OCCUPY_END);
                        m_maintainer.Add(g_dispatcher->quicktimer().SetTimeoutWithLinker(std::bind(&FamousCity::OnOccupyEnd, this), time * 1000), TAG_OCCUPY_END);       
                        m_occupyallianceId = troop->allianceId(); 
                        ChangeOccupy(m_allianceId, m_occupyallianceId);
                    }
                }
                
                m_troops.push_back(troop);
                m_troopIds.push_back(troop->id());

                UpdateDefTroop();
                UpdateCityDefRecover();
                m_isOccupy = true;
                SetDirty();
            }
        }

        void FamousCity::Garrison(Troop* troop)
        {
            if (troop) {
                LOG_DEBUG("FamousCity::Garrison ");
                if (troop->allianceId() !=  allianceId()) {
                    LOG_DEBUG("Garrison  alliance not the same");
                    return;
                }

                m_troops.push_back(troop);
                m_troopIds.push_back(troop->id());

                UpdateDefTroop();
                UpdateCityDefRecover();
                SetDirty();
            }
        }

        void FamousCity::UpdateDefTroop()
        {
            
            // int maxPower = 0;
            // for (auto troop :  m_troops) {
            //     if (troop) {
            //         if (troop->armyList() && troop->armyList()->Power() > maxPower) {
            //             maxPower = troop->armyList()->Power();
            //             defTroop = troop;
            //         }
            //     }
            // }
            
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

        bool FamousCity::SwitchTroop()
        { 
            //现在是取出队尾处理
            //1.如果NPC队列不为空则先走NPC队列的
            bool ret = false;
            if (!m_armyLists.empty()) {
                //取出最上层的军队
                m_armyLists.pop_back();
                if (!m_armyLists.empty()) {
                    ret = true;
                    SetDirty();
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

        void FamousCity::OnTroopLeave(Troop* troop)
        {
            if (std::find(m_troops.begin(),  m_troops.end(),  troop) !=  m_troops.end()) {
                m_troops.remove(troop);
                m_troopIds.remove(troop->id());

                if (troop ==  m_troop) {
                    UpdateDefTroop();
                }
                UpdateCityDefRecover();
                NoticeUnitUpdate();
                SetDirty();
            }
        }
        
        void FamousCity::SetTpl(const MapUnitTpl* tpl)
        {
            if (tpl && tpl->ToFamousCity()) {
                Unit::SetTpl(tpl);
            }
        }

        void FamousCity::OnIntervalUpdate(size_t updateSeconds)
        {
//             if (updateSeconds % 60 == 0) {
//                 int min = base::utils::now_minute();
//                 if (min == 0 || min == 30) {
//                     if (!troop()) {
//                         Refresh();
//                         NoticeUnitUpdate();
//                     }
//                 }
//             }

            if (m_cityDefenseRecover > 0) {
                if (m_cityDefenseRecoverTime > 0) {
                    ++m_updateSeconds;

                    if (m_updateSeconds % 60 == 0) {
                        m_cityDefenseRecoverTime += 60;
                        AddCityDefense(m_cityDefenseRecover);
                        NoticeUnitUpdate();
                        SetDirty();
                    }
                }
            }
        }

        void FamousCity::StartRecoverCityDefense()
        {
            int64_t now = g_dispatcher->GetTimestampCache();
            m_cityDefenseRecoverTime = now;
            UpdateCityDefRecover();
            SetDirty();
        }

        void FamousCity::UpdateCityDefRecover()
        {
//             •名城城防值需要通过驻守部队数量来对应恢复，算法公式：恢复的城防值X点/分钟=(1+系数a*兵力总数/（系数b+系数c*兵力总数）)*系数d
// 其中，系数a=5,系数b=3000,系数c=9,系数d=1000

            int totalArmy = 0;
            for (auto troop :  m_troops) {
                if (troop) {
                    totalArmy +=  troop->GetArmyTotal();
                }
            }

            m_cityDefenseRecover = 1.0f * (1 + 5 * totalArmy/(3000 + 9 * totalArmy)) * 1000;
            SetDirty();
        }

        //名城npc重置
        void FamousCity::OnNpcReset() 
        {   
            LOG_DEBUG("npc reset ---------------- 1111");
            if (!m_isOccupy) {
                int armyGroup = 0;
                int armyCount = 0;
                int armyNum = 0;
                if (m_cityTpl->armyGroup > 0 && m_cityTpl->armyCount) {
                    armyGroup = m_cityTpl->armyGroup;
                    armyCount = m_cityTpl->armyCount;
                    armyNum = m_cityTpl->armyNum;
                }
                m_armyLists.clear();
                m_armyListsTotal = 0;
                for (int i = 0; i < armyNum; ++i)
                {
                    ArmyList armyList = tpl::m_tploader->GetRandomArmyList(armyGroup, armyCount);
                    m_armyLists.push_back(armyList);
                    //初始化的时候记录一下NPC总数
                    //armyListsTotal
                    m_armyListsTotal += armyList.ArmyCount(ArmyState::NORMAL) + armyList.ArmyCount(ArmyState::DIE);   
                }            
                m_maintainer.ClearByTag(TAG_NPC_RESET); 
                // m_armyRecoveryTime = g_tploader->configure().cityConfig.armyRecoveryTime + g_dispatcher->GetTimestampCache(); 
                // int64_t time = m_armyRecoveryTime - g_dispatcher->GetTimestampCache();
                // m_maintainer.Add(g_dispatcher->quicktimer().SetTimeoutWithLinker(std::bind(&FamousCity::OnNpcReset, this), time * 1000), TAG_NPC_RESET);  
            }
        }

        void FamousCity::ClearAllDefenders()
        {
            while(!m_troops.empty())
            {
                Troop* tp = m_troops.front();
                tp->GoHome();
            }
        }

        void FamousCity::ClearNpc()
        {       
//             m_trapSet.ClearAll();
//             m_turretAtkPower = 0;
        }

        void FamousCity::HurtNpc()
        {
            if (!m_isOccupy) {
                //npc被打恢复处理
                int64_t time = m_armyRecoveryTime - g_dispatcher->GetTimestampCache(); 
                if (time < 0) {
                    m_armyRecoveryTime = g_dispatcher->GetTimestampCache() + g_tploader->configure().cityConfig.armyRecoveryTime;
                    LOG_DEBUG("-------- %d, -------------%d --------- ", m_armyRecoveryTime, g_tploader->configure().cityConfig.armyRecoveryTime);
                    int64_t leftTime = g_tploader->configure().cityConfig.armyRecoveryTime;
                    m_maintainer.ClearByTag(TAG_NPC_RESET); 
                    m_maintainer.Add(g_dispatcher->quicktimer().SetTimeoutWithLinker(std::bind(&FamousCity::OnNpcReset, this), leftTime * 1000), TAG_NPC_RESET);   
                }
            }
        }

        const int FamousCity::CurrentArmyTotal()
        {
            int armyListsTotal = 0;
            for (std::list<ArmyList>::iterator it = m_armyLists.begin(); it != m_armyLists.end(); ++it)
            {
                armyListsTotal += (*it).ArmyCount(ArmyState::NORMAL);
            }
            return armyListsTotal;
        }

        std::string FamousCity::Serialize()
        {
            std::string jsonString;
            try {
                rapidjson::StringBuffer jsonbuffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                writer.StartObject();

                writer.Key("pos");
                writer.StartArray();
                writer.Int(m_pos.x);
                writer.Int(m_pos.y);
                writer.EndArray();
                
                writer.Key("cityDefense");
                writer.Int(m_cityDefense);

                writer.Key("cityDefenseRecoverTime");
                writer.Int64(m_cityDefenseRecoverTime);

                writer.Key("troopId");
                writer.StartArray();
                for (auto troop :  m_troops) {
                    if (troop) {
                        writer.Int(troop->id());
                    }
                }
                writer.EndArray();

                writer.Key("isOccupy");
                writer.Bool(m_isOccupy);

                writer.Key("allianceId");
                writer.Int64(m_allianceId);

                writer.Key("state");
                writer.Int((int)m_state);

                writer.Key("occupyLeftTime");
                writer.Int64(m_occupyLeftTime);

                writer.Key("armyRecoveryTime");
                writer.Int64(m_armyRecoveryTime);

                writer.Key("occupyallianceId");
                writer.Int64(m_occupyallianceId);

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
                LOG_ERROR("save FamousCity json fail: %s\n", ex.what());
            }
            return jsonString;
        }

        bool FamousCity::Deserialize(std::string data)
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
                    
                    m_cityDefense = doc["cityDefense"].GetInt();
                    m_cityDefenseRecoverTime = doc["cityDefenseRecoverTime"].GetInt64();

                    if (doc["troopId"].IsArray()) {
                        auto& temp = doc["troopId"];

                        for (int i = 0; i < (int)temp.Size(); ++i) {
                            int troopId = temp[i].GetInt();
                            m_troopIds.push_back(troopId);
                        }
                    }

                    m_isOccupy = doc["isOccupy"].GetBool();
                    m_allianceId = doc.HasMember("allianceId")  ? doc["allianceId"].GetInt64() : 0;
                    
                    m_occupyallianceId = doc.HasMember("occupyallianceId") ?  doc["occupyallianceId"].GetInt64() : 0;

                    auto& armyLists = doc["armyLists"];
                    for (size_t i = 0; i < armyLists.Size(); ++i) {
                        ArmyList armyList;
                        armyList.Desrialize(armyLists[i]);
                        m_armyLists.push_back(armyList);
                    }
                    m_armyListsTotal = doc["armyListsTotal"].GetInt64();
                    //m_occupyLeftTime = (int64_t) (doc.HasMember("occupyLeftTime") ? doc["occupyLeftTime"].GetInt() : 0);
                    //LOG_DEBUG("----------------------1111");
                    m_occupyLeftTime = doc.HasMember("occupyLeftTime") ? doc["occupyLeftTime"].GetInt64() : 0;
                    m_state = doc.HasMember("state") ?  static_cast<model::CastleState>(doc["state"].GetInt()) : model::CastleState::NORMAL;
                    
                    m_armyRecoveryTime = doc.HasMember("m_armyRecoveryTime") ? doc["m_armyRecoveryTime"].GetInt64() : 0;


                }
                return true;
            } catch (std::exception& ex) {
                LOG_ERROR("json string to FamousCity fail: %s\n", ex.what());
                return false;
            }
        }
        
        void FamousCity::FinishDeserialize()
        {
            Init();
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

            // 恢复城防值
            if (m_cityDefenseRecoverTime > 0) {
                int64_t now = g_dispatcher->GetTimestampCache();
                int intervalSecond = now - m_cityDefenseRecoverTime;
                if (intervalSecond > 0) {
                    UpdateCityDefRecover();
                    AddCityDefense(intervalSecond/ 60 * m_cityDefenseRecover);
                    if (m_cityDefenseRecoverTime > 0) {
                        m_updateSeconds = intervalSecond % 60;
                        m_cityDefenseRecoverTime = now - m_updateSeconds;
                    }
                }
            }
        }

    }
}
