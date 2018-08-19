#include "resnode.h"
#include "../troop.h"
#include "../agent.h"
#include <base/logger.h>
#include <base/3rd/rapidjson/document.h>
#include <base/3rd/rapidjson/writer.h>
#include <base/3rd/rapidjson/stringbuffer.h>
#include <model/tpl/item.h>
#include <model/tpl/templateloader.h>
#include <model/tpl/configure.h>

namespace ms
{
    namespace map
    {
        using namespace std;
        using namespace model;
        using namespace model::tpl;

        ResNode::ResNode(int id, const MapUnitResourceTpl* tpl, const Point& bornPoint)
            : Unit(id, tpl), m_tpl(tpl), m_bornPoint(bornPoint), m_troop(nullptr)
        {
            m_canGather = tpl->capacity;
        }

        ResNode::~ResNode()
        {
        }

        void ResNode::Init()
        {
            m_armyList = tpl::m_tploader->GetRandomArmyList(m_tpl->armyGroup, m_tpl->armyCount);
            //SetDirty();
        }

        int ResNode::troopId() const
        {
            if (m_troop == nullptr) {
                return 0;
            }
            return m_troop->id();
        }

        int64_t ResNode::uid() const
        {
            if (m_troop == nullptr) {
                return 0;
            }
            return m_troop->uid();
        }

        int64_t ResNode::allianceId() const
        {
            if (m_troop == nullptr) {
                return 0;
            }
            return m_troop->allianceId();
        }

        const std::string ResNode::nickname() const
        {
            if (!m_troop) {
                return "";
            }
            return m_troop->nickname();
        }

        float ResNode::gatherSpeed() const
        {
            return m_tpl->speed;
        }

        float ResNode::resourceLoad() const
        {
            if (m_tpl->type == model::MapUnitType::FARM_FOOD) {
                return g_tploader->configure().resourceLoad.food;
            } else if (m_tpl->type == model::MapUnitType::FARM_WOOD) {
                return g_tploader->configure().resourceLoad.wood;
            } else if (m_tpl->type == model::MapUnitType::MINE_IRON) {
                return g_tploader->configure().resourceLoad.iron;
            } else if (m_tpl->type == model::MapUnitType::MINE_STONE) {
                return g_tploader->configure().resourceLoad.stone;
            }
            return g_tploader->configure().resourceLoad.food;
        }

        float ResNode::troopGatherSpeed() const
        {
            if (m_troop) {
                return m_troop->gatherSpeed();
            }
            return .0f;
        }

        bool ResNode::CanOccupy(Troop* troop)
        {
            return m_canGather > 0 && (!m_troop || (m_troop->uid() != troop->uid() 
                && (m_troop->allianceId() == 0 || m_troop->allianceId() != troop->allianceId())));
        }
        
        void ResNode::ClearTroop()
        {
//             if (m_troop) {
//                 m_troop->agent().RemoveRelatedUnit(this);
//             }
            m_troop = nullptr;
        }

        void ResNode::Occupy(Troop* troop)
        {
//             if (m_troop) {
//                 m_troop->agent().RemoveRelatedUnit(this);
//             }

            m_troop = troop;

//             if (m_troop) {
//                 m_troop->agent().AddRelatedUnit(this);
//             }
            //SetDirty();
        }

        void ResNode::Gather()
        {
            if (m_troop) {
                int num = hasGather();
                m_hadGather += num;
                m_canGather -= num;

                if (m_hurtKeepTime > 0) {
                    m_hurtKeepTime = 0;
                }

                if (m_hadGather > 0) {
                    m_troop->ClearDropItem();
                    SpecialPropIdType spType = DropItemType();
                    m_troop->AddDropItem(int(spType), m_hadGather);
                }
                SetDirty();
            }
        }

        int ResNode::CalculateGatherTime()
        {
            if (m_troop) {
                if (m_troop->gatherSpeed() == .0f) return 0;

                int canGatherNum = m_troop->gatherLoad() / resourceLoad();
                //std::cout << " gatherNum = " << canGatherNum << " troop->gatherSpeed() = " << troop->gatherSpeed() << " resourceLoad = " << resourceLoad() << " hadGather = " << hadGather() << std::endl;
                //此处hadGather不为0 出现在资源抢夺战后的情况
                if (canGatherNum <= hadGather()) {
                    return 0;
                } else {
                    canGatherNum -= hadGather();
                }
                if (canGatherNum > canGather()) {
                    canGatherNum = canGather();
                }
                //std::cout << "time =  /  " << canGatherNum / troop->gatherSpeed() << ", = ceil / " << (int)ceil(canGatherNum / troop->gatherSpeed()) << std::endl;

                return (int)ceil(canGatherNum / m_troop->gatherSpeed());
            }
            return 0;
        }

