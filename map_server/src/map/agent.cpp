#include "agent.h"
#include "agentProxy.h"
#include "mapMgr.h"
#include "mapProxy.h"
#include "unit/unit.h"
#include <model/rpc/map.h>
#include <model/protocol.h>
#include <base/cluster/message.h>
#include <base/data.h>
#include <base/3rd/rapidjson/document.h>
#include <base/3rd/rapidjson/writer.h>
#include <base/3rd/rapidjson/stringbuffer.h>
#include <lua/llimits.h>
#include "unit/castle.h"
#include "unit/resnode.h"
#include "troop.h"
#include "unit/monster.h"
#include "unit/camp.h"
#include "unit/famouscity.h"
#include "unit/catapult.h"
#include "unit/palace.h"
#include "unit/worldboss.h"
#include <model/metadata.h>
#include <base/lua/parser.h>
#include "luawrite.h"
#include "msgqueue/msgrecord.h"
#include "info/agentinfo.h"
#include "alliance.h"
#include "tpl/mapunittpl.h"
#include "tpl/mapcitytpl.h"
#include "tpl/scouttypetpl.h"
#include "qo/commandagentsave.h"
#include "qo/commandunitsave.h"
#include <model/tpl/templateloader.h>
#include <model/tpl/configure.h>
#include <model/tpl/item.h>
#include <model/tpl/army.h>
#include <model/tpl/hero.h>
#include <algorithm>
#include <tuple>
#include <base/logger.h>
#include "base/event.h"
#include "battle/battlemgr.h"
#include "trapset.h"


namespace ms
{
    namespace map
    {
        using namespace std;
        using namespace base;
        using namespace base::cluster;
        using namespace model::rpc;
        using namespace model;
        using namespace battle;
        using namespace msgqueue;
        using namespace info;
        using namespace model::tpl;

        Agent::Agent(int64_t uid)
            : m_uid(uid), m_viewPoint(-1, -1), m_castlePos(-1, -1)
        {
            m_agentProxy = new AgentProxy(*this);
        }

        Agent::~Agent()
        {
        }

        string Agent::Serialize() const
        {
            string jsonString;
            try {
                rapidjson::StringBuffer jsonbuffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                writer.StartObject();

                writer.String("headId");
                writer.Int64(m_headId);
                writer.String("nickname");
                writer.String(m_nickname.c_str());
                writer.String("level");
                writer.Int(m_level);
                writer.String("exp");
                writer.Int(m_exp);
                writer.String("langType");
                writer.Int((int)m_langType);

                writer.Key("heroes");
                writer.StartArray();
                for (auto it = m_heroes.begin(); it != m_heroes.end(); ++it) {
                    if (it->second) {
                        const HeroInfo& heroInfo = *it->second;
                        heroInfo.Serialize(writer);
                    }
                }
                writer.EndArray();

                writer.Key("arrArmyList");
                writer.StartArray();
                for (auto& armyList : m_arrArmyList) {
                    armyList.Serialize(writer);
                }
                writer.EndArray();

                writer.Key("trapSets");
                m_trapSets.Serialize(writer);

                writer.Key("armySet");
                m_armySet.Serialize(writer);

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

                writer.String("attackWins");
                writer.Int(m_attackWins);
                writer.String("attackLosses");
                writer.Int(m_attackLosses);
                writer.String("defenseWins");
                writer.Int(m_defenseWins);
                writer.String("defenseLosses");
                writer.Int(m_defenseLosses);
                writer.String("scoutCount");
                writer.Int(m_scoutCount);
                writer.String("kills");
                writer.Int(m_kills);
                writer.String("losses");
                writer.Int(m_losses);
                writer.String("heals");
                writer.Int(m_heals);

                writer.String("lordPower");
                writer.Int(m_lordPower);
                writer.String("troopPower");
                writer.Int(m_troopPower);
                writer.String("buildingPower");
                writer.Int(m_buildingPower);
                writer.String("sciencePower");
                writer.Int(m_sciencePower);
                writer.String("trapPower");
                writer.Int(m_trapPower);
                writer.String("heroPower");
                writer.Int(m_heroPower);
                writer.String("totalPower");
                writer.Int(m_totalPower);

                writer.Key("watchtowerLevel");
                writer.Int(m_watchtowerLevel);
                writer.Key("turretLevel");
                writer.Int(m_turretLevel);
                writer.String("monsterDefeatedLevel");
                writer.Int(m_monsterDefeatedLevel);

                writer.String("lastLoginTimestamp");
                writer.Int64(m_lastLoginTimestamp);
                writer.String("registerTimestamp");
                writer.Int64(m_registerTimestamp);
                writer.String("allianceCdTimestamp");
                writer.Int64(m_allianceCdTimestamp);
                writer.String("useSkillHelp");
                writer.Bool(m_useSkillHelp);
                writer.String("titleId");
                writer.Int(m_titleId);
                writer.String("equipView");
                writer.Bool(m_equipView);

                writer.String("collectInfos");
                writer.StartArray();
                for (const CollectInfo & info : m_collectInfos) {
                    writer.StartArray();
                    writer.Int(info.gridId);
                    writer.Int((int)info.type);
                    writer.Int(info.output);
                    writer.Int(info.capacity);
                    writer.Int64(info.timestamp);
                    writer.EndArray();
                }
                writer.EndArray();

                writer.String("medals");
                writer.StartArray();
                for (size_t i = 0; i < m_medals.size(); ++i) {
                    writer.Int(m_medals[i]);
                }
                writer.EndArray();

                writer.String("cityBuffs");
                writer.StartArray();
                for (auto it = m_cityBuffs.begin(); it != m_cityBuffs.end(); ++it) {
                    const BuffInfo& info = it->second;
                    writer.StartArray();
                    writer.Int(it->first);
                    writer.Int(info.param1);
                    writer.Int64(info.endTimestamp);
                    writer.String(info.attr.Serialize().c_str());
                    writer.EndArray();
                }
                writer.EndArray();

                writer.String("skillBuffs");
                writer.StartArray();
                for (auto it = m_skillBuffs.begin(); it != m_skillBuffs.end(); ++it) {
                    const BuffInfo& info = it->second;
                    writer.StartArray();
                    writer.Int(it->first);
                    writer.Int(info.param1);
                    writer.Int64(info.endTimestamp);
                    writer.String(info.attr.Serialize().c_str());
                    writer.EndArray();
                }
                writer.EndArray();

                writer.String("technologies");
                writer.StartArray();
                for (auto it = m_technologies.begin(); it != m_technologies.end(); ++it) {
                    writer.StartArray();
                    writer.Int(it->first);
                    writer.Int(it->second);
                    writer.EndArray();
                }
                writer.EndArray();

                writer.String("skills");
                writer.StartArray();
                for (auto it = m_skills.begin(); it != m_skills.end(); ++it) {
                    writer.StartArray();
                    writer.Int(it->first);
                    writer.Int(it->second);
                    writer.EndArray();
                }
                writer.EndArray();

                writer.String("plunderCastleCount");
                writer.StartArray();
                for (auto it = m_plunderCastleCount.begin(); it != m_plunderCastleCount.end(); ++it) {
                    writer.StartArray();
                    writer.Int(it->first);
                    writer.Int(it->second);
                    writer.EndArray();
                }
                writer.EndArray();

                writer.String("plunderTotalCount");
                writer.Int(m_plunderTotalCount);

                writer.String("plunderResetTime");
                writer.Int64(m_plunderResetTime);

                // vip
                writer.String("vip");
                writer.StartObject();
                writer.String("level");
                writer.Int(m_vip.level);
                writer.EndObject();

                writer.String("allianceInvalidBuffs");
                writer.StartArray();
                for (auto it = m_allianceInvalidBuffs.begin(); it != m_allianceInvalidBuffs.end(); ++it) {
                    writer.StartArray();
                    writer.Int((int)it->first);
                    writer.Int64(it->second);
                    writer.EndArray();
                }
                writer.EndArray();

                writer.Key("properties");

                m_property.Serialize(writer);
                
                writer.Key("cityPatrol");
                writer.StartObject();
                writer.String("patrolCount");
                writer.Int(m_cityPatrol.patrolCount);

                writer.String("patrolCD");
                writer.StartArray();
                for (auto it = m_cityPatrol.patrolCD.begin(); it != m_cityPatrol.patrolCD.end(); ++it) {
                    writer.StartArray();
                    writer.Int(it->first);
                    writer.Int64(it->second);
                    writer.EndArray();
                }
                writer.EndArray();
                writer.String("cityPatrolResetTime");
                writer.Int64(m_cityPatrolResetTime);
                writer.String("castleDefenderId");
                writer.Int(m_castleDefenderId);
                writer.EndObject();

                writer.EndObject();
                jsonString = jsonbuffer.GetString();
            } catch (exception& ex) {
                LOG_ERROR("save Agent json fail: %s\n", ex.what());
            }
            return jsonString;
        }

