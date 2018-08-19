#ifndef ENGINE_LINESEGMENT_H
#define ENGINE_LINESEGMENT_H
#include "../map/point.h"

namespace ms
{
    namespace engine
    {
        class LineSegment
        {
        public:
            LineSegment(const map::FloatPoint& pA, const map::FloatPoint& pB)
                : m_pA(pA), m_pB(pB) {}
            virtual ~LineSegment() {}

            const map::FloatPoint& pA() const {
                return m_pA;
            }

            const map::FloatPoint& pB() const {
                return m_pB;
            }

        public:
            // 线段ls是否和线段自身所在的直线相交，但不去分平行的状况
            bool IntersectExtline(const LineSegment& ls) const;

            //用于检测ls线段是否和自己相交
            bool Intersect(const LineSegment& ls) const;

        private:
            map::FloatPoint m_pA;
            map::FloatPoint m_pB;
        };
    }
}

#endif // ENGINE_LINESEGMENT_H
