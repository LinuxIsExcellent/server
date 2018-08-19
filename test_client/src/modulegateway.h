#ifndef MODULEGATEWAY_H
#define MODULEGATEWAY_H

#include <base/modulebase.h>

namespace base
{
    namespace gateway
    {
        class UserClient;
    }
}

namespace ts
{
    class FSUserClientEventHandler;
    class FSClientSession;

    class ModuleGateway : public base::ModuleBase
    {
    public:
        static ModuleGateway* Create();
        ~ModuleGateway();

    private:
        ModuleGateway();
        virtual void OnModuleSetup();
        virtual void OnModuleCleanup();

        FSUserClientEventHandler* m_handle = nullptr;
        base::gateway::UserClient* m_client = nullptr;
        FSClientSession* m_session = nullptr;
    };

}

#endif // MODULEGATEWAY_H
