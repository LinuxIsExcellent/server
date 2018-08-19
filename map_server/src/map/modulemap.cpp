#include "modulemap.h"
#include "mapMgr.h"
#include <base/configurehelper.h>
#include "qo/dataservice.h"

namespace ms
{
    namespace map
    {
        ModuleMap* ModuleMap::Create()
        {
            ModuleMap* obj = new ModuleMap();
            obj->AutoRelease();
            return obj;
        }

        ModuleMap::ModuleMap() : ModuleBase("ms.map")
        {
            AddDependentModule("cluster");
            AddDependentModule("http_client");
            AddDependentModule("map.tploader");
        }

        ModuleMap::~ModuleMap()
        {
            qo::DataService::Destroy();
            SAFE_DELETE(m_mapMgr);
        }

        void ModuleMap::OnModuleSetup()
        {
            base::ConfigureHelper helper;
            tinyxml2::XMLElement* msNode = helper.fetchConfigNodeWithServerRule();
            if (msNode == nullptr) {
                SetModuleState(base::MODULE_STATE_DELETE);
                return;
            }
            tinyxml2::XMLElement* mapServiceNode = helper.FirstChildElementWithPath(msNode, "mapService");
            if (mapServiceNode == nullptr) {
                SetModuleState(base::MODULE_STATE_DELETE);
            }
            int id = mapServiceNode->IntAttribute("id");

            m_mapMgr = MapMgr::Create(id);
            m_mapMgr->BeginSetup([this](bool result) {
                if (result) {
                    SetModuleState(base::MODULE_STATE_RUNNING);
                } else {
                    SetModuleState(base::MODULE_STATE_DELETE);
                }
            });

            qo::DataService::Create();
        }

        void ModuleMap::OnModuleCleanup()
        {
            SetModuleState(base::MODULE_STATE_IN_CLEANUP);
            m_mapMgr->BeginCleanup([this]() {
                SetModuleState(base::MODULE_STATE_DELETE);
            });
        }
    }
}
