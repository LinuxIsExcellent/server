#include "troop.h"
#include "agent.h"
#include "map.h"
#include "unit/unit.h"
#include "unit/castle.h"
#include "unit/monster.h"
#include "unit/camp.h"
#include "unit/resnode.h"
#include "unit/famouscity.h"
#include "unit/catapult.h"
#include "tpl/maptrooptpl.h"
#include "qo/dataservice.h"
#include "battle/battlemgr.h"
#include <base/event/dispatcher.h>
#include <base/logger.h>
#include <base/3rd/rapidjson/document.h>
#include <base/3rd/rapidjson/writer.h>
#include <base/3rd/rapidjson/stringbuffer.h>
#include <model/tpl/drop.h>
#include <model/tpl/templateloader.h>
#include <model/tpl/item.h>
#include <model/tpl/army.h>
#include <model/tpl/configure.h>


namespace ms
{
    namespace map
    {
        using namespace std;
        using namespace base;
        using namespace model;
        using namespace model::tpl;
        using namespace base::event;
        using namespace battle;

        TroopFactory& TroopFactory::instance()
        {
            static TroopFactory troopFactory;
            return troopFactory;
        }

        Troop* TroopFactory::CreateTroop(int id, const std::string& data)
        {
            Troop* troop = new Troop(id);
            try {
                rapidjson::Document doc;
                doc.Parse<0>(data.c_str());
                if (doc.IsObject()) {
                    if (troop) {
                        troop->Deserialize(doc);
                    }
                }
            } catch (std::exception& ex) {
                LOG_ERROR("json string to Troop fail: %s\n", ex.what());
                return nullptr;
            }
            return troop;
        }

        Troop* TroopFactory::CreateTroop(int id, Agent* agent, model::MapTroopType type, const Point& from, const Point& to, int teamId)
        {
            Troop* troop = new Troop(id, agent, type, from, to, 0.0f, teamId);
            return troop;
        }

        /*****Troop*****/
        Troop::Troop(int id)
            : m_id(id), m_parentSource(-1, -1),m_defaultTroopImpl(this), m_troopToRes(this), m_troopToMonster(this), m_troopToCity(this), m_troopToCastle(this), m_troopToCampTemp(this),
              m_troopToCampFixed(this), m_troopToScout(this), m_troopToReinforcements(this), m_troopToCatapult(this), m_troopToWolrdBoss(this), m_troopToTransport(this)
        {
        }

        Troop::Troop(int id, Agent* agent, MapTroopType type, const Point& from, const Point& to, float speed, int teamId)
            : m_id(id), m_agent(agent), m_type(type), m_from(from), m_to(to), m_parentSource(-1, -1), m_source(from), m_speed(speed), m_teamId(teamId),
              m_defaultTroopImpl(this), m_troopToRes(this), m_troopToMonster(this), m_troopToCity(this), m_troopToCastle(this), m_troopToCampTemp(this),
              m_troopToCampFixed(this), m_troopToScout(this), m_troopToReinforcements(this), m_troopToCatapult(this), m_troopToWolrdBoss(this), m_troopToTransport(this)
        {
            m_currentPos = from;
            m_createTime = g_dispatcher->GetTimestampCache();

            SwitchTroopImpl();
        }

        Troop::~Troop()
        {
            SAFE_RELEASE(m_linker);
        }

        int64_t Troop::uid() const
        {
            return m_agent->uid();
        }

        const string& Troop::nickname() const
        {
            return m_agent->nickname();
        }

        int64_t Troop::headId() const
        {
            return m_agent->headId();
        }

        int Troop::lordLevel() const
        {
            return m_agent->level();
        }

        int64_t Troop::allianceId() const
        {
            return m_agent->allianceId();
        }

        int Troop::allianceLv() const
        {
            return 1;
        }

        const string& Troop::allianceNickname() const
        {
            return m_agent->allianceNickname();
        }

        const string& Troop::allianceName() const
        {
            return m_agent->allianceName();
        }

        int Troop::allianceBannerId() const
        {
            return m_agent->allianceBannerId();
        }

        ArmyList* Troop::armyList()
        {
            return m_agent->TeamArmyList(m_teamId);
        }

        int Troop::GetArmyTotal()
        {
            int total = 0;
            if (armyList()) {
                total = armyList()->GetArmyTotal();
            }
            return total;
        }

        int Troop::GetLoadTotal()
        {
            int total = 0;
            if (armyList()) {
                total = armyList()->GetLoadTotal();
            }
            return total;
        }

        void Troop::Debug()
        {
            cout << "food = " << m_food << " wood = " << m_wood << " iron = " << m_iron << " stone = " << m_stone << endl;
        }

        int Troop::power()
        {
            // 目前用兵力的20倍来显示   TODO by zds
            int power = 0;
            for (auto it = armyList()->armies().begin(); it != armyList()->armies().end(); ++it) {
                const ArmyGroup& armyGroup = it->second;
                power = power + armyGroup.armyInfo().count(ArmyState::NORMAL) * 20;
            }
            return power;
        }

