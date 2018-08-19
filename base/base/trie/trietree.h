#ifndef BASE_TRIE_TRIETREE_H
#define BASE_TRIE_TRIETREE_H

#include "../global.h"
#include <string>
#include <vector>

namespace base
{
    namespace trie
    {
        class TrieChildrenCollection;

        // 关键字过滤系统
        // 注意仅支持utf8格式的字符串
        // 环境变量需要先设置为
        // setlocale(LC_ALL, "en_US.utf8");
        class TrieTree
        {
            DISABLE_COPY(TrieTree)
        public:
            static void CreateInstnace();
            static void DestroyInstance();

            // 从文件中加载脏字
            bool LoadFile(const char* filename, char sep = ',');
            // 添加一个脏字
            void AddWord(const std::string& w);

            // 检查是否包含脏字
            bool IsContain(const std::string& w) const;
            // 过滤掉脏字
            void Filter(std::string& data, char rep = '*') const;

        private:
            TrieTree();
            ~TrieTree();
            TrieChildrenCollection* root_;
        };

        extern TrieTree* g_trie;
    }
}

#endif // TRIETREE_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
