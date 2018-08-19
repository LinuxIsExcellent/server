#include "area.h"
#include <algorithm>
#include "unit/unit.h"
#include "troop.h"
#include "agent.h"
#include <algorithm>
#include "../engine/geometry.h"
#include "../engine/linesegment.h"

namespace ms
{
    namespace map
    {
        using namespace std;
        using namespace engine;

        void Area::AddViewer(Agent* agent)
        {
            auto it = find(m_viewers.begin(), m_viewers.end(), agent);
            if (it == m_viewers.end()) { 
                m_viewers.push_back(agent);
            }
        }

        void Area::RemoveViewer(Agent* agent)
        {
            auto it = find(m_viewers.begin(), m_viewers.end(), agent);
            if (it != m_viewers.end()) {
                m_viewers.erase(it);
            }
        }

        void Area::AddUnit(Unit* unit)
        {
            auto it = m_units.find(unit->id());
            if (it == m_units.end()) {
                m_units.emplace(unit->id(), unit);
            }
        }

        void Area::RemoveUnit(Unit* unit)
        {
            m_units.erase(unit->id());
        }

        bool Area::AddTroop(Troop* troop)
        {
            //cout << "Area::AddTroop" << endl;
            return m_troops.emplace(troop->id(), troop).second;
//             auto it = m_troops.find(troop->id());
//             if (it == m_troops.end()) {
//                 m_troops.emplace(troop->id(), troop);
//                 return true;
//             }
//             return false;
        }

        void Area::RemoveTroop(Troop* troop)
        {
            m_troops.erase(troop->id());
        }

        void Area::Init(int i, int j, int width, int height)
        {
            m_pA.x = i * width;
            m_pA.y = j * height;

            m_pB.x = (i + 1) * width;
            m_pB.y = (j + 1) * height;
            //printf("Area::Init aX=%d, aY=%d, bX=%d, bY=%d\n", m_pA.x, m_pA.y, m_pB.x, m_pB.y);
        }

        bool Area::IsCross(const Point& a, const Point& b)
        {
            if (is_in_rect(a, m_pA, m_pB) || is_in_rect(b, m_pA, m_pB)) {
                return true;
            }
            if (a.x < m_pA.x && b.x < m_pA.x) {
                return false;
            }
            if (a.x > m_pB.x && b.x > m_pB.x) {
                return false;
            }
            if (a.y < m_pA.y && b.y < m_pA.y) {
                return false;
            }
            if (a.y > m_pB.y && b.y > m_pB.y) {
                return false;
            }

            LineSegment tls = LineSegment(a, b);
            Point pC = Point(m_pA.x, m_pB.y);
            Point pD = Point(m_pB.x, m_pA.y);

            LineSegment temp = LineSegment(m_pA, m_pB);
            if (tls.Intersect(temp)) {
                return true;
            }
            temp = LineSegment(pC, pD);
            if (tls.Intersect(temp)) {
                return true;
            }

//             LineSegment temp = LineSegment(m_pA, pC);
//             if (tls.Intersect(temp)) {
//                 return true;
//             }
//             temp = LineSegment(m_pA, pD);
//             if (tls.Intersect(temp)) {
//                 return true;
//             }
//             temp = LineSegment(m_pB, pC);
//             if (tls.Intersect(temp)) {
//                 return true;
//             }
//             temp = LineSegment(m_pB, pD);
//             if (tls.Intersect(temp)) {
//                 return true;
//             }
            return false;
        }

        void Area::Debug()
        {
            cout << "######################################################################################" << endl;
            printf("AreaForTroop::Debug aX=%d, aY=%d, bX=%d, bY=%d\n", m_pA.x, m_pA.y, m_pB.x, m_pB.y);
            cout << "viewers begin" << endl;
            for (auto it = m_viewers.begin(); it != m_viewers.end(); ++it) {
                Agent* agt = *it;
                cout << agt->uid() << ",";
            }
            cout << "viewers end" << endl;
            cout << "troops begin" << endl;
            for (auto it = m_troops.begin(); it != m_troops.end(); ++it) {
                Troop* tp = it->second;
                cout << tp->id() << ",";
            }
            cout << "troops end" << endl;
            
            cout << "m_units begin" << endl;
            for (auto &value : m_units) {
                auto unit = value.second;
                cout << "id = " << unit->id() << " type = " << (int)unit->type() << " x = " << unit->x() << " y = " << unit->y() << endl;
            }
            cout << "m_units end" << endl;
            cout << "######################################################################################" << endl;
        }

    }
}