        int ResNode::ReCalculateGatherTime()
        {
            Gather();
            return CalculateGatherTime();
        }

        int ResNode::leftGather() const
        {
            int left = 0;
            if (m_troop && m_troop->IsReach()) {
                left = m_troop->leftSecond() * m_troop->gatherSpeed();
            }
            //cout << "leftGather = " << left << endl;
            return left;
        }

        int ResNode::hasGather() const
        {
            int gather = 0;
            if (m_troop) {
                int leftGather = ceil(m_troop->leftSecond() * m_troop->gatherSpeed());
                int gatherMax = m_troop->gatherLoad() / resourceLoad() - m_hadGather;
                if (gatherMax > 0) {
                    if (gatherMax > m_canGather) gatherMax = m_canGather;
                    gather = gatherMax - leftGather;
                    if (gather < 0) gather = 0;
                }
            }
            return gather;
        }

        void ResNode::Reset()
        {
            if (m_armyList.size() <= 0) {
                m_armyList = tpl::m_tploader->GetRandomArmyList(m_tpl->armyGroup, m_tpl->armyCount);
            }
            m_hadGather = 0;
            m_canGather = m_tpl->capacity;
            m_hurtKeepTime = 0;

            SetDirty();
        }

        int ResNode::TakeGather()
        {
            int gather = 0;
            int hasGatherd = hasGather();
            m_canGather -= hasGatherd;
            gather = m_hadGather + hasGatherd;
            m_hadGather = 0;
            SetDirty();
            return gather;
        }

        model::SpecialPropIdType ResNode::DropItemType()
        {
            SpecialPropIdType spType = (SpecialPropIdType)0;
            switch (type()) {
                case model::MapUnitType::FARM_FOOD: {
                    spType = SpecialPropIdType::FOOD;
                }
                break;
                case model::MapUnitType::FARM_WOOD: {
                    spType = SpecialPropIdType::WOOD;
                }
                break;
                case model::MapUnitType::MINE_IRON: {
                    spType = SpecialPropIdType::IRON;
                }
                break;
                 case model::MapUnitType::MINE_STONE: {
                    spType = SpecialPropIdType::STONE;
                }
                break;
                default:
                    break;
            }
            return spType;
        }

        void ResNode::OnTroopLeave(Troop* troop)
        {
            if (troopId() != troop->id()) {
                return;
            }
            m_noviceuid = 0;

            int gather = TakeGather();
            if (gather > 0) {
                troop->ClearDropItem();
                SpecialPropIdType spType = DropItemType();
                troop->AddDropItem(int(spType), gather);
                g_mapMgr->OnGather(gather);
            }
            troop->SetGatherRemain(canGather());

            ClearTroop();
            NoticeUnitUpdate();
            if (canGather() <= 0) {
                RefreshSelf();
            } else {
                CheckHurt();
            }
        }

        void ResNode::SetTpl(const MapUnitTpl* tpl)
        {
            if (tpl && tpl->ToResource()) {
                Unit::SetTpl(tpl);

                m_tpl = tpl->ToResource();
                m_canGather = m_tpl->capacity;
                SetDirty();
            }
        }

        std::string ResNode::Serialize()
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

                writer.String("canGather");
                writer.Int(m_canGather);
                writer.String("hadGather");
                writer.Int(m_hadGather);

                writer.String("hurtKeepTime");
                writer.Int64(m_hurtKeepTime);

                writer.String("noviceuid");
                writer.Int64(m_noviceuid);

                writer.EndObject();
                jsonString = jsonbuffer.GetString();
            } catch (std::exception& ex) {
                LOG_ERROR("save ResNode json fail: %s\n", ex.what());
            }
            return jsonString;
        }

        bool ResNode::Deserialize(std::string data)
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
                    m_canGather = doc["canGather"].GetInt();
                    m_hadGather = doc["hadGather"].GetInt();
                    m_hurtKeepTime = doc["hurtKeepTime"].GetInt64();
                    m_noviceuid = doc["noviceuid"].GetInt64();
                }
                return true;
            } catch (std::exception& ex) {
                LOG_ERROR("json string to ResNode fail: %s\n", ex.what());
                return false;
            }
        }

        void ResNode::CheckHurt()
        {
            float remain = m_canGather / m_tpl->capacity * 1.0f;
            if (remain <= 0.3f) {
                int64_t now = g_dispatcher->GetTimestampCache();
                m_hurtKeepTime = RESOURCE_HURTED_KEEPTIME + now;
                SetDirty();
            }
        }

        void ResNode::FinishDeserialize()
        {
            if (m_hadGather <= 0) {
                Init();
            }
        }
    }
}


