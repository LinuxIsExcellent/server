-- 生成模板，用于将模板数据同步到客户端
local clientTpl = {}
local p = require('protocol')
local dbo = require('dbo')
local utils = require('utils')

local _clientTemplates = {
    -- 'item.json' = {hash = '51313512143145312511', content= ''},
    -- 'map_units.json' = {hash = '13431531532134134532151', content= ''}
}

function clientTpl.getTemplates()
    return _clientTemplates
end

local function _serializeAndCompress(name, t)
    for k,v in pairs(t) do
        if v.remark then
            v.remark = nil -- clear remarks
        end
    end
    --转小写
    name = string.lower(name)
    local compressedJson, hash = utils.toCompressedJson(t)
    _clientTemplates[name] = {
        name = name,
        hash = hash,
        content = compressedJson
    }
    print ('client tempalte loaded:', hash, name)
    local path = utils.getResourceDir() .. '/tpl_example/raw_' .. name
    local f, err = io.open(path, 'w+')
    if f then
        f:write(utils.toJson(t))
        f:close()
    else
        print(err)
    end
end

-- 多语言
local function _loadLang(db)
    local rs = db:execute('SELECT * FROM tpl_lang')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('lang.json', t)
end

local function _loadLocalization(db)
    local t = {}
    local rs = db:execute('SELECT * FROM tpl_localization')
    assert(rs.ok)
    for _, row in ipairs(rs) do
        table.insert(t, {
            name = row.name,
            cn = row.lang_cn,
            tw = row.lang_tw,
            en = row.lang_en,
            fr = row.lang_fr,
            de = row.lang_de,
            ru = row.lang_ru,
            kr = row.lang_kr,
            th = row.lang_th,
            jp = row.lang_jp,
            pt = row.lang_pt,
            es = row.lang_es,
            tr = row.lang_tr,
            id = row.lang_id,
            it = row.lang_it,
            pl = row.lang_pl,
            nl = row.lang_nl,
            ar = row.lang_ar,
            ro = row.lang_ro,
            fa = row.lang_fa
        })
    end
    _serializeAndCompress('localization.json', t)
end

local function _loadLocalizationPatch(db)
    local t = {}
    local rs = db:execute('SELECT * FROM tpl_localization_patch')
    assert(rs.ok)
    for _, row in ipairs(rs) do
        table.insert(t, {
            name = row.name,
            cn = row.lang_cn,
            tw = row.lang_tw,
            en = row.lang_en,
            fr = row.lang_fr,
            de = row.lang_de,
            ru = row.lang_ru,
            kr = row.lang_kr,
            th = row.lang_th,
            jp = row.lang_jp,
            pt = row.lang_pt,
            es = row.lang_es,
            tr = row.lang_tr,
            id = row.lang_id,
            it = row.lang_it,
            pl = row.lang_pl,
            nl = row.lang_nl,
            ar = row.lang_ar,
            ro = row.lang_ro,
            fa = row.lang_fa
        })
    end
    _serializeAndCompress('localizationPatch.json', t)
end

local function _loadLocalizationHero(db)
    local t = {}
    local rs = db:execute('SELECT * FROM tpl_localization_hero')
    assert(rs.ok)
    for _, row in ipairs(rs) do
        table.insert(t, {
            name = row.name,
            cn = row.lang_cn,
            tw = row.lang_tw,
            en = row.lang_en,
            fr = row.lang_fr,
            de = row.lang_de,
            ru = row.lang_ru,
            kr = row.lang_kr,
            th = row.lang_th,
            jp = row.lang_jp,
            pt = row.lang_pt,
            es = row.lang_es,
            tr = row.lang_tr,
            id = row.lang_id,
            it = row.lang_it,
            pl = row.lang_pl,
            nl = row.lang_nl,
            ar = row.lang_ar,
            ro = row.lang_ro,
            fa = row.lang_fa
        })
    end
    _serializeAndCompress('localizationHero.json', t)
end

