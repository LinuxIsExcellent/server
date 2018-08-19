#include "unit.h"
#include "castle.h"
#include "resnode.h"
#include "monster.h"
#include "camp.h"
#include "famouscity.h"
#include "catapult.h"
#include "palace.h"
#include "capital.h"
#include "worldboss.h"
#include <base/framework.h>
#include <base/event/dispatcher.h>

namespace ms
{
    namespace map
    {
        using namespace std;
        using namespace base;
        using namespace model;

        Unit::Unit(int id, const MapUnitTpl* tpl)
            : m_id(id), m_tpl(tpl)
        {
            if (m_tpl && !m_tpl->ToTree()) {
                UpdateRefreshTime();
            }
        }

        Unit::~Unit()
        {
        }

        void Unit::UpdateRefreshTime()
        {
            m_refreshTime = g_dispatcher->GetTimestampCache();
            m_refreshTime += framework.random().GenRandomNum(1, 6 * 60 * 60);
            SetDirty();
        }

        Castle* Unit::ToCastle()
        {
            return type() == MapUnitType::CASTLE ? static_cast<Castle*>(this) : nullptr;
        }

        ResNode* Unit::ToResNode()
        {
            return IsResNode() ? static_cast<ResNode*>(this) : nullptr;
        }

        Monster* Unit::ToMonster()
        {
            return IsMonster() ? static_cast<Monster*>(this) : nullptr;
        }

        CampTemp* Unit::ToCampTemp()
        {
            return type() == MapUnitType::CAMP_TEMP ? static_cast<CampTemp*>(this) : nullptr;
        }

        CampFixed* Unit::ToCampFixed()
        {
            return type() == MapUnitType::CAMP_FIXED ? static_cast<CampFixed*>(this) : nullptr;
        }

        FamousCity* Unit::ToFamousCity()
        {
            return IsFamousCity() ? static_cast<FamousCity*>(this) : nullptr;
        }

        Capital* Unit::ToCapital()
        {
            return IsCapital() ? static_cast<Capital*>(this) : nullptr;
        }

        Catapult* Unit::ToCatapult()
        {
            return IsCatapult() ? static_cast<Catapult*>(this) : nullptr;
        }

        WorldBoss* Unit::ToWorldBoss()
        {
            return IsWorldBoss() ? static_cast<WorldBoss*>(this) : nullptr;
        }

        model::ResAreaType Unit::GetResArea()
        {
            model::ResAreaType type = model::ResAreaType::FIRST;
            // if ( ((130 <= m_pos.x < 260) || (940 < m_pos.x <=1070)) && ((130 <= m_pos.y < 260) || (940 < m_pos.y <=1070)) ) {
            //     type = model::ResAreaType::SECOND;
            // } else if ( ((260 <= m_pos.x < 390) || (810 < m_pos.x <=940)) && ((260 <= m_pos.y < 390) || (810 < m_pos.y <=940)) ) {
            //     type = model::ResAreaType::THIRD;
            // } else if ( ((390 <= m_pos.x < 520) || (680 < m_pos.x <=810)) && ((390 <= m_pos.y < 520) || (680 < m_pos.y <=810)) ) {
            //     type = model::ResAreaType::FOURTH;
            // } else if ( (520 <= m_pos.x <= 680)  && (520 <= m_pos.y <= 680)) {
            //     type = model::ResAreaType::FIFTH;
            // }

            return type;
        }

        void Unit::NoticeUnitUpdate()
        {
            g_mapMgr->OnUnitUpdate(this);
        }

        bool Unit::RefreshSelf()
        {
            bool ok = g_mapMgr->UpdateUnit(this);
            if (ok) {
//                 Init();
                SetDirty();
            }
            return ok;
        }

        bool Unit::RemoveSelf()
        {
            //std::cout << "Unit::RemoveSelf" << std::endl;
            return g_mapMgr->RemoveUnit(this);
        }

        bool Unit::MoveSelf(int x, int y)
        {
            return g_mapMgr->MoveUnit(this, x, y);
        }

        bool Unit::IsInRect(const Point& pos, const Point& start, const Point& end) const
        {
            return g_mapMgr->is_in_rect(pos, start, end);
        }
    }
}

