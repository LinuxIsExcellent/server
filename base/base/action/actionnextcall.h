#ifndef BASE_ACTION_ACTIONNEXTCALL_H
#define BASE_ACTION_ACTIONNEXTCALL_H

#include "actionbase.h"
#include <functional>

namespace base
{
    namespace action
    {
        // 将在下一帧执行的动作
        class ActionNextCall : public ActionBase
        {
        public:
            ActionNextCall();
            ActionNextCall(std::function<void()> cb);
            virtual ~ActionNextCall();

            virtual const char* GetObjectName() {
                return "base::action::ActionNextCall";
            }

        private:
            virtual void OnExecute();
            virtual void OnUpdate(int64_t tick, int32_t span);
            virtual bool IsDone();

            bool executed_;
            std::function<void()> cb_;
        };
    }
}

#endif // ACTIONNEXTCALL_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