        void Troop::AddResource(ResourceType type, int count)
        {
            if (count <= 0) {
                return;
            }
            if (type == ResourceType::FOOD) {
                m_food += count;
            } else if (type == ResourceType::WOOD) {
                m_wood += count;
            } else if (type == ResourceType::IRON) {
                m_iron += count;
            } else if (type == ResourceType::STONE) {
                m_stone += count;
            } else if (type == ResourceType::GOLD) {
                m_gold += count;
            }
            SetDirty();
            //Debug();
        }

        void Troop::AddDropItem(int tplid, int count)
        {
            if (const ItemTpl* tpl = g_tploader->FindItem(tplid)) {
                m_dropItems.emplace_back(*tpl, count);
            }
            SetDirty();
        }

        void Troop::AddDropItem(vector< DropItem >& drops)
        {
            MergeDrops(m_dropItems, drops);
            SetDirty();
        }

        int Troop::DropItemCount()
        {
            int count = 0;
            for (size_t i = 0; i < m_dropItems.size(); ++i) {
                const model::tpl::DropItem& drop = m_dropItems[i];
                count += drop.count;
            }
            return count;
        }

        void Troop::ClearDropItem()
        {
            m_dropItems.clear();
        }

        void Troop::ResetLinker()
        {
            //cout << "Troop::ResetLinker state = " << (int)m_state << " leftSecond/totalSecond = " << leftSecond() << "/" << totalSecond() << endl;
            SAFE_RELEASE(m_linker);
            switch (m_state) {
                case model::MapTroopState::MARCH: {
                    m_linker = g_dispatcher->quicktimer().SetTimeoutWithLinker(std::bind(&Troop::OnReach, this), leftSecond() * 1000);
                }
                break;
                case model::MapTroopState::REACH: {
                    if (m_type == model::MapTroopType::GATHER) {
                        m_linker = g_dispatcher->quicktimer().SetTimeoutWithLinker(std::bind(&Troop::GoHome, this), leftSecond() * 1000);
                    }
                }
                break;
                case model::MapTroopState::GO_HOME: {
                    m_linker = g_dispatcher->quicktimer().SetTimeoutWithLinker(std::bind(&Troop::OnArriveHome, this), leftSecond() * 1000);
                }
                break;
                default:
                    LOG_ERROR("Troop::ResetLinker() STATE = %d ERROR", (int)m_state);
                    break;
            }
            m_linker->Retain();
        }

        void Troop::March()
        {
            // cout << "Troop::March()" << endl;
            Unit* fromUnit = g_mapMgr->FindUnit(m_from);
            if (fromUnit) {
                fromUnit->OnTroopLeave(this);
            }

            Unit* toUnit = g_mapMgr->FindUnit(m_to);

            // 9.       行营及行营出征的部队要有绑定关系，
            // 当玩家派遣部队从A行营到B行营（B行营空闲）时，
            // 此时该部队解除与A行营的绑定关系，与B行营绑定，
            // B行营无法再派遣其他部队进驻，A行营则可以派遣其他部队，
            // 如此时玩家召回A行营去B行营的部队判定A行营有无和其他部队绑定，
            // 有则部队直接回城，没有则回A行营）
            if (m_type ==  model::MapTroopType::CAMP_FIXED_OCCUPY || m_type ==  model::MapTroopType::CAMP_FIXED || m_type ==  model::MapTroopType::CAMP_TEMP  
                || m_type ==  model::MapTroopType::SIEGE_CITY) {
                if (toUnit) {
                    if (auto toCamp = toUnit->ToCampFixed()) {
                        if (toCamp->uid() ==  uid()) {
                            toCamp->SetBindTroopId(id());
                            toCamp->NoticeUnitUpdate();
                        }
                    }
                }

                if (fromUnit) {
                    if (auto fromCamp = fromUnit->ToCampFixed()) {
                        if (fromCamp->uid() ==  uid()) {
                            fromCamp->SetBindTroopId(0);
                            fromCamp->NoticeUnitUpdate();
                        }
                    }
                }
            }

//             // test
//             if (m_type ==  model::MapTroopType::SIEGE_CITY) {
//                 if (toUnit) {
//                     if (auto city = toUnit->ToFamousCity()) {
//                         if (allianceId() > 0 && city->allianceId() ==  allianceId()) {
//                             m_type =  model::MapTroopType::PATROL_CITY;
//                         }
//                     }
//                 }
//             }


            int time = CalculateMarchTime(m_from, m_to);
            if (toUnit) {
                if (Castle* castle = toUnit->ToCastle()) {
                    for (auto it = castle->agent().skillBuffs().begin(); it != castle->agent().skillBuffs().end(); ++it) {
//                         const info::BuffInfo& buff = it->second;
//                         if (it->first == (int)model::BuffType::ENEMY_TROOP_TIME_INCR_PER && buff.endTimestamp > g_dispatcher->GetTimestampCache()) {
//                             time *= buff.param1;
//                         }
                    }
                }
            }

            //中立城池 衰减
            if (m_agent->uid() == 0) {
                time = ceil(time * g_tploader->configure().neutralCastle.attackPlayerTimeAttenuation);
            }
            m_state = MapTroopState::MARCH;

            int64_t now = g_dispatcher->GetTimestampCache();
            m_beginTimestamp = now;
            m_currentPos = m_from;
            m_endTimestamp = now + time;

            ResetLinker();

            SetDirty();

            if (toUnit) {
                toUnit->OnTroopMarch(this);
            }
        }

