#include "tree.h"
#include <model/tpl/templateloader.h>
#include <base/logger.h>

namespace ms
{
    namespace map
    {
        using namespace model::tpl;
        Tree::Tree(int id, const MapUnitTreeTpl* tpl, const Point& bornPoint)
            : Unit(id, tpl), m_tpl(tpl), m_bornPoint(bornPoint)
        {
        }

        Tree::~Tree()
        {
        }

        void Tree::Init()
        {
        }

        std::string Tree::Serialize()
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

                writer.EndObject();
                jsonString = jsonbuffer.GetString();
            } catch (std::exception& ex) {
                LOG_ERROR("save Tree json fail: %s\n", ex.what());
            }
            return jsonString;
        }

        bool Tree::Deserialize(std::string data)
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
                }
                return true;
            } catch (std::exception& ex) {
                LOG_ERROR("json string to Tree fail: %s\n", ex.what());
                return false;
            }
        }
    }
}
