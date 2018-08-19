#include "armylist.h"
#include <model/metadata.h>
#include <model/tpl/army.h>
#include <model/tpl/templateloader.h>
#include <base/framework.h>
#include <base/logger.h>
#include <base/data.h>
#include <cmath>
#include "tpl/npcarmytpl.h"
#include "model/tpl/hero.h"
#include "model/tpl/configure.h"

namespace ms
{
    namespace map
    {
        using namespace std;
        using namespace base;
        using namespace model;
        using namespace model::tpl;
        using namespace tpl;

//-------------------------------------------
        
        ArmyInfo::ArmyInfo(int armyType, int armyLevel)
        : m_type(armyType), m_level(armyLevel)
        {
            m_armyTpl = g_tploader->FindNArmy(armyType, armyLevel);
            if (m_armyTpl) {
                m_tplid = m_armyTpl->id;
            }
        }

        void ArmyInfo::InitProp()
        {
//             SetBaseAttr(AttributeType::TROOPS_HEALTH, hp, model::AttributeAdditionType::BASE);
//             SetBaseAttr(AttributeType::TROOPS_ATTACK, attack, model::AttributeAdditionType::BASE);
//             SetBaseAttr(AttributeType::TROOPS_DEFENSE, defense, model::AttributeAdditionType::BASE);
//             SetBaseAttr(AttributeType::TROOPS_SPEED, speed, model::AttributeAdditionType::BASE);
//             SetBaseAttr(AttributeType::TROOPS_ATTACK_CITY, wallAttack, model::AttributeAdditionType::BASE);

            // auto getProp = [this](model::AttributeType type, int base) {
            //     return (GetBaseAttr(type, AttributeAdditionType::BASE) + base) * (1 +GetBaseAttr(type, AttributeAdditionType::PCT)/10000.0) +GetBaseAttr(type, AttributeAdditionType::EXT);
            // };

            if (m_armyTpl) {
                // m_hp = getProp(AttributeType::TROOPS_HEALTH, m_armyTpl->hp);
                // m_attack = getProp(AttributeType::TROOPS_ATTACK, m_armyTpl->attack);
                // m_defense = getProp(AttributeType::TROOPS_DEFENSE, m_armyTpl->defense);
                // m_speed = getProp(AttributeType::TROOPS_SPEED, m_armyTpl->speed);
                // m_wallAttack = getProp(AttributeType::TROOPS_ATTACK_CITY, m_armyTpl->attackWall);
                 
                 
                m_hp = 1;
                m_attack = 1;
                m_defense = 1;
                m_speed = 1;
                m_wallAttack = 1; 
            }

//             if (m_armyTpl) {
//                 m_hp = m_armyTpl->hp;
//                 m_attack = m_armyTpl->attack;
//                 m_defense = m_armyTpl->defense;
//                 m_speed = m_armyTpl->speed;
//             }
        }

        void ArmyInfo::SetProp(model::AttributeType type, int base, int pct, int ext)
        {
            SetBaseAttr(type, base, model::AttributeAdditionType::BASE);
            SetBaseAttr(type, pct, model::AttributeAdditionType::PCT);
            SetBaseAttr(type, ext, model::AttributeAdditionType::EXT);
        }

        int ArmyInfo::GetProp(model::AttributeType type) const
        {
            return GetBaseAttr(type, AttributeAdditionType::BASE) * (1 +GetBaseAttr(type, AttributeAdditionType::PCT)/10000.0) +GetBaseAttr(type, AttributeAdditionType::EXT);
        }

        int ArmyInfo::GetBaseAttr(model::AttributeType type, model::AttributeAdditionType addType) const
        {
            int index = GetIndex(type);
            if (index >= 0 && index < kArmyAttrCount) {
                return m_attrBase[index][(int)addType];
            }
            return 0;
        }

        void ArmyInfo::SetBaseAttr(model::AttributeType type, int value, model::AttributeAdditionType addType)
        {
            int index = GetIndex(type);
            if (index >= 0 && index < kArmyAttrCount) {
                m_attrBase[index][(int)addType] = value;
            }
        }

        bool ArmyInfo::IsTrap()
        {
            if (m_armyTpl) {
                return m_armyTpl->IsTrap();
            }
            return false;
        }

