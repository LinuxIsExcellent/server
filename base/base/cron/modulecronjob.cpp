#include "modulecronjob.h"
#include "cronjob.h"

namespace base
{
    namespace cron
    {
        ModuleCronJob::ModuleCronJob(): ModuleBase("cronjob")
        {
        }

        ModuleCronJob::~ModuleCronJob()
        {
            SAFE_DELETE(cron_job_);
        }

        ModuleCronJob* ModuleCronJob::Create()
        {
            ModuleCronJob* obj = new ModuleCronJob;
            obj->AutoRelease();
            return obj;
        }

        void ModuleCronJob::OnModuleSetup()
        {
            if (CronJob::instance() != nullptr) {
                SetModuleState(MODULE_STATE_DELETE);
                return;
            }

            cron_job_ = new CronJob;

            SetModuleState(MODULE_STATE_RUNNING);
        }

        void ModuleCronJob::OnModuleCleanup()
        {
            SAFE_DELETE(cron_job_);
            SetModuleState(MODULE_STATE_DELETE);
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