        model::MarchErrorNo Troop::ReMarch(MapTroopType type, const Point& to)
        {
            model::MarchErrorNo errorNo = MarchErrorNo::SUCCESS;
            //cout << "Troop::ReMarch" << endl;
            if (!IsReach()) {
                errorNo = MarchErrorNo::OTHER;
                return errorNo;
            }

            if (m_type ==  model::MapTroopType::SIEGE_CITY && to ==  m_to && type ==  model::MapTroopType::PATROL_CITY) {
                // 巡逻
                auto toUnit = g_mapMgr->FindUnit(to);
                m_type = model::MapTroopType::PATROL_CITY;
                BattleMgr::instance().PatrolCity(this, toUnit->ToFamousCity());
                if (armyList() && armyList()->GetArmyTotal() > 0) {
                    m_type = model::MapTroopType::SIEGE_CITY;
                } else {
                    GoHome();
                }
                return errorNo;
            }

            if (m_type != model::MapTroopType::CAMP_TEMP && m_type != model::MapTroopType::CAMP_FIXED && m_type != model::MapTroopType::ASSIGN) {
                errorNo = MarchErrorNo::OTHER;
                return errorNo;
            }

            errorNo = g_mapMgr->CanMarch(type, m_to, to, *m_agent);
            if (errorNo != MarchErrorNo::SUCCESS) {
                return errorNo;
            }

            OnLeave();

            //对type做过滤 比如不能为交易和侦查等
            m_from = move(m_to);
            m_to = move(to);

            if (m_type == model::MapTroopType::CAMP_FIXED || m_type == model::MapTroopType::ASSIGN ) {
                m_parentType = m_type;
                m_parentSource = m_source;
                m_source = m_from;
            }

            SetType(type);

            March();
            NoticeTroopUpdate(true);

            // OnMarch
            agent().OnMarch(this,  true);
            SetDirty();

            return errorNo;
        }

        bool Troop::GoHome()
        {
            //TODO 与高级召回 和 立即到家 区分
            //cout << "Troop::GoHome() troopId = " << m_id << endl;
            bool resetCross = false;
            if (m_type == MapTroopType::GATHER || m_type  ==  MapTroopType::SIEGE_CITY || m_type == MapTroopType::CAMP_TEMP || m_type == MapTroopType::CAMP_FIXED) {
                resetCross = true;
            }

          /*  if (parentType() ==  MapTroopType::CAMP_FIXED) {
                do {
                    Unit* fromUnit = g_mapMgr->FindUnit(m_from);
                    if (fromUnit) {
                        if (auto fromCamp = fromUnit->ToCampFixed()) {
                            if (fromCamp->uid() ==  uid() && fromCamp->bindTroopId() == id()) {
                                break;
                            }
                        }
                    }
                    // ResetParent();
                } while (0);
            }*/

            OnBack();
            OnLeave();

            int64_t now = g_dispatcher->GetTimestampCache();
            m_state = MapTroopState::GO_HOME;
            m_from = m_currentPos.toPoint();
            // cout << "###m_currentPos.toPoint"  << " x: " << m_currentPos.toPoint().x << "y: " << m_currentPos.toPoint().y << endl;

            // cout << "###m_from"  << " x: " << m_from.x << "y: " << m_from.y << endl;

            
            // cout << "###m_source"  << " x: " << m_source.x << "y: " << m_source.y << endl;

            m_to = m_source;
            m_beginTimestamp = now;
            m_endTimestamp = now + CalculateMarchTime(m_from, m_to);

            ResetLinker();
            NoticeTroopUpdate(resetCross);

            SetDirty();
            return true;
        }

        void Troop::ImmediateReturn()
        {
            // 自己部队全部返回，运输失败
            if (type() == MapTroopType::TRANSPORT)
            {
                OnMarchBack();
            }
            //cout << "Troop::ImmediateReturn" << endl;
            OnBack();
            OnLeave();

            int64_t now = g_dispatcher->GetTimestampCache();
            m_state = MapTroopState::GO_HOME;
            m_from = m_currentPos.toPoint();
            m_to = m_source;
            m_beginTimestamp = now;
            m_endTimestamp = now;

            SAFE_RELEASE(m_linker);
            OnArriveHome();
            SetDirty();
        }

