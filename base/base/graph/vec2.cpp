#include "vec2.h"
#include <iostream>

namespace base
{
    namespace graph
    {
        std::ostream& operator << (std::ostream& out, const Vec2& p)
        {
            out << "{x:" << p.x << ",y:" << p.y << "}";
            return out;
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
