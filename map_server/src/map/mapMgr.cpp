#include "mapMgr.h"
#include "mapProxy.h"
#include "agent.h"
#include "agentProxy.h"
#include <base/utils/utils_string.h>
#include <base/utils/file.h>
#include <base/cluster/message.h>
#include <base/cluster/nodemonitor.h>
#include <base/event/dispatcher.h>
#include <base/memory/memorypoolmgr.h>
#include <base/framework.h>
#include <model/rpc/map.h>
#include <model/protocol.h>
#include <model/tpl/templateloader.h>
#include <model/tpl/configure.h>
#include <model/tpl/alliance.h>
#include "tpl/templateloader.h"
#include "tpl/maptrooptpl.h"
#include "tpl/mapcitytpl.h"
#include "tpl/resourcetpl.h"
#include "tpl/monstertpl.h"
#include "unit/castle.h"
#include "unit/resnode.h"
#include "troop.h"
#include "unit/monster.h"
#include "unit/camp.h"
#include "unit/tree.h"
#include "unit/famouscity.h"
#include "unit/capital.h"
#include "unit/palace.h"
#include "unit/catapult.h"
#include "unit/worldboss.h"
#include <fstream>
#include <base/utils/utils_time.h>
#include <base/lua/luamessage.h>
#include "alliance.h"
#include "qo/commandagentload.h"
#include "qo/commandagentsave.h"
#include "qo/commandunitload.h"
#include "qo/commandunitsave.h"
#include "qo/commandtroopload.h"
#include "qo/commandfighthistoryload.h"
#include "qo/commandsuccessivekingsload.h"
#include "qo/commandserverinfoload.h"
#include "qo/commandserverinfosave.h"
#include "qo/commanddictload.h"
#include "qo/commanddictsave.h"
#include "qo/commandtransportrecordload.h"
#include "qo/dataservice.h"
#include "qo/commandstatsave.h"
#include "luawrite.h"
#include "activitymgr.h"
#include <base/logger.h>
#include <base/dbo/dbpool.h>
#include <base/utils/utils_time.h>
#include <base/3rd/rapidjson/filestream.h>
#include <base/3rd/rapidjson/document.h>
#include <base/3rd/rapidjson/writer.h>
#include <base/3rd/rapidjson/stringbuffer.h>
#include "bitmap_image.hpp"

namespace ms
{
    namespace map
    {
        using namespace std;
        using namespace tpl;
        using namespace model;
        using namespace model::tpl;
        using namespace model::rpc;
        using namespace base::cluster;
        using namespace base::lua;
        using namespace qo;
        using namespace info;

        MapMgr* g_mapMgr = nullptr;

        MapMgr* MapMgr::Create(int id)
        {
            MapMgr* m = new MapMgr(id);
            return m;
        }

        MapMgr::MapMgr(int id) : Map(id) 
        {
        	g_mapMgr = this;
        }

        MapMgr::~MapMgr() 
        {
        	g_mapMgr = nullptr;
        }

        void MapMgr::OnBeginSetup()
        {
            Setup();

            // 读取数据库
            m_runner.PushCommand(new qo::CommandServerInfoLoad([this](bool result0) {
                m_runner.PushCommand(new qo::CommandAgentLoad(*this, [this](bool result1) {
                    m_runner.PushCommand(new qo::CommandUnitLoad(*this, [this](bool result3) {
                        m_runner.PushCommand(new qo::CommandTroopLoad(*this, [this](bool result4) {
                            m_runner.PushCommand(new qo::CommandDictLoad([this](bool result6) {
                                m_runner.PushCommand(new qo::CommandTransportRecordLoad(*this, [this](bool result7) {
                                FinishDbLoad();
                                m_isSetupFinish = true;
                                NotifySetupFinish(true);
                                }));
                            }));
                        }));
                    }));
                }));
            }));
        }

        void MapMgr::OnBeginCleanup()
        {
            cout << "OnBeginCleanup" << endl;
            m_isSetupFinish = false;
            // 保存数据
            g_dataService->OnCleanup([this]() {
                NotifyCleanupFinish();
                cout << "OnBeginCleanup finished" << endl;
            });
            SaveMapGlobalData();
        }

        void MapMgr::Setup()
        {
            NodeMonitor::instance().evt_node_up.Attach(std::bind(&MapMgr::OnNodeUp, this, std::placeholders::_1), m_autoObserver);
            NodeMonitor::instance().evt_node_down.Attach(std::bind(&MapMgr::OnNodeDown, this, std::placeholders::_1), m_autoObserver);

            m_mapProxy->SetUp();

            cout << "AREA_MAX_X = " << AREA_MAX_X << " AREA_MAX_Y = " << AREA_MAX_Y << endl;

            //init area2
            for (int i = 0; i <= AREA_MAX_X; ++i) {
                for (int j = 0; j <= AREA_MAX_Y; ++j) {
                    m_areas[i][j].Init(i, j, AREA_WIDTH, AREA_HEIGHT);
                }
            }

            ReadMapData();
        }

        void MapMgr::OnNodeUp(const NodeInfo& node)
        {
            cout << "MapMgr::OnNodeUp " << node << endl;
        }

        void MapMgr::OnNodeDown(const NodeInfo& node)
        {
            cout << "MapMgr::OnNodeDown " << node << endl;
            if (node.node_name == "cs") {
                LOG_WARN("map_server will shutdown because the center_server shutdown!");
                framework.Stop();
            } else {
                for (auto it = m_agents.begin(); it != m_agents.end(); ++it) {
                    Agent* agent = it->second;
                    if (agent->uid() > 0 && agent->mbid().nodeid() == node.node_id) {
                        //ViewerLeave(agent);
                        agent->Disconnect();
                    }
                }
                for (auto it = m_agentsVisitor.begin(); it != m_agentsVisitor.end();) {
                    Agent* agent = it->second;
                    if (agent->uid() > 0 && agent->mbid().nodeid() == node.node_id) {
                        ViewerLeave(agent);
                        agent->Disconnect();
                        it = m_agentsVisitor.erase(it);
                        delete agent;
                    } else {
                        ++it;
                    }
                }
            }
        }

        void MapMgr::FinishDbLoad()
        {
            for (auto it = m_agents.begin(); it != m_agents.end(); ++it) {
                if (Agent* agent = it->second) {
                    agent->FinishDeserialize();
                    agent->SetClean();
                }
            }

            auto troops = m_troops;
            for (auto it = troops.begin(); it != troops.end(); ++it) {
                if (Troop* troop = it->second) {
                    troop->FinishDeserialize();
                }
            }

            for (int i = 0; i < MAP_WIDTH; ++i) {
                for (int j = 0; j < MAP_HEIGHT; ++j) {
                    Unit* u = m_units[i][j];
                    if (u != nullptr) {
                        u->FinishDeserialize();
                    }
                }
            }

            // 2. 根据生成规则动态生成
            InitMapUnits();

            //设置定时器
            g_dispatcher->quicktimer().SetInterval(std::bind(&MapMgr::OnIntervalUpdate, this), 1000, m_autoObserver);
            g_dispatcher->quicktimer().SetInterval(std::bind(&MapMgr::OnUpdate, this), 200, m_autoObserver);
            m_lastUpdateTick = g_dispatcher->GetTickCache();
            m_lastUpdateSecond = base::utils::timestamp();
            m_capital->InitWar();

//             m_monsterSiege->Init();
//
//
            m_activityMgr->Init();
        }

