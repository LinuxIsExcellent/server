#include "character.h"
#include "frontserver.h"
#include "clientimpl.h"
#include <iostream>
#include <unistd.h>
#include <base/utils/utils_string.h>
#include <base/utils/file.h>
#include <base/utils/crypto.h>
#include <base/3rd/rapidjson/document.h>

namespace dal
{
    using namespace std;
    using namespace client;
    using namespace base::utils;
    
    Character::Character(ClientImpl *impl, FrontServer *fs) : g_fs(fs),  islogin_(false), username_("")
    {
        HANDLE_MAP(model::SC::LOGIN_RESPONSE, Character::HandleLoginResponse);
        HANDLE_MAP(model::SC::TEMPLATE_UPDATE, Character::HandleTemplateUpdate);
        HANDLE_MAP(model::SC::USER_UPDATE, Character::HandleUserUpdate);
        HANDLE_MAP(model::SC::READY, Character::HandleReady);
        HANDLE_MAP(model::SC::LOGOUT, Character::HandleLogout);
        
        HANDLE_MAP(model::SC::PING, Character::HandlePing);
        HANDLE_MAP(model::SC::MISC_SIGN_UPDATE, Character::HandleSignUpdate);
        HANDLE_MAP(model::SC::MISC_ONLINE_UPDATE, Character::HandleOnlineUpdate);
        //HANDLE_MAP(model::SC::MAIL_UPDATE, Character::HandleMailUpdate);
        HANDLE_MAP(model::SC::QUEST_UPDATE, Character::HandleQuestUpdate);
        
        ParseTpl();
    }

    Character::~Character()
    {
    }

    void Character::Login(uint32_t namesuffix, const std::string& username)
    {
        result_ = 9999;
        namesuffix_ = namesuffix,
        username_ = username;

        static int num = 0;
        client::PacketOut pktout((uint16_t)model::CS::LOGIN, 20, g_fs->mempool());
        pktout.WriteString("client_test"); // sdk type
        pktout.WriteString(username);
        pktout.WriteString("token");    
        pktout.WriteString(""); // idfa
        pktout.WriteString("test"); // channel
        pktout.WriteString("1.0.0"); // vsn
        pktout.WriteVarInteger(0); // lang type
        pktout.WriteBoolean(false); // recon
        g_fs->Send(pktout);
    }
    
    void Character::TemplateCheck()
    {
        string dir = g_fs->path() + "/tpl";
        vector<string> files = dir_get_files(dir.c_str(), 1);
        client::PacketOut pktout((uint16_t)model::CS::TEMPLATE_CHECK, 30, g_fs->mempool());
        pktout.WriteVarInteger(files.size());
        for (const string& file : files) {
            string fileName = file.substr(dir.length() + 1, file.length());
            string content = file_get_content(file.c_str());
            string hash = base::utils::sha1hex(content.c_str(), content.length());
            //cout << fileName << " " << hash << endl;
            pktout.WriteString(fileName);
            pktout.WriteString(hash);
        }
        g_fs->Send(pktout);
    }

    void Character::HandleLoginResponse(client::PacketIn& pktin)
    {
        uint32_t result = pktin.ReadVarInteger<int32_t>();
        if (result != 0) {
            result_ = result;
        }
        switch (result) {
            case 0:
                printf("user [%s] login success !!!\n", username_.c_str());
                TemplateCheck();
                break;
            case 1:
                printf("user [%s] bad session!\n", username_.c_str());
                break;
            case 2:
                printf("user [%s] unknown error!\n", username_.c_str());
                break;
            default:
                printf("user [%s] login fail, error=%u\n", username_.c_str(), result);
                break;
        }
    }
    
