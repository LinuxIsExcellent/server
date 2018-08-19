#include "interface.h"
#include "battle/singlecombat.h"
#include "battle/mixedcombat.h"
#include <assert.h>
#include <base/3rd/rapidjson/document.h>
#include <base/3rd/rapidjson/writer.h>
#include <base/3rd/rapidjson/stringbuffer.h>
#include <base/logger.h>
#include <cmath>

namespace engine
{
    using namespace std;
    using namespace tpl;
    using namespace battle;

// init tpl (初始化模板)
    bool InitBattleTemplate(std::vector<engine::tpl::T_SpellNode>& skillNodes, std::vector<engine::tpl::T_SpellBase>& skillBases,
                            std::vector<engine::tpl::T_Buff>& buffs, std::vector<engine::tpl::T_Hero>& heroes)
    {
        // g_battleTploader.AddSkillNode(skillNodes);
        // g_battleTploader.AddSkillBase(skillBases);
        // g_battleTploader.AddBuff(buffs);
        // g_battleTploader.AddHero(heroes);
        return true;
    }

    bool InitBattleTemplate2(std::vector<tpl::T_Army>& armies, std::vector<tpl::T_ArmySpell>& armySpells, std::vector<tpl::T_AttackDiscount>& attackDiscount)
    {
        // g_battleTploader.AddArmy(armies);
        // g_battleTploader.AddArmySpell(armySpells);
        // g_battleTploader.AddAttackDiscount(attackDiscount); 
        return true;
    }

    bool InitBattleConfig(tpl::T_BattleConfig& battleConfig)
    {
        // g_battleTploader.SetBattleConfig(battleConfig);
        return true;
    }

    bool InitHeroPowerConfig(tpl::T_HeroPowerConfig& heroPowerConfig)
    {
        // g_battleTploader.SetHeroPowerConfig(heroPowerConfig);
        return true;
    }

    Combat::Combat(int randSeed, bool mustSingleCombat, bool isAutomatic, int levelType)
        :m_mustSingleCombat(mustSingleCombat),m_isAutomatic(isAutomatic), m_randSeed(randSeed), m_randEngine(randSeed), m_levelType(levelType)
    {
    }
    Combat::~Combat()
    {
        if (m_singleCombat != nullptr) {
            delete m_singleCombat;
            m_singleCombat = nullptr;
        }

        if (m_mixedCombat != nullptr) {
            delete m_mixedCombat;
            m_mixedCombat = nullptr;
        }
    }

