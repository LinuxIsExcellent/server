#ifndef MAP_CAPITAL_H
#define MAP_CAPITAL_H
#include "unit.h"
#include "../troop.h"
#include <base/event/dispatcher.h>
#include "famouscity.h"

namespace ms
{
    namespace map
    {
        // 州郡
        class ChowPrefecture : public FamousCity
        {
        public:
            ChowPrefecture(int id, const ms::map::tpl::MapUnitFamousCityTpl* tpl, const Point& bornPoint);
            virtual ~ChowPrefecture();

            // 是否可驻守
            virtual bool CanGarrison(Troop* troop) override;
            // 是否可占领
            virtual bool CanOccupy(Troop* troop) override;

            virtual void FinishDeserialize() override;
            
        };

        // 县城
        class County : public FamousCity
        {
        public:
            County(int id, const ms::map::tpl::MapUnitFamousCityTpl* tpl, const Point& bornPoint);
            virtual ~County();

            // 是否可驻守
            virtual bool CanGarrison(Troop* troop) override;
            // 是否可占领
            virtual bool CanOccupy(Troop* troop) override;

            virtual void OnTroopLeave(Troop* troop) override;
        };
    }
}

#endif // MAP_CAPITAL_H
