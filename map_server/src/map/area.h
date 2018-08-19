#ifndef MAP_AREA_H
#define MAP_AREA_H

#include <list>
#include <unordered_map>
#include "point.h"

namespace ms
{
    namespace map
    {

        class Troop;
        class Agent;
        class Unit;

        class Area
        {
        public:
            Area() {};
            ~Area() {};

            const Point& pA() const {
                return m_pA;
            }
            const Point& pB() const {
                return m_pB;
            }
            const std::unordered_map<int, Unit*>& units() const {
                return m_units;
            }
            const std::list<Agent*>& viewers() const {
                return m_viewers;
            }
            const std::unordered_map<int, Troop*>& troops() const {
                return m_troops;
            }

        public:
            void AddViewer(Agent* agent);
            void RemoveViewer(Agent* agent);

            void AddUnit(Unit* unit);
            void RemoveUnit(Unit* uint);

            bool AddTroop(Troop* troop);
            void RemoveTroop(Troop* troop);

            // 初始化灯塔信息
            void Init(int i, int j, int width, int height);
            // 行军是否与灯塔相交
            bool IsCross(const Point& a, const Point& b);
            void Debug();
             
        private:
            std::list<Agent*> m_viewers;
            std::unordered_map<int, Unit*> m_units;

            //troop
            Point m_pA;
            Point m_pB;
            std::unordered_map<int, Troop*> m_troops;
        };
    }
}

#endif // MAP_AREA_H
