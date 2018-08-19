#ifndef BASE_COMMAND_COMMANDBASE_H
#define BASE_COMMAND_COMMANDBASE_H

#include "../global.h"
#include "../observer.h"
#include <functional>

namespace base
{
    namespace command
    {
        class CommandQueue;

        class Command
        {
        public:
            Command() : executed_(false), finish_(false) {}
            virtual ~Command() {}

            int64_t beginExecuteTick() const {
                return m_beginExecuteTick;
            }

            void Stop(bool noError = true) {
                m_noError = noError;
                finish_ = true;
            }

            void SetFinishCallback(std::function<void(bool)> cb) {
                m_finishCallback = cb;
            }

        protected:
            virtual void OnCostTooMuchTimeWarning();

            base::AutoObserver m_autoObserver;

        private:
            virtual bool OnSetup() {
                return true;
            }
            void Cleanup() {
                if (m_finishCallback) {
                    m_finishCallback(m_noError);
                    m_finishCallback = nullptr;
                }
                OnCleanup();
            }
            virtual void OnCleanup() {}

            void Execute();

            virtual void OnCommandExecute() = 0;

            bool executed_;
            int64_t m_beginExecuteTick = 0;
            bool finish_;
            bool m_noError = false;
            std::function<void(bool)> m_finishCallback;
            int64_t m_lastWarningTick = 0;

            friend class CommandQueue;
        };
    }
}

#endif // COMMANDBASE_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
