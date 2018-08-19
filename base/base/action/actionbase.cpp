#include "actionbase.h"

namespace base
{
    namespace action
    {
        ActionBase::ActionBase()
            : stopped_(false), tag_(0)
        {

        }

        ActionBase::~ActionBase()
        {

        }
    }
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