        bool Agent::Deserialize(string data)
        {
            try {
                rapidjson::Document doc;
                doc.Parse<0>(data.c_str());
                if (doc.IsObject()) {

                    m_headId = doc["headId"].GetInt64();
                    m_nickname = doc["nickname"].GetString();
                    m_level = doc["level"].GetInt();
                    m_exp = doc["exp"].GetInt();
                    m_langType = static_cast<model::LangType>(doc["langType"].GetInt());

                    for (size_t i = 0; i < doc["heroes"].Size(); ++i) {
                        auto& hero = doc["heroes"][i];
                        std::shared_ptr<HeroInfo> heroInfo = std::make_shared<HeroInfo>();
                        heroInfo->Desrialize(hero);
                        m_heroes.emplace(heroInfo->tplId(), heroInfo);
                    }

                    auto& arrArmyList = doc["arrArmyList"];
                    for (size_t i = 0; i < arrArmyList.Size(); ++i) {
                        m_arrArmyList.at(i).Desrialize(arrArmyList[i]);
                    }

                    if (doc.HasMember("trapSets")) {
                        m_trapSets.Desrialize(doc["trapSets"]);
                    }
                    m_armySet.Desrialize(doc["armySet"]);

                    m_gold = doc["gold"].GetInt();
                    m_food = doc["food"].GetInt();
                    m_wood = doc["wood"].GetInt();
                    m_iron = doc["iron"].GetInt();
                    m_stone = doc["stone"].GetInt();

                    m_attackWins = doc["attackWins"].GetInt();
                    m_attackLosses = doc["attackLosses"].GetInt();
                    m_defenseWins = doc["defenseWins"].GetInt();
                    m_defenseLosses = doc["defenseLosses"].GetInt();
                    m_scoutCount = doc["scoutCount"].GetInt();
                    m_kills = doc["kills"].GetInt();
                    m_losses = doc["losses"].GetInt();
                    m_heals = doc["heals"].GetInt();

                    m_lordPower = doc["lordPower"].GetInt();
                    m_troopPower = doc["troopPower"].GetInt();
                    m_buildingPower = doc["buildingPower"].GetInt();
                    m_sciencePower = doc["sciencePower"].GetInt();
                    m_trapPower = doc["trapPower"].GetInt();
                    m_heroPower = doc["heroPower"].GetInt();
                    m_totalPower = doc["totalPower"].GetInt();

                    m_watchtowerLevel = doc["watchtowerLevel"].GetInt();
                    m_turretLevel = doc["turretLevel"].GetInt();
                    m_monsterDefeatedLevel = doc["monsterDefeatedLevel"].GetInt();

                    m_lastLoginTimestamp = doc["lastLoginTimestamp"].GetInt64();
                    m_registerTimestamp = doc["registerTimestamp"].GetInt64();
                    m_allianceCdTimestamp = doc["allianceCdTimestamp"].GetInt64();
                    m_useSkillHelp = doc["useSkillHelp"].GetBool();
                    m_titleId = doc["titleId"].GetInt();
                    m_equipView = doc["equipView"].GetBool();

                    if (doc["collectInfos"].IsArray()) {
                        auto& temp = doc["collectInfos"];
                        for (size_t i = 0; i < temp.Size(); ++i) {
                            CollectInfo ci;
                            ci.gridId = temp[i][0u].GetInt();
                            ci.type = static_cast<model::ResourceType>(temp[i][1].GetInt());
                            ci.output = temp[i][2].GetInt();
                            ci.capacity = temp[i][3].GetInt();
                            ci.timestamp = temp[i][4].GetInt64();
                            m_collectInfos.push_back(ci);
                        }
                    }

                    if (doc["medals"].IsArray()) {
                        auto& temp = doc["medals"];
                        for (size_t i = 0; i < temp.Size(); ++i) {
                            m_medals.push_back(temp[i].GetInt());
                        }
                    }

                    if (doc["cityBuffs"].IsArray()) {
                        auto& temp = doc["cityBuffs"];
                        for (size_t i = 0; i < temp.Size(); ++i) {
                            int id = temp[i][0u].GetInt();
                            BuffInfo bi;
                            bi.param1 = temp[i][1].GetInt();
                            bi.endTimestamp = temp[i][2].GetInt64();
                            if (temp[i].Size() >= 4) {
                                std::string strAttr = temp[i][3].GetString();
                                bi.attr = base::lua::Parser::ParseAsDataTable(strAttr);
                            }
                            m_cityBuffs.emplace(id, bi);
                        }
                    }

                    if (doc["skillBuffs"].IsArray()) {
                        auto& temp = doc["skillBuffs"];
                        for (size_t i = 0; i < temp.Size(); ++i) {
                            int id = temp[i][0u].GetInt();
                            BuffInfo bi;
                            bi.param1 = temp[i][1].GetInt();
                            bi.endTimestamp = temp[i][2].GetInt64();
                            if (temp[i].Size() >= 4) {
                                std::string strAttr = temp[i][3].GetString();
                                bi.attr = base::lua::Parser::ParseAsDataTable(strAttr);
                            }
                            m_skillBuffs.emplace(id, bi);
                        }
                    }

                    if (doc["technologies"].IsArray()) {
                        auto& temp = doc["technologies"];
                        for (size_t i = 0; i < temp.Size(); ++i) {
                            int tplid = temp[i][0u].GetInt();
                            int level = temp[i][1].GetInt();
                            m_technologies.emplace(tplid, level);
                        }
                    }

                    if (doc["skills"].IsArray()) {
                        auto& temp = doc["skills"];
                        for (size_t i = 0; i < temp.Size(); ++i) {
                            int tplid = temp[i][0u].GetInt();
                            int level = temp[i][1].GetInt();
                            m_skills.emplace(tplid, level);
                        }
                    }

                    if (doc["plunderCastleCount"].IsArray()) {
                        auto& temp = doc["plunderCastleCount"];
                        for (size_t i = 0; i < temp.Size(); ++i) {
                            int castleId = temp[i][0u].GetInt();
                            int count = temp[i][1].GetInt();
                            m_plunderCastleCount.emplace(castleId,  count);
                        }
                    }
                    m_plunderTotalCount = doc["plunderTotalCount"].GetInt();
                    m_plunderResetTime = doc.HasMember("plunderResetTime")  ? doc["plunderResetTime"].GetInt64() : 0;

                    if (doc["vip"].IsObject()) {
                        auto& temp = doc["vip"];
                        m_vip.level = temp["level"].GetInt();
                    }

                    if (doc.HasMember("allianceInvalidBuffs") && doc["allianceInvalidBuffs"].IsArray()) {
                        auto& temp = doc["allianceInvalidBuffs"];
                        for (size_t i = 0; i < temp.Size(); ++i) {
                            int type = temp[i][0u].GetInt();
                            int64_t endTimestamp = temp[i][1].GetInt64();
                            m_allianceInvalidBuffs.emplace(static_cast<model::AllianceBuffType>(type),  endTimestamp);
                        }
                    }

                    if (doc["properties"].IsObject()) {
                        auto& temp = doc["properties"];
                        m_property.Deserialize(doc);
                    }

                    if (doc.HasMember("cityPatrol") && doc["cityPatrol"].IsObject()) {
                        if (doc["cityPatrol"]["patrolCD"].IsArray()) {
                            auto& temp = doc["cityPatrol"]["patrolCD"];
                            for (size_t i = 0; i < temp.Size(); ++i) {
                                int id = temp[i][0u].GetInt();
                                int64_t endTime = temp[i][1].GetInt64();
                                m_cityPatrol.patrolCD.emplace(id,  endTime);
                            }
                        }
                        m_cityPatrol.patrolCount = doc["cityPatrol"]["patrolCount"].GetInt();
                        m_cityPatrolResetTime = doc["cityPatrol"]["cityPatrolResetTime"].GetInt64();
                    }
                    if (doc.HasMember("castleDefenderId"))
                    {
                        m_castleDefenderId = doc["castleDefenderId"].GetInt();
                    }
                    // ****** 每次增加一个新的字段的时候，先用hasmember判断一下，要不然就会出错，以免清服
                }
                return true;
            } catch (exception& ex) {
                LOG_ERROR("json string to Agent fail: %s\n", ex.what());
                return false;
            }
        }

