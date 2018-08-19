#include "memorypoolmgr.h"
#include <cassert>
#include <iostream>

namespace base
{
    namespace memory
    {
        using namespace std;

        MemoryPoolMgr* g_memory_pool_mgr = nullptr;

        MemoryPoolMgr::MemoryPoolMgr()
        {
            assert(g_memory_pool_mgr == nullptr);
            g_memory_pool_mgr = this;
        }

        MemoryPoolMgr::~MemoryPoolMgr()
        {
            for (auto it = all_.begin(); it != all_.end(); ++it) {
                delete it->second;
            }
            all_.clear();

            g_memory_pool_mgr = nullptr;
        }

        MemoryPool* MemoryPoolMgr::Aquire(const string& name, uint32_t chunk_size, uint32_t pool_size)
        {
            MemoryPool* exist = Query(name);
            if (exist != nullptr) {
                return exist;
            }

            MemoryPool* pool = new MemoryPool(chunk_size, pool_size);
            all_.emplace(name, pool);
            return pool;
        }

        void MemoryPoolMgr::DumpStatistic()
        {
            DumpStatistic(cout);
        }

        void MemoryPoolMgr::DumpStatistic(ostream& out)
        {
            out << "memory pool statistic:" << endl;
            for (auto it = all_.begin(); it != all_.end(); ++it) {
                MemoryPool* pool = it->second;
                out << "  " << it->first << ": free=" << pool->freesize() << "/" << pool->poolsize() << ", chunk_size=" << pool->chunksize() << endl;
            }
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
