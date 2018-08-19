#include "trapset.h"
#include <base/data.h>
#include <model/tpl/templateloader.h>
#include <model/tpl/configure.h>
#include <model/tpl/army.h>
#include <base/data.h>

namespace ms
{
    namespace map
    {
        using namespace std;
        using namespace model;
        using namespace model::tpl;
        using namespace tpl;
        using namespace base;

        //--------------------- TrapInfo ----------------------
        
        TrapInfo::TrapInfo(int trapType)
        : m_type(trapType)
        {
            m_armyTpl = g_tploader->FindNArmy(trapType, 1);
            if (m_armyTpl) {
                m_tplid = m_armyTpl->id;
            }
        }

        void TrapInfo::InitProp()
        {
            if (m_armyTpl) {
                // m_attack = getProp(AttributeType::TROOPS_ATTACK, m_armyTpl->attack);
                 
                m_attack = 1;
            }
        }

        void TrapInfo::SetProp(model::AttributeType type, int base, int pct, int ext)
        {
            SetBaseAttr(type, base, model::AttributeAdditionType::BASE);
            SetBaseAttr(type, pct, model::AttributeAdditionType::PCT);
            SetBaseAttr(type, ext, model::AttributeAdditionType::EXT);
        }

        int TrapInfo::GetProp(model::AttributeType type) const
        {
            return GetBaseAttr(type, AttributeAdditionType::BASE) * (1 +GetBaseAttr(type, AttributeAdditionType::PCT)/10000.0) +GetBaseAttr(type, AttributeAdditionType::EXT);
        }

        int TrapInfo::GetBaseAttr(model::AttributeType type, model::AttributeAdditionType addType) const
        {
            int index = GetIndex(type);
            if (index >= 0 && index < kTrapAttrCount) {
                return m_attrBase[index][(int)addType];
            }
            return 0;
        }

        void TrapInfo::SetBaseAttr(model::AttributeType type, int value, model::AttributeAdditionType addType)
        {
            int index = GetIndex(type);
            if (index >= 0 && index < kTrapAttrCount) {
                m_attrBase[index][(int)addType] = value;
            }
        }

        void TrapInfo::Add(model::ArmyState state, int count)
        {
            if (count <= 0) return;

            auto it = m_states.find((int)state);
            if (it != m_states.end()) {
                it->second += count;
            } else {
                m_states.emplace((int)state, count);
            }
        }

        bool TrapInfo::Remove(model::ArmyState state, int count)
        {
            if (count <= 0) return true;

            auto it = m_states.find((int)state);
            if (it != m_states.end() && it->second >= count) {
                it->second -= count;
                return true;
            }
            return false;
        }

        void TrapInfo::Set(ArmyState state, int count)
        {
            auto it = m_states.find((int)state);
            if (it != m_states.end()) {
                it->second = count;
            } else {
                m_states.emplace((int)state, count);
            }
        }

        bool TrapInfo::Die(int count)
        {
            if (Remove(model::ArmyState::NORMAL, count)) {
                Add(model::ArmyState::DIE, count);
                return true;
            }
            return false;
        }

        void TrapInfo::OnPropertyUpdate(const info::Property& property)
        {
            auto getProp = [this](model::AttributeType type, int base, float pct, int ext) {
                return (GetBaseAttr(type, AttributeAdditionType::BASE) + base) * (1 +GetBaseAttr(type, AttributeAdditionType::PCT)/10000.0 + pct) +GetBaseAttr(type, AttributeAdditionType::EXT) + ext;
            };

            if (m_armyTpl) {
                //m_attack = getProp(AttributeType::TRAP_ATTACK, m_armyTpl->attack, property.trapAttackPct, property.trapAttackExt);
                m_attack = 1;
            }
        }

        int TrapInfo::GetIndex(model::AttributeType type) const
        {
            int index = 0;
            switch (type) {
                case AttributeType::TRAP_ATTACK:
                    index = 1;
                    break;
                default:
                    break;
            }
            return index;
        }


        void TrapInfo::Serialize(rapidjson::Writer< rapidjson::StringBuffer >& writer) const
        {
            writer.StartObject();
            writer.Key("type");
            writer.Int(type());

            writer.Key("attrBase");
            writer.StartArray();
            for (int i = 0; i < kTrapAttrCount; ++i) {
                writer.StartArray();
                writer.Int(m_attrBase[i][(int)AttributeAdditionType::BASE]);
                writer.Int(m_attrBase[i][(int)AttributeAdditionType::PCT]);
                writer.Int(m_attrBase[i][(int)AttributeAdditionType::EXT]);
                writer.EndArray();
            }
            writer.EndArray();


            writer.Key("state");
            writer.StartArray();
            for (auto it2 = states().begin(); it2 != states().end(); ++it2) {
                if (it2->second > 0) {
                    writer.StartArray();
                    writer.Int((int)it2->first);
                    writer.Int(it2->second);
                    writer.EndArray();
                }
            }
            writer.EndArray();
            writer.EndObject();
        }

