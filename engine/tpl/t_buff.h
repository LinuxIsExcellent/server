#ifndef ENGINE_MODEL_T_BUFF_H
#define ENGINE_MODEL_T_BUFF_H
#include <stdint.h>
#include <vector>
#include "../metadata.h"

namespace engine
{
    namespace tpl
    {
        struct T_Buff {
            int id;
            BuffTarget target;
            BuffType type;
            BuffValueType valueType;
            float base_value1;
            float level_value1;
            int extra_value;
            int rounds;
            TriggerBuffState trigger_state; //触发状态  
        };

        struct T_Effect {
            int id;
            EffectCondType condType;
            EffectRangeType range;
            EffectType type;
            float base_value;
            float up_value;
        };
    }
}
#endif // ENGINE_MODEL_T_BUFF_H
