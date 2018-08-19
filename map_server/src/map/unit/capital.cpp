#include "capital.h"
#include <algorithm>
#include <base/logger.h>
#include <model/tpl/templateloader.h>
#include <model/tpl/alliance.h>
#include "../alliance.h"
#include "../agent.h"

namespace ms
{
    namespace map
    {
         using namespace model::tpl;

        ChowPrefecture::ChowPrefecture(int id, const MapUnitFamousCityTpl* tpl, const Point& bornPoint)
            : FamousCity(id, tpl, bornPoint)
        {

        }

        ChowPrefecture::~ChowPrefecture()
        {

        }

        bool ChowPrefecture::CanGarrison(Troop* troop)
        {
            if (allianceId() >0 && allianceId() ==  troop->allianceId()) {
                bool canGarrison = true;

                return canGarrison;
            }
            return false;
        }

        bool ChowPrefecture::CanOccupy(Troop* troop)
        {
            if (allianceId() == 0 || allianceId() != troop->allianceId()) {
                bool canOccupy = true;

                return canOccupy;
            }
            return false;
        }

        void ChowPrefecture::FinishDeserialize()
        {
            FamousCity::FinishDeserialize();
            /*if (allianceId() > 0) {
                g_map->AllianceOwnCity(allianceId(), this);
            }*/
        }


        County::County(int id, const MapUnitFamousCityTpl* tpl, const Point& bornPoint)
            : FamousCity(id, tpl, bornPoint)
        {

        }

        County::~County()
        {

        }

        bool County::CanGarrison(Troop* troop)
        {
            if (allianceId() >= 0 && allianceId() == troop->allianceId()) {
                return true;
            }
            return false;
        }

         bool County::CanOccupy(Troop* troop)
        {
            return allianceId() == 0 || allianceId() != troop->allianceId();
        }

        void County::OnTroopLeave(Troop* troop)
        {
            FamousCity::OnTroopLeave(troop);
            if (troop ==  m_troop) {
                m_allianceId = 0;
            }
        }
    }
}