        Unit* MapMgr::CreateUnit(const map::MapUnitTpl* tpl, int x, int y, int id)
        {
            if (!CanPlace(tpl, x, y)) {
                return nullptr;
            }
            bool isNew = false;
            if (id == 0) {
                isNew = true;
                id = (tpl->IsMonster()) ? GenerateDynamicUnitID() : GenerateID();
            }

            Unit* unit = nullptr;
            switch (tpl->type) {
                case MapUnitType::FARM_FOOD:
                case MapUnitType::FARM_WOOD:
                case MapUnitType::MINE_IRON:
                case MapUnitType::MINE_STONE: {
                    unit = new ResNode(id, tpl->ToResource(), Point(x, y));
                    m_refreshUnits.push_back(unit);
                }
                break;
                case MapUnitType::MONSTER: {
                    unit = new Monster(id, tpl->ToMonster(), Point(x, y));
                    m_refreshUnits.push_back(unit);
                }
                break;
                case MapUnitType::CHOW:
                case MapUnitType::PREFECTURE: {
                    unit = new ChowPrefecture(id, tpl->ToFamousCity(), Point(x, y));
                    m_cities.push_back(unit);
                }
                break;
                case MapUnitType::COUNTY: {
                    unit = new County(id, tpl->ToFamousCity(), Point(x, y));
                    m_cities.push_back(unit);
                }
                break;
                case MapUnitType::CAPITAL: {
                    Capital* capital = new Capital(id, tpl->ToCapital(), Point(x, y));
                    m_capital = capital;
                    unit = m_capital;
                    m_cities.push_back(unit);
                }
                break;
                case MapUnitType::CAMP_FIXED: {
                    unit = new CampFixed(id, tpl->ToCampFixed(), nullptr);
                    m_campFixed.push_back(unit);
                }
                break;
//                 case MapUnitType::CAMP_TEMP: {
//                     unit = new CampTemp(id, tpl->ToCampTemp(), nullptr);
//                 }
//                 break;
                case MapUnitType::TREE: {
                    unit = new Tree(id, tpl->ToTree(), Point(x, y));
                }
                break;
                case MapUnitType::CATAPULT: {
                    unit = new Catapult(id, tpl->ToCatapult(), Point(x, y));
                    m_catapults.push_back(static_cast<Catapult*>(unit));
                }
                break;
                case MapUnitType::WORLD_BOSS: {
                    unit = new WorldBoss(id, tpl->ToWorldBoss(), Point(x, y));
                }
                break;
                default:
                    LOG_ERROR("Map::CreateUnit type=%d not exist\n", (int)tpl->type);
                    break;
            }

            if (unit) {
                if (isNew) {
                    unit->Init();
                }
                MarkPlace(tpl, x, y);
                m_units[x][y] = unit;
                unit->SetPosition(x, y);
                OnUnitAdd(unit);

                if (unit->IsTree()) {
                    SetTreePos(unit);
                }
            }
            return unit;
        }

        void MapMgr::CreateResource(MapUnitType type, int level, ResAreaType areaType, int unitCount) {
            int curIndex = 0;
            std::vector<Point> vecPoint;
            for (auto & pointSet : m_blockPoints) {
                vecPoint.insert(vecPoint.end(), pointSet.begin(), pointSet.end());
            }

            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(vecPoint.begin(), vecPoint.end(), g);

            LOG_DEBUG("unittype: %d level:%d resType: %d count: %d", type, level, areaType, unitCount);
            if (const MapUnitTpl* tpl_unit = m_tploader->FindMapUnit(type, level)) {
                if (unitCount <= 0) { return; }
                //int totalCount = unitCount / 3;
                int totalCount = unitCount;
                int single_count = 0;

                auto isExsitNearly = [this](int x, int y) {
                    int xL = x - 1;
                    int xR = x + 1;
                    int yD = y - 1;
                    int yU = y + 1;

                    int count = 0;
                    for (int i = xL; i <= xR; ++i) {
                        for (int j = yD; j <= yU; ++j) {
                            if (CheckPos(i, j) &&  this->m_walkable[i][j] == false) {
                                if (++count >= 2) {
                                    return true;
                                }
                            }
                        }
                    }
                    return false;
                };

                for (int i = curIndex; i < (int)vecPoint.size(); ++i) {
                    cout << "=vecPoint, x = " << vecPoint.at(i).x << ", y = " << vecPoint.at(i).y << endl;
                    ++curIndex;
                    auto& point = vecPoint.at(i);

                    if (isExsitNearly(point.x, point.y)) {
                        continue;
                    }

                    if (!InResArea(areaType, point.x, point.y)) {
                        continue;
                    }

                    if (InCapitalArea(point)) {
                        continue;
                    }

                    /*if (type == MapUnitType::MONSTER && MonsterDensity(point)) {
                        continue;
                    }*/

                    if (this->CreateUnit(tpl_unit, point.x, point.y)) {
                        m_count++;
                        single_count++;
                        if (--totalCount <= 0) {
                            break;
                        }
                    }
                }
                cout << "=createResource=>type  " << (int)type << "  level  "  << level <<  "  count  " << unitCount - totalCount 
                    << " m_count : " << m_count << endl;
            }
        }

        void MapMgr::RefreshMonster(){
            //刷新补齐野怪
            int refresh_count = 0;
            int serverLv = 1; //服务器等级，现在默认为1
            auto monster_tpl = m_tploader->GetMonsterDist(serverLv);             
            for (const auto& t : monster_tpl->distribute) {
                int regionLv = t.first;
                auto value = m_globalData.monsterDist.Get(regionLv);
                if (value) {
                    auto& table = value->ToTable();
                    for (const auto& pair : t.second.level_dist) {
                        int level = pair.first;   
                        int num = pair.second;
                        auto current_num = table.Get(level)->ToNumber();
                        if (current_num) {
                            int fill_num = num  - current_num;
                            //LOG_DEBUG(" Refresh Monster NNN :%d ", fill_num);
                            if (fill_num > 0 && refresh_count <= 2000) {
                                LOG_DEBUG(" Refresh Monster Num:%d ", fill_num);
                                CreateResource(MapUnitType::MONSTER, level, (ResAreaType)regionLv, fill_num);
                                refresh_count += fill_num;
                                table.Set(level, current_num + fill_num);
                                m_globalData.isDirty = true;
                            }
                        }
                    }
                } 
            }
        }

