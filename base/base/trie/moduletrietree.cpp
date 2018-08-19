#include "moduletrietree.h"
#include "trietree.h"
#include "../framework.h"

namespace base
{
    namespace trie
    {
        ModuleTrieTree* ModuleTrieTree::Create()
        {
            ModuleTrieTree* obj = new ModuleTrieTree;
            obj->AutoRelease();
            return obj;
        }

        ModuleTrieTree::ModuleTrieTree(): ModuleBase("trie")
        {
        }

        ModuleTrieTree::~ModuleTrieTree()
        {
            TrieTree::DestroyInstance();
        }

        void ModuleTrieTree::OnModuleSetup()
        {
            TrieTree::CreateInstnace();
            g_trie->LoadFile((framework.resource_dir() + "/badword.txt").c_str());
            SetModuleState(MODULE_STATE_RUNNING);
        }

        void ModuleTrieTree::OnModuleCleanup()
        {
            TrieTree::DestroyInstance();
            SetModuleState(MODULE_STATE_DELETE);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
