#ifndef BASE_GATEWAY_CONFIGURE_H
#define BASE_GATEWAY_CONFIGURE_H

#include "../global.h"
#include <string>

namespace base
{
    namespace gateway
    {
        struct GatewayInfo {
            std::string ip;
            int32_t port;
            uint32_t max_connections;
        };

        class Configure
        {
        public:
            Configure();
            ~Configure();

            const GatewayInfo& gatewayinfo() const {
                return gatewayinfo_;
            }

            bool Parse();

        private:
            GatewayInfo gatewayinfo_;
        };
    }
}

#endif // CONFIGURE_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
