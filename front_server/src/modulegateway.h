#ifndef MODULEGATEWAY_H
#define MODULEGATEWAY_H

#include <base/modulebase.h>

namespace base
{
    namespace gateway
    {
        class Gateway;
    }
}

namespace fs
{
    class FSGatewayEventHandler;

    class ModuleGateway : public base::ModuleBase
    {
    public:
        static ModuleGateway* Create();
        ~ModuleGateway();
        
        virtual const char* GetObjectName() {
            return "fs::ModuleGateway";
        }

    private:
        ModuleGateway();
        virtual void OnModuleSetup();
        virtual void OnModuleCleanup();

        FSGatewayEventHandler* m_evtHandler = nullptr;
        base::gateway::Gateway* m_gateway = nullptr;
    };

}

#endif // MODULEGATEWAY_H
