#ifndef MODEL_TPL_BATTLE_ARRT_TRANSFORM_H
#define MODEL_TPL_BATTLE_ARRT_TRANSFORM_H
#include "../metadata.h"
#include <vector>
#include "item.h"

namespace model
{
    namespace tpl
    {
        struct BattleArrtTransformTpl {
            int id;
            int sceondArrt;
            int firstArrt;
            float transformNum;
        };

        struct BattleArrtTpl {
        	int type;
        	int battlePower;
        };
    }
}

#endif // MODEL_TPL_BATTLE_ARRT_TRANSFORM_H
