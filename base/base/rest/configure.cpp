#include "configure.h"
#include <tinyxml/tinyxml2.h>
#include "../utils/file.h"
#include "../framework.h"
#include "../logger.h"
#include "../configurehelper.h"

namespace base
{
    namespace rest
    {
        using namespace std;

        bool BinaryConfigure::ReadFromConfigureFile()
        {
            ConfigureHelper helper;
            tinyxml2::XMLElement* node = helper.fetchConfigNodeWithServerRule();
            if (node == nullptr) {
                return false;
            }

            tinyxml2::XMLElement* binary = node->FirstChildElement("binaryRest");
            if (!binary) {
                return false;
            }

            tinyxml2::XMLElement* listener = binary->FirstChildElement("listen");
            if (!listener) {
                return false;
            }

            const char* ip_str = listener->Attribute("ip");
            if (!ip_str) {
                return false;
            }
            ip = ip_str;

            port = listener->IntAttribute("port");
            if (port == 0) {
                return false;
            }

            tinyxml2::XMLElement* key = binary->FirstChildElement("authkey");
            if (!key) {
                return false;
            }
            auth_key = key->GetText();
            if (auth_key.empty()) {
                return false;
            }
            return true;
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