        void Agent::FinishDeserialize()
        {
            //m_castle m_troops
            if (isLocalPlayer()) {
//                 m_castle = g_mapMgr->FindCastleByUid(m_uid);
                m_castle = g_mapMgr->FindCastleByPos(castlePos());
                if (!m_castle) {
                    LOG_ERROR("Agent::FinishDeserialize castle not found! uid = %ld", m_uid);
                }
                for (auto it = g_mapMgr->m_troops.begin(); it != g_mapMgr->m_troops.end(); ++it) {
                    Troop* troop = it->second;
                    if (troop->uid() == m_uid) {
                        m_troops.emplace(troop->id(), troop);
                    }
                }

                for (auto& armyList :  m_arrArmyList) {
                    if (armyList.team() ==  0) {
                        continue;
                    }
                    armyList.OnPropertyUpdate(m_property);
                }

                CheckResetTimer();
            }
        }

        bool Agent::isLocalPlayer() const
        {
            return g_mapMgr->isLocalUid(m_uid);
        }

        int Agent::castleLevel() const
        {
            return m_castle ? m_castle->level() : 0;
        }

        //abandon
        ArmyList* Agent::defArmyList()
        {
            // return &m_defArmyList;  UpdateDefArmyList
            // Todo: 已废弃 
            return nullptr;
        }

        int Agent::getDefTeam() const
        {
            return m_castleDefenderId;
        }

