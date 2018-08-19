local p = require('protocol')
local t = require('tploader')
local timer = require('timer')
local misc = require('libs/misc')
local utils = require('utils')

local hero = {
    --武将信息
    heroInfo = {
        --基础
        id = 0,     --武将ID
        tpl = {},
        exp = 0,    --武将当前经验
        level = 0,  --武将等级
        star = 0,   --武将星级
        soulLevel = 0,  --武将命魂阶级  
        physical = 0,   --武将体力
        physicalRecoveryTimeStamp = 0,  --武将恢复时间戳

        -- 武将一级属性
        heroPower = 0,   --武将武力
        heroDefense = 0,   --武将统帅
        heroWisdom = 0,   --武将智力
        heroLucky = 0,   --武将运气
        heroSkill = 0,   --武将士气
        heroAgile = 0,   --武将速度
        heroLife = 0,   --武将攻城

        -- 武将二级属性
        heroPhysicalPower = 0,   --武将物理攻击力
        heroPhysicalDefense = 0,    --武将物理防御力
        heroSkillPower = 0,     --武将谋略攻击力
        heroSkillDefense = 0,    --武将谋略防御力
        heroHit = 0,             --武将命中值
        heroAvoid = 0,          --武将回避值
        heroCritHit = 0,        --武将暴击命中值
        heroCritAvoid = 0,      --武将暴击回避值
        heroSpeed = 0,          --武将攻击速度
        heroCityLife = 0,       --武将攻城值

        heroSolohp = 0,   --武将体力  (HERO_FIGHT) 
        heroTroops = 0,   --武将领导力 (HERO_SINGLE_ENERGY)

        -- 
        slotNum = 0,     --特殊技开放的槽位

        additionList = {},  --属性加成列表{ [AttributeType] = { [AttributeAdditionType]=0, [AttributeAdditionType]=0, [AttributeAdditionType]=0 },...}

        secondAttrAddtionList = {},   --二级属性加成列表

        isLock = false,  --是否锁定
        --技能
        skill = {},     --<skillId, skillInfo>


        --命魂
        soul = {},        --<soulId, soulInfo>

        isSync = false,
        isDirty = false,
    },

    skillInfo = {
        tplId = 0,  --技能ID
        type = 0,   --技能类型HeroSkillType
        level = 0,
        slot = 0,       --星级技能上阵的位置
        isSync = false,
    },

    soulInfo = {
        tplId = 0,   -- 命魂属性Id
        soulLevel = 0,   -- 命魂阶级
        level = 0,    -- 属性升级次数
        isSync = false,
    },
}

function hero.heroInfo:new(o)
    o = o or {}
    if o.skill == nil then o.skill = {} end
    if o.soul == nil then o.soul = {} end
    setmetatable(o, self)
    self.__index = self
    return o
end

