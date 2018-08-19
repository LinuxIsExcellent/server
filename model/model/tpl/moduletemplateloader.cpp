#include "moduletemplateloader.h"
#include "templateloader.h"
#include <functional>

namespace model
{
    namespace tpl
    {
        ModuleTemplateLoader::ModuleTemplateLoader()
            : ModuleBase("model.tploader")
        {
            AddDependentModule("dbo");
        }

        ModuleTemplateLoader::~ModuleTemplateLoader()
        {
            SAFE_DELETE(m_tploader);
        }

        ModuleTemplateLoader* ModuleTemplateLoader::Create()
        {
            ModuleTemplateLoader* obj = new ModuleTemplateLoader;
            obj->AutoRelease();
            return obj;
        }

        void ModuleTemplateLoader::OnModuleSetup()
        {
            SetModuleState(base::MODULE_STATE_IN_SETUP);

            m_tploader = new TemplateLoader();
            m_tploader->BeginSetup([this](bool result) {
                if (result) {
                    m_tploader->DebugDump();

                    SetModuleState(base::MODULE_STATE_RUNNING);
                } else {
                    SetModuleState(base::MODULE_STATE_DELETE);
                }
            });
        }

        void ModuleTemplateLoader::OnModuleCleanup()
        {
            SetModuleState(base::MODULE_STATE_DELETE);
        }
    }
}