local function _loadLocalizationQuest(db)
    local t = {}
    local rs = db:execute('SELECT * FROM tpl_localization_quest')
    assert(rs.ok)
    for _, row in ipairs(rs) do
        table.insert(t, {
            name = row.name,
            cn = row.lang_cn,
            tw = row.lang_tw,
            en = row.lang_en,
            fr = row.lang_fr,
            de = row.lang_de,
            ru = row.lang_ru,
            kr = row.lang_kr,
            th = row.lang_th,
            jp = row.lang_jp,
            pt = row.lang_pt,
            es = row.lang_es,
            tr = row.lang_tr,
            id = row.lang_id,
            it = row.lang_it,
            pl = row.lang_pl,
            nl = row.lang_nl,
            ar = row.lang_ar,
            ro = row.lang_ro,
            fa = row.lang_fa
        })
    end
    _serializeAndCompress('localizationQuest.json', t)
end

-- 物品
local function _loadItem(db)
    local rs = db:execute('SELECT * FROM tpl_item')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('item.json', t)
end
-- store
local function _loadStore(db)
    local rs = db:execute('SELECT * FROM tpl_store')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('store.json', t)
end

-- drop
local function _loadDrop(db)
    local rs = db:execute('SELECT * FROM tpl_drop')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('drop.json', t)
end

-- 地图中的元件
local function _loadMapUnit(db)
    local t = {}
    local rs = db:execute('SELECT * FROM tpl_map_unit')
    assert(rs.ok)
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('mapunits.json', t)
end

-- 地图中的城市
local function _loadMapCity(db)
    local t = {}
    local rs = db:execute('SELECT * FROM tpl_map_city')
    assert(rs.ok)
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('mapcity.json', t)
end

local function _loadCityPatrolEvent(db)
    local t = {}
    local rs = db:execute('SELECT * FROM tpl_city_patrol_event')
    assert(rs.ok)
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('citypatrolevent.json', t)
end

local function _loadMapTroop(db)
    local t = {}
    local rs = db:execute('SELECT * FROM tpl_map_troop')
    assert(rs.ok)
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('maptroop.json', t)
end

-- 全局配置    
local function _loadConfigure(db)
    local rs = db:execute('SELECT name, value FROM tpl_configure')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('configure.json', t)
end

-- 
local function _loadLordLevel(db)
    local rs = db:execute('SELECT * FROM tpl_lord_level')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('lordLevel.json', t)
end

-- 
local function _loadLordHead(db)
    local rs = db:execute('SELECT * FROM tpl_lord_head')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('lordHead.json', t)
end

-- 
local function _loadIconFunction(db)
    local rs = db:execute('SELECT id, name, icon, jump, description FROM tpl_icon_function')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('iconFunction.json', t)
end

-- 
local function _loadForest(db)
    local rs = db:execute('SELECT * FROM tpl_forest')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('forest.json', t)
end

-- 
local function _loadBuilding(db)
    local rs = db:execute('SELECT * FROM tpl_building')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('building.json', t)
end

-- 
local function _loadBuildingLevel(db)
    local rs = db:execute('SELECT * FROM tpl_building_level')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('buildinglevel.json', t)
end

local function _loadTakeTax(db)
    local rs = db:execute('SELECT * FROM tpl_take_tax')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('taketax.json', t)
end

-- 
local function _loadArmy(db)
    local rs = db:execute('SELECT * FROM tpl_army')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('army.json', t)
end

local function _loadArmySpell(db)
    local rs = db:execute('SELECT * FROM tpl_army_spell')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('armyspell.json', t)
end

local function _loadAttackDiscount(db)
    local rs = db:execute('SELECT * FROM tpl_battle_attack_discount')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('attackdiscount.json', t)
end

-- 
local function _loadTechnology(db)
    local rs = db:execute('SELECT * FROM tpl_technology')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('technology.json', t)
end

--
local function _loadMessage(db)
    local rs = db:execute('SELECT * FROM tpl_message')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('message.json', t)
end

--
local function _loadMail(db)
    local rs = db:execute('SELECT * FROM tpl_mail')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('mail.json', t)
end

local function _loadNovice(db)
    local rs = db:execute('SELECT * FROM tpl_novice')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('novice.json', t)
end

local function _loadChat(db)
    local rs = db:execute('SELECT * FROM tpl_chat')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('chat.json', t)
end

local function _loadActivity(db)
    local rs = db:execute('SELECT * FROM tpl_activity')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('activity.json', t)
end

local function _loadHero(db)
    local rs = db:execute('SELECT * FROM tpl_hero')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('hero.json', t)
end

local function _loadHeroDraw(db)
    local rs = db:execute('SELECT * FROM tpl_hero_draw')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('heroDraw.json', t)
end

local function _loadHeroLevel(db)
    local rs = db:execute('SELECT * FROM tpl_hero_level')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('heroLevel.json', t)