        void MapMgr::InitMapUnits()
        {
            cout << "=== create units begin ===" << endl;
            if (!m_isDbLoad) {
                //箭塔
                //每种城市类型对应的RGB的green值
                int rgbGreenPerCityType[] = {10, 50, 100, 0};
                string worldMapBmp = framework.resource_dir() + "/world_map.bmp";
                bitmap_image image(worldMapBmp);
                if (!image) {
                    LOG_ERROR("Error - Failed to open: input.bmp\n");
                    return;
                }
                unsigned char red = 0;
                unsigned char green = 0;
                unsigned char blue = 0;

                const unsigned int height = image.height();
                const unsigned int width  = image.width();

                auto getCityType = [&](int green) {
                    for (int i = 0; i < 4; ++i) {
                        if (rgbGreenPerCityType[i] == green) {
                            return i + 1;
                        }
                    }
                    return 0;
                };

                for (std::size_t y = 0; y < height; ++y) {
//                     int count = 0;
                    for (std::size_t x = 0; x < width; ++x) {
                        image.get_pixel(x, y, red, green, blue);
                        int pointX = x;
                        int pointY = MAP_HEIGHT - y - 1;
                        if (green != 255) {
                            int cityType = getCityType(green);
                            if (cityType > 0) {
                                if (const MapUnitTpl* tpl_unit = tpl::m_tploader->FindMapUnit((model::MapUnitType)cityType, 1)) {
                                    //创建名城
                                    CreateUnit(tpl_unit, pointX, pointY);
                                }
                            }
                        }
                    }
                }

                if (const MapUnitTpl* tpl_unit = m_tploader->FindMapUnit(MapUnitType::CATAPULT, 1)) {
                    CreateUnit(tpl_unit, 603, 595);
                    CreateUnit(tpl_unit, 595, 595);
                    CreateUnit(tpl_unit, 595, 603);
                    CreateUnit(tpl_unit, 603, 603);
                }

                // 保存unwalkable的坐标
                string dir = framework.resource_dir() + "/map.txt";
                ofstream file(dir.c_str());
                for (int i = 0; i < 1200; ++i) {
                    for (int j = 0; j < 1200; ++j) {
                        if (!m_walkable[i][j]) {
                            file <<  i << ',' <<  j <<  '\n';
                        }
                    }
                }
                file.close();

                unsigned int curIndex = 0;
                std::vector<Point> vecPoint;
                for (auto & pointSet : m_blockPoints) {
                    vecPoint.insert(vecPoint.end(), pointSet.begin(), pointSet.end());
                }

                std::random_device rd;
                std::mt19937 g(rd());
                std::shuffle(vecPoint.begin(), vecPoint.end(), g);

                LOG_DEBUG("vector size --- %d --- ", vecPoint.size());

                auto createResource = [&](MapUnitType type, int level, ResAreaType areaType, int unitCount) {
                    LOG_DEBUG("unittype: %d level:%d resType: %d count: %d", type, level, areaType, unitCount);
                    if (const MapUnitTpl* tpl_unit = m_tploader->FindMapUnit(type, level)) {
                        if (unitCount <= 0) { return; }
                        int totalCount = unitCount;
                        int single_count = 0;

                        if (curIndex >= vecPoint.size()) 
                        {
                            curIndex = 0;
                            vecPoint.clear();
                            for (auto & pointSet : m_blockPoints) {
                                vecPoint.insert(vecPoint.end(), pointSet.begin(), pointSet.end());
                            }

                            LOG_DEBUG("new vector size --- %d --- !!!", vecPoint.size());
            
                            std::random_device rd;
                            std::mt19937 g(rd());
                            std::shuffle(vecPoint.begin(), vecPoint.end(), g);
                        }
                        for (int i = curIndex; i < (int)vecPoint.size(); ++i) {
                            auto& point = vecPoint.at(i);
                            if (!InResArea(areaType, point.x, point.y)) {
                                continue;
                            }

                            if (this->m_walkable[point.x][point.y] == false) {
                                continue;
                            }

                            if (InCapitalArea(point)) {
                                continue;
                            }

                            if (this->CreateUnit(tpl_unit, point.x, point.y)) {
                                m_count++;
                                single_count++;
                                if (--totalCount <= 0) {
                                    break;
                                }
                            }
                        }
                        cout << "=createResource=>type  " << (int)type << "  level  "  << level <<  "  count  " << unitCount - totalCount 
                            << " m_count : " << m_count << endl;
                    }
                };

                auto res_tpl = m_tploader->GetResourceRange();
                for (auto t : res_tpl) {
                    int regionLv = t.first;
                    for(const auto& num_tpl : t.second->resourceNumTpl) {
                        //LOG_DEBUG(" -------------rangeLv:%d ---- resourceLv:%d ", regionLv, num_tpl.resourceLv);
                        createResource(MapUnitType::FARM_FOOD, num_tpl.resourceLv, (ResAreaType)regionLv, num_tpl.food);
                        createResource(MapUnitType::FARM_WOOD, num_tpl.resourceLv, (ResAreaType)regionLv, num_tpl.wood);
                        createResource(MapUnitType::MINE_IRON, num_tpl.resourceLv, (ResAreaType)regionLv, num_tpl.iron);
                        createResource(MapUnitType::MINE_STONE,  num_tpl.resourceLv, (ResAreaType)regionLv, num_tpl.ore);
                    }
                }
                LOG_DEBUG(" -------------After Resource countUnit:%d ---- ", m_count);

                int serverLv = 1; //服务器等级，现在默认为1
                auto monster_tpl = m_tploader->GetMonsterDist(serverLv);             
                for (const auto& t : monster_tpl->distribute) {
                    int regionLv = t.first;
                    DataTable table;
                    for (const auto& pair : t.second.level_dist) {
                        int level = pair.first;
                        int num = pair.second;
                        //LOG_DEBUG(" -------------rangeLv:%d ---- monsterLv:%d  num:%d ", regionLv, level, num);
                        createResource(MapUnitType::MONSTER, level, (ResAreaType)regionLv, num);
                        
                        table.Set(level, num);
                    }
                    //统计野怪数
                    m_globalData.monsterDist.Set(regionLv, table);
                    m_globalData.isDirty = true;
                }

                LOG_DEBUG(" -------------countUnit:%d ---- ", m_count);

                cout << "=== MapUnitType::TREE ===" << endl;
                //Todo: 先把创建树的逻辑去掉
                // createResource(MapUnitType::TREE, 1, 25000 * 3);
                // createResource(MapUnitType::TREE, 2, 25000 * 3);
            }

            cout << "=== create units end. ===" << endl;
        }

        void MapMgr::AddUnitFromDb(int id, int x, int y, const std::string& data, const tpl::MapUnitTpl* tpl)
        {
            //Debug();
            Unit* unit = nullptr;
            if (const MapUnitCastleTpl* castle_tpl = tpl->ToCastle()) {
                unit = new Castle(id, castle_tpl, nullptr, castle_tpl->level);
                MarkPlace(castle_tpl, x, y);
                m_units[x][y] = unit;
                unit->SetPosition(x, y);
                OnUnitAdd(unit);
            } else {
                unit = CreateUnit(tpl, x, y, id);
            }
            if (unit) {
                unit->Deserialize(data);
                unit->SetClean();
            }
            //把每个联盟拥有的名城放在一个数据结构中
            if (tpl->type == MapUnitType::COUNTY || tpl->type == MapUnitType::CAPITAL || tpl->type == MapUnitType::CHOW || tpl->type == MapUnitType::PREFECTURE)
            {
                FamousCity* famouscity = dynamic_cast<FamousCity*>(unit);
                int64_t allianceId = famouscity->allianceId();
                if (allianceId > 0)
                {
                    if (famouscity && famouscity->cityTpl()) {
                        auto it = m_allianceCity.find(allianceId);
                        if (it != m_allianceCity.end()) {
                                it->second.emplace_back(famouscity);
                        } else {
                            m_allianceCity.emplace(allianceId, std::list<FamousCity*>(1, famouscity));
                        }
                    }
                }
            }
        }

        void MapMgr::AddTroopFromDb(int id, const string& data)
        {
            if (id == 0) {
                LOG_DEBUG("Map::AddTroopFromDb id = %d, data = %s", id, data.c_str());
            }
            if (Troop* troop = TroopFactory::instance().CreateTroop(id, data)) {
//             if (Troop* troop = new Troop(id)) {
//                 if (troop->Deserialize(data)) {
                m_troops.emplace(id, troop);
                OnTroopAdd(troop);
//                 }
            }
        }

