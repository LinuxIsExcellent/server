#include "map.h"
#include "frontserver.h"
#include "clientimpl.h"
#include "character.h"
#include "misc.h"
#include <sstream>

namespace dal
{
    using namespace std;

    Map::Map(ClientImpl *impl, FrontServer *fs) : g_fs(fs)
    {
        HANDLE_MAP(model::SC::MAP_PERSONAL_INFO_UPDATE, Map::HandleInfoUpdate);
        HANDLE_MAP(model::SC::MAP_UNIT_UPDATE, Map::HandleUnitUpdate);
        HANDLE_MAP(model::SC::MAP_TROOP_UPDATE, Map::HandleTroopUpdate);
    }

    Map::~Map()
    {
    }

    void Map::MapView()
    {
        int x = g_fs->character()->random().GenRandomNum(x_ - 10, x_ + 10);
        int y = g_fs->character()->random().GenRandomNum(y_ - 10, y_ + 10);
        
        client::PacketOut pktout((uint16_t)model::CS::MAP_VIEW, 20, g_fs->mempool());
        //pktout.WriteVarInteger(k_);
        pktout.WriteVarInteger(x);
        pktout.WriteVarInteger(y);
        g_fs->Send(pktout);
        
        printf("### MapView x=%d, y=%d\n", x, y);
    }
    
    void Map::MapLeave()
    {
        //m_units.clear();
        
        client::PacketOut pktout((uint16_t)model::CS::MAP_LEAVE, 20, g_fs->mempool());
        g_fs->Send(pktout);
    }
    
    void Map::MapMarch()
    {
        if(m_units.size() == 0 
            || (g_fs->character()->username() != "test1" && m_troops.size() >= 3)) {
            return;
        }
        
        int x = 0;
        int y = 0;
        int size = m_units.size() + 1;

        int r = g_fs->character()->random().GenRandomNum(1, size * 2);
        int index = 1;
        int tplId = 0;

        model::MapTroopType tType = model::MapTroopType::CAMP_TEMP;
        if (r < size) {
            for (auto it = m_units.begin(); it != m_units.end(); ++it) {
                if (index == r) {
                    tplId = it->second.tplid;
                    x = it->second.x;
                    y = it->second.y;
                    break;
                }
                index++;
            }
        } else {
            x = g_fs->character()->random().GenRandomNum(1, 1200);
            y = g_fs->character()->random().GenRandomNum(1, 1200);
        }
        
        if(x == x_ && y == y_) {
            return;
        }

        if (tplId == 0) {
            int r = g_fs->character()->random().GenRandomNum(1, 10000);
            if (r > 5000) {
                tType = model::MapTroopType::CAMP_FIXED;
                {
                    client::PacketOut pktout((uint16_t)model::CS::CHAT_GM, 20, g_fs->mempool());
                    char buf[20] = {0};
                    sprintf(buf, "addexp, %d", 50000);
                    pktout.WriteString(buf);
                    g_fs->Send(pktout);
                }

                for (int i = 1; i <= 5; ++i) {
                    client::PacketOut pktout((uint16_t)model::CS::CHAT_GM, 20, g_fs->mempool());
                    char buf[20] = {0};
                    sprintf(buf, "addres, %d, %d", i, 50000);
                    pktout.WriteString(buf);
                    g_fs->Send(pktout);
                }
            }
        } else {
            if (tplId >= 1600001 && tplId <= 1600004) {
                tType = model::MapTroopType::SIEGE_CITY;
            } else if (tplId >= 1601001 && tplId <= 1604009) {
                tType = model::MapTroopType::GATHER;
            } else if (tplId >= 1605001 && tplId <= 1605012) {
                tType = model::MapTroopType::MONSTER;
            } else if (tplId >= 1650001 && tplId <= 1650030) {
                tType = model::MapTroopType::SIEGE_CASTLE;
                bool scout = g_fs->character()->random().GenRandomNum(1, 10000) > 5000;
                if (scout) {
                    tType = model::MapTroopType::SCOUT;
                }
            } else if (tplId == 1600006) {
                tType = model::MapTroopType::CAMP_FIXED_ATTACK;
            } else if (tplId == 1600007) {
                tType = model::MapTroopType::CAMP_TEMP_ATTACK;
            }
        }
        
        bool next = false;
        int team = 0;
        do {
            next = false;
            ++team;
            for (auto troop :  m_troops) {
                if (troop.second.team == team) {
                    next = true;
                    break;
                }
            }
        } while (next);

        int num = 200;
        if(tType == model::MapTroopType::MONSTER) {
            num = 5;
            if (g_fs->character()->stamina() < 5) {
                return;
            }
        }

        client::PacketOut pktout1((uint16_t)model::CS::SAVE_TEAM_INFO, 20, g_fs->mempool());
        pktout1.WriteVarInteger(1);
        pktout1.WriteVarInteger(team);
        // army list size
        pktout1.WriteVarInteger(2);

        pktout1.WriteVarInteger(1300039);
        pktout1.WriteVarInteger(1);
        pktout1.WriteVarInteger(num);
        pktout1.WriteVarInteger(1);

        pktout1.WriteVarInteger(1300040);
        pktout1.WriteVarInteger(4);
        pktout1.WriteVarInteger(num);
        pktout1.WriteVarInteger(3);
        g_fs->Send(pktout1);

        client::PacketOut pktout((uint16_t)model::CS::MAP_MARCH, 20, g_fs->mempool());
        pktout.WriteVarInteger(int(tType));
        pktout.WriteVarInteger(x);
        pktout.WriteVarInteger(y);
        pktout.WriteVarInteger(team);
        g_fs->Send(pktout);
        
        std::cout << "### MapMarch tplId=" << tplId << " tType=" << (int)tType << std::endl;
    }

