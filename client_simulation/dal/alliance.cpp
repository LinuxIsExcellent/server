#include "alliance.h"
#include "frontserver.h"
#include "clientimpl.h"
#include "character.h"
#include <sstream>
#include <base/event/dispatcher.h>

namespace dal
{
    using namespace std;
    using namespace model;

    Alliance::Alliance(ClientImpl *impl, FrontServer *fs) : g_fs(fs)
    {
        HANDLE_MAP(model::SC::ALLIANCE_INFO_UPDATE, Alliance::HandleAllianceUpdate);
        HANDLE_MAP(model::SC::ALLIANCE_SEARCH_RESPONSE, Alliance::HandleAllianceSearchResponse);
        
        HANDLE_MAP(model::SC::ALLIANCE_SCIENCE_UPDATE, Alliance::HandleAllianceScienceUpdate);
        HANDLE_MAP(model::SC::ALLIANCE_HELP_UPDATE, Alliance::HandleAllianceHelpUpdate);
    }

    Alliance::~Alliance()
    {
    }

    void Alliance::CheckCreate()
    {
        int r = g_fs->character()->random().GenRandomNum(10000);
        if(r > 3000) {
            return;
        }
        // 前100人创建，其它人加入
        if (aid_ == 0 && g_fs->character()->namesuffix() <= 100) {
            const std::string& name = g_fs->character()->username();
            std::string nickname = name.substr(name.length() - 3, 3);

            client::PacketOut pktout((uint16_t)model::CS::ALLIANCE_CREATE, 20, g_fs->mempool());
            pktout.WriteString(name);
            pktout.WriteString(nickname);
            pktout.WriteVarInteger<int>(0); // all language
            g_fs->Send(pktout);
            
            printf("### ALLIANCE_CREATE \n");
        }
    }
    
     void Alliance::CheckQuit()
    {
        int r = g_fs->character()->random().GenRandomNum(10000);
        if(r > 10) {
            return;
        }
        
        client::PacketOut pktout((uint16_t)model::CS::ALLIANCE_QUIT, 20, g_fs->mempool());
        g_fs->Send(pktout);
        printf("### ALLIANCE_QUIT \n");
    }
    
    void Alliance::Join()
    {
        // 5个人有1个人不加入联盟
        if(g_fs->character()->namesuffix() <= 100 || g_fs->character()->namesuffix() % 5 == 0 || g_fs->character()->namesuffix() % 4 == 0) {
            return;
        }
        // 不要一下子全加入了  慢慢的有一些人加入到满为止
        int r = g_fs->character()->random().GenRandomNum(10000);
        if(r > 1000) {
            return;
        }
        
        std::string name = "";
        client::PacketOut pktout((uint16_t)model::CS::ALLIANCE_SEARCH, 20, g_fs->mempool());
        pktout.WriteString(name);
        pktout.WriteVarInteger<int>(0); // all language
        g_fs->Send(pktout);
        printf("### ALLIANCE_SEARCH \n");
    }
    
    void Alliance::Explore()
    {
        int r = g_fs->character()->random().GenRandomNum(10000);
        if(r > 7000) {
            return;
        }
        
        if (aid_ == 0) {
            Join();
            return;
        }
        if(m_explores.size() == 0) {
            return;
        }
        for (auto it = m_explores.begin(); it != m_explores.end(); ++it) {
            if (it->second.accept) {
                return;
            }
        }
        
        int exploreId = 0;
        for (auto it = m_explores.begin(); it != m_explores.end(); ++it) {
            if (!it->second.accept) {
                exploreId = it->first;
                break;
            }
        }
        if (exploreId > 0) {
            //client::PacketOut pktout((uint16_t)model::CS::ALLIANCE_EXPLORE_ACCEPT, 20, g_fs->mempool());
            //pktout.WriteVarInteger<int>(exploreId);
            //g_fs->Send(pktout);
            //printf("### Explore \n");
        }
    }

    void Alliance::Help()
    {
        int r = g_fs->character()->random().GenRandomNum(10000);
        if(r > 7000) {
            return;
        }
        
        if (aid_ == 0) {
            Join();
            return;
        }
        
        r = g_fs->character()->random().GenRandomNum(10000);
        if(r > 5000) {
            client::PacketOut pktout((uint16_t)model::CS::ALLIANCE_HELP_ALL, 20, g_fs->mempool());
            g_fs->Send(pktout);
            printf("### ALLIANCE_HELP_ALL \n");
        } else {
            int helpId = 0;
            r = g_fs->character()->random().GenRandomNum(1, m_helps.size() + 1);
            int index = 1;
            for(auto it = m_helps.begin(); it != m_helps.end(); ++it) {
                if(index == r) {
                    helpId = it->first;
                    break;
                }
                index++;
            }
            if(helpId > 0) {
                client::PacketOut pktout((uint16_t)model::CS::ALLIANCE_HELP_ONE, 20, g_fs->mempool());
                pktout.WriteVarInteger<int>(helpId);
                g_fs->Send(pktout);
                printf("### ALLIANCE_HELP_ONE \n");
            }
        }
    }

