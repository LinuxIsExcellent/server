#include "modulebattle.h"
#include <base/configurehelper.h>
#include <model/tpl/templateloader.h>
#include <model/tpl/army.h>
#include <model/tpl/hero.h>
#include <model/tpl/configure.h>
#include "interface.h"
#include "tpl/t_spell.h"
#include "tpl/t_buff.h"
#include "tpl/t_hero.h"
#include "tpl/t_army.h"

namespace ms
{
    namespace map
    {
        ModuleBattle* ModuleBattle::Create()
        {
            ModuleBattle* obj = new ModuleBattle();
            obj->AutoRelease();
            return obj;
        }

        ModuleBattle::ModuleBattle() : ModuleBase("ms.battle")
        {
            AddDependentModule("model.tploader");
        }

        ModuleBattle::~ModuleBattle()
        {
        }

        void ModuleBattle::OnModuleSetup()
        {
            // //初始化战斗模板表
            // std::vector<engine::tpl::T_SpellNode> skillNodes;
            // std::vector<engine::tpl::T_SpellBase> skillBases;
            // std::vector<engine::tpl::T_Buff> buffs;
            // std::vector<engine::tpl::T_Hero> heroes;
            // std::vector<engine::tpl::T_Army> armies;
            // std::vector<engine::tpl::T_ArmySpell> armySpells;
            // std::vector<engine::tpl::T_AttackDiscount> attackDiscount;

            // const model::tpl::skillnode_map_t& skillNodesMap = model::tpl::g_tploader->skillnode();
            // const model::tpl::skillbase_map_t& skillBasesMap = model::tpl::g_tploader->skillbase();
            // const model::tpl::buff_map_t& buffsMap = model::tpl::g_tploader->buff();
            // const model::tpl::hero_map_t& heroMap = model::tpl::g_tploader->heroes();
            // const model::tpl::armys_map_t& armysMap = model::tpl::g_tploader->armies();
            // const model::tpl::attack_discount_map_t& attackDiscountMap = model::tpl::g_tploader->attackDiscount();
            // const model::tpl::army_spell_map_t& armySpellMap = model::tpl::g_tploader->armySpell();

            // for (auto & node : skillNodesMap) {
            //     auto nodeValue = node.second;
            //     engine::tpl::T_SpellNode tSkillNode;
            //     tSkillNode.id = nodeValue->id;
            //     tSkillNode.typeExt = nodeValue->typeExt;
            //     tSkillNode.buffIds = nodeValue->buffIds;
            //     tSkillNode.rage = nodeValue->rage;
            //     skillNodes.push_back(tSkillNode);
            // }

            // for (auto & base : skillBasesMap) {
            //     auto baseValue = base.second;
            //     engine::tpl::T_SpellBase tSkillBase;
            //     tSkillBase.id = baseValue->id;
            //     tSkillBase.singleNodeId = baseValue->singleNodeId;
            //     tSkillBase.mixNodeId = baseValue->mixNodeId;
            //     tSkillBase.active = baseValue->active;
            //     tSkillBase.super = baseValue->super;
            //     tSkillBase.weight = baseValue->weight;
            //     skillBases.push_back(tSkillBase);
            // }

            // for (auto & buff : buffsMap) {
            //     auto buffValue = buff.second;
            //     engine::tpl::T_Buff tBuff;
            //     tBuff.id = buffValue->id;
            //     tBuff.target = (engine::BuffTarget)buffValue->target;
            //     tBuff.type = (engine::BuffType)buffValue->type;
            //     tBuff.valueType = (engine::BuffValueType)buffValue->valueType;
            //     tBuff.value = buffValue->value;
            //     tBuff.rounds = buffValue->rounds;
            //     tBuff.coefficients = buffValue->coefficients;
            //     tBuff.probability = buffValue->probability;
            //     tBuff.isDebuff = buffValue->isDebuff;
            //     tBuff.buffs = buffValue->buffs;
            //     buffs.push_back(tBuff);
            // }

            // for (auto & hero : heroMap) {
            //     auto heroValue = hero.second;
            //     engine::tpl::T_Hero tHero;
            //     tHero.id = heroValue->id;
            //     tHero.sex = heroValue->sex;
            //     tHero.office = heroValue->office;
            //     tHero.hp = heroValue->hp;
            //     tHero.attack = heroValue->attack;
            //     tHero.defense = heroValue->defense;
            //     tHero.strategy = heroValue->strategy;
            //     tHero.speed = heroValue->speed;
            //     tHero.rage = heroValue->rage;
            //     tHero.hpUp = heroValue->hpUp;
            //     tHero.attackUp = heroValue->attackUp;
            //     tHero.defenseUp = heroValue->defenseUp;
            //     tHero.strategyup = heroValue->strategyup;
            //     tHero.speedUp = heroValue->speedUp;
            //     tHero.singleSkillCalcul = heroValue->singleSkillCalcul;
            //     heroes.push_back(tHero);
            // }

            // for (auto &army : armysMap) {
            //     auto armyValue = army.second;
            //     engine::tpl::T_Army tArmy;
            //     tArmy.id = armyValue->id;
            //     tArmy.mainType = armyValue->mainType;
            //     tArmy.subType = armyValue->subType;
            //     tArmy.level = armyValue->level;
            //     tArmy.hp = armyValue->hp;
            //     tArmy.attack = armyValue->attack;
            //     tArmy.defense = armyValue->defense;
            //     tArmy.speed = armyValue->speed;

            //     tArmy.attackRange = armyValue->attackRange;
            //     tArmy.power = armyValue->power;

            //     for (auto &effect : armyValue->armyEffects) {
            //         tArmy.armyEffects.emplace_back(effect.armyType, effect.percentage);
            //     }
            //     tArmy.armyAttrList = armyValue->armyAttrList;

            //     armies.push_back(tArmy);
            // }

            // for (auto & armySpell : armySpellMap) {
            //     auto armySpellValue = armySpell.second;
            //     engine::tpl::T_ArmySpell tArmySpell;
            //     tArmySpell.id = armySpellValue->id;
            //     tArmySpell.buffIds = armySpellValue->buffIds;
            //     tArmySpell.probability = armySpellValue->probability;
            //     armySpells.push_back(tArmySpell);
            // }

            // for (auto & discount : attackDiscountMap) {
            //     auto discountlValue = discount.second;
            //     engine::tpl::T_AttackDiscount tAttackDiscount;
            //     tAttackDiscount.target = (engine::BuffTarget)discountlValue->target;
            //     tAttackDiscount.discount = discountlValue->discount;
            //     attackDiscount.push_back(tAttackDiscount);
            // }

            // engine::InitBattleTemplate(skillNodes, skillBases, buffs, heroes);
            // engine::InitBattleTemplate2(armies, armySpells, attackDiscount);

            // auto& battleConfig = model::tpl::g_tploader->configure().battleConfig;
            // engine::tpl::T_BattleConfig tBattleConfig;
            // tBattleConfig.wuli = battleConfig.wuli;
            // tBattleConfig.zhili = battleConfig.zhili;
            // tBattleConfig.fangyu = battleConfig.fangyu;
            // tBattleConfig.moulue = battleConfig.moulue;
            // tBattleConfig.xueliang = battleConfig.xueliang;
            // tBattleConfig.sudu = battleConfig.sudu;
            // tBattleConfig.win_die_rage = battleConfig.win_die_rage;
            // tBattleConfig.win_die_attr_up = battleConfig.win_die_attr_up;
            // tBattleConfig.win_active_attr_up = battleConfig.win_active_attr_up;
            // tBattleConfig.lose_die_attr_down = battleConfig.lose_die_attr_down;
            // tBattleConfig.lose_active_attr_down = battleConfig.lose_active_attr_down;
            // engine::InitBattleConfig(tBattleConfig);

            // auto& heroPowerConfig = model::tpl::g_tploader->configure().heroPowerConfig;
            // engine::tpl::T_HeroPowerConfig tHeroPowerConfig;
            // tHeroPowerConfig.wuli = heroPowerConfig.wuli;
            // tHeroPowerConfig.zhili = heroPowerConfig.zhili;
            // tHeroPowerConfig.fangyu = heroPowerConfig.fangyu;
            // tHeroPowerConfig.moulue = heroPowerConfig.moulue;
            // tHeroPowerConfig.xueliang = heroPowerConfig.xueliang;
            // tHeroPowerConfig.sudu = heroPowerConfig.sudu;
            // tHeroPowerConfig.divisor = heroPowerConfig.divisor;
            // engine::InitHeroPowerConfig(tHeroPowerConfig);

            SetModuleState(base::MODULE_STATE_RUNNING);
        }

        void ModuleBattle::OnModuleCleanup()
        {
            SetModuleState(base::MODULE_STATE_DELETE);
        }
    }
}
