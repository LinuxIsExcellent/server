#ifndef MODULECENTERSERVER_H
#define MODULECENTERSERVER_H

#include <base/modulebase.h>
#include <base/observer.h>

namespace cs
{
    class ModuleCenterServer : public base::ModuleBase
    {
    public:
        static ModuleCenterServer* Create();
        ~ModuleCenterServer();

    private:
        ModuleCenterServer();
        virtual void OnModuleSetup();
        virtual void OnModuleCleanup();
        
    private:
        void OnTimerUpdate();
        base::AutoObserver m_autoObserver;
    };
}

#endif // MODULECENTERSERVER_H
