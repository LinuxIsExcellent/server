#include "geometry.h"

namespace ms
{
    namespace map
    {
        int GetCrossoverPoint(const FloatPoint& a, const FloatPoint& b, const FloatPoint& c, const FloatPoint& d, FloatPoint& crossPoint)
        {
            return 0;
        }

        float GetDistanceInRect(const FloatPoint& a, const FloatPoint& b, const Point& rectStart, const Point& rectEnd)
        {
            return 0.0f;
            /*
            float dis = .0f;
            bool aInRect = false, bInRect = false;
            aInRect = is_in_rect(a, rectStart, rectEnd);
            bInRect = is_in_rect(b, rectStart, rectEnd);

            if (aInRect && bInRect) {
                dis = get_distance(a, b);
            } else if ((aInRect && !bInRect) || (!aInRect && bInRect)) {
            } else if (!aInRect && !bInRect) {
            }
            return dis;
            */
        }
    }
}

