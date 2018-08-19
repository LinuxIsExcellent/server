#ifndef MAP_WORLDBOSS_H
#define MAP_WORLDBOSS_H
#include "monster.h"

namespace model
{
    namespace tpl
    {
        class ArmyTpl;
    }
}

namespace ms
{
    namespace map
    {
        class WorldBoss : public Monster
        {
        public:
            WorldBoss(int id, const map::tpl::MapUnitWorldBossTpl* tpl, const Point& bornPoint);
            virtual ~WorldBoss();
            
            const MapUnitWorldBossTpl& tpl() const {
                return *m_tpl;
            }
            
            const model::tpl::ArmyTpl& bossTpl() const {
                return *m_bossTpl;
            }

            virtual ArmyList* DefArmyList() override{
                return &m_defArmyList;
            }

            const int CurrentArmyTotal();

            const int ArmyListsTotal()
            {
                return armyListsTotal;
            }
            
            virtual bool RemoveSelf();
            bool SwitchTroop();
            void Debug();
        public:
            virtual void Init() override;
            
        private:
            static constexpr int DEBUG = 1;

            const map::tpl::MapUnitWorldBossTpl* m_tpl = nullptr;
            const model::tpl::ArmyTpl* m_bossTpl = nullptr;  
            std::list<ArmyList> m_armyLists;      //世界Boss军队列表
            ArmyList m_defArmyList;       //世界Boss首军
            int armyListsTotal;           //士兵总数
            int64_t m_activityId = 0;             //对应的活动Id

            base::ObjectMaintainer m_maintainer;
        };
    }
}


#endif // MAP_WORLDBOSS_H
