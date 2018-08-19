#include "command.h"
#include "../event/dispatcher.h"
#include "../logger.h"
#include "../utils/utils_string.h"

namespace base
{
    namespace command
    {
        using namespace std;

        void Command::Execute()
        {
            executed_ = true;
            m_beginExecuteTick = g_dispatcher->GetTickCache();
            OnCommandExecute();
        }

        void Command::OnCostTooMuchTimeWarning()
        {
            int64_t now = g_dispatcher->GetTickCache();
            if (now - m_lastWarningTick > 3 * 1000) {
                string name = utils::demangle_name(typeid(*this));
                LOG_WARN("execute command name=%s, cost too much time, up to %d seconds!", name.c_str(), (now - m_beginExecuteTick) / 1000);
                m_lastWarningTick = now;
            }
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
