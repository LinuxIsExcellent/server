#ifndef MAP_UNIT_H
#define MAP_UNIT_H
#include <model/metadata.h>
#include <model/dirtyflag.h>
#include "../tpl/templateloader.h"
#include "../tpl/mapunittpl.h"
#include "../mapMgr.h"
#include "../point.h"
#include <base/event.h>
#include <base/objectmaintainer.h>

namespace ms
{
    namespace map
    {
        using namespace tpl;

        class Map;
        class Capital;
        class Castle;
        class ResNode;
        class Monster;
        class CampTemp;
        class CampFixed;
        class FamousCity;
        class Catapult;
        class WorldBoss;

        class Unit : public model::Dirty
        {
        public:
            Unit(int id, const MapUnitTpl* tpl);

            virtual ~Unit();

            virtual std::string Serialize() {
                return "";
            }
            virtual bool Deserialize(std::string data) {
                return true;
            }
            virtual void FinishDeserialize() { }

            virtual Troop* troop() {
                return nullptr;
            }
            
            // 防守部队
            virtual ArmyList* DefArmyList() {
                return nullptr;
            }

            int id() const {
                return m_id;
            }

            virtual int64_t uid() const
            {
                return 0;
            }

            virtual int64_t allianceId() const {
                return 0;
            }

            const Point& pos() const {
                return m_pos;
            }

            int x() const {
                return m_pos.x;
            }
            int y() const {
                return m_pos.y;
            }
 
            model::ResAreaType GetResAreaType() const {
                model::ResAreaType type = model::ResAreaType::FIRST;
                if ( (520 <= m_pos.x && m_pos.x <= 680) && (520 <= m_pos.y && m_pos.y <= 680)) {
                    type = model::ResAreaType::FIFTH;
                } else if ( (390 <= m_pos.x &&  m_pos.x <=810) && (390 <= m_pos.y &&  m_pos.y <=810) ) {
                    type = model::ResAreaType::FOURTH;
                } else if ( (260 <= m_pos.x &&  m_pos.x <=940) && (260 <= m_pos.y &&  m_pos.y <=940) ) {
                    type = model::ResAreaType::THIRD;
                } else if ( (130 <= m_pos.x && m_pos.x <= 1070) && (130 <= m_pos.y && m_pos.y<=1070) ) {
                    type = model::ResAreaType::SECOND;
                } 

                return type;
            } 

            model::MapUnitType type() const {
                return m_tpl->type;
            }

            const MapUnitTpl& tpl() const {
                return *m_tpl;
            }

            int64_t refreshTime() const {
                return m_refreshTime;
            }

            void UpdateRefreshTime();

            void SetPosition(int x, int y) {
                m_pos.x = x;
                m_pos.y = y;
                SetDirty();
            }

            bool IsCapital() const {
                return type() == model::MapUnitType::CAPITAL;
            }
            bool IsChow() const {
                return type() == model::MapUnitType::CHOW;
            }
            bool IsPrefecture() const {
                return type() == model::MapUnitType::PREFECTURE;
            }
            bool IsCounty() const {
                return type() == model::MapUnitType::COUNTY;
            }

            bool IsCastle() const {
                return type() == model::MapUnitType::CASTLE;
            }
            bool IsCampTemp() const {
                return type() == model::MapUnitType::CAMP_TEMP;
            }
            bool IsCampFixed() const {
                return type() == model::MapUnitType::CAMP_FIXED;
            }

            bool IsFarmFood() const {
                return type() == model::MapUnitType::FARM_FOOD;
            }
            bool IsFarmWood() const {
                return type() == model::MapUnitType::FARM_WOOD;
            }
            bool IsMineIron() const {
                return type() == model::MapUnitType::MINE_IRON;
            }

            bool IsMineStone() const {
                return type() == model::MapUnitType::MINE_STONE;
            }

            bool IsMonster() const {
                return m_tpl->IsMonster();
            }

            bool IsResNode() const {
                return IsFarmFood() || IsFarmWood() || IsMineIron() || IsMineStone();
            }

            bool IsFamousCity() const {
                return IsChow() || IsPrefecture() || IsCounty() ||  IsCapital();
            }

            bool IsTree() const {
                return type() == model::MapUnitType::TREE;
            }

            bool IsCatapult() const {
                return type() == model::MapUnitType::CATAPULT;
            }

            bool IsWorldBoss() const {
                return type() == model::MapUnitType::WORLD_BOSS;
            }

            Capital* ToCapital();

            Castle* ToCastle();
            CampTemp* ToCampTemp();
            CampFixed* ToCampFixed();
            ResNode* ToResNode();
            Monster* ToMonster();
            FamousCity* ToFamousCity();

            Catapult* ToCatapult();
            WorldBoss* ToWorldBoss();
			
            //ResAreaType 资源带类型
            model::ResAreaType GetResArea(); 

            virtual void Init() = 0;
            virtual void Occupy(Troop* troop) {}

            virtual bool SwitchTroop() { return false; }
            //判断是否是最后一个部队
            virtual bool IsLastTroop() { return false; }

            //通知附近灯塔本单位更新
            void NoticeUnitUpdate();
            bool RefreshSelf();
            virtual bool RemoveSelf();
            bool MoveSelf(int x, int y);
            bool IsInRect(const Point& pos, const Point& start, const Point& end) const;

            bool IsDelete() {
                return m_delete;
            }

            void SetDelete() {
                m_delete = true;
            }

        public:
            //被行军
            virtual void OnTroopMarch(Troop* troop) {}
            //行军到达
            virtual void OnTroopReach(Troop* troop) {}
            //行军离开
            virtual void OnTroopLeave(Troop* troop) {}
            //行军返回
            virtual void OnTroopBack(Troop* troop) {}
            // 军队占领
            virtual void OnTroopOcuppy(Troop* troop) {}
            // 军队驻军
            virtual void OnTroopGarrison(Troop* troop) {}
            //迁移
            virtual void OnTeleport() {}

            virtual void SetTpl(const MapUnitTpl* tpl) {
                m_tpl = tpl;
                SetDirty();
            }

        protected:
            bool m_delete = false;
            int m_id = 0;
            Point m_pos;
            const MapUnitTpl* m_tpl = nullptr;
            int64_t m_refreshTime;
            base::ObjectMaintainer m_maintainer;
        };
    }
}


#endif // UNIT_H
