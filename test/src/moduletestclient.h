#ifndef MODULETESTCLIENT_H
#define MODULETESTCLIENT_H

#include <base/modulebase.h>

class ModuleTestClientImpl;

class ModuleTestClient : public base::ModuleBase
{
public:
    static ModuleTestClient* Create(std::string ip, int port);
    virtual ~ModuleTestClient();

private:
    ModuleTestClient(std::string ip, int port);
    
    virtual void OnModuleSetup();
    virtual void OnModuleCleanup();

    std::string m_ip;
    int m_port;
    ModuleTestClientImpl* m_impl = nullptr;
    friend class ModuleTestClientImpl;
};

#endif // MODULETESTCLIENT_H