        void ArmyInfo::Add(model::ArmyState state, int count)
        {
            if (count <= 0) return;

            auto it = m_states.find((int)state);
            if (it != m_states.end()) {
                it->second += count;
            } else {
                m_states.emplace((int)state, count);
            }
        }

        bool ArmyInfo::Remove(model::ArmyState state, int count)
        {
            if (count <= 0) return true;

            auto it = m_states.find((int)state);
            if (it != m_states.end() && it->second >= count) {
                it->second -= count;
                return true;
            }
            return false;
        }

        void ArmyInfo::Set(ArmyState state, int count)
        {
            auto it = m_states.find((int)state);
            if (it != m_states.end()) {
                it->second = count;
            } else {
                m_states.emplace((int)state, count);
            }
        }

        bool ArmyInfo::Die(int count)
        {
            if (Remove(model::ArmyState::NORMAL, count)) {
                Add(model::ArmyState::DIE, count);
                return true;
            }
            return false;
        }

        bool ArmyInfo::Wound(int count)
        {
            if (Remove(model::ArmyState::NORMAL, count)) {
                Add(model::ArmyState::WOUNDED, count);
                return true;
            }
            return false;
        }

        bool ArmyInfo::Recover(int count)
        {
            if (Remove(model::ArmyState::NORMAL, count)) {
                Add(model::ArmyState::RECOVER, count);
                return true;
            }
            return false;
        }

        int ArmyInfo::DieToWound(float percent)
        {
            if (percent > 1) percent = 1.0f;
            int cnt = std::floor(count(model::ArmyState::DIE) * percent);
            if (cnt > 0) {
                if (Remove(model::ArmyState::DIE, cnt)) {
                    Add(model::ArmyState::WOUNDED, cnt);
                }
            }
            return cnt;
        }

        int ArmyInfo::ToWound(float percent)
        {
            if (percent > 1) percent = 1.0f;
            int cnt = std::floor(count(model::ArmyState::DIE) * percent);
            if (cnt > 0) {
                if (Remove(model::ArmyState::NORMAL, cnt)) {
                    Add(model::ArmyState::WOUNDED, cnt);
                }
            }
            return cnt;
        }

        int ArmyInfo::HurtHp(int hurtHp)
        {
            int die = hurtHp / m_hp + (hurtHp % m_hp > 0  ? 1 : 0);
            int cnt = count(model::ArmyState::NORMAL);
            if (die > cnt) {
                die = cnt;
            }

            Die(die);
            return die * m_hp;
        }

        int ArmyInfo::Power()
        {
            //Todo: modify
            //return  m_armyTpl->power * count(model::ArmyState::NORMAL);
            return 1;
        }

        int ArmyInfo::ClearDie()
        {
            int count = 0;
            auto it = m_states.find((int)ArmyState::DIE);
            if (it != m_states.end()) {
                count = it->second;
                it->second = 0;
            }
            return count;
        }

        void ArmyInfo::OnPropertyUpdate(const info::Property& property)
        {
            // auto getProp = [this](model::AttributeType type, int base, float pct, int ext) {
            //     return (GetBaseAttr(type, AttributeAdditionType::BASE) + base) * (1 +GetBaseAttr(type, AttributeAdditionType::PCT)/10000.0 + pct) +GetBaseAttr(type, AttributeAdditionType::EXT) + ext;
            // };

            if (m_armyTpl) {
                // m_hp = getProp(AttributeType::TROOPS_HEALTH, m_armyTpl->hp, property.troopHpPct, property.troopHpExt);
                // m_attack = getProp(AttributeType::TROOPS_ATTACK, m_armyTpl->attack, property.troopAttackPct, property.troopAttackExt);
                // m_defense = getProp(AttributeType::TROOPS_DEFENSE, m_armyTpl->defense, property.troopDefensePct, property.troopDefenseExt);
                // m_speed = getProp(AttributeType::TROOPS_SPEED, m_armyTpl->speed, property.troopSpeedPct, property.troopSpeedExt);
                // m_wallAttack = getProp(AttributeType::TROOPS_ATTACK_CITY, m_armyTpl->attackWall, property.troopAttackCityPct, property.troopAttackCityExt);
                m_hp = 1;
                m_attack = 1;
                m_defense = 1;
                m_speed = 1;
                m_wallAttack = 1;
            }
        }