end

local function _loadHeroStarLevel(db)
    local rs = db:execute('SELECT * FROM tpl_hero_star_level')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('heroStarLevel.json', t)
end

local function _loadHeroSlotUnlock(db)
    local rs = db:execute('SELECT * FROM tpl_hero_slot_unlock')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('heroSlotUnlock.json', t)
end

local function _loadHeroSkillLevel(db)
    local rs = db:execute('SELECT * FROM tpl_hero_skill_level')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('heroSkillLevel.json', t)
end

local function _loadHeroLevelAttr(db)
    local rs = db:execute('SELECT * FROM tpl_hero_level_attr')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('herolevelattr.json', t)
end

local function _loadHeroSoul(db)
    local rs = db:execute('SELECT * FROM tpl_hero_soul')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('herosoul.json', t)
end

local function _loadHeroTalent(db)
    local rs = db:execute('SELECT * FROM tpl_hero_talent')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('herotalent.json', t)
end

local function _loadEquipLevel(db)
    local rs = db:execute('SELECT * FROM tpl_equip_level')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('equipLevel.json', t)
end

local function _loadQualityInfo(db)
    local rs = db:execute('SELECT * FROM tpl_quality_info')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('qualityInfo.json', t)
end

local function _loadEquipSuccinctInlay(db)
    local rs = db:execute('SELECT * FROM tpl_equip_succinct_inlay')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('equipSuccinctInlay.json', t)
end

local function _loadSuccinctAttribute(db)
    local rs = db:execute('SELECT * FROM tpl_succinct_attribute')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('succinctAttribute.json', t)
end

local function _loadSgMusic(db)
    local rs = db:execute('SELECT * FROM tpl_sg_music')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('sgmusic.json', t)
end

local function _loadBattleSpellBase(db)
    local rs = db:execute('SELECT * FROM tpl_battle_spell_base')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('battleSpellBase.json', t)
end

local function _loadBattleSpellNode(db)
    local rs = db:execute('SELECT * FROM tpl_battle_spell_node')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('battleSpellNode.json', t)
end

local function _loadBattleBuff(db)
    local rs = db:execute('SELECT * FROM tpl_battle_buff')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('battleBuff.json', t)
end

local function _loadSystemunlock(db)
    local rs = db:execute('SELECT * FROM tpl_system_unlock')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('systemUnlock.json', t)
end

local function _loadMapNpcArmy(db)
    local rs = db:execute('SELECT * FROM tpl_map_npc_army')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('mapNpcArmy.json', t)
end

local function _loadScenarioSection(db)
    local rs = db:execute('SELECT * FROM tpl_scenario_section')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('scenarioSection.json', t)
end

local function _loadScenarioChapter(db)
    local rs = db:execute('SELECT * FROM tpl_scenario_chapter')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('scenarioChapter.json', t)
end

local function _loadQuestionPool(db)
    local rs = db:execute('SELECT * FROM tpl_question_pool')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('questionPool.json', t)
end

local function _loadQuestionDialogue(db)
    local rs = db:execute('SELECT * FROM tpl_question_dialogue')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('questionDialogue.json', t)
end

local function _loadStore(db)
    local rs = db:execute('SELECT * FROM tpl_store')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('store.json', t)
end

local function _loadStoreItemLibrary(db)
    local rs = db:execute('SELECT * FROM tpl_store_item_library')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('storeItemLibrary.json', t)
end

local function _loadStoreRefreshPrice(db)
    local rs = db:execute('SELECT * FROM tpl_store_refresh_price')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('storeRefreshPrice.json', t)
end

local function _loadMapTroop(db)
    local rs = db:execute('SELECT * FROM tpl_map_troop')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('maptroop.json', t)
end

local function _loadStaminaBuy(db)
    local rs = db:execute('SELECT * FROM tpl_stamina_buy')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('staminabuy.json', t)
end
local function _loadEnergyBuy(db)
    local rs = db:execute('SELECT * FROM tpl_energy_buy')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('energybuy.json', t)
end
local function _loadVip(db)
    local rs = db:execute('SELECT * FROM tpl_vip')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('vip.json', t)
end

local function _loadQuest(db)
    local rs = db:execute('SELECT * FROM tpl_quest')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('quest.json', t)
end

local function _loadDailyTask(db)
    local rs = db:execute('SELECT * FROM tpl_daily_task')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('dailytask.json', t)
