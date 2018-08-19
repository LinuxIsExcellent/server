#include "templateloader.h"
#include "localization.h"
#include "item.h"
#include "drop.h"
#include "army.h"
#include "hero.h"
#include "alliance.h"
#include "attackrange.h"
#include "configure.h"
#include "miscconf.h"
#include "battlearrttransform.h"
#include <base/logger.h>
#include <base/dbo/dbpool.h>
#include <base/dbo/connection.h>
#include <base/framework.h>
#include <base/utils/file.h>
#include <base/utils/utils_string.h>
#include <base/lua/parser.h>

namespace model
{
    namespace tpl
    {
        using namespace std;
        using namespace base::dbo;
        using namespace base::lua;
        using namespace base;

        TemplateLoader* g_tploader = nullptr;

        TemplateLoader::TemplateLoader()
        {
            assert(g_tploader == nullptr);
            g_tploader = this;

            m_configures = new Configure();
            m_miscConf = new MiscConf();
        }

        TemplateLoader::~TemplateLoader()
        {
            g_tploader = nullptr;

            for (auto it = m_localizations.begin(); it != m_localizations.end(); ++it) {
                delete it->second;
            }
            m_localizations.clear();

            for (auto it = m_items.begin(); it != m_items.end(); ++it) {
                delete it->second;
            }
            m_items.clear();

            for (auto it = m_drops.begin(); it != m_drops.end(); ++it) {
                delete it->second;
            }
            m_drops.clear();

            for (auto it = m_armies.begin(); it != m_armies.end(); ++it) {
                delete it->second;
            }
            m_armies.clear();

            for (auto it = m_skillbases.begin(); it != m_skillbases.end(); ++it) {
                delete it->second;
            }
            m_skillbases.clear();

            for (auto it = m_skillnodes.begin(); it != m_skillnodes.end(); ++it) {
                delete it->second;
            }
            m_skillnodes.clear();

            for (auto it = m_buffs.begin(); it != m_buffs.end(); ++it) {
                delete it->second;
            }
            m_buffs.clear();

            for (auto it = m_heroes.begin(); it != m_heroes.end(); ++it) {
                delete it->second;
            }
            m_heroes.clear();

            for (auto it = m_attack_discount.begin(); it != m_attack_discount.end(); ++it) {
                delete it->second;
            }
            m_attack_discount.clear();

            for (auto it = m_army_spell.begin(); it != m_army_spell.end(); ++it) {
                delete it->second;
            }
            m_army_spell.clear();

            for (auto it = m_alliance_level_map.begin(); it != m_alliance_level_map.end(); ++it) {
                delete it->second;
            }
            m_alliance_level_map.clear();

            SAFE_DELETE(m_configures);
            SAFE_DELETE(m_miscConf);
        }

        string TemplateLoader::FindLocalizations(string name, LangType lang) const
        {
            auto const it = m_localizations.find(name);
            if (it != m_localizations.end()) {
                Localization* l = it->second;
                auto const it2 = l->values.find(lang);
                if (it2 != l->values.end()) {
                    return it2->second;
                }
            }
            return "";
        }


        void TemplateLoader::DebugDump()
        {
            cout << "model template dump:";
            cout << " localizations:" << m_localizations.size();
            cout << " items:" << m_items.size();
            cout << " drops:" << m_drops.size();
            cout << " armies:" << m_armies.size();

            cout << endl;
        }


        void TemplateLoader::BeginSetup(std::function<void(bool)> cb)
        {
            m_cb_setup = cb;

            ParseMiscConf();
        }

        void TemplateLoader::ParseMiscConf()
        {
            cout << "TemplateLoader::ParseMiscConf()" << endl;
            string dir = framework.resource_dir() + "/misc.txt";
            string content = base::utils::file_get_content(dir.c_str());
            // cout << "content = " << content << endl;
            DataTable dt = Parser::ParseAsDataTable(content);
            // dt.DumpInfo();
            const DataTable& hubSiteDt = dt.Get("hubSite")->ToTable();
            m_miscConf->hubSite.ip = hubSiteDt.Get("ip")->ToString();
            m_miscConf->hubSite.port = hubSiteDt.Get("port")->ToInteger();
            printf("hubSite ip=%s,port=%d\n", m_miscConf->hubSite.ip.c_str(), m_miscConf->hubSite.port);
            LoadLocalization();
        }

        /******** load template **********/

