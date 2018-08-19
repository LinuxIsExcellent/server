#ifndef MODEL_TPL_MISCCONF_H
#define MODEL_TPL_MISCCONF_H
#include <string>

namespace model
{
    namespace tpl
    {
        struct Http {
            std::string ip = "0.0.0.0";
            uint16_t port = 9030;
        };
        
        struct HubSite {
            std::string ip;
            uint16_t port;
        };
        
        struct Sdk {
        };
        
        struct MiscConf {
            Http http;
            HubSite hubSite;
            Sdk sdk;
        };
    }
}

#endif // MODEL_TPL_MISCCONF_H