    void Map::Teleport()
    {
        int r = g_fs->character()->random().GenRandomNum(10000);
        if (r > 100) {
            return;
        }
        
        int   x = g_fs->character()->random().GenRandomNum(1, 1200);
        int   y = g_fs->character()->random().GenRandomNum(1, 1200);

        char buf[20] = {0};

        sprintf(buf, "{[1]=%d, [2]=%d}", x, y);

        g_fs->misc()->SendBagUse(2034002, 1, true, buf, "");
    }

    void Map::HandleInfoUpdate(client::PacketIn& pktin)
    {
        k_ = pktin.ReadVarInteger<int>();
        x_ = pktin.ReadVarInteger<int>();
        y_ = pktin.ReadVarInteger<int>();
        // others ...
        
        printf("### Map::HandleInfoUpdate k=%d, x=%d, y=%d\n", k_, x_, y_);
    }

    void Map::HandleUnitUpdate(client::PacketIn& pktin)
    {
        int k = pktin.ReadVarInteger<int>();
        
        int len1 = pktin.ReadVarInteger<int>();
        for (int i = 0; i < len1; ++i) {
            int id = pktin.ReadVarInteger<int>();
            int x = pktin.ReadVarInteger<int>();
            int y = pktin.ReadVarInteger<int>();
            int tplId = pktin.ReadVarInteger<int>();
            //std::cout << "unit id=" << id << " tplId=" << tplId << std::endl;
            int64_t uid = pktin.ReadVarInteger<int64_t>();
            int64_t noviceuid = pktin.ReadVarInteger<int64_t>();
            std::string nickname = pktin.ReadString();
            int level = pktin.ReadVarInteger<int>();
            int CastleState = pktin.ReadVarInteger<int>();
            int leftCount = pktin.ReadVarInteger<int>();
            int troopId = pktin.ReadVarInteger<int>();
            float gatherSpeed = pktin.ReadFloat();
            int canGather = pktin.ReadVarInteger<int>();
            int hadGather = pktin.ReadVarInteger<int>();

            int armyLen = pktin.ReadVarInteger<int>();
            for (int i = 0; i < armyLen; ++i) {
                int heroTplId = pktin.ReadVarInteger<int>();
                int armyTplId = pktin.ReadVarInteger<int>();
                int armyCount = pktin.ReadVarInteger<int>();
                int position = pktin.ReadVarInteger<int>();
            }

            int cityDefence = pktin.ReadVarInteger<int>();
            int cityDefenceMax = pktin.ReadVarInteger<int>();
            int allianceId = pktin.ReadVarInteger<int>();
            std::string allianceNickname = pktin.ReadString();
            int bannerId = pktin.ReadVarInteger<int>();

            int troopLen = pktin.ReadVarInteger<int>();
            for (int i = 0; i < troopLen; ++i) {
                int uid = pktin.ReadVarInteger<int64_t>();
                int troopId = pktin.ReadVarInteger<int>();
            }

            MapUnit u;
            u.tplid = tplId;
            u.x = x;
            u.y = y;
            u.uid = uid;
            m_units[id] = u;
        }
        //removelist
        int len2 = pktin.ReadVarInteger<int>();
        for (int i = 0; i < len2; ++i) {
            int id = pktin.ReadVarInteger<int>();
            if(m_units.find(id) != m_units.end()) {
                m_units.erase(id);
            }
            //std::cout << "remove id=" << id << std::endl;
        }
        
        printf("### HandleUnitUpdate updatelist=%d, removelist=%d, m_units.size()=%d \n", len1, len2, m_units.size());
    }

