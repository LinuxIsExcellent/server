#ifndef MAP_ARMYLIST_H
#define MAP_ARMYLIST_H
#include <unordered_map>
#include <model/metadata.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include <model/tpl/hero.h>
#include <vector>
#include <memory>
#include "info/property.h"
#include "heroinfo.h"

namespace base
{
    class DataTable;
}

namespace model
{
    namespace tpl
    {
        struct HeroTpl;
        struct ArmyTpl;
        struct NHeroTpl;
        struct NArmyTpl;
        struct SkillTpl;
    }
}

namespace ms
{
    namespace map
    {
        namespace tpl
        {
            struct NpcArmyTpl;
        }

        using namespace model::tpl;

        const static int kArmyAttrCount = 8;

        //队伍里单个兵种信息
        class ArmyInfo
        {
        public:
            ArmyInfo() {}
            ArmyInfo(int armyType, int armyLevel);
            virtual ~ArmyInfo() {}

            void InitProp();

            int tplid() const {
                return m_tplid;
            }

            int type() const {
                return m_type;
            }

            int level() const {
                return m_level;
            }

            int hp() const{
                return m_hp;
            }

            int attack() const{
                return m_attack;
            }

            int defense() const{
                return m_defense;
            }

            int speed() const{
                return m_speed;
            }

            int wallAttack() const {
                return m_wallAttack;
            }

            void SetProp(model::AttributeType type, int base, int pct, int ext);
            int GetProp(model::AttributeType type) const;

            int GetBaseAttr(model::AttributeType type, model::AttributeAdditionType addType) const;
            void SetBaseAttr(model::AttributeType type, int value, model::AttributeAdditionType addType);

            const std::unordered_map<int, int>& states() const {
                return m_states;
            }

            std::unordered_map<int, int>& states() {
                return m_states;
            }

            int count(model::ArmyState state) const {
                auto it = m_states.find((int)state);
                return it != m_states.end() ? it->second : 0;
            }

            bool IsTrap();
            void Add(model::ArmyState state, int count);
            bool Remove(model::ArmyState state, int count);
            void Set(model::ArmyState state, int count);
            bool Die(int count);
            bool Wound(int count);
            bool Recover(int count);
            //按照死亡百分比转为受伤士兵
            int DieToWound(float percent);
            int ToWound(float percent);
            //清除死亡状态的兵并返回数量
            int ClearDie();
            // 返回实际hurtHp
            int HurtHp(int hurtHp);

            int Power();

            void OnPropertyUpdate(const info::Property& property);

            //序列化 JSON
            void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const;
            //反序列化 JSON
            void Desrialize(rapidjson::GenericValue<rapidjson::UTF8< char >>& gv);

        protected:
            int GetIndex(model::AttributeType type) const;

        protected:
            const model::tpl::NArmyTpl* m_armyTpl = nullptr;
            int m_tplid = 0;
            int m_type = 0;
            int m_level = 0;
            std::unordered_map<int, int> m_states; //key : model::ArmyState, value : count

            int m_hp = 0, m_attack = 0, m_defense = 0, m_speed = 0, m_wallAttack = 0;
            int m_attrBase[kArmyAttrCount][4] = {{0, 0, 0, 0}};
        };

        // 兵
        class ArmySet
        {
        public:
            ArmySet() {}
            virtual ~ArmySet() {}

            void AddArmy(int armyType, int armyLevel, model::ArmyState state, int count);
            void AddArmy(const ArmyInfo& armyInfo);
            bool Die(int armyType, int count);
            bool Set(int armyType, model::ArmyState state, int count);

            const std::unordered_map<int, ArmyInfo>& indieArmies() const {
                return m_indieArmies;
            }

            int size() const {
                return m_indieArmies.size();
            }

             ArmyInfo* GetArmy(int armyType);

            int Total() const;

            // 清除正常状态以外的信息
            void ClearAllExceptNormal();

            void ClearAll();

            void OnPropertyUpdate(const info::Property& property);

            void InitProp();

        public:
            //放入DataTable
            // specify = 0 表示所有，specify = 1表示等级，specify=2表示数量
            void SetDataTable(base::DataTable& dt,  int specify = 0, bool isFalse = false) const;
            //序列化 JSON
            void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const;
            //反序列化 JSON
            void Desrialize(rapidjson::GenericValue<rapidjson::UTF8< char >>& gv);

