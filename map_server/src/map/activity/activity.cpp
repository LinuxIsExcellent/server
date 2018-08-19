#include "activity.h"
#include <base/logger.h>

namespace ms
{
    namespace map
    {
        namespace activity
        {
            void Activity::Start()
            {
                LOG_DEBUG("Activity::Start id=%ld,type=%d,openTime=%ld,closeTime=%ld\n", m_id, (int)m_type, m_openTime, m_closeTime);
                m_state = START;
                OnStart();
            }

            void Activity::End()
            {
                LOG_DEBUG("Activity::End id=%ld,type=%d,openTime=%ld,closeTime=%ld\n", m_id, (int)m_type, m_openTime, m_closeTime);
                m_state = END;
                OnEnd();
            }

        }
    }
}
