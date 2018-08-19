
-- 由工具自动生成，请不要手动修改

local M = {}

-----
-- 语言
M.LangType = {
    CN = 1,        -- 中文简体
    EN = 2,        -- 英文
    TW = 3,        -- 中文繁体
    KR = 4,        -- 韩文
    JP = 5,        -- 日文
    DE = 6,        -- 德文
    TH = 7,        -- 泰文
    VN = 8,        -- 越南文
}

-----
-- 登录结果
M.LoginResultType = {
    OK = 0,        -- 成功
    BAD_SESSION = 1,        -- 错误的认证
    UNKNOWN_ERROR = 2,        -- 未知错误
}

-----
-- 被踢下线的原因
M.KickType = {
    LOGIN_DUPLICATE = 1,        -- 帐号在别处登录
    FOUND_ERROR = 2,        -- 检查到帐号异常
    MAINTENANCE = 3,        -- 服务器维护
}

-----
-- 设置昵称结果
M.SetNicknameResultType = {
    OK = 0,        -- OK
    EXIST = 2,        -- 已经存在
    BAD_FORMAT = 3,        -- 存在非法字符
}

-----
-- 
M.AttributeType = {
    INFANTRY_ATTACK = 1001,        -- 步兵攻击加成
    CAVALRY_ATTACK = 1002,        -- 骑兵攻击加成
    ARCHER_ATTACK = 1003,        -- 弓兵攻击加成
    CHARIOT_ATTACK = 1004,        -- 车兵攻击加成
    INFANTRY_DEFENSE = 1005,        -- 步兵防御加成
    CAVALRY_DEFENSE = 1006,        -- 骑兵防御加成
    ARCHER_DEFENSE = 1007,        -- 弓兵防御加成
    CHARIOT_DEFENSE = 1008,        -- 车兵防御加成
    INFANTRY_HEALTH = 1009,        -- 步兵生命加成
    CAVALRY_HEALTH = 1010,        -- 骑兵生命加成
    ARCHER_HEALTH = 1011,        -- 弓兵生命加成
    CHARIOT_HEALTH = 1012,        -- 车兵生命加成
    ARCHER_DAMAGE = 1013,        -- 弓兵伤害加成
    DAMAGE_TO_INFANTRY_REDUCE = 1014,        -- 步兵受伤害减免
    CAVALRY_CHARGE_DAMAGE = 1015,        -- 骑兵冲锋伤害加成
    CHARIOT_SIEGE_ATTACK = 1016,        -- 车兵攻城战攻击加成
    TROOPS_ATTACK = 1017,        -- 部队攻击加成
    TROOPS_DEFENSE = 1018,        -- 部队防御加成
    TROOPS_HEALTH = 1019,        -- 部队生命加成
    TURRET_ATTACK = 1020,        -- 箭塔攻击加成
    TURRET_ATTACK_SPEED = 1021,        -- 箭塔攻击速度加成
    FIRE_ARROW_DAMAGE_TO_CAVALRY = 1022,        -- 火箭对骑兵伤害加成
    ROCKFALL_DAMAGE_TO_INFANTRY = 1023,        -- 落石对步兵伤害加成
    ROLLING_LOG_DAMAGE_TO_ARCHER = 1024,        -- 滚木对弓兵伤害加成
    MAX_WOUNDED_V = 1025,        -- 医疗帐篷数量上限增加
    MAX_WOUNDED = 1026,        -- 医疗帐篷数量上限百分比加成
    MAX_MARCHING_QUEUE = 1027,        -- 出征队列上限增加
    MAX_MARCHING_TROOPS_V = 1028,        -- 单支出征部队上限数量增加
    MAX_MARCHING_TROOPS = 1029,        -- 单支出征部队上限百分比加成
    ATTACK_MONSTER_MARCH_SPEED = 1030,        -- 打怪行军速度加成
    MARCH_SPEED = 1031,        -- 行军速度加成
    SCOUT_SPEED = 1032,        -- 侦查速度加成
    TROOP_LOAD_V = 1033,        -- 部队负重数量增加
    TROOP_LOAD = 1034,        -- 部队负重百分比加成
    WOOD_INCOME = 2001,        -- 粮食产量加成
    FOOD_INCOME = 2002,        -- 木材产量加成
    IRON_INCOME = 2003,        -- 铁产量加成
    MITHRIL_INCOME = 2004,        -- 秘银产量加成
    WOOD_GATHER_SPEED = 2005,        -- 粮食采集速度加成
    FOOD_GATHER_SPEED = 2006,        -- 木材采集速度加成
    IRON_GATHER_SPEED = 2007,        -- 铁采集速度加成
    MITHRIL_GATHER_SPEED = 2008,        -- 秘银采集速度加成
    GOLD_GATHER_SPEED = 2009,        -- 金币采集速度加成
    PROTECTED_RESOURCES_V = 2010,        -- 仓库容量增加
    UPKEEP_REDUCE = 2011,        -- 部队粮食消耗减少（百分比）
    GATHER_SPEED = 2012,        -- 世界采集资源速度提高（金币除外）
    COLLECT_INCOME = 2013,        -- 城中采集资源产量提高
    STAMINA_RECOVERY_SPEED = 3001,        -- 体力恢复速度加成
    BUILDING_SPEED = 3002,        -- 建筑建造速度加成
    TECHNOLOGY_RESEARCH_SPEED = 3003,        -- 科技研究速度加成
    TRAINING_SPEED = 3004,        -- 士兵训练速度加成
    HEAL_SPEED = 3006,        -- 伤兵恢复速度加成
    HEAL_RESOURCES = 3007,        -- 医疗所需资源减少
    MAX_TRAP_V = 5001,        -- 陷阱上限增加
    TRAINING_NUM_V = 5002,        -- 士兵训练量增加
    CITY_DEFENSE_V = 5003,        -- 城防值增加
    DEFENDER_ATTACK = 5004,        -- 守军攻击加成
    DEFENDER_DEFENSE = 5005,        -- 守军防御加成
    TRAP_ATTACK = 5006,        -- 陷阱攻击加成
    CONVERT_TO_WOUNDED = 5007,        -- 出征战斗中的伤兵转化率加成
    TRAP_BUILDING_SPEED = 5008,        -- 陷阱制造速度加成
}