function hero.newHeroInfo(o)
    if o == nil then
        o = {}
    end

    -- print("--- hero------- id ", o.id)

    local tpl = t.heros[o.id]
    if tpl == nil then
        print ("hero.newHeroInfo id = ", o.id)
        return nil
    end
    local tempHero = {}
    tempHero.id = o.id
    tempHero.tpl = tpl
    tempHero.exp = o.exp or 0
    tempHero.level = o.level or tpl.level
    tempHero.star = o.star or tpl.star
    tempHero.soulLevel = o.soulLevel or 0
    -- tempHero.physical = math.floor(o.physical or t.configure["heroPhysical"].initial + tempHero.level * t.configure["heroPhysical"].increase)
    tempHero.physicalRecoveryTimeStamp = timer.getTimestampCache()
    tempHero.slotNum = o.slotNum or 0
    tempHero.isLock = o.isLock or false

    local info = hero.heroInfo:new(tempHero)
    info.isSync = false
    info.isDirty = true

    if o.skill and next(o.skill) then
        for skillId, v in pairs(o.skill) do
            info.skill[skillId] = hero.skillInfo:new(v)
        end
    else 
        if tpl.nNirvanaSkills ~= 0 then
            info.skill[tpl.nNirvanaSkills] = hero.skillInfo:new({tplId = tpl.nNirvanaSkills, type = p.HeroSkillType.HERO_SKILL_TALENT, level = 1, slot = 0})
        end
        -- 星级技能
        if tpl.ntechnicalSkill1 ~= 0 then
            info.skill[tpl.ntechnicalSkill1] = hero.skillInfo:new({tplId = tpl.ntechnicalSkill1, type = p.HeroSkillType.HERO_SKILL_STAR, level = 1, slot = 0})
        end
        if tpl.ntechnicalSkill2 ~= 0 then
            info.skill[tpl.ntechnicalSkill2] = hero.skillInfo:new({tplId = tpl.ntechnicalSkill2, type = p.HeroSkillType.HERO_SKILL_STAR, level = 1, slot = 0})
        end
        if tpl.ntechnicalSkill3 ~= 0 then
            info.skill[tpl.ntechnicalSkill3] = hero.skillInfo:new({tplId = tpl.ntechnicalSkill3, type = p.HeroSkillType.HERO_SKILL_STAR, level = 1, slot = 0})
        end
        if tpl.ntechnicalSkill4 ~= 0 then
            info.skill[tpl.ntechnicalSkill4] = hero.skillInfo:new({tplId = tpl.ntechnicalSkill4, type = p.HeroSkillType.HERO_SKILL_STAR, level = 1, slot = 0})
        end
        if tpl.ntechnicalSkill5 ~= 0 then
            info.skill[tpl.ntechnicalSkill5] = hero.skillInfo:new({tplId = tpl.ntechnicalSkill5, type = p.HeroSkillType.HERO_SKILL_STAR, level = 1, slot = 0})
        end
        if tpl.ntechnicalSkill6 ~= 0 then
            info.skill[tpl.ntechnicalSkill6] = hero.skillInfo:new({tplId = tpl.ntechnicalSkill6, type = p.HeroSkillType.HERO_SKILL_STAR, level = 1, slot = 0})
        end
        if tpl.ntechnicalSkill7 ~= 0 then
            info.skill[tpl.ntechnicalSkill7] = hero.skillInfo:new({tplId = tpl.ntechnicalSkill7, type = p.HeroSkillType.HERO_SKILL_STAR, level = 1, slot = 0})
        end
        if tpl.ntechnicalSkill8 ~= 0 then
            info.skill[tpl.ntechnicalSkill8] = hero.skillInfo:new({tplId = tpl.ntechnicalSkill8, type = p.HeroSkillType.HERO_SKILL_STAR, level = 1, slot = 0})
        end

    end
    -- 填充命魂
    if o.soul and next(o.soul) then
        for soulId, v in pairs(o.soul) do
            info.soul[soulId] = hero.soulInfo:new(v)
        end
    else
        for i = 0, info.soulLevel do
            for _, tpl in pairs(t.heroSoulLib[info.tpl.nDamageType][i]) do
                info.soul[tpl.id] = hero.soulInfo:new({tplId = tpl.id, soulLevel = tpl.soulLevel, level = 0})
            end
        end
    end
    -- 设置技能槽位
    info:checkSkillSlot()
    --设置属性
    info:setHeroBaseProperty()
    return info
end

-- skill begin

function hero.skillInfo:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end

function hero.heroInfo:checkSkillSlot()
    local info = self
    if not info then
        return
    end
    self.slotNum = 0
    -- print("self.tpl.starHoleUnlock", utils.serialize(self.tpl.starHoleUnlock))
    if self.tpl.starHoleUnlock then
        for _,v in pairs(self.tpl.starHoleUnlock) do
            if self.star >= v then
                self.slotNum = self.slotNum + 1
            end
        end
    end
    if self.slotNum >= self.tpl.starHole then
        self.slotNum = self.tpl.starHole
    end
    -- print("self.slotNum", self.slotNum)
end

function hero.heroInfo:getHeroSkill(bAll)
    if bAll == nil then
        bAll = true
    end
    local list = {}
    for slot, v in pairs(self.skill) do
        if bAll then
            list[slot] = { tplId = v.tplId, type = v.type, level = v.level, exp = v.exp }
        else
            if v.isOpen then
                list[slot] = { tplId = v.tplId, type = v.type, level = v.level, exp = v.exp }
            end
        end
    end
    return list
