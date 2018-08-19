#include "heroinfo.h"
#include <base/logger.h>
#include <model/tpl/templateloader.h>
#include "model/tpl/configure.h"
#include "model/tpl/battlearrttransform.h"
#include "model/tpl/army.h"

namespace ms
{
    namespace map
    {
        using namespace std;
        using namespace model;
        using namespace model::tpl;
        using namespace tpl;

        HeroInfo::HeroInfo(int tplId, int level, int star)
            : m_tplid(tplId), m_level(level), m_star(star)
        {
            m_heroTpl = model::tpl::g_tploader->FindNHero(m_tplid);
        }

        void HeroInfo::InipProp(int armyType /* = 0*/, int level /* = 0*/)
        {


            const std::vector<HeroStarLevelTpl*>* heroStarTpl = g_tploader->FindHeroStarLevel(m_tplid);
            const std::vector<HeroLevelAttrTpl*>* heroLevelTpl = g_tploader->FindHeroLevelAttr(m_tplid);

            float PowerUp = 0.0;
            float DefenseUp = 0.0;
            float WisdomUp = 0.0;
            float SkillUp = 0.0;
            float AgileUp = 0.0;
            float LuckyUp = 0.0;
            float LifeUp = 0.0;
            int TroopsUp = 0;
            int SolohpUp = 0;
            if (heroStarTpl)
            {
                for (auto tpl : *heroStarTpl)
                {
                    if (tpl->level <= m_star)
                    {
                        PowerUp = PowerUp + tpl->PowerAdd;
                        DefenseUp = DefenseUp + tpl->DefenseAdd;
                        WisdomUp = WisdomUp + tpl->WisdomAdd;
                        SkillUp = SkillUp + tpl->SkillAdd;
                        AgileUp = AgileUp + tpl->AgileAdd;
                        LuckyUp = LuckyUp + tpl->LuckyAdd;
                        LifeUp = LifeUp + tpl->LifeAdd;
                        TroopsUp = TroopsUp + tpl->TroopsAdd;
                        SolohpUp = SolohpUp + tpl->SolohpAdd;
                    }
                }
            }

            if (heroLevelTpl)
            {
                for (auto tpl : *heroLevelTpl)
                {
                    if (m_level > tpl->levelmax)
                    {
                        PowerUp = PowerUp + tpl->nGrowthPower * (tpl->levelmax - tpl->levelmin - 1);
                        DefenseUp = DefenseUp + tpl->nGrowthDefense * (tpl->levelmax - tpl->levelmin - 1);
                        WisdomUp = WisdomUp + tpl->nGrowthWisdom * (tpl->levelmax - tpl->levelmin - 1);
                        SkillUp = SkillUp + tpl->nGrowthSkill * (tpl->levelmax - tpl->levelmin - 1);
                        AgileUp = AgileUp + tpl->nGrowthAgile * (tpl->levelmax - tpl->levelmin - 1);
                        LuckyUp = LuckyUp + tpl->nGrowthLucky * (tpl->levelmax - tpl->levelmin - 1);
                        LifeUp = LifeUp + tpl->nGrowthLife * (tpl->levelmax - tpl->levelmin - 1);
                        TroopsUp = TroopsUp + tpl->nGrowthTroops * (tpl->levelmax - tpl->levelmin - 1);
                        SolohpUp = SolohpUp + tpl->nGrowthSolohp * (tpl->levelmax - tpl->levelmin - 1);
                        continue;
                    }
                    if (m_level >= tpl->levelmin)
                    {
                        PowerUp = PowerUp + tpl->nGrowthPower * (m_level - tpl->levelmin - 1);
                        DefenseUp = DefenseUp + tpl->nGrowthDefense * (m_level - tpl->levelmin - 1);
                        WisdomUp = WisdomUp + tpl->nGrowthWisdom * (m_level - tpl->levelmin - 1);
                        SkillUp = SkillUp + tpl->nGrowthSkill * (m_level - tpl->levelmin - 1);
                        AgileUp = AgileUp + tpl->nGrowthAgile * (m_level - tpl->levelmin - 1);
                        LuckyUp = LuckyUp + tpl->nGrowthLucky * (m_level - tpl->levelmin - 1);
                        LifeUp = LifeUp + tpl->nGrowthLife * (m_level - tpl->levelmin - 1);
                        TroopsUp = TroopsUp + tpl->nGrowthTroops * (m_level - tpl->levelmin - 1);
                        SolohpUp = SolohpUp + tpl->nGrowthSolohp * (m_level - tpl->levelmin - 1);
                        continue;
                    }
                }    
            }
            

            //命魂属性相 + 加上基础属性 (5001,5002,5003,5004,5005,5006,5007,5018,5019)
            m_firstAttrMap.clear();
            hero_soul_map_t heroSoulMap = g_tploader->GetHeroSoul();
            for (auto it = heroSoulMap.begin(); it != heroSoulMap.end(); ++it)
            {
                if (it->second->nDamageType == m_heroTpl->damageType && it->second->soulLevel <= m_soul)
                {
                    float base,pct,ext;
                    if (it->second->addType == AttributeAdditionType::BASE)
                    {
                        base = it->second->value;
                    }
                    else if (it->second->addType == AttributeAdditionType::PCT)
                    {
                        pct = it->second->value;
                    }
                    else if (it->second->addType == AttributeAdditionType::EXT)
                    {
                        ext = it->second->value;
                    }
                    AddProp(it->second->type, base, pct, ext);
                }
            }

            auto getProp = [this](model::AttributeType type, float base) {
                float value = (GetBaseAttr(type, AttributeAdditionType::BASE) + base) * (1 +GetBaseAttr(type, AttributeAdditionType::PCT)/10000.0) +GetBaseAttr(type, AttributeAdditionType::EXT);
                m_firstAttrMap.emplace((int)type, value);                
                return value;
            };

            // 二级属性转换
            auto transformAttr = [&](model::AttributeType type) {
                const BattleArrtTransformTpl* tpl = g_tploader->FindBattleArrtTransform((int)type);
                float value = 0.0;
                do {
                    if (!tpl)
                    {
                     break;
                    }
                    auto it = m_firstAttrMap.find(tpl->firstArrt);
                    if (it == m_firstAttrMap.end())
                    {
                        break;
                    }
                    float base = 0;
                    float pct = 0;
                    float ext = 0;
                    // 加上兵种属性加成
                    const NArmyTpl* armyTpl = g_tploader->FindNArmy(armyType, level);
                    if (armyTpl)
                    {
                        for (auto& attr : armyTpl->attrList)
                        {
                            if (attr.type == (int)type)
                            {
                                switch(attr.addType) {
                                    case 1: {
                                        base = base + attr.value;
                                    }
                                    break;
                                    case 2: {
                                        pct = pct + attr.value;
                                    }
                                    break;
                                    case 3: {
                                        ext = ext + attr.value;
                                    }   
                                    break;
                                    default:
                                          LOG_ERROR("HeroInfo::OnPropertyUpdate attr.type=%d,attr.addType=%d,armyType=%d,armyLevel=%d", attr.type, attr.addType, armyType, level);  
                                    break;
                                }
                            }
                        }
                    }
                    value = (it->second * tpl->transformNum + base) * (1 + pct) + ext; 
                }while(0);
                return value;
            };

            if (m_heroTpl) {
                m_heroPower = getProp(AttributeType::HERO_POWER, m_heroTpl->power + PowerUp);
                m_heroDefense = getProp(AttributeType::HERO_DEFENSE, m_heroTpl->defense + DefenseUp);
                m_heroWisdom = getProp(AttributeType::HERO_WISDOM, m_heroTpl->wisdom + WisdomUp);
                m_heroLucky = getProp(AttributeType::HERO_LUCKY, m_heroTpl->lucky + LuckyUp);
                m_heroSkill = getProp(AttributeType::HERO_SKILL, m_heroTpl->skill + SkillUp);
                m_heroAgile = getProp(AttributeType::HERO_AGILE, m_heroTpl->agile + AgileUp);
                m_heroLife = getProp(AttributeType::HERO_LIFE, m_heroTpl->life + LifeUp);
                m_heroPhysicalPower = transformAttr(AttributeType::HERO_PHYSICAL_ATTACK);
                m_heroPhysicalDefense = transformAttr(AttributeType::HERO_PHYSICAL_DEFENSE);
                m_heroSkillPower = transformAttr(AttributeType::HERO_WISDOM_ATTACK);
                m_heroSkillDefense = transformAttr(AttributeType::HERO_WISDOM_DEFENSE);
                m_heroHit = transformAttr(AttributeType::HERO_HIT);
                m_heroAvoid = transformAttr(AttributeType::HERO_AVOID);
                m_heroCritHit = transformAttr(AttributeType::HERO_CRIT_HIT);
                m_heroCritAvoid = transformAttr(AttributeType::HERO_CRIT_AVOID);
                m_heroSpeed = transformAttr(AttributeType::HERO_SPEED);
                m_heroCityLife = transformAttr(AttributeType::HERO_CITY_LIFE);
                m_heroSolohp = (int)getProp(AttributeType::HERO_SINGLE_ENERGY, m_heroTpl->soloHp + SolohpUp);
                m_physical = m_heroSolohp;
                m_heroTroops = (int)getProp(AttributeType::HERO_FIGHT, m_heroTpl->troops + TroopsUp);

                /*std::cout << "###init_heroinfo..." << std::endl;

                std::cout << "m_heroPower = " << m_heroPower << std::endl; 
                std::cout << "m_heroDefense = " << m_heroDefense << std::endl; 
                std::cout << "m_heroWisdom = " << m_heroWisdom << std::endl; 
                std::cout << "m_heroLucky = " << m_heroLucky << std::endl; 
                std::cout << "m_heroSkill = " << m_heroSkill << std::endl; 
                std::cout << "m_heroAgile = " << m_heroAgile << std::endl; 
                std::cout << "m_heroLife = " << m_heroLife << std::endl; 
                std::cout << "m_heroPhysicalPower = " << m_heroPhysicalPower << std::endl; 
                std::cout << "m_heroPhysicalDefense = " << m_heroPhysicalDefense << std::endl; 
                std::cout << "m_heroSkillPower = " << m_heroSkillPower << std::endl; 
                std::cout << "m_heroSkillDefense = " << m_heroSkillDefense << std::endl; 
                std::cout << "m_heroHit = " << m_heroHit << std::endl; 
                std::cout << "m_heroAvoid = " << m_heroAvoid << std::endl; 
                std::cout << "m_heroCritHit = " << m_heroCritHit << std::endl; 
                std::cout << "m_heroCritAvoid = " << m_heroCritAvoid << std::endl; 
                std::cout << "m_heroSpeed = " << m_heroSpeed << std::endl; 
                std::cout << "m_heroCityLife = " << m_heroCityLife << std::endl; 
                std::cout << "m_heroSolohp = " << m_heroSolohp << std::endl; 
                std::cout << "m_heroTroops = " << m_heroTroops << std::endl;*/ 

                UpdatePower();
            }
        }

