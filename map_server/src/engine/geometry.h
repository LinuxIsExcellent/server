#ifndef ENGINE_GEOMETRY_H
#define ENGINE_GEOMETRY_H
#include "../map/point.h"

namespace ms
{
    namespace engine
    {
        using namespace map;

        inline bool is_in_rect(int x, int y, const Point& start, const Point& end)
        {
            return (x >= start.x && x <= end.x && y >= start.y && y <= end.y);
        }
        inline bool is_in_rect(const Point& pos, const Point& start, const Point& end)
        {
            return is_in_rect(pos.x, pos.y, start, end);
        }
        inline bool is_in_rect(const FloatPoint& pos, const Point& start, const Point& end)
        {
            return (pos.x >= start.x && pos.x <= end.x && pos.y >= start.y && pos.y <= end.y);
        }

        inline float get_distance(const Point& a, const Point& b)
        {
            return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
        }
        inline float get_distance(const FloatPoint& a, const Point& b)
        {
            return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
        }
        inline float get_distance(const FloatPoint& a, const FloatPoint& b)
        {
            return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
        }

        // 求两线段ab与cd交点
        // return : 0不相交 1相交 2重合
        int GetCrossoverPoint(const FloatPoint& a, const FloatPoint& b, const FloatPoint& c, const FloatPoint& d, FloatPoint& crossPoint);

        // 求线段ab在矩形中的距离
        float GetDistanceInRect(const FloatPoint& a, const FloatPoint& b, const Point& rectStart, const Point& rectEnd);

        // 求两线段是否相交
    }
}

#endif //ENGINE_GEOMETRY_H