end

local function _loadDailyTaskReward(db)
    local rs = db:execute('SELECT * FROM tpl_daily_task_reward')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('dailytaskreward.json', t)
end

local function _loadScoutType(db)
    local rs = db:execute('SELECT * FROM tpl_scout_type')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('scouttype.json', t)
end

local function _loadAllianceLevel(db)
    local rs = db:execute('SELECT * FROM tpl_alliance_level')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('alliancelevel.json', t)
end

local function _loadAllianceBanner(db)
    local rs = db:execute('SELECT * FROM tpl_alliance_banner')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('alliancebanner.json', t)
end

local function _loadAllianceRankDetail(db)
    local rs = db:execute('SELECT * FROM tpl_alliance_rank_detail')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('alliancerankdetail.json', t)
end

local function _loadAllianceScience(db)
    local rs = db:execute('SELECT * FROM tpl_alliance_science')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('alliancescience.json', t)
end

local function _loadAllianceDonate(db)
    local rs = db:execute('SELECT * FROM tpl_alliance_donate')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('alliancedonate.json', t)
end

local function _loadAllianceBuff(db)
    local rs = db:execute('SELECT * FROM tpl_alliance_buff')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('alliancebuff.json', t)
end

local function _loadAllianceMessage(db)
    local rs = db:execute('SELECT * FROM tpl_alliance_message')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('alliancemessage.json', t)
end

local function _loadHeroFetter(db)
    local rs = db:execute('SELECT * FROM tpl_hero_fetter')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('herofetter.json', t)
end

local function _loadHeroFetterAttribute(db)
    local rs = db:execute('SELECT * FROM tpl_hero_fetter_attribute')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('herofetterattribute.json', t)
end

--
local function _loadAchievement(db)
    local rs = db:execute('SELECT * FROM tpl_achievement')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('achievement.json', t)
end

local function _loadBabel(db)
    local rs = db:execute('SELECT * FROM tpl_babel')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('babel.json', t)
end

local function _loadBronzeSparrowTower(db)
    local rs = db:execute('SELECT * FROM tpl_bronze_sparrow_tower')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('bronzesparrowtower.json', t)
end

local function _loadTitle(db)
    local rs = db:execute('SELECT * FROM tpl_title')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('title.json', t)
end

local function _loadChest(db)
    local rs = db:execute('SELECT * FROM tpl_chest')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('chest.json', t)
end

local function _loadHeroDrawType(db)
    local rs = db:execute('SELECT * FROM tpl_hero_draw_type')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('herodrawtype.json', t) 
end

local function _loadHeros(db)
    local rs = db:execute('SELECT * FROM tpl_heros')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('heros.json', t) 
end

local function _loadArmys(db)
    local rs = db:execute('SELECT * FROM tpl_armys')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('armys.json', t) 
end

local function _loadSkillPreBattle(db)
    local rs = db:execute('SELECT * FROM tpl_skill_pre_battle')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('skillprebattle.json', t) 
end

local function _loadSkill(db)
    local rs = db:execute('SELECT * FROM tpl_skill')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('skill.json', t) 
end

local function _loadEffect(db)
    local rs = db:execute('SELECT * FROM tpl_effect')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('effect.json', t) 
end

local function _loadAttackRange(db)
    local rs = db:execute('SELECT * FROM tpl_attackrange')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('attackrange.json', t)
end

local function _loadMore(db)
    local rs = db:execute('SELECT * FROM tpl_more')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('more.json', t)
end

local function _loadQuestStory(db)
    local rs = db:execute('SELECT * FROM tpl_quest_story')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('queststory.json', t)
end

local function _loadQuestDay(db)
    local rs = db:execute('SELECT * FROM tpl_quest_day')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('questday.json', t)
end

local function _loadQuestDayPoint(db)
    local rs = db:execute('SELECT * FROM tpl_quest_day_points')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('questdaypoints.json', t)
end

local function _loadStronge(db)
    local rs = db:execute('SELECT * FROM tpl_stronge')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('stronge.json', t) 
end

local function _loadArenaRankReward(db)
    local rs = db:execute('SELECT * FROM tpl_arena_rank_reward')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('arenarankreward.json', t) 
end

local function _loadPlayerQte(db)
    local rs = db:execute('SELECT * FROM tpl_player_qte')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('playerqte.json', t) 
end