end

-- skill end

-- soul begin

function hero.soulInfo:new(o)
    o = o or {}
    setmetatable(o, self)
    self.__index = self
    return o
end

--soul end

function hero.heroInfo:getHeroAdditionList()
    local list = {}
    for attrType, v in pairs(self.additionList) do
        list[attrType] = {
            [p.AttributeAdditionType.BASE] = v[p.AttributeAdditionType.BASE],
            [p.AttributeAdditionType.PCT] = v[p.AttributeAdditionType.PCT],
            [p.AttributeAdditionType.EXT] = v[p.AttributeAdditionType.EXT],
        }
    end
    return list
end

function hero.heroInfo:getHeroSecondAdditionList()
    local list = {}
    for attrType, v in pairs(self.secondAttrAddtionList) do
        list[attrType] = {
            [p.AttributeAdditionType.BASE] = v[p.AttributeAdditionType.BASE],
            [p.AttributeAdditionType.PCT] = v[p.AttributeAdditionType.PCT],
            [p.AttributeAdditionType.EXT] = v[p.AttributeAdditionType.EXT],
        }
    end
    return list
end

--property begin
--武将属性=（武将基础属性+升级属性*当前等级*（1+升星属性）+ 被动技能 + 羁绊技能 + 命魂属性）*（1+全局BUFF）+额外数值
function hero.heroInfo:setHeroBaseProperty()
    -- 星级和武将等级只增加一级属性和部分二级属性(体力，领导力)
    local addAttrList = t.getHeroAddAttrByLevelStar(self.id, self.level, self.star)
    -- print("addAttrListaddAttrListaddAttrListaddAttrList", utils.serialize(addAttrList))
    self.heroPower = self.tpl.nInitialPower + (addAttrList.addPower or 0)        --武将武力
    self.heroDefense = self.tpl.nInitialDefense + (addAttrList.addDefense or 0)     --武将统帅 
    self.heroWisdom = self.tpl.nInitialWisdom + (addAttrList.addWisdom or 0)      --武将智力
    self.heroLucky = self.tpl.nInitialLucky + (addAttrList.addLucky or 0)        --武将运气
    self.heroSkill = self.tpl.nInitialSkill + (addAttrList.addSkill or 0)       --武将士气
    self.heroAgile = self.tpl.nInitialAgile + (addAttrList.addAgile or 0)        --武将速度
    self.heroLife = self.tpl.nInitialLife + (addAttrList.addLife or 0)        --武将攻城
    self.heroSolohp = math.floor(self.tpl.nInitialSolohp + (addAttrList.addSolohp or 0))      --武将体力
    self.heroTroops = math.floor(self.tpl.nInitialTroops + (addAttrList.addTroops or 0))      --武将领导力

    local physicalAttackUp = 0
    local physicalDefenseUp = 0
    local wisdomAttackUp = 0
    local wisdomDefenseUp = 0
    local hitUp = 0
    local avoidUp = 0
    local critHitUp = 0
    local critAvoidUp = 0
    local speedUp = 0
    local cityLifeUp = 0
    
    local additionList = {
        [p.AttributeType.HERO_POWER] =  { [p.AttributeAdditionType.BASE] = self.heroPower,    [p.AttributeAdditionType.PCT] = 0, [p.AttributeAdditionType.EXT] = 0 },
        [p.AttributeType.HERO_DEFENSE] =  { [p.AttributeAdditionType.BASE] = self.heroDefense,  [p.AttributeAdditionType.PCT] = 0, [p.AttributeAdditionType.EXT] = 0 },
        [p.AttributeType.HERO_WISDOM] =  { [p.AttributeAdditionType.BASE] = self.heroWisdom,      [p.AttributeAdditionType.PCT] = 0, [p.AttributeAdditionType.EXT] = 0 },
        [p.AttributeType.HERO_SKILL] =   { [p.AttributeAdditionType.BASE] = self.heroSkill,    [p.AttributeAdditionType.PCT] = 0, [p.AttributeAdditionType.EXT] = 0 },
        [p.AttributeType.HERO_AGILE] =   { [p.AttributeAdditionType.BASE] = self.heroAgile,  [p.AttributeAdditionType.PCT] = 0, [p.AttributeAdditionType.EXT] = 0 },
        [p.AttributeType.HERO_LUCKY] =  { [p.AttributeAdditionType.BASE] = self.heroLucky,   [p.AttributeAdditionType.PCT] = 0, [p.AttributeAdditionType.EXT] = 0 },
        [p.AttributeType.HERO_LIFE] =   { [p.AttributeAdditionType.BASE] = self.heroLife,    [p.AttributeAdditionType.PCT] = 0, [p.AttributeAdditionType.EXT] = 0 },
        [p.AttributeType.HERO_FIGHT] = { [p.AttributeAdditionType.BASE] = self.heroTroops,    [p.AttributeAdditionType.PCT] = 0, [p.AttributeAdditionType.EXT] = 0 },
        [p.AttributeType.HERO_SINGLE_ENERGY] =  { [p.AttributeAdditionType.BASE] = self.heroSolohp,     [p.AttributeAdditionType.PCT] = 0, [p.AttributeAdditionType.EXT] = 0 },
    }

    local secondAttrAddtionList = {
        [p.AttributeType.HERO_PHYSICAL_ATTACK] =   { [p.AttributeAdditionType.BASE] = physicalAttackUp,  [p.AttributeAdditionType.PCT] = 0, [p.AttributeAdditionType.EXT] = 0 },
        [p.AttributeType.HERO_PHYSICAL_DEFENSE] =   { [p.AttributeAdditionType.BASE] = physicalDefenseUp,   [p.AttributeAdditionType.PCT] = 0, [p.AttributeAdditionType.EXT] = 0 },
        [p.AttributeType.HERO_WISDOM_ATTACK] =   { [p.AttributeAdditionType.BASE] = wisdomAttackUp,  [p.AttributeAdditionType.PCT] = 0, [p.AttributeAdditionType.EXT] = 0 },
        [p.AttributeType.HERO_WISDOM_DEFENSE] =   { [p.AttributeAdditionType.BASE] = wisdomDefenseUp,   [p.AttributeAdditionType.PCT] = 0, [p.AttributeAdditionType.EXT] = 0 },
        [p.AttributeType.HERO_HIT] =  { [p.AttributeAdditionType.BASE] = hitUp,   [p.AttributeAdditionType.PCT] = 0, [p.AttributeAdditionType.EXT] = 0 },
        [p.AttributeType.HERO_AVOID] =  { [p.AttributeAdditionType.BASE] = avoidUp,    [p.AttributeAdditionType.PCT] = 0, [p.AttributeAdditionType.EXT] = 0 },
        [p.AttributeType.HERO_CRIT_HIT] =  { [p.AttributeAdditionType.BASE] = critHitUp,   [p.AttributeAdditionType.PCT] = 0, [p.AttributeAdditionType.EXT] = 0 },
        [p.AttributeType.HERO_CRIT_AVOID] =  { [p.AttributeAdditionType.BASE] = critAvoidUp,  [p.AttributeAdditionType.PCT] = 0, [p.AttributeAdditionType.EXT] = 0 },
        [p.AttributeType.HERO_SPEED] =  { [p.AttributeAdditionType.BASE] = speedUp,   [p.AttributeAdditionType.PCT] = 0, [p.AttributeAdditionType.EXT] = 0 },
        [p.AttributeType.HERO_CITY_LIFE] =  { [p.AttributeAdditionType.BASE] = cityLifeUp,   [p.AttributeAdditionType.PCT] = 0, [p.AttributeAdditionType.EXT] = 0 },
    }

    --list :{1001,1,5}
    local function setAttributeValue(attributeType, attrAddType, value)
        -- print('----attributeType, attrAddType, value----------', attributeType, attrAddType, value)
        if additionList[attributeType] then
            local addition = additionList[attributeType][attrAddType]
            if addition then
                addition = addition + value
                additionList[attributeType][attrAddType] = addition
            end
        end
    end
    -- 命魂属性
    for _, v in pairs(self.soul) do
        setAttributeValue(t.heroSoul[v.tplId].attrList[1], t.heroSoul[v.tplId].attrList[2], t.heroSoul[v.tplId].attrList[3] *  v.level)
    end
    -- 被动技能和羁绊技能
    for _, v in pairs(self.skill) do
        if v.type == p.HeroSkillType.HERO_SKILL_PASSIVE or v.type == p.HeroSkillType.HERO_SKILL_FETTER then
            local skillInfo = t.skillPreBattle[v.tplId]
            if skillInfo then
                setAttributeValue(skillInfo.attrList[1], skillInfo.attrList[2], skillInfo.attrList[3])
            end
        end
    end
    self.additionList = additionList
    self.secondAttrAddtionList = secondAttrAddtionList
    -- self:dump()