        void MapMgr::OnIntervalUpdate()
        {
            if (!m_isSetupFinish) {
                return;
            }

            // 整点刷新
            if (m_lastUpdateSecond % base::utils::SECONDS_OF_ONE_HOUR ==  0) {
                tm* p = localtime(&m_lastUpdateSecond);
                OnHour(p->tm_hour);
            }
          
            ++m_lastUpdateSecond;
            ++m_updateSeconds;

            // 一些定时更新的逻辑
            int64_t now = g_dispatcher->GetTimestampCache();

            size_t length = m_refreshUnits.size();
            if (m_refreshIndex >= length) {
                m_refreshIndex = 0;
            }

            int max = 1000;
            for (; m_refreshIndex < length; ++m_refreshIndex) {
                max--;
                Unit* unit = m_refreshUnits[m_refreshIndex];
                if (unit && now >= unit->refreshTime()) {
                    bool refresh = false;
                    switch (unit->type()) {
                        case model::MapUnitType::FARM_FOOD:
                        case model::MapUnitType::FARM_WOOD:
                        case model::MapUnitType::MINE_IRON:
                        case model::MapUnitType::MINE_STONE: {
                            ResNode* res = unit->ToResNode();
                            if (res->troopId() == 0 && !res->IsHurtKeep() && res->noviceuid() ==  0) {
                                refresh = true;
                            }
                        }
                        break;
                        case model::MapUnitType::MONSTER: {
                            refresh = true;
                        }
                        break;
                        default:
                            break;
                    }

                    if (refresh) {
                        UpdateUnit(unit);
                        break;
                    }
                    if (max <= 0) {
                        break;
                    }
                }
            }

            // 通知所有名城刷新
            for (auto & unit : m_cities) {
                auto city = unit->ToFamousCity();
                if (city != nullptr) {
                    city->OnIntervalUpdate(m_updateSeconds);
                }
            }

            // 通知所有的行营刷新
            for (auto & unit : m_campFixed) {
                if (unit)
                {
                    auto campFixed = unit->ToCampFixed();
                    if (campFixed != nullptr) {
                        campFixed->OnIntervalUpdate(m_updateSeconds);
                    }
                }
            }

            if (m_updateSeconds % 60 == 0) {
                // castle Avg Level
                UpdateCastleAvgLevel();

                // per 5 minutes SaveToDB
                g_dataService->On5MinuteSave();
                SaveMapGlobalData();

                //释放unit
                for (auto it = m_unitsToDelete.begin(); it != m_unitsToDelete.end();) {
                    auto unit = *it;
                    if (unit->IsCampFixed()) {
                        // 删除行营
                        m_runner.PushCommand(new qo::CommandUnitDelete(unit->id()));
                    }
                    SAFE_DELETE(unit);
                    it = m_unitsToDelete.erase(it);
                }
            }

            if (m_updateSeconds % 10 == 0) {
                string json = base::dbo::g_dbpool->SqlCountsToJson();
                m_runner.PushCommand(new qo::CommandStatSave("sql_counts_ms", json));
            }
        }

        void MapMgr::OnHour(int hour)
        {
            if (hour ==  0 ||  hour ==  5) {
                for (auto& value : m_agents) {
                    auto agent = value.second;
                    if (agent !=  nullptr) {
                        agent->OnHour(hour);
                    }
                }
            }

            if (hour == 0) {
                CheckTotalPower();
            }

            // RefreshMonster();
        }

        bool MapMgr::CheckTotalPower()
        {
            int64_t totalPower = 0;
            for (auto& value : m_agents) {
                auto agent = value.second;
                if (agent !=  nullptr) {
                    totalPower += agent->totalPower();
                }
            }
            return OnTotalPowerChange(totalPower);
        }

        void MapMgr::UpdateCastleAvgLevel()
        {
            int32_t levels = 0;
            int32_t count = 0;
            for (auto it = m_agents.begin(); it != m_agents.end(); ++it) {
                if (Agent* agent = it->second) {
                    if (agent->isLocalPlayer() && agent->castle() && agent->castle()->level() >= 8) {
                        levels += agent->castle()->level();
                        count++;
                    }
                }
            }
            if (levels > 0) {
                m_castleLevelAvg = (int)ceil(levels / count);
                if (m_castleLevelAvg > 30) {
                    m_castleLevelAvg = 30;
                }
            }
        }

        void MapMgr::OnUpdate()
        {
            //更新行军
            int64_t now = g_dispatcher->GetTickCache();
            for (auto it = m_troops.begin(); it != m_troops.end(); ++it) {
                Troop* tp = it->second;
                tp->OnUpdate(now, now - m_lastUpdateTick);
            }
            m_lastUpdateTick = now;
        }

        Castle* MapMgr::CreateNewCastle(Agent& agent, int level)
        {
//             if (Castle* castle = FindCastleByUid(agent.uid())) {
            if (Castle* castle = FindCastleByPos(agent.castlePos())) {
                if (castle->level() != level) {
                    OnCastleLevelUp(castle, level);
                }
                return castle;
            }

            auto moveFarm = [this](const Point & pos) {
                auto matcher = [](Unit * u) {
                    return u->type() == MapUnitType::FARM_FOOD && u->tpl().level == 1 && u->ToResNode()->troopId() == 0;
                };
                Unit* farm = FindNearestUnit(pos, matcher);
                if (farm != nullptr) {
                    Point start;
                    for (int range = 1; range <= 5; ++range) {
                        int grid = 2 * range;
                        start.x = pos.x - range;
                        start.y = pos.y - range;
                        for (int x = start.x; x < start.x + grid; ++x) {
                            if (MoveUnit(farm, x, start.y)) {
                                return farm;
                            }
                        }
                        start.x = pos.x + range;
                        for (int y = start.y; y < start.y + grid; ++y) {
                            if (MoveUnit(farm, start.x, y)) {
                                return farm;
                            }
                        }
                        start.y = pos.y + range;
                        for (int x = start.x; x > start.x - grid; --x) {
                            if (MoveUnit(farm, x, start.y)) {
                                return farm;
                            }
                        }
                        start.x = pos.x - range;
                        for (int y = start.y; y > start.y - grid; --y) {
                            if (MoveUnit(farm, start.x, y)) {
                                return farm;
                            }
                        }
                    }
                }
                return farm;
            };

            if (const MapUnitCastleTpl* tpl = m_tploader->FindMapUnit(model::MapUnitType::CASTLE, level)->ToCastle()) {
                Castle* castle = new Castle(GenerateID(), tpl, &agent, level);
                while (castle) {
                    Point pos = GetRandomPoint();
                    if (InResArea(model::ResAreaType::FIRST, pos.x, pos.y)) {
                        if (CanPlace(tpl, pos.x, pos.y)) {
                            MarkPlace(tpl, pos.x, pos.y);
                            m_units[pos.x][pos.y] = castle;
                            castle->SetPosition(pos.x, pos.y);
                            agent.SetCastlePos(castle->pos());
                            OnUnitAdd(castle);
                            auto farm = moveFarm(castle->pos());
                            if (farm) {
                                if (ResNode* res = farm->ToResNode()) {
                                    res->SetNoviceuid(agent.uid());
                                }
                            }
    //                         moveMonster(castle->pos());
                            qo::g_dataService->AppendData(qo::g_dataService->CreateDbInfo(&agent));
                            qo::g_dataService->AppendData(qo::g_dataService->CreateDbInfo(castle));

                            OnCreateCastle(castle);
                            return castle;
                        }
                    }
                }
            }
            return nullptr;
        }

        void MapMgr::ChangeKing(int64_t uid)
        {
            if (Agent* agt = FindAgentByUID(uid)) {
                m_king = agt;
                BroadBeKing();
                m_canChangeServerName = true;
            } else {
                //清理所有数据
                m_king = nullptr;
                //m_serverName = capital().tpl().name;
                m_canChangeServerName = false;
                m_resourceNodeRateId = 0;
                m_lastChangeResIdTime = 0;
            }
            capital().OnKingChanged();
            for (Catapult * cp : catapults()) {
                cp->NoticeUnitUpdate();
            }

            m_runner.PushCommand(new CommandServerInfoSave());
            //m_runner.PushCommand(new CommandPalaceWarSave());
        }

