#ifndef ENGINE_MODEL_T_SPELL_H
#define ENGINE_MODEL_T_SPELL_H
#include <vector>

namespace engine
{
    namespace tpl
    {
        struct T_SpellNode {
            int id;
            int typeExt;                    // 1逃跑 2伪退
            std::vector<int> buffIds;
            int rage;
        };

        struct T_SpellBase {
            int id;
            int singleNodeId;
            int mixNodeId;
            bool active;
            bool super;
            int weight;
        };
    }
}

#endif // ENGINE_MODEL_T_SPELL_H
