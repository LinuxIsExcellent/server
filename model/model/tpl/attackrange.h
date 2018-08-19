#ifndef MODEL_TPL_ATTACK_RANGE_H
#define MODEL_TPL_ATTACK_RANGE_H
#include "../metadata.h"
#include <vector>
#include <unordered_map>
#include "item.h"

namespace model
{
    namespace tpl
    {
        struct AttackRangeTpl {
            int id = 0;
            std::unordered_map<int, std::vector<int>> m_attack_matrix;
        };
    }
}

#endif // MODEL_TPL_ATTACK_RANGE_H
