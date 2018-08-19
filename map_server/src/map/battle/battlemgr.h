#ifndef MAP_BATTLE_BATTLEMGR_H
#define MAP_BATTLE_BATTLEMGR_H
# include <string>

namespace engine
{
    struct InitialDataInput;
    struct WarReport;
}

namespace model
{
    enum class BattleType;
}

namespace base
{
    namespace gateway
    {
        class PacketOutBase;
    }
    class DataTable;
}

namespace ms
{
    namespace map
    {

        class Unit;
        class FamousCity;
        class Troop;
        class Agent;
        class Point;

        namespace battle
        {
            class Battle;

            //战场管理器 同一时间只有一场战斗
            class BattleMgr
            {
            public:
                virtual ~BattleMgr() {}

            public:
                static BattleMgr& instance() {
                    static BattleMgr mgr;
                    return mgr;
                }

                bool GenerateBattle (Troop* troop, Unit* unit);

                void PatrolCity(Troop* troop, FamousCity* city);

            protected:

                std::string SerializeReportData(engine::WarReport& report, int& reportId);

            private:
                BattleMgr() {}
                BattleMgr (const BattleMgr& mgr) {}
                void operator= (const BattleMgr& mgr) {}

            private:

                unsigned int m_maxBattleId = 10000;
                Battle* m_currentBattle = nullptr;      //当前战斗
            };
        }
    }
}

#endif // MAP_BATTLE_BATTLEMGR_H