        int Agent::TroopCount() {
            int i = 0;
            for (auto it = m_troops.begin(); it != m_troops.end(); ++it) {
                it->second->OnPropertyChanged();
                if (it->second->type() != MapTroopType::TRANSPORT && it->second->type() != MapTroopType::SCOUT)
                    i++;
            }
            return i;
        }

        int Agent::TroopCountByType(MapTroopType type)
        {   
            int i = 0;
            for (auto it = m_troops.begin(); it != m_troops.end(); ++it) {
                if (it->second->type() == type)
                    i++;
            }
            return i;
        }

        bool Agent::isKing() const
        {
            if (const Agent* king = g_mapMgr->king()) {
                return king->uid() == m_uid;
            }
            return false;
        }

        bool Agent::isVip() const
        {
            return m_vip.level > 0;
        }

        int64_t Agent::allianceId() const
        {
            if (m_alliance) {
                return m_alliance->info.id;
            }
            return 0;
        }

        const string& Agent::allianceName() const
        {
            if (m_alliance) {
                return m_alliance->info.name;
            }
            return m_nullstr;
        }

        int Agent::allianceLevel() const
        {
            if (m_alliance) {
                return m_alliance->info.level;
            }
            return 0;
        }

        const string& Agent::allianceNickname() const
        {
            if (m_alliance) {
                return m_alliance->info.nickname;
            }
            return m_nullstr;
        }

        int Agent::allianceBannerId() const
        {
            if (m_alliance) {
                return m_alliance->info.bannerId;
            }
            return 0;
        }

        void Agent::UpdateTitleId(int titleId)
        {
            m_titleId = titleId;
            SetDirty();
            if (m_castle) {
                m_castle->NoticeUnitUpdate();
            }
        }

        void Agent::AddResource(const vector< DropItem >& drops)
        {
            if (drops.empty()) {
                return;
            }
            for (const DropItem & drop : drops) {
                if (const PropTpl* tpl = drop.tpl.ToPropTpl()) {
                    switch (tpl->propType) {
                        case ItemPropType::FOOD:
                            m_food += drop.count;
                            break;
                        case ItemPropType::WOOD:
                            m_wood += drop.count;
                            break;
                        case ItemPropType::IRON:
                            m_iron += drop.count;
                            break;
                        case ItemPropType::STONE:
                            m_stone += drop.count;
                            break;
                        case ItemPropType::GOLD:
                            m_gold += drop.count;
                            break;
                        default:
                            break;
                    }
                }
            }
            SetDirty();
        }

        bool Agent::IsActiveUser() const
        {
            if (isPlayer()) {
                int64_t time = g_dispatcher->GetTimestampCache() - m_lastLoginTimestamp;
                return time >= 0 && time < g_tploader->configure().activeUserTime * 86400;
            }
            return false;
        }

        int Agent::GetCollectTotal(ResourceType type)
        {
            int total = 0;
            int64_t now = g_dispatcher->GetTimestampCache();
            for (const CollectInfo & info : m_collectInfos) {
                if (info.type == type && now > info.timestamp) {
                    int output = (float)info.output * (now - info.timestamp) / 3600;
                    if (output > info.capacity) {
                        output = info.capacity;
                    } else if (output < 0) {
                        output = 0;
                        LOG_ERROR("Agent::GetCollectTotal output < 0, output=%d, now=%ld, info.timestamp=%ld", info.output, now, info.timestamp);
                    }
                    total += output;
                }
            }
            return total;
        }

        int Agent::ResourceBePlundered(ResourceType type, int innerResource,  int outerResource)
        {
            if (outerResource > 0) {
                int64_t now = g_dispatcher->GetTimestampCache();
                for (CollectInfo & info : m_collectInfos) {
                    if (info.type == type && now > info.timestamp) {
                        int output = (float)info.output * (now - info.timestamp) / 3600;
                        if (output > info.capacity) {
                            output = info.capacity;
                        }

                        if (outerResource >= output) {
                            outerResource -= output;
                            info.timestamp = now;
                        } else {
                            info.timestamp = now - (int64_t)((double)outerResource * 3600 / info.output);
                            if (now < info.timestamp) {
                                LOG_ERROR("Agent::ResourceBeRansacked now < timestamp count=%d, output=%d, now=%ld, info.timestamp=%ld", outerResource, info.output, now, info.timestamp);
                            }
                        }
                    }
                }
            }

            if (innerResource > 0) {
                if (type == ResourceType::FOOD) {
                    m_food -= innerResource;
                    if (m_food < 0) m_food = 0;
                } else if (type == ResourceType::WOOD) {
                    m_wood -= innerResource;
                    if (m_wood < 0) m_wood = 0;
                } else if (type == ResourceType::IRON) {
                    m_iron -= innerResource;
                    if (m_iron < 0) m_iron = 0;
                } else if (type == ResourceType::STONE) {
                    m_stone -= innerResource;
                    if (m_stone < 0) m_stone = 0;
                }
            }
            SetDirty();
            return innerResource;
        }

        void Agent::OnHour(int hour)
        {
            CheckResetTimer();
        }

        void Agent::GetCollectOutput(int& food, int& wood, int& iron, int& stone)
        {
            int64_t now = g_dispatcher->GetTimestampCache();
            for (CollectInfo & info : m_collectInfos) {
                int count = info.output * (now - info.timestamp);
                if (count > info.capacity) {
                    count = info.capacity;
                } else if (count < 0) {
                    count = 0;
                    LOG_ERROR("Agent::GetCollectOutput count < 0, output=%d, now = %ld, info.timestamp = %ld", info.output, now, info.timestamp);
                }
                if (info.type == ResourceType::FOOD) {
                    food += count;
                } else if (info.type == ResourceType::WOOD) {
                    wood += count;
                } else if (info.type == ResourceType::IRON) {
                    iron += count;
                } else if (info.type == ResourceType::STONE) {
                    stone += count;
                }
            }
        }

        bool Agent::CheckBuffIsValid(model::BuffType type, bool isCityBuff)
        {
            auto it = m_cityBuffs.find((int)type);
            if (it != m_cityBuffs.end()) {
                int64_t now = g_dispatcher->GetTimestampCache();
                BuffInfo& info = it->second;
                if (now < info.endTimestamp) {
                    return true;
                } else {
                    //handle param1
                    if (info.param1 > 0) {
                        info.param1 = 0;
                    }
                }
            }
            return false;
        }