    bool Combat::Init(InitialDataInput& attackInput, InitialDataInput& defenceInput)
    {
        m_attackInput = attackInput;
        m_defenceInput = defenceInput;

        if (m_attackInput.team.groups.empty() || m_defenceInput.team.groups.empty()) {
            return false;
        }
     
        /*auto getSingleHeroIndex = [this](I_Team & team) {
            int totalChallenge = 0;
            for (I_Group & group : team.groups) {
                if (group.hero.hp > 0) {
                    totalChallenge +=group.hero.challenge;
                }
            }
            

            int heroIndex = -1;
            if (totalChallenge == 0) {
                int temp = 0;
                // int rand = m_randEngine.RandBetween(1, totalChallenge);
                for (int i = 0; i < (int)team.groups.size(); ++i) {
                    if (team.groups[i].hero.hp > 0) {
                        temp +=  team.groups[i].hero.challenge;
                        if (temp >= rand) {
                            heroIndex = i;
                            break;
                        }
                    }
                    heroIndex = i;
                }
            }
            return heroIndex;
        };*/

        //获取指定的武将
       /* auto getSpecifiedHeroIndex = [this] (I_Team & team, int heroId) {
            int heroIndex = -1;
            for (int i = 0; i < (int)team.groups.size(); ++i) {
                if (heroId ==  team.groups[i].hero.id) {
                    heroIndex = i;
                    break;
                }
            }
            return heroIndex;
        };*/
        
        if (m_mustSingleCombat) {
            /*if (m_attackInput.singleCombatHero > 0) {
                attackChallengeHero = getSpecifiedHeroIndex(m_attackInput.team, m_attackInput.singleCombatHero);
            } else {
                attackChallengeHero = getSingleHeroIndex(m_attackInput.team);
            }
            
            if (m_defenceInput.singleCombatHero > 0) {
                defenceChallengeHero = getSpecifiedHeroIndex(m_defenceInput.team, m_defenceInput.singleCombatHero);
            } else {
                defenceChallengeHero = getSingleHeroIndex(m_defenceInput.team);
            }
            
            if (attackChallengeHero >= 0 && defenceChallengeHero >= 0) {
                m_attackHero = &m_attackInput.team.groups[attackChallengeHero].hero;
                m_defenceHero = &m_defenceInput.team.groups[defenceChallengeHero].hero;
                return true;
            }*/
            return true;
        } else if (m_levelType == 2) //混合战之中也有一定概率触发单挑
        {
            /*//进攻方单挑Hero
            attackChallengeHero = getSingleHeroIndex (m_attackInput.team);
            //防守方单挑Hero
            defenceChallengeHero = getSingleHeroIndex (m_defenceInput.team);
            if (attackChallengeHero >= 0 && defenceChallengeHero >= 0) {
                // 4.1.2判断是否单挑
                // 1）取己方a武将和敌方b武将的单挑值。
                // 2）单挑概率=16*（己方单挑值+敌方单挑值）/（16*（己方单挑值+敌方单挑值）+3000）
                // 3）通过单挑概率，服务器取随即值判断是否单挑
                m_attackHero = &m_attackInput.team.groups[attackChallengeHero].hero;
                m_defenceHero = &m_defenceInput.team.groups[defenceChallengeHero].hero;

                int triggerWeight = 16 * (m_attackHero->challenge + m_defenceHero->challenge);
                int totalWeight = (16 * (m_attackHero->challenge + m_defenceHero->challenge) + 3000) ;

                int rand = m_randEngine.RandBetween(1, totalWeight);
                //return true;
                if (triggerWeight >= rand) {
                    return true;
                }
            }*/
            return true;            
        }
        return false;
    }

    void Combat::OutputSingleResult(const engine::SingleCombatResult& singleResult)
    {
        for (I_Group & group : m_attackInput.team.groups) {
            int cnt = std::floor(group.army.armyCount * singleResult.attackLoseArmyPercent);
            group.army.armyCount = group.army.armyCount - cnt;
        }
        for (I_Group & group : m_defenceInput.team.groups) {
            int cnt = std::floor(group.army.armyCount * singleResult.defenseLostArmyPercent);
            group.army.armyCount = group.army.armyCount - cnt;
        }
    }

    // single combat (单挑)
    bool Combat::CreateSingleCombat()
    {
        assert(m_singleCombat == nullptr);
        /*if (m_attackInput.team && m_defenceInput.team) {*/
            m_singleCombat = new SingleCombat(m_randSeed + 1);
            m_singleCombat->SetAutomatic(m_isAutomatic);
            m_singleCombat->Init(m_attackInput.team, m_defenceInput.team);
            return true;
        /*}*/
        return false;
    }

    bool Combat::CreateMixedCombat()
    {
        if (m_attackInput.team.groups.empty() || m_defenceInput.team.groups.empty()) {
            return false;
        }
        WarReport::SoloData* attackSoloReport = nullptr;
        WarReport::SoloData* defenseSoloReport = nullptr;
        if (m_singleCombat)
        {
            attackSoloReport = &m_singleCombat->AttackSoloReport();
            defenseSoloReport = &m_singleCombat->DefenseSoloReport(); 

            attackSoloReport->uid = m_attackInput.uid;
            attackSoloReport->headId = m_attackInput.headIcon;
            attackSoloReport->nickName = m_attackInput.name;

            defenseSoloReport->uid = m_defenceInput.uid;
            defenseSoloReport->headId = m_defenceInput.headIcon;
            defenseSoloReport->nickName = m_defenceInput.name;
        }
        m_mixedCombat = new MixedCombat(m_randSeed + 2);
        m_mixedCombat->SetAutomatic(m_isAutomatic);
        if (m_isAutomatic) {
            m_mixedCombat->SetMaxRound(30); //default 30
        } else {
            m_mixedCombat->SetMaxRound(999);
        }
        m_mixedCombat->Init(m_attackInput.team, m_defenceInput.team, attackSoloReport, defenseSoloReport);
        return true;
    }