        bool Troop::Recall()
        {
            //cout << "Troop::Recall" << endl;
            if (!IsMarching()) {
                return false;
            }

//             if (parentType() ==  MapTroopType::CAMP_FIXED) {
//                 do {
//                     Unit* fromUnit = g_mapMgr->FindUnit(m_from);
//                     if (fromUnit) {
//                         if (auto fromCamp = fromUnit->ToCampFixed()) {
//                             if (fromCamp->uid() ==  uid() && fromCamp->bindTroopId() == id()) {
//                                 break;
//                             }
//                         }
//                     }
//                     ResetParent();
//                 } while (0);
//             }

            OnBack();
            OnLeave();

            int64_t now = g_dispatcher->GetTimestampCache();
            if (m_from == m_source) {
                m_from = m_to;
            } else {
                m_from = m_currentPos.toPoint();
            }
            m_to = m_source;
            m_state = MapTroopState::GO_HOME;
            m_beginTimestamp = now * 2 - m_endTimestamp;    //= now - leftTime = now - (m_endTimestamp - now)
            m_endTimestamp = now + CalculateMarchTime(m_currentPos.toPoint(), m_to);

            NoticeTroopUpdate();
            ResetLinker();
            SetDirty();
            return true;
        }

        bool Troop::AdvancedRecall()
        {
            //cout << "Troop::AdvancedRecall" << endl;
            if (!IsMarching()) {
                return false;
            }

            Recall();
            SetDirty();
            return true;
        }

        bool Troop::SpeedUp(float speedUp)
        {
            //cout << "Troop::SpeedUp = " << speedUp << endl;
            if (IsMarching() || IsGoHome()) {
                /*int64_t now = g_dispatcher->GetTimestampCache();
                int leftNew = leftSecond() * (1 - speedUp);
                cout << "leftNew = " << leftNew << endl;
                m_endTimestamp = now + leftNew;
                g_mapMgr->BroadMapTroopUpdate(this);
                ResetLinker();*/
                //m_speed *= (1.0f / (1.0f - speedUp));
                m_speedUp += speedUp;
                ReCalculateMarch(speedUp);
                return true;
            }
            return false;
        }

        bool Troop::Repatriate()
        {
            if (m_type == MapTroopType::SIEGE_CITY) {
                if (IsReach()) {
                    return GoHome();
                }
            }

//             if (m_type == MapTroopType::REINFORCEMENTS) {
//                 if (IsMarching()) {
//                     return Recall();
//                 } else if (IsReach()) {
//                     return GoHome();
//                 }
//             }
            return false;
        }

        void Troop::FinishDeserialize()
        {
            m_troopImpl->FinishDeserialize();
        }

        //攻击失败
        void Troop::OnAttackFailed(Unit* unit)
        {
            m_troopImpl->OnAttackFailed(unit);
        }

        //攻击成功
        void Troop::OnAttackWin(Unit* unit)
        {
            m_troopImpl->OnAttackWin(unit);
        }
        //防守失败
        void Troop::OnDefenceFailed(Unit* unit)
        {
            m_troopImpl->OnDefenceFailed(unit);
        }
        //防守成功
        void Troop::OnDefenceWin(Unit* unit)
        {
            m_troopImpl->OnDefenceWin(unit);
        }


        void Troop::OnReach()
        {
            //cout << "Troop::OnReach() type = " << (int)m_type << endl;
            bool resetCross = false;
            if (m_type == MapTroopType::GATHER || m_type  ==  MapTroopType::SIEGE_CITY  || m_type == MapTroopType::CAMP_TEMP 
                || m_type == MapTroopType::CAMP_FIXED || m_type == MapTroopType::SIEGE_CATAPULT || m_type == MapTroopType::WORLDBOSS ) {
                resetCross = true;
            }
            m_speed = 0.0f;
            m_initialSpeed = 0.0f;
            m_speedUp = 0.0f;
            m_state = model::MapTroopState::REACH;
            m_currentPos = m_to;
            Unit* to = g_mapMgr->FindUnit(m_to);
            if (!to)
            {
                // 如果行军类型是这些，但是单位已不在这里，行军直接返回
                // 采集, 打怪, 攻击玩家城池, 侦查, 攻击驻扎, 攻击行营, 占领行营(驻守行营), 拆除行营, 攻击世界BOSS, 
                if (m_type == MapTroopType::GATHER || m_type  ==  MapTroopType::MONSTER  || m_type == MapTroopType::SIEGE_CASTLE 
                    || m_type == MapTroopType::SCOUT || m_type == MapTroopType::CAMP_TEMP_ATTACK 
                    || m_type == MapTroopType::CAMP_FIXED_ATTACK  || m_type == MapTroopType::CAMP_FIXED_OCCUPY || m_type == MapTroopType::CAMP_FIXED_DISMANTLE || m_type == MapTroopType::WORLDBOSS || m_type == MapTroopType::ASSIGN) {
                    GoHome();
                    return;
                }
            }
            if (m_agent->isPlayer()) {
                NoticeTroopUpdate(resetCross);

                if (to) {
                    to->OnTroopReach(this);
                }
                bool isInValid = OnReachProc(to);
                if (isInValid) {
                    OnReachInvalid();
                    GoHome();
                }
            } else {
                bool isInvalid = true;
                switch (m_type) {
                    // 运输失败，目标丢失
                    case MapTroopType::TRANSPORT: {
                        OnMarchBack();
                    }
                    break;
                    default:
                        break;
                }
                NoticeTroopUpdate();
                if (isInvalid) {
                    GoHome();
                }
            }
            SetDirty();
        }