    void Map::HandleTroopUpdate(client::PacketIn& pktin)
    {
        int k = pktin.ReadVarInteger<int>();
        int len1 = pktin.ReadVarInteger<int>();
        for (int i = 0; i < len1; ++i) {
            int tid = pktin.ReadVarInteger<int>();
            int troopType = pktin.ReadVarInteger<int>();
            int troopState = pktin.ReadVarInteger<int>();
            int64_t uid = pktin.ReadVarInteger<int64_t>();
            int fromX = pktin.ReadVarInteger<int>();
            int fromY = pktin.ReadVarInteger<int>();
            int fromTplId = pktin.ReadVarInteger<int>();
            int toX = pktin.ReadVarInteger<int>();
            int toY = pktin.ReadVarInteger<int>();
            //std::cout << "troop tid=" << tid << " toX=" << toX << " toY=" << toY << std::endl;
            int toTplId = pktin.ReadVarInteger<int>();
            int64_t toId = pktin.ReadVarInteger<int64_t>();

            int64_t toAllianceId = pktin.ReadVarInteger<int64_t>();
            std::string toAllianceNickname = pktin.ReadString();
            std::string toPlayerNickname = pktin.ReadString();
            int64_t toPlayerHeadId = pktin.ReadVarInteger<int64_t>();
            int toPlayerLevel = pktin.ReadVarInteger<int>();

            int64_t allianceId = pktin.ReadVarInteger<int64_t>();
            std::string allianceNickname = pktin.ReadString();
            std::string nickname = pktin.ReadString();
            int64_t headId = pktin.ReadVarInteger<int64_t>();
            int level = pktin.ReadVarInteger<int>();

            int leftSeconds = pktin.ReadVarInteger<int>();
            int TotalSeconds = pktin.ReadVarInteger<int>();
            float speed = pktin.ReadFloat();
            float initialSpeed = pktin.ReadFloat();
            float speedUp = pktin.ReadFloat();
            float currentX = pktin.ReadFloat();
            float currentY = pktin.ReadFloat();
            int team = pktin.ReadVarInteger<int>();
            int power = pktin.ReadVarInteger<int>();

            //armyInfo
            int armyList_len = pktin.ReadVarInteger<int>();
            for (int armyInfo_i = 0; armyInfo_i < armyList_len; ++armyInfo_i) {
                int heroTplId = pktin.ReadVarInteger<int>();
                int heroLevel = pktin.ReadVarInteger<int>();
                int heroStar = pktin.ReadVarInteger<int>();
                int armTplId = pktin.ReadVarInteger<int>();
                int position = pktin.ReadVarInteger<int>();
                int states_len = pktin.ReadVarInteger<int>();
                for (int states_i = 0; states_i < states_len; ++states_i) {
                    int state = pktin.ReadVarInteger<int>();
                    int count = pktin.ReadVarInteger<int>();
                }
            }
            int param1 = pktin.ReadVarInteger<int>();
            int param2 = pktin.ReadVarInteger<int>();

            if(uid == g_fs->character()->uid()) {
                MapTroop troopData;
                troopData.troopId = tid;
                troopData.team = team;
                m_troops[tid] = troopData;
            }
        }
        //removelist
        int len2 = pktin.ReadVarInteger<int>();
        for (int i = 0; i < len2; ++i) {
            int tid = pktin.ReadVarInteger<int>();
            if(m_troops.find(tid) != m_troops.end()) {
                m_troops.erase(tid);
            }
            //std::cout << "remove tid=" << tid << std::endl;
        }
        
        printf("### HandleTroopUpdate updatelist=%d, removelist=%d, m_troops.size()=%d \n", len1, len2, m_troops.size());
    }

}

