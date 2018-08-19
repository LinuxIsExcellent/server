#include "playersession.h"
#include "worldmgr.h"
#include <base/gateway/userclient.h>
#include <base/3rd/lua/lua.hpp>
#include <base/framework.h>
#include <base/utils/file.h>
#include <base/lua/lua_module_net.h>
#include <base/lua/luaex.h>
#include <base/lua/lua_module_dbo.h>
#include <base/lua/lua_module_timer.h>
#include <base/lua/lua_module_utils.h>
#include <base/lua/modulelua.h>
#include <base/lua/luavmpool.h>
#include <base/logger.h>
#include <base/event/dispatcher.h>
#include <base/utils/utils_string.h>
#include <model/protocol.h>
#include <model/metadata.h>
#include "model/tpl/templateloader.h"
#include <model/tpl/configure.h>
#include "luamapagent.h"
#include "interface.h"
#include "base/utils/crypto.h"

namespace fs
{
    using namespace std;
    using namespace base::lua;
    using namespace model::tpl;

    static const char ADDR_PS = 'c';
    static const char* MT_LUA_PLAYER = "mt_player";
    ///
    /// LuaPlayerSession
    ///
    class LuaPlayerSession
    {
    public:
        LuaPlayerSession(PlayerSession& ps, int msServiceId) : m_ps(ps), m_msServiceId(msServiceId), m_observer(ps.m_autoObserver.GetObserver()) {
            m_observer->Retain();
        }
        ~LuaPlayerSession() {
            m_observer->Release();
            cout << "##LuaPlayerSession::~dtor" << endl;
        }

        static int _lua_player_newPktout(lua_State* L) {
            LuaPlayerSession* self = (LuaPlayerSession*)luaL_checkudata(L, 1, MT_LUA_PLAYER);
            if (self->m_observer->IsExist()) {
                int code = luaL_checkinteger(L, 2);
                int size = luaL_checkinteger(L, 3);
                int session = luaL_optinteger(L, 4, 0);
                return lua_net_new_pktout(self->m_ps.client(), L, code, size, session);
            }
            return 0;
        }

        static int _lua_player_sendPktout(lua_State* L) {
            LuaPlayerSession* self = (LuaPlayerSession*)luaL_checkudata(L, 1, MT_LUA_PLAYER);
            if (self->m_observer->IsExist()) {
                return lua_net_send_values(self->m_ps.client(), L, 2);
            }
            return 0;
        }

        static int _lua_player_send(lua_State* L) {
            LuaPlayerSession* self = (LuaPlayerSession*)luaL_checkudata(L, 1, MT_LUA_PLAYER);
            if (self->m_observer->IsExist()) {
                base::gateway::PacketOut* pktout = (base::gateway::PacketOut*)lua_touserdata(L, 2);
                self->m_ps.Send(*pktout);
            }
            return 0;
        }

        static int _lua_player_replyPktout(lua_State* L) {
            LuaPlayerSession* self = (LuaPlayerSession*)luaL_checkudata(L, 1, MT_LUA_PLAYER);
            int session = luaL_checkinteger(L, 2);
            if (self->m_observer->IsExist()) {
                return lua_net_send_values(self->m_ps.client(), L, 3, session);
            }
            return 0;
        }

        static int _lua_player_connectMapService(lua_State* L) {
            LuaPlayerSession* self = (LuaPlayerSession*)luaL_checkudata(L, 1, MT_LUA_PLAYER);
            if (self && self->m_observer->IsExist()) {
                int64_t uid = luaL_checkinteger(L, 2);
                LuaMapAgent* ud = (LuaMapAgent*)lua_newuserdata(L, sizeof(LuaMapAgent));
                new(ud)LuaMapAgent(&(self->m_ps));
                ud->SetUid(uid);
                ud->SetMapServiceId(self->m_msServiceId);
                std::string name;
                base::utils::string_append_format(name, "map.%d@ms", self->m_msServiceId);
                if (ud->ConnectMapService(name)) {
                    int r = luaL_newmetatable(L, MT_LUA_MAP_AGENT);
                    if (r != 0) {
                        luaL_Reg func[] = {
                            {"call", LuaMapAgent::_luaCall},
                            {"cast", LuaMapAgent::_luaCast},
                            {"forward", LuaMapAgent::_forward},
                            {"setMapEvtHandler", LuaMapAgent::_setMapEvtHandler},
                            {"crossTeleport", LuaMapAgent::_crossTeleport},
                            {"checkMsExist", LuaMapAgent::_checkMsExist},
                            {"quitAllMap", LuaMapAgent::_quitAllMap},
                            {"__gc", LuaMapAgent::_gc},
                            {nullptr, nullptr},
                        };

                        luaL_setfuncs(L, func, 0);
                        lua_pushstring(L, "__index");
                        lua_pushvalue(L, -2);
                        lua_rawset(L, -3);
                    }
                    lua_setmetatable(L, -2);
                    return 1;
                }
            } else {
                LOG_ERROR("_lua_player_connectMapService m_observer->IsNotExist()");
            }
            return 0;
        }