    void Character::HandleTemplateUpdate(PacketIn& pktin)
    {
        int progress = pktin.ReadVarInteger<int>();
        cout << "progress = " << progress;
        int total = pktin.ReadVarInteger<int>();
        cout << " total = " << total << endl;
        if (total > 0) {
            string name = pktin.ReadString();
            //cout << "name = " << name;
            int partProgress =  pktin.ReadVarInteger<int>();
            //cout << " partProgress = " << partProgress;
            int partTotal = pktin.ReadVarInteger<int>();
            //cout << " partTotal = " << partTotal;
            string content = pktin.ReadString();
            //cout << " content.len = " << content.length();
            AddJson(name, content);
            //cout << endl;
            if (progress == total && partProgress == partTotal) {
                // 写入tpl
                string dir = g_fs->path() + "/tpl";
                for (auto it = jsons_.begin(); it != jsons_.end(); ++it) {
                    string file = dir + "/" + it->first;
                    // cout << "file = " << file << endl;
                    string json = base::utils::zlib_uncompress(it->second);
                    file_save_content(file.c_str(), json);
                }
                ParseTpl();
            }
        }
    }
    
    void Character::HandleUserUpdate(client::PacketIn& pktin)
    {
        uid_ = pktin.ReadVarInteger<int64_t>();
        headId_ = pktin.ReadVarInteger<int64_t>();
        nickname_ = pktin.ReadString();
        level_ = pktin.ReadVarInteger<int32_t>();
        int exp = pktin.ReadVarInteger<int32_t>();
        int silver = pktin.ReadVarInteger<int32_t>();
        int gold = pktin.ReadVarInteger<int32_t>();
        int food = pktin.ReadVarInteger<int32_t>();
        int wood = pktin.ReadVarInteger<int32_t>();
        int iron = pktin.ReadVarInteger<int32_t>();
        int stone = pktin.ReadVarInteger<int32_t>();
        stamina_ = pktin.ReadVarInteger<int32_t>();
        // others ...
        
        printf("### HandleUserUpdate uid=%ld, username=%s, nickname=%s, level=%d, gold=%d, stone=%d \n", uid_, username_.c_str(), nickname_.c_str(), level_, gold, stamina_);
    }
    
    void Character::HandleReady(client::PacketIn& pktin)
    {
        result_ = 0;
        islogin_ = true;
        
        
        // push sync
        /*
        client::PacketOut pktout((uint16_t)model::CS::PUSH_DEVICE_SYNC, 20, g_fs->mempool());
        pktout.WriteString("android");
        pktout.WriteString("deviceToken");
        g_fs->Send(pktout);*/
    }
    
    void Character::HandleLogout(PacketIn& pktin)
    {
        int kickType = pktin.ReadVarInteger<int>();
        printf("### HandleLogout kickType=%d \n", kickType);
    }

    
    void Character::HandlePing(client::PacketIn& pktin)
    {
        int sequence = pktin.ReadVarInteger<int>();
        printf("### HandlePing sequence=%d \n", sequence);
        
        // reply ping
        client::PacketOut pktout((uint16_t)model::CS::PING_REPLY, 20, g_fs->mempool());
        pktout.WriteVarInteger<int>(sequence);
        g_fs->Send(pktout);
    }
    
    void Character::HandleSignUpdate(PacketIn& pktin)
    {
        int signToday = pktin.ReadBoolean() ? 1 : 0;
        int signCount = pktin.ReadVarInteger<int>();

        if (!signToday) {
            // sign reward
            client::PacketOut pktout((uint16_t)model::CS::MISC_SIGN, 20, g_fs->mempool());
            g_fs->Send(pktout);
        }
        printf("### HandleSignUpdate signToday=%d, signCount=%d \n", signToday, signCount);
    }
    
    void Character::HandleOnlineUpdate(PacketIn& pktin)
    {
        int step = pktin.ReadVarInteger<int>();
        int leftSeconds = pktin.ReadVarInteger<int>();
        int tplId = pktin.ReadVarInteger<int>();
        int count = pktin.ReadVarInteger<int>();

        if (step > 0 && leftSeconds == 0) {
            // draw reward
            client::PacketOut pktout((uint16_t)model::CS::MISC_DRAW_ONLINE, 20, g_fs->mempool());
            g_fs->Send(pktout);
        }
        printf("### HandleOnlineUpdate step=%d, leftSeconds=%d \n", step, leftSeconds);
    }
    