        void HeroInfo::SetProp(model::AttributeType type, float base, float pct, float ext)
        {
            SetBaseAttr(type, base, model::AttributeAdditionType::BASE);
            SetBaseAttr(type, pct, model::AttributeAdditionType::PCT);
            SetBaseAttr(type, ext, model::AttributeAdditionType::EXT);
        }

        void HeroInfo::AddProp(model::AttributeType type, float base, float pct, float ext)
        {
            AddBaseAttr(type, base, model::AttributeAdditionType::BASE);
            AddBaseAttr(type, pct, model::AttributeAdditionType::PCT);
            AddBaseAttr(type, ext, model::AttributeAdditionType::EXT);
        }

        float HeroInfo::GetProp(model::AttributeType type) const
        {
            return GetBaseAttr(type, AttributeAdditionType::BASE) * (1 +GetBaseAttr(type, AttributeAdditionType::PCT)/10000.0) +GetBaseAttr(type, AttributeAdditionType::EXT);
        }

        float HeroInfo::GetBaseAttr(model::AttributeType type, model::AttributeAdditionType addType) const
        {
            int index = GetIndex(type);
            if (index >= 0 && index < kHeroAttrCount) {
                return m_attrBase[index][(int)addType];
            }
            return 0;
        }

        void HeroInfo::SetBaseAttr(model::AttributeType type, float value, model::AttributeAdditionType addType)
        {
            int index = GetIndex(type);
            if (index >= 0 && index < kHeroAttrCount) {
                m_attrBase[index][(int)addType] = value;
            }
        }