end

-- 单挑武将战斗力  
function hero.heroInfo:getSingleHeroPower(propList)
    -- 先虚拟一个 TODO by zds
    local attrList = self:getHeroProperty(self.additionList, self.secondAttrAddtionList, 0, 0, propList)
    return attrList.heroPhysicalPower * t.battleArrt[p.AttributeType.HERO_PHYSICAL_ATTACK].battlePower + 
    attrList.heroPhysicalDefense * t.battleArrt[p.AttributeType.HERO_PHYSICAL_DEFENSE].battlePower + 
    attrList.heroSolohp * t.battleArrt[p.AttributeType.HERO_SINGLE_ENERGY].battlePower
end

-- 常规武将战斗力

function hero.heroInfo:getHeroPower(propList)
    local attrList = self:getHeroProperty(self.additionList, self.secondAttrAddtionList, 0, 0, propList)
    return attrList.heroPhysicalPower * t.battleArrt[p.AttributeType.HERO_PHYSICAL_ATTACK].battlePower +
    attrList.heroPhysicalDefense * t.battleArrt[p.AttributeType.HERO_PHYSICAL_DEFENSE].battlePower +
    attrList.heroSkillPower * t.battleArrt[p.AttributeType.HERO_WISDOM_ATTACK].battlePower + 
    attrList.heroSkillDefense * t.battleArrt[p.AttributeType.HERO_WISDOM_DEFENSE].battlePower + 
    attrList.heroHit * t.battleArrt[p.AttributeType.HERO_HIT].battlePower + 
    attrList.heroAvoid * t.battleArrt[p.AttributeType.HERO_AVOID].battlePower + 
    attrList.heroCritHit * t.battleArrt[p.AttributeType.HERO_CRIT_HIT].battlePower + 
    attrList.heroCritAvoid * t.battleArrt[p.AttributeType.HERO_CRIT_AVOID].battlePower + 
    attrList.heroSpeed * t.battleArrt[p.AttributeType.HERO_SPEED].battlePower + 
    attrList.heroCityLife * t.battleArrt[p.AttributeType.HERO_CITY_LIFE].battlePower