        bool MapMgr::ChangeServerName(const string& name)
        {
            if (m_canChangeServerName) {
                m_serverName = name;
                m_canChangeServerName = false;
                m_runner.PushCommand(new CommandServerInfoSave());
                //m_runner.PushCommand(new CommandPalaceWarSave());
                capital().NoticeUnitUpdate();
                return true;
            }
            return false;
        }

        bool MapMgr::ChangeResNodeRate(int id)
        {
            //cout << "ChangeResNodeRate id = " << id << " size = " << g_tploader->configure().resourceRates.size() << endl;
            if (id > 0 && id < (int)g_tploader->configure().resourceRates.size()) {
                if (!base::utils::is_today_from_timestamp(m_lastChangeResIdTime)) {
                    m_resourceNodeRateId = id;
                    m_lastChangeResIdTime = g_dispatcher->GetTimestampCache();
//                     m_runner.PushCommand(new CommandPalaceWarSave());
                    //广播
                    m_mapProxy->BroadNoticeMessage(1903005, 2, to_string(m_resourceNodeRateId));
                    return true;
                }
            }
            return false;
        }

        void MapMgr::MoveLoseCastle(const Point& pos, const int width, const int height, const Unit* origin_unit, const int allianceId)
        {
            auto matcher = [](Unit * u, int allianceId) {
                if (u->type() != model::MapUnitType::CASTLE) {
                    return false;
                }    
                Castle* castle = u->ToCastle();
                //LOG_DEBUG("castle  ---------------1111----- %d  %d", (int)castle->allianceId(), allianceId);
                if ((int)castle->allianceId() != allianceId) {
                    return false;
                }       
                //LOG_DEBUG("castle  -------------------- %d  %d", castle->allianceId(), allianceId);
                return true;  
            };

            //找名城原联盟内的玩家城池
            // Point start = Point(pos.x - width, pos.y - height);
            // Point end = Point(pos.x + width, pos.y + height);

            auto city_tpl = m_tploader->FindMapUnit(origin_unit->type(), 1);
            auto limit = GetCenterPosLimit(pos, city_tpl, width);
            LOG_DEBUG("pos x:%d y:%d ---  start x:%d  y: %d  ---  end  x:%d y:%d ", 
                pos.x, pos.y, limit.start.x, limit.start.y, limit.end.x, limit.end.y);

            auto units = GetUnitsInRect(limit.start, limit.end, allianceId, matcher);
            int total_units_size = units.size();
            int index = 0;  //游标
            LOG_DEBUG("MoveLoseCastle  -------------------- 2");

            auto near_matcher = [](Unit * u, int allianceId) {
                if (u->type() != MapUnitType::CAPITAL && u->type() != MapUnitType::CHOW && u->type() != MapUnitType::PREFECTURE 
                    && u->type() != MapUnitType::COUNTY)
                {
                    return false;
                }
                
                FamousCity* city = u->ToFamousCity();
                if ((int)city->allianceId() != (int64_t)allianceId) {
                    return false;
                }
                LOG_DEBUG("MoveLoseCastle  ----- nearst city --- %d  ----%d ---- %d ", allianceId, city->bornPoint().x, 
                    city->bornPoint().y);
                return true;
            };
            

            if (total_units_size == 0 ) {
                LOG_DEBUG("MoveLoseCastle  have no castle to move ");
                return;
            }

            //找到最近同联盟的名城，迁移  略过第一个也就是玩家自己的位置
            Unit* u = FindNearestUnit(pos, allianceId, near_matcher, true);
            FamousCity* city = nullptr;
            if (u != nullptr) {
                city = u->ToFamousCity();
            }
        
            //迁城至最近联盟名城
            LOG_DEBUG("MoveLoseCastle  ------- total_size: %d ", total_units_size);
            if (city != nullptr && total_units_size > 0)
            {
                Point city_pos = city->bornPoint();
                Point start;
                const MapUnitTpl* tpl = m_tploader->FindMapUnit(model::MapUnitType::CASTLE, 1);
                LOG_DEBUG("MoveLoseCastle  ----------- have alliance city");
                for (int range = 1; range <= width; ++range) {
                    if (index >= total_units_size) {
                        break;
                    }
                    Unit* unit = units[index];

                    int grid = 2 * range;
                    start.x = city_pos.x - range;
                    start.y = city_pos.y - range;
                    for (int x = start.x; x < start.x + grid; ++x) {
                        if (!CheckPos(x, start.y) || m_units[x][start.y]) {
                            continue;
                        }
                        if (CanPlace(tpl, x, start.y)) {
                            if (MoveUnit(unit, x, start.y)) {
                                index++;
                                break;
                            }
                        }
                    }
                    if (index >= total_units_size) {
                        break;
                    }
                    unit = units[index];

                    start.x = city_pos.x + range;
                    for (int y = start.y; y < start.y + grid; ++y) {
                        if (!CheckPos(start.x, y) || m_units[start.x][y]) {
                            continue;
                        }
                        if (CanPlace(tpl, start.x, y)) {
                            if (MoveUnit(unit, start.x, y)) {
                                index++;
                                break;
                            }
                        }
                    }
                    if (index >= total_units_size) {
                        break;
                    }
                    unit = units[index];

                    start.y = city_pos.y + range;
                    for (int x = start.x; x > start.x - grid; --x) {
                        if (!CheckPos(x, start.y) || m_units[x][start.y]) {
                            continue;
                        }
                        if (CanPlace(tpl, x, start.y)) {
                            if (MoveUnit(unit, x, start.y)) {
                                index++;
                                break;
                            }
                        }
                    }
                    if (index >= total_units_size) {
                        break;
                    }
                    unit = units[index];

                    start.x = city_pos.x - range;
                    for (int y = start.y; y > start.y - grid; --y) {
                        if (!CheckPos(start.x, y) || m_units[start.x][y]) {
                            continue;
                        }
                        if (CanPlace(tpl, start.x, y)) {
                            if (MoveUnit(unit, start.x, y)) {
                                index++;
                                break;
                            }
                        }
                    }
                }
            }
            LOG_DEBUG("MoveLoseCastle  -------------------- 4  %d %d", index, total_units_size);

            if (index < total_units_size) {
                LOG_DEBUG("random teleport ");
                for (int i = index; i < total_units_size; i++) {
                    Unit* unit = units[index];
                    if (!RandomMoveInWholeMap(unit)) {
                        LOG_DEBUG("random move failed");
                    } 
                }
            }
        }

        void MapMgr::AllianceOwnCity(int64_t allianceId, FamousCity* city)
        {
            if (city && city->cityTpl()) {
                auto it = m_allianceCity.find(allianceId);
                if (it != m_allianceCity.end()) {
                    it->second.emplace_back(city);
                } else {
                    m_allianceCity.emplace(allianceId, std::list<FamousCity*>(1, city));
                }

                DataTable dtAllianceOwnCity;
                dtAllianceOwnCity.Set("allianceId", allianceId);
                dtAllianceOwnCity.Set("cityId", city->cityTpl()->cityId);
                m_mapProxy->SendToCS([&](base::lua::LuaMessageWriter & lmw) {
                    lmw.WriteArgumentCount(2);
                    lmw.WriteString("allianceOwnCity");
                    lmw.WriteValue(dtAllianceOwnCity);
                });

                m_mapProxy->SendcsAddAllianceRecord(allianceId, model::AllianceMessageType::NEUTRALCASTLE_OCCUPY, city->cityTpl()->name);
            }
        }