        void HeroInfo::AddBaseAttr(model::AttributeType type, float value, model::AttributeAdditionType addType)
        {
            int index = GetIndex(type);
            if (index >= 0 && index < kHeroAttrCount) {
                m_attrBase[index][(int)addType] = m_attrBase[index][(int)addType] + value;
            }
        }
        
        void HeroInfo::OnPropertyUpdate(const info::Property& property, int armyType, int level)
        {
            m_firstAttrMap.clear();
            auto getProp = [this](model::AttributeType type, float base, float pct, float ext) {
                float value = (GetBaseAttr(type, AttributeAdditionType::BASE) + base) * (1 +GetBaseAttr(type, AttributeAdditionType::PCT)/10000.0 + pct) +GetBaseAttr(type, AttributeAdditionType::EXT) + ext;
                m_firstAttrMap.emplace((int)type, value);
                return value;
            };
            // 二级属性转换
            auto transformAttr = [&](model::AttributeType type, float base, float pct, float ext) {
                const BattleArrtTransformTpl* tpl = g_tploader->FindBattleArrtTransform((int)type);
                float value = 0.0;
                do {
                    if (!tpl)
                    {
                     break;
                    }
                    auto it = m_firstAttrMap.find(tpl->firstArrt);
                    if (it == m_firstAttrMap.end())
                    {
                        break;
                    }
                    // 加上兵种属性加成
                    const NArmyTpl* armyTpl = g_tploader->FindNArmy(armyType, level);
                    if (armyTpl)
                    {
                        // 兵种科技属性加成
                        if (type == model::AttributeType::HERO_PHYSICAL_ATTACK || type == model::AttributeType::HERO_WISDOM_ATTACK)
                        {
                            property.armyAttackAddition(static_cast<model::ArmysType>(armyType), base, pct, ext);
                        }
                        else if (type == model::AttributeType::HERO_PHYSICAL_DEFENSE || type == model::AttributeType::HERO_WISDOM_DEFENSE)
                        {
                            property.armyDefenseAddition(static_cast<model::ArmysType>(armyType), base, pct, ext);
                        }

                        for (auto& attr : armyTpl->attrList)
                        {
                            if (attr.type == (int)type)
                            {
                                switch(attr.addType) {
                                    case 1: {
                                        base = base + attr.value;
                                    }
                                    break;
                                    case 2: {
                                        pct = pct + attr.value;
                                    }
                                    break;
                                    case 3: {
                                        ext = ext + attr.value;
                                    }   
                                    break;
                                    default:
                                          LOG_ERROR("HeroInfo::OnPropertyUpdate attr.type=%d,attr.addType=%d,armyType=%d,armyLevel=%d", attr.type, attr.addType, armyType, level);  
                                    break;
                                }
                            }
                        }
                    }
                    value = (it->second * tpl->transformNum + base) * (1 + pct) + ext; 
                }while(0);
                return value;
            };

            if (m_heroTpl) {
                m_heroPower = getProp(AttributeType::HERO_POWER, property.heroPowerBase, property.heroPowerPct, property.heroPowerExt);
                m_heroDefense = getProp(AttributeType::HERO_DEFENSE, property.heroDefenseBase, property.heroDefensePct, property.heroDefenseExt);
                m_heroWisdom = getProp(AttributeType::HERO_WISDOM, property.heroWisdomBase, property.heroWisdomPct, property.heroWisdomExt);
                m_heroLucky = getProp(AttributeType::HERO_LUCKY, property.heroLuckyBase, property.heroLuckyPct, property.heroLuckyExt);
                m_heroSkill = getProp(AttributeType::HERO_SKILL, property.heroSkillBase, property.heroSkillPct, property.heroSkillExt);
                m_heroAgile = getProp(AttributeType::HERO_AGILE, property.heroAgileBase, property.heroAgilePct, property.heroAgileExt);
                m_heroLife = getProp(AttributeType::HERO_LIFE, property.heroLifeBase, property.heroLifePct, property.heroLifeExt);
                m_heroPhysicalPower = transformAttr(AttributeType::HERO_PHYSICAL_ATTACK, property.heroPhysicalAttackBase, property.heroPhysicalAttackPct, property.heroPhysicalAttackExt);
                m_heroPhysicalDefense = transformAttr(AttributeType::HERO_PHYSICAL_DEFENSE, property.heroPhysicalDefenseBase, property.heroPhysicalDefensePct, property.heroPhysicalDefenseExt);
                m_heroSkillPower = transformAttr(AttributeType::HERO_WISDOM_ATTACK, property.heroWisdomAttackBase, property.heroWisdomAttackPct, property.heroWisdomAttackExt);
                m_heroSkillDefense = transformAttr(AttributeType::HERO_WISDOM_DEFENSE, property.heroWisdomDefenseBase, property.heroWisdomDefensePct, property.heroWisdomDefenseExt);
                m_heroHit = transformAttr(AttributeType::HERO_HIT, property.heroHitBase, property.heroHitPct, property.heroHitExt);
                m_heroAvoid = transformAttr(AttributeType::HERO_AVOID, property.heroAvoidBase, property.heroAvoidPct, property.heroAvoidExt);
                m_heroCritHit = transformAttr(AttributeType::HERO_CRIT_HIT, property.heroCritHitBase, property.heroCritHitPct, property.heroCritHitExt);
                m_heroCritAvoid = transformAttr(AttributeType::HERO_CRIT_AVOID, property.heroCritAvoidBase, property.heroCritAvoidPct, property.heroCritAvoidExt);
                m_heroSpeed = transformAttr(AttributeType::HERO_SPEED, property.heroSpeedBase, property.heroSpeedPct, property.heroSpeedExt);
                m_heroCityLife = transformAttr(AttributeType::HERO_CITY_LIFE, property.heroCityLifeBase, property.heroCityLifePct, property.heroCityLifeExt);
                m_heroSolohp = (int)getProp(AttributeType::HERO_SINGLE_ENERGY, property.heroSingleEnergyBase, property.heroSingleEnergyPct, property.heroSingleEnergyExt);
                m_heroTroops = (int)getProp(AttributeType::HERO_FIGHT, property.heroFightBase, property.heroFightPct, property.heroFightExt);

                /*std::cout << "###update_heroinfo..." << std::endl;

                std::cout << "m_heroPower = " << m_heroPower << std::endl; 
                std::cout << "m_heroDefense = " << m_heroDefense << std::endl; 
                std::cout << "m_heroWisdom = " << m_heroWisdom << std::endl; 
                std::cout << "m_heroLucky = " << m_heroLucky << std::endl; 
                std::cout << "m_heroSkill = " << m_heroSkill << std::endl; 
                std::cout << "m_heroAgile = " << m_heroAgile << std::endl; 
                std::cout << "m_heroLife = " << m_heroLife << std::endl; 
                std::cout << "m_heroPhysicalPower = " << m_heroPhysicalPower << std::endl; 
                std::cout << "m_heroPhysicalDefense = " << m_heroPhysicalDefense << std::endl; 
                std::cout << "m_heroSkillPower = " << m_heroSkillPower << std::endl; 
                std::cout << "m_heroSkillDefense = " << m_heroSkillDefense << std::endl; 
                std::cout << "m_heroHit = " << m_heroHit << std::endl; 
                std::cout << "m_heroAvoid = " << m_heroAvoid << std::endl; 
                std::cout << "m_heroCritHit = " << m_heroCritHit << std::endl; 
                std::cout << "m_heroCritAvoid = " << m_heroCritAvoid << std::endl; 
                std::cout << "m_heroSpeed = " << m_heroSpeed << std::endl; 
                std::cout << "m_heroCityLife = " << m_heroCityLife << std::endl; 
                std::cout << "m_heroSolohp = " << m_heroSolohp << std::endl; 
                std::cout << "m_heroTroops = " << m_heroTroops << std::endl; */

                UpdatePower();
            }
        }

