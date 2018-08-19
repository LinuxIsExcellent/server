#include "activitymgr.h"
#include "activity/activity.h"
#include "activity/worldbossactivity.h"
#include <base/data.h>
#include <base/event/dispatcher.h>
#include "tpl/templateloader.h"
#include "tpl/mapunittpl.h"
#include <model/tpl/templateloader.h>
#include <model/tpl/item.h>

namespace ms
{
    namespace map
    {
        using namespace std;
        using namespace tpl;
        using namespace base;
        using namespace model;
        using namespace activity;
        using namespace model::tpl;

        ActivityMgr::~ActivityMgr()
        {
            for (auto it = m_activities.begin(); it != m_activities.end(); ++it) {
                SAFE_DELETE(it->second);
            }
        }

        void ActivityMgr::Init()
        {
            // cout << "ActivityMgr::Init" << endl;

            g_dispatcher->quicktimer().SetInterval(std::bind(&ActivityMgr::CheckActivityStart, this), 60 * 1000, m_autoObserver);
        }

        void ActivityMgr::HandleMessage(const std::string& method, const base::DataTable& dt)
        {
            if (method == "activitiesUpdate") {
                vector<int64_t> ids;
                const DataTable& list = dt.Get(1)->ToTable();
                list.ForeachVector([&](int64_t k, const DataValue & v) {
                    const DataTable& row = v.ToTable();
                    int64_t id = row.Get("id")->ToInteger();
                    ids.push_back(id);
                    if (Activity* aty = FindActivity(id)) {
                        if (aty->IsStart()) {
                            return false;
                        } else {
                            m_activities.erase(id);
                        }
                    }
                    ActivityType type = (ActivityType)row.Get("type")->ToInteger();
                    int64_t openTime = row.Get("openTime")->ToInteger();
                    int64_t closeTime = row.Get("closeTime")->ToInteger();
                    const DataTable& config = row.Get("config")->ToTable();
                    if (type == ActivityType::WORLD_BOSS) {
                        int max = 1000;
                        unordered_map<int, int> worldBossTpl;
                        config.ForeachVector([&](const DataValue & k, const DataValue & v) {
                            const DataTable& row = v.ToTable();
                            int tplid = row.Get("monsterId")->ToInteger();
                            int count = row.Get("refreshCount")->ToInteger();  //数量最大值限制
                            if (count <= 0) {
                                return false;
                            } else if (count > max) {
                                count = max;
                            }
                            const MapUnitTpl* tpl = m_tploader->FindMapUnit(tplid);
                            if (tpl && tpl->IsWorldBoss()) {
                                worldBossTpl.emplace(tplid, count);
                            }
                            return false;
                        });
                        if (worldBossTpl.size() > 0) {
                            WorldBossActivity* aty = new WorldBossActivity(id, openTime, closeTime, worldBossTpl);
                            m_activities.emplace(id, aty);
                        }
                    }
/*                    if (type == ActivityType::MONSTER_COLLECTION) {
                        int max = 1000;
                        unordered_map<int, int> monsterTpl;
                        config.ForeachVector([&](const DataValue & k, const DataValue & v) {
                            const DataTable& row = v.ToTable();
                            int tplid = row.Get("monsterId")->ToInteger();
                            int count = row.Get("refreshCount")->ToInteger();  //数量最大值限制
                            if (count <= 0) {
                                return false;
                            } else if (count > max) {
                                count = max;
                            }
                            const MapUnitTpl* tpl = m_tploader->FindMapUnit(tplid);
                            if (tpl && tpl->IsEventMonster()) {
                                monsterTpl.emplace(tplid, count);
                            }
                            return false;
                        });
                        if (monsterTpl.size() > 0) {
                            MonsterCollectionActivity* aty = new MonsterCollectionActivity(id, openTime, closeTime, monsterTpl);
                            m_activities.emplace(id, aty);
                        }
                    } else if (type == ActivityType::MYSTERIOUS_CITY) {
                        // cout << "type == ActivityType::MYSTERIOUS_CITY" << endl;
                        MysteriousCityTpl tpl;
                        // param1.DumpInfo();
                        tpl.food = config.Get("food")->ToInteger();
                        tpl.wood = config.Get("wood")->ToInteger();
                        tpl.iron = config.Get("iron")->ToInteger();
                        tpl.stone = config.Get("stone")->ToInteger();
                        const DataTable& treasures = config.Get("treasures")->ToTable();
                        treasures.ForeachVector([&](int64_t k, const DataValue& v){
                            MysteriousCityTpl::Treasure trea;
                            const DataTable& treaTable = v.ToTable();
                            trea.tplid = treaTable.Get("itemId")->ToInteger();
                            trea.count = treaTable.Get("count")->ToInteger();
                            if (trea.count <= 0) {
                                return false;
                            }
                            trea.getMax = treaTable.Get("getMax")->ToInteger();
                            if (trea.getMax <= 0) {
                                return false;
                            }
                            tpl.treasures.push_back(trea);
                            return false;
                        });
                        tpl.existTime = config.Get("existTime")->ToInteger();
                        tpl.roundInterval = config.Get("roundInterval")->ToInteger();
                        tpl.refreshCount = config.Get("refreshCount")->ToInteger();
                        if (tpl.refreshCount > 10) {
                            tpl.refreshCount = 10;
                        }
                        tpl.refreshInterval = config.Get("refreshInterval")->ToInteger();
                        tpl.percent = config.Get("percent")->ToDouble();
                        tpl.Debug();
                        MysteriousCityActivity* aty = new MysteriousCityActivity(id, openTime, closeTime, tpl);
                        m_activities.emplace(id, aty);
                    } else if (type == ActivityType::GOBLIN_CAMP) {
                        // cout << "type == ActivityType::GOBLIN_CAMP" << endl;
                        int max = 30;
                        unordered_map<int, GoblinCampActTpl> goblinCampActTpls;
                        config.ForeachVector([&](const DataValue & k, const DataValue & v) {
                            const DataTable& row = v.ToTable();
                            int level = row.Get("goblinCampLevel")->ToInteger();
                            int count = row.Get("refreshCount")->ToInteger();  //数量最大值限制
                            if (count <= 0) {
                                return false;
                            } else if (count > max) {
                                count = max;
                            }
                            const MapUnitTpl* tpl = m_tploader->FindMapUnit(model::MapUnitType::GOBLIN_CAMP, level);
                            if (tpl && tpl->IsGoblinCamp()) {
                                GoblinCampActTpl tpl;
                                tpl.level = level;
                                tpl.count = count;
                                const DataTable& drops = row.Get("dropItems")->ToTable();
                                drops.ForeachVector([&](const DataValue & k, const DataValue & v) {
                                    const DataTable& row2 = v.ToTable();
                                    int groupId = row2.Get("groupId")->ToInteger();
                                    int itemId = row2.Get("itemId")->ToInteger();
                                    int min = row2.Get("min")->ToInteger();
                                    int max = row2.Get("max")->ToInteger();
                                    int rate = row2.Get("rate")->ToInteger();
                                    if (const ItemTpl* itemTpl = g_tploader->FindItem(itemId)) {
                                        if (groupId == 0) {
                                            tpl.dropTpl.items.emplace_back(*itemTpl, groupId, min, max, rate);
                                        } else {
                                            tpl.dropTpl.addToGroup(*itemTpl, groupId, min, max, rate);
                                        }
                                    }
                                    return false;
                                });
                                goblinCampActTpls.emplace(level, tpl);
                            }
                            return false;
                        });
                        GoblinCampActivity* aty = new GoblinCampActivity(id, openTime, closeTime, goblinCampActTpls);
                        m_activities.emplace(id, aty);
                    }*/
                    return false;
                });

                // remove activity
                for (auto it = m_activities.begin(); it != m_activities.end();) {
                    bool remove = true;
                    int64_t id = it->first;
                    Activity* aty = it->second;
                    for (int64_t v : ids) {
                        if (id == v) {
                            remove = false;
                        }
                    }

                    if (remove) {
                        if (aty->IsStart()) {
                            aty->End();
                        }
                        delete aty;
                        it = m_activities.erase(it);
                    } else {
                        ++it;
                    }
                }
            }

            Debug();
            
        }

