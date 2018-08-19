#include "trietree.h"
#include <cwchar>
#include <cstring>
#include "../logger.h"

namespace base
{
    namespace trie
    {
        using namespace std;

        TrieTree* g_trie = NULL;

        /// TrieNode

        struct TrieNode {
            TrieNode();
            ~TrieNode();

            wchar_t code;
            bool is_tail;
            TrieChildrenCollection* children;
        };

        /// TrieChildrenCollection

        class TrieChildrenCollection
        {
        public:
            ~TrieChildrenCollection() {
                for (std::size_t i = 0u; i < children_.size(); ++i) {
                    delete children_[i];
                }
                children_.clear();
            }

            bool empty() const {
                return children_.empty();
            }

            TrieNode* Find(wchar_t code) const {
                for (std::size_t i = 0u; i < children_.size(); ++i) {           // 可优化为二分查找 TODO
                    if (children_[i]->code == code) {
                        return children_[i];
                    }
                }
                return NULL;
            }

            TrieNode* BuildNode(wchar_t code, bool is_tail) {
                TrieNode* n = NULL;
                for (std::size_t i = 0u; i < children_.size(); ++i) {           // 可优化为二分查找
                    if (children_[i]->code == code) {
                        n = children_[i];
                        break;
                    }
                }
                if (n == NULL) {
                    n = new TrieNode;
                    n->code = code;
                    n->is_tail = is_tail;
                    children_.push_back(n);
                }
                if (is_tail && !n->is_tail) {
                    n->is_tail = is_tail;
                }
                return n;
            }

        private:
            std::vector<TrieNode*> children_;
        };

        TrieNode::TrieNode() : code(0), is_tail()
        {
            children = new TrieChildrenCollection;
        }

        TrieNode::~TrieNode()
        {
            delete children;
        }

        /// TrieTree

        void TrieTree::CreateInstnace()
        {
            if (g_trie == NULL) {
                g_trie = new TrieTree;
            }
        }

        void TrieTree::DestroyInstance()
        {
            if (g_trie != NULL) {
                delete g_trie;
                g_trie = NULL;
            }
        }

        TrieTree::TrieTree()
        {
            root_ = new TrieChildrenCollection;
        }

        TrieTree::~TrieTree()
        {
            delete root_;
        }

        bool TrieTree::LoadFile(const char* filename, char sep)
        {
            FILE* f = fopen(filename, "r");
            if (f == NULL) {
                return false;
            }
            string word;
            char c;
            int total = 0;
            do {
                c = fgetc(f);
                if (c == ' ' || c == '\r') {
                    ;
                } else {
                    if (c == sep || c == EOF || c == '\n') {
                        if (!word.empty()) {
                            AddWord(word);
                            word.clear();
                        }
                    } else {
                        word.push_back(c);
                        ++total;
                    }
                }
            } while (c != EOF);
            fclose(f);
            LOG_DEBUG("load badword, total=%d\n", total);
            return true;
        }

        void TrieTree::AddWord(const std::string& w)
        {
            TrieChildrenCollection* collect = root_;
            mbstate_t state = mbstate_t();
            const char* ptr = w.data();
            const char* end = w.data() + w.length();
            int len;
            wchar_t wc;
            while ((len = std::mbrtowc(&wc, ptr, end - ptr, &state)) > 0) {
                TrieNode* node = collect->BuildNode(wc, ptr + len == end);
                collect = node->children;
                ptr += len;
            }
        }

        bool TrieTree::IsContain(const string& w) const
        {
            mbstate_t state = mbstate_t();
            size_t len;
            wchar_t wc;
            bool revert = false;
            const char* begin = w.data();
            const char* end = w.data() + w.length();
            const char* ptr = begin;
            const TrieChildrenCollection* collect = root_;
            int processed = 0;
            while (ptr != end && (len = std::mbrtowc(&wc, ptr, end - ptr, &state)) > 0) {
                revert = false;
                TrieNode* node = collect->Find(wc);
                if (node == NULL) {
                    revert = true;
                } else {
                    if (node->is_tail) {
                        return true;
                    }
                    if (!node->children->empty()) {
                        collect = node->children;
                    } else {
                        revert = true;
                    }
                }
                if (revert) {
                    ++processed;
                    collect = root_;
                    ptr = begin + processed;
                } else {
                    ptr += len;
                }
            }
            return false;
        }

        void TrieTree::Filter(string& w, char rep) const
        {
            mbstate_t state = mbstate_t();
            int len;
            wchar_t wc;
            bool revert = false;
            bool found = false;
            const char* begin = w.data();
            const char* end = w.data() + w.length();
            const char* ptr = begin;
            const TrieChildrenCollection* collect = root_;
            int processed = 0;
            while (ptr != end) {
                len = std::mbrtowc(&wc, ptr, end - ptr, &state);
                if (len <= 0) {
                    break;
                }
                found = false;
                revert = false;
                if (processed == 0) {
                    processed = len;
                } else {
                    processed += len;
                }
                TrieNode* node = collect->Find(wc);
                if (node == NULL) {
                    revert = true;
                } else {
                    if (node->is_tail) {
                        found = true;
                        revert = true;
                    } else {
                        if (!node->children->empty()) {
                            collect = node->children;
                        } else {
                            revert = true;
                        }
                    }
                }
                if (found) {
                    const char* p = begin;
                    for (int i = 0; i < processed; ++i) {
                        char* c = (char*)(p + i);
                        *c = rep;
                    }
                }
                if (revert) {
                    collect = root_;
                    ptr = begin + processed;
                    begin = ptr;
                    processed = 0;
                } else {
                    ptr += len;
                }
            }
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