        void TrapInfo::Desrialize(rapidjson::GenericValue< rapidjson::UTF8< char > >& trap)
        {
            int trapType = trap["type"].GetInt();

            auto& attrBase = trap["attrBase"];
            for (size_t i = 0; i < attrBase.Size() && i < kTrapAttrCount; ++i) {
                m_attrBase[i][(int)AttributeAdditionType::BASE] = attrBase[i][0].GetInt();
                m_attrBase[i][(int)AttributeAdditionType::PCT] = attrBase[i][1].GetInt();
                m_attrBase[i][(int)AttributeAdditionType::EXT] = attrBase[i][2].GetInt();
            }

            auto& ArmyState = trap["state"];
            for (size_t i = 0; i < ArmyState.Size(); ++i) {
                model::ArmyState state = static_cast<model::ArmyState>(ArmyState[i][0u].GetInt());
                int count = ArmyState[i][1].GetInt();
                Add(state, count);
            }

            m_type = trapType;
            m_armyTpl = g_tploader->FindNArmy(trapType, 1);
            if (m_armyTpl) {
                m_tplid = m_armyTpl->id;
            }
        }


        // ----------------------------- TrapSet ------------------------------

        void TrapSet::AddTrap(int trapType,  model::ArmyState state, int count)
        {
            auto it = m_indieArmies.find((int)trapType);
            if (it != m_indieArmies.end()) {
                it->second.Add(state,  count);
            } else {
                auto rt = m_indieArmies.emplace((int)trapType, TrapInfo((int)trapType));
                if (rt.second) {
                    rt.first->second.Add(state,  count);
                }
            }
        }

        void TrapSet::AddTrap(const TrapInfo& trapInfo)
        {
            m_indieArmies.emplace(trapInfo.type(),  trapInfo);
        }

        bool TrapSet::Die(int trapType, int count)
        {
            auto it = m_indieArmies.find(trapType);
            if (it != m_indieArmies.end()) {
                it->second.Die(count);
                return true;
            }
            return false;
        }

        bool TrapSet::Set(int trapType, model::ArmyState state, int count)
        {
            auto it = m_indieArmies.find(trapType);
            if (it != m_indieArmies.end()) {
                it->second.Set(state, count);
                return true;
            }
            return false;
        }

        TrapInfo* TrapSet::GetTrap(int trapType)
        {
            auto it = m_indieArmies.find(trapType);
            if (it !=  m_indieArmies.end()) {
                return &it->second;
            }
            return nullptr;
        }

        int TrapSet::Total() const
        {
            int sum = 0;
            for (auto it = m_indieArmies.begin(); it != m_indieArmies.end(); ++it) {
                const TrapInfo& trap = it->second;
                if (const NArmyTpl* tpl = g_tploader->FindNArmy(trap.tplid())) {
                    if (tpl->IsTrap()) {
                        sum += trap.count(ArmyState::NORMAL);
                    }
                }
            }
            return sum;
        }

        void TrapSet::ClearAllExceptNormal()
        {
            for (auto it = m_indieArmies.begin(); it != m_indieArmies.end(); ++it) {
                TrapInfo& info = it->second;
                for (auto it2 = info.states().begin(); it2 != info.states().end(); ++it2) {
                    if (it2->first != (int)ArmyState::NORMAL) {
                        it2->second = 0;
                    }
                }
            }
        }

        void TrapSet::ClearAll()
        {
            m_indieArmies.clear();
        }

        void TrapSet::OnPropertyUpdate(const info::Property& property)
        {
            for (auto it = m_indieArmies.begin(); it != m_indieArmies.end(); ++it) {
                TrapInfo& trap = it->second;
                trap.OnPropertyUpdate(property);
            }
        }

        void TrapSet::InitProp()
        {
            for (auto it = m_indieArmies.begin(); it != m_indieArmies.end(); ++it) {
                TrapInfo& trap = it->second;
                trap.InitProp();
            }
        }

        void TrapSet::SetDataTable(base::DataTable& dt,   int specify, bool isFalse) const
        {
            for (auto it = m_indieArmies.begin(); it != m_indieArmies.end(); ++it) {
                const TrapInfo& trapInfo = it->second;
                DataTable trap;
                trap.Set("trapType", trapInfo.type());

                if (specify ==  0 ||  specify ==  2) {
                    DataTable states;
                    for (auto it2 = trapInfo.states().begin(); it2 != trapInfo.states().end(); ++it2) {
                        if (it2->second > 0) {
                            if (!isFalse) {
                                states.Set(it2->first, it2->second);
                            } else {
                                states.Set(it2->first, it2->second * 2);
                            }
                        }
                    }
                    trap.Set("state", states);
                }
                dt.Set(it->first,  trap);
            }
        }

        void TrapSet::Serialize(rapidjson::Writer< rapidjson::StringBuffer >& writer) const
        {
            writer.StartObject();
            writer.Key("trap");
            writer.StartArray();
            for (auto it = m_indieArmies.begin(); it != m_indieArmies.end(); ++it) {
                const TrapInfo& trapInfo = it->second;
                trapInfo.Serialize(writer);
            }
            writer.EndArray();
            writer.EndObject();
        }

        void TrapSet::Desrialize(rapidjson::GenericValue< rapidjson::UTF8< char > >& gv)
        {
            for (size_t i = 0; i < gv["trap"].Size(); ++i) {
                auto& trap = gv["trap"][i];
                TrapInfo trapInfo;
                trapInfo.Desrialize(trap);
                AddTrap(trapInfo);
            }
        }
    }
}