end
--property end

function hero.heroInfo:getHeroProperty(additionList, secondAdditionList, armyType, armyLevel, propAttrList)
    local attrList = {
        heroPower = 0,
        heroDefense = 0,
        heroWisdom = 0,
        heroLucky = 0,
        heroSkill = 0,
        heroAgile = 0,
        heroLife = 0,
        heroPhysicalPower = 0,
        heroPhysicalDefense = 0,
        heroSkillPower = 0,
        heroSkillDefense = 0, 
        heroHit = 0,
        heroAvoid = 0,
        heroCritHit = 0,
        heroCritAvoid = 0,
        heroSpeed = 0,
        heroCityLife = 0,
        heroFight = 0,
        heroSolohp = 0,
    }

    local firstAttrList = {}
    for k, v in pairs(additionList) do
        local base = v[p.AttributeAdditionType.BASE] or 0
        local pct = v[p.AttributeAdditionType.PCT] or 0
        local ext = v[p.AttributeAdditionType.EXT] or 0
        --全局属性
        if propAttrList then
            for attributeType, attr in pairs(propAttrList) do
                if k == attributeType then
                    local tempBase = attr[p.AttributeAdditionType.BASE] or 0
                    local tempPct = attr[p.AttributeAdditionType.PCT] or 0
                    local tempExt = attr[p.AttributeAdditionType.EXT] or 0
                    base = base + tempBase
                    pct = pct + tempPct
                    ext = ext + tempExt
                end
            end
        end
        pct = pct/10000
        if k == p.AttributeType.HERO_POWER then
            attrList.heroPower = attrList.heroPower + base
            attrList.heroPower = attrList.heroPower * (1 + pct) + ext
            firstAttrList[k] = attrList.heroPower
        elseif k == p.AttributeType.HERO_DEFENSE then
            attrList.heroDefense = attrList.heroDefense + base
            attrList.heroDefense = attrList.heroDefense * (1 + pct) + ext
            firstAttrList[k] = attrList.heroDefense
        elseif k == p.AttributeType.HERO_WISDOM then
            attrList.heroWisdom = attrList.heroWisdom + base
            attrList.heroWisdom = attrList.heroWisdom * (1 + pct) + ext
            firstAttrList[k] = attrList.heroWisdom
        elseif k == p.AttributeType.HERO_SKILL then
            attrList.heroSkill = attrList.heroSkill + base
            attrList.heroSkill = attrList.heroSkill * (1 + pct) + ext
            firstAttrList[k] = attrList.heroSkill
        elseif k == p.AttributeType.HERO_AGILE then
            attrList.heroAgile = attrList.heroAgile + base
            attrList.heroAgile = attrList.heroAgile * (1 + pct) + ext
            firstAttrList[k] = attrList.heroAgile
        elseif k == p.AttributeType.HERO_LUCKY then
            attrList.heroLucky = attrList.heroLucky + base
            attrList.heroLucky = attrList.heroLucky * (1 + pct) + ext
            firstAttrList[k] = attrList.heroLucky
        elseif k == p.AttributeType.HERO_LIFE then
            attrList.heroLife = attrList.heroLife + base
            attrList.heroLife = attrList.heroLife * (1 + pct) + ext
            firstAttrList[k] = attrList.heroLife
        elseif k == p.AttributeType.HERO_FIGHT then
            attrList.heroFight = attrList.heroFight + base
            attrList.heroFight = attrList.heroFight * (1 + pct) + ext
            firstAttrList[k] = attrList.heroFight
        elseif k == p.AttributeType.HERO_SINGLE_ENERGY then
            attrList.heroSolohp = attrList.heroSolohp + base
            attrList.heroSolohp = attrList.heroSolohp * (1 + pct) + ext
            firstAttrList[k] = attrList.heroSolohp
        end
    end
    -- print("###firstAttrSerialize...", utils.serialize(attrList))
    -- 部队属性加成
    local armysTpl = t.findArmysTpl(armyType, armyLevel)
    if armysTpl then
        for _,v in pairs(armysTpl.arrtValue) do
            local attrType = v[1]
            local attrAddType = v[2]
            local attrValue = v[3]
            secondAdditionList[attrType][attrAddType] = attrValue
        end
    end
    -- print("###secondAttrSerialize, army...", utils.serialize(attrList))
    -- print("###secondAttrSerialize...", utils.serialize(secondAdditionList))
    if not secondAdditionList then
        print(debug.traceback())
    end
    for k,v in pairs(secondAdditionList) do
        local base = v[p.AttributeAdditionType.BASE] or 0
        local pct = v[p.AttributeAdditionType.PCT] or 0
        local ext = v[p.AttributeAdditionType.EXT] or 0
        --全局属性
        if propAttrList then
            for attributeType, attr in pairs(propAttrList) do
                if k == attributeType then
                    local tempBase = attr[p.AttributeAdditionType.BASE] or 0
                    local tempPct = attr[p.AttributeAdditionType.PCT] or 0
                    local tempExt = attr[p.AttributeAdditionType.EXT] or 0
                    base = base + tempBase
                    pct = pct + tempPct
                    ext = ext + tempExt
                end
            end
        end
        pct = pct/10000
        local transformTpl = t.battleArrtTransform[k]
        if k == p.AttributeType.HERO_PHYSICAL_ATTACK then
            attrList.heroPhysicalPower = firstAttrList[transformTpl.firstArrt] * transformTpl.transformNum
            attrList.heroPhysicalPower = attrList.heroPhysicalPower + base
            attrList.heroPhysicalPower = attrList.heroPhysicalPower * (1 + pct) + ext
        elseif k == p.AttributeType.HERO_PHYSICAL_DEFENSE then
            attrList.heroPhysicalDefense = firstAttrList[transformTpl.firstArrt] * transformTpl.transformNum
            attrList.heroPhysicalDefense = attrList.heroPhysicalDefense + base
            attrList.heroPhysicalDefense = attrList.heroPhysicalDefense * (1 + pct) + ext
        elseif k == p.AttributeType.HERO_WISDOM_ATTACK then
            attrList.heroSkillPower = firstAttrList[transformTpl.firstArrt] * transformTpl.transformNum
            attrList.heroSkillPower = attrList.heroSkillPower + base
            attrList.heroSkillPower = attrList.heroSkillPower * (1 + pct) + ext
        elseif k == p.AttributeType.HERO_WISDOM_DEFENSE then
            attrList.heroSkillDefense = firstAttrList[transformTpl.firstArrt] * transformTpl.transformNum
            attrList.heroSkillDefense = attrList.heroSkillDefense + base
            attrList.heroSkillDefense = attrList.heroSkillDefense * (1 + pct) + ext
        elseif k == p.AttributeType.HERO_HIT then
            attrList.heroHit = firstAttrList[transformTpl.firstArrt] * transformTpl.transformNum
            attrList.heroHit = attrList.heroHit + base
            attrList.heroHit = attrList.heroHit * (1 + pct) + ext
        elseif k == p.AttributeType.HERO_AVOID then
            attrList.heroAvoid = firstAttrList[transformTpl.firstArrt] * transformTpl.transformNum
            attrList.heroAvoid = attrList.heroAvoid + base
            attrList.heroAvoid = attrList.heroAvoid * (1 + pct) + ext
        elseif k == p.AttributeType.HERO_CRIT_HIT then
            attrList.heroCritHit = firstAttrList[transformTpl.firstArrt] * transformTpl.transformNum
            attrList.heroCritHit = attrList.heroCritHit + base
            attrList.heroCritHit = attrList.heroCritHit * (1 + pct) + ext
        elseif k == p.AttributeType.HERO_CRIT_AVOID then
            attrList.heroCritAvoid = firstAttrList[transformTpl.firstArrt] * transformTpl.transformNum
            attrList.heroCritAvoid = attrList.heroCritAvoid + base
            attrList.heroCritAvoid = attrList.heroCritAvoid * (1 + pct) + ext
        elseif k == p.AttributeType.HERO_SPEED then
            attrList.heroSpeed = firstAttrList[transformTpl.firstArrt] * transformTpl.transformNum
            attrList.heroSpeed = attrList.heroSpeed + base
            attrList.heroSpeed = attrList.heroSpeed * (1 + pct) + ext
        elseif k == p.AttributeType.HERO_CITY_LIFE then
            attrList.heroCityLife = firstAttrList[transformTpl.firstArrt] * transformTpl.transformNum
            attrList.heroCityLife = attrList.heroCityLife + base
            attrList.heroCityLife = attrList.heroCityLife * (1 + pct) + ext
        end
    end
    -- print("###secondAttrSerialize...", utils.serialize(attrList))
    return attrList
end

function hero.heroInfo:dump()
    print('============hero begin===============')
    print('heroId, level, exp, star, soul', self.id, self.level, self.exp, self.star, self.soulLevel)
    print('heroPower,heroDefense,heroWisdom,heroLucky,heroSkill,heroAgile,heroLife,heroSolohp,heroTroops', self.heroPower,self.heroDefense,self.heroWisdom,self.heroLucky,self.heroSkill,self.heroAgile,self.heroLife,self.heroSolohp,self.heroTroops)
    print('skill: ', utils.serialize(self.skill))
    print('soul: ', utils.serialize(self.soul))
    print('additionList: ', utils.serialize(self.additionList))
    print('============hero end=================')
end


return hero