#include "linesegment.h"
#include "vector2.h"
#include <base/graph/vec2.h>

namespace ms
{
    namespace engine
    {
        using namespace std;

        bool LineSegment::IntersectExtline(const LineSegment& ls) const
        {
            Vector2 vt0, vt1, vt2;
            vt0.Set(m_pB.x - m_pA.x, m_pB.y - m_pA.y);

            vt1.Set(m_pA.x - ls.m_pA.x, m_pA.y - ls.m_pA.y);
            vt2.Set(m_pA.x - ls.m_pB.x, m_pA.y - ls.m_pB.y);

            return (vt1.Cross(vt0) * vt2.Cross(vt0)) <= 0;
        }

        bool LineSegment::Intersect(const LineSegment& ls) const
        {
            // 检测自己的延长线是否和ls线段相交
            if (IntersectExtline(ls)) {
                // 检测ls的延长线是否和自己相交或者重合
                return ls.IntersectExtline(*this);
            }
            return false;
        }


    }
}
