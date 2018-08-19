#include "modulehttpclient.h"
#include "httpclient.h"
#include "../memory/memorypoolmgr.h"

namespace base
{
    namespace http
    {
        ModuleHttpClient* ModuleHttpClient::Create()
        {
            ModuleHttpClient* obj = new ModuleHttpClient();
            obj->AutoRelease();
            return obj;
        }

        ModuleHttpClient::ModuleHttpClient(): ModuleBase("http_client")
        {
            AddDependentModule("threadpool");
        }

        ModuleHttpClient::~ModuleHttpClient()
        {
            HttpClient::Destroy();
        }

        void ModuleHttpClient::OnModuleSetup()
        {
            memory::MemoryPool* mempool = memory::g_memory_pool_mgr->Aquire("http_client_memory_pool", 128, 128);
            HttpClient::Create(*mempool);
            SetModuleState(MODULE_STATE_RUNNING);
        }

        void ModuleHttpClient::OnModuleCleanup()
        {
            HttpClient::Destroy();
            SetModuleState(MODULE_STATE_DELETE);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