-----
-- 
M.ItemType = {
    EQUIP = 1,        -- 装备
    PROP = 2,        -- 道具
}

-----
-- 
M.ItemEquipType = {
    WEAPON = 1,        -- 武器
    HELMET = 2,        -- 头盔
    ARMOR = 3,        -- 衣服
    LEG_ARMOR = 4,        -- 裤子
    MILITARY_BOOTS = 5,        -- 靴子
    RING = 6,        -- 戒子
}

-----
-- 
M.ItemPropType = {
    SPECIAL_ITEMS = 1,        -- 特殊物品
    GIFT = 2,        -- 礼包
    GOLD = 3,        -- 金币
    STAMINA = 4,        -- 体力
    VIP = 5,        -- VIP激活
    VIP_POINTS = 6,        -- VIP点数
    WOOD = 7,        -- 木头资源包
    FOOD = 8,        -- 粮食资源包
    IRON = 9,        -- 铁矿资源包
    MITHRIL = 10,        -- 秘银矿资源包
    SPEEDUP = 11,        -- 加速
    BUILDING_SPEEDUP = 12,        -- 建筑升级加速
    TROOP_TRAINING_SPEEDUP = 13,        -- 部队训练加速
    TROOP_MARCHING_SPEEDUP = 14,        -- 部队行军加速
    WOUNDED_RECOVERY_SPEEDUP = 15,        -- 伤兵恢复加速
    SCIENCE_RESEARCH_SPEEDUP = 16,        -- 科技研究加速
    FORGE_SPEEDUP = 17,        -- 锻造加速
    TRAPS_BUILDING_SPEEDUP = 18,        -- 陷阱建造加速
    MARCH_RECALL = 19,        -- 行军召回
    ADVANCED_RECALL = 20,        -- 高级召回
    SAWMILL_BOOST = 21,        -- 伐木场产量提升
    FRAM_BOOST = 22,        -- 农田产量提升
    IRON_MINE_BOOST = 23,        -- 铁矿产量提升
    MITHIL_MINE_BOOST = 24,        -- 秘银矿产量提升
    SMALL_UPKEEP_REDUCTION = 25,        -- 粮食消耗减少
    ATTACK_BONUS = 26,        -- 攻击加成
    DEFENSE_BONUS = 27,        -- 防御加成
    GATHER_BONUS = 28,        -- 采集速度加成
    PEACE_SHIELD = 29,        -- 战争守护
    NO_SCOUT = 30,        -- 反侦察 ?
    MARCH_LIMIT = 31,        -- 出征上限提升 ? 
    RANDOM_TELEPORT = 32,        -- 随机迁城
    ADVANCED_TELEPORT = 33,        -- 高级迁城
    LORD_RENAME = 34,        -- 领主改名
    CHANGE_APPREARANCE = 35,        -- 更换形象
}