        static int _lua_player_isTrust(lua_State* L) {
            bool trust = false;
            LuaPlayerSession* self = (LuaPlayerSession*)luaL_checkudata(L, 1, MT_LUA_PLAYER);
            if (self->m_observer->IsExist()) {
                trust = self->m_ps.client()->is_trust();
            }
            lua_pushboolean(L, trust);
            return 1;
        }

        static int _lua_player_setTrust(lua_State* L) {
            LuaPlayerSession* self = (LuaPlayerSession*)luaL_checkudata(L, 1, MT_LUA_PLAYER);
            if (self->m_observer->IsExist()) {
                self->m_ps.client()->SetTrust();
            }
            return 0;
        }

        static int _lua_player_setEvtHandler(lua_State* L) {
            luaL_checkudata(L, 1, MT_LUA_PLAYER);
            luaL_checktype(L, 2, LUA_TTABLE);
            lua_pushvalue(L, 2);
            lua_setuservalue(L, 1);
            return 0;
        }

        static int _lua_player_exit(lua_State* L) {
            LuaPlayerSession* self = (LuaPlayerSession*)luaL_checkudata(L, 1, MT_LUA_PLAYER);
            if (self->m_observer->IsExist()) {
                self->m_ps.Exit();
            }
            return 0;
        }

        static int _lua_player___gc(lua_State* L) {
            LuaPlayerSession* self = (LuaPlayerSession*)luaL_checkudata(L, 1, MT_LUA_PLAYER);
            self->~LuaPlayerSession();
            return 0;
        }

