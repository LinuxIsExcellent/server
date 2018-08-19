#ifndef MODEL_TPL_DROP_H
#define MODEL_TPL_DROP_H
#include "../metadata.h"
#include <base/cluster/message.h>
#include <string>
#include <vector>

namespace base
{
    class DataTable;
}

namespace model
{
    namespace tpl
    {
        struct ItemTpl;

        struct DropItem {
        public:
            DropItem(const ItemTpl& t, int c) : tpl(t), count(c) {}

            const ItemTpl& tpl;
            int count;

            DropItem(const DropItem& rhs) : tpl(rhs.tpl), count(rhs.count) {
            }

            void WriteToPacketOut(base::gateway::PacketOutBase& pktout) const;
        };

        struct DropTpl {
            struct Item {
                Item(const ItemTpl& t, int g, int min, int max, int p)
                    : tpl(t), groupId(g), countMin(min), countMax(max), probability(p) {}
                const ItemTpl& tpl;
                int groupId;
                int countMin;
                int countMax;
                int probability;
            };

            int dropId;
            std::vector<Item> items;
            std::vector<std::vector<Item>> groups;

            // 同组仅抽取一个
            void addToGroup(const ItemTpl& t, int g, int min, int max, int p);

            // 获取掉落几率
            int getDropRate(const DropItem& drop) const;

            // 进行掉落
            std::vector<DropItem> DoDrop() const;
            
            // 物品一览
            std::vector<int> DisplayItems() const;
        };
        
        void AddToDrops(std::vector<DropItem>& drops, int tplid, int count);
        void PushResourceToDrops(std::vector<DropItem>& drops, int m_food, int m_wood, int m_iron, int m_mithril, int m_gold);
        void MergeDrops(std::vector<DropItem>& drops1, const std::vector<DropItem>& drops2);
        void SetDropsTable(base::DataTable& dropsTable, const std::vector<model::tpl::DropItem>& drops);
            
    }
}
#endif // MAPUNITTPL_H
