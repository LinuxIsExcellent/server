#include "configure.h"
#include "../../logger.h"
#include "../../utils/file.h"
#include "../../framework.h"
#include <tinyxml/tinyxml2.h>
#include <string>

namespace base
{
    namespace dbo
    {
        namespace internal
        {
            using namespace std;

            bool ParseDBConfigureFile(const std::string& conf_file, std::vector<ConnectionPoolInfo>& conf)
            {
                tinyxml2::XMLDocument doc;
                tinyxml2::XMLError err = doc.LoadFile(conf_file.c_str());
                if (err != tinyxml2::XML_NO_ERROR) {
                    doc.GetErrorStr1();
                    LOG_ERROR("parse dbo conf file fail: %s", doc.GetErrorStr1());
                    return false;
                }

                tinyxml2::XMLElement* xml_framework = doc.FirstChildElement("framework");
                if (!xml_framework) {
                    LOG_ERROR("parse dbo conf file fail: bad format, not framework");
                    return false;
                }
                tinyxml2::XMLElement* xml_dbo = xml_framework->FirstChildElement("dbo");
                if (!xml_dbo) {
                    LOG_ERROR("parse dbo conf file fail: bad format, not dbo");
                    return false;
                }

                tinyxml2::XMLElement* xml_pool = xml_dbo->FirstChildElement("connpool");
                while (xml_pool) {
                    ConnectionPoolInfo info;
                    info.poolid = xml_pool->IntAttribute("id");
                    info.poolsize = xml_pool->IntAttribute("size");
                    int max_poolsize = xml_pool->IntAttribute("max");
                    if (max_poolsize < info.poolsize) {
                        max_poolsize = info.poolsize * 2;
                    }
                    info.max_poolsize = max_poolsize;
                    info.serverinfo.port = xml_pool->IntAttribute("port");
                    const char* host = xml_pool->Attribute("host");
                    const char* user = xml_pool->Attribute("user");
                    const char* pass = xml_pool->Attribute("pass");
                    const char* db = xml_pool->Attribute("db");
                    if (host == nullptr || user == nullptr || pass == nullptr || db == nullptr) {
                        LOG_ERROR("parse dbo conf file fail: bad format, missing attribute");
                    }
                    info.serverinfo.host = host;
                    info.serverinfo.username = user;
                    info.serverinfo.password = pass;
                    info.serverinfo.dbname = db;
                    conf.push_back(info);
                    xml_pool = xml_pool->NextSiblingElement("connpool");
                }

                return true;
            }
        }
    }
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