        void HeroInfo::UpdatePower()
        {
            m_power = m_heroPhysicalPower * g_tploader->FindBattleArrt(AttributeType::HERO_PHYSICAL_ATTACK)->battlePower
             + m_heroPhysicalDefense * g_tploader->FindBattleArrt(AttributeType::HERO_PHYSICAL_DEFENSE)->battlePower
             + m_heroSkillPower * g_tploader->FindBattleArrt(AttributeType::HERO_WISDOM_ATTACK)->battlePower
             + m_heroSkillDefense * g_tploader->FindBattleArrt(AttributeType::HERO_WISDOM_DEFENSE)->battlePower
             + m_heroHit * g_tploader->FindBattleArrt(AttributeType::HERO_HIT)->battlePower
             + m_heroAvoid * g_tploader->FindBattleArrt(AttributeType::HERO_AVOID)->battlePower
             + m_heroCritHit * g_tploader->FindBattleArrt(AttributeType::HERO_CRIT_HIT)->battlePower
             + m_heroCritAvoid * g_tploader->FindBattleArrt(AttributeType::HERO_CRIT_AVOID)->battlePower
             + m_heroSpeed * g_tploader->FindBattleArrt(AttributeType::HERO_SPEED)->battlePower
             + m_heroCityLife * g_tploader->FindBattleArrt(AttributeType::HERO_CITY_LIFE)->battlePower;
             // std::cout << "m_power = " << m_power << std::endl;
        }

