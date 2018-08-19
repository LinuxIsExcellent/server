#ifndef BASE_HTTP_MODULEHTTPCLIENT_H
#define BASE_HTTP_MODULEHTTPCLIENT_H

#include "../modulebase.h"

namespace base
{
    namespace http
    {
        class ModuleHttpClient : public base::ModuleBase
        {
        public:
            static ModuleHttpClient* Create();
            ~ModuleHttpClient();

        private:
            ModuleHttpClient();
            virtual void OnModuleSetup();
            virtual void OnModuleCleanup();
        };
    }
}

#endif // MODULEHTTPCLIENT_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
