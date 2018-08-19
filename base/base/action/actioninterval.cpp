#include "actioninterval.h"

namespace base
{
    namespace action
    {
        ActionInterval::ActionInterval(int32_t interval)
            : interval_(interval), acc_(0)
        {
        }

        ActionInterval::~ActionInterval()
        {
        }

        void ActionInterval::OnUpdate(int64_t tick, int32_t span)
        {
            acc_ += span;
            if (acc_ >= interval_) {
                OnInterval(tick, acc_);
                acc_ = 0;
            }
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