    bool Combat::StartSingleCombat()
    {
        if (m_singleCombat) {
            return m_singleCombat->Start();
        }
        return false;
    }

    bool Combat::StartMixedCombat()
    {
        if (m_mixedCombat) {
            return m_mixedCombat->Start();
        }
        return false;
    }

    std::string Combat::SerializeReportData(engine::WarReport& report, int reportId)
    {
        std::string jsonString;
        try {
            rapidjson::StringBuffer jsonbuffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(jsonbuffer);

            auto HeroArmySerialize = [&](engine::WarReport::HeroArmyData& data) {
                writer.StartObject();
                writer.String("attackType");
                writer.Int((int)data.attackType);
                writer.String("armyId");
                writer.Int(data.armyId);
                writer.String("heroId");
                writer.Int(data.heroId);
                writer.String("soldierId");
                writer.Int(data.soldierId);
                writer.String("hp");
                writer.Int(data.hp);
                writer.String("initPosition");
                writer.Int(data.initPosition);

                writer.String("power");
                writer.Int(data.power);
                writer.String("defense");
                writer.Int(data.defense);
                writer.String("wisdom");
                writer.Int(data.wisdom);
                writer.String("skill");
                writer.Int(data.skill);
                writer.String("agile");
                writer.Int(data.agile);
                writer.String("lucky");
                writer.Int(data.lucky);

                writer.EndObject();
            };

            auto HeroArmyDatasSerialize = [&](engine::WarReport::HeroArmyDatas& data) {
                writer.StartObject();

                writer.String("armyDataType");
                writer.Int((int)data.armyDataType);

                writer.String("heroArmyData");
                writer.StartArray();
                for (auto& heroArmyData : data.heroArmyData)
                {
                    HeroArmySerialize(heroArmyData);
                }
                writer.EndArray();

                writer.EndObject();
            };

            auto InitTeamDataSerialize = [&](engine::WarReport::InitTeamData& data) {
                writer.StartObject();

                writer.String("uid");
                writer.Int(data.uid);
                writer.String("headId");
                writer.Int(data.headId);
                writer.String("nickName");
                writer.String(data.nickName.c_str());
                writer.String("soloHeroId");
                writer.Int(data.soloHeroId);

                writer.EndObject();
            };

            auto ReceiverSerialize = [&](engine::WarReport::Receiver& data) {
                writer.StartObject();

                writer.String("armyId");
                writer.Int(data.armyId);
                writer.String("value");
                writer.Int(data.value);

                writer.EndObject();
            };

            auto BuffDataSerialize = [&](engine::WarReport::BuffData& data) {
                writer.StartObject();

                writer.String("buffType");
                writer.Int(data.buffType);

                writer.String("receiverRound");
                writer.Int(data.receiverRound);

                writer.String("keepRound");
                writer.Int(data.keepRound);

                writer.String("receivers");
                writer.StartArray();
                for (auto& receiver : data.receivers)
                {
                    ReceiverSerialize(receiver);
                }
                writer.EndArray();

                writer.EndObject();
            };

            auto SkillDataSerialize = [&](engine::WarReport::SkillData& data) {
                writer.StartObject();

                writer.String("skillId");
                writer.Int(data.skillId);

                writer.String("receivers");
                writer.StartArray();
                for (auto& receiver : data.receivers) {
                    ReceiverSerialize(receiver);
                }
                writer.EndArray();

                writer.String("buffs");
                writer.StartArray();
                for (auto& buff : data.buffs) {
                    BuffDataSerialize(buff);
                }
                writer.EndArray();
                
                writer.EndObject();
            };

            auto UnitRoundSerialize = [&](engine::WarReport::UnitRound& data) {
                writer.StartObject();

                writer.String("armyId");
                writer.Int(data.armyId);

                writer.String("targetId");
                writer.Int(data.targetId);

                writer.String("castType");
                writer.Int((int)data.castType);

                writer.String("skill");
                SkillDataSerialize(data.skill);

                writer.String("changeHp");
                writer.Int(data.changeHp);

                writer.String("changeBatVal");
                writer.Int(data.changeBatVal);

                writer.String("changeRageVal");
                writer.Int(data.changeRageVal);

                writer.String("crit");
                writer.Int(data.crit);
               
                writer.EndObject();
            };

            auto BigRoundSerialize = [&](engine::WarReport::BigRound& data) {
                writer.StartObject();

                writer.String("bigRoundId");
                writer.Int(data.bigRoundId);

                writer.String("complexRounds");
                writer.StartArray();
                for (auto unitRound : data.complexRounds)
                {
                    UnitRoundSerialize(unitRound);
                }
                writer.EndArray();
                
                writer.String("allUnitRounds");
                writer.StartArray();
                for (auto unitRound : data.allUnitRounds)
                {
                    UnitRoundSerialize(unitRound);
                }
                writer.EndArray();

                writer.EndObject();
            };

            auto ResultDataSerialize = [&](engine::WarReport::ResultData& data) {
                writer.StartObject();

                writer.String("win");
                writer.Int((int)data.win);

                writer.EndObject();
            };

            auto SoloRoundDataSerialize = [&](engine::WarReport::SoloRound& data) {
                writer.StartObject();

                writer.String("action");
                writer.Int((int)data.action);

                writer.String("rageChange");
                writer.Int(data.rageChange);

                writer.String("hpChange");
                writer.Int(data.hpChange);

                writer.String("result");
                writer.Int((int)data.result);

                writer.EndObject();
            };

            auto SoloDataSerialize = [&](engine::WarReport::SoloData& data) {
                writer.StartObject();

                writer.String("attackType");
                writer.Int((int)data.attackType);

                writer.String("uid");
                writer.Int(data.uid);

                writer.String("headId");
                writer.Int(data.headId);

                writer.String("nickName");
                writer.String(data.nickName.c_str());

                writer.String("Id");
                writer.Int(data.Id);

                writer.String("hp");
                writer.Int(data.hp);

                writer.String("rage");
                writer.Int(data.rage);

                writer.String("power");
                writer.Int(data.power);

                writer.String("rounds");
                writer.StartArray();
                for (auto rounds : data.rounds)
                {
                    SoloRoundDataSerialize(rounds);
                }
                writer.EndArray();

                writer.String("result");
                writer.Int((int)data.result);

                writer.String("loseArmysPercent");
                writer.Double(data.loseArmysPercent);

                writer.EndObject();
            };

            auto ArenaDataSerialize = [&](engine::WarReport::ArenaData& data)
            {
                writer.StartObject();

                writer.String("roundId");
                writer.Int(data.roundId);

                writer.String("datas");
                writer.StartArray();
                for (auto solodata : data.datas)
                {
                    SoloDataSerialize(solodata);   
                }
                writer.EndArray();

                writer.EndObject();
            };

            writer.StartObject();

            writer.String("battleId");
            writer.Int(reportId);
            
            writer.String("initTeamDatas");
            writer.StartArray();
            for (auto& initTeamData : report.initTeamDatas)
            {
                InitTeamDataSerialize(initTeamData);
            }
            writer.EndArray();

            writer.String("trapIds");
            writer.StartArray();

            for (auto trapId : report.trapIds)
            {
                writer.Int(trapId);
            }
            writer.EndArray();

            writer.String("heroArmyDatas");
            writer.StartArray();
            for (auto heroArmyData : report.heroArmyDatas)
            {
                HeroArmyDatasSerialize(heroArmyData);
            }
            writer.EndArray();

            writer.String("allBigRounds");
            writer.StartArray();
            for (auto allBigRound : report.allBigRounds)
            {
                BigRoundSerialize(allBigRound);
            }
            writer.EndArray();

            writer.String("result");
            ResultDataSerialize(report.result);

            writer.String("soloDatas");
            writer.StartArray();
            for (auto soloData : report.soloDatas)
            {
                SoloDataSerialize(soloData);
            }
            writer.EndArray();

            writer.String("arenaDatas");
            writer.StartArray();
            for (auto arenaData : report.arenaDatas)
            {
                ArenaDataSerialize(arenaData);
            }
            writer.EndArray();

            writer.EndObject();
            jsonString = jsonbuffer.GetString();
        }
        catch (std::exception& ex) {
        LOG_ERROR("Serialize ReportData json fail: %s\n", ex.what());
        }
        return jsonString;
    }
    
}