-----
-- 
M.ResourceType = {
    FOOD = 1,        -- 粮食
    WOOD = 2,        -- 木材
    IRON = 3,        -- 铁
    MITHRIL = 4,        -- 秘银
}

-----
-- 特殊道具
M.SpecialPropIdType = {
    FOOD = 1,        -- 粮食
    WOOD = 2,        -- 木材
    IRON = 3,        -- 铁
    MITHRIL = 4,        -- 秘银
    GOLD = 5,        -- 金币
    STAMINA = 6,        -- 体力
    LORD_EXP = 7,        -- 领主经验
    VIP_EXP = 8,        -- VIP经验
}

-----
-- 建筑类型
M.BuildingType = {
    CASTLE = 1,        -- 城堡
    DEPOT = 2,        -- 仓库
    DRILL_GROUNDS = 3,        -- 校场
    BARRACKS = 4,        -- 兵营
    STABLES = 5,        -- 马厩
    RANGE = 6,        -- 靶场
    CHARIOT_PLANT = 7,        -- 战车工坊
    FORTRESS = 8,        -- 战争堡垒
    HALL_OF_WAR = 9,        -- 战争大厅
    EMBASSY = 10,        -- 大使馆
    MARKET = 11,        -- 市场
    WISHING_WELL = 12,        -- 许愿池
    COLLEGE = 13,        -- 学院
    BLACKSMITH = 14,        -- 铁匠铺
    WALLS = 15,        -- 城墙
    TURRET = 16,        -- 箭塔
    WATCH_TOWER = 17,        -- 瞭望塔
    MILITARY_TENT = 18,        -- 行军帐篷（已去除）
    HOSPITAL = 19,        -- 医院
    FARM = 20,        -- 农田
    SAWMILL = 21,        -- 伐木场
    IRON_MINE = 22,        -- 铁矿场
    MITHRIL_MINE = 23,        -- 秘银矿场
}

-----
-- 建筑状态
M.BuildingState = {
    NORMAL = 1,        -- 正常
    BUILDING = 2,        -- 创建中
    UPGRADING = 3,        -- 升级中
    DEMOLISHING = 4,        -- 拆除中
    FORGING = 5,        -- 锻造中
    RESEARCHING = 6,        -- 研究中
    TRAINING = 7,        -- 训练中/建造中
    CARING = 8,        -- 治疗中
}

-----
-- 建筑属性类型
M.BuildingAttributeType = {
    FOOD_PROTECT_LIMIT = 1,        -- 粮食保护上限
    IRON_PROTECT_LIMIT = 2,        -- 铁矿保护上限
    MITHRIL_PROTECT_LIMIT = 3,        -- 秘银保护上限
    TRAP_CAPACITY = 4,        -- 陷阱容量
    CITY_DEFENSE = 5,        -- 城防值
    MARCHING_TROOPS_LIMIT = 6,        -- 出征部队上限
    FORGING_SPEED = 7,        -- 锻造速度提高
    STELL_USE_REDUCE = 8,        -- 钢材消耗减少
    WOOD_OUTPUT = 9,        -- 木材产出（每小时）
    WOOD_CAPACITY = 10,        -- 木材容量
    FOOD_OUTPUT = 11,        -- 粮食产出（每小时）
    FOOD_CAPACITY = 12,        -- 粮食容量
    IRON_OUTPUT = 13,        -- 铁矿产出（每小时）
    IRON_CAPACITY = 14,        -- 铁矿容量
    MITHRIL_OUTPUT = 15,        -- 秘银产出（每小时）
    MITHRIL_CAPACITY = 16,        -- 秘银容量
    TRAINING_NUM = 17,        -- 士兵训练量
    TRAINING_SPEED = 18,        -- 训练速度提高
    WOUNDED_CAPACITY = 19,        -- 伤兵容量
    HELP_CD_TIME = 20,        -- 帮助减少时间
    REINFORCEMENT_CAPACITY = 21,        -- 援兵容纳量
    ASSEMBLED_TROOPS_NUM = 22,        -- 集结部队数目
    CARAVAN_WEIGHT_LIMIT = 23,        -- 商队负重上限
    GET_WOOD = 24,        -- 获得木材
    GET_FOOD = 25,        -- 获得粮食
    GET_IRON = 26,        -- 获得铁矿
    GET_MITHRIL = 27,        -- 获得秘银
    FREE_WISHING_TIMES = 28,        -- 免费许愿次数
    TURRET_ATTACK_FORCE = 29,        -- 箭塔攻击力
    TURRET_ATTACK_SPEED = 30,        -- 箭塔攻击速度
    TAX_RATE = 31,        -- 税率
    WOOD_RPOTECT_LIMIT = 32,        -- 木材保护上限
    HELP_TIMES = 33,        -- 帮助次数
}

