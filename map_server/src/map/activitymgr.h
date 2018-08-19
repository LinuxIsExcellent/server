#ifndef MAP_ACTIVITYMGR_H
#define MAP_ACTIVITYMGR_H
#include <string>
#include <unordered_map>
#include <base/observer.h>

namespace base
{
	class DataTable;
}

namespace ms
{
	namespace map
	{
		namespace activity
		{
			class Activity;
		}

		class ActivityMgr
		{
		public:
			ActivityMgr() {}
			virtual ~ActivityMgr();

		public:
			void Init();
			void HandleMessage(const std::string& method, const base::DataTable& dt);

		private:
			activity::Activity* FindActivity(int64_t id) const {
				auto it = m_activities.find(id);
				return it != m_activities.end() ? it->second : nullptr;
			}
			void CheckActivityStart();
			void Debug();

		private:
			base::AutoObserver m_autoObserver;
			std::unordered_map<int64_t, activity::Activity*> m_activities;
		};

	}
}
#endif // MAP_ACTIVITYMGR_H