        static int  _lua_combat(lua_State* L) {
            luaL_checktype(L, 2, LUA_TTABLE); //battleData
            //luaL_checktype(L, 3, LUA_TTABLE); //releaseSpells
            
            auto getInitialDataInput = [](lua_State * L, int tableIndex, engine::InitialDataInput & initialDataInput, engine::battle::HeroCombatDataInput & heroCombatDataInput) {
                lua_pushstring(L, "uid");
                lua_rawget(L, tableIndex);
                initialDataInput.uid = luaL_optinteger(L, -1, 0);
                heroCombatDataInput.uid = luaL_optinteger(L, -1, 0);
                lua_pop(L, 1);

                lua_pushstring(L, "singleCombatHero");
                lua_rawget(L, tableIndex);
                initialDataInput.singleCombatHero = luaL_optinteger(L, -1, 0);
                initialDataInput.singleCombatHero = luaL_optinteger(L, -1, 0);
                lua_pop(L, 1);

                lua_pushstring(L, "headId");
                lua_rawget(L, tableIndex);
                heroCombatDataInput.headIcon = luaL_optinteger(L, -1, 0);
                lua_pop(L, 1);

                lua_pushstring(L, "nickname");
                lua_rawget(L, tableIndex);
                std::string nullstr = "";
                size_t size = nullstr.size();
                heroCombatDataInput.name = luaL_optlstring(L, -1, nullstr.data(),  &size);
                lua_pop(L, 1);

                lua_pushstring(L, "level");
                lua_rawget(L, tableIndex);
                heroCombatDataInput.level = luaL_optinteger(L, -1, 0);
                lua_pop(L, 1);

                lua_pushstring(L, "team");
                lua_rawget(L, tableIndex);
                if (lua_istable(L, -1)) {
                    lua_pushnil(L);
                    while (lua_next(L, -2)) {
                        engine::I_Group i_group;
                        lua_pushvalue(L, -2);
//                         const char* key = lua_tostring(L, -1);
                        if (lua_istable(L, -2)) {
                            lua_pushstring(L, "hero");
                            lua_rawget(L, -3);
                            if (lua_istable(L, -1)) {
                                lua_pushstring(L, "id");
                                lua_rawget(L, -2);
                                i_group.hero.id = luaL_optinteger(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "level");
                                lua_rawget(L, -2);
                                i_group.hero.level = luaL_optinteger(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "star");
                                lua_rawget(L, -2);
                                i_group.hero.star = luaL_optinteger(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "heroPower");
                                lua_rawget(L, -2);
                                i_group.hero.heroPower = luaL_optnumber(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "heroDefense");
                                lua_rawget(L, -2);
                                i_group.hero.heroDefense = luaL_optnumber(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "heroWisdom");
                                lua_rawget(L, -2);
                                i_group.hero.heroWisdom = luaL_optnumber(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "heroLucky");
                                lua_rawget(L, -2);
                                i_group.hero.heroLucky = luaL_optnumber(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "heroSkill");
                                lua_rawget(L, -2);
                                i_group.hero.heroSkill = luaL_optnumber(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "heroAgile");
                                lua_rawget(L, -2);
                                i_group.hero.heroAgile = luaL_optnumber(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "heroLife");
                                lua_rawget(L, -2);
                                i_group.hero.heroLife = luaL_optnumber(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "heroPhysicalPower");
                                lua_rawget(L, -2);
                                i_group.hero.heroPhysicalPower = luaL_optnumber(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "heroPhysicalDefense");
                                lua_rawget(L, -2);
                                i_group.hero.heroPhysicalDefense = luaL_optnumber(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "heroSkillPower");
                                lua_rawget(L, -2);
                                i_group.hero.heroSkillPower = luaL_optnumber(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "heroSkillDefense");
                                lua_rawget(L, -2);
                                i_group.hero.heroSkillDefense = luaL_optnumber(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "heroHit");
                                lua_rawget(L, -2);
                                i_group.hero.heroHit = luaL_optnumber(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "heroAvoid");
                                lua_rawget(L, -2);
                                i_group.hero.heroAvoid = luaL_optnumber(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "heroCritHit");
                                lua_rawget(L, -2);
                                i_group.hero.heroCritHit = luaL_optnumber(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "heroCritAvoid");
                                lua_rawget(L, -2);
                                i_group.hero.heroCritAvoid = luaL_optnumber(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "heroSpeed");
                                lua_rawget(L, -2);
                                i_group.hero.heroSpeed = luaL_optnumber(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "heroCityLife");
                                lua_rawget(L, -2);
                                i_group.hero.heroCityLife = luaL_optnumber(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "heroFight");
                                lua_rawget(L, -2);
                                i_group.hero.heroTroops = luaL_optinteger(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "heroSolohp");
                                lua_rawget(L, -2);
                                i_group.hero.heroSolohp = luaL_optinteger(L, -1, 0);
                                i_group.hero.physical = i_group.hero.heroSolohp;
                                lua_pop(L, 1);

                                lua_pushstring(L, "skill");
                                lua_rawget(L, -2);
                                if (lua_istable(L, -1)) {
                                    lua_pushnil(L);
                                    while (lua_next(L, -2)) {
                                        engine::I_Spell i_spell;
                                        lua_pushvalue(L, -2);
//                                         const char* key = lua_tostring(L, -1);
                                        if (lua_istable(L, -2)) {
                                            lua_pushstring(L, "tplId");
                                            lua_rawget(L, -3);
                                            i_spell.id = luaL_optinteger(L, -1, 0);
                                            lua_pop(L, 1);

                                            lua_pushstring(L, "level");
                                            lua_rawget(L, -3);
                                            i_spell.level = luaL_optinteger(L, -1, 0);
                                            lua_pop(L, 1);
                                            i_group.hero.spells.push_back(i_spell);
                                        }
                                        lua_pop(L, 2);
                                    }
                                }
                                lua_pop(L, 1);
                            }
                            lua_pop(L, 1);

                            lua_pushstring(L, "army");
                            lua_rawget(L, -3);
                            if (lua_istable(L, -1)) {
                                lua_pushstring(L, "armyType");
                                lua_rawget(L, -2);
                                i_group.army.armyType = luaL_optinteger(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "armyLevel");
                                lua_rawget(L, -2);
                                i_group.army.armyLevel = luaL_optinteger(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "armyCount");
                                lua_rawget(L, -2);
                                i_group.army.armyCount = luaL_optinteger(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "hp");
                                lua_rawget(L, -2);
                                i_group.army.hp = luaL_optinteger(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "attack");
                                lua_rawget(L, -2);
                                i_group.army.attack = luaL_optinteger(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "defense");
                                lua_rawget(L, -2);
                                i_group.army.defense = luaL_optinteger(L, -1, 0);
                                lua_pop(L, 1);

                                lua_pushstring(L, "speed");
                                lua_rawget(L, -2);
                                i_group.army.speed = luaL_optinteger(L, -1, 0);
                                lua_pop(L, 1);
                            }
                            lua_pop(L, 1);

                            lua_pushstring(L, "position");
                            lua_rawget(L, -3);
                            i_group.position = luaL_optinteger(L, -1, 0);
                            lua_pop(L, 1);
                        }
                        lua_pop(L, 2);

                        initialDataInput.team.groups.push_back(i_group);
                        heroCombatDataInput.heroes.push_back(i_group.hero);
                    }
                }
                lua_pop(L, 1);
            };

            engine::InitialDataInput attackInput;
            engine::InitialDataInput defenseInput;

            engine::battle::HeroCombatDataInput attackHeroCombatInput;
            engine::battle::HeroCombatDataInput defenseHeroCombatInput;

            
            lua_pushstring(L, "attack");
            lua_rawget(L, 2);
            getInitialDataInput(L, 3, attackInput, attackHeroCombatInput);
            lua_pop(L, 1);
            
            lua_pushstring(L, "defence");
            lua_rawget(L, 2);
            getInitialDataInput(L, 3, defenseInput, defenseHeroCombatInput);
            lua_pop(L, 1);
            
            lua_pushstring(L, "randSeed");
            lua_rawget(L, 2);
            int randSeed = luaL_optinteger(L, 3, 100);
            lua_pop(L, 1);
            
            lua_pushstring(L, "levelType");
            lua_rawget(L, 2);
            int levelType = luaL_optinteger(L, 3, 0);
            lua_pop(L, 1);

            lua_pushstring(L, "battleType");
            lua_rawget(L, 2);
            model::BattleType battleType = static_cast<model::BattleType>(luaL_optinteger(L, 3, 0));
            lua_pop(L, 1);

            std::cout << "lua ------ combat ------------- 1 " << levelType << std::endl;

            //将levelType = 0时就是必定单挑
            engine::Combat combat(randSeed, levelType == 0, false, levelType);
            engine::battle::HeroCombat heroCombat(randSeed);
            engine::WarReport report;
            std::vector<engine::WarReport::ArenaData> arenaReport;
            bool isWin = false;
            int attackWinRound = 0;
            int maxRound = 0;
            bool single = combat.Init(attackInput, defenseInput);
            if (single) {
                // 判断是何种单挑
                do 
                {
                    if (battleType == model::BattleType::SCENARIO_COPY_COMBAT && levelType == 0)
                    {
                        heroCombat.Init(attackHeroCombatInput, defenseHeroCombatInput);
                        while (engine::battle::HeroCombatState::ALL_OVER != heroCombat.Next());
                        auto& result = heroCombat.arenaReport();
                        arenaReport = result;
                        isWin = heroCombat.IsAttackWin();
                        attackWinRound = heroCombat.attackWinCount();
                        maxRound = heroCombat.maxBigRound();
                        break;
                    }
                    // 如果不是副本单挑或者副本是混战（单挑 + 混战），则走城外的 1v1单挑
                    else if (battleType != model::BattleType::SCENARIO_COPY_COMBAT || (battleType == model::BattleType::SCENARIO_COPY_COMBAT && levelType == 2))
                    {
                        combat.CreateSingleCombat();
                        combat.StartSingleCombat();
                        // 做一个减兵处理 
                        combat.OutputSingleResult(combat.singleCombat()->result());
                        break;
                    }
                }while(0);
            }
            if (levelType == 0)
            {
                if (!single)
                {
                    lua_pushinteger(L, 1);
                    return 1;
                }
                if (battleType == model::BattleType::SCENARIO_COPY_COMBAT)
                {
                    
                }
                else
                {
                    auto& singleResult = combat.singleCombat()->result();
                    engine::WarReport::SoloData attackSoloReport;
                    engine::WarReport::SoloData defenseSoloReport;
                    attackSoloReport = combat.singleCombat()->AttackSoloReport();
                    defenseSoloReport = combat.singleCombat()->DefenseSoloReport();
                    report.soloDatas.push_back(attackSoloReport);
                    report.soloDatas.push_back(defenseSoloReport);
                    isWin = singleResult.isAttackWin;
                }        
                //返回战斗结果
                lua_pushinteger(L, isWin ? 1 : 0);
                lua_pushinteger(L, attackWinRound);
                lua_pushinteger(L, maxRound);
                lua_pushinteger(L, 0);
                lua_pushinteger(L, 0);

                int reportId = 0;
                engine::WarReport::InitTeamData initTeamData1;
                initTeamData1.uid = attackInput.uid;
                initTeamData1.headId = attackInput.headIcon;
                initTeamData1.nickName = attackInput.name;
                initTeamData1.soloHeroId = -1;

                engine::WarReport::InitTeamData initTeamData2;
                initTeamData2.uid = defenseInput.uid;
                initTeamData2.headId = defenseInput.headIcon;
                initTeamData2.nickName = defenseInput.name;
                initTeamData2.soloHeroId = -1;
                report.initTeamDatas.push_back(initTeamData1);
                report.initTeamDatas.push_back(initTeamData2);
                report.arenaDatas = arenaReport;
                report.result.win = heroCombat.IsAttackWin() ? engine::TeamType::ATTACKER : engine::TeamType::DEFENDER;
                std::string reportData = engine::Combat::SerializeReportData(report, reportId);
                lua_pushstring(L, reportData.c_str());

                return 6;
            }

            combat.CreateMixedCombat();
            combat.StartMixedCombat();
            if (combat.mixedCombat()) {
                auto& mixedResult = combat.mixedCombat()->result();
                //返回战斗结果
                lua_pushinteger(L, mixedResult.isAttackWin ? 1 : 0);
                lua_pushinteger(L, attackWinRound);
                lua_pushinteger(L, maxRound);
                lua_pushinteger(L, combat.mixedCombat()->beginArmyCount());
                lua_pushinteger(L, combat.mixedCombat()->endArmyCount());

                // std::cout << mixedResult.isAttackWin << endl;
                report = combat.mixedCombat()->report();
                engine::WarReport::InitTeamData initTeamData1;
                initTeamData1.uid = attackInput.uid;
                initTeamData1.headId = attackInput.headIcon;
                initTeamData1.nickName = attackInput.name;
                initTeamData1.soloHeroId = -1;

                engine::WarReport::InitTeamData initTeamData2;
                initTeamData2.uid = defenseInput.uid;
                initTeamData2.headId = defenseInput.headIcon;
                initTeamData2.nickName = defenseInput.name;
                initTeamData2.soloHeroId = -1;
                report.initTeamDatas.push_back(initTeamData1);
                report.initTeamDatas.push_back(initTeamData2);

                bool isTroopWin = mixedResult.isAttackWin;
                report.result.win = isTroopWin ? engine::TeamType::ATTACKER : engine::TeamType::DEFENDER;
                report.arenaDatas = arenaReport;
                int reportId = 0;
                std::string reportData = engine::Combat::SerializeReportData(report, reportId);
                lua_pushstring(L, reportData.c_str());

                return 6;
            }
            return 0;
        }

