#ifndef BASE_GRAPH_VEC2_H
#define BASE_GRAPH_VEC2_H

#include <cmath>
#include <iosfwd>
#include "../gateway/packet_base.h"

namespace base
{
    namespace graph
    {
        struct Vec2 {
            Vec2() : x(0.0f), y(0.0f) {}

            float x;
            float y;

            bool operator == (const Vec2& rhs) const {
                return rhs.x == x && rhs.y == y;
            }

            bool operator != (const Vec2& rhs) const {
                return rhs.x != x || rhs.y != y;
            }

            const Vec2& operator = (const Vec2& rhs) {
                x = rhs.x;
                y = rhs.y;
                return *this;
            }

            void Set(float _x, float _y) {
                x = _x;
                y = _y;
            }

            PACKET_DEFINE(x, y)
        };

        std::ostream& operator << (std::ostream& out, const Vec2& p);
    }
}

#endif // VEC2_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
