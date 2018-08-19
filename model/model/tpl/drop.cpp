#include "drop.h"
#include "item.h"
#include "templateloader.h"
#include <base/framework.h>
#include <base/data.h>
#include <set>

namespace model
{
    namespace tpl
    {
        using namespace std;
        using namespace base;

        void DropItem::WriteToPacketOut(base::gateway::PacketOutBase& pktout) const
        {
            pktout << tpl.id << count;
        }

        void AddToDrops(vector< DropItem >& drops, int tplid, int count)
        {
            if (count <= 0) {
                return;
            }
            if (const ItemTpl* tpl = g_tploader->FindItem(tplid)) {
                bool find = false;
                for (DropItem& d : drops) {
                    if (d.tpl.id == tplid) {
                        d.count += count;
                        find = true;
                        break;
                    }
                }
                if (!find) {
                    drops.push_back({*tpl, count});
                }
            }
        }

        void PushResourceToDrops(vector< DropItem >& drops, int m_food, int m_wood, int m_iron, int m_mithril, int m_gold)
        {
            if (const ItemTpl* tpl = g_tploader->FindItem((int)SpecialPropIdType::FOOD)) {
                if (m_food > 0) {
                    drops.push_back( {*tpl, m_food});
                }
            }
            if (const ItemTpl* tpl = g_tploader->FindItem((int)SpecialPropIdType::WOOD)) {
                if (m_wood > 0) {
                    drops.push_back( {*tpl, m_wood});
                }
            }
            if (const ItemTpl* tpl = g_tploader->FindItem((int)SpecialPropIdType::IRON)) {
                if (m_iron > 0) {
                    drops.push_back( {*tpl, m_iron});
                }
            }
            if (const ItemTpl* tpl = g_tploader->FindItem((int)SpecialPropIdType::STONE)) {
                if (m_mithril > 0) {
                    drops.push_back( {*tpl, m_mithril});
                }
            }
            if (const ItemTpl* tpl = g_tploader->FindItem((int)SpecialPropIdType::GOLD)) {
                if (m_gold > 0) {
                    drops.push_back( {*tpl, m_gold});
                }
            }
        }
        
        void MergeDrops(std::vector< DropItem >& drops1, const vector< DropItem >& drops2)
        {
            for (const DropItem& d2 : drops2) {
                bool find = false;
                for (DropItem& d1 : drops1) {
                    if (d1.tpl.id == d2.tpl.id) {
                        d1.count += d2.count;
                        find = true;
                        break;
                    }
                }
                if (!find) {
                    drops1.push_back(d2);
                }
            }
        }

        void DropTpl::addToGroup(const ItemTpl& t, int g, int min, int max, int p)
        {
            if (groups.empty()) {
                groups.resize(1);
                groups.at(0).emplace_back(t, g, min, max, p);
            } else {
                for (auto it = groups.begin(); it != groups.end(); ++it) {
                    vector<Item>& items = *it;
                    if (items.at(0).groupId == g) {
                        items.emplace_back(t, g, min, max, p);
                        return;
                    }
                }
                std::vector<Item> items;
                items.emplace_back(t, g, min, max, p);
                groups.push_back(items);
            }
        }

        int DropTpl::getDropRate(const DropItem& drop) const
        {
            for (auto it = items.begin(); it != items.end(); ++it) {
                if (drop.tpl.id == it->tpl.id) {
                    return it->probability;
                }
            }

            for (auto it = groups.begin(); it != groups.end(); ++it) {
                const vector<Item>& items = *it;
                for (size_t i = 0; i < items.size(); ++i) {
                    const Item& item = items.at(i);
                    if (drop.tpl.id == item.tpl.id) {
                        return item.probability;
                    }
                }
            }

            return 0;
        }

        std::vector< DropItem > DropTpl::DoDrop() const
        {
            vector<DropItem> drops;

            for (auto it = items.begin(); it != items.end(); ++it) {
                const Item& item = *it;

                if (item.probability < 10000) {
                    int r = framework.random().GenRandomNum(10000);
                    if (r >= item.probability) {
                        continue;
                    }
                }

                int count = item.countMin;
                if (item.countMin != item.countMax) {
                    count = framework.random().GenRandomNum(item.countMin, item.countMax + 1);
                }
                if (count == 0) {
                    continue;
                }

                drops.emplace_back(item.tpl, count);
            }

            vector<const Item*> temp;
            int total_probability = 0;
            //cout << "groups.size = " << groups.size() << endl;
            for (auto it = groups.begin(); it != groups.end(); ++it) {
                temp.clear();
                total_probability = 0;

                const vector<Item>& items = *it;
                for (size_t i = 0; i < items.size(); ++i) {
                    const Item& item = items.at(i);
                    if (item.probability == 0) {
                        continue;
                    }
                    temp.push_back(&item);
                    total_probability += item.probability;
                }

                if (total_probability > 0) {
                    int r = framework.random().GenRandomNum(total_probability);
                    int acc = 0;
                    for (auto it = temp.begin(); it != temp.end(); ++it) {
                        const Item& item = *(*it);
                        acc += item.probability;
                        if (acc - 1 >= r) {
                            int count = item.countMin;
                            if (item.countMin != item.countMax) {
                                count = framework.random().GenRandomNum(item.countMin, item.countMax + 1);
                            }
                            if (count > 0) {
                                drops.emplace_back(item.tpl, count);
                            }
                            break;
                        }
                    }
                }
            }

            return drops;
        }
        
        vector< int > DropTpl::DisplayItems() const
        {
            vector<int> result;
            set<int> temp;
            for (const Item& item : items) {
                temp.emplace(item.tpl.id);
            }
            for (const vector<Item>& group : groups) {
                for (const Item& item : group) {
                    temp.emplace(item.tpl.id);
                }
            }
            for (int tplid : temp) {
                result.push_back(tplid);
            }
            
            return result;
        }


        void SetDropsTable(base::DataTable& dropsTable, const vector< DropItem >& drops)
        {
            for (size_t i = 0; i < drops.size(); ++i) {
                DataTable dropTable;
                dropTable.Set("tplId", drops[i].tpl.id);
                dropTable.Set("count", drops[i].count);
                dropsTable.Set(i + 1, dropTable);
            }
        }
        
    }
}
