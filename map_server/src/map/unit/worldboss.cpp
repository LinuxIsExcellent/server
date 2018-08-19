#include "worldboss.h"
#include <model/tpl/templateloader.h>
#include <base/event/dispatcher.h> 
#include <model/tpl/army.h>

namespace ms
{
    namespace map
    {
        using namespace std;
        using namespace tpl;
        using namespace model::tpl;
        
        
        WorldBoss::WorldBoss(int id, const map::tpl::MapUnitWorldBossTpl* tpl, const Point& bornPoint) 
            : Monster(id, tpl, bornPoint), m_tpl(tpl)
        {
            m_activityId = id;
        }


        WorldBoss::~WorldBoss()
        {
        }
        
        void WorldBoss::Init()
        {
            int armyListCount = m_tpl->armyNum;
            for (int i = 0; i < armyListCount; ++i)
            {
                ArmyList armyList = tpl::m_tploader->GetRandomArmyList(m_tpl->armyGroup, m_tpl->armyCount);
                m_armyLists.push_back(armyList);
                armyListsTotal += armyList.ArmyCount(ArmyState::NORMAL);
            }
            m_defArmyList = m_armyLists.back();
            // m_maintainer.Add(g_dispatcher->quicktimer().SetIntervalWithLinker(std::bind(&WorldBoss::Debug, this), 30 * 1000), DEBUG);
        }

        const int WorldBoss::CurrentArmyTotal()
        {
            int armyListsTotal = 0;
            for (std::list<ArmyList>::iterator it = m_armyLists.begin(); it != m_armyLists.end(); ++it)
            {
                armyListsTotal += (*it).ArmyCount(ArmyState::NORMAL);
            }
            return armyListsTotal;
        }

        bool WorldBoss::SwitchTroop()
        {
            bool ret = false;
            if (!m_armyLists.empty()) {
                //取出最上层的军队
                m_armyLists.pop_back();
                if (!m_armyLists.empty()) {
                    m_defArmyList = m_armyLists.back();
                    ret = true;
                }
            }
            return ret;
        }

        bool WorldBoss::RemoveSelf()
        {
            return Unit::RemoveSelf();
        }
        
        void WorldBoss::Debug()
        {
            cout << "WorldBoss:Debug" << endl;
            cout << "armyList count = " << m_armyLists.size() << endl;
            cout << "m_defArmyList count = " << m_defArmyList.size() << endl;
            cout << "bornPoint is " << bornPoint().x << ", " <<  bornPoint().y << endl;
            cout << "armyListsTotal = " << ArmyListsTotal() << endl;
            cout << "CurrentArmyTotal = " << CurrentArmyTotal() << endl;
            // m_defArmyList.Debug();
            /*for (std::list<ArmyList>::iterator it = m_armyLists.begin(); it != m_armyLists.end(); ++it)
            {
                (*it).Debug();
            }*/
        }
    }
}