    void Character::HandleMailUpdate(PacketIn& pktin)
    {
        int len = pktin.ReadVarInteger<int>();
        for (int i = 0; i < len; ++i) {
            int64_t mailId = pktin.ReadVarInteger<int64_t>();
            std::string title = pktin.ReadString();
            int type = pktin.ReadVarInteger<int>();
            int subType = pktin.ReadVarInteger<int>();
            int isRead = pktin.ReadBoolean() ? 1 : 0;
            // others ...
            
            if (isRead == 0) {
                {
                    // read mail
                    client::PacketOut pktout((uint16_t)model::CS::MAIL_VIEW, 20, g_fs->mempool());
                    pktout.WriteVarInteger<int64_t>(mailId);
                    g_fs->Send(pktout);
                }
                {
                    // delete mail
                    client::PacketOut pktout((uint16_t)model::CS::MAIL_DELETE, 20, g_fs->mempool());
                    pktout.WriteVarInteger<int32_t>(1);
                    pktout.WriteVarInteger<int64_t>(mailId);
                    pktout.WriteVarInteger<int64_t>(0);
                    g_fs->Send(pktout);
                }
            }
        }
        printf("### HandleMailUpdate len=%d \n", len);
    }
    
    void Character::HandleQuestUpdate(PacketIn& pktin)
    {
        int len = pktin.ReadVarInteger<int>();
        for (int i = 0; i < len; ++i) {
            int questId = pktin.ReadVarInteger<int>();
            int progress = pktin.ReadVarInteger<int>();
            int total = FindQuest(questId);
            if (total > 0 && progress >= total) {
                // draw quest reward
                client::PacketOut pktout((uint16_t)model::CS::QUEST_DRAW, 20, g_fs->mempool());
                pktout.WriteVarInteger<int>(questId);
                g_fs->Send(pktout);
            }
        }
        len = pktin.ReadVarInteger<int>();
        for (int i = 0; i < len; ++i) {
            int questId = pktin.ReadVarInteger<int>();
        }
        //printf("### HandleQuestUpdate len=%d \n", len);
    }

    void Character::ParseTpl()
    {
        cout << "Character::ParseTpl" << g_fs->path() << endl;
        string dir = g_fs->path() + "/tpl";
        vector<string> files = dir_get_files(dir.c_str(), 1);
        for (const string& file : files) {
            string fileName = file.substr(dir.length() + 1, file.length());
            //cout << "fileName = " << fileName << endl;
            string content = file_get_content(file.c_str());
            try {
                rapidjson::Document doc;
                doc.Parse<0>(content.c_str());
                if (doc.IsArray()) {
                    for (size_t i = 0; i < doc.Size(); ++i) {
                        rapidjson::GenericValue<rapidjson::UTF8< char >>& row = doc[i];
                        if (row.IsObject()) {
                            if (fileName == "quest.json") {
                                int id = row["id"].GetInt();
                                int count = row["count"].GetInt();
                                tplQuests_.emplace(id, count);
                            } else if (fileName == "seven_target.json") {
                                int id = row["id"].GetInt();
                                int count = row["count"].GetInt();
                                tplSevenTargets_.emplace(id, count);
                            } else if (fileName == "alliance_science.json") {
                                AllianceScienceTpl tpl;
                                tpl.tplid = row["id"].GetInt();
                                tpl.donationId = row["donationId"].GetInt();
                                tpl.donationCount = row["donationCount"].GetInt();
                                tplAllianceSciences_.emplace(tpl.tplid, tpl);
                            }
                        }
                    }
                } else {
                    printf("ParseTpl json is not array : fileName = %s\n", fileName.c_str());
                }
            } catch (exception& ex) {
                printf("ParseTpl json to string fail: %s\n", ex.what());
            }
        }
        
        cout << "quest size = " << tplQuests_.size() << endl;
        cout << "seven target size = " << tplSevenTargets_.size() << endl;
        cout << "alliance science size = " << tplAllianceSciences_.size() << endl;
    }
    
}

