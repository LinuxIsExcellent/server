#include "configurehelper.h"
#include "framework.h"
#include "utils/file.h"
#include "logger.h"

namespace base
{
    using namespace std;

    static bool load_configure_xml(tinyxml2::XMLDocument& doc)
    {
        string cf = framework.resource_dir() + "/common.conf";
        if (!utils::file_is_exist(cf.c_str())) {
            LOG_ERROR("%s not exist", cf.c_str());
            return false;
        }

        tinyxml2::XMLError err = doc.LoadFile(cf.c_str());
        if (err != tinyxml2::XML_NO_ERROR) {
            LOG_ERROR("parse common.conf fail: %s", doc.GetErrorStr2());
            return false;
        }
        return true;
    }

    ConfigureHelper::ConfigureHelper()
    {
        _has_error = !load_configure_xml(_doc);
    }

    tinyxml2::XMLElement* ConfigureHelper::FirstChildElementWithPath(tinyxml2::XMLElement* node, const char* path)
    {
        tinyxml2::XMLElement* elem = node;
        const char* end = nullptr;
        do {
            end = strchr(path, '/');
            string str;
            if (end == nullptr) {
                str.append(path);
            } else {
                str.append(path, end - path);
                path = end + 1;
            }
            elem = elem->FirstChildElement(str.c_str());
        } while (end != nullptr && elem != nullptr);

        if (elem == node) {
            return nullptr;
        } else {
            return elem;
        }
    }

    tinyxml2::XMLElement* ConfigureHelper::fetchConfigNodeWithServerRule(const std::string& name, uint16_t id)
    {
        if (_has_error) {
            return nullptr;
        }
        tinyxml2::XMLElement* s = _doc.RootElement()->FirstChildElement(name.c_str());
        while (s) {
            if (s->IntAttribute("id") == id) {
                return s;
            }
            s = s->NextSiblingElement(name.c_str());
        }
        return nullptr;
    }

    tinyxml2::XMLElement* ConfigureHelper::fetchConfigNodeWithServerRule()
    {
        const ServerRule& rule = framework.server_rule();
        return fetchConfigNodeWithServerRule(rule.name(), rule.id());
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
