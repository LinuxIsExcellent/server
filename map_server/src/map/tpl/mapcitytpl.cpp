#include "mapcitytpl.h"
#include <base/framework.h>

namespace ms
{
    namespace map
    {
        namespace tpl
        {
            using namespace base;

            const CityPatrolEventTpl* CityPatrolEvtGroupTpl::GetRandomEvent(bool isNormal) const
            {
                const CityPatrolEventTpl* evtTpl = nullptr;
                if (isNormal) {
                    int rand = framework.random().GenRandomNum(normalTotalWeight);
                    int temp = 0;
                    for (auto evt : normalEvents) {
                        if (evt) {
                            temp +=  evt->weight;
                            if (temp > rand) {
                                evtTpl = evt;
                                break;
                            }
                        }
                    }
                } else {
                    int rand = framework.random().GenRandomNum(rewardTotalWeight);
                    int temp = 0;
                    for (auto evt : rewardEvents) {
                        if (evt) {
                            temp +=  evt->weight;
                            if (temp > rand) {
                                evtTpl = evt;
                                break;
                            }
                        }
                    }
                }

                return evtTpl;
            }

        }
    }
}