-----
-- 兵种类型
M.ArmyType = {
    MONSTER = 0,        -- 野怪
    INFANTRY = 1,        -- 盾兵
    PIKEMAN = 2,        -- 枪兵
    CAVALRY_RIDER = 3,        -- 战骑
    CAVALRY_SHOOTER = 4,        -- 骑射
    ARCHER = 5,        -- 弓手
    CROSSBOWMAN = 6,        -- 弩手
    CHARIOT_SHORT = 7,        -- 近战车
    CHARIOT_LONG = 8,        -- 远程车
    ROCKFALL = 9,        -- 落石
    FIRE_ARROWS = 10,        -- 火箭
    ROLLING_LOGS = 11,        -- 滚木
}

-----
-- 士兵技能类型
M.ArmySkillType = {
    ATTACK = 1,        -- 攻击提高
    DEFENSE = 2,        -- 防御提高
    HP = 3,        -- 生命提高
    CHARGE = 4,        -- 冲锋
    SPURTING = 5,        -- 溅射
    ATTACK_SPEED = 6,        -- 攻击速度
    POWER_ATTACK = 7,        -- 致命一击(概率倍数攻击)
}

-----
-- 战斗类型
M.BattleType = {
    CASTLE = 1,        -- 城战
    CAMP = 2,        -- 营地战
    RESOURCE = 3,        -- 资源战
    MONSTER = 4,        -- 打怪
}

-----
-- 攻击类型
M.AttackType = {
    BOTH = 1,        -- 所有
    ATTACK = 2,        -- 攻击
    DEFENSE = 3,        -- 防守
}

-----
-- CD类型 道具加速时用来判断
M.CDType = {
    COMMON = 0,        -- 通用
    BUILD = 1,        -- 建造 升级
    FORGE = 2,        -- 锻造
    RESEARCH = 3,        -- 研究
    TRAIN = 4,        -- 训练
    HEAL = 5,        -- 治疗
    TRAP = 6,        -- 陷阱
}

-----
-- CD加速类型
M.CDSpeedUpType = {
    FREE = 1,        -- 免费
    GOLD = 2,        -- 金币
    PROP = 3,        -- 道具
}

-----
-- 军队状态
M.ArmyState = {
    NORMAL = 1,        -- 正常(守城)
    MARCHING = 2,        -- 出征
    WOUNDED = 3,        -- 受伤
    DIE = 4,        -- 死亡
    KILL = 5,        -- 杀敌数量
}

-----
-- 技能类型
M.SkillType = {
    COMBAT = 1,        -- 战斗
    DEVELOP = 2,        -- 发展
    SUPPORT = 3,        -- 辅助
}

-----
-- 地图中的元件
M.MapUnitType = {
    FARM_WOOD = 1,        -- 林场
    FARM_FOOD = 2,        -- 农田
    MINE_IRON = 3,        -- 铁矿
    MINE_MITHRIL = 4,        -- 秘银矿
    MINE_GOLD = 5,        -- 金矿
    RUINS = 6,        -- 遗迹
    MONSTER = 7,        -- 野怪
    PALACE = 8,        -- 王城
    CASTLE = 9,        -- 玩家城池
    CAMP = 10,        -- 扎营
}

-----
-- 城池状态
M.CastleState = {
    NORMAL = 0,        -- 正常
    PROTECTED = 1,        -- 保护
    BURN = 2,        -- 燃烧
}

-----
-- 地图中的队伍类型
M.MapTroopType = {
    SIEGE = 1,        -- 攻城
    RALLY = 2,        -- 集结
    GATHER = 3,        -- 采集
    MONSTER = 4,        -- 打怪
    CAMP = 5,        -- 扎营
    SCOUT = 6,        -- 侦查
    TRADE = 7,        -- 交易
    REINFORCEMENTS = 8,        -- 援兵
    DECLARE = 9,        -- 宣战
}

