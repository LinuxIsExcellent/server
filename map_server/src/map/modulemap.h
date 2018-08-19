#ifndef MAP_MODULEMAP_H
#define MAP_MODULEMAP_H
#include <base/modulebase.h>

namespace ms
{
    namespace map
    {
        class MapMgr;
        class ModuleMap : public base::ModuleBase
        {
        public:
            static ModuleMap* Create();
            virtual ~ModuleMap();

        private:
            ModuleMap();
            virtual void OnModuleSetup() override;
            virtual void OnModuleCleanup() override;

            MapMgr* m_mapMgr = nullptr;
        };
    }
}

#endif // MODULEMAP_H
