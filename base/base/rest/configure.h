#ifndef BASE_REST_CONFIGURE_H
#define BASE_REST_CONFIGURE_H

#include "../global.h"
#include <string>

namespace base
{
    namespace rest
    {
        struct Configure {
            virtual ~Configure() {}

            std::string ip;
            int port;
            std::string auth_key;

            virtual bool ReadFromConfigureFile() = 0;
        };

        struct BinaryConfigure : public Configure {
            virtual bool ReadFromConfigureFile();
        };
    }
}

#endif // CONFIGURE_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