        void Agent::SetMonsterDefeatedLevel(int level)
        {
                m_monsterDefeatedLevel = level;
                m_agentProxy->SendCanMarchMonsterLevelUpdate();
        }

        void Agent::GetArmyAddition(std::vector<std::tuple<int, int, int>>& adds) const
        {
            auto addProperty = [&](int t, int base, int pct, int ext) {
                if (base != 0) {
                    adds.emplace_back(std::make_tuple(t, (int)model::AttributeAdditionType::BASE, base));
                }
                if (pct != 0) {
                    adds.emplace_back(std::make_tuple(t, (int)model::AttributeAdditionType::PCT, pct));
                }
                if (ext != 0) {
                    adds.emplace_back(std::make_tuple(t, (int)model::AttributeAdditionType::EXT, ext));
                }
            };

            addProperty((int)model::AttributeType::TROOPS_ATTACK, 0, m_property.troopAttackPct*10000, m_property.troopAttackExt);
            addProperty((int)model::AttributeType::TROOPS_DEFENSE, 0, m_property.troopDefensePct*10000, m_property.troopDefenseExt);
            addProperty((int)model::AttributeType::TROOPS_HEALTH, 0, m_property.troopHpPct*10000, m_property.troopHpExt);
            addProperty((int)model::AttributeType::TROOPS_SPEED, 0, m_property.troopSpeedPct*10000, m_property.troopSpeedExt);
            addProperty((int)model::AttributeType::TROOPS_MORALE, 0, 0, m_property.troopMorale);
            addProperty((int)model::AttributeType::TROOPS_ATTACK_CITY, 0, m_property.troopAttackCityPct*10000, m_property.troopAttackCityExt);

            addProperty((int)model::AttributeType::ATTACK_MONSTER_MARCH_SPEED, 0, m_property.attackMonsterSpeedPct*10000, 0);
            addProperty((int)model::AttributeType::MARCH_SPEED, 0, m_property.marchSpeedPct*10000, 0);
            addProperty((int)model::AttributeType::SCOUT_SPEED, 0, m_property.scoutSpeedPct*10000, 0);
            addProperty((int)model::AttributeType::TROOPS_LOAD, 0, m_property.troopLoadPct*10000, 0);
            addProperty((int)model::AttributeType::ROB_RESOURCE_MAX, 0, m_property.robResourcePct*10000, 0);
        }

        void Agent::AfterCastleBeAttacked()
        {
        }

        void Agent::OnTroopArriveCastle(Troop* troop)
        {
            //cout << "Agent::OnTroopArriveHome" << endl;
            m_msgQueue.AppendMsgMarchBack(true, troop->type(), troop->from(), troop->food(), troop->wood(), troop->iron(), troop->stone(), troop->armyList());
            m_food += troop->food();
            m_wood += troop->wood();
            m_iron += troop->iron();
            m_stone += troop->stone();

            if (/*troop->type() == MapTroopType::GATHER &&*/ !troop->dropItems().empty()) {
                AddResource(troop->dropItems());
                m_msgQueue.AppendMsgGatherResource(troop->from(), troop->reachTplId(), troop->dropItems(),  troop->gatherRemain());
            }

            // push notification
            g_mapMgr->m_mapProxy->SendcsAppendPush(m_uid, PushClassify::TROOP_BACK, 0, 0, g_dispatcher->GetTimestampCache());

            int teamId = troop->teamId();
            if (teamId > 0 && teamId <= 5) {
                m_arrArmyList.at(teamId - 1).setTroopId(0);
                /*m_arrArmyList.at(teamId - 1).SetTeam(0);
                m_arrArmyList.at(teamId - 1).ClearAll();*/
            }

            SetDirty();
        }

        void Agent::OnTroopArriveCampFixed(Troop* troop)
        {
            m_msgQueue.AppendMsgMarchBack(false, troop->type(), troop->from(), troop->food(), troop->wood(), troop->iron(), troop->stone(), troop->armyList());
        }

//         void Agent::OnMonsterBattleEnd(model::AttackType winner)
//         {
//             //cout << "Agent::OnMonsterBattleEnd" << endl;
// //             m_msgQueue.AppendMsgAttackMonster(battle->winner(), battle->unit()->pos(), battle->unit()->tpl().id, battle->monsterHpBegin(), battle->monsterHpEnd(), battle->dropItems(), battle->troop()->armyList(), false, battle->unit()->id(), battle->troop()->id());
//         }
//
//         void Agent::OnCampBattleEnd(model::AttackType winner, AttackType myAttackType)
//         {
//             //cout << "Agent::OnCampBattleEnd" << endl;
//             AddBattleCount(myAttackType, winner);
// //             m_msgQueue.AppendMsgAttackCamp(battle->winner(), myAttackType, battle->attackerInfo(), battle->defenderInfo(), battle->unit()->id(), battle->troop()->id());
// //             if (myAttackType == AttackType::DEFENSE) {
// //                 g_mapMgr->m_mapProxy->SendcsAppendPush(m_uid, PushClassify::CAMP_ATTACTED, battle->attackerInfo().nickname, 0, g_dispatcher->GetTimestampCache());
// //             }
//         }

        void Agent::OnScout(AttackType winner, AttackType myAttackType, const std::vector< model::WATCHTOWER_SCOUT_TYPE >& scoutTypes, const MsgPlayerInfo& attacker, const MsgScoutDefenderInfo& defender)
        {
            //cout << "Agent::OnScout" << endl;
            if (myAttackType == AttackType::ATTACK) {
                m_scoutCount += 1;
            } else {
                g_mapMgr->m_mapProxy->SendcsAppendPush(m_uid, PushClassify::SCOUTED, attacker.nickname, 0, g_dispatcher->GetTimestampCache());
            }
            msgQueue().AppendMsgScout(winner, myAttackType, scoutTypes, attacker, defender);
        }

//         void Agent::OnResourceBattleEnd(model::AttackType winner, AttackType myAttackType)
//         {
//             //cout << "Agent::OnResourceBattleEnd" << endl;
//             AddBattleCount(myAttackType, winner);
//         }
//
//         void Agent::OnCastleBattleEnd(model::AttackType winner, model::AttackType myAttackType)
//         {
//             //cout << "Agent::OnCastleBattleEnd" << endl;
//             AddBattleCount(myAttackType, winner);
//         }