        bool Troop::OnReachProc(Unit* to)
        {
            if (m_agent) {
                auto armyList = m_agent->TeamArmyList(m_teamId);
                if (armyList) {
                    armyList->OnPropertyUpdate(m_agent->property());
                }

                if (to) {
                    auto defTroop = to->troop();
                    if (defTroop && defTroop->armyList()) {
                        defTroop->armyList()->OnPropertyUpdate(defTroop->agent().property());
                    }
                }
            }

            return m_troopImpl->OnReachProc(to);
        }

        void Troop::OnLeave()
        {
            m_troopImpl->OnLeave();
        }

        void Troop::OnArriveHome()
        {
            m_troopImpl->OnArriveHome();
        }

        void Troop::OnBack()
        {
            m_troopImpl->OnBack();
        }

        //到达失效
        void Troop::OnReachInvalid()
        {
            m_troopImpl->OnReachInvalid();
        }

        // 到达返回(目标失去，自己迁城)
        void Troop::OnMarchBack()
        {   
            m_troopImpl->OnMarchBack();
        }

        void Troop::Remove()
        {
            //cout << "Troop::Remove" << endl;
            if (m_state == MapTroopState::REMOVE) {
                return;
            }
//             OnLeave();
            m_state = MapTroopState::REMOVE;
            m_removeTime = g_dispatcher->GetTimestampCache();
            m_agent->RemoveTroop(m_id);
            g_mapMgr->OnTroopRemove(this);
            SAFE_RELEASE(m_linker);

            if (uid() > 0) {
                SetClean();
                qo::g_dataService->AppendData(qo::g_dataService->CreateDbInfo(this));
            }
        }

        void Troop::OnBattleEnd()
        {
            if (armyList()) {
                //战斗后清空击杀和死亡记录
                for (auto it = armyList()->armies().begin(); it != armyList()->armies().end(); ++it) {
                    ArmyInfo& info = it->second.armyInfo();
                    for (auto it2 = info.states().begin(); it2 != info.states().end(); ++it2) {
                        ArmyState state = (ArmyState)it2->first;
                        if (state == model::ArmyState::DIE || state == model::ArmyState::KILL) {
                            it2->second = 0;
                        }
                    }
                }
            }
            if (uid() > 0) {
                SetClean();
                qo::g_dataService->AppendData(qo::g_dataService->CreateDbInfo(this));
            }
        }

        void Troop::NoticeTroopUpdate(bool resetCross)
        {
            if (!IsRemove()) {
                g_mapMgr->OnTroopUpdate(this, resetCross);
            }
        }

        void Troop::OnPropertyChanged()
        {
            if (IsReach()) {
                if (m_agent) {
                    auto armyList = m_agent->TeamArmyList(m_teamId);
                    if (armyList) {
                        armyList->OnPropertyUpdate(m_agent->property());
                    }
                }

                if (m_type == MapTroopType::GATHER) {
                    if (Unit* unit = g_mapMgr->FindUnit(m_to)) {
                        if (ResNode* res = unit->ToResNode()) {
                            float oldSpeed = gatherSpeed();
                            int leftGather = leftSecond() * oldSpeed;
                            if (InitGather(res)) {
                                int64_t now = g_dispatcher->GetTimestampCache();
                                int time = leftGather / gatherSpeed();
                                m_endTimestamp = now + time;

                                ResetLinker();
                                NoticeTroopUpdate();
                                res->NoticeUnitUpdate();
                                SetDirty();
                            }
                        }
                    }
                }
            }
        }

        bool Troop::Gather(ResNode* res)
        {
            if (res) {
                res->Occupy(this);
                SetReachTplId(res->tpl().id);

                if (InitGather(res)) {
                    int64_t now = g_dispatcher->GetTimestampCache();
                    SetBeginTimestamp(now);
                    int time = res->CalculateGatherTime();
                    SetEndTimestamp(now + time);

                    ResetLinker();
                    NoticeTroopUpdate(true);
                    SetDirty();
                }
                return true;
            }
            return false;
        }

