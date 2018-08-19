#ifndef DAL_CHARACTER_H
#define DAL_CHARACTER_H

#include <string>
#include <unordered_map>
#include <base/utils/random.h>
#include <vector>

namespace client
{
    class PacketIn;
}

namespace dal
{
    struct AllianceScienceTpl {
        int tplid;
        int donationId;
        int donationCount;
    };
    
    struct ScenarioChapterTpl {
        int tplid;
        int type;
        std::vector<int> sectionList;
    };

    struct ScenarioSectionTpl {
        int tplid;
        int maxNum;
        int battleType;
    };

    class ClientImpl;
    class FrontServer;
    class Character
    {
    public:
        Character(ClientImpl *impl, FrontServer *fs);
        virtual ~Character();

        int64_t uid() const {
            return uid_;
        }
        
        int64_t namesuffix() const {
            return namesuffix_;
        }
        
        const std::string& username() const {
            return username_;
        }
        
        const std::string& nickname() const {
            return nickname_;
        }
        
        int32_t level() const {
            return level_;
        }
        
        int32_t stamina() const {
            return stamina_;
        }
        
        bool islogin() const {
            return islogin_;
        }
        
        int loginResult() const {
            return result_;
        }
        
        const std::unordered_map<int, int>& tplQuests() const {
            return tplQuests_;
        }
        
        const std::unordered_map<int, int>& tplSevenTargets() const {
            return tplSevenTargets_;
        }
        
        const std::unordered_map<int, AllianceScienceTpl>& tplAllianceSciences() const {
            return tplAllianceSciences_;
        }
        
        int FindQuest(int tplid) const {
            auto it = tplQuests_.find(tplid);
            return it != tplQuests_.end() ? it->second : 0;
        }

        int FindSevenTarget(int tplid) const {
            auto it = tplSevenTargets_.find(tplid);
            return it != tplSevenTargets_.end() ? it->second : 0;
        }

        const AllianceScienceTpl* FindAllianceScienceTpl(int tplid) const {
            auto it = tplAllianceSciences_.find(tplid);
            return it != tplAllianceSciences_.end() ? &it->second : nullptr;
        }
        
    public:
        // 随机数生成器
        base::utils::Random& random() {
            return random_;
        }
        
        void Login(uint32_t namesuffix, const std::string& username);
        void TemplateCheck();

    private:
        void HandleLoginResponse(client::PacketIn& pktin);
        void HandleTemplateUpdate(client::PacketIn& pktin);
        void HandleUserUpdate(client::PacketIn& pktin);
        void HandleReady(client::PacketIn& pktin);
        void HandleLogout(client::PacketIn& pktin);
        
        void HandlePing(client::PacketIn& pktin);
        void HandleSignUpdate(client::PacketIn& pktin);
        void HandleOnlineUpdate(client::PacketIn& pktin);
        void HandleMailUpdate(client::PacketIn& pktin);
        void HandleQuestUpdate(client::PacketIn& pktin);
        
    private:
        void AddJson(const std::string& name, const std::string& content) {
            auto it = jsons_.find(name);
            if (it != jsons_.end()) {
                it->second += content;
            } else {
                jsons_.emplace(name, content);
            }
        }
        void ParseTpl();

    private:
        bool islogin_;
        uint32_t result_;
        base::utils::Random random_;
        
        uint32_t namesuffix_ = 0;
        std::string username_;
        
        int64_t uid_ = 0;
        int64_t headId_ = 0;
        std::string nickname_;
        int32_t level_;
        int32_t stamina_;
        
        FrontServer* g_fs;
        
        // tpls
        std::unordered_map<std::string, std::string> jsons_;
        std::unordered_map<int, int> tplQuests_;                // <id, count>
        std::unordered_map<int, int> tplSevenTargets_;          // <id, count>
        std::unordered_map<int, AllianceScienceTpl> tplAllianceSciences_;

        std::unordered_map<int, ScenarioChapterTpl> tplScenarioChapter_;
        std::unordered_map<int, ScenarioSectionTpl> tplScenarioSection_;
    };
}

#endif // CHARACTER_H
