#ifndef BASE_MEMORY_MEMORYPOOLMGR_H
#define BASE_MEMORY_MEMORYPOOLMGR_H

#include "memorypool.h"
#include <unordered_map>
#include <ostream>

namespace base
{
    class Framework;

    namespace memory
    {
        class MemoryPoolMgr
        {
        public:
            const std::unordered_map<std::string, MemoryPool*>& all_memory_pools() const {
                return all_;
            }
            MemoryPool* Query(const std::string& name) {
                auto it = all_.find(name);
                return it == all_.end() ? nullptr : it->second;
            }
            MemoryPool* Aquire(const std::string& name, uint32_t trunk_size, uint32_t pool_size);
            //
            void DumpStatistic();
            void DumpStatistic(std::ostream& out);

        private:
            MemoryPoolMgr();
            ~MemoryPoolMgr();
            std::unordered_map<std::string, MemoryPool*> all_;

            friend class base::Framework;
        };

        extern MemoryPoolMgr* g_memory_pool_mgr;
    }
}

#endif // MEMORYPOOLMGR_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