        bool Troop::InitGather(ResNode* res)
        {
            if (res) {
                int oldLoad = gatherLoad();
                float oldSpeed = gatherSpeed();

                int loadSum = GetLoadTotal();
                auto& property = agent().property();
                SetGatherLoad(loadSum * (1 + property.troopLoadPct) + property.troopLoadExt);

                SetInitialSpeed(res->gatherSpeed());
                SetSpeedUp(GetGatherSpeedAddition(res->type()));
                SetGatherSpeed(initialSpeed() * (1 + speedUp()));
                SetSpeed(gatherSpeed());

                if (oldLoad !=  gatherLoad() || oldSpeed !=  gatherSpeed()) {
                    return true;
                }
            }
            return false;
        }

        int Troop::CalculateMarchTime(const Point& start, const Point& end)
        {
            m_initialSpeed = 0.0f;
            m_speedUp = 0.0f;
            float time = 0;
            if (const MapTroopTpl* tpl = map::tpl::m_tploader->FindMapTroop(m_type)) {
                m_speed = 0;
                //1 先获取兵种速度
                float baseSpeed = GetBaseSpeed();
                //cout << "baseSpeed = " << baseSpeed << " speedAddition = " << GetSpeedAddition() << endl;
                baseSpeed *= (1 + GetSpeedAddition());
                float dis = g_mapMgr->get_distance(start, end);
                //cout << dis << " " << tpl->unitTime << " " << baseSpeed << " " << tpl->speedAddition << " " << tpl->speedAlter << endl;
                time += dis * tpl->unitTime / (1.0 + baseSpeed * tpl->speedAddition * tpl->speedAlter);
                m_speed = dis / time;
                m_initialSpeed = m_speed;
            }

            // cout << "startX = " << start.x << " startY = " << start.y << " endX = " << end.x << " endY = " << end.y << " time = " << time << " speed = " << m_speed << endl;

            return ceil(time);
        }

        void Troop::ReCalculateMarch()
        {
            if ((!IsMarching() && !IsGoHome()) || m_speed == 0) {
                return;
            }

            //根据当前速度和坐标  重新算时间 重置结束时间戳 定时器重新投放
            int time = 0;
            int64_t now = g_dispatcher->GetTimestampCache();
            float dis = g_mapMgr->get_distance(m_currentPos, m_to);
            time = dis / m_speed;
            m_endTimestamp = now + time;
            SetDirty();

            ResetLinker();

            NoticeTroopUpdate();
        }

        void Troop::ReCalculateMarch(float speedUp)
        {
            if ((!IsMarching() && !IsGoHome()) || m_speed == 0) {
                return;
            }

            const MapTroopTpl* tpl = map::tpl::m_tploader->FindMapTroop(m_type);
            if (tpl !=  nullptr) {
                //通过speedUp直接计算时间 然后求速度
                m_speed = .0f;
                int64_t now = g_dispatcher->GetTimestampCache();
                int left = leftSecond();
                left *= (1 - speedUp);
                m_endTimestamp = now + left;
                float dis  = g_mapMgr->get_distance(m_currentPos, m_to);
                m_speed = dis / left;
                SetDirty();
                //cout << "speed = " << m_speed << " leftSecond = " << left << endl;

                ResetLinker();
                NoticeTroopUpdate();
            }
        }

        void Troop::OnUpdate(int64_t tick, int32_t span)
        {
            //recalculate current pos
            if (IsMarching() || IsGoHome()) {
                float dis = g_mapMgr->get_distance(m_from, m_to);
                float disX = m_to.x - m_from.x;
                float disY = m_to.y - m_from.y;

                float speedX = disX / dis * m_speed;
                float speedY = disY / dis * m_speed;
                //cout << "dis = " << dis << " disX = " << disX << " disY = " << disY << endl;

                m_currentPos.x += speedX * span / 1000;
                m_currentPos.y += speedY * span / 1000;
                SetDirty();
            }
        }

        float Troop::GetBaseSpeed()
        {
            float speed = 0.0f;

            if (armyList()) {
                speed = armyList()->GetBaseSpeed();
            }

            if (speed == 0.0f && (m_type == MapTroopType::SCOUT || m_type == MapTroopType::TRANSPORT)) {
                speed = 1;
                const MapTroopTpl* tpl = map::tpl::m_tploader->FindMapTroop(m_type);
                if (tpl !=  nullptr) {
                    speed = tpl->baseSpeed;
                }
            }
            return speed;
        }