        int HeroInfo::GetIndex(model::AttributeType type) const
        {
            int index = 0;
            switch (type) {
                case AttributeType::HERO_POWER:
                    index = 1;
                    break;
                case AttributeType::HERO_DEFENSE:
                    index = 2;
                    break;
                case AttributeType::HERO_WISDOM:
                    index = 3;
                    break;
                case AttributeType::HERO_SKILL:
                    index = 4;
                    break;
                case AttributeType::HERO_AGILE:
                    index = 5;
                    break;
                case AttributeType::HERO_LUCKY:
                    index = 6;
                    break;
                case AttributeType::HERO_LIFE:
                    index =7;
                    break;
                case AttributeType::HERO_FIGHT:
                    index =8;
                    break;
                case AttributeType::HERO_SINGLE_ENERGY:
                    index = 9;
                    break;
                default:
                    break;
            }
            return index;
        }

        void HeroInfo::Serialize(rapidjson::Writer< rapidjson::StringBuffer >& writer) const
        {
            writer.StartObject();
            writer.Key("tplId");
            writer.Int(tplId());
            writer.Key("level");
            writer.Int(level());
            writer.Key("star");
            writer.Int(star());
            writer.Key("soul");
            writer.Int(soul());

            writer.Key("attrBase");
            writer.StartArray();
            for (int i = 0; i < kHeroAttrCount; ++i) {
                writer.StartArray();
                writer.Double(m_attrBase[i][(int)AttributeAdditionType::BASE]);
                writer.Double(m_attrBase[i][(int)AttributeAdditionType::PCT]);
                writer.Double(m_attrBase[i][(int)AttributeAdditionType::EXT]);
                writer.EndArray();
            }
            writer.EndArray();

            writer.Key("skill");
            writer.StartArray();
            for (auto & value : skill()) {
                writer.StartObject();
                writer.Key("id");
                writer.Int(value.id);
                writer.Key("level");
                writer.Int(value.level);
                writer.EndObject();
            }
            writer.EndArray();

            writer.EndObject();
        }

