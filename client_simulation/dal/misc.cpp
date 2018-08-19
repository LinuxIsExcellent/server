#include "misc.h"
#include "frontserver.h"
#include "clientimpl.h"
#include "character.h"
#include "alliance.h"
#include <sstream>
#include <base/event/dispatcher.h>
#include <base/utils/utils_time.h>
#include <base/logger.h>
#include <algorithm>

namespace dal
{
    using namespace std;
    using namespace base::utils;

    Misc::Misc(ClientImpl* impl, FrontServer* fs) : g_fs(fs)
    {
        HANDLE_MAP(model::SC::CHAT_RECV, Misc::HandleChatRecv);
        HANDLE_MAP(model::SC::RANGE_FETCH_RESPONSE, Misc::HandleRangeFetchResponse);
        HANDLE_MAP(model::SC::CDLIST_UPDATE, Misc::HandleCDListUpdate);
        HANDLE_MAP(model::SC::BUILDING_UPDATE, Misc::HandleBuildingUpdate);
        HANDLE_MAP(model::SC::TECHNOLOGY_INFOS_UPDATE, Misc::HandleTechUpdate);
        HANDLE_MAP(model::SC::BABEL_DATA_UPDATE, Misc::HandleBabelDataUpdate);
    }

    Misc::~Misc()
    {
    }

    int Misc::GetCastleLevel() const
    {
        for (auto it = m_buildings.begin(); it != m_buildings.end(); ++it) {
            const BuildingInfo& info = it->second;
            if (info.tplId == 1201000) {
                return info.level;
            }
        }
        return 0;
    }

    void Misc::DoSomething()
    {
        g_fs->alliance()->CheckCreate();

        KingdomChat();
        AllianceChat();

        SomeAction action = static_cast<SomeAction>(g_fs->character()->random().GenRandomNum(1, (int)SomeAction::ACTION_END));
        switch (action) {
            case SomeAction::range_fetch:
                RangeFetch();
                break;
            case SomeAction::open_queue2:
                //OpenQueue2();
                break;
            case SomeAction::research_tech:
                ResearchTech();
                break;
            case SomeAction::upgrade_building:
                UpgradeBuilding();
                break;
            case SomeAction::collect_resource:
                CollectResource();
                break;
            case SomeAction::training_army:
                //TrainingArmy();
                break;
            case SomeAction::training_trap:
                //TrainingTrap();
                break;

            case SomeAction::alliance_explore:
                //g_fs->alliance()->Explore();
                break;
            case SomeAction::alliance_help:
                g_fs->alliance()->Help();
                break;
            case SomeAction::alliance_donate:
                //g_fs->alliance()->Donate();
                break;
            case SomeAction::babel_fight:
                //BabelFight();
                break;
            default:
                break;
        }

        g_fs->alliance()->CheckQuit();
    }

    void Misc::KingdomChat()
    {
        if (g_fs->character()->namesuffix() != 1) {
            return;
        }
        int r = g_fs->character()->random().GenRandomNum(10000);
        if (r > 6000) {
            return;
        }

        string content = string("world, my name is ") + to_string(r);
        client::PacketOut pktout((uint16_t)model::CS::CHAT_SEND, 20, g_fs->mempool());
        pktout.WriteVarInteger<int>(int(model::ChatType::KINGDOM));
        pktout.WriteString(content);
        g_fs->Send(pktout);

        printf("### KingdomChat \n");
    }

    void Misc::AllianceChat()
    {
        if (g_fs->alliance()->aid() == 0) {
            return;
        }
        if (g_fs->alliance()->leaderId() != g_fs->character()->uid()) {
            return;
        }

        int r = g_fs->character()->random().GenRandomNum(10000);
        if (r > 4000) {
            return;
        }

        string content = string("alliance, my name is ") + g_fs->character()->username();
        client::PacketOut pktout((uint16_t)model::CS::CHAT_SEND, 20, g_fs->mempool());
        pktout.WriteVarInteger<int>(int(model::ChatType::ALLIANCE));
        pktout.WriteString(content);
        g_fs->Send(pktout);

        printf("### AllianceChat \n");
    }

