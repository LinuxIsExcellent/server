#include "luavmpool.h"
#include "../logger.h"
#include "../event/dispatcher.h"
#include "../framework.h"

namespace base
{
    namespace lua
    {
        const int64_t LuaVmPool::SERVICE_EXPIRED_SECONDS = 3 * 60 * 60;          // 3 hours

        LuaVmPool::LuaVmPool(int id, int minSize, int maxSize)
            : m_id(id), m_minSize(minSize), m_maxSize(maxSize)
        {
        }

        LuaVmPool::~LuaVmPool()
        {
            for (LuaVm * vm : m_expiredVmArray) {
                vm->Release();
            }
            m_expiredVmArray.clear();
            for (LuaVm * vm : m_vmArray) {
                vm->Release();
            }
            m_vmArray.clear();
        }

        void LuaVmPool::GenerateAllVm(const std::string& boostrapScript)
        {
            m_boostrapScript = boostrapScript;
            for (auto it = m_vmArray.begin(); it != m_vmArray.end(); ++it) {
                LuaVm* vm = *it;
                vm->setExpired();
                m_expiredVmArray.push_back(vm);
            }
            m_vmArray.clear();

            for (int i = 0; i < m_minSize; ++i) {
                LuaVm* vm = new LuaVm();
                vm->Bootstrap(m_boostrapScript);
                m_vmArray.push_back(vm);
            }
        }

        LuaVm* LuaVmPool::AquireVm()
        {
            for (auto it = m_vmArray.begin(); it != m_vmArray.end(); ++it) {
                LuaVm* vm = *it;
                if (vm->state() == LuaVmState::OK) {
                    return vm;
                }
            }
            return nullptr;
        }

        void LuaVmPool::IntervalCheck()
        {
            /*if(m_maxSize == 1) {
                return;
            }

            int workingSize = 0;
            int64_t now = g_dispatcher->GetTimestampCache();
            std::vector<LuaVm*> temp;
            for (auto it = m_vmArray.begin(); it != m_vmArray.end(); ++it) {
                LuaVm* vm = *it;
                if (vm->state() == LuaVmState::OK) {
                    if (vm->createTime() > 0 && now - vm->createTime() >= SERVICE_EXPIRED_SECONDS) {
                        vm->setExpired();
                        LOG_WARN("create new LuaVm ...");
                        LuaVm* newVm = new LuaVm();
                        newVm->Bootstrap(m_boostrapScript);
                        temp.push_back(newVm);
                    } else {
                        ++workingSize;
                    }
                }
            }
            if(workingSize > 0) {
                for (auto it = m_vmArray.begin(); it != m_vmArray.end();) {
                    LuaVm* vm = *it;
                    if (vm->createTime() == 0) {
                        m_expiredVmArray.push_back(vm);
                        it = m_vmArray.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
            for (auto it = temp.begin(); it != temp.end(); ++it) {
                m_vmArray.push_back(*it);
            }

            for (auto it = m_expiredVmArray.begin(); it != m_expiredVmArray.end();) {
                LuaVm* vm = *it;
                if (vm->reference_count() == 1) {
                    LOG_WARN("delete expired LuaVm ...");
                    it = m_expiredVmArray.erase(it);
                    vm->Release();
                } else {
                    ++it;
                }
            }*/
        }

    }
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