        static int  _lua_hero_combat(lua_State* L) {
            //LuaPlayerSession* self = (LuaPlayerSession*)luaL_checkudata(L, 1, MT_LUA_PLAYER);
            luaL_checktype(L, 2, LUA_TTABLE); //battleData

            auto getHeroCombatDataInput = [](lua_State * L, int tableIndex, engine::battle::HeroCombatDataInput & heroCombatDataInput) {
                lua_pushstring(L, "uid");
                lua_rawget(L, tableIndex);
                heroCombatDataInput.uid = luaL_optinteger(L, -1, 0);
                lua_pop(L, 1);

                lua_pushstring(L, "headId");
                lua_rawget(L, tableIndex);
                heroCombatDataInput.headIcon = luaL_optinteger(L, -1, 0);
                lua_pop(L, 1);

                lua_pushstring(L, "nickname");
                lua_rawget(L, tableIndex);
                std::string nullstr = "";
                size_t size = nullstr.size();
                heroCombatDataInput.name = luaL_optlstring(L, -1, nullstr.data(),  &size);
                lua_pop(L, 1);

                lua_pushstring(L, "level");
                lua_rawget(L, tableIndex);
                heroCombatDataInput.level = luaL_optinteger(L, -1, 0);
                lua_pop(L, 1);

                lua_pushstring(L, "heroList");
                lua_rawget(L, tableIndex);
                if (lua_istable(L, -1)) {
                    lua_pushnil(L);
                    while (lua_next(L, -2)) {
                        engine::I_Hero i_hero;
                        lua_pushvalue(L, -2);
//                         const char* key = lua_tostring(L, -1);
                        if (lua_istable(L, -2)) {
                            lua_pushstring(L, "id");
                            lua_rawget(L, -3);
                            i_hero.id = luaL_optinteger(L, -1, 0);
                            lua_pop(L, 1);

                            lua_pushstring(L, "level");
                            lua_rawget(L, -3);
                            i_hero.level = luaL_optinteger(L, -1, 0);
                            lua_pop(L, 1);

                            lua_pushstring(L, "star");
                            lua_rawget(L, -3);
                            i_hero.star = luaL_optinteger(L, -1, 0);
                            lua_pop(L, 1);

                            lua_pushstring(L, "heroPower");
                            lua_rawget(L, -3);
                            i_hero.heroPower = luaL_optnumber(L, -1, 0);
                            lua_pop(L, 1);

                            lua_pushstring(L, "heroDefense");
                            lua_rawget(L, -3);
                            i_hero.heroDefense = luaL_optnumber(L, -1, 0);
                            lua_pop(L, 1);

                            lua_pushstring(L, "heroWisdom");
                            lua_rawget(L, -3);
                            i_hero.heroWisdom = luaL_optnumber(L, -1, 0);
                            lua_pop(L, 1);

                            lua_pushstring(L, "heroLucky");
                            lua_rawget(L, -3);
                            i_hero.heroLucky = luaL_optnumber(L, -1, 0);
                            lua_pop(L, 1);

                            lua_pushstring(L, "heroSkill");
                            lua_rawget(L, -3);
                            i_hero.heroSkill = luaL_optnumber(L, -1, 0);
                            lua_pop(L, 1);

                            lua_pushstring(L, "heroAgile");
                            lua_rawget(L, -3);
                            i_hero.heroAgile = luaL_optnumber(L, -1, 0);
                            lua_pop(L, 1);

                            lua_pushstring(L, "heroLife");
                            lua_rawget(L, -3);
                            i_hero.heroLife = luaL_optnumber(L, -1, 0);
                            lua_pop(L, 1);

                            lua_pushstring(L, "heroPhysicalPower");
                            lua_rawget(L, -3);
                            i_hero.heroPhysicalPower = luaL_optnumber(L, -1, 0);
                            lua_pop(L, 1);

                            lua_pushstring(L, "heroPhysicalDefense");
                            lua_rawget(L, -3);
                            i_hero.heroPhysicalDefense = luaL_optnumber(L, -1, 0);
                            lua_pop(L, 1);

                            lua_pushstring(L, "heroSkillPower");
                            lua_rawget(L, -3);
                            i_hero.heroSkillPower = luaL_optnumber(L, -1, 0);
                            lua_pop(L, 1);

                            lua_pushstring(L, "heroSkillDefense");
                            lua_rawget(L, -3);
                            i_hero.heroSkillDefense = luaL_optnumber(L, -1, 0);
                            lua_pop(L, 1);

                            lua_pushstring(L, "heroHit");
                            lua_rawget(L, -3);
                            i_hero.heroHit = luaL_optnumber(L, -1, 0);
                            lua_pop(L, 1);

                            lua_pushstring(L, "heroAvoid");
                            lua_rawget(L, -3);
                            i_hero.heroAvoid = luaL_optnumber(L, -1, 0);
                            lua_pop(L, 1);

                            lua_pushstring(L, "heroCritHit");
                            lua_rawget(L, -3);
                            i_hero.heroCritHit = luaL_optnumber(L, -1, 0);
                            lua_pop(L, 1);

                            lua_pushstring(L, "heroCritAvoid");
                            lua_rawget(L, -3);
                            i_hero.heroCritAvoid = luaL_optnumber(L, -1, 0);
                            lua_pop(L, 1);

                            lua_pushstring(L, "heroSpeed");
                            lua_rawget(L, -3);
                            i_hero.heroSpeed = luaL_optnumber(L, -1, 0);
                            lua_pop(L, 1);

                            lua_pushstring(L, "heroCityLife");
                            lua_rawget(L, -3);
                            i_hero.heroCityLife = luaL_optnumber(L, -1, 0);
                            lua_pop(L, 1);

                            lua_pushstring(L, "heroFight");
                            lua_rawget(L, -3);
                            i_hero.heroTroops = luaL_optinteger(L, -1, 0);
                            lua_pop(L, 1);

                            lua_pushstring(L, "heroSolohp");
                            lua_rawget(L, -3);
                            i_hero.heroSolohp = luaL_optinteger(L, -1, 0);
                            i_hero.physical = i_hero.heroSolohp;
                            lua_pop(L, 1);

                            lua_pushstring(L, "skill");
                            lua_rawget(L, -3);
                            if (lua_istable(L, -1)) {
                                lua_pushnil(L);
                                while (lua_next(L, -2)) {
                                    engine::I_Spell i_spell;
                                    lua_pushvalue(L, -2);
//                                         const char* key = lua_tostring(L, -1);
                                    if (lua_istable(L, -2)) {
                                        lua_pushstring(L, "tplId");
                                        lua_rawget(L, -3);
                                        i_spell.id = luaL_optinteger(L, -1, 0);
                                        lua_pop(L, 1);

                                        lua_pushstring(L, "level");
                                        lua_rawget(L, -3);
                                        i_spell.level = luaL_optinteger(L, -1, 0);
                                        lua_pop(L, 1);
                                        i_hero.spells.push_back(i_spell);
                                    }
                                    lua_pop(L, 2);
                                }
                            }
                            lua_pop(L, 1);
                        }
                        lua_pop(L, 2);

                        heroCombatDataInput.heroes.push_back(i_hero);
                    }
                }
                lua_pop(L, 1);
            };

            engine::battle::HeroCombatDataInput attackInput;
            engine::battle::HeroCombatDataInput defenceInput;

            lua_pushstring(L, "attack");
            lua_rawget(L, 2);
            getHeroCombatDataInput(L, 3, attackInput);
            lua_pop(L, 1);

            lua_pushstring(L, "defence");
            lua_rawget(L, 2);
            getHeroCombatDataInput(L, 3, defenceInput);
            lua_pop(L, 1);

            lua_pushstring(L, "randSeed");
            lua_rawget(L, 2);
            int randSeed = luaL_optinteger(L, -1, 100);
            lua_pop(L, 1);

            // lua_pushstring(L, "battleId");
            // lua_rawget(L, -2);
            // int battleId = luaL_optinteger(L, -1, 0);
            // lua_pop(L, 1);

            // lua_pushstring(L, "battleType");
            // lua_rawget(L, -2);
            // int battleType = luaL_optinteger(L, -1, 0);
            // lua_pop(L, 1);

            engine::battle::HeroCombat heroCombat(randSeed);
            heroCombat.Init(attackInput, defenceInput);

            while (engine::battle::HeroCombatState::ALL_OVER != heroCombat.Next());

            //返回战斗结果
            lua_pushinteger(L, heroCombat.IsAttackWin() ? 1 : 0);

            engine::WarReport report;
            report.arenaDatas = heroCombat.arenaReport();
            engine::WarReport::InitTeamData initTeamData1;
            initTeamData1.uid = attackInput.uid;
            initTeamData1.headId = attackInput.headIcon;
            initTeamData1.nickName = attackInput.name;
            initTeamData1.soloHeroId = -1;

            engine::WarReport::InitTeamData initTeamData2;
            initTeamData2.uid = defenceInput.uid;
            initTeamData2.headId = defenceInput.headIcon;
            initTeamData2.nickName = defenceInput.name;
            initTeamData2.soloHeroId = -1;

            report.initTeamDatas.push_back(initTeamData1);
            report.initTeamDatas.push_back(initTeamData2);

            report.result.win = heroCombat.IsAttackWin() ? engine::TeamType::ATTACKER : engine::TeamType::DEFENDER;

            int reportId = 0;
            std::string reportData = engine::Combat::SerializeReportData(report, reportId);
            lua_pushstring(L, reportData.c_str());
            return 2;
        }

