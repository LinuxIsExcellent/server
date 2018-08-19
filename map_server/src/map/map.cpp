#include "map.h"
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

        Map::Map(int id)
        {
            m_localServiceId = id;
            m_alliance = new Alliance;
            m_activityMgr = new ActivityMgr();
            base::utils::string_append_format(m_name, "map.%d", id);
            //计算王城影响范围
            const MapUnitTpl* city_tpl = m_tploader->FindMapUnit(MapUnitType::CAPITAL, 1);
            const MapUnitFamousCityTpl* famouscity_tpl = city_tpl->ToFamousCity();
            //capital坐标 716997 
            int x = 716997 / 1200;
            int y = 716997 % 1200;
            Point capital_pos(x, y);
            int capital_width = famouscity_tpl->range.x;
            m_capital_poslimit = GetCenterPosLimit(capital_pos, city_tpl, capital_width);
            m_mapProxy = new MapProxy(*this);
        }

        Map::~Map()
        {
            delete m_mapProxy;

            for (int i = 0; i < MAP_WIDTH; ++i) {
                for (int j = 0; j < MAP_HEIGHT; ++j) {
                    SAFE_DELETE(m_units[i][j]);
                }
            }

            for (auto it = m_agentsWaitInit.begin(); it != m_agentsWaitInit.end(); ++it) {
                SAFE_DELETE(it->second);
            }

            for (auto it = m_agents.begin(); it != m_agents.end(); ++it) {
                SAFE_DELETE(it->second);
            }

            for (auto it = m_troops.begin(); it != m_troops.end(); ++it) {
                SAFE_DELETE(it->second);
            }
            for (auto it = m_troopsRemoved.begin(); it != m_troopsRemoved.end(); ++it) {
                SAFE_DELETE(it->second);
            }

            for (auto it = m_battleInfoList.begin(); it != m_battleInfoList.end(); ++it) {
                SAFE_DELETE(it->second);
            }

            SAFE_DELETE(m_alliance);
            SAFE_DELETE(m_activityMgr);
        }

        base::memory::MemoryPool& Map::mempool()
        {
            return proxy()->mempool();   
        }

        bool Map::isLocalUid(int64_t uid) const
        {
            int sid = uid > 0 ? (uid >> 32) : 0;
            if (sid > 0) {
                for (size_t i = 0; i < m_merged_ids.size(); ++i) {
                    if (m_merged_ids[i] == sid) {
                        return true;
                    }
                }
            }
            return false;
        }

        std::tuple<int, int, int, int> Map::OccupyCityCnt()
        {
            int capital = 0, chow = 0, prefecture = 0, county = 0;
            for (auto & unit : m_cities) {
                auto city = unit->ToFamousCity();
                if (city != nullptr && city->isOccupy()) {
                    auto type = city->tpl().type;
                    if (type ==  model::MapUnitType::CAPITAL) {
                        ++capital;
                    } else if (type ==  model::MapUnitType::CHOW) {
                        ++chow;
                    } else if (type ==  model::MapUnitType::PREFECTURE) {
                        ++prefecture;
                    } else if (type ==  model::MapUnitType::COUNTY) {
                        ++county;
                    }
                }
            }
            return std::make_tuple(capital, chow, prefecture, county);
        }

        void Map::UnitsForeach(const std::function<void(Unit*)>& f)
        {
            for (int i = 0; i < MAP_WIDTH; ++i) {
                for (int j = 0; j < MAP_HEIGHT; ++j) {
                    Unit* u = m_units[i][j];
                    if (u != nullptr) {
                        f(u);
                    }
                }
            }
        }

        bool Map::CheckPos(int x, int y)
        {
            return x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT ? false : true;
        }

        Point Map::TransPos(int x, int y)
        {
            return Point(x / AREA_WIDTH, y / AREA_HEIGHT);
        }

        short Map::GetBlockByPoint(const Point& pos)
        {
            auto it = m_pointBlock.find(std::make_tuple(pos.x, pos.y));
            if (it != m_pointBlock.end()) {
                return it->second;
            }
            return -1;
        }

        Point Map::GetRandomPoint()
        {
            int x = 0;
            int y = 0;
//             int block  = framework.random().GenRandomNum(1, BLOCK_SIZE + 1);

            int totalCount = 0;
            for (int i = 1; i < (int)m_blockPoints.size(); ++i) {
                totalCount += m_blockPoints[i].size();
            }

            int rand = framework.random().GenRandomNum(1, totalCount + 1);
            int block = 0;
            int temp = 0;
            for (int i = 1; i < (int)m_blockPoints.size(); ++i) {
                temp += m_blockPoints[i].size();
                if (temp >=  rand) {
                    block = i;
                    break;
                }
            }

            if (block >= 0 && block < (int)m_blockPoints.size()) {
                auto& blockPoints = m_blockPoints[block];
                int r = framework.random().GenRandomNum(blockPoints.size());
                int index = 0;
                for (auto it = blockPoints.begin(); it != blockPoints.end(); ++it) {
                    if (r == index++) {
                        x = it->x;
                        y = it->y;
                        break;
                    }
                }
            }
            return Point(x, y);
        }

        Point Map::GetRandomPoint(const Point& bornPos, const Point& nowPos, int range)
        {
            Point bornP = TransPos(bornPos);
            Point nowP = TransPos(nowPos);
            PosLimit limit = GetAreaPosLimit(bornP, range);
            vector<Point> pList;
            for (int i = limit.start.x; i <= limit.end.x; ++i) {
                for (int j = limit.start.y; j <= limit.end.y; ++j) {
                    if (i == nowP.x && j == nowP.y) {
                        continue;
                    }
                    Area& area = m_areas[i][j];
                    for (int i = area.pA().x; i <= area.pB().x; ++i) {
                        for (int j = area.pA().y; j <= area.pB().y; ++j) {
                            if (CheckPos(i, j) && m_walkable[i][j]) {
                                pList.emplace_back(i, j);
                            }
                        }
                    }
                }
            }

            if (!pList.empty()) {
                int index = framework.random().GenRandomNum(pList.size());
                return pList[index];
            }
            return nowPos;
        }

        bool Map::RandomMoveInWholeMap(Unit* unit)
        {
            int loop = 0;
            while (true) {
                if (++loop > 15) {
                    LOG_DEBUG("Map::RandomMoveInWholeMap failed loop>15, type=%d\n", (int)unit->type());
                    break;
                }
                Point pos = GetRandomPoint();
                if (MoveUnit(unit, pos.x, pos.y)) {
                    return true;
                }
            }
            return false;
        }

        PosLimit Map::GetAreaPosLimit(const Point& pos, int range)
        {
            PosLimit limit;

            if (pos.x - range < 0) {
                limit.start.x = 0;
                limit.end.x = range;
            } else if (pos.x + range > AREA_MAX_X) {
                limit.start.x = AREA_MAX_X - range;
                limit.end.x = AREA_MAX_X;
            } else {
                limit.start.x = pos.x - range;
                limit.end.x = pos.x + range;
            }

            if (pos.y - range < 0) {
                limit.start.y = 0;
                limit.end.y = range;
            } else if (pos.y + range > AREA_MAX_Y) {
                limit.start.y = AREA_MAX_Y - range;
                limit.end.y = AREA_MAX_Y;
            } else {
                limit.start.y = pos.y - range;
                limit.end.y = pos.y + range;
            }

            return limit;
        }

        //获得一个中心的范围位置
        PosLimit Map::GetCenterPosLimit(const Point& pos, const tpl::MapUnitTpl* tpl, int range)
        {
            PosLimit limit;

            int centerX = (int)(pos.x + tpl->cellSizeX * 0.5f);
            int centerY = (int)(pos.y + tpl->cellSizeY * 0.5f);

            int rangeGridOffsetX = (int) (range*0.5f);
            int rangeGridOffsetY = (int) (range*0.5f);

            int startX = (int) (centerX - rangeGridOffsetX);
            int startY = (int) (centerY - rangeGridOffsetY);

            int endX = (int) (centerX + rangeGridOffsetX);
            int endY = (int) (centerY + rangeGridOffsetY);

            LOG_DEBUG("centerY:%d ---- rangeGridOffsetY:%d ----- endY %d maxY : %d ", 
                centerY, rangeGridOffsetY, endY, AREA_MAX_Y);

            limit.start.x = startX < 0 ? 0 : startX;
            limit.start.y = startY < 0 ? 0: startY;
            limit.end.x = endX > MAP_WIDTH ? MAP_WIDTH : endX;
            limit.end.y = endY > MAP_HEIGHT ? MAP_HEIGHT : endY;

            return limit;
        }


        void Map::OnUnitAdd(Unit* unit)
        {
            if (unit) {
                Point p = TransPos(unit->pos());
                Area& area = m_areas[p.x][p.y];
                area.AddUnit(unit);

                evt_unit.Trigger(unit,  MapUnitEvent::ADD);

//                 for (auto it = area.viewers().begin(); it != area.viewers().end(); ++it) {
//                     Agent* agent = *it;
//                     agent->SendMapUnitUpdate(unit);
//                     //cout << "OnUnitAdd notice viewer = " << agent->viewPoint().x << " " << agent->viewPoint().y << " unit " << unit->pos().x << " " << unit->pos().y << endl;
//                 }
            }
        }

        void Map::OnUnitRemove(Unit* unit)
        {
            if (unit) {
                Point p = TransPos(unit->pos());
                Area& area = m_areas[p.x][p.y];
                area.RemoveUnit(unit);

                evt_unit.Trigger(unit,  MapUnitEvent::REMOVE);

//                 for (auto it = area.viewers().begin(); it != area.viewers().end(); ++it) {
//                     Agent* agent = *it;
//                     agent->SendMapUnitRemove(unit->id());
//                     //cout << "OnUnitRemove notice viewer = " << agent->viewPoint().x << " " << agent->viewPoint().y << " unit " << unit->pos().x << " " << unit->pos().y << endl;
//                 }
            }
        }

        void Map::OnUnitUpdate(Unit* unit)
        {
            if (unit) {
//                 Point p = TransPos(unit->pos());
//                 Area& area = m_areas[p.x][p.y];

                evt_unit.Trigger(unit,  MapUnitEvent::UPDATE);

//                 for (auto it = area.viewers().begin(); it != area.viewers().end(); ++it) {
//                     Agent* agent = *it;
//                     agent->SendMapUnitUpdate(unit);
// //                     cout << "OnUnitUpdate notice viewer = " << agent->nickname() << " " << agent->viewPoint().x << " " << agent->viewPoint().y << " unit " << unit->pos().x << " " << unit->pos().y << endl;
//                 }
            }
        }

        void Map::OnTroopAdd(Troop* tp)
        {
//             printf("-----------------Map::OnTroopAdd fX=%d, fY=%d, tX=%d, tY=%d\n", tp->from().x, tp->from().y, tp->to().x, tp->to().y);
            if (tp) {
                evt_troop.Trigger(tp, MapTroopEvent::ADD);
//                 set<Agent*> vs;
                if (tp->IsReach()) {
                    Point p = TransPos(tp->to());
                    Area& area = m_areas[p.x][p.y];
                    area.AddTroop(tp);
                } else {
                    for (int i = 0; i <= AREA_MAX_X; ++i) {
                        for (int j = 0; j <= AREA_MAX_Y; ++j) {
                            Area& area = m_areas[i][j];
                            if (area.IsCross(tp->from(), tp->to())) {
                                area.AddTroop(tp);
//                             printf("Map::OnTroopAdd area.IsCross fX=%d, fY=%d, tX=%d, tY=%d\n", area.pA().x, area.pA().y, area.pB().x, area.pB().y);
//                             const list<Agent*>& viewers = area.viewers();
//                             for (auto it = viewers.begin(); it != viewers.end(); ++it) {
//                                 vs.emplace(*it);
//                             }
                            }
                        }
                    }
                }
//                 // send to vs （通知正在看地图的）
//                 for (auto it = vs.begin(); it != vs.end(); ++it) {
//                     (*it)->SendMapTroopUpdate(tp);
//                 }

//                 BroadMapTroopUpdateEx(tp, vs);
            }
        }

        void Map::OnTroopUpdate(Troop* tp, bool resetCross)
        {
            // cout << "Map::OnTroopUpdate" << endl;
            if (tp) {
                if (resetCross) {
                    if (tp->IsReach()) {
                        for (int i = 0; i <= AREA_MAX_X; ++i) {
                            for (int j = 0; j <= AREA_MAX_Y; ++j) {
                                Area& area = m_areas[i][j];
                                if (area.IsCross(tp->from(), tp->to())) {
                                    area.RemoveTroop(tp);
                                }
                            }
                        }

                        Point p = TransPos(tp->to());
                        Area& area = m_areas[p.x][p.y];
                        area.AddTroop(tp);
                    } else {
                        for (int i = 0; i <= AREA_MAX_X; ++i) {
                            for (int j = 0; j <= AREA_MAX_Y; ++j) {
                                Area& area = m_areas[i][j];
                                if (area.IsCross(tp->from(), tp->to())) {
                                    area.AddTroop(tp);
                                }
                            }
                        }
                    }
                }

                evt_troop.Trigger(tp, MapTroopEvent::UPDATE);
            }
        }

        void Map::OnTroopRemove(Troop* tp)
        {
            if (tp) {
                m_troops.erase(tp->id());
                m_troopsRemoved.emplace(tp->id(), tp);

                evt_troop.Trigger(tp, MapTroopEvent::REMOVE);
                for (int i = 0; i <= AREA_MAX_X; ++i) {
                    for (int j = 0; j <= AREA_MAX_Y; ++j) {
                        Area& area = m_areas[i][j];
                        area.RemoveTroop(tp);
                    }
                }
//                 BroadMapTroopRemove(tp->id());
            }
        }

        void Map::OnViewerAdd(Agent* agent, const Point& pos, int range)
        {
            //cout << "OnViewerAdd" << endl;
            if (range < 0) {
                return;
            }
            range = range > MAX_RANGE ? MAX_RANGE : range;

            Point p = TransPos(pos);
            PosLimit limit = GetAreaPosLimit(p, range);
            //cout << p.x << " " << p.y << endl;
            //cout << limit.start.x << " " << limit.start.y << " " << limit.end.x << " " << limit.end.y << " " << endl;

            std::vector<Unit*> result;
            std::vector<Troop*> result2;

            for (int i = limit.start.x; i <= limit.end.x; ++i) {
                for (int j = limit.start.y; j <= limit.end.y; ++j) {
                    Area& area = m_areas[i][j];
                    area.AddViewer(agent);
                    const std::unordered_map<int, Unit*>& units = area.units();
                    for (auto it = units.begin(); it != units.end(); ++it) {
                        result.push_back(it->second);
                    }
                    //cout << "AddViewer " << i << " " << j << endl;

                    const std::unordered_map<int, Troop*>& tps = area.troops();
                    for (auto it  = tps.begin(); it != tps.end(); ++it) {
                        Troop* tp = it->second;
                        //printf("id=%d, uid=%ld, fX=%d, fY=%d, tX=%d, tY=%d\n", tp->id(), tp->uid(), tp->from().x, tp->from().y, tp->to().x, tp->to().y);
                        result2.push_back(tp);
                    }
                }
            }
            agent->proxy()->SendMapUnitUpdate(result, std::vector<int>(),  true);
            agent->proxy()->SendMapTroopUpdate(result2, true);
        }

        void Map::OnViewerRemove(Agent* agent, const Point& pos, int range)
        {
            if (range < 0) {
                return;
            }
            range = range > MAX_RANGE ? MAX_RANGE : range;

            Point p = TransPos(pos);
            PosLimit limit = GetAreaPosLimit(p, range);
            for (int i = limit.start.x; i <= limit.end.x; ++i) {
                for (int j = limit.start.y; j <= limit.end.y; ++j) {
                    m_areas[i][j].RemoveViewer(agent);
                    //cout << "RemoveViewer " << i << " " << j << endl;
                }
            }
        }

        // 根据玩家城池的坐标来寻找周围的资源
        void Map::OnSearchMap(Agent* agent, const Point& CastlePos, int type, int level, int range)
        {
            if (!CheckPos(CastlePos))
            {
                return;
            }

            // 得到城池所在的area
            Point cp = TransPos(CastlePos);
            // 得到索搜范围
            PosLimit lmt = GetAreaPosLimit(cp, g_tploader->configure().searchScope);
            // 记录搜索时间戳
            int now = g_dispatcher->GetTimestampCache();
            // 如果上次搜索时间超过了60, 并且连续搜索次数大于5次，重置
            if (now - agent->GetMapSearchTimeStamp() >= 60 || agent->GetMapSearchTime() > 5 || agent->GetMapSearchType() != type || agent->GetMapSearchLevel() != level)
            {
                agent->ReSetMapSearch();
            }
            agent->SetMapSearchTimeStamp(now);
            // cout << "###MapSearch begin" << endl;
            int tx = -1;
            int ty = -1;
            int temp = 0;
            for (int x = lmt.start.x; x <= lmt.end.x; ++x) {
                for (int y = lmt.start.y; y <= lmt.end.y; ++y) {                    
                        Area& area = m_areas[x][y];
                        const std::unordered_map<int, Unit*>& units = area.units();

                        for (auto it = units.begin(); it != units.end(); ++it) {
                            // result.push_back(it->second);
                            // cout << "unit.x, unit.y, unit.type, unit.level   " << it->second->x() << "," <<  it->second->y() << "," << (int)it->second->type() << "," << it->second->tpl().level << endl;
                            if ((int)it->second->type() == type && it->second->tpl().level == level)
                            {

                                // cout << "Is Search ### unit.x, unit.y, unit.type, unit.level   " << it->second->x() << "," <<  it->second->y() << "," << (int)it->second->type() << "," << it->second->tpl().level << endl;
                                int dis = get_distance(CastlePos,{it->second->x(), it->second->y()});
                                // cout << "Is Search ### dis, mapSearchDis, mapSearchTime" << dis << "," << agent->GetMapSearchDis() << "," << agent->GetMapSearchTime() << endl;
                                if (temp == 0)
                                {
                                    tx = it->second->x();
                                    ty = it->second->y();
                                    temp = dis;
                                }
                                else if (dis <= temp && dis > agent->GetMapSearchDis())
                                {
                                    temp = dis;
                                    tx = it->second->x();
                                    ty = it->second->y();
                                }
                            }
                        }
                }
            }
            // 更新索搜信息
            agent->SetMapSearchDis(temp);
            agent->SetMapSearchTime(agent->GetMapSearchTime() + 1);
            agent->SetMapSearchType(type);
            agent->SetMapSearchLevel(level);
            if (tx == -1 || ty == -1)
            {
                agent->proxy()->SendNoticeMessage(ErrorCode::CANNOT_SEARCH_AIM, 1);
            } 
            else 
            {
                // cout << "#######SendMapSearchResponse" << tx << "," << ty << endl;
                agent->proxy()->SendMapSearchResponse(tx, ty);
            }
            // cout << "###MapSearch end" << endl;
        }

        bool Map::OnViewerMove(Agent* agent, const Point& oldPos, const Point& newPos, int oldRange, int newRange)
        {
            //cout << "Map::OnViewerMove" << endl;
            if (!CheckPos(oldPos) || !CheckPos(newPos) || oldPos == newPos || oldRange < 0 || newRange < 0) {
                return false;
            }

            Point op = TransPos(oldPos);
            Point np = TransPos(newPos);

            if (op == np) {
                return false;
            }

            oldRange = oldRange > MAX_RANGE ? MAX_RANGE : oldRange;
            newRange = newRange > MAX_RANGE ? MAX_RANGE : newRange;

            PosLimit olmt = GetAreaPosLimit(op, oldRange);
            PosLimit nlmt = GetAreaPosLimit(np, newRange);

            std::vector<Unit*> result;
            std::vector<int> removelist;
            std::vector<Troop*> result2;
            for (int x = nlmt.start.x; x <= nlmt.end.x; ++x) {
                for (int y = nlmt.start.y; y <= nlmt.end.y; ++y) {
                    if (!is_in_rect(Point(x, y), olmt.start, olmt.end)) {
                        Area& area = m_areas[x][y];
                        // area.Debug();
                        m_areas[x][y].AddViewer(agent);
                        const std::unordered_map<int, Unit*>& units = area.units();
                        for (auto it = units.begin(); it != units.end(); ++it) {
                            result.push_back(it->second);
                        }
                        const std::unordered_map<int, Troop*>& tps = area.troops();
                        for (auto it = tps.begin(); it != tps.end(); ++it) {
                            result2.push_back(it->second);
                        }
                        //cout << "AddViewer By Move " << x << " " << y << endl;
                    }
                }
            }
            /*cout << "addArea End" << endl;*/
            agent->proxy()->SendMapUnitUpdate(result, removelist,  true);
            agent->proxy()->SendMapTroopUpdate(result2, true);
            return true;
        }

        std::vector< Unit* > Map::GetUnitsByPos(int x, int y, int range)
        {
            std::vector<Unit*> result;
            if (!CheckPos(x, y) || range < 0) {
                return result;
            }

            range = range > MAX_RANGE ? MAX_RANGE : range;

            Point p = TransPos(x, y);
            PosLimit limit = GetAreaPosLimit(p, range);
            for (int i = limit.start.x; i <= limit.end.x; ++i) {
                for (int j = limit.start.y; j <= limit.end.y; ++j) {
                    const std::unordered_map<int, Unit*>& units = m_areas[i][j].units();
                    for (auto it = units.begin(); it != units.end(); ++it) {
                        result.push_back(it->second);
                    }
                }
            }
            return result;
        }

        void Map::ReadMapData()
        {
            for (int i = 0; i < MAP_WIDTH; ++i) {
                for (int j = 0; j < MAP_HEIGHT; ++j) {
                    m_walkable[i][j] = true;
                }
            }

            for (int i = 0; i < MAP_WIDTH; ++i) {
                for (int j = 0; j < MAP_HEIGHT; ++j) {
                    m_trees[i][j] = Point(-1, -1);
                }
            }

            int usedCount = 0;
            m_blockPoints.resize(BLOCK_SIZE + 1, std::set<Point>());
            //读取地图数据
            {
                //每种城市类型对应的RGB的green值
                int rgbGreenPerCityType[] = {10, 50, 100, 0};
                //每个州RGB的red值
                int rgbRedPerBlock[] =  {0,        120,    180,     140,   80,    220,   255,     20,      160,    200,    40,      100,    60};
                string worldMapBmp = framework.resource_dir() + "/world_map.bmp";
                bitmap_image image(worldMapBmp);
                if (!image) {
                    LOG_ERROR("Error - Failed to open: world_map.bmp\n");
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

                auto getBlockId = [&](int red) {
                    for (int i = 0; i < BLOCK_SIZE; ++i) {
                        if (rgbRedPerBlock[i] == red) {
                            return i + 1;
                        }
                    }
                    return -1;
                };

                auto markCityPlace = [&](int x, int y, int cityType) {
                    if (const MapUnitTpl* tpl_unit = tpl::m_tploader->FindMapUnit((model::MapUnitType)cityType, 1)) {
                        //标志为不可行走
                        for (int i = x; i < x + tpl_unit->cellSizeX; ++i) {
                            for (int j = y; j < y + tpl_unit->cellSizeY; ++j) {
                                this->m_walkable[i][j] = false;
                                ++usedCount;
                            }
                        }
                    } else {
                        LOG_ERROR("Error - markCityPlace  cityType%d\n", cityType);
                    }
                };

//                 // 保存unwalkable的坐标
//                 string dir = framework.resource_dir() + "/unwalkable_map.txt";
//                 ofstream file(dir.c_str());
//                 for (std::size_t x = 0; x < width; ++x) {
//                     for (std::size_t y = 0; y < height; ++y) {
//                         image.get_pixel(x,y,red,green,blue);
//                         int pointX = x;
//                         int pointY = MAP_HEIGHT - y - 1;
//                         //标志阻挡区域
//                         if (std::make_tuple(red, green, blue) == std::make_tuple(255, 255, 0)) {
//                             //m_walkable[pointX][pointY] = false;
//                             file <<  pointX << ',' <<  pointY <<  '\n';
//                         }
//                     }
//                 }
//                 file.close();

                for (std::size_t y = 0; y < height; ++y) {
                    for (std::size_t x = 0; x < width; ++x) {
                        image.get_pixel(x, y, red, green, blue);
                        int pointX = x;
                        int pointY = MAP_HEIGHT - y - 1;
                        //标志阻挡区域
                        if (std::make_tuple(red, green, blue) == std::make_tuple(255, 255, 0)) {
                            m_walkable[pointX][pointY] = false;
                            //m_invalidPoint[pointX][pointY] = true;
                            continue;
                        }

                        //标志城市所在位置不可走
                        int blockId = getBlockId(red);
                        if (blockId > 0) {
                            m_pointBlock.insert(std::make_pair(std::make_tuple(pointX, pointY), blockId));
                            if (green != 255) {
                                int cityType = getCityType(green);
                                if (cityType > 0) {
                                    markCityPlace(pointX, pointY, cityType);
                                } else {
                                    LOG_ERROR("Error - cityType %d\n -x%d -y%d", cityType, x, y);
                                }
                            }
                        } else {
                            LOG_ERROR("Error - blockId %d\n", blockId);
                        }
                    }
                }

                for (std::size_t y = 0; y < height; ++y) {
                    for (std::size_t x = 0; x < width; ++x) {
                        image.get_pixel(x, y, red, green, blue);
                        int blockId = getBlockId(red);
                        if (blockId > 0) {
                            int pointX = x;
                            int pointY = MAP_HEIGHT - y - 1;
                            //存储每个坐标对应的区
                            m_pointBlock.insert(std::make_pair(std::make_tuple(pointX, pointY), blockId));

                            //存储可用的坐标
                            if (green == 255 && blue == 255) {
                                Point point(pointX, pointY);
                                if (m_walkable[pointX][pointY]) {
                                    m_blockPoints[blockId].insert(point);
                                }
                            }
                        } else {
                            LOG_ERROR("Error - blockId %d\n", blockId);
                        }
                    }
                }
            }
        }

        void Map::ViewerLeave(Agent* agent)
        {
            if (agent->isViewing()) {
                OnViewerRemove(agent, agent->viewPoint());
                agent->SetViewPoint(-1, -1);
            }
        }

        bool Map::InResArea(ResAreaType areaType, int x, int y) {
            model::ResAreaType type = model::ResAreaType::FIRST;
            if ( (520 <= x && x <= 680) && (520 <= y && y <= 680)) {
                type = model::ResAreaType::FIFTH;
            } else if ( ((390 <= x &&  x <=810)) && ((390 <= y &&  y <=810)) ) {
                type = model::ResAreaType::FOURTH;
            } else if ( ((260 <= x &&  x <=940)) && ((260 <= y &&  y <=940)) ) {
                type = model::ResAreaType::THIRD;
            } else if ( ((130 <= x && x <= 1070)) && ((130 <= y && y<=1070)) ) {
                type = model::ResAreaType::SECOND;
            } 

            if (type == areaType) {
                return true;
            }

            return false;
        }

        bool Map::InCapitalArea(const Point& p) {            
            if ((m_capital_poslimit.start.x <= p.x && p.x <= m_capital_poslimit.end.x) 
                && (m_capital_poslimit.start.y <= p.y && p.y <= m_capital_poslimit.end.x)) {
                return true;
            }
               
            return false;
        }

        // 野怪密度判断,如果密度超过就返回 true
        bool Map::MonsterDensity(const Point& p)
        {
            int xL = p.x - 5;
            int xR = p.x + 5;
            int yL = p.y - 5;
            int yR = p.y + 5;
            
            // 遍历以周围的点为中心的圈子的野怪密度
            for (int x = xL; x <= xR; x++) {
                for (int y = yL; y <= yR; y++) {
                    if (Unit* unit = m_units[x][y])
                    {
                        if (Monster* monster = unit->ToMonster())
                        {
                            int xL1 = monster->x() - 5;
                            int xR1 = monster->x() + 5;
                            int yL1 = monster->y() - 5;
                            int yR1 = monster->y() + 5;
                            int count = 0;
                            for(int x1 = xL1; x1 <= xR1; x1++)
                            {
                                for(int y1 = yL1; y1 <= yR1; y1++)
                                {
                                    if (Unit* subUnit = m_units[x1][y1])
                                    {
                                        if (subUnit->ToMonster())
                                        {
                                            count++;
                                        }
                                    }
                                }
                            }
                            if (count > 20)
                                return false;
                        }
                    }
                }
            }
            return true;
        }

        bool Map::CanPlace(const map::MapUnitTpl* tpl, int x, int y)
        {
            // if (tpl->type == MapUnitType::MONSTER)
            // {
            //     LOG_DEBUG("--------------- CanPlace ---------- Monster  ------------ %d --------- %d ", x, y);
            // }
            // spec case
            if (tpl->type == MapUnitType::CAPITAL || tpl->type == MapUnitType::CHOW || tpl->type == MapUnitType::PREFECTURE
                    || tpl->type == MapUnitType::COUNTY || tpl->type == MapUnitType::MONSTER) {
                return true;
            }

            if (!CheckPos(x, y) || m_units[x][y]) {
                return false;
            }
            // 如果超出大地图 或 有点不可走
            for (int i = x; i < x + tpl->cellSizeX; ++i) {
                for (int j = y; j < y + tpl->cellSizeY; ++j) {
                    if (!CheckPos(i, j) || !this->m_walkable[i][j]) {
                        return false;
                    }
                }
            }

            return true;
        }

        bool Map::CanPlaceExcludeTree(const tpl::MapUnitTpl* tpl, int x, int y, bool moveTree)
        {
            if (tpl ==  nullptr || tpl->ToTree()) {
                return false;
            }

            std::set<Unit*> trees;
            // 如果超出大地图 或 有点不可走
            for (int i = x; i < x + tpl->cellSizeX; ++i) {
                for (int j = y; j < y + tpl->cellSizeY; ++j) {
                    if (!CheckPos(i, j)/* || this->m_invalidPoint[i][j]*/) {
                        return false;
                    } else {
                        if (!this->m_walkable[i][j]) {
                            Unit* unit = FindUnit(m_trees[i][j]);
                            if (unit != nullptr && unit->IsTree()) {
                                trees.emplace(unit);
                            } else {
                                return false;
                            }
                        }
                    }
                }
            }

            if (moveTree) {
                bool allMove = true;
                for (auto tree :  trees) {
                    if (!RandomMoveInWholeMap(tree)) {
                        allMove = false;
                        break;
                    }
                }
                return allMove;
            }
            return true;
        }

        void Map::MarkPlace(const map::MapUnitTpl* tpl, int x, int y)
        {
            // spec case
            if (tpl->type == MapUnitType::CAPITAL || tpl->type == MapUnitType::CHOW || tpl->type == MapUnitType::PREFECTURE
                    || tpl->type == MapUnitType::COUNTY) {
                return;
            }

            // 把占用的点从可随机的点列表中去除
            for (int i = x; i < x + tpl->cellSizeX; ++i) {
                for (int j = y; j < y + tpl->cellSizeY; ++j) {
                    this->m_walkable[i][j] = false;

                    int id = GetBlockByPoint(Point(i, j));
                    if (id >= 0) {
                        auto& block = m_blockPoints[id];
                        block.erase(Point(i, j));
                    }
                }
            }
        }

        void Map::UnmarkPlace(const map::MapUnitTpl* tpl, int x, int y)
        {
            // 把占用的点加回可随机的点列表中
            for (int i = x; i < x + tpl->cellSizeX; ++i) {
                for (int j = y; j < y + tpl->cellSizeY; ++j) {
                    this->m_walkable[i][j] = true;

                    int id = GetBlockByPoint(Point(i, j));
                    if (id >= 0) {
                        auto& block = m_blockPoints[id];
                        block.insert(Point(i, j));
                    }
                }
            }
        }

        bool Map::RemoveUnit(Unit* unit)
        {
            if (unit) {
                OnUnitRemove(unit);
                UnmarkPlace(&unit->tpl(), unit->x(), unit->y());
                m_units[unit->x()][unit->y()] = nullptr;
                if (unit->ToCampFixed())
                {
                    m_campFixed.remove(unit);
                    // m_campFixed.erase(remove(m_campFixed.begin(),　m_campFixed.end(), unit),m_campFixed.end());
                }
//                 delete unit;
                unit->SetDelete();
                m_unitsToDelete.push_back(unit);
                return true;
            }

            return false;
        }

        bool Map::MoveUnit(Unit* unit, int toX, int toY)
        {
            if (!CanPlace(&unit->tpl(), toX, toY)) {
                return false;
            }

            if (unit->IsTree()) {
                OnTreeMove(unit);
            }

            OnUnitRemove(unit);
            UnmarkPlace(&unit->tpl(), unit->x(), unit->y());
            m_units[unit->x()][unit->y()] = nullptr;

            MarkPlace(&unit->tpl(), toX, toY);
            unit->SetPosition(toX, toY);
            m_units[toX][toY] = unit;
            OnUnitAdd(unit);

            if (unit->IsTree()) {
                SetTreePos(unit);
            }

            return true;
        }

        void Map::OnTreeMove(Unit* unit)
        {
            if (unit) {
                for (int i = unit->x(); i < unit->x() + unit->tpl().cellSizeX; ++i) {
                    for (int j = unit->y(); j < unit->y() + unit->tpl().cellSizeY; ++j) {
                        m_trees[i][j] = Point(-1, -1);
                    }
                }
            }
        }

        void Map::SetTreePos(Unit* unit)
        {
            if (unit) {
                for (int i = unit->x(); i < unit->x() + unit->tpl().cellSizeX; ++i) {
                    for (int j = unit->y(); j < unit->y() + unit->tpl().cellSizeY; ++j) {
                        m_trees[i][j] = unit->pos();
                    }
                }
            }
        }

        bool Map::UpdateUnit(Unit* unit)
        {
            //LOG_DEBUG("--------- Map::UpdateUnit ------- ");
            if (!unit) {
                return false;
            }
            switch (unit->type()) {
                case model::MapUnitType::FARM_FOOD:
                case model::MapUnitType::FARM_WOOD:
                case model::MapUnitType::MINE_IRON:
                case model::MapUnitType::MINE_STONE: {
                    //unit->Init();
                    return UpdateResource(unit);
                }
                break;
                case model::MapUnitType::MONSTER: {
                    unit->Init();
                    return UpdateMonster(unit);
                }
                break;
                default:
                    break;
            }
            return false;
        }

        bool Map::UpdateResource(Unit* unit)
        {
            ResNode* res = unit->ToResNode();
            if (res->troopId() != 0) {
                return false;
            }
            res->Reset();
            unit->UpdateRefreshTime();

            const Point& nowPos = res->pos();
            Point newPos = GetRandomPoint(res->bornPoint(), nowPos, 2);
            if (newPos == nowPos) {
                unit->NoticeUnitUpdate();
                LOG_DEBUG("Map::UpdateResource same point x=%d, y=%d\n", newPos.x, newPos.y);
            } else {
                if (MoveUnit(unit, newPos.x, newPos.y)) {
                    //LOG_DEBUG("Map::UpdateResource new point x=%d, y=%d\n", newPos.x, newPos.y);
                    return true;
                } else {
                    unit->NoticeUnitUpdate();
                    LOG_DEBUG("Map::UpdateResource same point x=%d, y=%d\n", newPos.x, newPos.y);
                }
            }

            LOG_ERROR("Map::UpdateResource failed point born.x=%d, born.y=%d, now.x=%d, now.y=%d\n", res->bornPoint().x, res->bornPoint().y, nowPos.x, nowPos.y);
            return false;
        }

        bool Map::UpdateMonster(Unit* unit)
        {
            return true;

            if (!unit || unit->type() != model::MapUnitType::MONSTER) {
                return false;
            }

            Monster* monster = unit->ToMonster();
            unit->UpdateRefreshTime();
            const Point& nowPos = unit->pos();
            int block = GetBlockByPoint(Point(nowPos.x, nowPos.y));

            int level = 0;
            int midLevel = m_castleLevelAvg / 2;
            // 1-3 2-4 3-5 4-6 5-7 6-8
            int r = framework.random().GenRandomNum(1, 10000);
            if ((block == 0 && r <= 9000)
                    || (block == 1 && r <= 7000)
                    || (block == 2 && r <= 5000)
                    || (block == 3 && r <= 3000)
                    || (block == 4 && r <= 2000)
                    || (block == 5 && r <= 1000)
               ) {
                level = framework.random().GenRandomNum(1, midLevel + 1);
            } else {
                level = framework.random().GenRandomNum(midLevel + 1, m_castleLevelAvg + 1);
            }
            if (const MapUnitTpl* tpl_unit = m_tploader->FindMapUnit(MapUnitType::MONSTER, level)) {
                unit->SetTpl(tpl_unit);
                Point newPos = GetRandomPoint(monster->bornPoint(), nowPos);
                if (newPos == nowPos) {
                    unit->NoticeUnitUpdate();
                    LOG_DEBUG("Map::UpdateMonster same point x=%d, y=%d\n", newPos.x, newPos.y);
                } else {
                    if (MoveUnit(unit, newPos.x, newPos.y)) {
                        // LOG_DEBUG("Map::UpdateMonster new point x=%d, y=%d\n", newPos.x, newPos.y);
                        return true;
                    } else {
                        unit->NoticeUnitUpdate();
                        LOG_DEBUG("Map::UpdateMonster same point x=%d, y=%d\n", newPos.x, newPos.y);
                    }
                }
            }
            LOG_ERROR("Map::UpdateMonster failed point born.x=%d, born.y=%d, now.x=%d, now.y=%d\n", monster->bornPoint().x, monster->bornPoint().y, nowPos.x, nowPos.y);
            return false;
        }

        int Map::GetUnitTplid(const Point& pos)
        {
            int tplid = 0;
            if (CheckPos(pos)) {
                if (Unit* unit = m_units[pos.x][pos.y]) {
                    tplid = unit->tpl().id;
                }
            } else {
                std::cout <<  "GetUnitTplid " << "x = " <<  pos.x <<  "y = " <<  pos.y <<  std::endl;
            }
            return tplid;
        }

        std::vector< Unit* > Map::FetchUnitsBy2Radius(const Point& pos, int rOut, int rIn)
        {
            vector<Unit*> units;
            if (!CheckPos(pos) || rIn < 0 || rOut < 0 || rIn > rOut) {
                return units;
            }

            Point pInMin = Point(pos.x - rIn, pos.y - rIn), pInMax = Point(pos.x + rIn, pos.y + rIn);
            Point pOutMin = Point(pos.x - rOut, pos.y - rOut), pOutMax = Point(pos.x + rOut, pos.y + rOut);
            for (int x = pOutMin.x; x <= pOutMax.x; ++x) {
                for (int y = pOutMin.y; y <= pOutMax.y; ++y) {
                    if (CheckPos(x, y) && !is_in_rect(Point(x, y), pInMin, pInMax)) {
                        if (Unit* unit = m_units[x][y]) {
                            units.push_back(unit);
                        }
                    }
                }
            }

            return units;
        }

        void Map::FindMatchUnit(std::vector<Unit*>& units, const std::function<bool(Unit*)>& matcher)
        {
            for (int i = 0; i < MAP_WIDTH; ++i) {
                for (int j = 0; j < MAP_HEIGHT; ++j) {
                    if (Unit* unit = m_units[i][j]) {
                        if (matcher(unit)) {
                            units.push_back(unit);
                        }
                    }
                }
            }
        }

        int Map::GetHighestMonsterLv()
        {
            int lv = 0;
            for (int i = 0; i < MAP_WIDTH; ++i) {
                for (int j = 0; j < MAP_HEIGHT; ++j) {
                    if (Unit* unit = m_units[i][j]) {
                        if (Monster* m = unit->ToMonster()) {
                            if (lv < m->tpl().level) {
                                lv = m->tpl().level;
                            }
                        }
                    }
                }
            }
            return lv;
        }

        Castle* Map::FindCastleByPos(const Point& pos)
        {
            Castle* castle = nullptr;
              auto unit = FindUnit(pos);
              if (unit) {
                  castle = unit->ToCastle();
              }
              return castle;
        }

        Unit* Map::FindNearestUnit(const Point& pos, const std::function<bool(Unit*)>& matcher)
        {
            Point start;
            for (int range = 1; range <= MAP_WIDTH; ++range) {
                int grid = 2 * range;
                start.x = pos.x - range;
                start.y = pos.y - range;
                for (int x = start.x; x < start.x + grid; ++x) {
                    if (!CheckPos(x, start.y) || !m_units[x][start.y]) {
                        continue;
                    }
                    if (matcher(m_units[x][start.y])) {
                        return m_units[x][start.y];
                    }
                }
                start.x = pos.x + range;
                for (int y = start.y; y < start.y + grid; ++y) {
                    if (!CheckPos(start.x, y) || !m_units[start.x][y]) {
                        continue;
                    }
                    if (matcher(m_units[start.x][y])) {
                        return m_units[start.x][y];
                    }
                }
                start.y = pos.y + range;
                for (int x = start.x; x > start.x - grid; --x) {
                    if (!CheckPos(x, start.y) || !m_units[x][start.y]) {
                        continue;
                    }
                    if (matcher(m_units[x][start.y])) {
                        return m_units[x][start.y];
                    }
                }
                start.x = pos.x - range;
                for (int y = start.y; y > start.y - grid; --y) {
                    if (!CheckPos(start.x, y) || !m_units[start.x][y]) {
                        continue;
                    }
                    if (matcher(m_units[start.x][y])) {
                        return m_units[start.x][y];
                    }
                }
            }

            return nullptr;
        }

        Unit* Map::FindNearestUnit(const Point& pos, const int allianceId,  
            const std::function<bool(Unit*, int)>& matcher, bool skipSelf)
        {
            Point start;
            for (int range = 1; range <= MAP_WIDTH; ++range) {
                int grid = 2 * range;
                start.x = pos.x - range;
                start.y = pos.y - range;
                for (int x = start.x; x < start.x + grid; ++x) {
                    if (skipSelf && pos.x == x && pos.y == start.y ) {
                            continue;
                    }

                    if (!CheckPos(x, start.y) || !m_units[x][start.y]) {
                        continue;
                    }

                    if (matcher(m_units[x][start.y], allianceId)) {
                        return m_units[x][start.y];
                    }
                }
                start.x = pos.x + range;
                for (int y = start.y; y < start.y + grid; ++y) {
                    if (skipSelf && pos.x == start.x && pos.y == y) {
                        continue;
                    }

                    if (!CheckPos(start.x, y) || !m_units[start.x][y]) {
                        continue;
                    }
 
                    if (matcher(m_units[start.x][y], allianceId)) {
                        return m_units[start.x][y];
                    }
                }
                start.y = pos.y + range;
                for (int x = start.x; x > start.x - grid; --x) {
                    if (skipSelf && pos.x == x && pos.y == start.y) {
                            continue;
                    }

                    if (!CheckPos(x, start.y) || !m_units[x][start.y]) {
                        continue;
                    }

                    if (matcher(m_units[x][start.y], allianceId)) {
                        return m_units[x][start.y];
                    }
                }
                start.x = pos.x - range;
                for (int y = start.y; y > start.y - grid; --y) {
                    if (skipSelf && pos.x == start.x && pos.y == y) {
                            continue;
                    }

                    if (!CheckPos(start.x, y) || !m_units[start.x][y]) {
                        continue;
                    }
                   
                    if (matcher(m_units[start.x][y], allianceId)) {
                        return m_units[start.x][y];
                    }
                }
            }

            return nullptr;
        }

        std::vector< Unit* > Map::GetUnitsInRect(const Point& start, const Point& end, MapUnitType type)
        {
            std::vector<Unit*> units;
            for (int i = start.x; i <= end.x; ++i) {
                for (int j = start.y; j <= end.y; ++j) {
                    if (!CheckPos(i, j)) {
                        continue;
                    }
                    if (Unit* unit = m_units[i][j]) {
                        if (unit->type() != type) {
                            continue;
                        }
                        units.push_back(unit);
                    }
                }
            }
            return units;
        }

        std::vector<Unit*> Map::GetUnitsInRect(const Point& start, const Point& end)
        {
            std::vector<Unit*> units;
            for (int i = start.x; i <= end.x; ++i) {
                for (int j = start.y; j <= end.y; ++j) {
                    if (!CheckPos(i, j)) {
                        continue;
                    }
                    if (Unit* unit = m_units[i][j]) {
                        units.push_back(unit);
                    }
                }
            }
            return units;
        }

        std::vector<Unit*> Map::GetUnitsInRect(const Point& start, const Point& end, const int allianceId, const std::function<bool(Unit*, int)>& matcher)
        {
            std::vector<Unit*> units;
            for (int i = start.x; i <= end.x; ++i) {
                for (int j = start.y; j <= end.y; ++j) {
                    if (!CheckPos(i, j)) {
                        continue;
                    }
                    if (Unit* unit = m_units[i][j]) {
                        if (matcher(unit, allianceId)) {
                            units.push_back(unit);
                        }
                    }
                }
            }
            return units;
        }


        void Map::VisitorJoin(Agent* agent)
        {
            if (agent) {
                m_agentsVisitor.emplace(agent->uid(), agent);
            }
        }
        void Map::VisitorLeave(int64_t uid)
        {
            auto it = m_agentsVisitor.find(uid);
            if (it != m_agentsVisitor.end()) {
                m_agentsVisitor.erase(it);
            }
        }


        void Map::OnAgentInitFinished(int64_t uid)
        {
            auto it = m_agentsWaitInit.find(uid);
            if (it != m_agentsWaitInit.end()) {
                m_agents.emplace(uid, it->second);
                m_agentsWaitInit.erase(uid);
            }
            //cout << "Map::OnAgentInitFinished az = " << m_agents.size() << " awiz = " << m_agentsWaitInit.size() << endl;
        }


        void Map::RebuildCastle(Castle* castle)
        {
            if (castle) {
                castle->agent().ImmediateReturnAllTroop();
            }

            while (castle) {
                Point pos = GetRandomPoint();

                if (MoveUnit(castle, pos.x, pos.y)) {
                    castle->OnRebuild();
                    return;
                }
            }
        }

        void Map::BroadBeKing()
        {
            //cout << "Map::BroadBeKing" << endl;
            if (const Agent* agt = king()) {
                string param = agt->nickname() + "," + to_string(m_localServiceId);
                m_mapProxy->BroadNoticeMessage(1903001, 2, param);
            }
        }

        CampTemp* Map::CreateCampTemp(int x, int y, Troop* troop)
        {
            if (CheckPos(x, y)) {
                if (const MapUnitTpl* tpl = m_tploader->FindMapUnit(model::MapUnitType::CAMP_TEMP, 1)) {
                    if (!troop || !CanPlaceExcludeTree(tpl, x, y, true)) {
                        //cout << "Map::CreateCamp fail x = " << x << " y = " << y << endl;
                        return nullptr;
                    }

                    if (!CanPlace(tpl, x, y)) {
                        return nullptr;
                    }

                    CampTemp* unit = new CampTemp(GenerateID(), tpl->ToCampTemp(), troop);
                    if (unit) {
                        MarkPlace(tpl, x, y);
                        m_units[x][y] = unit;
                        unit->SetPosition(x, y);
                        OnUnitAdd(unit);
                    }
                    return unit;
                }
            }
            return nullptr;
        }

        CampFixed* Map::CreateCampFixed(int x, int y, Troop* troop)
        {
            if (CheckPos(x, y)) {
                if (const MapUnitTpl* tpl = m_tploader->FindMapUnit(model::MapUnitType::CAMP_FIXED, 1)) {
                    if (!troop || !CanPlaceExcludeTree(tpl, x, y, true)) {
                        //cout << "Map::CreateCamp fail x = " << x << " y = " << y << endl;
                        return nullptr;
                    }

                    if (!CanPlace(tpl, x, y)) {
                        return nullptr;
                    }

                    CampFixed* unit = new CampFixed(GenerateID(), tpl->ToCampFixed(), troop);
                    if (unit) {
                        MarkPlace(tpl, x, y);
                        m_units[x][y] = unit;
                        m_campFixed.push_back(unit);
                        unit->SetPosition(x, y);
                        OnUnitAdd(unit);
                    }
                    return unit;
                }
            }
            return nullptr;
        }

        bool Map::OnTransport(Agent* agent, Agent* toAgent, model::MapTroopType type, const Point& from, const Point& to, base::DataTable& resourceParams)
        {
            if (auto troop = CreateTroop(*agent, type, from, to, 0)) {
                agent->OnMarch(troop,  false);
                troop->SetTransportAgent(toAgent);
                resourceParams.ForeachAll([&](const DataValue & k, const DataValue & v) {
                    int tplId = k.ToInteger();
                    int count = v.ToInteger();
                    troop->AddResource((model::ResourceType)tplId, count);
                    return false;
                });
                // 产生运输记录ID
                int32_t transportId1 = GenerateTransportRecordID();
                int32_t transportId2 = GenerateTransportRecordID();
                troop->SetTransportRecordId1(transportId1);
                troop->SetTransportRecordId2(transportId2);
                m_mapProxy->SendcsAddTransportRecord(troop, agent->uid(), agent->headId(), agent->nickname(), toAgent->uid(), 
                    toAgent->headId(), toAgent->nickname(), model::TransportArriveType::TRANSPORTING, troop->endTimestamp());
                return true;
            }
            return false;
        }

        bool Map::Teleport(Unit* unit, int x, int y)
        {
            if (!unit || !CheckPos(x, y)) {
                return false;
            }

            const tpl::MapUnitTpl* tpl = &unit->tpl();
            if (!CanPlaceExcludeTree(tpl, x, y, true)) {
                return false;
            }

            if (MoveUnit(unit, x, y)) {
                unit->OnTeleport();
                qo::g_dataService->AppendData(qo::g_dataService->CreateDbInfo(unit));
                return true;
            }
            return false;
        }

        void Map::OnPalaceWarStart()
        {
            m_mapProxy->SendcsPalaceWarStart();
            //cs返回国王清理的协议包之后会执行ChangeKing清理数据
        }

        Troop* Map::CreateTroop(Agent& agent, MapTroopType type, const Point& from, const Point& to, int teamId)
        {
            int id = GenerateTroopID();
            if (Troop* troop = TroopFactory::instance().CreateTroop(id, &agent, type, from, to, teamId)) {
                troop->March();
                m_troops.emplace(id, troop);
                OnTroopAdd(troop);

                if (agent.uid() > 0) {
                    troop->SetClean();
                    qo::g_dataService->AppendData(qo::g_dataService->CreateDbInfo(troop));
                }

                return troop;
            }
            return nullptr;
        }

        void Map::OnCastleLevelUp(Castle* castle, int level)
        {
            if (castle) {
                castle->OnLevelUp(level);
            }
        }

        void Map::OnCreateCastle(Castle* castle)
        {
            ++m_globalData.castleCnt;
            m_globalData.isDirty = true;
            m_mapProxy->SendcsMapGlobalData();
        }

        void Map::OnKillNpc(int count)
        {
            m_globalData.killNpcCnt += count;
            m_globalData.isDirty = true;
            m_mapProxy->SendcsMapGlobalData();
        }

        void Map::OnOccupyRes(ResNode* res)
        {
            if (res) {
                int count = 1;
                m_globalData.occupyResCnt += count;
                auto oldValue = m_globalData.occupyRes.Get(res->tpl().level);
                if (oldValue) {
                    count += oldValue->ToInteger();
                }
                m_globalData.occupyRes.Set(res->tpl().level, count);
                m_globalData.isDirty = true;
                m_mapProxy->SendcsMapGlobalData();
            }
        }

        void  Map::OnOccupyCity(FamousCity* city)
        {
            if (city) {
                auto type = city->tpl().type;
                if (type ==  model::MapUnitType::CAPITAL) {
                    ++m_globalData.occupyCapitalCnt;
                } else if (type ==  model::MapUnitType::CHOW) {
                    ++m_globalData.occupyChowCnt;
                } else if (type ==  model::MapUnitType::PREFECTURE) {
                    ++m_globalData.occupyPrefectureCnt;
                } else if (type ==  model::MapUnitType::COUNTY) {
                    ++m_globalData.occupyCountyCnt;
                }
                m_globalData.isDirty = true;
                m_mapProxy->SendcsMapGlobalData();
            }
        }

        bool Map::OnTotalPowerChange(int64_t totalPower)
        {
            if (totalPower > m_globalData.totalPower) {
                m_globalData.totalPower += totalPower;
                m_globalData.isDirty = true;
                m_mapProxy->SendcsMapGlobalData();
                return true;
            }
            return false;
        }

        void Map::OnGather(int count)
        {
            m_globalData.gatherCnt += count;
            m_globalData.isDirty = true;
            m_mapProxy->SendcsMapGlobalData();
        }

        void Map::OnAddHero(int heroId)
        {
            auto tpl = model::tpl::g_tploader->FindHero(heroId);
            if (tpl != nullptr) {
                int count = 1;
                auto oldValue = m_globalData.heroCnt.Get(tpl->quality);
                if (oldValue) {
                    count += oldValue->ToInteger();
                }
                m_globalData.heroCnt.Set(tpl->quality, count);
                m_globalData.isDirty = true;
                m_mapProxy->SendcsMapGlobalData();
            }
        }

        void Map::SaveMapGlobalData()
        {
            if (m_globalData.isDirty) {
                DataTable dtGlobal;
                dtGlobal.Set("castleCnt", globalData().castleCnt);
                dtGlobal.Set("killNpcCnt", globalData().killNpcCnt);
                dtGlobal.Set("occupyResCnt", globalData().occupyResCnt);
                dtGlobal.Set("occupyRes", globalData().occupyRes);
                dtGlobal.Set("occupyCapitalCnt", globalData().occupyCapitalCnt);
                dtGlobal.Set("occupyChowCnt", globalData().occupyChowCnt);
                dtGlobal.Set("occupyPrefectureCnt", globalData().occupyPrefectureCnt);
                dtGlobal.Set("occupyCountyCnt", globalData().occupyCountyCnt);

                dtGlobal.Set("totalPower", globalData().totalPower);
                dtGlobal.Set("gatherCnt", globalData().gatherCnt);
                dtGlobal.Set("heroCnt", globalData().heroCnt);

                dtGlobal.Set("monsterDist", globalData().monsterDist);

                string k = "ms_global";
                string v = dtGlobal.Serialize();
                m_runner.PushCommand(new CommandDictSave(k, v));
                m_globalData.isDirty = false;
            }
        }

        void Map::OnBattleFinish(const msgqueue::MsgPvpPlayerInfo& attacker, const msgqueue::MsgPvpPlayerInfo& defender, AttackType winner, MapTroopType troopType)
        {
            BattleInfo* info = new BattleInfo;
            info->id = GenerateFightID();
            info->red = TeamInfo(attacker.uid, attacker.nickname, attacker.allianceId, attacker.allianceNickname);
            info->blue = TeamInfo(defender.uid, defender.nickname, defender.allianceId, defender.allianceNickname);
            if (winner == model::AttackType::ATTACK) {
                info->winTeam = 1;
            } else {
                info->winTeam = 2;
            }

            info->battleType = 1;
            info->battleTime = g_dispatcher->GetTimestampCache();

            m_battleInfoList.emplace(info->id, info);
            m_alliance->OnBattleUpdate(info);

            qo::g_dataService->AppendData(qo::g_dataService->CreateDbInfo(info));
        }

        void Map::OnMonsterSiege(Castle* castle, int level)
        {
            if (!castle) {
                return;
            }
            //cout << "Map::OnMonsterSiege id = " << castle->id() << " level = " << level << endl;

            Point p = TransPos(castle->pos());
            Area& area = m_areas[p.x][p.y];
            for (auto it = area.viewers().begin(); it != area.viewers().end(); ++it) {
                if (Agent* agent = *it) {
                    // agent->SendMapMonsterSiege(castle->id(), level);
                }
            }
        }

        const std::list<FamousCity*>* Map::AllianceCity(int64_t allianceId)
        {
            auto it = m_allianceCity.find(allianceId);
            if (it != m_allianceCity.end()) {
                return &it->second;
            }
            return nullptr;
        }

        int Map::AllianceCityCnt(int64_t allianceId)
        {
            auto it = m_allianceCity.find(allianceId);
            if (it != m_allianceCity.end()) {
                return it->second.size();
            }
            return 0;
        }

    }
}