-----
-- 行军状态
M.MapTroopState = {
    MARCH = 1,        -- 出征
    REACH = 2,        -- 到达
    GO_HOME = 3,        -- 返回
    RALLYING = 4,        -- 集结中
}






    
M.CS_TEMPLATE_CHECK = 1
M.CS_LOGIN = 5
M.CS_PING_REPLY = 9
M.CS_SET_OR_CHECK_NICKNAME = 20
M.CS_SET_LANG_TYPE = 22
M.CS_BAG_USE = 51
M.CS_BUILDING_CREATE = 202
M.CS_BUILDING_UPGRADE = 204
M.CS_BUILDING_UPGRADE_CANCEL = 206
M.CS_BUILDING_MOVE = 208
M.CS_BUILDING_DEMOLISH = 210
M.CS_BUILDING_DEMOLISH_CANCEL = 212
M.CS_BUILDING_TRAIN = 214
M.CS_BUILDING_TRAIN_CANCEL = 216
M.CS_BUILDING_HEAL = 218
M.CS_BUILDING_HEAL_CANCEL = 220
M.CS_BUILDING_COLLECT = 222
M.CS_BUILDING_COLLECT_BOOST = 224
M.CS_BUILDING_OPEN_BUILDER2 = 226
M.CS_BUILDING_OPEN_FOREST = 228
M.CS_BUILDING_TECHNOLOGY_RESEARCH = 230
M.CS_CDLIST_SPEED_UP = 302
M.CS_ARMY_DISMISS = 352
M.CS_SKILL_RESET = 402
M.CS_SKILL_ADD_POINT = 404
M.CS_MAP_VIEW = 1000
M.CS_MAP_SWITCH = 1005
M.CS_MAP_JOIN = 1010
M.CS_MAP_LEAVE = 1012
M.CS_MAP_TELEPORT = 1014
M.CS_MAP_MARCH = 1016
M.CS_MAP_GO_HOME = 1018
M.CS_MAP_CAMP_MARCH = 1020
M.CS_MAP_RECALL = 1022
M.CS_MAP_SPEED_UP = 1024


    
M.SC_TEMPLATE_UPDATE = 2
M.SC_LOGIN_RESPONSE = 6
M.SC_LOGOUT = 7
M.SC_PING = 8
M.SC_PING_RESULT = 10
M.SC_READY = 11
M.SC_SET_OR_CHECK_NICKNAME_RESPONSE = 21
M.SC_SET_LANG_TYPE_RESPONSE = 23
M.SC_NOTICE_MESSAGE = 24
M.SC_USER_UPDATE = 30
M.SC_ATTR_PLUS_UPDATE = 31
M.SC_BAG_UPDATE = 50
M.SC_BAG_USE_RESPONSE = 52
M.SC_BUILDING_UPDATE = 201
M.SC_BUILDING_CREATE_RESPONSE = 203
M.SC_BUILDING_UPGRADE_RESPONSE = 205
M.SC_BUILDING_UPGRADE_CANCEL_RESPONSE = 207
M.SC_BUILDING_MOVE_RESPONSE = 209
M.SC_BUILDING_DEMOLISH_RESPONSE = 211
M.SC_BUILDING_DEMOLISH_CANCEL_RESPONSE = 213
M.SC_BUILDING_TRAIN_RESPONSE = 215
M.SC_BUILDING_TRAIN_CANCEL_RESPONSE = 217
M.SC_BUILDING_HEAL_RESPONSE = 219
M.SC_BUILDING_HEAL_CANCEL_RESPONSE = 221
M.SC_BUILDING_COLLECT_RESPONSE = 223
M.SC_BUILDING_COLLECT_BOOST_RESPONSE = 225
M.SC_BUILDING_OPEN_BUILDER2_RESPONSE = 227
M.SC_BUILDING_OPEN_FOREST_RESPONSE = 229
M.SC_BUILDING_TECHNOLOGY_RESEARCH_RESPONSE = 231
M.SC_CDLIST_UPDATE = 301
M.SC_CDLIST_SPEED_UP_RESPONSE = 303
M.SC_ARMY_INFOS_UPDATE = 351
M.SC_ARMY_DISMISS_RESPONSE = 353
M.SC_SKILL_UPDATE = 401
M.SC_SKILL_RESET_RESPONSE = 403
M.SC_SKILL_ADD_POINT_RESPONSE = 405
M.SC_TECHNOLOGY_INFOS_UPDATE = 451
M.SC_MAP_UNIT_UPDATE = 1001
M.SC_MAP_TROOP_UPDATE = 1002
M.SC_MAP_PERSONAL_INFO_UPDATE = 1003
M.SC_MAP_TELEPORT_RESPONSE = 1015



return M