    private:
        PlayerSession& m_ps;
        int m_msServiceId;
        base::Observer* m_observer = nullptr;
    };

    ///
    /// PlayerSession
    ///
    PlayerSession::PlayerSession(base::gateway::UserClient* client, int msServiceId): UserSession(client), m_msServiceId(msServiceId), m_vm(nullptr)
    {
    }

    PlayerSession::~PlayerSession()
    {
        if (m_vm) {
            lua_State* L = m_vm->l();
            lua_rawgetp(L, LUA_REGISTRYINDEX, &ADDR_PS);
            lua_pushnil(L);
            lua_rawsetp(L, -2, this);
            if (fs::lua_get_map_registry(L) == LUA_TTABLE) {
                lua_pushnil(L);
                lua_rawsetp(L, -2, this);
            }
            lua_gc(L, LUA_GCCOLLECT, 0);  // statistic time cost
            // 假如Agent里有数据关联着会导致垃圾回收不了 LuaPlayerSession和LuaMapAgent无法析构
            SAFE_RELEASE(m_vm);
        }

        if (m_luaMapAgent) {
            LOG_ERROR("luaMapAgent not null when playerSession delete");
            m_luaMapAgent->ClearPlayerSession();
            m_luaMapAgent = nullptr;
        }
        cout << "PlayerSession::~dtor" << endl;
    }