    void Misc::HandleChatRecv(client::PacketIn& pktin)
    {
        int chatType = pktin.ReadVarInteger<int>();
        printf("### HandleChatRecv type=%s \n", chatType == 1 ? "国家聊天" : "联盟聊天");
    }


    void Misc::RangeFetch()
    {
        int r = g_fs->character()->random().GenRandomNum(10000);
        if (r > 500) {
            return;
        }

        int type = g_fs->character()->random().GenRandomNum(1, 3);
        if (m_range_vsns.find(type) == m_range_vsns.end()) {
            m_range_vsns[type] = 0;
        }
        int vsn = m_range_vsns.at(type);
        client::PacketOut pktout((uint16_t)model::CS::RANGE_FETCH, 20, g_fs->mempool());
        pktout.WriteVarInteger(vsn);
        pktout.WriteVarInteger(type);
        g_fs->Send(pktout);

        printf("### RangeFetch type=%d, vsn=%d \n", type, vsn);
    }

    void Misc::OpenQueue2()
    {
        int r = g_fs->character()->random().GenRandomNum(1, 10000);
        if (buider2LeftSeconds_ > 300 || r > 5000) {
            return;
        }

        //client::PacketOut pktout((uint16_t)model::CS::BUILDING_OPEN_BUILDER2, 20, g_fs->mempool());
        //g_fs->Send(pktout);

        printf("### OpenQueue2 \n");
    }

    void Misc::ResearchTech()
    {
        int r = g_fs->character()->random().GenRandomNum(10000);
        if (r > 7000) {
            return;
        }

        int techId = 0;
        r = g_fs->character()->random().GenRandomNum(1, 9000);
        if (r <= 3000) {
            techId = 2501001;
        } else if (r <= 6000) {
            techId = 2502001;
        } else {
            techId = 2504001;
        }
        if (m_skills.find(techId) != m_skills.end() && m_skills.at(techId) >= 5) {
            return;
        }
        int gridId = 0;
        for (auto it = m_buildings.begin(); it != m_buildings.end(); ++it) {
            if (it->second.tplId == 1207000) {
                gridId = it->second.gridId;
                break;
            }
        }
        if (gridId == 0) {
            return;
        }

        client::PacketOut pktout((uint16_t)model::CS::BUILDING_TECHNOLOGY_RESEARCH, 20, g_fs->mempool());
        pktout.WriteVarInteger<int>(gridId); // 格子ID
        pktout.WriteVarInteger<int>(techId); // 科技模板ID  TODO
        pktout.WriteBoolean(false); // 是否立即研究
        g_fs->Send(pktout);

        printf("### ResearchTech \n");
    }

    void Misc::UpgradeBuilding()
    {
        int r = 0;
        if (buider1_ > 0) {
            if (buider2_ > 0 || buider2LeftSeconds_ < 3600) {
                r = g_fs->character()->random().GenRandomNum(1, 10000);
                // 使用金币加速
                client::PacketOut pktout((uint16_t)model::CS::CDLIST_SPEED_UP, 20, g_fs->mempool());
                pktout.WriteVarInteger<int>(buider1_);
                pktout.WriteVarInteger<int>((int)model::CDSpeedUpType::GOLD);
                pktout.WriteVarInteger<int>(0);
                pktout.WriteVarInteger<int>(0);
                g_fs->Send(pktout);

                return;
            }
        }
        if (buider1_ > 0 && buider2_ > 0) {
            return;
        }

        r = g_fs->character()->random().GenRandomNum(10000);
        if (r > 7000) {
            return;
        }

        r = g_fs->character()->random().GenRandomNum(1, m_buildings.size() + 1);
        int index = 1;
        int gridId = 0;
        for (auto it = m_buildings.begin(); it != m_buildings.end(); ++it) {
            if (index == r) {
                if (it->second.tplId > 0 && it->second.state == model::BuildingState::NORMAL) {
                    gridId = it->second.gridId;
                }
                break;
            }
            index++;
        }
        if (gridId > 0) {
            client::PacketOut pktout((uint16_t)model::CS::BUILDING_UPGRADE, 20, g_fs->mempool());
            pktout.WriteVarInteger<int>(gridId); // 格子ID
            pktout.WriteBoolean(false); // 是否立即升级
            g_fs->Send(pktout);

            printf("### UpgradeBuilding \n");
        }
    }

