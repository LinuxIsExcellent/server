#include "configure.h"
#include "../logger.h"
#include "../framework.h"
#include "../configurehelper.h"

namespace base
{
    namespace gateway
    {
        using namespace std;

        Configure::Configure()
        {
        }

        Configure::~Configure()
        {
        }

        bool Configure::Parse()
        {
            ConfigureHelper helper;
            tinyxml2::XMLElement* node = helper.fetchConfigNodeWithServerRule();
            if (node == nullptr) {
                return false;
            }

            tinyxml2::XMLElement* xmlGateway = node->FirstChildElement("gateway");
            if (xmlGateway == nullptr) {
                return false;
            }
            const char* strIP = xmlGateway->Attribute("ip");
            if (!strIP) {
                return false;
            }
            gatewayinfo_.ip = strIP;
            gatewayinfo_.port = xmlGateway->IntAttribute("port");
            gatewayinfo_.max_connections = xmlGateway->UnsignedAttribute("maxConnection");

            return !gatewayinfo_.ip.empty() && gatewayinfo_.port > 0 && gatewayinfo_.max_connections > 0;
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