        void MapMgr::AllianceLoseCity(int64_t allianceId, FamousCity* city, int atkAllianceId)
        {
            if (city && city->cityTpl()) {
                auto it = m_allianceCity.find(allianceId);
                if (it != m_allianceCity.end()) {
                    it->second.remove(city);
                }

                DataTable dtAllianceLoseCity;
                dtAllianceLoseCity.Set("allianceId", allianceId);
                dtAllianceLoseCity.Set("cityId", city->cityTpl()->cityId);

                m_mapProxy->SendToCS([&](base::lua::LuaMessageWriter & lmw) {
                    lmw.WriteArgumentCount(2);
                    lmw.WriteString("allianceLoseCity");
                    lmw.WriteValue(dtAllianceLoseCity);
                });

                // 联盟日志
                std::string param(city->cityTpl()->name);
                std::string allianceName = "";
                if (AllianceSimple* as = alliance().FindAllianceSimple(atkAllianceId)) {
                    allianceName = as->info.name;
                }
                param.append(",");
                param.append(allianceName);
                m_mapProxy->SendcsAddAllianceRecord(allianceId, model::AllianceMessageType::NEUTRALCASTLE_ROBBED, param);
            }
        }

        void MapMgr::AllianceLoseOccupyCity(int64_t allianceId, FamousCity* city, int atkAllianceId)
        {
            if (city && city->cityTpl()) {
                auto it = m_allianceCity.find(allianceId);
                if (it != m_allianceCity.end()) {
                    it->second.remove(city);
                }

                DataTable dtAllianceLoseCity;
                dtAllianceLoseCity.Set("allianceId", allianceId);
                dtAllianceLoseCity.Set("cityId", city->cityTpl()->cityId);

                m_mapProxy->SendToCS([&](base::lua::LuaMessageWriter & lmw) {
                    lmw.WriteArgumentCount(2);
                    lmw.WriteString("allianceLoseOccupyCity");
                    lmw.WriteValue(dtAllianceLoseCity);
                });
            }
        }

        void MapMgr::AllianceOwnOccupyCity(int64_t allianceId, FamousCity* city)
        {
            if (city && city->cityTpl()) {
                auto it = m_allianceCity.find(allianceId);
                if (it != m_allianceCity.end()) {
                    it->second.emplace_back(city);
                } else {
                    m_allianceCity.emplace(allianceId, std::list<FamousCity*>(1, city));
                }

                DataTable dtAllianceOwnCity;
                dtAllianceOwnCity.Set("allianceId", allianceId);
                dtAllianceOwnCity.Set("cityId", city->cityTpl()->cityId);

                m_mapProxy->SendToCS([&](base::lua::LuaMessageWriter & lmw) {
                    lmw.WriteArgumentCount(2);
                    lmw.WriteString("allianceOwnOccupyCity");
                    lmw.WriteValue(dtAllianceOwnCity);
                });
            }
        }

        void MapMgr::AllianceCityBeAttacked(int64_t allianceId, FamousCity* city, Troop* atkTroop)
        {
            if (city && city->cityTpl()) {
                // 联盟日志
                std::string param(city->cityTpl()->name);
                if (atkTroop) {
                    param.append(",");
                    param.append(atkTroop->nickname());
                }
                m_mapProxy->SendcsAddAllianceRecord(allianceId, model::AllianceMessageType::NEUTRALCASTLE_BE_ATTACKED, param);
            }
        }

        void MapMgr::AllianceCastleBeAttacked(int64_t allianceId, Castle* castle, Troop* atkTroop)
        {
            if (castle && atkTroop) {
                // 联盟日志
                std::string param(castle->nickname());
                param.append(",");
                param.append(atkTroop->nickname());
                m_mapProxy->SendcsAddAllianceRecord(allianceId, model::AllianceMessageType::ALLY_BE_ATTACKED, param);
            }
        }

        void MapMgr::AllianceCatapultBeAttacked(int64_t allianceId, Catapult* catapult, Troop* atkTroop)
        {
            if (catapult && atkTroop) {
                std::string nickName = "";
                // 联盟日志
                if (catapult->troop())
                {
                    nickName = catapult->troop()->nickname();
                }
                std::string param(nickName);
                param.append(",");
                param.append(atkTroop->nickname());
                m_mapProxy->SendcsAddAllianceRecord(allianceId, model::AllianceMessageType::ALLY_BE_ATTACKED, param);
            }
        }

        bool MapMgr::CheckSiegeCityLimit(int64_t allianceId, model::MapUnitType attackUnitType, Agent& agent)
        {
            //这里判断上一级名城是否足够
            int limitNum = 0;
            model::MapUnitType nextUnitType;
            if (attackUnitType == model::MapUnitType::CHOW)
            {
                nextUnitType = model::MapUnitType::PREFECTURE;
                limitNum = g_tploader->configure().cityNumLimit.chow;
            }
            else if (attackUnitType == model::MapUnitType::PREFECTURE)
            {
                nextUnitType = model::MapUnitType::COUNTY;
                limitNum = g_tploader->configure().cityNumLimit.prefecture;
            }
            else
            {
                return true;
            }
            int nextCount = 0;
            int attackCount = 0;
            if (auto famousCityList = AllianceCity(allianceId)) {
                for (auto it = famousCityList->begin(); it != famousCityList->end(); ++it)
                {
                    if ((*it)->type() == nextUnitType)
                        nextCount++;
                    if ((*it)->type() == attackUnitType)
                        attackCount++;
                }
                
            }
            if (limitNum == 0) {
                LOG_DEBUG("Map::CheckSiegeCityLimit LimitNum is %d\n", 0);
            } 
            else {
                int canHaveNum = nextCount / limitNum;
                if (canHaveNum > attackCount)
                {
                    return true;
                }
                else 
                {   
                    attackCount++;
                    int needNum = attackCount * limitNum - nextCount;
                    if (attackUnitType == model::MapUnitType::CHOW)
                    {
                        agent.proxy()->SendNoticeMessage(ErrorCode::TROOP_PREFECTURE_CITY_NOT_ENOUGH, 1, std::to_string(needNum));
                    }
                    else if (attackUnitType == model::MapUnitType::PREFECTURE) {
                        agent.proxy()->SendNoticeMessage(ErrorCode::TROOP_COUNTY_CITY_NOT_ENOUGH, 1, std::to_string(needNum));
                    }
                }
            }           
            cout << "###MapUnitType" << (int) attackUnitType << endl;
            return false;
        }


        bool MapMgr::IsViewingUnit(const Point& unitPos, const Point& viewPos, int range)
        {
            Point p = TransPos(unitPos);

            Point vp = TransPos(viewPos);
            PosLimit vlimit = GetAreaPosLimit(vp, range);
            if (is_in_rect(p, vlimit.start, vlimit.end)) {
                return true;
            }
            return false;
        }

        bool MapMgr::IsViewingTroop(const Point& from, const Point& to, const Point& viewPos, int range)
        {
            if (range < 0) {
                return false;
            }
            range = range > MAX_RANGE ? MAX_RANGE : range;

            Point vp = TransPos(viewPos);
            PosLimit vlimit = GetAreaPosLimit(vp, range);

            for (int i = vlimit.start.x; i <= vlimit.end.x; ++i) {
                for (int j = vlimit.start.y; j <= vlimit.end.y; ++j) {
                    Area& area = m_areas[i][j];
                    if (area.IsCross(from, to)) {
                        return true;
                    }
                }
            }
            return false;
        }