    void Misc::CollectResource()
    {
        int r = g_fs->character()->random().GenRandomNum(10000);
        if (r > 7000) {
            return;
        }

        vector<int> gridIds;
        for (auto it = m_buildings.begin(); it != m_buildings.end(); ++it) {
            BuildingInfo& info = it->second;
            if (info.gridId >= 100 && info.gridId <= 124) {
                gridIds.push_back(info.gridId);
            }
        }
        int count = g_fs->character()->random().GenRandomNum(3, 7);
        random_shuffle(gridIds.begin(), gridIds.end());
        for (int i = 0; i < gridIds.size(); ++i) {
            if (count-- == 0) {
                break;
            }
            int gridId = gridIds[i];
            client::PacketOut pktout((uint16_t)model::CS::BUILDING_COLLECT, 20, g_fs->mempool());
            pktout.WriteVarInteger<int>(gridId); // 格子ID
            pktout.WriteVarInteger<int>(0);         // 0表示一键收取
            g_fs->Send(pktout);
        }
    }

    void Misc::TrainingArmy()
    {
        int r = g_fs->character()->random().GenRandomNum(10000);
        if (r > 7000) {
            return;
        }

        int gridId = 0;
        int armyId = 0;
        int count = 0;
        bool trainNow = g_fs->character()->random().GenRandomNum(1, 10000) > 5000;
        for (auto it = m_buildings.begin(); it != m_buildings.end(); ++it) {
            if (it->second.tplId > 0 && it->second.gridId == 3) {
                gridId = 3;
                armyId = 1001001;
                count = 20;
                break;
            } else if (it->second.tplId > 0 && it->second.gridId == 4) {
                gridId = 4;
                armyId = 1002001;
                count = 20;
                break;
            } else if (it->second.tplId > 0 && it->second.gridId == 5) {
                gridId = 5;
                armyId = 1003001;
                count = 20;
                break;
            } else if (it->second.tplId > 0 && it->second.gridId == 6) {
                gridId = 6;
                armyId = 1004001;
                count = 20;
                break;
            }
        }
        if (gridId == 0) {
            return;
        }

        client::PacketOut pktout((uint16_t)model::CS::BUILDING_TRAIN, 20, g_fs->mempool());
        pktout.WriteVarInteger<int>(gridId);
        pktout.WriteVarInteger<int>(armyId);
        pktout.WriteVarInteger<int>(count);
        pktout.WriteBoolean(trainNow);
        g_fs->Send(pktout);

        printf("### TrainingArmy \n");
    }

    void Misc::TrainingTrap()
    {
        int r = g_fs->character()->random().GenRandomNum(10000);
        if (r > 7000) {
            return;
        }

        bool trainNow = g_fs->character()->random().GenRandomNum(1, 10000) > 5000;

        int gridId = 0;
        for (auto it = m_buildings.begin(); it != m_buildings.end(); ++it) {
            if (it->second.gridId == 102) {
                gridId = 102;
                break;
            }
        }
        if (gridId == 0) {
            return;
        }

        client::PacketOut pktout((uint16_t)model::CS::BUILDING_TRAIN, 20, g_fs->mempool());
        pktout.WriteVarInteger<int>(gridId);
        pktout.WriteVarInteger<int>(1005001);
        pktout.WriteVarInteger<int>(10);
        pktout.WriteBoolean(trainNow);
        g_fs->Send(pktout);

        printf("### TrainingTrap \n");
    }

