#include "monster.h"
#include "../troop.h"
#include <model/tpl/templateloader.h>
#include <model/tpl/army.h>
#include <base/logger.h>

namespace ms
{
    namespace map
    {
        using namespace model::tpl;
        Monster::Monster(int id, const MapUnitMonsterTpl* tpl, const Point& bornPoint)
            : Unit(id, tpl), m_tpl(tpl), m_bornPoint(bornPoint), m_direction(0), m_level(tpl->level)
        {
        }

        Monster::~Monster()
        {
        }

        void Monster::Init()
        {
            m_armyList = tpl::m_tploader->GetRandomArmyList(m_tpl->armyGroup, m_tpl->armyCount);
            SetDirty();
        }

        bool Monster::IsDie() const
        {
            return m_armyList.IsAllDie();
        }

        void Monster::SetTpl(const MapUnitTpl* tpl)
        {
            if (tpl && tpl->IsMonster()) {
                Unit::SetTpl(tpl);
                Init();
            }
        }

        std::string Monster::Serialize()
        {
            std::string jsonString;
            try {
                rapidjson::StringBuffer jsonbuffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);
                writer.StartObject();

                writer.String("pos");
                writer.StartArray();
                writer.Int(m_bornPoint.x);
                writer.Int(m_bornPoint.y);
                writer.EndArray();

                writer.String("level");
                writer.Int(m_level);

                writer.EndObject();
                jsonString = jsonbuffer.GetString();
            } catch (std::exception& ex) {
                LOG_ERROR("save ResNode json fail: %s\n", ex.what());
            }
            return jsonString;
        }

        bool Monster::Deserialize(std::string data)
        {
            try {
                rapidjson::Document doc;
                doc.Parse<0>(data.c_str());
                if (doc.IsObject()) {

                    if (doc["pos"].IsArray()) {
                        auto& temp = doc["pos"];
                        m_bornPoint.x = temp[0u].GetInt();
                        m_bornPoint.y = temp[1].GetInt();
                    }

                    m_level = doc.HasMember("level")  ? doc["level"].GetInt() : 0;
                }
                return true;
            } catch (std::exception& ex) {
                LOG_ERROR("json string to MonsterNode fail: %s\n", ex.what());
                return false;
            }
        }

        void Monster::FinishDeserialize() 
        {
            Init();
        }

        bool Monster::RemoveSelf()
        {
            return Unit::RemoveSelf();
        }

    }
}