        void MapMgr::Enter(int64_t uid, const MailboxID& mbid)
        {
            Agent* agent = FindAgentByUID(uid);
            if (agent == nullptr) {
                agent = new Agent(uid);
                m_agentsWaitInit.emplace(uid, agent);
            }
            agent->Connect(mbid);
            //std::cout << "agent ENTER uid,nodeid=" << agent->uid() << " " << agent->mbid().nodeid() << endl;
        }

        void MapMgr::Quit(int64_t uid)
        {
            if (isLocalUid(uid)) {
                if (Agent* agent = FindAgentByUID(uid)) {
                    ViewerLeave(agent);
                    agent->Disconnect();
                }
            } else {
                if (Agent* agent = FindVisitorByUID(uid)) {
                    ViewerLeave(agent);
                    agent->Disconnect();
                    VisitorLeave(uid);
                    delete agent;
                }
            }
        }

        void MapMgr::ViewerJoin(Agent* agent, int x, int y)
        {
            //cout << "isViewing = " << (int) agent->isViewing() << endl;
            if (agent->isViewing() || !CheckPos(x, y)) {
                return;
            }
            agent->SetViewPoint(x, y);
            OnViewerAdd(agent, agent->viewPoint());
        }

        void MapMgr::ViewerMove(Agent* agent, int x, int y)
        {
            if (!CheckPos(x, y)) {
                return;
            }
            // cout << "Move " << x << " " << y << endl;
            if (agent->isViewing()) {
                if (OnViewerMove(agent, agent->viewPoint(), {x, y})) {
                    agent->SetViewPoint(x, y);
                }
            } else {
                agent->SetViewPoint(x, y);
                OnViewerAdd(agent, agent->viewPoint());
            }
        }

        void MapMgr::SearchMap(Agent* agent, int x, int y, int type, int level)
        {
            if (!CheckPos(x, y)) {
                return;
            }

            if (agent->isViewing()) {
                OnSearchMap(agent, {x, y}, type, level);
            }
        }

        model::MarchErrorNo MapMgr::March(Agent& agent, MapTroopType type, const Point& from, const Point& to, int teamId)
        {
            model::MarchErrorNo errorNo = CanMarch(type, from, to, agent);
            if (errorNo == MarchErrorNo::SUCCESS) {
                if (auto troop = CreateTroop(agent, type, from, to, teamId)) {
                    agent.OnMarch(troop,  false);
                } else {
                    errorNo = MarchErrorNo::OTHER;
                }
            }
            return errorNo;
        }

