#ifndef MAP_TREE_H
#define MAP_TREE_H
#include "unit.h"

namespace ms
{
    namespace map
    {

        class Tree : public Unit
        {
        public:
            Tree(int id, const MapUnitTreeTpl* tpl, const Point& bornPoint);
            ~Tree();

            virtual void Init() override;

            const MapUnitTreeTpl& tpl() const {
                return *m_tpl;
            }

            const Point& bornPoint() const {
                return m_bornPoint;
            }

            virtual std::string Serialize() override;
            virtual bool Deserialize(std::string data) override;

        private:
            const MapUnitTreeTpl* m_tpl = nullptr;
            Point m_bornPoint;
        };

    }
}

#endif // TREE_H