        private:
            std::unordered_map<int, ArmyInfo> m_indieArmies; //key : armyType
        };

        class ArmyGroup
        {
        public:
            ArmyGroup() {}
            ArmyGroup(ms::map::tpl::NpcArmyTpl* npcArmyTpl);
            ArmyGroup(int armyType, int armyLevel, HeroInfo& heroInfo, int pos);

            const ArmyInfo& armyInfo() const {
                return m_armyInfo;
            }

           const HeroInfo& heroInfo() const {
                return m_heroInfo;
            }

            ArmyInfo& armyInfo() {
                return m_armyInfo;
            }

           HeroInfo& heroInfo() {
                return m_heroInfo;
            }

            int position() const {
                return m_position;
            };

            void setPosition(int position) {
                m_position = position;
            }

            void updateHeroInfo(const HeroInfo& heroInfo) {
                m_heroInfo = heroInfo;
            }

            int Power();

        protected:
            ArmyInfo   m_armyInfo;
            HeroInfo    m_heroInfo;
            int m_position = 0;
        };

        //行军队伍列表
        class ArmyList
        {
        public:
            ArmyList() {}
            virtual ~ArmyList() {}

            ArmyGroup* AddArmyGroup(ms::map::tpl::NpcArmyTpl* npcArmyTpl);
            ArmyGroup* AddArmyGroup(int armyType, int armyLevel, HeroInfo& heroInfo, int pos);
            void AddArmyGroup(const ArmyGroup& armyGroup);
            void RemoveArmy(int heroTplId, model::ArmyState state, int count);
            bool SetArmy(int heroTplId, model::ArmyState state, int count);
            bool Die(int heroTplId,int count);
            bool Wound(int heroTplId,int count);
            bool Recover(int heroTplId,int count);
            void Add(int heroTplId,int count);

            // 活着的部队数量
            int ActiveSize();
            
            const std::unordered_map<int, ArmyGroup>& armies() const {
                return m_armies;
            }

            std::unordered_map<int, ArmyGroup>& armies() {
                return m_armies;
            }

            int size() const {
                return m_armies.size();
            }

            int team() const {
                return m_team;
            }

            void SetTeam(int team) {
                m_team = team;
            }

            int troopId() {
                return m_troopId;
            }

            void setTroopId(int troopId) {
                m_troopId = troopId;
            }

            float addDefensePct() const {
                return m_addDefensePct;
            }

            void SetAddDefensePct(float addDefensePct)
            {
                m_addDefensePct = addDefensePct;
            }

            bool IsOut() {
                return m_troopId > 0;
            }

            ArmyGroup* GetArmyGroup(int heroTplId);
            ArmyGroup* GetArmyGroupByPos(int pos);

            int ArmyCount(model::ArmyState state) const;

            float GetBaseSpeed();

            // isFalse 是否开启伪装
            int GetArmyTotal(bool isFalse = false) const;   //获取状态为NORMAL的士兵总数
            int GetArmyTotalProbably(bool isFalse = false) const;
            int GetArmyWounded() const;
            int GetLoadTotal() const;
            int GetHurtTotal() const;
            bool IsAllDie() const;
            void Debug() const;
            void DieToWound(float percent = 1.0f);
            // 单挑结果：按正常百分比转换成伤兵
            void ToWound(float percent);
            void ClearAllWounded();
            void ClearAllDie();
            void ClearAllHurt();
            // 清除正常状态以外的信息
            void ClearAllExceptNormal();
            //清除所有兵力
            void ClearAll();
            // 把待恢复的兵变成正常兵
            void RecoverToNormal();
            
            void AssignPosition();

            int Power();

            void OnPropertyUpdate(const info::Property& property);

            void InitProp();

        public:
            //放入DataTable
            // specify = 0 表示所有，specify = 1表示武将，specify=2表示兵
            void SetDataTable(base::DataTable& dt,  int specify = 0, bool isFalse = false) const;
            //序列化 JSON
            void Serialize(rapidjson::Writer<rapidjson::StringBuffer>& writer) const;
            //反序列化 JSON
            void Desrialize(rapidjson::GenericValue<rapidjson::UTF8< char >>& gv);

        private:
            std::unordered_map<int, ArmyGroup> m_armies; //key : herotplid
            int m_team = 0;
            int m_troopId = 0;                              //行军ID
            float m_addDefensePct = 0.0f;
        };
    }
}
#endif // MAP_ARMYLIST_H
