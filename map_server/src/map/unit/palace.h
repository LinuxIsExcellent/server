#ifndef MAP_PALACE_H
#define MAP_PALACE_H
#include "unit.h"
#include "../info/palaceWar.h"
#include "capital.h"

namespace model
{
    namespace tpl
    {
        class ArmyTpl;
    }
}

namespace ms
{
    namespace map
    {
        namespace qo
        {

            class CommandSuccessiveKingsLoad;
        }


         // 都城,就是王城palace
        class Capital : public FamousCity
        {
        public:
            Capital(int id, const ms::map::tpl::MapUnitCapitalTpl* tpl, const Point& bornPoint);
            virtual ~Capital();

            virtual void Init() override;

            const ms::map::tpl::MapUnitCapitalTpl& tpl() const {
                return *m_tpl;
            }

            const ms::map::tpl::MapCityTpl& CityTpl() const {
                return *m_cityTpl;
            }
            
            /*
            const ArmyList* NpcAmryList() const {
                return m_ArmyList;
            }

            ArmyList* NpcAmryList() {
                return m_NpcArmyList;
            }
            */
           

            virtual ArmyList* DefArmyList() {
                if (m_armyLists.empty()) {   
                    return nullptr;
                }
                else
                {
                    return &m_armyLists.back();
                }
            }

            bool isNotStart() const {
                return m_state == model::PalaceState::NOSTART || m_state == model::PalaceState::PROTECTED || m_state == model::PalaceState::CHOOSE;
            }

            model::PalaceState state() const {
                return m_state;
            }

            int64_t occupierId() const {
                return m_occupierId;
            }

            const Agent* occupier() const {
                return m_occupier;
            }
            
            Agent* occupier() {
                return m_occupier;
            }

            const std::unordered_map<int64_t, Troop*>& defenders() const {
                return m_defenders;
            }

            const std::list<info::SuccessiveKing>& successiveKings() const {
                return m_successiveKings;
            }

            const std::list<info::PalaceWarRecord>& palaceWarRecords() const {
                return m_palaceWarRecords;
            }
            
            void SetState(model::PalaceState state) {
                m_state = state;
            }

            void SetOccupier(Agent* agent);

            int64_t startLeftTime() const;
            int64_t endLeftTime() const;
            //选择国王剩余时间
            int64_t chooseLeftTime() const;
            int64_t NpcRecoverLeftTime() const;
            int64_t cpRecoverLeftTime() const;

            //void AddDragonHp(int num);
            //void RemoveDragonHp(int num);
            //重置龙的ArmyList 用于战斗 但不重置血量
            //void ResetDragonArmyList();

            //Npc被箭塔攻击
            int NpcBeAttackByCatapult(float catapultAttack);
            //玩家被魔法塔攻击 返回击杀数量
            int PlayersBeAttackByCatapult(float catapultAttack);
            //增加记录
            void AddPalaceWarRecord(model::PalaceWarRecordType type, const std::string& param);
            //增加历任国王记录
            void AddSuccessiveKingRecord(const Agent& king);
            //全部遣返
            void RepatriateAllDefenders();
            // 增加打龙记录
            //void AddAttackDragonRecord(int64_t uid, int hurt);
            //
            int ArmyCount();

            int NormalArmyCout(); 

        public:
            //palaceWar
            void InitWar();
            void StartWar();
            void EndWar();
            void NPCRecover();
            void CatapultsRecover();

        public:

            // 是否可占领
            virtual bool CanOccupy(Troop* troop);
            /*virtual void OnTroopLeave(Troop* troop);*/
            void Garrison(Troop* troop);
            //行军加载完成
            void OnTroopLoadFinished(Troop* troop);

            void OnPlaceWarStart();
            void OnPlaceWarEnd();
            void OnKingChanged();
            // 龙死亡
            //void OnDragonDie(Agent& killer);
            //占领
            void Occupy(Troop* troop);
        public:
            virtual std::string Serialize() override;
            virtual bool Deserialize(std::string data) override;
            virtual void FinishDeserialize() override;

            void DebugKillDragonRanks();
        private:
            void Debug();
            /*info::PalaceWarAttackDragonRecord* FindAttackDragonRecord(int64_t uid) {
                auto it = m_attackDragonRecords.find(uid);
                return it != m_attackDragonRecords.end() ? &it->second : nullptr;
            }
            */
            //检查防守军 死亡则回家
            void ResetPrepareNoticeTimer();

        private:
            static constexpr int START_WAR = 1;
            static constexpr int END_WAR = 2;
            static constexpr int NPC_RECOVER = 3;
            static constexpr int CATAPULT_RECOVER = 4;
            static constexpr int PREPARE_NOTICE = 5;               //备战通知
            static constexpr int DEBUG_DATA = 6;          //测试输出数据



            const ms::map::tpl::MapUnitCapitalTpl* m_tpl = nullptr;
            const ms::map::tpl::MapCityTpl* m_cityTpl = nullptr;

            //const model::tpl::ArmyTpl* m_dragonTpl = nullptr;       //要保证龙模板不为空
            //ArmyList* m_armyList = nullptr;

            //std::list<Troop*> m_troops;
            //std::list<int> m_troopIds;

            //int m_dragonHp = 0;         //龙当前血量
            int64_t m_occupierId = 0; //占领者联盟ID
            std::unordered_map<int64_t, Troop*> m_defenders;    //uid为key, 同时只能存在一个玩家的队列
            model::PalaceState m_state = model::PalaceState::NOSTART;
            Agent* m_occupier = nullptr;    //占领者信息

            //placeWar 王城争霸
            int64_t m_startTimestamp = 0;
            int64_t m_endTimestamp = 0;
            int64_t m_nextNPCRecoverTimestamp = 0;           //下一次王城NPC恢复时间戳
            int64_t m_nextCpRecoverTimestamp = 0;           //下一次魔法塔恢复时间戳
            std::unordered_map<int64_t, ArmyList> m_recordByCatapultAttack;     //被魔法塔攻击的死亡记录 <uid, ArmyList>
            std::list<info::SuccessiveKing> m_successiveKings;        //历任国王记录 只记录100个
            std::list<info::PalaceWarRecord> m_palaceWarRecords;      //王城争霸记录 只记录100个
            //std::unordered_map<int64_t, info::PalaceWarAttackDragonRecord> m_attackDragonRecords;       // 打龙伤害记录 用于杀龙排行
            
            base::command::Runner m_runner;

            friend class qo::CommandSuccessiveKingsLoad;
     
        };
    }
}
#endif // MAP_PALACE_H