        void HeroInfo::Desrialize(rapidjson::GenericValue< rapidjson::UTF8< char > >& hero)
        {
            int tplId = hero["tplId"].GetInt();
            int level = hero["level"].GetInt();
            int star = hero["star"].GetInt();
            int soul = hero["soul"].GetInt();

            auto& attrBase = hero["attrBase"];
            for (size_t i = 0; i < attrBase.Size() && i < kHeroAttrCount; ++i) {
                m_attrBase[i][(int)AttributeAdditionType::BASE] = attrBase[i][0].GetDouble();
                m_attrBase[i][(int)AttributeAdditionType::PCT] = attrBase[i][1].GetDouble();
                m_attrBase[i][(int)AttributeAdditionType::EXT] = attrBase[i][2].GetDouble();
            }

            std::vector<HeroSkill> skill;
            auto& skillArray = hero["skill"];
            for (size_t i = 0; i < skillArray.Size(); ++i) {
                skill.emplace_back(skillArray[i]["id"].GetInt(), skillArray[i]["level"].GetInt());
            }

            m_tplid = tplId;
            m_heroTpl = model::tpl::g_tploader->FindNHero(m_tplid);
            m_level = level;
            m_star = star;
            m_soul = soul;
            SetSkill(skill);
        }
    }
}