        void Agent::OnRemovePeaceShield()
        {
            //cout << "Agent::OnRemovePeaceShield" << endl;
            if (isPlayer() && m_castle && m_castle->IsProtected()) {
                m_castle->RemovePeaceShield();
                auto it = m_cityBuffs.find((int)BuffType::PEACE_SHIELD);
                if (it != m_cityBuffs.end()) {
                    BuffInfo& info = it->second;
                    info.endTimestamp = 0;
                    info.param1 = 0;
                    m_msgQueue.AppendMsgBuffRemove(BuffType::PEACE_SHIELD);
                }
            }
        }

        void Agent::OnResourceHelp(Troop& troop, const Agent& recipient)
        {
            cout << "Agent::OnResourceHelp" << endl;

            MsgPlayerInfo rpt;
            rpt.SetData(recipient);

            m_msgQueue.AppendMsgResourceHelp(rpt, AttackType::ATTACK, troop.food(), troop.wood(), troop.iron(), troop.stone());
        }

        void Agent::OnResouceHelped(Troop& troop)
        {
            cout << "Agent::OnResouceHelped food = " << troop.food() << " wood = " << troop.wood() << " iron = " << troop.iron() << " stone = " << troop.stone() << endl;

            m_food += troop.food();
            m_wood += troop.wood();
            m_iron += troop.iron();
            m_stone += troop.stone();
            SetDirty();

            MsgPlayerInfo helper;
            helper.SetData(troop.agent());

            m_msgQueue.AppendMsgResourceHelp(helper, AttackType::DEFENSE, troop.food(), troop.wood(), troop.iron(), troop.stone());
        }

        void Agent::OnReinforcements(Troop& troop, const Agent& recipient)
        {
            cout << "Agent::OnReinforcements" << endl;
            if (troop.armyList()) {
                MsgPlayerInfo rpt;
                rpt.SetData(recipient);
                m_msgQueue.AppendMsgReinforcements(rpt, AttackType::ATTACK, *troop.armyList());
            }
        }

        void Agent::OnMarch(Troop* troop, bool isRemarch)
        {
            if (!isRemarch) {
                int teamId = troop->teamId();
                if (teamId > 0 && teamId <= 5) {
                    m_arrArmyList.at(teamId - 1).setTroopId(troop->id());
                }

                SetDirty();
                m_troops.emplace(troop->id(), troop);
            }
            m_msgQueue.AppendMsgMarch(troop->type(), troop->to(), troop->armyList(), troop->id());
            //m_armyList->Debug();
        }

        void Agent::ImmediateReturnAllTroop()
        {
            while (!m_troops.empty()) {
                std::vector<Troop*> allTroops;
                for (auto troop : m_troops) {
                    allTroops.push_back(troop.second);
                }

                for (Troop* troop : allTroops) {
                    if (troop) {
                        if (troop->type() == MapTroopType::CAMP_FIXED && troop->IsMarching()) {
                            msgQueue().AppendMsgCreateCampFixedFailed();
                        } 
                        troop->ImmediateReturn();
                    }
                }
            }
        }

        //alliance buff
        void Agent::AllianceBuffOpen(const info::AllianceBuffInfo& buffInfo)
        {
            bool needAdd = true;
            auto it = m_allianceInvalidBuffs.find(buffInfo.type);
            if (it !=  m_allianceInvalidBuffs.end()) {
                if (buffInfo.endTimestamp == it->second) {
                    needAdd = false;
                } else {
                    m_allianceInvalidBuffs.erase(it);
                }
            }

            if (needAdd) {
                if (buffInfo.type ==  model::AllianceBuffType::PEACE_SHIELD) {
                    m_castle->CheckPeaceShield();
                } else if (buffInfo.type ==  model::AllianceBuffType::CITY_WALL_RECOVER) {
                    int cityDefenseMax = m_castle->cityDefenseMax();
                    m_castle->AddCityDefense(buffInfo.param1/10000.0 * cityDefenseMax);
                    m_castle->NoticeUnitUpdate();
                }
            }
        }

        void Agent::AllianceBuffClosed(model::AllianceBuffType buffType)
        {
            m_allianceInvalidBuffs.erase(buffType);
        }

        void Agent::DisableAllianceBuff(model::AllianceBuffType buffType)
        {
            int64_t endTimestamp = 0;
            for (auto it = m_alliance->allianceBuffs.begin(); it != m_alliance->allianceBuffs.end(); ++it) {
                auto& buff = it->second;
                if (buffType ==  buff.type) {
                    endTimestamp = buff.endTimestamp;
                    break;
                }
            }
            if (endTimestamp > 0) {
                m_allianceInvalidBuffs.emplace(buffType, endTimestamp);
            }
        }

        const info::AllianceBuffInfo* Agent::GetValidAllianceBuff(model::AllianceBuffType buffType)
        {
            const info::AllianceBuffInfo* buffInfo = nullptr;
            auto isValidType = [&](model::AllianceBuffType buffType) {
                auto it = m_allianceInvalidBuffs.find(buffType);
                return it == m_allianceInvalidBuffs.end();
            };

            if (m_alliance) {
                if (isValidType(buffType)) {
                    for (auto it = m_alliance->allianceBuffs.begin(); it != m_alliance->allianceBuffs.end(); ++it) {
                        auto& buff = it->second;
                        if (buff.type ==  buffType) {
                            buffInfo = &buff;
                        }
                        break;
                    }
                }
            }

            return buffInfo;
        }

        void Agent::OnAllianceInfoUpdate(const AllianceSimple* alliance, bool needBroadTroops)
        {
            auto oldAlliance = m_alliance;
            m_alliance = alliance;

            if (!oldAlliance && alliance) {
                SendRelatedTroops();
                SendRelatedUnits();
            }

            //通知城池
            if (m_castle) {
                m_castle->NoticeUnitUpdate();
            }
            //行军UPDATE
            if (needBroadTroops) {
                for (auto it = m_troops.begin(); it != m_troops.end(); ++it) {
                    Troop* tp = it->second;
                    tp->NoticeTroopUpdate();
                }
            }
        }