        float Troop::GetSpeedAddition()
        {
            const info::Property& p = m_agent->property();
            float speedAddition = p.marchSpeedPct;
            switch (m_type) {
                case MapTroopType::MONSTER: {
                    speedAddition += p.attackMonsterSpeedPct;
                }
                break;
                case MapTroopType::SCOUT: {
                    speedAddition += p.scoutSpeedPct;
                }
                break;
                case MapTroopType::TRANSPORT: {
                    speedAddition += p.transportSpeedUpPct;
                }
                break;
                case MapTroopType::CAMP_TEMP: {
                    speedAddition += p.camptempSpeedPct;
                }
                break;
                case MapTroopType::GATHER: {
                    speedAddition += p.gatherSpeedPct;
                }
                break;
                case MapTroopType::SIEGE_CASTLE: {
                    speedAddition += p.attackPlayerSpeedPct;
                }
                break;
                case MapTroopType::REINFORCEMENTS: {
                    speedAddition += p.reinforcementsSpeedPct;
                }
                break;
                default:
                    break;
            }
            return speedAddition;
        }

        float Troop::GetGatherSpeedAddition(MapUnitType type)
        {
            float gsa = .0f;
            if (type == model::MapUnitType::FARM_FOOD) {
                gsa = agent().property().foodGatherSpeedPct;
            } else if (type == model::MapUnitType::FARM_WOOD) {
                gsa = agent().property().woodGatherSpeedPct;
            } else if (type == model::MapUnitType::MINE_IRON) {
                gsa = agent().property().ironGatherSpeedPct;
            } else if (type == model::MapUnitType::MINE_STONE) {
                gsa = agent().property().stoneGatherSpeedPct;
            }
            return gsa;
        }

        std::string Troop::Serialize() const
        {
            std::string jsonString;
            try {
                rapidjson::StringBuffer jsonbuffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                writer.StartObject();

                writer.String("uid");
                writer.Int64(m_agent->uid());
                writer.String("type");
                writer.Int((int)m_type);
                writer.String("state");
                writer.Int((int)m_state);

                writer.Key("parentType");
                writer.Int((int)m_parentType);
                writer.Key("parentSource");
                writer.StartArray();
                writer.Int(m_parentSource.x);
                writer.Int(m_parentSource.y);
                writer.EndArray();

                writer.String("from");
                writer.StartArray();
                writer.Int(m_from.x);
                writer.Int(m_from.y);
                writer.EndArray();
                writer.String("to");
                writer.StartArray();
                writer.Int(m_to.x);
                writer.Int(m_to.y);
                writer.EndArray();
                writer.String("source");
                writer.StartArray();
                writer.Int(m_source.x);
                writer.Int(m_source.y);
                writer.EndArray();

                writer.String("speed");
                writer.Double((double)m_speed);
                writer.String("currentPos");
                writer.StartArray();
                writer.Double((double)m_currentPos.x);
                writer.Double((double)m_currentPos.y);
                writer.EndArray();

                writer.String("saveTimestamp"); // temp
                writer.Int64(g_dispatcher->GetTimestampCache());
                writer.String("beginTimestamp");
                writer.Int64(m_beginTimestamp);
                writer.String("endTimestamp");
                writer.Int64(m_endTimestamp);

                writer.String("reachTplId");
                writer.Int(m_reachTplId);
                writer.String("gold");
                writer.Int(m_gold);
                writer.String("food");
                writer.Int(m_food);
                writer.String("wood");
                writer.Int(m_wood);
                writer.String("iron");
                writer.Int(m_iron);
                writer.String("stone");
                writer.Int(m_stone);

                writer.String("teamId");
                writer.Int(m_teamId);

                writer.String("dropItems");
                writer.StartArray();
                for (size_t i = 0; i < m_dropItems.size(); ++i) {
                    const model::tpl::DropItem& drop = m_dropItems[i];
                    writer.StartArray();
                    writer.Int(drop.tpl.id);
                    writer.Int(drop.count);
                    writer.EndArray();
                }
                writer.EndArray();

                writer.String("gatherSpeed");
                writer.Double(m_gatherSpeed);
                writer.String("gatherLoad");
                writer.Int(m_gatherLoad);

                writer.String("speedUp");
                writer.Double((double)m_speedUp);

                writer.String("initialSpeed");
                writer.Double((double)m_initialSpeed);

                writer.String("transportUid");
                writer.Int64(m_transportAgent? m_transportAgent->uid() : 0);

                writer.String("transportId1");
                writer.Int64(m_transportId1);

                writer.String("transportId2");
                writer.Int64(m_transportId2);

                writer.EndObject();
                jsonString = jsonbuffer.GetString();
            } catch (std::exception& ex) {
                LOG_ERROR("save Troop json fail: %s\n", ex.what());
            }
            return jsonString;
        }