local function _loadTaixuedianReward(db)
    local rs = db:execute('SELECT * FROM tpl_taixuedian_reward')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('taixuedianreward.json', t) 
end

local function _loadStory(db)
    local rs = db:execute('SELECT * FROM tpl_story')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('story.json', t) 
end

local function _loadBattleArrtTransform(db)
    local rs = db:execute('SELECT * FROM tpl_battle_arrt_transform')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('battlearrttransform.json', t) 
end

local function _loadArmysType(db)
    local rs = db:execute('SELECT * FROM tpl_armys_type')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('armystype.json', t) 
end

local function _loadSeowon(db)
    local rs = db:execute('SELECT * FROM tpl_seowon')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('seowon.json', t) 
end

local function _loadBattleArrt(db)
    local rs = db:execute('SELECT * FROM tpl_battle_arrt')
    assert(rs.ok)
    local t = {}
    for _, row in ipairs(rs) do
        table.insert(t, row)
    end
    _serializeAndCompress('battlearrt.json', t) 
end

function clientTpl.loadAllTemplate()
    local db = dbo.open(1)
    -- _loadLang(db)
    _loadLocalization(db)
    _loadLocalizationPatch(db)
    _loadLocalizationQuest(db)
    _loadLocalizationHero(db)
    _loadItem(db)
    _loadDrop(db)
    _loadMapUnit(db)
    _loadMapCity(db)
    _loadCityPatrolEvent(db)
    -- _loadMapTroop(db)
    _loadConfigure(db)
    _loadLordLevel(db)
    _loadLordHead(db)
    _loadIconFunction(db)
    _loadForest(db)
    _loadBuilding(db)
    _loadBuildingLevel(db)
    _loadTakeTax(db)
    _loadArmy(db)
    _loadArmySpell(db)
    _loadAttackDiscount(db)
    _loadTechnology(db)
    _loadMessage(db)
    _loadMail(db)
    _loadNovice(db)
    -- _loadChat(db)
    _loadActivity(db)
    _loadHero(db)
    _loadHeroDraw(db)

    _loadHeroLevel(db)
    _loadHeroStarLevel(db)
    _loadHeroSlotUnlock(db)
    _loadHeroSkillLevel(db)
    _loadHeroLevelAttr(db)
    _loadHeroSoul(db)
    _loadHeroTalent(db)
    _loadEquipLevel(db)
    _loadQualityInfo(db)
    _loadEquipSuccinctInlay(db)
    _loadSuccinctAttribute(db)
    _loadHeroFetter(db)
    _loadHeroFetterAttribute(db)
    _loadAchievement(db)

    _loadSgMusic(db)
    _loadBattleSpellBase(db)
    _loadBattleSpellNode(db)
    _loadBattleBuff(db)
    _loadMapNpcArmy(db)
    _loadScenarioSection(db)
    _loadScenarioChapter(db)
    _loadQuestionPool(db)
    _loadQuestionDialogue(db)
    _loadSystemunlock(db)
    _loadStore(db)
    _loadStoreItemLibrary(db)
    _loadStoreRefreshPrice(db)
    _loadMapTroop(db)
    _loadStaminaBuy(db)
    _loadVip(db)
    _loadQuest(db)
    _loadDailyTask(db)
    _loadDailyTaskReward(db)
    _loadScoutType(db)
    _loadAllianceLevel(db)
    _loadAllianceBanner(db)
    _loadAllianceRankDetail(db)
    _loadAllianceScience(db)
    _loadAllianceDonate(db)
    _loadAllianceBuff(db)
    _loadAllianceMessage(db)
    _loadBabel(db)
    _loadBronzeSparrowTower(db)
    _loadHeros(db)
    _loadArmys(db)
    _loadSkill(db)
    _loadSkillPreBattle(db)
    _loadTitle(db)
    _loadChest(db)
    _loadHeroDrawType(db)
    _loadEffect(db)
    _loadAttackRange(db)
    _loadMore(db)
    _loadQuestStory(db)
    _loadQuestDay(db)
    _loadQuestDayPoint(db)
    _loadStronge(db)
    _loadArenaRankReward(db)
    _loadPlayerQte(db)
    _loadTaixuedianReward(db)
    _loadStory(db)
    _loadEnergyBuy(db)
    _loadArmysType(db)
    _loadSeowon(db)
    _loadBattleArrtTransform(db)
    _loadBattleArrt(db)
    
end


return clientTpl