        int ArmyInfo::GetIndex(model::AttributeType type) const
        {
            int index = 0;
            switch (type) {
                case AttributeType::TROOPS_ATTACK:
                    index = 1;
                    break;
                case AttributeType::TROOPS_DEFENSE:
                    index = 2;
                    break;
                case AttributeType::TROOPS_HEALTH:
                    index = 3;
                    break;
                case AttributeType::TROOPS_SPEED:
                    index = 4;
                    break;
                case AttributeType::TROOPS_MORALE:
                    index = 5;
                    break;
                case AttributeType::TROOPS_ATTACK_CITY:
                    index = 6;
                    break;
                default:
                    break;
            }
            return index;
        }


        void ArmyInfo::Serialize(rapidjson::Writer< rapidjson::StringBuffer >& writer) const
        {
            writer.StartObject();
            writer.Key("type");
            writer.Int(type());
            writer.Key("level");
            writer.Int(level());

            writer.Key("attrBase");
            writer.StartArray();
            for (int i = 0; i < kArmyAttrCount; ++i) {
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

        void ArmyInfo::Desrialize(rapidjson::GenericValue< rapidjson::UTF8< char > >& army)
        {
            int armyType = army["type"].GetInt();
            int armyLevel = army["level"].GetInt();

            auto& attrBase = army["attrBase"];
            for (size_t i = 0; i < attrBase.Size() && i < kArmyAttrCount; ++i) {
                m_attrBase[i][(int)AttributeAdditionType::BASE] = attrBase[i][0].GetInt();
                m_attrBase[i][(int)AttributeAdditionType::PCT] = attrBase[i][1].GetInt();
                m_attrBase[i][(int)AttributeAdditionType::EXT] = attrBase[i][2].GetInt();
            }

            auto& armyState = army["state"];
            for (size_t i = 0; i < armyState.Size(); ++i) {
                model::ArmyState state = static_cast<model::ArmyState>(armyState[i][0u].GetInt());
                int count = armyState[i][1].GetInt();
                Add(state, count);
            }

            m_type = armyType;
            m_level = armyLevel;
            m_armyTpl = g_tploader->FindNArmy(armyType, armyLevel);
            if (m_armyTpl) {
                m_tplid = m_armyTpl->id;
            }
        }

        // ----------------------------- Armyset ------------------------------

        void ArmySet::AddArmy(int armyType, int armyLevel, model::ArmyState state, int count)
        {
            auto it = m_indieArmies.find((int)armyType);
            if (it != m_indieArmies.end()) {
                it->second.Add(state,  count);
            } else {
                auto rt = m_indieArmies.emplace((int)armyType, ArmyInfo((int)armyType,  armyLevel));
                if (rt.second) {
                    rt.first->second.Add(state,  count);
                }
            }
        }

        void ArmySet::AddArmy(const ArmyInfo& armyInfo)
        {
            m_indieArmies.emplace(armyInfo.type(),  armyInfo);
        }

        bool ArmySet::Die(int armyType, int count)
        {
            auto it = m_indieArmies.find(armyType);
            if (it != m_indieArmies.end()) {
                it->second.Die(count);
                return true;
            }
            return false;
        }

        bool ArmySet::Set(int armyType, model::ArmyState state, int count)
        {
            auto it = m_indieArmies.find(armyType);
            if (it != m_indieArmies.end()) {
                it->second.Set(state, count);
                return true;
            }
            return false;
        }

        ArmyInfo* ArmySet::GetArmy(int armyType)
        {
            auto it = m_indieArmies.find(armyType);
            if (it !=  m_indieArmies.end()) {
                return &it->second;
            }
            return nullptr;
        }

        int ArmySet::Total() const
        {
            int sum = 0;
            for (auto it = m_indieArmies.begin(); it != m_indieArmies.end(); ++it) {
                const ArmyInfo& army = it->second;
                if (const NArmyTpl* tpl = g_tploader->FindNArmy(army.tplid())) {
                    if (tpl->IsTrap()) {
                        sum += army.count(ArmyState::NORMAL);
                    }
                }
            }
            return sum;
        }

        void ArmySet::ClearAllExceptNormal()
        {
            for (auto it = m_indieArmies.begin(); it != m_indieArmies.end(); ++it) {
                ArmyInfo& info = it->second;
                for (auto it2 = info.states().begin(); it2 != info.states().end(); ++it2) {
                    if (it2->first != (int)ArmyState::NORMAL) {
                        it2->second = 0;
                    }
                }
            }
        }

        void ArmySet::ClearAll()
        {
            m_indieArmies.clear();
        }

        void ArmySet::OnPropertyUpdate(const info::Property& property)
        {
            for (auto it = m_indieArmies.begin(); it != m_indieArmies.end(); ++it) {
                ArmyInfo& army = it->second;
                army.OnPropertyUpdate(property);
            }
        }

        void ArmySet::InitProp()
        {
            for (auto it = m_indieArmies.begin(); it != m_indieArmies.end(); ++it) {
                ArmyInfo& army = it->second;
                army.InitProp();
            }
        }

        void ArmySet::SetDataTable(base::DataTable& dt,   int specify, bool isFalse) const
        {
            for (auto it = m_indieArmies.begin(); it != m_indieArmies.end(); ++it) {
                const ArmyInfo& armyInfo = it->second;
                DataTable army;
                army.Set("armyType", armyInfo.type());

                if (specify ==  0 ||  specify ==  1) {
                    army.Set("level", armyInfo.level());
                }

                if (specify ==  0 ||  specify ==  2) {
                    DataTable states;
                    for (auto it2 = armyInfo.states().begin(); it2 != armyInfo.states().end(); ++it2) {
                        if (it2->second > 0) {
                            if (!isFalse) {
                                states.Set(it2->first, it2->second);
                            } else {
                                states.Set(it2->first, it2->second * 2);
                            }
                        }
                    }
                    army.Set("state", states);
                }
                dt.Set(it->first,  army);
            }
        }

        void ArmySet::Serialize(rapidjson::Writer< rapidjson::StringBuffer >& writer) const
        {
            writer.StartObject();
            writer.Key("army");
            writer.StartArray();
            for (auto it = m_indieArmies.begin(); it != m_indieArmies.end(); ++it) {
                const ArmyInfo& armyInfo = it->second;
                armyInfo.Serialize(writer);
            }
            writer.EndArray();
            writer.EndObject();
        }

        void ArmySet::Desrialize(rapidjson::GenericValue< rapidjson::UTF8< char > >& gv)
        {
            for (size_t i = 0; i < gv["army"].Size(); ++i) {
                auto& army = gv["army"][i];
                ArmyInfo armyInfo;
                armyInfo.Desrialize(army);
                AddArmy(armyInfo);
            }
        }

        ArmyGroup::ArmyGroup(NpcArmyTpl* npcArmyTpl)
            : m_armyInfo(npcArmyTpl->army.type, npcArmyTpl->army.level), m_heroInfo(npcArmyTpl->hero.id, npcArmyTpl->hero.level, npcArmyTpl->hero.star)
        {
            m_heroInfo.SetSkill(npcArmyTpl->hero.skill);
            m_heroInfo.SetSoul(npcArmyTpl->hero.soul);
            m_heroInfo.InipProp(npcArmyTpl->army.type, npcArmyTpl->army.level);

            m_armyInfo.Add(ArmyState::NORMAL, npcArmyTpl->army.count);
            //LOG_DEBUG("armylist id %d position %d ", npcArmyTpl->hero.id, m_position);
           
            if (npcArmyTpl->hero.position != 0) {
                m_position = npcArmyTpl->hero.position;
            }
            //m_armyInfo.InitProp();
        }

//         ArmyGroup::ArmyGroup(ArmyInfo& armyInfo, HeroInfo& heroInfo, int pos)
//           :m_armyInfo(armyInfo), m_heroInfo(heroInfo), m_position(pos)
//         {
//         }

        ArmyGroup::ArmyGroup(int armyType, int armyLevel, HeroInfo& heroInfo, int pos)
          : m_armyInfo(armyType, armyLevel), m_heroInfo(heroInfo), m_position(pos)
        {
        }

        int ArmyGroup::Power()
        {
            //战力
            //return m_armyInfo.Power();
            return m_armyInfo.Power() *( m_heroInfo.power()/(5000 + 5.0 *m_heroInfo.power()));
        }

        ArmyGroup* ArmyList::AddArmyGroup(ms::map::tpl::NpcArmyTpl* npcArmyTpl)
        {
            ArmyGroup* ag = nullptr;
            if (npcArmyTpl) {
                auto pair = m_armies.emplace(npcArmyTpl->hero.id, npcArmyTpl);
                if (pair.second) {
                    ag = &pair.first->second;
                }
            }
            return ag;
        }

        ArmyGroup* ArmyList::AddArmyGroup(int armyType, int armyLevel, HeroInfo& heroInfo, int pos)
        {
            ArmyGroup* ag = nullptr;
            auto pair = m_armies.emplace(heroInfo.tplId(), ArmyGroup(armyType, armyLevel, heroInfo, pos));
            if (pair.second) {
                ag = &pair.first->second;
            }
            return ag;
        }

        void ArmyList::AddArmyGroup(const ArmyGroup& armyGroup)
        {
            int type = armyGroup.armyInfo().type();
            if (type >= 1 &&  type <=  9) {
                if (m_armies.emplace(armyGroup.heroInfo().tplId(), armyGroup).second ==  false) {
                    LOG_WARN("exist same hero.");
                }
            }
        }

        void ArmyList::RemoveArmy(int heroTplId, model::ArmyState state, int count)
        {
            if (count == 0) return;
            auto it = m_armies.find(heroTplId);
            if (it != m_armies.end()) {
                it->second.armyInfo().Remove(state, count);
            }
        }

        bool ArmyList::SetArmy(int heroTplId, ArmyState state, int count)
        {
            auto it = m_armies.find(heroTplId);
            if (it != m_armies.end()) {
                it->second.armyInfo().Set(state, count);
                return true;
            }
            return false;
        }

        bool ArmyList::Die(int heroTplId,int count)
        {
            auto it = m_armies.find(heroTplId);
            if (it != m_armies.end()) {
                return it->second.armyInfo().Die(count);
            }
            return false;
        }

        bool ArmyList::Wound(int heroTplId,int count)
        {
            auto it = m_armies.find(heroTplId);
            if (it != m_armies.end()) {
                return it->second.armyInfo().Wound(count);
            }
            return false;
        }

        bool ArmyList::Recover(int heroTplId,int count)
        {
            auto it = m_armies.find(heroTplId);
            if (it != m_armies.end()) {
                return it->second.armyInfo().Recover(count);
            }
            return false;
        }

        void ArmyList::Add(int heroTplId,int count)
        {
            auto it = m_armies.find(heroTplId);
            if (it != m_armies.end()) {
                it->second.armyInfo().Add(model::ArmyState::NORMAL, count);
            }
        }


        int ArmyList::ActiveSize()
        {
            int size = 0;
            for (auto it = m_armies.begin(); it != m_armies.end(); ++it) {
                const ArmyInfo& army = it->second.armyInfo();
                if (army.count(ArmyState::NORMAL) > 0) {
                    ++size;
                }
            }
            return size;
        }

        ArmyGroup* ArmyList::GetArmyGroup(int heroTplId)
        {
            auto it = m_armies.find(heroTplId);
            if (it != m_armies.end()) {
                return &it->second;
            }
            return nullptr;
        }

        ArmyGroup* ArmyList::GetArmyGroupByPos(int pos)
        {
            ArmyGroup* ag = nullptr;
            for (auto it = m_armies.begin(); it != m_armies.end(); ++it) {
                auto& armyGroup = it->second;
                if (armyGroup.position() ==  pos) {
                    ag =&armyGroup;
                    break;
                }
            }
            return ag;
        }

        int ArmyList::ArmyCount(model::ArmyState state) const
        {
            int sum = 0;
            for (auto it = m_armies.begin(); it != m_armies.end(); ++it) {
                const ArmyInfo& army = it->second.armyInfo();
                //if (const NArmyTpl* tpl = g_tploader->FindNArmy(army.tplid())) {
                    //if (tpl->IsTrap()) continue;
                    sum += army.count(state);
                //}
            }
            return sum;
        }

        int ArmyList::GetArmyTotal(bool isFalse) const
        {
            int sum = 0;
            for (auto it = m_armies.begin(); it != m_armies.end(); ++it) {
                const ArmyInfo& army = it->second.armyInfo();
                //if (const NArmyTpl* tpl = g_tploader->FindNArmy(army.tplid())) {
                    //if (tpl->IsTrap()) continue;
                    if (isFalse) {
                        sum += army.count(ArmyState::NORMAL) * 2;
                    } else {
                        sum += army.count(ArmyState::NORMAL);
                    }
                //}
            }
            return sum;
        }

        float ArmyList::GetBaseSpeed()
        {
            float speed = 0.0f;
            auto findMinSpeed = [&](const ArmyList & list) {
                for (auto it = list.armies().begin(); it != list.armies().end(); ++it) {
                    const ArmyInfo& army = it->second.armyInfo();
                    if (const NArmyTpl* tpl = g_tploader->FindNArmy(army.tplid())) {
                        //if (tpl->IsTrap()) continue;
                        if (speed == 0) {
                            speed = tpl->speed;
                        } else {
                            if (speed > tpl->speed) {
                                speed = tpl->speed;
                            }
                        }
                    }
                }
            };

            findMinSpeed(*this);
            return speed;
        }

        int ArmyList::GetArmyTotalProbably(bool isFalse) const
        {
            int rand = framework.random().GenRandomNum(80, 120);
            int mul = 1;
            if (isFalse) mul = 2;
            return GetArmyTotal() * rand / 100 * mul;
        }

        int ArmyList::GetArmyWounded() const
        {
            int sum = 0;
            for (auto it = m_armies.begin(); it != m_armies.end(); ++it) {
                const ArmyInfo& army = it->second.armyInfo();
                //if (const NArmyTpl* tpl = g_tploader->FindNArmy(army.tplid())) {
                    //if (tpl->IsTrap()) continue;
                    sum += army.count(ArmyState::WOUNDED);
                //}
            }
            return sum;
        }

        int ArmyList::GetLoadTotal() const
        {
            int sum = 0;
            for (auto it = m_armies.begin(); it != m_armies.end(); ++it) {
                const ArmyInfo& army = it->second.armyInfo();
                if (const NArmyTpl* tpl = g_tploader->FindNArmy(army.tplid())) {
                    sum += tpl->loads * army.count(ArmyState::NORMAL);
                    //std::cout << "loads = " << tpl->loads << " count = " << army.count(ArmyState::NORMAL) << " sum = " << sum << std::endl;
                }
            }
            return sum;
        }

        int ArmyList::GetHurtTotal() const
        {
            int sum = 0;
            for (auto it = m_armies.begin(); it != m_armies.end(); ++it) {
                const ArmyInfo& info = it->second.armyInfo();
                sum += info.count(ArmyState::HURT);
            }
            return sum;
        }

        bool ArmyList::IsAllDie() const
        {
            int count = 0;
            for (auto it = m_armies.begin(); it != m_armies.end(); ++it) {
                const ArmyInfo& army = it->second.armyInfo();
                count += army.count(ArmyState::NORMAL);
            }
            return (count > 0) ? false : true;
        }

        void ArmyList::Debug() const
        {
            // cout << "ArmyList::Debug" << endl;
            for (auto it = m_armies.begin(); it != m_armies.end(); ++it) {
                const ArmyInfo& army = it->second.armyInfo();
                LOG_DEBUG("ArmyList tplid = %d, NORMAL = %d, MARCH = %d, WOUNDED = %d, DIE = %d, KILL = %d, HURT = %d", it->first, army.count(ArmyState::NORMAL), army.count(ArmyState::MARCHING), army.count(ArmyState::WOUNDED), army.count(ArmyState::DIE), army.count(ArmyState::KILL), army.count(ArmyState::HURT));
            }
            // LOG_DEBUG("HeroInfos::Debug");
            for (auto it = m_armies.begin(); it != m_armies.end(); ++it) {
                const HeroInfo& info = it->second.heroInfo();
                LOG_DEBUG("HeroInfo, heroId = %d, heroPhysical = %d", info.tplId(), info.physical());
            }
        }

        void ArmyList::ClearAllWounded()
        {
            for (auto it = m_armies.begin(); it != m_armies.end(); ++it) {
                ArmyInfo& army = it->second.armyInfo();
                auto it2 = army.states().find((int)ArmyState::WOUNDED);
                if (it2 != army.states().end()) {
                    it2->second = 0;
                }
            }
        }

        void ArmyList::ClearAllHurt()
        {
            for (auto it = m_armies.begin(); it != m_armies.end(); ++it) {
                ArmyInfo& army = it->second.armyInfo();
                auto it2 = army.states().find((int)ArmyState::HURT);
                if (it2 != army.states().end()) {
                    it2->second = 0;
                }
            }
        }

        void ArmyList::DieToWound(float percent)
        {
            for (auto it = m_armies.begin(); it != m_armies.end(); ++it) {
                ArmyInfo& info = it->second.armyInfo();
                const NArmyTpl* tpl = g_tploader->FindNArmy(info.tplid());
                //if (tpl && !tpl->IsTrap()) {
                if (tpl) {
                    info.DieToWound(percent);
                }
            }
        }

        void ArmyList::ToWound(float percent)
        {
            for (auto it = m_armies.begin(); it != m_armies.end(); ++it) {
                ArmyInfo& info = it->second.armyInfo();
                const NArmyTpl* tpl = g_tploader->FindNArmy(info.tplid());
                //if (tpl && !tpl->IsTrap()) {
                if (tpl) {
                    info.ToWound(percent);
                }
            }
        }

        void ArmyList::ClearAllExceptNormal()
        {
            for (auto it = m_armies.begin(); it != m_armies.end(); ++it) {
                ArmyInfo& info = it->second.armyInfo();
                for (auto it2 = info.states().begin(); it2 != info.states().end(); ++it2) {
                    if (it2->first != (int)ArmyState::NORMAL) {
                        it2->second = 0;
                    }
                }
            }
        }

        void ArmyList::ClearAll()
        {
            m_armies.clear();
        }

        void ArmyList::RecoverToNormal()
        {
            for (auto it = m_armies.begin(); it != m_armies.end(); ++it) {
                ArmyInfo& info = it->second.armyInfo();
                for (auto it2 = info.states().begin(); it2 != info.states().end(); ++it2) {
                    if (it2->first == (int)ArmyState::RECOVER) {
                        info.Remove(model::ArmyState::RECOVER, it2->second);
                        info.Add(model::ArmyState::NORMAL, it2->second);
                    }
                }
            }
        }

        void ArmyList::ClearAllDie()
        {
            for (auto it = m_armies.begin(); it != m_armies.end(); ++it) {
                ArmyInfo& army = it->second.armyInfo();
                auto it2 = army.states().find((int)ArmyState::DIE);
                if (it2 != army.states().end()) {
                    it2->second = 0;
                }
            }
        }

        void ArmyList::AssignPosition()
        {
            //设置站位
            int positionAssigned[10] = {0};
            size_t assignedCount = 0;
            // 之前通过tpl_map_npc_army里面的hero的第四个数字设置的位置
            for (auto& army : m_armies) {
                ArmyGroup& armyGroup = army.second;
                if (armyGroup.position() != 0) {
                    positionAssigned[armyGroup.position()] = 1;
                }
            }

            for (size_t i = 0; i < 9; ++i) {
                for (auto & army : m_armies) {
                    ArmyGroup& armyGroup = army.second;
                    if (armyGroup.position() == 0) {
                        auto armyTpl = model::tpl::g_tploader->FindNArmy(armyGroup.armyInfo().type(), 
                            armyGroup.armyInfo().level());
                        if (armyTpl) {
                            int position = 0;
                            if (armyTpl->priority.size() > i) {
                                position = armyTpl->priority.at (i);
                                if (positionAssigned[position] == 0) {
                                    armyGroup.setPosition(position);
                                    positionAssigned[position] = armyGroup.heroInfo().tplId();
                                    ++assignedCount;
                                }
                            }
                        }
                    }
                }
                if (assignedCount >= m_armies.size()) {
                    break;
                }
            }
        }

        int ArmyList::Power()
        {
            int power = 0;
            for (auto it = m_armies.begin(); it != m_armies.end(); ++it) {
                auto& armyGroup = it->second;
                power +=  armyGroup.Power();
            }
            return power;
        }

        void ArmyList::OnPropertyUpdate(const info::Property& property)
        {
            for (auto it = m_armies.begin(); it != m_armies.end(); ++it) {
                ArmyInfo& army = it->second.armyInfo();

                HeroInfo& hero = it->second.heroInfo();
                army.OnPropertyUpdate(property);
                hero.OnPropertyUpdate(property, army.type(), army.level());
            }
        }

        void ArmyList::InitProp()
        {
            for (auto it = m_armies.begin(); it != m_armies.end(); ++it) {
                ArmyInfo& army = it->second.armyInfo();
                HeroInfo& hero = it->second.heroInfo();
                army.InitProp();
                hero.InipProp(army.type(), army.level());
            }
        }

        void ArmyList::SetDataTable(base::DataTable& dt, int specify, bool isFalse) const
        {
            dt.Set("team", m_team);
            DataTable armylist;
            for (auto it = armies().begin(); it != armies().end(); ++it) {
                const ArmyGroup& armyGroup = it->second;
                DataTable data;

                if (specify ==  0 ||  specify ==  1) {
                    DataTable hero;
                    hero.Set("tplId", armyGroup.heroInfo().tplId());
                    hero.Set("level", armyGroup.heroInfo().level());
                    hero.Set("star", armyGroup.heroInfo().star());
                    hero.Set("power", armyGroup.heroInfo().power());
                    hero.Set("physical", armyGroup.heroInfo().physical());
//                     DataTable equipment;
//                     int index = 0;
//                     for (auto & value : armyGroup.heroInfo().equipment()) {
//                         DataTable equipValue;
//                         equipValue.Set("tplId", value.id);
//                         equipValue.Set("level", value.level);
//                         equipment.Set(++index, equipValue);
//                     }
//                     hero.Set("equipment", equipment);
//
//                     DataTable skill;
//                     index = 0;
//                     for (auto & value : armyGroup.heroInfo().skill()) {
//                         DataTable skillValue;
//                         skillValue.Set("tplId", value.id);
//                         skillValue.Set("level", value.level);
//                         skill.Set(++index, skillValue);
//                     }
//                     hero.Set("skill", skill);

                    data.Set("hero", hero);
                }

                if (specify ==  0 ||  specify ==  2) {
                    DataTable army;
                    army.Set("armyType", armyGroup.armyInfo().type());
                    army.Set("level", armyGroup.armyInfo().level());
                    army.Set("position", armyGroup.position());
                    DataTable states;
                    for (auto it2 = armyGroup.armyInfo().states().begin(); it2 != armyGroup.armyInfo().states().end(); ++it2) {
                        if (it2->second > 0) {
                            if (!isFalse) {
                                states.Set(it2->first, it2->second);
                            } else {
                                states.Set(it2->first, it2->second * 2);
                            }
                        }
                    }
                    army.Set("state", states);

                    data.Set("army", army);
                }

                armylist.Set(it->first, data);
            }
            dt.Set("list", armylist);
        }

        void ArmyList::Serialize(rapidjson::Writer< rapidjson::StringBuffer >& writer) const
        {
            writer.StartObject();
            writer.Key("team");
            writer.Int(m_team);

            writer.Key("troopId");
            writer.Int(m_troopId);

            writer.Key("list");
            writer.StartArray();
            for (auto it = armies().begin(); it != armies().end(); ++it) {
                const ArmyGroup& armyGroup = it->second;
                writer.StartObject();

                writer.Key("position");
                writer.Int(armyGroup.position());

                writer.Key("heroId");
                writer.Int(armyGroup.heroInfo().tplId());

                writer.Key("hero");
                armyGroup.heroInfo().Serialize(writer);

                writer.Key("army");
                armyGroup.armyInfo().Serialize(writer);

                writer.EndObject();
            }
            writer.EndArray();
            writer.EndObject();
        }

        void ArmyList::Desrialize(rapidjson::GenericValue< rapidjson::UTF8< char > >& gv)
        {
            m_team = gv["team"].GetInt();
            m_troopId = gv["troopId"].GetInt();

            for (size_t i = 0; i < gv["list"].Size(); ++i) {
                ArmyGroup armyGroup;
                armyGroup.setPosition(gv["list"][i]["position"].GetInt());

                auto& hero = gv["list"][i]["hero"];
                armyGroup.heroInfo().Desrialize(hero);

                auto& army = gv["list"][i]["army"];
                armyGroup.armyInfo().Desrialize(army);

                AddArmyGroup(armyGroup);
                InitProp();
            }
        }
    }
}