        bool Troop::Deserialize(rapidjson::Document& doc)
        {
            if (doc.IsObject()) {
                int64_t uid = doc["uid"].GetInt64();
                m_agent = g_mapMgr->FindAgentByUID(uid);
                if (!m_agent) {
                    LOG_ERROR("Troop::Deserialize can not find agent uid = %d", uid);
                }
                m_type = static_cast<model::MapTroopType>(doc["type"].GetInt());
                m_state = static_cast<model::MapTroopState>(doc["state"].GetInt());

                m_parentType =  static_cast<model::MapTroopType>(doc["parentType"].GetInt());
                if (doc["parentSource"].IsArray()) {
                    auto& temp = doc["parentSource"];
                    m_parentSource.x = temp[0u].GetInt();
                    m_parentSource.y = temp[1].GetInt();
                }

                if (doc["from"].IsArray()) {
                    auto& temp = doc["from"];
                    m_from.x = temp[0u].GetInt();
                    m_from.y = temp[1].GetInt();
                }
                if (doc["to"].IsArray()) {
                    auto& temp = doc["to"];
                    m_to.x = temp[0u].GetInt();
                    m_to.y = temp[1].GetInt();
                }
                if (doc["source"].IsArray()) {
                    auto& temp = doc["source"];
                    m_source.x = temp[0u].GetInt();
                    m_source.y = temp[1].GetInt();
                }

                m_speed = doc["speed"].GetDouble();
                if (doc["currentPos"].IsArray()) {
                    auto& temp = doc["currentPos"];
                    m_currentPos.x = temp[0u].GetDouble();
                    m_currentPos.y = temp[1].GetDouble();
                }

                int64_t saveTimestamp = doc["saveTimestamp"].GetInt64();
                saveTimestamp = g_dispatcher->GetTimestampCache() - saveTimestamp;
                if (saveTimestamp < 0) {
                    saveTimestamp = 0;
                }
                m_beginTimestamp = doc["beginTimestamp"].GetInt64() + saveTimestamp;
                m_endTimestamp = doc["endTimestamp"].GetInt64() + saveTimestamp;

                m_reachTplId = doc["reachTplId"].GetInt();
                m_gold = doc["gold"].GetInt();
                m_food = doc["food"].GetInt();
                m_wood = doc["wood"].GetInt();
                m_iron = doc["iron"].GetInt();
                m_stone = doc["stone"].GetInt();

                m_teamId = doc["teamId"].GetInt();

                if (doc["dropItems"].IsArray()) {
                    auto& temp = doc["dropItems"];
                    for (size_t i = 0; i < temp.Size(); ++i) {
                        int tplid = temp[i][0u].GetInt();
                        int count = temp[i][1].GetInt();
                        const ItemTpl* tpl = g_tploader->FindItem(tplid);
                        if (tpl) {
                            m_dropItems.emplace_back(*tpl, count);
                        }
                    }
                }

                m_gatherSpeed = doc["gatherSpeed"].GetDouble();
                m_gatherLoad = doc["gatherLoad"].GetInt();

                m_speedUp = doc.HasMember("speedUp")  ? doc["speedUp"].GetDouble() : 0.0f;
                m_initialSpeed = doc.HasMember("initialSpeed")  ? doc["initialSpeed"].GetDouble() : 0.0f;


                int64_t transportUid = doc.HasMember("transportUid")  ? doc["transportUid"].GetInt64() : 0;
                m_transportAgent = g_mapMgr->FindAgentByUID(transportUid);

                m_transportId1 = doc.HasMember("transportId1") ? doc["transportId1"].GetInt64() : 0;
                m_transportId2 = doc.HasMember("transportId2") ? doc["transportId2"].GetInt64() : 0;

                SwitchTroopImpl();
                return true;
            }
            return false;
        }


        void Troop::SwitchTroopImpl()
        {
            switch (m_type) {
                case MapTroopType::GATHER:
                    m_troopImpl = &m_troopToRes;
                    break;
                case MapTroopType::MONSTER:
                    m_troopImpl = &m_troopToMonster;
                    break;
                case MapTroopType::SIEGE_CITY:
                case MapTroopType::PATROL_CITY:
                    m_troopImpl = &m_troopToCity;
                    break;
                case MapTroopType::SIEGE_CASTLE:
                    m_troopImpl = &m_troopToCastle;
                    break;
                case MapTroopType::CAMP_TEMP:
                case MapTroopType::CAMP_TEMP_ATTACK:
                    m_troopImpl = &m_troopToCampTemp;
                    break;
                case MapTroopType::CAMP_FIXED:
                case MapTroopType::CAMP_FIXED_ATTACK:
                case MapTroopType::CAMP_FIXED_OCCUPY:
                case MapTroopType::ASSIGN:
                    m_troopImpl = &m_troopToCampFixed;
                    break;
                case MapTroopType::SCOUT:
                    m_troopImpl = &m_troopToScout;
                    break;
                case MapTroopType::REINFORCEMENTS:
                    m_troopImpl = &m_troopToReinforcements;
                    break;
                case MapTroopType::SIEGE_CATAPULT:
                    m_troopImpl = &m_troopToCatapult;
                    break;
                case MapTroopType::WORLDBOSS:
                    m_troopImpl = &m_troopToWolrdBoss;
                    break;
                case MapTroopType::TRANSPORT:
                    m_troopImpl = &m_troopToTransport;
                    break;
                default:
                    m_troopImpl = &m_defaultTroopImpl;
                    break;
            }
        }
    }
}




