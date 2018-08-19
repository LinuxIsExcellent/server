#ifndef MAP_POINT_H
#define MAP_POINT_H

#include <base/cluster/message.h>
#include <base/data.h>
#include <cmath>

namespace ms
{
    namespace map
    {
        struct Point {
            Point() : x(0), y(0) {}
            Point(int _x, int _y) : x(_x), y(_y) {}
            Point(const Point& p) {
                x = p.x;
                y = p.y;
            }

            void operator=(const Point& p) {
                x = p.x;
                y = p.y;
            }
            bool operator==(const Point& p) const {
                return x == p.x && y == p.y ? true : false;
            }

            inline friend base::cluster::MessageOut& operator<< (base::cluster::MessageOut& msgout, const Point& p) {
                msgout << p.x << p.y;
                return msgout;
            }

            inline friend std::ostream& operator<< (std::ostream& out, const Point& p) {
                return out << "x = " << p.x << " y = " << p.y << std::endl;
            }
            
            bool operator<(const Point& other)  const {
                return ((x < other.x) || (!(other.x < x) && (y < other.y)));
            }
            
            int x;
            int y;
        };

        inline void Set(base::DataTable& dt, const Point& p)
        {
            dt.Set("x", p.x);
            dt.Set("y", p.x);
        }

        struct PosLimit {
            Point start;
            Point end;

            PosLimit() {}
            PosLimit(int startX, int startY, int endX, int endY) : start(Point(startX, startY)), end(Point(endX, endY)) {}
        };

        struct Rect {
            Rect() {}

            Point start;    //左下角的点
            Point end;      //右上角的点
        };

        struct FloatPoint {
            FloatPoint() : x(.0), y(.0) {}
            FloatPoint(float _x, float _y) : x(_x), y(_y) {}
            FloatPoint(const FloatPoint& fp) {
                x = fp.x;
                y = fp.y;
            }
            FloatPoint(const Point& p) {
                x = p.x;
                y = p.y;
            }

            int intX() const {
                return std::floor(x + 0.5);
            }

            int intY() const {
                return std::floor(y + 0.5);
            }

            Point toPoint() const {
                return {intX(), intY()};
            }

            bool operator==(const Point& p) const {
                return ((abs(x - p.x) < 1) && (abs(y - p.y) < 1)) ? true : false;
            }

            inline friend base::cluster::MessageOut& operator<< (base::cluster::MessageOut& msgout, const FloatPoint& fp) {
                msgout << fp.x << fp.y;
                return msgout;
            }

            void operator=(const Point& p) {
                x = p.x;
                y = p.y;
            }

            float x;
            float y;
        };
    }
}

#endif // POINT_H
