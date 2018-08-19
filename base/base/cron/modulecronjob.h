#ifndef BASE_CRON_MODULECRONJOB_H
#define BASE_CRON_MODULECRONJOB_H

#include "../modulebase.h"

namespace base
{
    namespace cron
    {
        class CronJob;

        class ModuleCronJob : public ModuleBase
        {
        public:
            static ModuleCronJob* Create();
            virtual ~ModuleCronJob();

            virtual const char* GetObjectName() {
                return "base::cron::ModuleCronJob";
            }

        private:
            ModuleCronJob();
            virtual void OnModuleSetup();
            virtual void OnModuleCleanup();

            CronJob* cron_job_ = nullptr;
        };
    }
}

#endif
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