        model::MarchErrorNo MapMgr::CanMarch(MapTroopType type, const Point& from, const Point& to, Agent& agent)
        {
            //cout << "Map::CanMarch" << endl;
            model::MarchErrorNo errorNo = MarchErrorNo::SUCCESS;
            if (!CheckPos(from) || !CheckPos(to) || from == to) {
                errorNo = MarchErrorNo::OTHER;
            } else {
                bool isAllowMarch = true;
                if (agent.MaxMarchingQueue() <=  agent.TroopCount()) {
                    isAllowMarch = false;

                    auto fromUnit = FindUnit(from);
                    if (fromUnit) {
                        auto fromCampFixed = fromUnit->ToCampFixed();
                        if (fromCampFixed) {
                            if (fromCampFixed->uid() ==  agent.uid()) {
                                isAllowMarch = true;
                            }
                        } else {
                            auto fromCampTemp = fromUnit->ToCampTemp();
                            if (fromCampTemp) {
                                if (fromCampTemp->uid() ==  agent.uid()) {
                                    isAllowMarch = true;
                                }
                            }
                        }
                    }
                }

                if (!isAllowMarch) {
                    agent.proxy()->SendNoticeMessage(ErrorCode::TROOP_TEAM_FULL, 1);
                    errorNo = MarchErrorNo::OTHER;
                } else {
                    switch (type) {
                        case MapTroopType::CAMP_FIXED: {
                            if (agent.MaxMarchingQueue() <=  agent.CampFixedCount()) {
                                // 行营不能超过最大行军数量
                                agent.proxy()->SendNoticeMessage(ErrorCode::TROOP_CAMPFIXED_FULL, 1, std::to_string(agent.CampFixedCount()));
                                errorNo = MarchErrorNo::OTHER;
                            } else {
                                if (const MapUnitTpl* tpl = m_tploader->FindMapUnit(model::MapUnitType::CAMP_FIXED, 1)) {
                                    if (!CanPlaceExcludeTree(tpl, to.x, to.y, false)) {
                                        errorNo = MarchErrorNo::TARGET_CAN_NOT_PLACE;
                                    }
                                }
                            }
                        }
                        break;
                        case MapTroopType::CAMP_FIXED_ATTACK: {
                            do {
                                auto toUnit = FindUnit(to);
                                if (toUnit) {
                                    if (auto toCamp = toUnit->ToCampFixed()) {
                                        if (agent.uid() !=  toCamp->uid()) {
                                            // 别人的行营，如果不是同盟的，可以攻击
                                            if (agent.allianceId() == 0 || agent.allianceId() != toCamp->allianceId()) {
                                                break;
                                            }
                                        }
                                    }
                                }
                                errorNo = MarchErrorNo::TARGET_INVALID;
                            } while (0);
                        }
                        break;
                        case MapTroopType::CAMP_FIXED_OCCUPY: {
                            do {
                                auto toUnit = FindUnit(to);
                                if (toUnit) {
                                    if (auto toCamp = toUnit->ToCampFixed()) {
                                        if (agent.allianceId() == toCamp->allianceId())
                                        {
                                            break;
                                            // if (agent.uid() ==  toCamp->uid()) {
                                            //     // 自己的行营，并且已经有部队去占领，无法再次占领
                                            //     if (toCamp->troopId() == 0) {
                                            //         if (toCamp->bindTroopId() == 0) {
                                            //             break;
                                            //         }
                                            //     }

                                            //     // 该行营已有部队
                                            //     agent.SendNoticeMessage(ErrorCode::TROOP_CAMPFIXED_EXIST_ARMY, 1);
                                            // } 
                                        }
                                        else {
                                            // 别人的行营，如果不是同盟的，可以占领
                                            if (agent.allianceId() == 0 || agent.allianceId() != toCamp->allianceId()) {
                                                if (agent.MaxMarchingQueue() <=  agent.CampFixedCount()) {
                                                    // 行营不能超过最大行军数量
                                                    agent.proxy()->SendNoticeMessage(ErrorCode::TROOP_CAMPFIXED_FULL, 1, std::to_string(agent.CampFixedCount()));
                                                    errorNo = MarchErrorNo::OTHER;
                                                    break;
                                                }
                                                break;
                                            }
                                        }
                                    }
                                }
                                errorNo = MarchErrorNo::TARGET_INVALID;
                            } while (0);
                        }
                        break;
                        case MapTroopType::CAMP_TEMP: {
                            if (const MapUnitTpl* tpl = m_tploader->FindMapUnit(model::MapUnitType::CAMP_TEMP, 1)) {
                                if (!CanPlaceExcludeTree(tpl, to.x, to.y, false)) {
                                    errorNo = MarchErrorNo::TARGET_CAN_NOT_PLACE;
                                }
                            }
                        }
                        break;
                        case MapTroopType::CAMP_TEMP_ATTACK: {

                        }
                        break;
                        case MapTroopType::SCOUT: {
                            auto toUnit = FindUnit(to);
                            if (g_tploader->configure().scoutLimit <=  agent.TroopCountByType(MapTroopType::SCOUT))
                            {
                                agent.proxy()->SendNoticeMessage(ErrorCode::SCOUT_TROOP_FULL, 1);
                                errorNo = MarchErrorNo::OTHER;
                            }
                            if (!toUnit) {
                                errorNo = MarchErrorNo::TARGET_INVALID;
                            }
                        }
                        break;
                        case MapTroopType::SIEGE_CITY: {
                                auto toUnit = FindUnit(to);
                                if (toUnit) {
                                    if (auto toCity = toUnit->ToFamousCity()) {
                                        if (agent.allianceId() == 0) {
                                            agent.proxy()->SendNoticeMessage(ErrorCode::TROOP_SIEGECITY_NO_ALLIANCE, 1);
                                            errorNo = MarchErrorNo::TARGET_INVALID;
                                        } 
                                        if (toCity->occupyAllianceId() !=  agent.allianceId() && !CheckSiegeCityLimit(agent.allianceId(), toUnit->type(), agent)) {
                                            errorNo = MarchErrorNo::TARGET_INVALID;
                                        }
                                    }
                                    if (Capital* capital = toUnit->ToCapital()){
                                        if (capital->isNotStart()) {
                                            agent.proxy()->SendNoticeMessage(ErrorCode::PALACEWAR_IS_NOT_START, 1);        
                                            errorNo = MarchErrorNo::TARGET_INVALID;
                                        } 
                                    }
                                    // 解除自身保护状态
                                    if (auto toCity = toUnit->ToFamousCity())
                                    {
                                        if (toCity->occupyAllianceId() != 0)
                                        {
                                            agent.OnRemovePeaceShield();
                                        }
                                    } 
                                } else
                                {
                                    errorNo = MarchErrorNo::TARGET_INVALID;
                                }

                        }
                        break;
                        case MapTroopType::SIEGE_CATAPULT: {
                                auto toUnit = FindUnit(to);
                                if (toUnit) {
                                    if (Catapult* catapult = toUnit->ToCatapult()) {
                                        if (agent.allianceId() == 0)
                                        {
                                            agent.proxy()->SendNoticeMessage(ErrorCode::TROOP_SIEGECITY_NO_ALLIANCE, 1);
                                            errorNo = MarchErrorNo::TARGET_INVALID;   
                                        } 
                                        if (catapult->isNotStart()) {
                                            agent.proxy()->SendNoticeMessage(ErrorCode::PALACEWAR_IS_NOT_START, 1);
                                            errorNo = MarchErrorNo::TARGET_INVALID;
                                        }
                                    }
                                } else 
                                {
                                    errorNo = MarchErrorNo::TARGET_INVALID;
                                }
                        }
                        break;
                        case MapTroopType::PATROL_CITY: {
                            do {
                                auto toUnit = FindUnit(to);
                                if (toUnit) {
                                    if (auto toCity = toUnit->ToFamousCity()) {
//                                         if (toCity->IsCapital() || toCity->IsChow() || toCity->IsPrefecture()) {
                                        if (agent.allianceId() > 0 && toCity->allianceId() == agent.allianceId()) {

                                            bool isWating = false;
                                            auto itCD = agent.cityPatrol().patrolCD.find(toCity->id());
                                            if (itCD != agent.cityPatrol().patrolCD.end()) {
                                                int64_t now = g_dispatcher->GetTimestampCache();
                                                if (itCD->second > now) {
                                                    isWating = true;
                                                }
                                            }

                                            if (!isWating) {
                                                if (auto as = alliance().FindAllianceSimple(agent.allianceId())) {
                                                    auto al = g_tploader->FindAllianceLevel(as->info.level);
                                                    if (al && al->patrolMax > agent.cityPatrol().patrolCount) {
                                                        break;
                                                    }
                                                }
                                            }
                                        }
//                                     }
                                    }
                                }
                                errorNo = MarchErrorNo::TARGET_INVALID;
                            } while (0);
                        }
                        break;
                        case MapTroopType::GATHER: {

                        }
                        break;
                        case MapTroopType::MONSTER: {  
                            auto toUnit = FindUnit(to);
                            if(toUnit){
                                if(auto toMonster = toUnit->ToMonster()){
                                    //判断怪物等级是否在玩家可以攻击的等级范围内                               
                                    if(  (toMonster->level()) > ( agent.GetMonsterDefeatedLevel()+1)  ){
                                        errorNo = MarchErrorNo::OTHER;
                                    }
                                }
                            }
                        }
                        break;
                        case MapTroopType::WORLDBOSS: {

                        }
                        break;
                        case MapTroopType::SIEGE_CASTLE: {
                            do {
                                auto toUnit = FindUnit(to);
                                if (toUnit) {
                                    if (auto toCastle = toUnit->ToCastle()) {
                                        if (toCastle->IsProtected())
                                        {
                                            agent.proxy()->SendNoticeMessage(ErrorCode::CASTLE_IS_PROTECTED, 1);
                                            errorNo = MarchErrorNo::TARGET_INVALID;
                                            break;            
                                        }
                                        if (agent.uid() !=  toCastle->uid()) {
                                            // 别人的城池，如果不是同盟的，可以攻击
                                            if (agent.allianceId() == 0 || agent.allianceId() != toCastle->allianceId()) {
                                                // 解除保护状态
                                                agent.OnRemovePeaceShield();
                                                break;
                                            }
                                        }
                                    }
                                }
                                errorNo = MarchErrorNo::TARGET_INVALID;
                            } while (0);
                        }
                        break;
                    case MapTroopType::REINFORCEMENTS: {
                        if ( Unit* unit = FindUnit ( to ) ) {
                            if ( Castle* castle = unit->ToCastle() ) {
                                if (castle->allianceId() == agent.allianceId()) {
                                    break;
                                }
                            }
                        }
                        errorNo = MarchErrorNo::TARGET_INVALID;
                    }
                    break;
                    // 调兵
                    case MapTroopType::ASSIGN: {
                            do {
                                auto toUnit = FindUnit(to);
                                if (toUnit) {
                                    if (auto toCamp = toUnit->ToCampFixed()) {
                                        if (agent.allianceId() == toCamp->allianceId())
                                        {
                                            break;
                                            // if (agent.uid() ==  toCamp->uid()) {
                                            //     // 自己的行营，并且已经有部队去占领，无法再次占领
                                            //     if (toCamp->troopId() == 0) {
                                            //         if (toCamp->bindTroopId() == 0) {
                                            //             break;
                                            //         }
                                            //     }

                                            //     // 该行营已有部队
                                            //     agent.SendNoticeMessage(ErrorCode::TROOP_CAMPFIXED_EXIST_ARMY, 1);
                                            // } 
                                        }
                                        else {
                                            // 别人的行营，如果不是同盟的，可以占领
                                            if (agent.allianceId() == 0 || agent.allianceId() != toCamp->allianceId()) {
                                                if (agent.MaxMarchingQueue() <=  agent.CampFixedCount()) {
                                                    // 行营不能超过最大行军数量
                                                    agent.proxy()->SendNoticeMessage(ErrorCode::TROOP_CAMPFIXED_FULL, 1, std::to_string(agent.CampFixedCount()));
                                                    errorNo = MarchErrorNo::OTHER;
                                                    break;
                                                }
                                                break;
                                            }
                                        }
                                    }
                                }
                                errorNo = MarchErrorNo::TARGET_INVALID;
                            } while (0);
                        }
                        break;
                        default:
                            errorNo = MarchErrorNo::OTHER;
                            break;
                    }
                }
            }

            if (errorNo != MarchErrorNo::SUCCESS) {
                printf("march fail troop type = %d, toX = %d, toY = %d, errorNo = %d\n", (int) type, to.x, to.y, (int) errorNo);
            }
            return errorNo;// == MarchErrorNo::SUCCESS;
        }
    }
}