        void Agent::OnAllianceInfoReset(const AllianceSimple* oldAlliance)
        {
            m_alliance = nullptr;

            if (m_castle) {
                m_castle->NoticeUnitUpdate();
            }

            for (auto it = m_troops.begin(); it != m_troops.end(); ++it) {
                Troop* tp = it->second;
                if (tp->type() ==  MapTroopType::SIEGE_CITY && tp->state() == MapTroopState::REACH) {
                    tp->GoHome();
                } else {
                    tp->NoticeTroopUpdate();
                }
            }

            m_cityPatrol.patrolCount = 0;
            m_cityPatrol.patrolCD.clear();
        }

        void Agent::OnAllianceLoseCity(int id)
        {
            m_cityPatrol.patrolCD.erase(id);
        }

        void Agent::OnCastleRebuild()
        {
            //cout << "OnCastleRebuild" << endl;
            m_msgQueue.AppendMsgCastleRebuild(m_castlePos);
        }

        void Agent::OnPlunderCastle(int castleId)
        {
            auto it = m_plunderCastleCount.find(castleId);
            if (it !=  m_plunderCastleCount.end()) {
                ++it->second;
            } else {
                m_plunderCastleCount.emplace(castleId,  1);
            }

            ++m_plunderTotalCount;
        }

        int Agent::GetPlunderCount(int castleId)
        {
            auto it = m_plunderCastleCount.find(castleId);
            if (it !=  m_plunderCastleCount.end()) {
                return it->second;
            }
            return 0;
        }

        void Agent::AddCampFixed(CampFixed* campFixed)
        {
            m_campFixed.push_back(campFixed);
        }

        void Agent::RemoveCampFixed(CampFixed* campFixed)
        {
            m_campFixed.remove(campFixed);
        }

        void Agent::AddCampTemp(CampTemp* campTemp)
        {
            m_campTemp.push_back(campTemp);
        }

        void Agent::RemoveCampTemp(CampTemp* campTemp)
        {
            m_campTemp.remove(campTemp);
        }

        void Agent::AddRelatedUnit(Unit* unit)
        {
            m_relatedUnit.push_back(unit);
        }

        void Agent::RemoveRelatedUnit(Unit* unit)
        {
            m_relatedUnit.remove(unit);
        }

        bool Agent::IsMarchingHero(int heroId)
        {
            for (auto& armyList : m_arrArmyList) {
                if (armyList.IsOut()) {
                    if (nullptr != armyList.GetArmyGroup(heroId)) {
                        return true;
                    }
                }
            }
            return false;
        }

        void Agent::OnBattleHistoryUpdate(BattleInfo* info)
        {
            //cout << "Agent::OnBattleHistoryUpdate" << endl;
            if (!info) {
                return;
            }
            std::list <BattleInfo*> bList;
            bList.push_back(info);
            // m_agentProxy->SendBattleHistoryList(bList);
        }

        void Agent::AddBattleCount(AttackType myAttackType, AttackType winner)
        {
            if (myAttackType == AttackType::ATTACK) {
                if (winner == AttackType::ATTACK) {
                    m_attackWins += 1;
                } else {
                    m_attackLosses += 1;
                }
            } else {
                if (winner == AttackType::ATTACK) {
                    m_defenseLosses += 1;
                } else {
                    m_defenseWins += 1;
                }
            }
        }

        void Agent::OnBeAttackedByCatapult(const ArmyList& dieList, bool isCaptive)
        {
            //cout << "Agent::OnBeAttackedByCatapult" << endl;
            m_msgQueue.AppendMsgBeAttackedByCatapult(dieList, isCaptive);
        }
        
        bool Agent::IsMyPos(const Point& pos)
        {
            if (m_castle && pos == m_castle->pos()) {
                return true;
            }
            for (auto it = m_troops.begin(); it != m_troops.end(); ++it) {
                Troop* tp = it->second;
                if (tp->IsReach() && (tp->type() == MapTroopType::CAMP_TEMP || tp->type() == MapTroopType::CAMP_FIXED || tp->type() == MapTroopType::GATHER)) {
                    if (tp->to() == pos) {
                        return true;
                    }
                }
            }
            return false;
        }

        void Agent::SendRelatedTroops()
        {
//             int64_t before = base::utils::nowtick();

            //update all troops
            std::vector<Troop*> troops;
            for (auto it = g_mapMgr->m_troops.begin(); it != g_mapMgr->m_troops.end(); ++it) {
                if (NeedSend(it->second)) {
                    troops.push_back(it->second);
                }
            }

            m_agentProxy->SendMapTroopUpdate(troops, true);

//             int64_t after = base::utils::nowtick();
//
//             std::cout << "---------------->SendRelatedTroops time = " << after - before <<  std::endl;
        }

        void Agent::SendRelatedUnits()
        {
            std::vector<Unit*> units;
            // 玩家行营
            units.insert(units.begin(), m_campFixed.begin(),  m_campFixed.end());

            // 同盟玩家
            if(m_alliance) {
                auto& members = m_alliance->members;
                for (auto uid : members) {
                    Agent* agent = g_mapMgr->FindAgentByUID(uid);
                    if (agent) {
                        auto castle = g_mapMgr->FindCastleByPos(agent->castlePos());
                        if (castle) {
                            units.emplace_back(castle);
                        }
                    }
                }
            }

            // 名城
            auto& cities = g_mapMgr->cities();
            for (auto city :  cities) {
                if (!city->IsCounty()) {
                    units.emplace_back(city);
                }
            }

            m_agentProxy->SendMapUnitUpdate(units,  std::vector<int>(),  true);
        }

        bool Agent::IsFalseArmy() const
        {
            return false;
        }

