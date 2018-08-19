#include "t_army.h"

namespace engine
{
    namespace tpl
    {
        void T_Army::Init()
        {
            for (int i = 0; i < (int)armyRestrain.size(); ++i) {
                armyRestrain[i] = 0.0f;
            }
            
            for (auto &value : armyEffects) {
                if (value.armyType > 0 && value.armyType <=  (int)armyEffects.size()) {
                    armyRestrain.at(value.armyType - 1) = value.percentage;
                }
            }
        }

    }
}