        void ActivityMgr::CheckActivityStart()
        {
            // cout << "ActivityMgr::CheckActivityStart" << endl;
            int64_t now = g_dispatcher->GetTimestampCache();
            for (auto it = m_activities.begin(); it != m_activities.end();) {
                Activity* aty = it->second;
                if (!aty->IsStart()) {
                    if (aty->m_openTime < now && now < aty->m_closeTime) {
                        aty->Start();
                    }
                } else {
                    if (now >= aty->m_closeTime) {
                        if (aty->IsStart()) {
                            aty->End();
                        }
                        delete aty;
                        it = m_activities.erase(it);
                        continue;
                    }
                }
                ++it;
            }
        }

        void ActivityMgr::Debug()
        {
            /*
            cout << "############## ActivityMgr::Debug size = " << m_activities.size() << " ############" << endl;
            for (auto it = m_activities.begin(); it != m_activities.end(); ++it) {
                Activity* aty = it->second;
                printf("id=%ld, type=%d, state=%d, openTime=%ld, closeTime=%ld\n", aty->m_id, (int)aty->m_type, (int)aty->m_state, aty->m_openTime, aty->m_closeTime);
                if (aty->m_type == ActivityType::MONSTER_COLLECTION) {
                    cout << "monsterTpl: ";
                    MonsterCollectionActivity* mca = aty->ToMonsterCollection();
                    for (auto it2 = mca->monsterTpl().begin(); it2 != mca->monsterTpl().end(); ++it2) {
                        printf("[%d]=%d,", it2->first, it2->second);
                    }
                    cout << endl;
                }
            }
            cout << "#################### ActivityMgr::Debug END ###################" << endl;
            */
        }


    }
}