    void Misc::BabelFight()
    {
        int r = g_fs->character()->random().GenRandomNum(10000);
        if (r > 7000 || m_babelLayer >= 100) {
            return;
        }

        int playerLevel = g_fs->character()->level();
        if (playerLevel >= 8) {
            client::PacketOut pktout((uint16_t)model::CS::BABEL_FIGHT, 20, g_fs->mempool());
            
            g_fs->Send(pktout);
            cout << "### BABEL_FIGHT" << endl;
        }
    }

    void Misc::BabelMopup()
    {
        int r = g_fs->character()->random().GenRandomNum(10000);
        if (r > 7000 || m_babelLayer >= 100) {
            return;
        }

        int playerLevel = g_fs->character()->level();
        if (playerLevel >= 8) {
            client::PacketOut pktout((uint16_t)model::CS::BABEL_MOPUP, 20, g_fs->mempool());
            
            g_fs->Send(pktout);
            cout << "### TowerMopup" << endl;
        }
    }


    void Misc::HandleRangeFetchResponse(client::PacketIn& pktin)
    {
        int type = pktin.ReadVarInteger<int>();
        int vsn = pktin.ReadVarInteger<int>();
        int selfRank = pktin.ReadVarInteger<int>();
        std::string data = pktin.ReadString();

        m_range_vsns[type] = vsn;

        printf("### HandleRangeFetchResponse type=%d, vsn=%d, selfRank=%d, data=%s \n", type, vsn, selfRank, data.c_str());
    }

    void Misc::HandleCDListUpdate(client::PacketIn& pktin)
    {
        //updateList
        int len1 = pktin.ReadVarInteger<int>();
        for (int i = 0; i < len1; ++i) {
            CDInfo info;
            info.id = pktin.ReadVarInteger<int>();
            info.type = static_cast<model::CDType>(pktin.ReadVarInteger<int>());
            info.remainTime = pktin.ReadVarInteger<int>();
            info.expireTime = nowtick() / 1000 + info.remainTime;
            int sumTime = pktin.ReadVarInteger<int>();

            if (info.remainTime > 0) {
                m_cds[info.id] = info;
                if (info.type != model::CDType::COMMON) {
                    if (info.remainTime <= 300) {
                        // 免费加速
                        client::PacketOut pktout((uint16_t)model::CS::CDLIST_SPEED_UP, 20, g_fs->mempool());
                        pktout.WriteVarInteger<int>(info.id);
                        pktout.WriteVarInteger<int>((int)model::CDSpeedUpType::FREE);
                        pktout.WriteVarInteger<int>(0);
                        pktout.WriteVarInteger<int>(0);
                        g_fs->Send(pktout);
                    } else {
                        int r = g_fs->character()->random().GenRandomNum(10000);
                        if (r > 8000) {
                            // 使用金币加速
                            client::PacketOut pktout((uint16_t)model::CS::CDLIST_SPEED_UP, 20, g_fs->mempool());
                            pktout.WriteVarInteger<int>(info.id);
                            pktout.WriteVarInteger<int>((int)model::CDSpeedUpType::GOLD);
                            pktout.WriteVarInteger<int>(0);
                            pktout.WriteVarInteger<int>(0);
                            g_fs->Send(pktout);
                        }
                    }
                }
            } else {
                if (m_cds.find(info.id) != m_cds.end()) {
                    m_cds.erase(info.id);
                }
            }
        }
        printf("### HandleCDListUpdate len=%d \n", len1);
    }

