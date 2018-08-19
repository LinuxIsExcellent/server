#include "configure.h"
#include "../logger.h"
#include "../framework.h"
#include "../utils/file.h"
#include <tinyxml/tinyxml2.h>
#include <ostream>

namespace base
{
    namespace cluster
    {
        using namespace std;
        using namespace tinyxml2;

        const char* NodeInfo::GetTypeString() const
        {
            switch (type) {
                case NodeType::NORMAL:
                    return "NORMAL";
                case NodeType::MASTER:
                    return "MASTER";
                case NodeType::STANDALONE:
                    return "STANDALONE";
                case NodeType::UNKNOWN:
                    return "UNKNOWN";
            }
            return "";
        }

        static bool parse_listener(tinyxml2::XMLElement* xml, string& ip, int& port)
        {
            if (!xml) {
                return false;
            }
            const char* str_ip = xml->Attribute("ip");
            if (str_ip == nullptr) {
                return false;
            }
            ip = str_ip;

            port = xml->IntAttribute("port");
            if (port == 0) {
                return false;
            }
            return true;
        }

        static bool parse_node_type(tinyxml2::XMLElement* node, NodeType& type)
        {
            type = NodeType::NORMAL;
            const char* str_type = node->Attribute("type");
            if (str_type != nullptr) {
                if (strcmp(str_type, "master") == 0) {
                    type = NodeType::MASTER;
                } else if (strcmp(str_type, "standalone") == 0) {
                    type = NodeType::STANDALONE;
                } else {
                    LOG_ERROR("parse cluster conf fail: no unrecognized type=%s", str_type);
                    return false;
                }
            }
            return true;
        }

        static bool load_configure_xml(tinyxml2::XMLDocument& doc)
        {
            string cf = framework.resource_dir() + "/common.conf";
            if (!utils::file_is_exist(cf.c_str())) {
                return false;
            }

            tinyxml2::XMLError err = doc.LoadFile(cf.c_str());
            if (err != tinyxml2::XML_NO_ERROR) {
                LOG_ERROR("parse cluster conf fail: %s", doc.GetErrorStr2());
                return false;
            }
            return true;
        }

        bool NodeInfo::ReadFromConfigFile(const string& name, uint16_t id)
        {
            tinyxml2::XMLDocument doc;
            if (!load_configure_xml(doc)) {
                return false;
            }

            XMLElement* s = doc.RootElement()->FirstChildElement(name.c_str());
            bool found = false;
            while (s) {
                if (s->UnsignedAttribute("id") == id) {
                    found = true;
                    break;
                } else {
                    s = s->NextSiblingElement(name.c_str());
                }
            }
            if (!found) {
                return false;
            }

            XMLElement* node = s->FirstChildElement("node");
            if (!node) {
                LOG_ERROR("parse cluster conf fail: can not find node section");
                return false;
            }
            if (!parse_listener(node->FirstChildElement("listener"), this->listen_ip, this->listen_port)) {
                return false;
            }
            if (!parse_node_type(node, this->type)) {
                return false;
            }
            this->node_id = id;
            this->node_name = name;
            return true;
        }

        bool SetupOption::ReadFromDefaultConfigFile()
        {
            tinyxml2::XMLDocument doc;
            if (!load_configure_xml(doc)) {
                LOG_ERROR("load cluster configure file fail!\n");
                return false;
            }

            const ServerRule& server_rule = framework.server_rule();
            XMLElement* s = doc.RootElement()->FirstChildElement(server_rule.name().c_str());
            bool found = false;
            while (s) {
                if (s->UnsignedAttribute("id") == server_rule.id()) {
                    found = true;
                    break;
                } else {
                    s = s->NextSiblingElement(server_rule.name().c_str());
                }
            }
            if (!found) {
                LOG_ERROR("parse cluster conf fail: can not find rule with [%s]", server_rule.full_name().c_str());
                return false;
            }

            XMLElement* node = s->FirstChildElement("node");
            if (!node) {
                LOG_ERROR("parse cluster conf fail: can not find node section");
                return false;
            }

            type = NodeType::NORMAL;
            const char* str_type = node->Attribute("type");
            if (str_type != nullptr) {
                if (strcmp(str_type, "master") == 0) {
                    type = NodeType::MASTER;
                } else if (strcmp(str_type, "standalone") == 0) {
                    type = NodeType::STANDALONE;
                } else if (strcmp(str_type, "normal") == 0) {
                    type = NodeType::NORMAL;
                } else {
                    LOG_ERROR("parse cluster conf fail: no unrecognized type=%s", str_type);
                    return false;
                }
            }

            XMLElement* listener = node->FirstChildElement("listener");
            if (!parse_listener(listener, listen_ip, listen_port)) {
                LOG_ERROR("parse cluster conf fail: no listener.ip or listener.port attribute");
                return false;
            }

            if (type == NodeType::NORMAL) {
                // parse master
                XMLElement* s = doc.RootElement()->FirstChildElement();
                while (s) {
                    XMLElement* node = s->FirstChildElement("node");
                    if (node) {
                        const char* str_type = node->Attribute("type");
                        if (str_type != nullptr && strcmp(str_type, "master") == 0) {
                            if (!parse_listener(node->FirstChildElement("listener"), master_ip, master_port)) {
                                return false;
                            }
                            break;
                        }
                    }
                    s = s->NextSiblingElement();
                }
            }

            node_name = server_rule.full_name();

            return true;
        }

        ostream& operator<<(ostream& out, const NodeInfo& info)
        {
            out << "[id:" << info.node_id << ",name:" << info.node_name << ",type:" << info.GetTypeString() << ", listen:" << info.listen_ip << ":" << info.listen_port;
            return out;
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