        void TemplateLoader::LoadLocalization()
        {
            Statement* stmt = Statement::Create(1, "select name, lang_cn, lang_tw, lang_en, lang_fr, lang_de, lang_ru, lang_kr, lang_th, \
                lang_jp, lang_pt, lang_es, lang_tr, lang_id, lang_it, lang_pl, lang_nl, lang_ar, lang_ro, lang_fa from tpl_localization");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load localization fail");
                } else {
                    while (rs.Next()) {
                        int idx = 1;
                        string name = rs.GetString(idx++);
                        string lang_cn = rs.GetString(idx++);
                        string lang_tw = rs.GetString(idx++);
                        string lang_en = rs.GetString(idx++);
                        string lang_fr = rs.GetString(idx++);
                        string lang_de = rs.GetString(idx++);
                        string lang_ru = rs.GetString(idx++);
                        string lang_kr = rs.GetString(idx++);
                        string lang_th = rs.GetString(idx++);
                        string lang_jp = rs.GetString(idx++);
                        string lang_pt = rs.GetString(idx++);
                        string lang_es = rs.GetString(idx++);
                        string lang_tr = rs.GetString(idx++);
                        string lang_id = rs.GetString(idx++);
                        string lang_it = rs.GetString(idx++);
                        string lang_pl = rs.GetString(idx++);
                        string lang_nl = rs.GetString(idx++);
                        string lang_ar = rs.GetString(idx++);
                        string lang_ro = rs.GetString(idx++);
                        string lang_fa = rs.GetString(idx++);

                        Localization* l = new Localization(name);
                        l->values[LangType::CN] = lang_cn;
                        l->values[LangType::TW] = lang_tw;
                        l->values[LangType::EN] = lang_en;
                        l->values[LangType::FR] = lang_fr;
                        l->values[LangType::DE] = lang_de;
                        l->values[LangType::RU] = lang_ru;
                        l->values[LangType::KR] = lang_kr;
                        l->values[LangType::TH] = lang_th;
                        l->values[LangType::JP] = lang_jp;
                        l->values[LangType::PT] = lang_pt;
                        l->values[LangType::ES] = lang_es;
                        l->values[LangType::TR] = lang_tr;
                        l->values[LangType::ID] = lang_id;
                        l->values[LangType::IT] = lang_it;
                        l->values[LangType::PL] = lang_pl;
                        l->values[LangType::NL] = lang_nl;
                        l->values[LangType::AR] = lang_ar;
                        l->values[LangType::RO] = lang_ro;
                        l->values[LangType::FA] = lang_fa;

                        m_localizations.emplace(name, l);
                    }

                    LoadItem();
                }
            }, m_auto_observer);
        }

        void TemplateLoader::LoadItem()
        {
            Statement* stmt = Statement::Create(1, "select id, name, type, subType, quality, requireLordLevel, requireCastleLevel, \
                price from tpl_item");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load item fail");
                } else {
                    while (rs.Next()) {
                        int idx = 1;
                        int id = rs.GetInt32(idx++);
                        string name = rs.GetString(idx++);
                        ItemType itemType = static_cast<ItemType>(rs.GetInt32(idx++));
                        int subType = rs.GetInt32(idx++);
                        int quality = rs.GetInt32(idx++);
                        int requireLordLevel = rs.GetInt32(idx++);
                        int requireCastleLevel = rs.GetInt32(idx++);
                        int price = rs.GetInt32(idx++);

                        ItemTpl* tpl = nullptr;
                        if (itemType == ItemType::EQUIP) {
                            tpl = new EquipTpl;
                            EquipTpl* equip = tpl->ToEquipTpl();
                            equip->equipType = static_cast<ItemEquipType>(subType);
                            // TODO equip attr ?

                        } else if (itemType == ItemType::PROP) {
                            tpl = new PropTpl;
                            PropTpl* prop = tpl->ToPropTpl();
                            prop->propType = static_cast<ItemPropType>(subType);
                            // TODO special params
                            switch (prop->propType) {
                                case ItemPropType::GOLD:

                                    break;
                                default:
                                    break;
                            }
                        }

                        if (tpl != nullptr) {
                            tpl->id = id;
                            tpl->name = name;
                            tpl->subType = subType;
                            tpl->quality = quality;
                            tpl->requireLordLevel = requireLordLevel;
                            tpl->requireCastleLevel = requireCastleLevel;
                            tpl->price = price;
                            m_items.emplace(id, tpl);
                        }
                    }

                    LoadDrop();
                }
            }, m_auto_observer);
        }

        void TemplateLoader::LoadDrop()
        {
            Statement* stmt = Statement::Create(1, "select dropId, groupId, tplId, countMin, countMax, probability from tpl_drop");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load drop fail");
                } else {
                    while (rs.Next()) {
                        int idx = 1;
                        int dropId = rs.GetInt32(idx++);
                        int groupId = rs.GetInt32(idx++);
                        int tplid = rs.GetInt32(idx++);
                        int countMin = rs.GetInt32(idx++);
                        int countMax = rs.GetInt32(idx++);
                        int probability = rs.GetInt32(idx++);

                        auto it = m_drops.find(dropId);
                        DropTpl* tpl = nullptr;
                        if (it == m_drops.end()) {
                            tpl = new DropTpl;
                            tpl->dropId = dropId;
                            m_drops.insert(make_pair(dropId, tpl));
                        } else {
                            tpl = it->second;
                        }
                        const ItemTpl* item = FindItem(tplid);
                        //ItemTpl* item = const_cast<ItemTpl*>(FindItem(tplid));
                        if (item == nullptr) {
                            LOG_WARN("not exist item tpl in drop tpl, when drop_id=%i. tplid=%i", dropId, tplid);
                            continue;
                        }
                        if (groupId == 0) {
                            tpl->items.emplace_back(*item, groupId, countMin, countMax, probability);
                        } else {
                            tpl->addToGroup(*item, groupId, countMin, countMax, probability);
                        }
                    }

                    LoadArmy();
                }
            }, m_auto_observer);
        }

        void TemplateLoader::LoadArmy()
        {
            Statement* stmt = Statement::Create(1, " SELECT id, name, type, subType, priority, level, hp, attack, defense, speed, attackRange,attackWall,loads, foodConsumption, \
                power, armyEffect,  armyAttrList FROM tpl_army");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load army fail");
                } else {
                    Parser p;

                    while (rs.Next()) {
                        int idx = 1;
                        ArmyTpl* tpl = new ArmyTpl;
                        tpl->id = rs.GetInt32(idx++);
                        tpl->name = rs.GetString(idx++);
                        tpl->mainType = rs.GetInt32(idx++);
                        tpl->subType = rs.GetInt32(idx++);
                        string priorityValue = rs.GetString(idx++);
                        tpl->level = rs.GetInt32(idx++);
                        tpl->hp = rs.GetInt32(idx++);
                        tpl->attack = rs.GetInt32(idx++);
                        tpl->defense = rs.GetInt32(idx++);
                        tpl->speed = rs.GetInt32(idx++);
                        tpl->attackRange = rs.GetInt32(idx++);
                        tpl->attackWall = rs.GetInt32(idx++);
                        tpl->loads = rs.GetInt32(idx++);
                        tpl->foodConsumption = rs.GetFloat(idx++);
                        tpl->power = rs.GetFloat(idx++);
                        string armyEffectValue = rs.GetString(idx++);
                        string armyAttrListValue = rs.GetString(idx++);

                        tpl->priority = p.ParseAsIntArray(priorityValue);

                        DataTable dt = p.ParseAsDataTable(armyEffectValue);
                        dt.ForeachAll([&](const DataValue & k, const DataValue & v) {
                            int armyType = k.ToInteger();
                            double percentage = v.ToDouble();
                            tpl->armyEffects.emplace_back(armyType, percentage);
                            return false;
                        });

                        tpl->armyAttrList = p.ParseAsIntArray(armyAttrListValue);

                        m_armies.emplace(tpl->id, tpl);
                        m_armys_type_level_map.emplace(std::make_tuple(tpl->subType, tpl->level), tpl);
                    }

                    // load next tpl or end
                    LoadNArmy();
                }
            }, m_auto_observer);
        }

        void TemplateLoader::LoadNArmy()
        {
            Statement* stmt = Statement::Create(1, " SELECT id, sJobName, nJobType, nRange, \
                arrtValue, description, level, speed, loads, priority, armyEffect FROM tpl_armys");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load narmy fail");
                } else {
                    Parser p;
                    while (rs.Next()) {
                        int idx = 1;
                        NArmyTpl* tpl = new NArmyTpl;
                        tpl->id = rs.GetInt32(idx++);
                        tpl->jobName = rs.GetString(idx++);
                        tpl->jobType = rs.GetInt16(idx++);
                        tpl->range = rs.GetInt16(idx++);
                        string attrValue = rs.GetString(idx++);

                        tpl->description = rs.GetString(idx++);
                        tpl->level = rs.GetInt16(idx++);
                        tpl->speed = rs.GetInt16(idx++);
                        tpl->loads = rs.GetInt16(idx++);
                        string priorityValue = rs.GetString(idx++);
                        tpl->priority = p.ParseAsIntArray(priorityValue);
                        DataTable dt = p.ParseAsDataTable(attrValue);
                        for (size_t i = 1; i <= dt.Count(); ++i) {
                            NArmyTpl::AttrValue attrValue;
                            DataTable& temp = dt.Get(i)->ToTable();
                            attrValue.type = temp.Get(1)->ToInteger();
                            attrValue.addType = temp.Get(2)->ToInteger();
                            attrValue.value = temp.Get(3)->ToDouble();
                            tpl->attrList.push_back(attrValue);
                        }

                        string armyEffectValue = rs.GetString(idx++);
                        DataTable army_dt = p.ParseAsDataTable(armyEffectValue);
                        army_dt.ForeachAll([&](const DataValue & k, const DataValue & v) {
                            int armyType = k.ToInteger();
                            double percentage = v.ToDouble();
                            tpl->armyEffects.emplace_back(armyType, percentage);
                            return false;
                        });

                        m_narmies.emplace(tpl->id, tpl);
                        m_narmys_type_level_map.emplace(std::make_tuple(tpl->jobType, tpl->level), tpl);
                    }

                    // load next tpl or end
                    LoadAttackRange();
                }
            }, m_auto_observer);
        }

        void TemplateLoader::LoadAttackRange()
        {
            Statement* stmt = Statement::Create(1, " SELECT id, attack1, attack2, attack3, attack4, attack5, attack6, \
                attack7, attack8, attack9 FROM tpl_attackrange");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load tpl_attackrange fail");
                } else {
                    Parser p;

                    while (rs.Next()) {
                        int idx = 1;
                        int attack_index = 0;
                        AttackRangeTpl* tpl = new AttackRangeTpl;
                        tpl->id = rs.GetInt32(idx++);
                        string attack1 = rs.GetString(idx++);
                        attack_index++;
                        tpl->m_attack_matrix.emplace(attack_index, p.ParseAsIntArray(attack1));
                        string attack2 = rs.GetString(idx++);
                        attack_index++;
                        tpl->m_attack_matrix.emplace(attack_index, p.ParseAsIntArray(attack2));
                        string attack3 = rs.GetString(idx++);
                        attack_index++;
                        tpl->m_attack_matrix.emplace(attack_index, p.ParseAsIntArray(attack3));
                        string attack4 = rs.GetString(idx++);
                        attack_index++;
                        tpl->m_attack_matrix.emplace(attack_index, p.ParseAsIntArray(attack4));
                        string attack5 = rs.GetString(idx++);
                        attack_index++;
                        tpl->m_attack_matrix.emplace(attack_index, p.ParseAsIntArray(attack5));
                        string attack6 = rs.GetString(idx++);
                        attack_index++;
                        tpl->m_attack_matrix.emplace(attack_index, p.ParseAsIntArray(attack6));
                        string attack7 = rs.GetString(idx++);
                        attack_index++;
                        tpl->m_attack_matrix.emplace(attack_index, p.ParseAsIntArray(attack7));
                        string attack8 = rs.GetString(idx++);
                        attack_index++;
                        tpl->m_attack_matrix.emplace(attack_index, p.ParseAsIntArray(attack8));
                        string attack9 = rs.GetString(idx++);
                        attack_index++;
                        tpl->m_attack_matrix.emplace(attack_index, p.ParseAsIntArray(attack9));

                        m_attackranges.emplace(tpl->id, tpl);
                    }

                    // load next tpl or end
                    LoadConfigure();
                }
            }, m_auto_observer);
        }

        void TemplateLoader::LoadConfigure()
        {   
            Statement* stmt = Statement::Create(1, "select name, value from tpl_configure");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load configure fail");
                } else {
                    Parser p;
                    while (rs.Next()) {
                        int idx = 1;
                        string name = rs.GetString(idx++);
                        string value = rs.GetString(idx++);
                        name = base::utils::string_lr_trim(name.c_str());
                        if (name == "battle") {
                            DataTable dt = p.ParseAsDataTable(value);
                            BattleConf& conf = m_configures->battle;
                            conf.injuryratio = dt.Get("injuryratio")->ToNumber();
                            conf.traptake = dt.Get("traptake")->ToNumber();
                            conf.wallprotect = dt.Get("wallprotect")->ToNumber();
                        } else if (name == "battleConfig") {
                            DataTable dt = p.ParseAsDataTable(value);
                            BattleConfig& battleConfig = m_configures->battleConfig;
                            battleConfig.wuli = dt.Get("wuli")->ToNumber();
                            battleConfig.zhili = dt.Get("zhili")->ToNumber();
                            battleConfig.fangyu = dt.Get("fangyu")->ToNumber();
                            battleConfig.moulue = dt.Get("moulue")->ToNumber();
                            battleConfig.xueliang = dt.Get("xueliang")->ToNumber();
                            battleConfig.sudu = dt.Get("sudu")->ToNumber();

                            battleConfig.win_die_rage = dt.Get("win_die_rage")->ToNumber();
                            battleConfig.win_die_attr_up = dt.Get("win_die_attr_up")->ToNumber();
                            battleConfig.win_active_attr_up = dt.Get("win_active_attr_up")->ToNumber();
                            battleConfig.lose_die_attr_down = dt.Get("lose_die_attr_down")->ToNumber();
                            battleConfig.lose_active_attr_down = dt.Get("lose_active_attr_down")->ToNumber();
                        } else if (name == "heroPower") {
                            DataTable dt = p.ParseAsDataTable(value);
                            HeroPowerConfig& heroPowerConfig = m_configures->heroPowerConfig;
                            heroPowerConfig.wuli = dt.Get("wuli")->ToNumber();
                            heroPowerConfig.zhili = dt.Get("zhili")->ToNumber();
                            heroPowerConfig.fangyu = dt.Get("fangyu")->ToNumber();
                            heroPowerConfig.moulue = dt.Get("moulue")->ToNumber();
                            heroPowerConfig.xueliang = dt.Get("xueliang")->ToNumber();
                            heroPowerConfig.sudu = dt.Get("sudu")->ToNumber();
                            heroPowerConfig.divisor = dt.Get("divisor")->ToNumber();
                        }   else if (name == "resourceLoad") {
                            DataTable dt = p.ParseAsDataTable(value);
                            ResourceLoadConf& conf = m_configures->resourceLoad;
                            conf.food = dt.Get("foodload")->ToNumber();
                            conf.wood = dt.Get("woodload")->ToNumber();
                            conf.iron = dt.Get("ironload")->ToNumber();
                            conf.stone = dt.Get("stoneload")->ToNumber();
                            //conf.gold = dt.Get("gold")->ToNumber();
                        } else if (name == "WallsValue") {
                            DataTable dt = p.ParseAsDataTable(value);
                            auto& wallsValue = m_configures->wallsValue;
                            wallsValue.burnbasetime = dt.Get("burnbasetime")->ToNumber();
                            wallsValue.burnincreasetime = dt.Get("burnincreasetime")->ToNumber();
                            wallsValue.gold = dt.Get("gold")->ToNumber();
                            wallsValue.recovery = dt.Get("recovery")->ToDouble();
                            wallsValue.born = dt.Get("born")->ToDouble();
                            wallsValue.base = dt.Get("base")->ToNumber();
                            wallsValue.ratio1 = dt.Get("ratio1")->ToDouble();
                            wallsValue.ratio2 = dt.Get("ratio2")->ToNumber();
                            wallsValue.span = dt.Get("span")->ToNumber();
                            wallsValue.reduce = dt.Get("reduce")->ToNumber();
                        } else if (name == "WallsRecovery") {
                            DataTable dt = p.ParseAsDataTable(value);
                            auto& wallsRecovery = m_configures->wallsRecovery;
                            wallsRecovery.price = dt.Get("price")->ToNumber();
                            wallsRecovery.monneycover = dt.Get("monneycover")->ToDouble();
                            wallsRecovery.monneycoverbase = dt.Get("monneycoverbase")->ToNumber();
                            wallsRecovery.monneycd = dt.Get("monneycd")->ToNumber();
                            wallsRecovery.gold = dt.Get("gold")->ToNumber();
                            wallsRecovery.goldcd = dt.Get("goldcd")->ToNumber();
                        } else if (name == "MonsterSiege") {
                            DataTable dt = p.ParseAsDataTable(value);
                            MonsterSiegeConf& conf = m_configures->monsterSiege;
                            conf.intervalTime = dt.Get("intervalTime")->ToNumber();
                            conf.extractionCity = dt.Get("extractionCity")->ToNumber();
                            conf.continuedTime = dt.Get("continuedTime")->ToNumber();
                            conf.resourcesPercent = dt.Get("resourcesPercent")->ToNumber();
                            conf.mixGrade = dt.Get("mixGrade")->ToNumber();
                            conf.lvMinCoe = dt.Get("lvMinCoe")->ToNumber();
                            if (conf.lvMinCoe > conf.mixGrade) {
                                LOG_ERROR("Configure MonsterSiege error : lvMinCoe(%d) > mixGrade(%d)", conf.lvMinCoe, conf.mixGrade);
                                conf.lvMinCoe = conf.mixGrade;
                            }
                            conf.lvRange = dt.Get("lvRange")->ToNumber();
                            const DataTable& drops_dt = dt.Get("extraDrops")->ToTable();
                            drops_dt.ForeachAll([&](const DataValue & k, const DataValue & v) {
                                int tplid = k.ToInteger();
                                int count = v.ToInteger();
                                if (const ItemTpl* item = FindItem(tplid)) {
                                    conf.extraDrops.emplace_back(*item, count);
                                } else {
                                    cout << "MonsterSiege's drops can not found item : itemid = " << tplid << endl;
                                }
                                return false;
                            });
                        } else if (name == "turret") {
                            DataTable dt = p.ParseAsDataTable(value);
                            TurretConf& conf = m_configures->turret;
                            conf.attackSpeed = dt.Get("attackSpeed")->ToNumber();
                            conf.hurtCoe = dt.Get("hurtCoe")->ToNumber();
                        } else if (name == "neutralCastle") {
                            DataTable dt = p.ParseAsDataTable(value);
                            NeutralCastleConf& conf = m_configures->neutralCastle;
                            conf.attackPlayerTimeAttenuation = dt.Get("attackPlayerTimeAttenuation")->ToNumber();
                            conf.attackPlayerTimeMin = dt.Get("attackPlayerTimeMin")->ToNumber();
                            conf.attackPlayerTimeMax = dt.Get("attackPlayerTimeMax")->ToNumber();
                            conf.initCount = dt.Get("initCount")->ToNumber();
                            conf.troopPercent = dt.Get("troopPercent")->ToNumber();
                            conf.castleLevel = dt.Get("castleLevel")->ToNumber();
                            conf.playerProtectedTime = dt.Get("playerProtectedTime")->ToNumber();
                        } else if (name == "palaceWar") {
                            DataTable dt = p.ParseAsDataTable(value);
                            PalaceWarConf& conf = m_configures->palaceWar;
                            conf.firstStartTime = dt.Get("firstStartTime")->ToNumber();
                            conf.recoverTime = dt.Get("recoverTime")->ToNumber();
                            //conf.dragonRecoverPercent = dt.Get("dragonRecoverPercent")->ToNumber();
                            conf.occupyTime = dt.Get("occupyTime")->ToNumber();
                            conf.chooseKingTime = dt.Get("chooseKingTime")->ToNumber();
                            conf.peaceTime = dt.Get("peaceTime")->ToNumber();
                        } else if (name == "catapult") {
                            DataTable dt = p.ParseAsDataTable(value);
                            CatapultConf& conf = m_configures->catapult;
                            conf.attackTime = dt.Get("attackTime")->ToNumber();
                            conf.recoverTime = dt.Get("recoverTime")->ToNumber();
                            DataTable& dtAttackDragon = dt.Get("attackDragon")->ToTable();
                            CatapultConf::AttackDragon& atkD = conf.attackDragon;
                            atkD.basePercent = dtAttackDragon.Get("basePercent")->ToNumber();
                            atkD.correction = dtAttackDragon.Get("correction")->ToNumber();
                            atkD.gainCoe = dtAttackDragon.Get("gainCoe")->ToNumber();
                            DataTable& dtAttackPlayer = dt.Get("attackPlayer")->ToTable();
                            CatapultConf::AttackPlayer& atkP = conf.attackPlayer;
                            atkP.baseAttack = dtAttackPlayer.Get("baseAttack")->ToNumber();
                            atkP.correction = dtAttackPlayer.Get("correction")->ToNumber();
                            atkP.gainCoe = dtAttackPlayer.Get("gainCoe")->ToNumber();
                        } else if (name == "resourceRate") {
                            DataTable dt = p.ParseAsDataTable(value);
                            std::vector<ResourceRateConf>& conf = m_configures->resourceRates;
                            for (size_t i = 0; i < dt.Count(); ++i) {
                                DataTable& temp = dt.Get(i)->ToTable();
                                ResourceRateConf rr;
                                rr.food = (int)(temp.Get("food")->ToDouble() * 10000);
                                rr.wood = (int)(temp.Get("wood")->ToDouble() * 10000);
                                rr.iron = (int)(temp.Get("iron")->ToDouble() * 10000);
                                rr.stone = (int)(temp.Get("stone")->ToDouble() * 10000);
                                conf.push_back(rr);
                            }
                        } else if (name == "palaceWarPrepare") {
                            DataTable dt = p.ParseAsDataTable(value);
                            std::vector<PalaceWarPrepareConf>& conf = m_configures->palaceWarPrepares;
                            for (size_t i = 1; i <= dt.Count(); ++i) {
                                DataTable& temp = dt.Get(i)->ToTable();
                                PalaceWarPrepareConf pwp;
                                pwp.times = i;
                                pwp.time = temp.Get("time")->ToInteger();
                                pwp.dropId = temp.Get("dropId")->ToInteger();
                                conf.push_back(pwp);
                            }
                        } else if (name == "castleProtectedLevel") {
                            m_configures->castleProtectedLevel = atoi(value.c_str());
                        } else if (name == "noviceProtected") {
                            DataTable dt = p.ParseAsDataTable(value);
                            NoviceProtectedConf& conf = m_configures->noviceProtected;
                            conf.level = dt.Get("level")->ToInteger();
                            conf.time = dt.Get("time")->ToInteger();
                        } else if (name == "palaceWarAttackDragon") {
                            DataTable dt = p.ParseAsDataTable(value);
                            PalaceWarAttackDragonConf& conf = m_configures->palaceWarAttackDragon;
                            conf.rank = dt.Get("rank")->ToInteger();
                        } else if (name == "activeUserTime") {
                            m_configures->activeUserTime = atoi(value.c_str());
                        } else if (name == "dragonNest") {
                            DataTable dt = p.ParseAsDataTable(value);
                            DragonNestConf& conf = m_configures->dragonNest;
                            conf.initCount = dt.Get("initCount")->ToInteger();
                        } else if (name == "goblinCamp") {
                            DataTable dt = p.ParseAsDataTable(value);
                            GoblinCampConf& conf = m_configures->goblinCamp;
                            conf.castleLevelLimit = dt.Get("castleLevelLimit")->ToInteger();
                            conf.digTime = dt.Get("digTime")->ToInteger();
                            conf.refreshInterval = dt.Get("refreshInterval")->ToInteger();
                            conf.roundInterval = dt.Get("roundInterval")->ToInteger();
                            conf.aX = dt.Get("aX")->ToInteger();
                            conf.aY = dt.Get("aY")->ToInteger();
                            conf.bX = dt.Get("bX")->ToInteger();
                            conf.bY = dt.Get("bY")->ToInteger();
                        } else if (name == "campFixedConsumed") {
                            DataTable dt = p.ParseAsDataTable(value);
                            auto& confCampFixed = m_configures->troopConfig.campFixedConfig;
                            confCampFixed.food = dt.Get("food")->ToNumber();
                            confCampFixed.wood = dt.Get("wood")->ToNumber();
                            confCampFixed.iron = dt.Get("iron")->ToNumber();
                            confCampFixed.stone = dt.Get("stone")->ToNumber();
                        } else if (name == "plunder") {
                            DataTable dt = p.ParseAsDataTable(value);
                            auto& plunderConfig = m_configures->plunderConfig;
                            plunderConfig.plunderlimit = dt.Get("plunderlimit")->ToNumber();
                            plunderConfig.perplunderlimit = dt.Get("perplunderlimit")->ToNumber();
                            plunderConfig.recovery = dt.Get("recovery")->ToDouble();
                            plunderConfig.foodoutresourcelimit = dt.Get("foodoutresourcelimit")->ToDouble();
                            plunderConfig.woodoutresourcelimit = dt.Get("woodoutresourcelimit")->ToDouble();
                            plunderConfig.stoneoutresourcelimit = dt.Get("stoneoutresourcelimit")->ToDouble();
                            plunderConfig.ironoutresourcelimit = dt.Get("ironoutresourcelimit")->ToDouble();
                            plunderConfig.foodinnerMax = dt.Get("foodinnerMax")->ToDouble();
                            plunderConfig.woodinnerMax = dt.Get("woodinnerMax")->ToDouble();
                            plunderConfig.stoneinnerMax = dt.Get("stoneinnerMax")->ToDouble();
                            plunderConfig.ironinnerMax = dt.Get("ironinnerMax")->ToDouble();

                            DataTable dtLevelLimit = dt.Get("levellimit")->ToTable();
                            for (size_t i = 1; i <= dtLevelLimit.Count(); ++i) {
                                DataTable& temp = dtLevelLimit.Get(i)->ToTable();
                                int level = temp.Get("level")->ToNumber();
                                float rate = temp.Get("rate")->ToDouble();
                                plunderConfig.levelLimits.push_back(std::make_tuple(level,  rate));
                            }
                            DataTable dtTimeDecays = dt.Get("timeDecays")->ToTable();
                            for (size_t i = 1; i <= dtTimeDecays.Count(); ++i) {
                                float params = dtTimeDecays.Get(i)->ToDouble();
                                plunderConfig.timeDecays.push_back(std::make_tuple(i,  params));
                            }
                        } else if (name == "cityWalls") {
                            DataTable dt = p.ParseAsDataTable(value);
                            m_configures->cityWallConfig.recovery = dt.Get("recovery")->ToDouble();
                        } else if (name == "scoutConsumed") {
                            DataTable dt = p.ParseAsDataTable(value);
                            m_configures->troopConfig.scoutConfig.food = dt.Get("food")->ToNumber();
                        } else if (name == "patrolCD") {
                            m_configures->troopConfig.patrolCD = std::stoi(value);
                        } else if (name == "Occupation") {
                            m_configures->cityConfig.occupation = std::stoi(value);
                        } else if (name == "armyRecoveryTime") {
                            m_configures->cityConfig.armyRecoveryTime = std::stoi(value);
                        } else if (name == "cityNumLimit") {
                            DataTable dt = p.ParseAsDataTable(value);
                            auto& cityNumLimit = m_configures->cityNumLimit;
                            cityNumLimit.prefecture = dt.Get("prefecture")->ToNumber();
                            cityNumLimit.chow = dt.Get("chow")->ToNumber();
                        } else if (name == "heroPhysical") {
                            DataTable dt = p.ParseAsDataTable(value);
                            auto& heroPhysical = m_configures->heroPhysical;
                            heroPhysical.initial = dt.Get("initial")->ToNumber();
                            heroPhysical.increase = dt.Get("increase")->ToNumber();
                            heroPhysical.recovery = dt.Get("recovery")->ToDouble();
                            heroPhysical.interval = dt.Get("interval")->ToNumber();
                        } else if (name == "characterInfluence") {
                            DataTable dt = p.ParseAsDataTable(value);
                            auto& characterInfluence = m_configures->characterInfluence;

                            CharacterInfluence::HeroSoloSkillProb precipitance;
                            DataTable& precipitanceData = dt.Get("precipitance")->ToTable();
                            precipitance.stone = precipitanceData.Get("stone")->ToNumber();
                            precipitance.shear = precipitanceData.Get("shear")->ToNumber();
                            precipitance.cloth = precipitanceData.Get("cloth")->ToNumber();

                            CharacterInfluence::HeroSoloSkillProb calm;
                            DataTable& calmData = dt.Get("calm")->ToTable();
                            calm.stone = calmData.Get("stone")->ToNumber();
                            calm.shear = calmData.Get("shear")->ToNumber();
                            calm.cloth = calmData.Get("cloth")->ToNumber();

                            CharacterInfluence::HeroSoloSkillProb prudent;
                            DataTable& prudentData = dt.Get("prudent")->ToTable();
                            prudent.stone = prudentData.Get("stone")->ToNumber();
                            prudent.shear = prudentData.Get("shear")->ToNumber();
                            prudent.cloth = prudentData.Get("cloth")->ToNumber();

                            CharacterInfluence::HeroSoloSkillProb fortitude;
                            DataTable& fortitudeData = dt.Get("fortitude")->ToTable();
                            fortitude.stone = fortitudeData.Get("stone")->ToNumber();
                            fortitude.shear = fortitudeData.Get("shear")->ToNumber();
                            fortitude.cloth = fortitudeData.Get("cloth")->ToNumber();

                            characterInfluence.heroSoloSkillProbs.push_back(precipitance);
                            characterInfluence.heroSoloSkillProbs.push_back(calm);
                            characterInfluence.heroSoloSkillProbs.push_back(prudent);
                            characterInfluence.heroSoloSkillProbs.push_back(fortitude);

                        } else if (name == "soloRage") {
                            DataTable dt = p.ParseAsDataTable(value);
                            auto& soloRage = m_configures->soloRage;
                            soloRage.init = dt.Get("init")->ToNumber();
                            soloRage.max = dt.Get("max")->ToNumber();
                            soloRage.success = dt.Get("success")->ToNumber();
                            soloRage.flat = dt.Get("flat")->ToNumber();
                            soloRage.fail = dt.Get("fail")->ToNumber();
                            soloRage.consume = dt.Get("consume")->ToNumber();
                        } else if (name == "escapePro") {
                            DataTable dt = p.ParseAsDataTable(value);
                            auto& escapePros = m_configures->escapePros;
                             for (size_t i = 1; i <= dt.Count(); ++i) {
                                DataTable& temp = dt.Get(i)->ToTable();
                                EscapePro escapePro;
                                escapePro.hpMin = temp.Get("hpMin")->ToNumber();
                                escapePro.hpMax = temp.Get("hpMax")->ToNumber();
                                escapePro.pro = temp.Get("pro")->ToNumber();
                                escapePros.push_back(escapePro);
                            }
                        } else if (name == "soloLose") {
                            DataTable dt = p.ParseAsDataTable(value);
                            auto& soloLose = m_configures->soloLose;
                            soloLose.runaway = dt.Get("runaway")->ToDouble();
                            soloLose.death = dt.Get("death")->ToDouble();
                        } else if (name == "cortressDurable")
                        {
                            DataTable dt = p.ParseAsDataTable(value);
                            auto& cortressDurable = m_configures->cortressDurable;
                            cortressDurable.durable = dt.Get("durable")->ToNumber();
                            cortressDurable.recoveryTime = dt.Get("recoveryTime")->ToNumber();
                            cortressDurable.recovery = dt.Get("recovery")->ToNumber();
                        } else if (name == "searchScope")
                        {
                            m_configures->searchScope = atoi(value.c_str());
                        } else if (name == "compensate") 
                        {
                            DataTable dt = p.ParseAsDataTable(value);
                            auto& compensate = m_configures->compensate;
                            compensate.resourceBase = dt.Get("resourcebase")->ToNumber();
                            compensate.food = dt.Get("foodr")->ToNumber();
                            compensate.wood = dt.Get("woodr")->ToNumber();
                            compensate.stone = dt.Get("stoner")->ToNumber();
                            compensate.iron = dt.Get("ironr")->ToNumber();
                            compensate.first = dt.Get("firstr")->ToNumber();
                            compensate.second = dt.Get("secondr")->ToNumber();
                            compensate.third = dt.Get("thirdr")->ToNumber();
                            compensate.fourth = dt.Get("fourthr")->ToNumber();
                            compensate.fifth = dt.Get("fifthr")->ToNumber();
                        } else if (name == "scoutLimit")
                        {
                            m_configures->scoutLimit = atoi(value.c_str());
                        }

                        // else if (name == "heroTroops") 
                        // {
                        //     DataTable dt = p.ParseAsDataTable(value);
                        //     auto& heroLeadershipConf = m_configures->heroLeadershipConf;
                        //     heroLeadershipConf.initial = dt.Get("initial")->ToNumber();
                        //     heroLeadershipConf.increase = dt.Get("increase")->ToNumber();     
                        // }
                    }

                    LoadSkillBase();
                }
            }, m_auto_observer);
        }


        void TemplateLoader::LoadSkillBase()
        {
            Statement* stmt = Statement::Create(1, "SELECT id, singleNodeId, mixNodeId, active, super, weight  FROM tpl_battle_spell_base");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load tpl_battle_spell_base fail");
                } else {
                    while (rs.Next()) {
                        int idx = 1;
                        SkillBaseTpl* tpl = new SkillBaseTpl;
                        tpl->id = rs.GetInt32(idx++);
                        tpl->singleNodeId = rs.GetInt32(idx++);
                        tpl->mixNodeId = rs.GetInt32(idx++);
                        tpl->active = rs.GetBoolean(idx++);
                        tpl->super = rs.GetBoolean(idx++);
                        tpl->weight = rs.GetInt32(idx++);

                        m_skillbases.emplace(tpl->id, tpl);
                    }
                    LoadSkillNode();
                }
            }, m_auto_observer);
        }

        void TemplateLoader::LoadSkillNode()
        {
            Statement* stmt = Statement::Create(1, "SELECT id, typeExt, buffs, rage  FROM tpl_battle_spell_node");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load tpl_battle_spell_node fail");
                } else {
                    Parser p;

                    while (rs.Next()) {
                        int idx = 1;
                        SkillNodeTpl* tpl = new SkillNodeTpl;
                        tpl->id = rs.GetInt32(idx++);
                        tpl->typeExt = rs.GetInt32(idx++);
                        string buffs = rs.GetString(idx++);
                        tpl->rage = rs.GetInt32(idx++);
                        tpl->buffIds = p.ParseAsIntArray(buffs);
                        m_skillnodes.emplace(tpl->id, tpl);
                    }
                    LoadBuff();
                }
            }, m_auto_observer);
        }

        void TemplateLoader::LoadBuff()
        {
            Statement* stmt = Statement::Create(1, "SELECT id, target, type, effectNow, valueType, value, rounds, coefficients, probability, isDebuff, buffs FROM tpl_battle_buff");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load tpl_battle_buff fail");
                } else {
                    Parser p;

                    while (rs.Next()) {
                        int idx = 1;
                        BuffTpl* tpl = new BuffTpl;
                        tpl->id = rs.GetInt32(idx++);
                        tpl->target = rs.GetInt32(idx++);
                        tpl->type = rs.GetInt32(idx++);
                        tpl->effectNow = rs.GetBoolean(idx++);
                        tpl->valueType = rs.GetInt32(idx++);
                        tpl->value = rs.GetInt32(idx++);
                        tpl->rounds = rs.GetInt32(idx++);
                        string coefficients = rs.GetString(idx++);
                        tpl->coefficients = p.ParseAsIntArray(coefficients);
                        tpl->probability = rs.GetInt32(idx++);
                        tpl->isDebuff = rs.GetBoolean(idx++);
                        string buffs = rs.GetString(idx++);
                        tpl->buffs = p.ParseAsIntArray(buffs);
                        m_buffs.emplace(tpl->id, tpl);
                    }
                    LoadHero();
                }
            }, m_auto_observer);
        }

        void TemplateLoader::LoadHero()
        {
            Statement* stmt = Statement::Create(1, "SELECT id, sex, camp, level, quality, office, singleSkillCalcul, leadership, hp, attack, defense, strategy, speed, intellect, star, rage, \
                                                challenge, leadershipUp, hpUp, attackUp, defenseUp, strategyUp, speedUp, saberPrac, pikemanPrac, halberdierPrac, \
                                                archerPrac, riderPrac, chariotPrac, stoneThrowerPrac, warElephantPrac, initSkill, uniqueSkill FROM tpl_hero;");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load tpl_hero fail");
                } else {
                    Parser p;

                    while (rs.Next()) {
                        int idx = 1;
                        HeroTpl* tpl = new HeroTpl;
                        tpl->id = rs.GetInt32(idx++);
                        tpl->sex = rs.GetInt16(idx++);
                        tpl->camp = rs.GetInt16(idx++);
                        tpl->level = rs.GetInt32(idx++);
                        tpl->quality = rs.GetInt16(idx++);
                        tpl->office =  rs.GetInt32(idx++);
                        std::string singleSkillCalcul = rs.GetString(idx++);
                        tpl->singleSkillCalcul = p.ParseAsIntArray(singleSkillCalcul);

                        tpl->leadership = rs.GetInt32(idx++);
                        tpl->hp = rs.GetInt32(idx++);
                        tpl->attack = rs.GetInt32(idx++);

                        tpl->defense = rs.GetInt32(idx++);
                        tpl->strategy = rs.GetInt32(idx++);
                        tpl->speed = rs.GetInt32(idx++);
                        tpl->intellect = rs.GetInt32(idx++);
                        tpl->star = rs.GetInt32(idx++);
                        tpl->rage = rs.GetInt32(idx++);
                        tpl->challenge = rs.GetInt32(idx++);

                        tpl->leadershipUp = rs.GetInt32(idx++);
                        tpl->hpUp = rs.GetInt32(idx++);
                        tpl->attackUp = rs.GetInt32(idx++);
                        tpl->defenseUp = rs.GetInt32(idx++);
                        tpl->strategyup = rs.GetInt32(idx++);
                        tpl->speedUp = rs.GetInt32(idx++);

                        tpl->saberPrac = rs.GetInt32(idx++);
                        tpl->pikemanPrac = rs.GetInt32(idx++);
                        tpl->halberdierPrac = rs.GetInt32(idx++);
                        tpl->archerPrac = rs.GetInt32(idx++);
                        tpl->riderPrac = rs.GetInt32(idx++);
                        tpl->chariotPrac = rs.GetInt32(idx++);

                        tpl->stoneThrowerPrac = rs.GetInt32(idx++);
                        tpl->warElephantPrac = rs.GetInt32(idx++);
                        tpl->initSkill = rs.GetInt32(idx++);
                        tpl->uniqueSkill = rs.GetInt32(idx++);

                        m_heroes.emplace(tpl->id, tpl);
                    }
                    LoadNHero();
                }
            }, m_auto_observer);
        }

        void TemplateLoader::LoadNHero()
        {
            Statement* stmt = Statement::Create(1, "SELECT id, nSex, sQuality, nDefaultJob, nDamageType, nInitialPower, nInitialDefense, nInitialWisdom, nInitialSkill, \
                                                nInitialAgile, nInitialLucky, nInitialLife, nInitialSolohp, nInitialTroops, nPikemanSkilled, nCavalrymanSkilled, nArcherSkilled, nMechanicsSkilled,\
                                                ntechnicalSkill1, ntechnicalSkill2, ntechnicalSkill3, ntechnicalSkill4, ntechnicalSkill5, ntechnicalSkill6, \
                                                nNirvanaSkills, \
                                                nCooperativeskills, nSynergyRole1, nSynergyRole2, nSynergyRole3, nSynergyRole4, nSynergyRole5,  nSupportPlus1, nSupportPlus2, nSupportPlus3, \
                                                nSupportPlus4, nSupportPlus5, nSupportValues1, nSupportValues2, nSupportValues3, nSupportValues4, nSupportValues5, trait FROM tpl_heros;");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load tpl_heros fail");
                } else {
                    while (rs.Next()) {
                        int idx = 1;
                        NHeroTpl* tpl = new NHeroTpl;
                        tpl->id = rs.GetInt32(idx++);
                        tpl->sex = rs.GetInt16(idx++);

                        tpl->quality = rs.GetInt16(idx++);
                        tpl->job = rs.GetInt32(idx++);
                        tpl->damageType = rs.GetInt16(idx++);

                        tpl->power = rs.GetFloat(idx++);
                        tpl->defense = rs.GetFloat(idx++);
                        tpl->wisdom = rs.GetFloat(idx++);
                        tpl->skill = rs.GetFloat(idx++);
                        tpl->agile = rs.GetFloat(idx++);
                        tpl->lucky = rs.GetFloat(idx++);
                        tpl->life = rs.GetFloat(idx++);

                        tpl->soloHp = rs.GetInt32(idx++); 
                        tpl->troops = rs.GetInt32(idx++);

                        tpl->pikemanSkill = rs.GetFloat(idx++);
                        tpl->cavalrySkill = rs.GetFloat(idx++);
                        tpl->archerSkill = rs.GetFloat(idx++);
                        tpl->mechanicsSkill = rs.GetFloat(idx++);

                        tpl->technicalSkill1 = rs.GetInt16(idx++);
                        tpl->technicalSkill2 = rs.GetInt16(idx++);
                        tpl->technicalSkill3 = rs.GetInt16(idx++);
                        tpl->technicalSkill4 = rs.GetInt16(idx++);
                        tpl->technicalSkill5 = rs.GetInt16(idx++);
                        tpl->technicalSkill6 = rs.GetInt16(idx++);

                        tpl->nirvanaSkill = rs.GetInt16(idx++);
                        tpl->cooperativeSkill = rs.GetInt16(idx++);

                        int synergyRole1 = rs.GetInt32(idx++);
                        if (synergyRole1 != 0) {
                            tpl->synergyRoles.push_back(synergyRole1);
                        }
                        
                        int synergyRole2 = rs.GetInt32(idx++);
                        if (synergyRole2 != 0) {
                            tpl->synergyRoles.push_back(synergyRole2);
                        }

                        int synergyRole3 = rs.GetInt32(idx++);  
                        if (synergyRole3 != 0) {
                            tpl->synergyRoles.push_back(synergyRole3);
                        }
       
                        int synergyRole4 = rs.GetInt32(idx++);  
                        if (synergyRole4 != 0) {
                            tpl->synergyRoles.push_back(synergyRole4);
                        }
         
                        int synergyRole5 = rs.GetInt32(idx++);    
                        if (synergyRole5 != 0) {
                            tpl->synergyRoles.push_back(synergyRole5);
                        }   

                        tpl->supportPlus1 = rs.GetFloat(idx++);
                        tpl->supportPlus2 = rs.GetFloat(idx++);
                        tpl->supportPlus3 = rs.GetFloat(idx++);
                        tpl->supportPlus4 = rs.GetFloat(idx++);
                        tpl->supportPlus5 = rs.GetFloat(idx++);

                        tpl->supportValues1 = rs.GetFloat(idx++);
                        tpl->supportValues2 = rs.GetFloat(idx++);
                        tpl->supportValues3 = rs.GetFloat(idx++);
                        tpl->supportValues4 = rs.GetFloat(idx++);
                        tpl->supportValues5 = rs.GetFloat(idx++);
                        tpl->trait = rs.GetInt32(idx++);
                        m_nheroes.emplace(tpl->id, tpl);
                    }
                    LoadSkill();
                }
            }, m_auto_observer);
        }

        void TemplateLoader::LoadSkill()
        {
            Statement* stmt = Statement::Create(1, "SELECT id, nSkillType, nDamageType, nLabelJob, nAngerConsumption, nWarConsumption, nPrestigeConsumption, \
                                                nLevelCap, nBasicParameters, nLevelParameters,  nHitJudgment, nDirectHit, nSkillRange, nAttacks, nProfessionalJob, \
                                                nRestrainHurt, nPassiveType1, nPassiveParameter1, nPassiveCoefficient1, nPassiveType2, nPassiveParameter2, nPassiveCoefficient2, \
                                                nStuntType1, nStuntParameter1, nStuntCoefficient1, nStuntType2, nStuntParameter2, nStuntCoefficient2, nStuntType3, \
                                                nStuntParameter3, nStuntCoefficient3, nEffectCondition1, nEffectRange1, nAddedEffect1, nAdditionalValue1, nUpValue1, nEffectCondition2, \
                                                nEffectRange2, nAddedEffect2, nAdditionalValue2, nUpValue2, nSkillicon, nSkillSpecial, nSkillSound FROM tpl_skill;");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load tpl_heros fail");
                } else {
                    while (rs.Next()) {
                        int idx = 1;
                        SkillTpl* tpl = new SkillTpl;
                        tpl->id = rs.GetInt32(idx++);
                        tpl->skillType = rs.GetInt16(idx++);
                        tpl->damageType = rs.GetInt16(idx++);
                        tpl->lableJob = rs.GetInt16(idx++);
                        tpl->angerConsumption = rs.GetInt16(idx++);
                        tpl->warConsumption = rs.GetInt16(idx++);
                        tpl->prestigeConsumption = rs.GetInt16(idx++);

                        tpl->levelCap = rs.GetInt16(idx++);
                        tpl->basicParameters = rs.GetInt16(idx++);
                        tpl->levelParameters = rs.GetInt16(idx++);
                        //tpl->effectLast = rs.GetInt16(idx++);  //effectLast
                        tpl->hitJudgment = rs.GetInt16(idx++);
                        tpl->directHit = rs.GetInt16(idx++);
                        tpl->skillRange = rs.GetInt16(idx++);
                        tpl->attacks = rs.GetInt16(idx++);
                        tpl->professionalJob = rs.GetInt16(idx++);

                        tpl->restrainHurt = rs.GetInt16(idx++);
                        tpl->passiveType1 = rs.GetInt16(idx++);
                        tpl->passiveParameter1 = rs.GetFloat(idx++);
                        tpl->passiveCoefficient1 = rs.GetFloat(idx++);
                        tpl->passiveType2 = rs.GetInt16(idx++);
                        tpl->passiveParameter2 = rs.GetFloat(idx++);
                        tpl->passiveCoefficient2 = rs.GetFloat(idx++);

                        tpl->stuntType1 = rs.GetInt16(idx++);
                        tpl->stuntParameter1 = rs.GetInt16(idx++);
                        tpl->stuntCoefficient1 = rs.GetInt16(idx++);
                        tpl->stuntType2 = rs.GetInt16(idx++);
                        tpl->stuntParameter2 = rs.GetInt16(idx++);
                        tpl->stuntCoefficient2 = rs.GetInt16(idx++);
                        tpl->stuntType3 = rs.GetInt16(idx++);

                        tpl->stuntParameter3 = rs.GetInt16(idx++);
                        tpl->stuntCoefficient3 = rs.GetInt16(idx++);
                        tpl->effectCondition1 = rs.GetInt16(idx++);
                        tpl->effectRange1 = rs.GetInt16(idx++);
                        tpl->addedEffect1 = rs.GetInt16(idx++);
                        tpl->additionalValue1 = rs.GetInt16(idx++);
                        tpl->upValue1 = rs.GetInt16(idx++);
                        tpl->effectCondition2 = rs.GetInt16(idx++);
                        
                        tpl->effectRange2 = rs.GetInt16(idx++);
                        tpl->addedEffect2 = rs.GetInt16(idx++);
                        tpl->additionalValue2 = rs.GetInt16(idx++);
                        tpl->upValue2 = rs.GetInt16(idx++);
                        tpl->nSkillicon = rs.GetInt16(idx++);
                        tpl->nSkillSpecial = rs.GetInt16(idx++);
                        tpl->nSkillSound = rs.GetInt16(idx++);

                        m_skills.emplace(tpl->id, tpl);
                    }
                    LoadAttackDiscount();
                }
            }, m_auto_observer);
        }

        void TemplateLoader::LoadAttackDiscount()
        {
            Statement* stmt = Statement::Create(1, "SELECT target, discount FROM tpl_battle_attack_discount");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load tpl_battle_attack_discount fail");
                } else {
                    while (rs.Next()) {
                        int idx = 1;
                        AttackDiscountTpl* tpl = new AttackDiscountTpl;
                        tpl->target = rs.GetInt32(idx++);
                        tpl->discount = rs.GetInt32(idx++);

                        m_attack_discount.emplace(tpl->target, tpl);
                    }
                    LoadArmySpell();
                }
            }, m_auto_observer);
        }

        void TemplateLoader::LoadArmySpell()
        {
            Statement* stmt = Statement::Create(1, "SELECT id, buffs, probability FROM tpl_army_spell");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load tpl_army_spell fail");
                } else {
                    Parser p;

                    while (rs.Next()) {
                        int idx = 1;
                        ArmySpellTpl* tpl = new ArmySpellTpl;
                        tpl->id = rs.GetInt32(idx++);
                        string buffs = rs.GetString(idx++);
                        tpl->probability = rs.GetInt32(idx++);
                        tpl->buffIds = p.ParseAsIntArray(buffs);

                        m_army_spell.emplace(tpl->id, tpl);
                    }
                    LoadAllianceLevel();
                }
            }, m_auto_observer);
        }

        void TemplateLoader::LoadAllianceLevel()
        {
            Statement* stmt = Statement::Create(1, "SELECT level, exp, alliesMax, castleMax, troopsMax, towerMax , patrolMax FROM tpl_alliance_level");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load tpl_alliance_level fail");
                } else {
//                     Parser p;

                    while (rs.Next()) {
                        int idx = 1;
                        AllianceLevelTpl* tpl = new AllianceLevelTpl;
                        tpl->level = rs.GetInt32(idx++);
                        tpl->exp = rs.GetInt32(idx++);
                        tpl->cityMax = rs.GetInt32(idx++);
                        tpl->castleMax = rs.GetInt32(idx++);
                        tpl->troopsMax = rs.GetInt32(idx++);
                        tpl->towerMax = rs.GetInt32(idx++);
                        tpl->patrolMax = rs.GetInt32(idx++);

                        m_alliance_level_map.emplace(tpl->level, tpl);
                    }
                    LoadBattleArrtTransform();
                }
            }, m_auto_observer);
        }

         void TemplateLoader::LoadBattleArrtTransform()
        {
            Statement* stmt = Statement::Create(1, "SELECT id, sceondArrt, firstArrt, transformNum FROM tpl_battle_arrt_transform");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load tpl_battle_arrt_transform fail");
                } else {
                    while (rs.Next()) {
                        int idx = 1;
                        BattleArrtTransformTpl* tpl = new BattleArrtTransformTpl;
                        tpl->id = rs.GetInt32(idx++);
                        tpl->sceondArrt = rs.GetInt32(idx++);
                        if (tpl->sceondArrt < 5008 || tpl->sceondArrt > 5017)
                        {
                            LOG_ERROR("LoadBattleArrtTransform error : tpl->sceondArrt(%d) is error", tpl->sceondArrt);
                            continue;
                        }
                        tpl->firstArrt = rs.GetInt32(idx++);
                        tpl->transformNum = rs.GetFloat(idx++);

                        /*std::cout << "tpl->sceondArrt = " << tpl->sceondArrt << "tpl->firstArrt = " << tpl->firstArrt << "tpl->transformNum" << tpl->transformNum << std::endl;*/
                        m_battleArrtTransform.emplace(tpl->sceondArrt, tpl);
                    }
                    LoadHeroSoul();
                }
            }, m_auto_observer);
        }

        void TemplateLoader::LoadHeroSoul()
        {
            Statement* stmt = Statement::Create(1, "SELECT id, nDamageType, soulLevel, attrList FROM tpl_hero_soul");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load tpl_hero_soul fail");
                } else {
                    Parser p;
                    while (rs.Next()) {
                        int idx = 1;
                        HeroSoulTpl* tpl = new HeroSoulTpl;
                        tpl->id = rs.GetInt32(idx++);
                        tpl->nDamageType = rs.GetInt32(idx++);
                        tpl->soulLevel = rs.GetInt32(idx++);
                        std::string strAttr = rs.GetString(idx++);
                        DataTable dt;
                        dt = p.ParseAsDataTable(strAttr);
                        tpl->type = static_cast<AttributeType>(dt.Get(1)->ToNumber());
                        tpl->addType = static_cast<AttributeAdditionType>(dt.Get(2)->ToNumber());
                        tpl->value = dt.Get(3)->ToNumber();
                    }
                    LoadHeroLevelAttr();
                }
            }, m_auto_observer);
        }

        void TemplateLoader::LoadHeroLevelAttr()
        {
            Statement* stmt = Statement::Create(1, "SELECT heroesId, remark, levelmin, levelmax, nGrowthPower, nGrowthDefense, \
                nGrowthWisdom, nGrowthSkill, nGrowthAgile, nGrowthLucky, nGrowthLife, nGrowthTroops, nGrowthSolohp FROM tpl_hero_level_attr");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load tpl_hero_soul fail");
                } else {
                    while (rs.Next()) {
                        int idx = 1;
                        HeroLevelAttrTpl* tpl = new HeroLevelAttrTpl;
                        tpl->heroesId = rs.GetInt32(idx++);
                        tpl->levelmin = rs.GetInt32(idx++);
                        tpl->levelmax = rs.GetInt32(idx++);
                        tpl->nGrowthPower = rs.GetFloat(idx++);
                        tpl->nGrowthDefense = rs.GetFloat(idx++);
                        tpl->nGrowthWisdom = rs.GetFloat(idx++);
                        tpl->nGrowthSkill = rs.GetFloat(idx++);
                        tpl->nGrowthAgile = rs.GetFloat(idx++);
                        tpl->nGrowthLucky = rs.GetFloat(idx++);
                        tpl->nGrowthLife = rs.GetFloat(idx++);
                        tpl->nGrowthTroops = rs.GetInt32(idx++);
                        tpl->nGrowthSolohp = rs.GetInt32(idx++);
                        auto it = m_heroLevelAttr.find(tpl->heroesId);
                        if (it !=  m_heroLevelAttr.end()) {
                            it->second.emplace_back(tpl);
                        } else {
                            m_heroLevelAttr.emplace(tpl->heroesId, std::vector<HeroLevelAttrTpl*>(1, tpl));
                        }  
                    }
                    LoadHeroStarLevel();
                }
            }, m_auto_observer);
        }

        void TemplateLoader::LoadHeroStarLevel()
        {
            Statement* stmt = Statement::Create(1, "SELECT heroesID, level, PowerAdd, DefenseAdd, WisdomAdd, SkillAdd, AgileAdd, LuckyAdd, \
                LifeAdd, SolohpAdd, TroopsAdd FROM tpl_hero_star_level");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load LoadHeroStarLevel fail");
                } else {
                    while (rs.Next()) {
                        int idx = 1;
                        HeroStarLevelTpl* tpl = new HeroStarLevelTpl;

                        tpl->heroesId = rs.GetInt32(idx++);
                        tpl->level = rs.GetInt32(idx++);
                        tpl->PowerAdd = rs.GetInt32(idx++);
                        tpl->DefenseAdd = rs.GetFloat(idx++);
                        tpl->WisdomAdd = rs.GetFloat(idx++);
                        tpl->SkillAdd = rs.GetFloat(idx++);
                        tpl->AgileAdd = rs.GetFloat(idx++);
                        tpl->LuckyAdd = rs.GetFloat(idx++);
                        tpl->LifeAdd = rs.GetFloat(idx++);
                        tpl->SolohpAdd = rs.GetInt32(idx++);
                        tpl->TroopsAdd = rs.GetInt32(idx++);

                        auto it = m_heroStarLevel.find(tpl->heroesId);
                        if (it !=  m_heroStarLevel.end()) {
                            it->second.emplace_back(tpl);
                        } else {
                            m_heroStarLevel.emplace(tpl->heroesId, std::vector<HeroStarLevelTpl*>(1, tpl));
                        }

                    }
                    LoadBattleArrt();
                }
            }, m_auto_observer);
        }

        void TemplateLoader::LoadBattleArrt()
        {
            Statement* stmt = Statement::Create(1, "SELECT id, battlePower FROM tpl_battle_arrt");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load LoadBattleArrt fail");
                } else {
                    while (rs.Next()) {
                        int idx = 1;
                        BattleArrtTpl* tpl = new BattleArrtTpl;

                        tpl->type = rs.GetInt32(idx++);
                        tpl->battlePower = rs.GetInt32(idx++);

                        m_battleArrt.emplace(tpl->type, tpl);

                    }
                    LoadBattleInjuredSoldiers();
                }
            }, m_auto_observer);
        }

        void TemplateLoader::LoadBattleInjuredSoldiers()
        {
            Statement* stmt = Statement::Create(1, "SELECT action, injuredSoldiers, deadSoldiers, defInjuredSoldiers, defDeadSoldiers FROM tpl_battle_injuredsoldiers");

            stmt->Execute([this](ResultSet & rs) {
                if (rs.HasError()) {
                    m_cb_setup(false);
                    LOG_DEBUG("load LoadBattleInjuredSoldiers fail");
                } else {
                    while (rs.Next()) {
                        int idx = 1;
                        BattleInjuredSoldiersTpl* tpl = new BattleInjuredSoldiersTpl;

                        tpl->action = rs.GetInt32(idx++);
                        tpl->injuredSoldiers = rs.GetFloat(idx++);
                        tpl->deadSoldiers = rs.GetFloat(idx++);
                        tpl->defInjuredSoldiers = rs.GetFloat(idx++);
                        tpl->defDeadSoldiers = rs.GetFloat(idx++);

                        m_battleInjuredSoldiers.emplace(tpl->action, tpl);

                    }
                    LoadEnd();
                }
            }, m_auto_observer);
        }

        void TemplateLoader::LoadEnd()
        {
            cout << "TemplateLoader::LoadEnd" << endl;
            g_dbpool->DisconnectAll(1);
            m_cb_setup(true);
        }
    }
}