    bool PlayerSession::Setup()
    {
        m_vm = g_module_lua->AquireVm(0);
        if (!m_vm) {
            return false;
        }
        m_vm->Retain();

        lua_State* L = m_vm->l();
        lua_settop(L, 0);

        LuaPlayerSession* ud = (LuaPlayerSession*)lua_newuserdata(L, sizeof(LuaPlayerSession));
        new(ud)LuaPlayerSession(*this, m_msServiceId);
        int r = luaL_newmetatable(L, MT_LUA_PLAYER);
        if (r != 0) {
            luaL_Reg playerFuncs[] = {
                {"newPktout", LuaPlayerSession::_lua_player_newPktout},
                {"sendPktout", LuaPlayerSession::_lua_player_sendPktout},
                {"send", LuaPlayerSession::_lua_player_send},
                {"replyPktout", LuaPlayerSession::_lua_player_replyPktout},
                {"exit", LuaPlayerSession::_lua_player_exit},
                {"setEvtHandler", LuaPlayerSession::_lua_player_setEvtHandler},
                {"isTrust", LuaPlayerSession::_lua_player_isTrust},
                {"setTrust", LuaPlayerSession::_lua_player_setTrust},
                {"connectMapService", LuaPlayerSession::_lua_player_connectMapService},
                {"__gc", LuaPlayerSession::_lua_player___gc},
                {"startCombat", LuaPlayerSession::_lua_combat},
                {"startHeroCombat", LuaPlayerSession::_lua_hero_combat}, 
                {nullptr, nullptr},
            };

            luaL_setfuncs(L, playerFuncs, 0);
            lua_pushstring(L, "__index");
            lua_pushvalue(L, -2);
            lua_rawset(L, -3);
        }
        lua_setmetatable(L, -2);
        // save player session to lua registry
        lua_rawgetp(L, LUA_REGISTRYINDEX, &ADDR_PS);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            lua_newtable(L);
            lua_pushvalue(L, -1);
            lua_rawsetp(L, LUA_REGISTRYINDEX, &ADDR_PS);
            lua_pushstring(L, "collect_name");
            lua_pushstring(L, "list_raw_agent");
            lua_rawset(L, -3);
        }
        lua_pushvalue(L, -2);
        lua_rawsetp(L, -2, this);
        lua_pop(L, 1);
        m_vm->ExecuteCachedScript("agent.lua", 1);
        return true;
    }

    void PlayerSession::Exit()
    {
        m_closed = true;

        client()->Close();
    }

    void PlayerSession::SendLogout()
    {
        base::gateway::PacketOut pktout((uint16_t)model::SC::LOGOUT, 30, client()->mempool());
        pktout.SetSessionID(0u);
        pktout.WriteVarInteger((int)model::KickType::MAINTENANCE);
        Send(pktout);
    }

    static int get_player_evt_handler(lua_State* L, const char* name, PlayerSession* ps)
    {
        lua_rawgetp(L, LUA_REGISTRYINDEX, &ADDR_PS);
        lua_rawgetp(L, -1, ps);
        lua_getuservalue(L, -1);
        if (lua_istable(L, -1)) {
            lua_pushstring(L, name);
            lua_rawget(L, -2);
            if (lua_isfunction(L, -1)) {
                return 1;
            }
        }
        return 0;
    }

    void PlayerSession::OnUserClientReceivePacket(base::gateway::PacketIn& pktin)
    {
        if (m_closed) {
            return;
        }
        
        lua_State* L = m_vm->l();
        lua_settop(L, 0);
        if (get_player_evt_handler(L, "onReceive", this)) {
            base::lua::lua_net_push_packet_in(L, pktin);
            int err = lua_pcall(L, 1, 0, 0);
            if (err) {
                LOG_ERROR("[%s] exec fail: %s, code=%d\n", "onReceive", lua_tostring(L, -1), pktin.code());
            }
        }
        lua_settop(L, 0);

        gWorldMgr->OnRecvPacket(pktin);
    }

    void PlayerSession::OnUserClientClose()
    {
        m_closed = true;

        lua_State* L = m_vm->l();
        lua_settop(L, 0);
        if (get_player_evt_handler(L, "onClose", this)) {
            int err = lua_pcall(L, 0, 0, 0);
            if (err) {
                LOG_ERROR("[%s] exec fail: %s\n", "onClose", lua_tostring(L, -1));
            }
        }
        lua_settop(L, 0);

        g_dispatcher->quicktimer().SetInterval([this]() {
            lua_State* L = m_vm->l();
            bool allFinish = true;
            if (get_player_evt_handler(L, "checkIsAllFinish", this)) {
                int err = lua_pcall(L, 0, 1, 0);
                if (err) {
                    LOG_ERROR("[%s] exec fail: %s\n", "checkIsAllFinish", lua_tostring(L, -1));
                } else {
                    allFinish = lua_toboolean(L, -1);
                }
            }
            lua_settop(L, 0);

            if (allFinish) {
                gWorldMgr->RemovePlayer(this);
            }
        }, 500, m_autoObserver);
    }

    void PlayerSession::OnUserSessionSend(base::gateway::PacketOut& pktout)
    {
        gWorldMgr->OnSendPacket(pktout);
    }
}