        unordered_map< int, BuffInfo > Agent::GetValidBuffs(int buffType, bool isWatchTower) const
        {
            unordered_map<int, BuffInfo> buffs;
            if (buffType ==  1 ||  buffType ==  3) {
                for (auto it = m_cityBuffs.begin(); it != m_cityBuffs.end(); ++it) {
                    const BuffInfo& info = it->second;
                    if (info.endTimestamp > g_dispatcher->GetTimestampCache()) {
                        if (isWatchTower) {
                            BuffType type = (BuffType)it->first;
                            if (type != BuffType::ATTACK_BONUS_PER && type != BuffType::DEFENSE_BONUS_PER) {
                                continue;
                            }
                        }
                        buffs.emplace(it->first, info);
                    }
                }
            }

            if (buffType ==  2 ||  buffType ==  3) {
                for (auto it = m_skillBuffs.begin(); it != m_skillBuffs.end(); ++it) {
                    const BuffInfo& info = it->second;
                    if (info.endTimestamp > g_dispatcher->GetTimestampCache()) {
                        if (isWatchTower) {
                            BuffType type = (BuffType)it->first;
                            if (type != BuffType::ATTACK_BONUS_PER && type != BuffType::DEFENSE_BONUS_PER) {
                                continue;
                            }
                        }
                        auto it2 = buffs.find(it->first);
                        if (it2 != buffs.end()) {
                            it2->second.param1 += info.param1;
                        } else {
                            buffs.emplace(it->first, info);
                        }
                    }
                }
            }
            return buffs;
        }

        bool Agent::NeedSend(Troop* troop)
        {
            bool isNeed = false;
            do {
//                 //行军类型过滤
//                 if (troop->type() == MapTroopType::MONSTER) {
//                     break;
//                 }

                // 无法看到其它玩家的侦察部队
                if (troop->type() ==  MapTroopType::SCOUT && troop->agent().uid() != this->uid())
                {
                    break;
                }

                if (troop->agent().uid() == this->uid()) {
                    isNeed = true;
                    break;
                }

                if (this->allianceId() > 0 && troop->agent().allianceId() == this->allianceId()) {
                    isNeed = true;
                    break;
                }

                if (m_tidCache.find(troop->id()) != m_tidCache.end()) {
                    isNeed = true;
                    break;
                }

                if (isViewing() && g_mapMgr->IsViewingTroop(troop->from(), troop->to(), this->viewPoint())) {
                    isNeed = true;
                    break;
                }

                // 被攻城和被同盟玩家增援
                if (troop->type() ==  MapTroopType::SIEGE_CASTLE || troop->type() ==  MapTroopType::REINFORCEMENTS) {
                    auto toUnit = g_mapMgr->FindUnit(troop->to());
                    if (toUnit !=  nullptr) {
                        if (m_castle !=  nullptr) {
                            if (toUnit->id() ==  m_castle->id()) {
                                isNeed = true;
                                break;
                            }
                        }

                        if (this->allianceId() > 0 && toUnit->allianceId() == this->allianceId()) {
                            isNeed = true;
                            break;
                        }
                    }
                }
            } while (0);

            return isNeed;
        }

        void Agent::AttachTroopEvent()
        {
            auto troopEventHander = [this](Troop * troop, MapTroopEvent event) {
                if (troop == nullptr) {
                    return;
                }
                switch (event) {
                    case MapTroopEvent::ADD:
                        if (NeedSend(troop)) {
                            m_agentProxy->SendMapTroopUpdate(troop);
                        }
                        break;
                    case MapTroopEvent::UPDATE:
                        if (NeedSend(troop)) {
                            m_agentProxy->SendMapTroopUpdate(troop);
                        }
                        break;
                    case MapTroopEvent::REMOVE:
                        if (NeedSend(troop)) {
                            m_agentProxy->SendMapTroopRemove(troop->id());
                        }
                        break;
                    default:
                        break;
                }
            };
            m_troopObserver.ReCreate();
            g_mapMgr->evt_troop.Attach(troopEventHander, m_troopObserver) ;
        }

        void Agent::DetachTroopEvent()
        {
            m_troopObserver.SetNotExist();
        }

        bool Agent::NeedSend(Unit* unit)
        {
            if (!unit) {
                return false;
            }

            bool isNeed = false;
            do {
                if (m_unitIdCache.find(unit->id()) != m_unitIdCache.end()) {
                    isNeed = true;
                    break;
                }

                if (isViewing() && g_mapMgr->IsViewingUnit(unit->pos(), this->viewPoint())) {
                    isNeed = true;
                    break;
                }

                if (auto camp = unit->ToCampFixed()) {
                    if (camp->uid() ==  uid()) {
                        isNeed = true;
                        break;
                    }
                }

                if (auto camp = unit->ToCampTemp()) {
                    if (camp->uid() ==  uid()) {
                        isNeed = true;
                        break;
                    }
                }

                if (auto castle = unit->ToCastle()) {
                    if (castle->uid() ==  uid() ||  (allianceId() > 0 && castle->allianceId() ==  allianceId())) {
                        isNeed = true;
                        break;
                    }
                }
            } while (0);

            return isNeed;
        }

        void Agent::AttachUnitEvent()
        {
            auto unitEventHander = [this](Unit * unit, MapUnitEvent event) {
                if (unit == nullptr) {
                    return;
                }
                switch (event) {
                    case MapUnitEvent::ADD:
                        if (NeedSend(unit)) {
                            m_agentProxy->SendMapUnitUpdate(unit);
                        }
                        break;
                    case MapUnitEvent::UPDATE:
                        if (NeedSend(unit)) {
                            m_agentProxy->SendMapUnitUpdate(unit);
                        }
                        break;
                    case MapUnitEvent::REMOVE:
                        if (NeedSend(unit)) {
                            m_agentProxy->SendMapUnitRemove(unit->id());
                        }
                        break;
                    default:
                        break;
                }
            };
            m_unitObserver.ReCreate();
            g_mapMgr->evt_unit.Attach(unitEventHander, m_unitObserver) ;
        }

        void Agent::DetachUnitEvent()
        {
            m_unitObserver.SetNotExist();
        }

        void Agent::CheckResetTimer()
        {
            auto now = g_dispatcher->GetTimestampCache(); //base::utils::timestamp();
            if (base::utils::is_ts_should_refresh_at_hour(m_plunderResetTime,  5)) {
                m_plunderCastleCount.clear();
                m_plunderTotalCount = 0;
                m_plunderResetTime = now;
            }

            if (base::utils::is_ts_should_refresh_at_hour(m_cityPatrolResetTime,  0)) {
                m_cityPatrol.patrolCount = 0;
                m_cityPatrol.patrolCD.clear();
                m_cityPatrolResetTime = now;
            }
        }

        void Agent::ReSetMapSearch()
        {
            m_mapSearchTime = 0;
            m_mapSearchDis = 0;
            m_mapSearchType = 0;
            m_mapSearchLevel = 0;
        }
    }
}