    void Misc::HandleBuildingUpdate(client::PacketIn& pktin)
    {
        //updateList
        int len1 = pktin.ReadVarInteger<int>();
        for (int i = 0; i < len1; ++i) {
            BuildingInfo info;
            info.id = pktin.ReadVarInteger<int>();
            info.gridId = pktin.ReadVarInteger<int>();
            info.tplId = pktin.ReadVarInteger<int>();
            info.level = pktin.ReadVarInteger<int>();
            cout << "tplId = " << info.tplId << " level = " << info.level << endl;
            info.maxLevel = pktin.ReadVarInteger<int>();
            info.state = static_cast<model::BuildingState>(pktin.ReadVarInteger<int>());
            int len2 = pktin.ReadVarInteger<int>();
            for (int i = 0; i < len2; ++i) {
                int cdId = pktin.ReadVarInteger<int>();
                info.cdIds.push_back(cdId);
            }
            info.param1 = pktin.ReadVarInteger<int>();
            info.param2 = pktin.ReadVarInteger<int>();
            info.param3 = pktin.ReadVarInteger<int>();
            info.param4 = pktin.ReadVarInteger<int>();
            m_buildings[info.id] = info;
        }
        //removelist
        int len3 = pktin.ReadVarInteger<int>();
        for (int i = 0; i < len3; ++i) {
            int id = pktin.ReadVarInteger<int>();
            if (m_buildings.find(id) != m_buildings.end()) {
                m_buildings.erase(id);
            }
        }
        int len4 = pktin.ReadVarInteger<int>();
        for (int i = 0; i < len4; ++i) {
            int forestId = pktin.ReadVarInteger<int>();
        }
        buider1_ = pktin.ReadVarInteger<int>();
        buider2_ = pktin.ReadVarInteger<int>();
        buider2LeftSeconds_ = pktin.ReadVarInteger<int>();

        printf("### HandleBuildingUpdate updateList=%d, buider1=%d, buider2=%d, buider1LeftSeconds=%d \n", len1, buider1_, buider2_, buider2LeftSeconds_);

        for (auto it = m_buildings.begin(); it != m_buildings.end(); ++it) {
            BuildingInfo& bInfo = it->second;
            if (bInfo.cdIds.empty()) {
                continue;
            }
            auto it2 = m_cds.find(bInfo.cdIds.front());
            if (it2 != m_cds.end()) {
                CDInfo& cdInfo = it2->second;
                if (cdInfo.type != model::CDType::COMMON) {
                    if (cdInfo.remainTime > 300) {
                        int r = g_fs->character()->random().GenRandomNum(10000);
                        if (r > 8000 && g_fs->alliance()->aid() > 0) {
                            // 联盟帮助
                            int gridId = bInfo.gridId;
                            client::PacketOut pktout((uint16_t)model::CS::ALLIANCE_HELP_REQUEST, 20, g_fs->mempool());
                            pktout.WriteVarInteger<int>(gridId);
                            g_fs->Send(pktout);
                        }
                    }
                }
            }
        }
    }

    void Misc::HandleTechUpdate(client::PacketIn& pktin)
    {
        //updateList
        int sanValue = pktin.ReadVarInteger<int>();
        int len1 = pktin.ReadVarInteger<int>();
        for (int i = 0; i < len1; ++i) {
            int tplid = pktin.ReadVarInteger<int>();
            int level = pktin.ReadVarInteger<int>();
            m_skills[tplid] = level;
        }
        printf("### HandleSkillUpdate len=%d \n", len1);
    }

    void Misc::HandleBabelDataUpdate(client::PacketIn& pktin)
    {
        m_babelLayer = pktin.ReadVarInteger<int>();

        printf("### HandleBabelDataUpdate layer=%d \n", m_babelLayer);
    }

    void Misc::SendBagUse(int tplid, int count, bool useGold, const string& p1, const string& p2)
    {
        client::PacketOut pktout((uint16_t)model::CS::BAG_USE, 20, g_fs->mempool());
        pktout << 0 << tplid << count << useGold << p1;
        g_fs->Send(pktout);
    }

}

