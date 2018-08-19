#ifndef MAP_ACTIVITY_WORLDBOSSACTIVITY_H
#define MAP_ACTIVITY_WORLDBOSSACTIVITY_H
#include "activity.h"
#include <unordered_map>

namespace ms
{
    namespace map
    {

        class WorldBoss;

        namespace activity
        {
            class WorldBossActivity : public Activity
            {
            public:
                WorldBossActivity(int64_t id, int64_t openTime, int64_t closeTime, const std::unordered_map<int, int>& worldBossTpl)
                    : Activity(id, model::ActivityType::WORLD_BOSS, openTime, closeTime), m_worldBossTpl(worldBossTpl) {}
                virtual ~WorldBossActivity() {}

            private:
                virtual void OnStart() override;
                virtual void OnEnd() override;
            private:
                static constexpr int CREATE_COUNT_EVERY_TIMES = 2;
                std::unordered_map<int, int> m_worldBossTpl;
                std::unordered_map<int, int> m_worldBossTplTemp;
                std::vector<ms::map::WorldBoss*> m_worldBosses;  // 保存已创建的boss
                base::ObjectMaintainer m_maintainer;
            };
        }
    }
}

#endif // MAP_ACTIVITY_WORLDBOSSACTIVITY_H
