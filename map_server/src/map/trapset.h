#ifndef MAP_TRAPSET_H
#define MAP_TRAPSET_H 
#include <unordered_map>
#include <model/metadata.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include <model/tpl/hero.h>
#include <vector>
#include <memory>
#include "info/property.h"

namespace model
{
    namespace tpl
    {
        struct NArmyTpl;
    }
}

namespace ms
{
    namespace map
    {
    	using namespace model::tpl;

    	const static int kTrapAttrCount = 1;

  		//Trap 信息
        class TrapInfo
        {
        public:
            TrapInfo() {}
            TrapInfo(int armyType);
            virtual ~TrapInfo() {}

            void InitProp();

            int tplid() const {
                return m_tplid;
            }

            int type() const {
                return m_type;
            }

            int attack() const{
                return m_attack;
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

            void Add(model::ArmyState state, int count);
            bool Remove(model::ArmyState state, int count);
            void Set(model::ArmyState state, int count);
            bool Die(int count);
           
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
            std::unordered_map<int, int> m_states; //key : model::ArmyState, value : count

            int m_attack = 0;

            int m_attrBase[kTrapAttrCount][4] = {{0, 0, 0, 0}};
        };

        //陷阱集合
        class TrapSet
        {
        public:
            TrapSet() {}
            virtual ~TrapSet() {}

            void AddTrap(int armyType, model::ArmyState state, int count);
            void AddTrap(const TrapInfo& armyInfo);
            bool Die(int armyType, int count);
            bool Set(int armyType, model::ArmyState state, int count);

            const std::unordered_map<int, TrapInfo>& indieArmies() const {
                return m_indieArmies;
            }

            int size() const {
                return m_indieArmies.size();
            }

            TrapInfo* GetTrap(int armyType);

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
            std::unordered_map<int, TrapInfo> m_indieArmies; //key : armyType
        };
    }
}
#endif