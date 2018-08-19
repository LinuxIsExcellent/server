#ifndef BASE_MODULE_TRIETREE_H
#define BASE_MODULE_TRIETREE_H

#include "../modulebase.h"

namespace base
{
    namespace trie
    {
        class ModuleTrieTree : public ModuleBase
        {
        public:
            static ModuleTrieTree* Create();
            ~ModuleTrieTree();

        private:
            ModuleTrieTree();

            virtual void OnModuleSetup();
            virtual void OnModuleCleanup();
        };
    }
}

#endif
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