    void Alliance::Donate()
    {
        int r = g_fs->character()->random().GenRandomNum(10000);
        if(r > 7000) {
            return;
        }
        
        if (aid_ == 0) {
            Join();
            return;
        }
        
        {
            int scienceId = g_fs->character()->random().GenRandomNum(2903101, 2903106 + 1);
            int donationId = 0;
            const std::unordered_map<int, AllianceScienceTpl>& list = g_fs->character()->tplAllianceSciences();
            if(list.find(scienceId) != list.end()) {
                const AllianceScienceTpl& it = list.at(scienceId);
                if(m_sciences.find(it.tplid) != m_sciences.end() && m_sciences.at(it.tplid) < 10) {
                    donationId = it.donationId;
                    
                    client::PacketOut pktout((uint16_t)model::CS::ALLIANCE_SCIENCE_DONATE, 20, g_fs->mempool());
                    pktout.WriteVarInteger<int>(scienceId);
                    pktout.WriteVarInteger<int>(donationId);
                    g_fs->Send(pktout);
                    printf("### ALLIANCE_SCIENCE_DONATE \n");
                }
            }
        }

        r = g_fs->character()->random().GenRandomNum(10000);
        if(r < 300) {
            //client::PacketOut pktout((uint16_t)model::CS::ALLIANCE_SCIENCE_DONATE_CD_CLEAR, 20, g_fs->mempool());
            //g_fs->Send(pktout);
        }
    }

    void Alliance::HandleAllianceUpdate(client::PacketIn& pktin)
    {
        aid_ = pktin.ReadVarInteger<int64_t>();
        name_ = pktin.ReadString();
        nickname_ = pktin.ReadString();
        leaderId_ = pktin.ReadVarInteger<int64_t>();
        leaderName_ = pktin.ReadString();
        leaderHeadId_ = pktin.ReadVarInteger<int64_t>();
        bannerId_ = pktin.ReadVarInteger<int>();
        alliesCount_ = pktin.ReadVarInteger<int>();
        alliesMax_ = pktin.ReadVarInteger<int>();
        // others ...
        
        printf("### HandleAllianceUpdate aid=%d, name=%s, nickname=%s, count=%d, max=%d \n", aid_, name_.c_str(), nickname_.c_str(), alliesCount_, alliesMax_);
    }

    void Alliance::HandleAllianceSearchResponse(client::PacketIn& pktin)
    {
        int len = pktin.ReadVarInteger<int>(); // max 100
        for (int i = 0; i < len; ++i) {
            int64_t aid = pktin.ReadVarInteger<int64_t>();
            std::string name = pktin.ReadString();
            std::string nickname = pktin.ReadString();
            int64_t leaderId = pktin.ReadVarInteger<int64_t>();
            std::string leaderName = pktin.ReadString();
            int64_t leaderHeadId = pktin.ReadVarInteger<int64_t>();
            int bannerId = pktin.ReadVarInteger<int>();
            int alliesCount = pktin.ReadVarInteger<int>();
            int alliesMax = pktin.ReadVarInteger<int>();
            int totalPower = pktin.ReadVarInteger<int>();
            int LangType = pktin.ReadVarInteger<int>();
            std::string slogan = pktin.ReadString();
            std::string rankTitle = pktin.ReadString();
            
            int r = g_fs->character()->random().GenRandomNum(10, 36);
            if (alliesCount < r) {
                client::PacketOut pktout((uint16_t)model::CS::ALLIANCE_APPLY, 20, g_fs->mempool());
                pktout.WriteVarInteger<int64_t>(aid);
                g_fs->Send(pktout);
                break;
            }
        }
        
        printf("### HandleAllianceSearchResponse len=%d \n", len);
    }
    
    void Alliance::HandleAllianceScienceUpdate(client::PacketIn& pktin)
    {
        int len = pktin.ReadVarInteger<int>();
        for (int i = 0; i < len; ++i) {
            int groupId = pktin.ReadVarInteger<int>();
            int tplId = pktin.ReadVarInteger<int>();
            int level = pktin.ReadVarInteger<int>();
            int exp = pktin.ReadVarInteger<int>();
            m_sciences[tplId] = level;
        }
        //honor_ = pktin.ReadVarInteger<int>();
        //donateCDTime_ = pktin.ReadVarInteger<int>();
        //donateGoldCount_ = pktin.ReadVarInteger<int>();
        
        printf("### HandleAllianceScienceUpdate len=%d \n", len);
    }

    void Alliance::HandleAllianceHelpUpdate(client::PacketIn& pktin)
    {
        // helps
        int len1 = pktin.ReadVarInteger<int>();
        for (int i = 0; i < len1; ++i) {
            int helpId = pktin.ReadVarInteger<int>();
            int buildingId = pktin.ReadVarInteger<int>();
            int times = pktin.ReadVarInteger<int>();
            int max = pktin.ReadVarInteger<int>();
            int64_t uid = pktin.ReadVarInteger<int64_t>();
            std::string nickname = pktin.ReadString();
            int64_t headId = pktin.ReadVarInteger<int64_t>();
            std::string helpInfo = pktin.ReadString();
            m_helps[helpId] = times >= max;
        }
        // deletes
        int len2 = pktin.ReadVarInteger<int>();
        for (int i = 0; i < len2; ++i) {
            int helpId = pktin.ReadVarInteger<int>();
            if(m_helps.find(helpId) != m_helps.end()) {
                m_helps.erase(helpId);
            }
        }
        
        printf("### HandleAllianceHelpUpdate helps=%d, deletes=%d \n", len1, len2);
    }
}

