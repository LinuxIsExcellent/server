#ifndef MAP_ACTIVITY_ACTIVITY_H
#define MAP_ACTIVITY_ACTIVITY_H
#include <model/metadata.h>
#include <stdint.h>
#include <base/objectmaintainer.h>

namespace ms
{
    namespace map
    {

        class ActivityMgr;
        namespace activity
        {

            class MonsterCollectionActivity;
            enum ActivityState {
                INIT = 0,
                START = 1,
                END = 2,
            };

            class Activity
            {
            public:
                Activity(int64_t id, model::ActivityType type, int64_t openTime, int64_t closeTime)
                    : m_id(id), m_type(type), m_openTime(openTime), m_closeTime(closeTime) {}
                virtual ~Activity() {}

                bool IsStart() const {
                    return m_state == START;
                }

            public:
                void Start();
                void End();

            private:
                virtual void OnStart() = 0;
                virtual void OnEnd() = 0;

            protected:
                base::ObjectMaintainer m_maintainer;

            private:
                int64_t m_id = 0;
                model::ActivityType m_type;
                int64_t m_openTime = 0;
                int64_t m_closeTime = 0;

                ActivityState m_state = INIT;

                friend class ms::map::ActivityMgr;
            };
        }
    }
}
#endif // MAP_ACTIVITY_ACTIVITY_H
