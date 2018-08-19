#ifndef MAP_MONSTER_H
#define MAP_MONSTER_H
#include "unit.h"

namespace ms
{
    namespace map
    {

        class ArmyList;

        class Monster : public Unit
        {
        public:
            Monster(int id, const MapUnitMonsterTpl* tpl, const Point& bornPoint);
            ~Monster();

            virtual void Init() override;

            virtual ArmyList* DefArmyList() override{
                return &m_armyList;
            }
            
            const MapUnitMonsterTpl& tpl() const {
                return *m_tpl;
            }

            const Point& bornPoint() const {
                return m_bornPoint;
            }

            const int level() const {
                return m_level;
            }

            ArmyList& armyList() {
                return m_armyList;
            }
            
            bool IsDie() const;

            //行军到达
            virtual void OnTroopReach(Troop* troop) override{
                //Todo: 当野怪已经被击败时，要做特殊处理 
            }
            //行军离开
            virtual void OnTroopLeave(Troop* troop) override {};

            virtual void SetTpl(const MapUnitTpl* tpl) override;

            virtual std::string Serialize() override;
            
            virtual bool Deserialize(std::string data) override;

            virtual void FinishDeserialize() override;

            virtual bool RemoveSelf();

        private:
            const MapUnitMonsterTpl* m_tpl = nullptr;
            Point m_bornPoint;
            int8_t m_direction = 0;              //朝向
            int m_level;
            Troop* m_troop = nullptr;
            ArmyList m_armyList;    //默认驻守军(包括陷阱)
            model::ResAreaType m_resAreaType = ResAreaType::FIRST; //资源带类型
        };

    }
}

#endif // MONSTER_H
