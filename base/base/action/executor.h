#ifndef BASE_ACTION_EXECUTOR_H
#define BASE_ACTION_EXECUTOR_H

#include "../object.h"
#include "../utils/intrusive_list.h"
#include <functional>

namespace base
{
    namespace action
    {
        class ActionBase;

        // Action执行器
        class Executor : public Object
        {
            INTRUSIVE_LIST(Executor)
        public:
            Executor();

            utils::IntrusiveList<ActionBase>& actions() {
                return actions_;
            }

            virtual const char* GetObjectName() {
                return "base::action::Executor";
            }

        protected:
            virtual ~Executor();

        public:
            ActionBase* GetActionByTag(int32_t tag);
            void RunAction(ActionBase* act);
            void StopAllAction();
            void BeginStopAllAction(std::function<void()> cb);

        private:
            void OnAllActionFinish();
            ActionBase* EraseAction(ActionBase* act);
            utils::IntrusiveList<ActionBase> actions_;
            std::function<void()> cb_stop_;
            bool registered_;
            friend class ActionManager;
        };
    }
}
#endif // NODEBASE_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
