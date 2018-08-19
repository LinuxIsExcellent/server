#include "worldbossactivity.h"
#include "../tpl/templateloader.h"
#include "../map.h"
#include "../mapMgr.h"
#include "../unit/worldboss.h"
#include <base/event/dispatcher.h> 
#include <base/logger.h>

namespace ms
{
    namespace map
    {
        namespace activity
        {
            using namespace std;
            using namespace tpl;

            void WorldBossActivity::OnStart()
            {
                cout << "worldbossactivity::OnStart" << endl;
                m_worldBossTplTemp = m_worldBossTpl;

                m_maintainer.Add(g_dispatcher->quicktimer().SetIntervalWithLinker([this] {
                    //cout << "WorldBossActivity CREATE WORLD BOSS" << endl;
                    if (m_worldBossTplTemp.empty()) {
                        m_maintainer.ClearAll();
                        cout << "CREATE FINISHED" << endl;
                        return;
                    }
                    
                    auto it = m_worldBossTplTemp.begin();
                    int tplid = it->first;
                    int count = it->second;
                    if (count > CREATE_COUNT_EVERY_TIMES) {
                        count = CREATE_COUNT_EVERY_TIMES;
                        it->second -= count;
                    } else {
                        m_worldBossTplTemp.erase(it);
                    }
                    
                    // cout << "tplid = " << tplid << " count = " << count << endl;
                    const MapUnitTpl* tpl = m_tploader->FindMapUnit(tplid);
                    if (tpl && tpl->IsWorldBoss()) {
                        //int64_t t1 = base::utils::nowtick();
                        for (int i = 0; i < count; ++i) {
                            int loop = 0;
                            while (true) {
                                if (++loop > 10) {
                                    LOG_DEBUG("worldbossactivity::OnStart create boss failed loop>10");
                                    break;
                                }
                                Point pos = g_mapMgr->GetRandomPoint();
                                if (Unit* mt = g_mapMgr->CreateUnit(tpl, pos.x, pos.y)) {
                                    // printf("create world boss x=%d, y=%d\n", pos.x, pos.y);
                                    m_worldBosses.push_back(mt->ToWorldBoss());
                                    break;
                                }
                            }
                        }
                        //int64_t t2 = base::utils::nowtick();
                        //cout << "t2 - t1 = " << t2 - t1 << endl;
                    }
                    //cout << "world boss size = " << m_worldBosses.size() << endl;
                }, 100));
            }

            void WorldBossActivity::OnEnd()
            {
                cout << "WorldBossActivity::OnEnd" << endl;
                for (WorldBoss * wb : m_worldBosses) {
                    wb->RemoveSelf();
                }
                m_worldBosses.clear();
                m_maintainer.ClearAll();
            }
        }
    }
}