
-- 由工具自动生成，请不要手动修改

local M = {}

-----
-- 物品操作日志类型
M.LogItemType = {
    GAIN = 1,        -- 获得
    CONSUME = 2,        -- 消耗
}

-----
-- 资源获得类型
M.ResourceGainType = {
    DEFAULT = 0,        -- 其它
    MAIL = 1,        -- 邮箱收取
    BOX_DROP = 2,        -- 宝箱掉落
    QUEST = 3,        -- 任务获得
    MONSTER_DROP = 4,        -- 打怪掉落
    COLLECT = 5,        -- 收集
    GATHER = 6,        -- 采集
    TAX = 7,        -- 税收
    VIP_BOX = 8,        -- VIP宝箱
    ACHIEVEMENT = 9,        -- 成就
    CATAPULT_DROP = 10,        -- 箭塔掉落
    COMPENSATE = 11,        -- 战报损兵补偿
    BAG_MELT = 34,        -- 背包熔炼
    BAG_SYNTHESIZE = 35,        -- 背包合成(技能书 装备 宝物)
    BAG_SELL = 36,        -- 背包出售
    HERO_DRAW = 37,        -- 武将抽卡
    SCENARIO_COPY = 38,        -- 副本
    EQUIP_INLAY_OFF = 39,        -- 装备镶嵌(宝石)卸下
    STORE_BUY = 40,        -- 商店购买
    ARENA = 41,        -- 擂台
    BABEL = 42,        -- 千层楼
    BRONZE = 43,        -- 铜雀台
    PLUNDER = 50,        -- 掠夺
    CITY_DROP = 51,        -- 名城掉落
    CAMP_FIXED_FAILED = 52,        -- 行营失败
    ALLIANCE_HIRE_HERO = 53,        -- 武将租借
    ALLIANCE_DONATE = 54,        -- 联盟捐献
    OPEN_TREASURE_BOX = 55,        -- 开启宝箱
    ACTIVITY_ACCUMULATE_LOGIN = 100,        -- 累计登录（七日签到）
    ACTIVITY_LOGIN_EVERYDAY = 101,        -- 每日登录
    ACTIVITY_TIME_MEAL = 102,        -- 限时大餐
    ACTIVITY_TARGET = 103,        -- 目标活动
    ACTIVITY_EXCHANGE = 104,        -- 兑换活动
    TRANSPORT = 105,        -- 运输获得
    TRANSPORT_FAIL = 106,        -- 运输失败获得
}

-----
-- 资源消耗类型
M.ResourceConsumeType = {
    DEFAULT = 0,        -- 其它
    STORE_BUY = 1,        -- 商店购买
    BUILDING_CREATE = 2,        -- 建筑创建
    BUILDING_UPGRADE = 3,        -- 建筑升级
    ARMY_CONSUME = 4,        -- 兵种消耗
    STORE_REFRESH = 5,        -- 商店刷新
    VIP_BOX = 6,        -- 购买VIP宝箱
    ARMY_UPKEEP = 15,        -- 养兵
    TRAINNING = 16,        -- 训练
    HEAL = 17,        -- 治疗
    TECHNOLOGY_RESEARCH = 18,        -- 科技研究
    OPEN_BUILDER2 = 19,        -- 开通第二建筑队列
    CD_SPEED_UP = 22,        -- CD加速
    ITEM_USE = 25,        -- 物品使用
    ITEM_BUY_USE = 26,        -- 物品购买并使用
    ADVANCED_TELEPORT = 27,        -- 高级迁城
    COLLECT_BOOST = 28,        -- 采集加速(城内)
    BAG_MELT = 34,        -- 背包熔炼
    BAG_SYNTHESIZE = 35,        -- 背包合成(技能书 装备 宝物)
    BAG_SELL = 36,        -- 背包出售
    HERO_DRAW = 37,        -- 武将抽卡
    HERO_STAR_UPGRADE = 38,        -- 武将升星
    HERO_SKILL_UPGRADE = 39,        -- 武将技能升级
    HERO_EQUIP_UPGRADE = 40,        -- 装备升级
    HERO_SYNTHESIZE = 41,        -- 武将合成
    EQUIP_INLAY_ON = 42,        -- 装备镶嵌(宝石)
    EQUIP_SUCCINCT = 43,        -- 装备洗练
    STAMINA_BUY = 44,        -- 体力购买
    BABEL_RESET_BUY = 45,        -- 千层楼重置购买
    ARENA_CLEAR_BATTLE_CD = 46,        -- 清除擂台战斗CD
    ARENA_RESET_BATTLE_COUNT = 47,        -- 擂台战斗次数重置
    ENERGY_BUY = 48,        -- 行动力购买
    PLUNDERED = 50,        -- 被掠夺
    WALL_OUTFIRE = 51,        -- 城墙灭火
    WALL_REPAIR = 52,        -- 城墙修复
    SCOUT = 53,        -- 侦查
    CAMP_FIXED = 54,        -- 行营
    ALLIANCE_CREATE = 60,        -- 创建联盟
    ALLIANCE_DONATE = 61,        -- 联盟捐献
    ALLIANCE_CHANGE_BANNER = 62,        -- 修改联盟旗帜
    ALLIANCE_HIRE_HERO = 63,        -- 武将租借
    ACTIVITY_TARGET = 70,        -- 目标活动
    ACTIVITY_EXCHANGE = 71,        -- 兑换活动
    RESET_SCENARIOCOPY = 72,        -- 重置副本挑战次数
    HERO_DEBRIS_TURN = 73,        -- 武将碎片兑换
    UPGRADE_HERO_SKILL = 74,        -- 升级武将技能
    TRANSPORT = 75,        -- 运输消耗
}

-----
-- 体力获得类型
M.StaminaGainType = {
    RECOVER = 1,        -- 自动恢复
    STAMINA_WATER = 2,        -- 使用体力药水
    SKILL = 3,        -- 使用主动技能
    ATTACK_MONSTER_INVALID = 4,        -- 打怪失效
    USER_UPGRADE = 5,        -- 城主升级赠送
    BUY = 6,        -- 体力购买
}

-----
-- 体力消耗类型
M.StaminaConsumeType = {
    SCENARIO_COPY = 1,        -- 副本
}

-----
-- 行动力获得类型
M.EnergyGainType = {
    RECOVER = 1,        -- 自动恢复
    ENERGY_WATER = 2,        -- 使用行动力药水
    SKILL = 3,        -- 使用主动技能
    ATTACK_MONSTER_INVALID = 4,        -- 打怪失效
    USER_UPGRADE = 5,        -- 城主升级赠送
    BUY = 6,        -- 行动力购买
}

-----
-- 行动力消耗类型
M.EnergyConsumeType = {
    ATTACK_MOSTER = 1,        -- 攻击山贼
}

-----
-- 技能点获得类型
M.SkillPointGainType = {
    HERO_DEBRIS_TURN = 1,        -- 武将碎片获得
}

-----
-- 技能点消耗类型
M.SkillPointConsumeType = {
    UPGRADE_SKILL = 1,        -- 升级技能
}

-----
-- 兵种获得类型
M.ArmyGainType = {
    DEFAULT = 0,        -- 其它
    INIT = 1,        -- 初始化
    TRAIN = 2,        -- 训练
    PROP = 3,        -- 道具
}

-----
-- 兵种消耗类型
M.ArmyConsumeType = {
    DEFAULT = 0,        -- 其它
    DISMISS = 1,        -- 解雇
    DIE = 2,        -- 死亡
}

-----
-- 武将获得类型
M.HeroGainType = {
    DEFAULT = 0,        -- 其它
    INIT = 1,        -- 初始化
    HERO_DRAW = 2,        -- 招贤馆抽卡获得
    DEBRIS_SYNTHESIZE = 3,        -- 武将碎片合成
    ACTIVITY = 4,        -- 活动赠送
}

-----
-- 数据存储队列分类
M.DataClassify = {
    MAIL = 1,        -- 邮件插入
    MAIL_UPDATE = 2,        -- 邮件更新
    USER_UPDATE = 3,        -- 用户更新
    DICT = 4,        -- 字典插入或更新
    BAG = 5,        -- 背包插入或更新
    HERO = 6,        -- 武将插入或更新
    QUEST = 7,        -- 任务插入或更新
    SEVEN_TARGET = 8,        -- 7日目标插入或更新
    ARENA_RECORD = 9,        -- 擂台战斗记录插入
    TRANSPORT_RECORD = 10,        -- 运输记录插入
}

-----
-- 日志存储队列分类
M.LogClassify = {
    LOGIN = 1,        -- 登录
    ITEM = 2,        -- 物品
    GOLD = 3,        -- 金币
    SILVER = 4,        -- 银两
    FOOD = 5,        -- 粮食
    WOOD = 6,        -- 木材
    IRON = 7,        -- 铁矿
    STONE = 8,        -- 石料
    STAMINA = 9,        -- 体力
    ROLE_UPGRADE = 10,        -- 角色升级
    BUILDING_UPGRADE = 11,        -- 建筑升级
    VIP_UPGRADE = 12,        -- VIP升级
    SIGN = 13,        -- 签到
    ONLINE_REWARD = 14,        -- 在线奖励
    MARCH = 15,        -- 行军
    MONSTER_SIEGE = 16,        -- 怪物攻城
    ALLIANCE_SCIENCE_DONATE = 17,        -- 科技捐献
    ALLIANCE_EXPLORE = 18,        -- 联盟探险
    TURNPLATE = 19,        -- 转盘
    ACTIVITY_TARGET_REWARD = 20,        -- 活动目标奖励
    STORE = 21,        -- 联盟商店
    TOWER = 22,        -- 爬塔
    TECHNOLOGY = 23,        -- 个人科技
    ARMY = 24,        -- 兵种
    HERO = 25,        -- 武将获得
    HERO_UPGRADE = 26,        -- 武将等级星级升级
    ARENA_WIN_POINT = 27,        -- 擂台胜点
    ALLIANCE_SCORE = 28,        -- 联盟积分
    BABEL_SCORE = 29,        -- 千层楼积分
    BRONZE_SCORE = 30,        -- 铜雀台积分
    SKILLPOINT = 31,        -- 技能点
}

-----
-- 语言
M.LangType = {
    ALL = 0,        -- 全部语言
    CN = 1,        -- 中文简体
    TW = 2,        -- 中文繁体
    EN = 3,        -- 英语
    FR = 4,        -- 法语
    DE = 5,        -- 德语
    RU = 6,        -- 俄语
    KR = 7,        -- 韩语
    TH = 8,        -- 泰语
    JP = 9,        -- 日语
    PT = 10,        -- 葡萄牙语
    ES = 11,        -- 西班牙语
    TR = 12,        -- 土耳其语
    ID = 13,        -- 印尼语
    IT = 14,        -- 意大利语
    PL = 15,        -- 波兰语
    NL = 16,        -- 荷兰语
    AR = 17,        -- 阿拉伯语(阿联酋)
    RO = 18,        -- 罗马尼亚语
    FA = 19,        -- 波斯语(伊朗)
}

-----
-- 登录结果
M.LoginResultType = {
    OK = 0,        -- 成功
    BAD_SESSION = 1,        -- 错误的认证
    LOCKED = 2,        -- 已被锁定
    UNKNOWN_ERROR = 3,        -- 未知错误
}

-----
-- 被踢下线的原因
M.KickType = {
    LOGIN_DUPLICATE = 1,        -- 帐号在别处登录
    ACCOUNT_LOCKED = 2,        -- 帐号已被锁定
    FOUND_ERROR = 3,        -- 检查到帐号异常
    MAINTENANCE = 4,        -- 服务器维护
    RECONNECT = 5,        -- 账号重新连接（自己踢自己下线）
    HEART_BEAT_REPLY = 6,        -- 账号心跳超时(300秒没有回应)
}

-----
-- 设置昵称结果
M.SetNicknameResultType = {
    OK = 0,        -- OK
    EXIST = 2,        -- 已经存在
    BAD_FORMAT = 3,        -- 存在非法字符
}

-----
-- 属性加成类型
M.AttributeAdditionType = {
    BASE = 1,        -- 基础数据加成(建筑属性)
    PCT = 2,        -- 万分比加成
    EXT = 3,        -- 整数数值加成
}

-----
-- 属性类型
M.AttributeType = {
    TRAINING_NUM = 1,        -- 训练量
    TRAINING_SPEED = 2,        -- 训练速度（百分比）
    TRAINING_QUEUE_NUM = 3,        -- 训练队列数量
    TRAP_CAPACITY = 4,        -- 陷阱容量
    CITY_DEFENSE = 5,        -- 城防值
    TRAP_MAKE_NUM = 6,        -- 陷阱制造量
    CITY_TROOPS_DEFENSE = 7,        -- (武将和部队)守军防御加成
    TURRET_ATTACK_FORCE = 8,        -- 箭塔攻击力
    RESOURCE_PROTECT_LIMIT = 9,        -- 资源保护上限
    FOOD_PROTECT_LIMIT = 10,        -- 粮食保护上限
    WOOD_PROTECT_LIMIT = 11,        -- 木材保护上限
    STONE_PROTECT_LIMIT = 12,        -- 石料保护上限
    IRON_PROTECT_LIMIT = 13,        -- 铁矿保护上限
    WOUNDED_CAPACITY = 14,        -- 伤兵容量
    FOOD_OUTPUT = 15,        -- 粮食产出/H
    FOOD_CAPACITY = 16,        -- 粮食容量
    WOOD_OUTPUT = 17,        -- 木材产出/H
    WOOD_CAPACITY = 18,        -- 木材容量
    STONE_OUTPUT = 19,        -- 石料产出/H
    STONE_CAPACITY = 20,        -- 石料容量
    IRON_OUTPUT = 21,        -- 铁矿产出/H
    IRON_CAPACITY = 22,        -- 铁矿容量
    CASTLE_TAX = 24,        -- 城主府征收收益
    TRANSPORT_NUM = 25,        -- 市场部队运输数量
    TRANSPORT_TAX = 26,        -- 市场运输税率
    INFANTRY_TRAINING_NUM = 27,        -- 步兵训练量
    INFANTRY_TRAINING_SPEED = 28,        -- 步兵训练速度（百分比）
    INFANTRY_TRAINING_QUEUE_NUM = 29,        -- 步兵训练队列数量
    RIDER_TRAINING_NUM = 30,        -- 骑兵训练量
    RIDER_TRAINING_SPEED = 31,        -- 骑兵训练速度（百分比）
    RIDER_TRAINING_QUEUE_NUM = 32,        -- 骑兵训练队列数量
    ARCHER_TRAINING_NUM = 33,        -- 弓兵训练量
    ARCHER_TRAINING_SPEED = 34,        -- 弓兵训练速度（百分比）
    ARCHER_TRAINING_QUEUE_NUM = 35,        -- 弓兵训练队列数量
    MECHANICAL_TRAINING_NUM = 36,        -- 器械训练量
    MECHANICAL_TRAINING_SPEED = 37,        -- 器械训练速度（百分比）
    MECHANICAL_TRAINING_QUEUE_NUM = 38,        -- 器械训练队列数量
    TROOPS_ATTACK = 1009,        -- 部队攻击
    TROOPS_DEFENSE = 1010,        -- 部队防御
    TROOPS_HEALTH = 1011,        -- 部队生命
    TROOPS_SPEED = 1012,        -- 部队速度
    TROOPS_MORALE = 1013,        -- 部队怒气
    TROOPS_ATTACK_CITY = 1014,        -- 攻城攻击
    ATTACK_MONSTER_MARCH_SPEED = 1016,        -- 打怪行军速度加成（百分比）
    MARCH_SPEED = 1017,        -- 行军速度加成（百分比）
    SCOUT_SPEED = 1018,        -- 侦查速度加成（百分比）
    TROOPS_LOAD = 1019,        -- 部队负重加成
    ROB_RESOURCE_MAX = 1020,        -- 增加掠夺他人资源上限（百分比）
    TRANSPORT_SPEED_UP = 1022,        -- 增加运输资源速度(百分比)
    CAMPTEMP_SPEED = 1023,        -- 驻扎行军速度(百分比)
    GATHER_SPEED = 1024,        -- 采集速度(百分比)
    ATTACK_PLAYER_SPEED = 1025,        -- 攻击玩家行军速度(百分比)
    REINFORCEMENTS_SPEED = 1026,        -- 援军行军速度(百分比)
    FOOD_GATHER_SPEED = 2005,        -- 粮食采集速度加成（百分比）
    WOOD_GATHER_SPEED = 2006,        -- 木材采集速度加成（百分比）
    STONE_GATHER_SPEED = 2007,        -- 石料采集速度加成（百分比）
    IRON_GATHER_SPEED = 2008,        -- 铁采集速度加成（百分比）
    UPKEEP_REDUCE = 2012,        -- 部队粮食消耗减少（百分比）
    REDUCE_RESEARCH_CONSUME = 2015,        -- 减少科技研究消耗（百分比）
    BUILDING_SPEED = 3002,        -- 建筑建造速度加成（百分比）
    HEAL_SPEED = 3003,        -- 伤兵恢复速度加成（百分比）
    REDUCE_HEAL_RESOURCES = 3004,        -- 医疗所需资源减少（百分比）
    ALLIANCE_HELP_COUNT = 3005,        -- 联盟帮助次数
    ALLIANCE_HELP_REDUSE_TIME = 3006,        -- 联盟帮助减少时间
    MARCH_QUEUE_MAX = 3007,        -- 出征队伍数量上限
    MARCH_HERO_MAX = 3008,        -- 队伍可配将数量上限
    HEAL_NUM = 3019,        -- 治疗量
    HEAL_QUEUE_NUM = 3020,        -- 治疗队列数量
    ALLIANCE_REINFORCEMENT_NUM = 3021,        -- 同盟增援部队数量
    DRILL_GROUNDS_SOLDIER_NUM_MAX = 3023,        -- 校场士兵数量上限
    TRAP_BUILDING_SPEED = 4011,        -- 陷阱制造速度加成（百分比）
    TRAP_ATTACK = 4012,        -- 陷阱攻击
    HERO_POWER = 5001,        -- 武将武力
    HERO_DEFENSE = 5002,        -- 武将统帅
    HERO_WISDOM = 5003,        -- 武将智力
    HERO_SKILL = 5004,        -- 武将士气
    HERO_AGILE = 5005,        -- 武将速度
    HERO_LUCKY = 5006,        -- 武将运气
    HERO_LIFE = 5007,        -- 武将攻城
    HERO_PHYSICAL_ATTACK = 5008,        -- 物理攻击力
    HERO_PHYSICAL_DEFENSE = 5009,        -- 物理防御力
    HERO_WISDOM_ATTACK = 5010,        -- 谋略攻击力
    HERO_WISDOM_DEFENSE = 5011,        -- 谋略防御力
    HERO_HIT = 5012,        -- 命中值
    HERO_AVOID = 5013,        -- 回避值
    HERO_CRIT_HIT = 5014,        -- 暴击命中值
    HERO_CRIT_AVOID = 5015,        -- 暴击回避值
    HERO_SPEED = 5016,        -- 攻击速度
    HERO_CITY_LIFE = 5017,        -- 攻城值
    HERO_FIGHT = 5018,        -- 兵力上限
    HERO_SINGLE_ENERGY = 5019,        -- 单挑血量
    TRAP_HORSE_DAMAGE = 6004,        -- 拒马克骑兵伤害
    TRAP_STONE_DAMAGE = 6005,        -- 镭石克步兵伤害
    TRAP_WOOD_DAMAGE = 6006,        -- 滚木克弓兵伤害
    TRAP_OIL_DAMAGE = 6007,        -- 火油克器械伤害
    RIDER_POWER = 6011,        -- 骑兵攻击力
    INFANTRY_POWER = 6012,        -- 步兵攻击力
    ARCHER_POWER = 6013,        -- 弓兵攻击力
    MECHANICAL_POWER = 6014,        -- 器械攻击力
    RIDER_DEFENSE = 6015,        -- 骑兵防御力
    INFANTRY_DEFENSE = 6016,        -- 步兵防御力
    ARCHER_DEFENSE = 6017,        -- 弓兵防御力
    MECHANICAL_DEFENSE = 6018,        -- 器械防御力
    REINFORCE_LIMIT = 6020,        -- 援兵容纳队列上限
    REDUCE_RIDER_TRAIN_CONSUME = 6021,        -- 骑兵训练消耗降低
    REDUCE_INFANTRY_TRAIN_CONSUME = 6022,        -- 步兵训练消耗降低
    REDUCE_ARCHER_TRAIN_CONSUME = 6023,        -- 弓兵训练消耗降低
    REDUCE_MECHANICAL_TRAIN_CONSUME = 6024,        -- 器械消耗降低
    RIDER_RESTRAIN_INFANTRY = 6025,        -- 骑兵克制步兵
    INFANTRY_RESTRAIN_ARCHER = 6026,        -- 步兵克制弓兵
    ARCHER_RESTRAIN_MACHINE = 6027,        -- 弓兵克制器械
    MACHINE_RESTRAIN_RIDER = 6028,        -- 器械克制骑兵
    CAN_ATTACK_MONSTER_LEVEL = 6029,        -- 可攻击野怪等级
    CAN_ATTACK_ELITE_MONSTER_LEVEL = 6030,        -- 可攻击精英野怪等级
    ATTACK_MONSTER_DAMAGE = 6034,        -- 攻击野怪伤害
    ATTACK_ELITE_MONSTER_DAMAGE = 6035,        -- 攻击精英野怪伤害
    ATTACK_WORLDBOSS_DAMAGE = 6036,        -- 攻击BOSS伤害
}

-----
-- 武将更新属性
M.HeroUpdateAttributeType = {
    HERO_POWER = 1,        -- 武将武力
    HERO_DEFENSE = 2,        -- 武将统帅
    HERO_WISDOM = 3,        -- 武将智力
    HERO_LUCKY = 4,        -- 武将运气
    HERO_SKILL = 5,        -- 武将谋略
    HERO_AGILE = 6,        -- 武将军略
    HERO_LIFE = 7,        -- 武将攻城
}

-----
-- 物品类型
M.ItemType = {
    EQUIP = 1,        -- 装备
    PROP = 2,        -- 道具
    TREASURE = 3,        -- 宝物
}

-----
-- 物品品质类型
M.ItemQualityType = {
    WHITE = 1,        -- 白
    GREEN = 2,        -- 绿
    BLUE = 3,        -- 蓝
    PURPLE = 4,        -- 紫
    ORANGE = 5,        -- 橙
}

-----
-- 装备类型
M.ItemEquipType = {
    WEAPON = 1,        -- 武器
    BREASTPLATE = 2,        -- 胸甲
    BOOT = 3,        -- 靴子
    BRACER = 4,        -- 护腕
    CASQUE = 5,        -- 头盔
    SILK_BAG = 6,        -- 锦囊
}

-----
-- 宝物类型
M.ItemTreasureType = {
    WEAPON = 1,        -- 武器
    SHORT_BOW = 2,        -- 短弓
    MOUNT = 3,        -- 坐骑
    PROPITIOUS_OMEN = 4,        -- 祥瑞
    BOOK = 5,        -- 书物
}

-----
-- 道具类型
M.ItemPropType = {
    SPECIAL_ITEMS = 1,        -- 特殊物品
    LORD_EXP = 1001,        -- 领主经验
    HERO_EXP = 2001,        -- 武将经验
    HERO_DEBRIS = 3001,        -- 武将碎片
    BUILDING_SPEEDUP = 4001,        -- 建筑升级加速
    TROOP_TRAINING_SPEEDUP = 4002,        -- 部队训练加速
    WOUNDED_RECOVERY_SPEEDUP = 4003,        -- 伤兵恢复加速
    SCIENCE_RESEARCH_SPEEDUP = 4004,        -- 科技研究加速
    TRAPS_BUILDING_SPEEDUP = 4005,        -- 陷阱建造加速
    FORGE_SPEEDUP = 4006,        -- 锻造加速
    SPEEDUP = 4007,        -- 通用加速
    VIP_EXP = 5001,        -- VIP经验
    VIP_TIME = 5002,        -- VIP时长
    FOOD = 6001,        -- 粮食增加
    WOOD = 6002,        -- 木头增加
    STONE = 6003,        -- 石料增加
    IRON = 6004,        -- 铁矿增加
    SILVER = 6005,        -- 银两增加
    GOLD = 6006,        -- 金币增加
    STAMINA = 6007,        -- 体力增加
    CHIP = 6008,        -- 筹码增加
    HONOR = 6009,        -- 荣誉增加
    SAN = 6010,        -- 脑残值增加
    HERO_PHYSICAL_RECOVER = 6011,        -- 武将体力恢复
    ENERGY = 6012,        -- 行动力增加
    HERO_LEADERSHIP = 7001,        -- 武将统率力增加
    ANTI_SCOUT = 7002,        -- 反侦察
    PEACE_SHIELD = 7003,        -- 战争守护
    TROOP_MARCHING_SPEEDUP = 7004,        -- 行军加速
    FALSE_ARMY = 7005,        -- 伪装术
    ATTACK_BONUS = 7006,        -- (所有武将和部队)攻击增加
    DEFENSE_BONUS = 7007,        -- (所有武将和部队)增加
    SMALL_UPKEEP_REDUCTION = 8001,        -- 粮食消耗减少
    GATHER_BONUS = 8002,        -- 采集速度加成
    FRAM_BOOST = 9001,        -- 良田产量提升
    SAWMILL_BOOST = 9002,        -- 木材产量提升
    STONE_MINE_BOOST = 9003,        -- 石矿产量提升
    IRON_MINE_BOOST = 9004,        -- 铁矿产量提升
    RANDOM_TELEPORT = 10001,        -- 随机迁城
    ADVANCED_TELEPORT = 10002,        -- 高级迁城
    LORD_RENAME = 10003,        -- 领主改名
    CHANGE_APPREARANCE = 10004,        -- 更换形象
    MARCH_RECALL = 10005,        -- 行军召回
    SOLDIER_RECRUITMENT = 10006,        -- 士兵招募
    GOLD_ARROW = 10007,        -- 黄金箭
    WAR_HORN = 10008,        -- 战争号角
    ADVANCED_RECALL = 10009,        -- 高级召回
    WISHING_COIN = 10010,        -- 祈愿币
    LORD_SKILL_RESET = 10011,        -- 领主技能重置
    ALLIANCE_QUEST_REFRESH = 10012,        -- 联盟任务刷新
    MOLTEN_STONE = 10013,        -- 熔铸之石
    HERO_DRAW_PROP = 10014,        -- 招贤道具
    MATERIAL_SYNTHESIZE = 10015,        -- 合成需要消耗材料
    GIFT = 11001,        -- 礼包
    SKILL_BOOK = 12001,        -- 技能书
    SKILL_DEBRIS = 12002,        -- 技能书碎片
    EQUIP_DEBRIS = 12003,        -- 装备碎片
    TREASURE_DEBRIS = 12004,        -- 宝物碎片
    ATTACK_GEM = 13001,        -- 攻击宝石
    DEFENSE_GEM = 13002,        -- 防御宝石
    SPECIAL_GEM = 13003,        -- 特殊宝石
    TREASURE_BOX = 20001,        -- 宝箱
}

-----
-- 资源类型
M.ResourceType = {
    FOOD = 1,        -- 粮食
    WOOD = 2,        -- 木材
    IRON = 3,        -- 铁
    STONE = 4,        -- 石料
    GOLD = 5,        -- 元宝
    SILVER = 6,        -- 银两
}

-----
-- 特殊道具
M.SpecialPropIdType = {
    FOOD = 1,        -- 粮食
    WOOD = 2,        -- 木材
    IRON = 3,        -- 铁
    STONE = 4,        -- 石料
    GOLD = 5,        -- 元宝
    SILVER = 6,        -- 银两
    STAMINA = 7,        -- 体力
    LORD_EXP = 8,        -- 领主经验
    VIP_EXP = 9,        -- VIP经验
    ALLIANCE_STORE = 10,        -- 联盟积分
    CHIP = 11,        -- 筹码
    HERO_EXP = 12,        -- 武将经验
    SAN = 13,        -- 脑残值
    WIN_POINT = 14,        -- 擂台胜点
    BABEL_SCORE = 15,        -- 千层楼积分
    SKILL_POINT = 17,        -- 技能点
    ENERGY = 18,        -- 行动力
    COIN = 19,        -- 铜币
}

-----
-- 商品类型
M.GoodsType = {
    RESOURCES = 1,        -- 资源
    WAR = 2,        -- 战争
    BUFF = 3,        -- 增益
    OTHERS = 4,        -- 其它
}

-----
-- 背包分类(前端用)
M.BagClasify = {
    RESOURCE = 1,        -- 资源
    TREASURE = 2,        -- 宝物
    EQUIP = 3,        -- 装备
    SKILL_BOOK = 4,        -- 技能书
    HERO_DEBRIS = 5,        -- 武将碎片
    GEM = 6,        -- 宝石
}

-----
-- 建筑类型
M.BuildingType = {
    CASTLE = 1,        -- 城主府 
    BARRACKS = 2,        -- 军营
    MARKET = 3,        -- 市场
    WALLS = 4,        -- 城墙
    TURRET = 5,        -- 箭塔
    WATCH_TOWER = 6,        -- 烽火台
    COLLEGE = 7,        -- 书院
    HEROES_HALL = 8,        -- 招贤馆
    DRILL_GROUNDS = 9,        -- 点将台
    ARENA = 10,        -- 擂台
    BRONZE_SWALLOW_TERRACE = 11,        -- 铜雀台
    DEPOT = 12,        -- 仓库
    HOSPITAL = 13,        -- 伤兵营
    THOUSAND_PARIDIS = 14,        -- 千重楼
    RIDING_ALONE = 15,        -- 千里走单骑
    SCENARIO_COPY = 16,        -- 剧情副本
    PEACH_GARDEN = 17,        -- 桃园
    WAISHICOURTYARD = 18,        -- 外使院
    INFANTRY_BARRACKS = 19,        -- 步兵营
    RIDER_BARRACKS = 20,        -- 骑兵营
    ARCHER_BARRACKS = 21,        -- 弓兵营
    MECHANICAL_BARRACKS = 22,        -- 器械营
    FARM = 30,        -- 农田
    SAWMILL = 31,        -- 林场
    STONE_MINE = 32,        -- 采石场
    IRON_MINE = 33,        -- 铁矿场
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
    TRAIN_FINISH = 9,        -- 训练完成
    DAMAGED = 10,        -- 破损
}

-----
-- 瞭望塔敌军信息功能类型
M.WATCHTOWER_ENEMY_STATUS_TYPE = {
    NICKNAME = 1,        -- 敌方领主名称
    TO = 2,        -- 敌方部队目的坐标
    FROM = 3,        -- 敌方来自坐标
    REACH_TIME = 4,        -- 敌方部队到达时间
    MARCHING_TROOP_TOTAL = 5,        -- 敌方部队总兵力
    CASTLE_LEVEL = 6,        -- 敌方部队领主城堡等级
    MARCHING_TROOP_TYPE = 7,        -- 敌方部队的兵种类型
    MARCHING_TROOP_COUNT = 8,        -- 敌方部队的兵种具体数量
    SKILL = 9,        -- 敌方部队的技能
    TECHNOLOGY = 10,        -- 敌方部队的科技
    LORD_LEVEL = 11,        -- 敌方部队领主等级
    EQUIP = 12,        -- 敌方部队装备
    BUFF = 13,        -- 敌方部队buff（战斗相关，使用道具and主动技能）
    VIP = 14,        -- VIP信息
}

-----
-- 瞭望塔侦查功能类型
M.WATCHTOWER_SCOUT_TYPE = {
    PLAYER_ALLIANCE = 101,        -- 
    PLAYER_NAME_LV = 102,        -- 
    PLAYER_RESOURCE_COUNT = 103,        -- 
    PLAYER_GLOBAL_BUFF = 104,        -- 
    WALL_DEFENSE_ARMY_COUNT = 105,        -- 
    WALL_DEFENSE_HERO_NAME = 106,        -- 
    WALL_DEFENSE_HERO_LEVEL = 107,        -- 
    WALL_DEFENSE_ARMY_TYPE_LEVEL = 108,        -- 
    WALL_DEFENSE_ROUGH_COUNT_DURABLE = 109,        -- 
    WALL_DEFENSE_ALL_TEAM_ARMY_ROUGH_COUNT = 110,        -- 
    WALL_DEFENSE_TEAM_ARMY_ROUGH_COUNT = 111,        -- 
    WALL_DEFENSE_TYPE_COUNT = 112,        -- 
    ALL_TEAM_HERO_NAME = 113,        -- 
    ALL_TEAM_HERO_LEVEL = 114,        -- 
    ALL_TEAM_ARMY_TYPE_LEVEL = 115,        -- 
    ALL_TEAM_ARMY_COUNT = 116,        -- 
    WALL_DEFENSE_COUNT = 117,        -- 
    TEAM_ARMY_COUNT = 118,        -- 
    CATAPULT_LEVEL = 119,        -- 
    TECHNOLOGY_INFO = 120,        -- 
    ALL_HERO_SKILL_CONFIGURATION = 121,        -- 
    SCOUT_SPEED_UP = 122,        -- 
    CASTLE_TEAM_PPLAYER_NHL_ALLIANCE_NAME = 123,        -- 
    CAMPFIXED_NAME = 201,        -- 
    CAMPFIXED_PLAYER_ALLIANCE = 202,        -- 
    CAMPFIXED_PLAYER_NAME = 203,        -- 
    CAMPFIXED_DEFENSE_TEAM_ONE_ARMY_ROUGH_COUNT = 204,        -- 
    CAMPFIXED_DEFENSE_TEAM_ONE_HERO_NAME = 205,        -- 
    CAMPFIXED_DEFENSE_TEAM_ONE_HERO_LEVEL = 206,        -- 
    CAMPFIXED_DEFENSE_TEAM_ONE_ARMY_TYPE_LEVEL = 207,        -- 
    CAMPFIXED_ALL_DEFENSE_TEAM_ARMY_ROUGH_COUNT = 208,        -- 
    CAMPFIXED_DEFENSE_TEAM_ARMY_ROUGH_COUNT = 209,        -- 
    CAMPFIXED_DEFENSE_TEAM_HERO_NAME = 210,        -- 
    CAMPFIXED_DEFENSE_TEAM_HERO_LEVEL = 211,        -- 
    CAMPFIXED_DEFENSE_TEAM_ARMY_TYPE_LEVEL = 212,        -- 
    CAMPFIXED_DEFENSE_TEAM_ALLIANCE_TECH_INFO = 213,        -- 
    CAMPFIXED_ALL_DEFENSE_TEAM_ARMY_COUNT = 214,        -- 
    CAMPFIXED_DEFENSE_TEAM_ARMY_COUNT = 215,        -- 
    CAMPFIXED_DEFENSE_TEAM_TECH_INFO = 216,        -- 
    CAMPFIXED_DEFENSE_TEAM_HERO_SKILL_CONFIGURATION = 217,        -- 
    CAMPFIXED_TEAM_PPLAYER_NHL_ALLIANCE_NAME = 218,        -- 
    CITY_NAME = 301,        -- 
    CITY_PLAYER_ALLIANCE = 302,        -- 
    CITY_DEFENSE_TEAM_ONE_ARMY_ROUGH_COUNT = 303,        -- 
    CITY_DEFENSE_TEAM_ONE_HERO_NAME = 304,        -- 
    CITY_DEFENSE_TEAM_ONE_HERO_LEVEL = 305,        -- 
    CITY_DEFENSE_TEAM_ONE_ARMY_TYPE_LEVEL = 306,        -- 
    CITY_ALL_DEFENSE_TEAM_ARMY_ROUGH_COUNT = 307,        -- 
    CITY_DEFENSE_TEAM_ARMY_ROUGH_COUNT = 308,        -- 
    CITY_DEFENSE_TEAM_HERO_NAME = 309,        -- 
    CITY_DEFENSE_TEAM_HERO_LEVEL = 310,        -- 
    CITY_DEFENSE_TEAM_ARMY_TYPE_LEVEL = 311,        -- 
    CITY_DEFENSE_TEAM_ALLIANCE_TECH_INFO = 312,        -- 
    CITY_ALL_DEFENSE_TEAM_ARMY_COUNT = 313,        -- 
    CITY_DEFENSE_TEAM_ARMY_COUNT = 314,        -- 
    CITY_DEFENSE_TEAM_TECH_INFO = 315,        -- 
    CITY_DEFENSE_TEAM_HERO_SKILL_CONFIGURATION = 316,        -- 
    CITY_TEAM_PPLAYER_NHL_ALLIANCE_NAME = 317,        -- 
    PLAY_HEAD_LEVEL = 501,        -- 
    MARCH_REMAIN_TIME = 502,        -- 
    MARCH_ARMY_ROUGH_COUNT = 503,        -- 
    MARCH_HERO_NAME = 504,        -- 
    MARCH_HERO_LEVEL = 505,        -- 
    MARCH_ARMY_TYPE_LEVEL = 506,        -- 
    MARCH_ARMY_COUNT = 507,        -- 
    MARCH_TECH_INFO = 508,        -- 
    MARCH_HERO_SKILL_CONFIGURATION = 509,        -- 
}

-----
-- 兵种分类
M.ArmyClass = {
    ARMY_TRAPS = 1,        -- 陷阱类
    ARMY_COMMON = 2,        -- 普通兵种
}

-----
-- 兵种类型
M.ArmyType = {
    SABER = 1,        -- 剑兵
    PIKEMAN = 2,        -- 枪兵
    HALBERDIER = 3,        -- 戟兵
    ARCHER = 4,        -- 弓兵
    RIDER = 5,        -- 骑兵
    CHARIOT = 6,        -- 冲车
    STONE_THROWER = 7,        -- 投石车
    WAR_ELEPHANT = 8,        -- 战象
    CHEVAL_DE_FRISE = 10,        -- 拒马
    ROLLING_LOGS = 11,        -- 滚木
    GRIND_STONE = 12,        -- 擂石
}

-----
-- 兵种类型
M.ArmysType = {
    INFANTRY = 1,        -- 轻步兵
    RIDER = 2,        -- 轻骑兵
    ARCHER = 3,        -- 轻弓兵
    MECHANICAL = 4,        -- 机械兵
    REDIF = 5,        -- 预备兵
    ROLLING_LOGS = 101,        -- 滚木
    GRIND_STONE = 102,        -- 礌石
    CHEVAL_DE_FRISE = 103,        -- 拒马
    KEROSENE = 104,        -- 火油
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
    DODGE = 8,        -- 闪避
}

-----
-- 战斗类型
M.BattleType = {
    UNKNOWN = 0,        -- 未定义
    GATHER = 1,        -- 采集
    MONSTER = 2,        -- 打怪
    SIEGE_CITY = 3,        -- 名城
    SIEGE_CASTLE = 4,        -- 玩家城池
    CAMP_TEMP = 5,        -- 驻扎
    CAMP_FIXED = 6,        -- 行营
    CATAPULT = 7,        -- 箭塔战斗
    WORLDBOSS = 8,        -- 世界BOSS战斗
    PALACE_WAR = 9,        -- 王城战
    OPEN_FOREST = 21,        -- 森林
    SCENARIO_COPY_COMBAT = 22,        -- 剧情副本战斗
    SCENARIO_COPY_ANSWER = 23,        -- 剧情副本答题
    ARENA_COMBAT = 24,        -- 擂台战斗
    BABEL_COMBAT = 25,        -- 千层楼战斗
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
    BUILD = 12,        -- 建造 升级
    FORGE = 17,        -- 锻造
    RESEARCH = 16,        -- 研究
    TRAIN = 13,        -- 训练
    HEAL = 15,        -- 治疗
    TRAP = 18,        -- 陷阱
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
    HURT = 6,        -- 造成伤害
    RECOVER = 7,        -- 战后恢复量（只做统计，还是直接加到normal里面去）
}

-----
-- 部队编组状态
M.TeamState = {
    LOCKED = 1,        -- 锁定
    IDLE = 2,        -- 闲置
    DEFENSE = 3,        -- 守城
    MARCH = 4,        -- 出征
}

-----
-- 技能类型
M.SkillType = {
    COMBAT = 1,        -- 战斗
    DEVELOP = 2,        -- 发展
    SUPPORT = 3,        -- 辅助
}

-----
-- 主动技能类型
M.ActiveSkillType = {
    IMMEDIATE_RETURN = 1,        -- 立即返回 (使所有在外出的部队立即返回城市，不包括参与组队战斗的部队)
    GENERAL_MOBILIZATION = 2,        -- 总动员 (主动技能，使用后行军上限增加10%,持续1小时)
    HELP = 3,        -- 救援 (使你的部队下一次出征战斗时可以产生伤兵。如果医院不能容纳伤兵，则会造成士兵的死亡)
    BUMPER_HARVEST = 4,        -- 丰收 (立即获得所有资源田5小时的资源产出)
    CRAZY_COLLOECTION = 5,        -- 疯狂采集 (主动技能，使用技能后采集资源速度提高100%，持续2小时)
    RESOURCE_PROTECTION = 6,        -- 资源保护 (使你的城市所有资源2小时内不会被掠夺)
    FULL_OF_ENERGY = 7,        -- 体力充沛 (主动技能，使用后立即获得30体力)
    SKILLFUL_WORKMAN = 8,        -- 能工巧匠 (立即获得500个陷阱，会再你可以建造的最高等级陷阱里面随机选择一个类型)
    FULL_OF_MUD = 9,        -- 遍地泥浆 (主动技能，使用后侦查或攻击你的敌人行军时间提高5倍，该技能只有在侦查或攻击前使用才会生效，城堡战斗无效)
}

-----
-- 邮件类型
M.MailType = {
    SYSTEM = 1,        -- 系统
    REPORT = 2,        -- 战报
    SCOUT = 3,        -- 侦查
    GATHER = 4,        -- 采集
    PRIVATE = 5,        -- 个人
}

-----
-- 邮件子类型
M.MailSubType = {
    SYSTEM_REWARD = 1,        -- 系统奖励
    SYSTEM_MAINTENANCE = 2,        -- 系统维护
    SYSTEM_OPERATION = 3,        -- 运营邮件
    SYSTEM_BAG_FULL = 4,        -- 背包邮件
    SYSTEM_ARENA_MAX_RECORD = 5,        -- 擂台最高记录奖励邮件
    SYSTEM_ALLIANCE_INVITE = 6,        -- 联盟邀请
    SYSTEM_ALLIANCE_KICK = 7,        -- 踢出联盟
    SYSTEM_ALLIANCE_DISBAND = 8,        -- 解散联盟
    SYSTEM_ALLIANCE_LEADER_TRANSFER = 9,        -- 盟主转让
    SYSTEM_ALLIANCE_LEADER_REPLACE = 10,        -- 盟主被取代
    SYSTEM_ALLIANCE_TELEPORT_INVITE = 11,        -- 联盟邀请迁城
    SYSTEM_ALLIANCE_REWARD = 12,        -- 联盟奖励
    SYSTEM_ALLIANCE_LEVEL_UP = 13,        -- 联盟升级
    SYSTEM_ALLIANCE_REWARD_DETAIL = 14,        -- 联盟活跃度奖励详细说明
    SYSTEM_ALLIANCE_OWN_CITY = 15,        -- 联盟获得名城主权
    SYSTEM_ALLIANCE_LOSE_CITY = 16,        -- 联盟失去名城主权
    SYSTEM_ALLIANCE_OWN_CITY_OCCUPY = 17,        -- 联盟获得名城占领权
    SYSTEM_ALLIANCE_LOSE_CITY_OCCUPY = 18,        -- 联盟失去名城占领权
    SYSTEM_ATTACK_CASTLE_SUCCESS = 21,        -- 攻击城池成功
    SYSTEM_DEFENSE_CASTLE_FAIL = 22,        -- 防守城池失败
    SYSTEM_PALACE_WAR_GIFT = 23,        -- 礼包邮件
    SYSTEM_WORLDBOSS_HURT_REWARD = 24,        -- 世界BOSS伤害奖励邮件
    SYSTEM_WORLDBOSS_KILL_REWARD = 25,        -- 世界BOSS击杀奖励邮件
    SYSTEM_WORLDBOSS_ALLIANCE_REWARD = 26,        -- 世界BOSS联盟奖励邮件
    SYSTEM_CITY_HURT_REWARD = 27,        -- 名城伤害奖励邮件
    SYSTEM_CITY_KILL_REWARD = 28,        -- 名城击杀奖励邮件
    SYSTEM_CITY_ALLIANCE_REWARD = 29,        -- 名城联盟奖励邮件
    SYSTEM_CATAPULT_HURT_REWARD = 30,        -- 箭塔伤害奖励邮件
    SYSTEM_CATAPULT_KILL_REWARD = 31,        -- 箭塔击杀奖励邮件
    SYSTEM_CATAPULT_ALLIANCE_REWARD = 32,        -- 箭塔联盟奖励邮件
    SYSTEM_BRONZESPARROWTOWER_REWARD = 33,        -- 铜雀台答题奖励邮件
    SYSTEM_COMPENSATE = 34,        -- 战斗损兵补偿
    REPORT_DEFENSE_CASTLE_SUCCESS = 501,        -- 守城胜利
    REPORT_DEFENSE_CASTLE_FAIL = 502,        -- 守城失败
    REPORT_ATTACK_CASTLE_SUCCESS = 503,        -- 攻城胜利
    REPORT_ATTACK_CASTLE_FAIL = 504,        -- 攻城失败
    REPORT_ATTACK_CAMP_FIXED_SUCCESS = 505,        -- 攻击行营胜利
    REPORT_ATTACK_CAMP_FIXED_FAIL = 506,        -- 攻击行营失败
    REPORT_DEFENSE_CAMP_FIXED_SUCCESS = 507,        -- 行营防守胜利
    REPORT_DEFENSE_CAMP_FIXED_FAIL = 508,        -- 行营防守失败
    REPORT_ATTACK_CAMP_TEMP_SUCCESS = 509,        -- 攻击驻扎地胜利
    REPORT_ATTACK_CAMP_TEMP_FAIL = 510,        -- 攻击驻扎地失败
    REPORT_DEFENSE_CAMP_TEMP_SUCCESS = 511,        -- 驻扎地防守胜利
    REPORT_DEFENSE_CAMP_TEMP_FAIL = 512,        -- 驻扎地防守失败
    REPORT_ATTACK_FAMOUS_CITY_SUCCESS = 513,        -- 攻击名城胜利
    REPORT_ATTACK_FAMOUS_CITY_FAIL = 514,        -- 攻击名城失败
    REPORT_DEFENSE_FAMOUS_CITY_SUCCESS = 515,        -- 防守名城成功
    REPORT_DEFENSE_FAMOUS_CITY_FAIL = 516,        -- 防守名城失败
    REPORT_OCCUPY_FAMOUS_CITY_SUCCESS = 517,        -- 占领名城胜利
    REPORT_OCCUPY_FAMOUS_CITY_FAIL = 518,        -- 占领名城失败
    REPORT_OCCUPY_CAMP_FIXED_SUCCESS = 519,        -- 占领行营成功
    REPORT_OCCUPY_CAMP_FIXED_FAIL = 520,        -- 占领行营失败
    REPORT_GATHER_BATTLE_SUCCESS = 521,        -- 资源采集战斗胜利(打怪)
    REPORT_GATHER_BATTLE_FAIL = 522,        -- 资源采集战斗失败(打怪)
    REPORT_ATTACK_RESOURCE_SUCCESS = 523,        -- 攻击资源点胜利(打人)
    REPORT_ATTACK_RESOURCE_FAIL = 524,        -- 攻击资源点失败(打人)
    REPORT_DEFENSE_RESOURCE_SUCCESS = 525,        -- 资源点防守胜利(打人)
    REPORT_DEFENSE_RESOURCE_FAIL = 526,        -- 资源点防守失败(打人)
    REPORT_ATTACK_MONSTER_SUCCESS = 527,        -- 攻击野怪胜利
    REPORT_ATTACK_MONSTER_FAIL = 528,        -- 攻击野怪失败
    REPORT_ATTACK_CATAPULT_SUCCESS = 529,        -- 攻击箭塔胜利
    REPORT_ATTACK_CATAPULT_FAIL = 530,        -- 攻击箭塔失败
    REPORT_DEFENSE_CATAPULT_SUCCESS = 531,        -- 防守箭塔胜利
    REPORT_DEFENSE_CATAPULT_FAIL = 532,        -- 防守箭塔失败
    REPORT_ATTACK_WORLDBOSS_SUCCESS = 533,        -- 攻击世界BOSS成功
    REPORT_ATTACK_WORLDBOSS_FAIL = 534,        -- 攻击世界BOSS失败
    REPORT_PATROL_MAIL = 601,        -- 巡逻邮件
    SCOUT_DEFENSE_SUCCESS = 1001,        -- 侦查防守胜利,避免了被对方侦查
    SCOUT_DEFENSE_FAIL = 1002,        -- 侦查防守失败,被对方侦查
    SCOUT_ATTACK_CASTLE_FAIL = 1003,        -- 侦查失败(通用城池,名城,行营,驻扎地,资源地)
    SCOUT_ATTACK_CASTLE_SUCCESS = 1004,        -- 侦查玩家城池成功
    SCOUT_ATTACK_FAMOUS_CITY_SUCCESS = 1005,        -- 侦查名城成功
    SCOUT_ATTACK_CAMP_FIXED_SUCCESS = 1006,        -- 侦查行营成功
    SCOUT_ATTACK_CAMP_TEMP_SUCCESS = 1007,        -- 侦查驻扎地成功
    SCOUT_ATTACK_RESOURCE_SUCCESS = 1008,        -- 侦查资源地成功
    GATHER_FOOD = 1501,        -- 粮食采集报告
    GATHER_WOOD = 1502,        -- 木材采集报告
    GATHER_STONE = 1503,        -- 石头采集报告
    GATHER_IRON = 1504,        -- 铁矿采集报告
    PRIVATE_MAIL = 1701,        -- 个人邮件
    PRIVATE_ALLIANCE_MEMBER = 1702,        -- 联盟全体邮件
}

-----
-- 邮件删除类型
M.MailDeleteType = {
    EXIST = 0,        -- 存在
    USER_DELETE = 1,        -- 用户删除
    BEYOND_DELETE = 2,        -- 超出上限删除
    EXPIRE_DELETE = 3,        -- 过期删除
}

-----
-- 任务类型
M.QuestType = {
    DAILY = 1,        -- 日常 
    NORMAL = 2,        -- 普通（支线）
    NOVICE = 3,        -- 推荐(主线）
    STORY = 4,        -- 剧情任务
    OTHER = 5,        -- 其它
}

-----
-- 任务条件类型
M.QuestCondType = {
    PASS_SPECIFY_SCENARIO_CHAPTER = 2001,        -- 通关XX章节
    PASS_SPECIFY_SCENARIO = 2002,        -- 通关XX关卡
    UPGRADE_BUILDING_LEVEL = 3001,        -- XX建筑达到X级
    UPGRADE_RES_BUILDING_LEVEL = 3002,        -- XX个资源建筑达到XXX级
    HERO_NUM = 4001,        -- 拥有武将个数达到XX个
    PURPLE_HERO_NUM = 4002,        -- 拥有XX品质武将达到X个
    HERO_NUM_LEVEL = 4003,        -- 拥有X个达到XX级的武将
    HERO_NUM_STAR = 4004,        -- 拥有X星武将达到X个
    GET_HERO = 4005,        -- 招募获得武将x
    GET_HERO_NUM = 4006,        -- 招募获得x个武将
    HERO_ACCUMULATIVE_LEVELUP = 4007,        -- 武将累计升x级
    HERO_LEVELUP = 4008,        -- 获得x个x级武将
    UPGRADE_HERO_SKILL_NUM = 4009,        -- 升级x次武将技能
    UPGRADE_HERO_SKILL = 4010,        -- 有x个武将技能升级到x级
    GET_HERO_TYPE_NUM = 4011,        -- 招贤馆中普通招募N次
    EQUIP_LEVELUP_NUM_LEVEL = 5001,        -- XX件装备强化到XX级
    ARENA_WIN_NUM = 6001,        -- 擂台胜利次数
    BABEL_WIN_NUM = 6002,        -- 通关千重楼到X层
    ATTACK_PLAYER_WIN_NUM = 6003,        -- 攻击其他玩家并获胜X次
    TELEPORT = 6006,        -- 迁城X次
    USE_RES_ITEM = 6007,        -- 使用资源道具达到X个
    SCOUT_NUM = 6008,        -- 侦查X目标X次
    USE_SPEED_ITEM = 6009,        -- 使用加速道具X次
    BRONZE_NUM = 6010,        -- 铜雀台中答题N次
    BRONZE_RIGHT_NUM = 6011,        -- 铜雀台中单次答对N题
    ARENA_FIGHT_NUM = 6101,        -- 擂台挑战次数
    ARENA_RANK = 6102,        -- 擂台中最高排名进入N名
    PLAYER_POWER = 7001,        -- 战力达到XXX
    PLAYER_LEVEL = 7002,        -- 城主等级达到XXX
    TOTAL_ADD_SILVER = 7003,        -- 累计获得银两XX
    BUILDING_TRAP = 8001,        -- 建造陷阱达到X个
    TRAIN_ARMY = 8002,        -- 建造部队达到X个(单个兵种数量和所有兵的总量 param1为0表示所有)
    TRAIN_ARMY_NUM = 8003,        -- 训练X兵种达到X个
    HEAL_ARMY_NUM = 9001,        -- 治疗x个伤兵
    HEAL_ARMY_TIME = 9002,        -- 治疗伤兵x次
    KILL_MONSTER_NUM = 9101,        -- 击杀x次x级野怪
    TAKE_RESOURCE_TAX_NUM = 9201,        -- 征收x次x资源
    TAKE_RESOURCE_TAX = 9202,        -- 征收x个x资源
    GATHER_RESOURCE_NUM = 9301,        -- 采集x次资源
    GATHER_RESOURCE = 9302,        -- 采集x资源x个
    MOPUP_SCENARIO = 9401,        -- 扫荡x次x副本
    PASS_SCENARIO_COPY_CHAPTER = 9501,        -- 通过x关卡并获得x数量的星星
    ALLIANCE_JOIN = 9601,        -- 加入一个同盟
    ALLIANCE_DONATE_NUM = 9701,        -- 捐献x次联盟科技
    ALLIANCE_HELP_NUM = 9801,        -- 帮助x次同盟玩家
    SHOP_BUY_ITEM = 9901,        -- 在x(特定商城)购买x次道具
    SCIENCE_STUDY = 10001,        -- 研究x科技
    SCIENCE_STUDY_NUM = 10002,        -- 研究科技x次
    OCCUPY_CITY = 10101,        -- 占领x（城池类型）城池
    FINISH_QUEST = 10201,        -- 完成特定任务
    USER_UPGRADE = 11001,        -- 主公升至N级
    RESOURCE_OUTPUT = 13001,        -- X资源的基础产量达到N/小时
    BE_SCOUT_NUM = 900001,        -- 被侦查X次
    TAKE_TAX = 900002,        -- 购买XX达到XX次
    CREATE_BUILDING = 900003,        -- 建造X资源地达到X个
    PASS_SPECIFY_SCENARIO_NUM = 900004,        -- 通关指定关卡次数
}

-----
-- 日常条件类型
M.DailyTaskCondType = {
    TRAIN_ARMY = 1001,        -- 训练x次预备兵
    SPEED_TRAIN = 1002,        -- 加速训练x次
    HEAL_ARMY = 1004,        -- 治疗x次伤兵
    COLLECT_RESOURCE = 1005,        -- 收集x次资源
    GATHER_FOOD = 1006,        -- 采集x次粮食
    UPGRADE_HERO_SKILL = 1007,        -- 升级x次武将技能
    UPGRADE_HERO = 1008,        -- 武将累计提升x级
    DONATE_ALLIANCE_SCIENCE = 1009,        -- 捐献x次同盟科技
    HELP_ALLIANCE = 1010,        -- 帮助x次同盟玩家
    ALLIANCE_STORE_BUY = 1011,        -- 在同盟商城买x次物品
    BABEL_WIN_NUM = 1012,        -- 通关千层楼x层x次
    PASS_NORMAL_SCENARIO = 1013,        -- 通关普通关卡x次
    PASS_ELITE_SCENARIO = 1014,        -- 通关精英关卡x次
    ARENA_WIN_NUM = 1015,        -- 擂台胜利x次
    TAKE_TAX = 1016,        -- 税收x次
    GOLD_TAKE_TAX = 1017,        -- 元宝征收x次(兑换金币)
    BRONZE_NUM = 1018,        -- 铜雀台答题x次
    UPGRADE_BUILDING = 1019,        -- 升级建筑x次
    BUILDING_SPEED = 1020,        -- 建筑加速x次
    ROB_PLAYER = 1021,        -- 掠夺其它玩家x次
    DRAW_GET_HERO = 1022,        -- 招贤馆中招募N次
    ATTACK_MONSTER_NUM = 1152,        -- 进攻N次X级以上野怪
    KILL_MONSTER_NUM = 1153,        -- 消灭x次x级精英野怪
    ARENA_NUM = 1171,        -- 擂台中挑战N次对手
    BUY_STAMINA = 1181,        -- 购买N次体力
    RECHARGE = 1182,        -- 累计充值N元
    CONSUME_GOLD = 1183,        -- 累计消耗N元宝
    STORE_BUY_SHOP = 1184,        -- 在商城中购买N次商品
    USE_BOOST_ITEM = 1185,        -- 使用任意产量提升道具N次
    USE_RESOURCE_ITEM = 1186,        -- 使用任意资源道具N次
    USE_SPEED_ITEM = 1187,        -- 使用任意加速道具N次
}

-----
-- BUFF类型
M.BuffType = {
    RESOURCE_CLLOECT_INCR_PER = 1,        -- 采集资源速度提高百分比
    SMALL_UPKEEP_REDUCTION_PER = 2,        -- 粮食消耗减少百分比
    ATTACK_BONUS_PER = 3,        -- (所有武将和部队)攻击加成百分比
    DEFENSE_BONUS_PER = 4,        -- (所有武将和部队)防御加成百分比
    PEACE_SHIELD = 5,        -- 战争守护
    ANTI_SCOUT = 6,        -- 反侦查
}

-----
-- 成就类型
M.AchievementType = {
    MAP_LAND = 1001,        -- 全地图{数量}格土地被占领
    ALLIANCE_NUM = 1002,        -- 全服（条件）联盟达到{数量}个
    KILL_NPC = 1003,        -- 全服剿灭守兵（怪物）达到{数量}个
    OCCUPY_RES = 1004,        -- 全地图{等级}级资源点被占领{数量}%
    OCCUPY_COUNTY = 1005,        -- 全国县城被占领{数量}%
    OCCUPY_PREFECTURE = 1006,        -- 全地图{数量}个郡城被占领
    OCCUPY_CHOW = 1007,        -- 全地图{数量}个州级城池被占领
    OCCUPY_CAPITAL = 1008,        -- 国都洛阳被占领
    TOTAL_POWER = 1009,        -- 总战力
    GATHER_RES = 1010,        -- 采集总量
    HERO_COUNT = 1011,        -- 武将数量
}

-----
-- 转盘奖品类型
M.turnplateType = {
    multiple = 1,        -- 倍数
    resource = 2,        -- 资源
    prop = 3,        -- 道具
}

-----
-- 玩家设置类型
M.PlayerSettingType = {
    EQUIP_VIEW = 1,        -- 装备他人可见
    RANKING = 2,        -- 参与排行榜
    ACTIVATION_CODE = 100,        -- 激活码功能
    TURNPLATE = 101,        -- 转盘功能
}

-----
-- 活动时间类型
M.ActivityTimeType = {
    ALL_TIME = 1,        -- 不限时
    OPEN_SERVER = 2,        -- 开服
    LIMIT = 3,        -- 限时
}

-----
-- 活动类型
M.ActivityType = {
    NEXT_LOGIN_GIVE_HERO = 1,        -- 次日登录送武将
    ACCUMULATE_LOGIN = 2,        -- 累计登录(七日签到)
    LOGIN_EVERYDAY = 3,        -- 每日登录
    TIME_MEAL = 4,        -- 限时大餐
    TARGET = 5,        -- 目标活动
    EXCHANGE = 6,        -- 兑换活动
    WORLD_BOSS = 7,        -- 世界BOSS活动
}

-----
-- 活动条件类型
M.ActivityCondType = {
    RECHARGE = 1,        -- 充值
    CONSUME = 2,        -- 消费
    VIP = 3,        -- VIP
}

-----
-- 任务条件类型
M.SevenTargetCondType = {
    SIGN_NUM = 1,        -- 签到x次
    UPGRADE_BUILDING_LEVEL = 2,        -- 升级A建筑到B级
    TRAIN_INFANTRY_NUM = 3,        -- 训练X个步兵
    TRAIN_CAVALRY_NUM = 4,        -- 训练X个骑兵
    TRAIN_ARCHER_NUM = 5,        -- 训练X个弓兵
    TRAIN_CHARIOT_NUM = 6,        -- 训练X个车兵
    GATHER_FOOD_NUM = 7,        -- 采集X单位的粮食
    GATHER_WOOD_NUM = 8,        -- 采集X单位的木材
    GATHER_IRON_NUM = 9,        -- 采集X单位的铁矿
    GATHER_STONE_NUM = 10,        -- 采集X单位的石料
    UPGRADE_USER_LEVEL = 11,        -- 玩家等级提升到X级
    ATTACK_MONSTER_WIN = 12,        -- 战胜X个怪物
    ALLIANCE_HELP = 13,        -- 联盟帮助X次
    ATTACK_CASTLE = 14,        -- 攻击他人城堡X次
    STORE_USE_GOLD = 15,        -- 商店花出X个金币
    ALOCATE_SKILLS_POINT = 16,        -- 分配X个技能点
    KILL_NUM = 17,        -- 杀敌数达到X
    CAPTIVE_NUM = 18,        -- 拥有X个奴隶
    POWER_NUM = 19,        -- 战斗力达到X
    ALLIANCE_RESOURCE = 20,        -- 资源援助盟友X次
    ALLIANCE_REINFORCEMENT = 21,        -- 兵力援助盟友X次
    ALLIANCE_DONATE = 22,        -- 联盟捐献X次
    PRODUCE_EQUIP = 23,        -- 锻造X件装备
    WEAR_EQUIP = 24,        -- 穿上X件装备
    BUILD_TRAP_NUM = 25,        -- 建造X个陷阱
    ALLIANCE_EXPLOR_TIME = 26,        -- 联盟探险X次
    RESEARCH_TECT_TIMES = 27,        -- 研究X次科技
    EXCHANGE_RESOURCE = 28,        -- 置换获得X单位资源
    SCOUT_TIMES = 29,        -- 侦查他人城堡X次
}

-----
-- 推送组
M.PushGroup = {
    FIGHT = 1,        -- 军事
    EVENT = 2,        -- 事件完成
    REWARD = 3,        -- 领取奖励
}

-----
-- 推送分类
M.PushClassify = {
    CASTLE_ATTACTED = 1,        -- 您的城市被X攻击了
    CAMP_ATTACTED = 2,        -- 您的驻军被X攻击了
    SCOUTED = 3,        -- 您被X侦查了
    TROOP_BACK = 4,        -- 军队已返回城市
    BUILDING_UP = 21,        -- X建筑升级完成
    TECHNOLOGY_UP = 22,        -- X科技已研究完成
    ARMY_TRAINED = 23,        -- X部队已经完成训练
    TRAP_BUILDED = 24,        -- X陷阱已经建造完成
    ARMY_HEALED = 25,        -- 伤兵治疗已完成
    EQUIP_FORGED = 26,        -- X装备锻造完成
    ALLIANCE_EXPLORE_DONE = 27,        -- 联盟探险完成了
    ONLINE_REWARD = 41,        -- 货船入港，可以收取物资了
    ALLIANCE_GIFT = 42,        -- 有新的联盟礼物可以领取
    CALLBACK = 81,        -- 领主大人，你好久没来城堡看看了
}

-----
-- 消息队列类型
M.MessageQueueType = {
    MARCH = 1,        -- 出征(各种情况)
    MARCH_BACK = 2,        -- 出征返回(各种情况)
    GATHER_RESOURCE = 3,        -- 资源采集
    ATTACK_MONSTER = 4,        -- 打野怪(胜利和失败)
    ATTACK_RESOURCE = 5,        -- 抢占资源(胜利和失败)
    ATTACK_CASTLE = 6,        -- 攻城守城(胜利和失败)
    SCOUT_RESULT = 8,        -- 侦查结果(胜利和失败)
    RUINS_EXPLOR = 9,        -- 遗迹探险
    BUFF_REMOVE = 10,        -- BUFF移除
    RESOURCE_HELP = 11,        -- 资源(被)帮助
    ATTACK_PALACE = 12,        -- 攻击王城
    ATTACK_NEUTRAL_CASTLE = 13,        -- 攻击中立城池
    NEUTRAL_CASTLE_NOTICE_MAIL = 14,        -- 中立城池提示邮件
    MONSTER_SIEGE = 15,        -- 怪物攻城
    MONSTER_SIEGE_RESOURCE_GET_BACK = 16,        -- 怪物攻城资源抢回
    ATTACK_CATAPULT = 17,        -- 攻击炮塔
    BE_ATTACK_BY_CATAPULT = 18,        -- 被箭塔攻击
    TROOP_REACH_INVALID = 19,        -- 行军到达无效
    ATTACK_MONSTER_INVALID = 20,        -- 攻击怪物无效
    CASTLE_REBUILD = 21,        -- 城堡重建
    KILL_DRAGON_RANK = 22,        -- 杀龙排行
    KILL_DRAGON_LAST_ATTACK = 23,        -- 杀龙最后一击
    REINFORCEMENTS = 24,        -- 士兵(被)援助
    EXPLORE_MYSTERIOUS_CITY = 25,        -- 探索神秘城市
    ATTACK_GOBLIN_CAMP = 26,        -- 攻击地精营地(胜利和失败)
    ATTACK_CITY = 27,        -- 攻打名城(胜利和失败)
    CITYDEFENSE_UPDATE = 28,        -- 城防值更新
    CREATE_CAMP_FIXED_FAILED = 29,        -- 创建行营失败
    ATTACK_CAMP_FIXED = 30,        -- 攻击行营(胜利和失败)
    OCCUPY_CAMP_FIXED = 31,        -- 占领行营(胜利和失败)
    ATTACK_CAMP_TEMP = 32,        -- 攻击驻扎(胜利和失败)
    CITY_PATROL = 33,        -- 巡逻
    BEAT_CASTLE = 34,        -- 击败玩家城池（胜利和失败）
    ATTACK_WORLDBOSS = 35,        -- 攻击世界BOSS（胜利和失败）
    ATTACK_WORLDBOSS_END = 36,        -- 攻击世界BOSS结算
    ATTACK_CITY_END = 37,        -- 攻击名城结算
    ATTACK_CATAPULT_END = 38,        -- 攻击箭塔结算
    TRANSPORT = 39,        -- 运输成功
    COMPENSATE = 40,        -- 战斗死兵补偿
    CASTLE_DEFENER_FILL = 41,        -- 玩家城池补兵
}

-----
-- 地图中的元件
M.MapUnitType = {
    UNKNOWN = 0,        -- 空地
    CAPITAL = 1,        -- 都城(王城)
    CHOW = 2,        -- 州城
    PREFECTURE = 3,        -- 郡城
    COUNTY = 4,        -- 县城
    CASTLE = 5,        -- 玩家城池
    CAMP_TEMP = 6,        -- 驻扎
    CAMP_FIXED = 7,        -- 行营
    FARM_FOOD = 8,        -- 农田
    FARM_WOOD = 9,        -- 林场
    MINE_IRON = 10,        -- 铁矿
    MINE_STONE = 11,        -- 晶石
    MONSTER = 12,        -- 野怪
    TREE = 13,        -- 树
    CATAPULT = 14,        -- 王城箭塔
    WORLD_BOSS = 15,        -- 世界BOSS
}

-----
-- 城池状态
M.CastleState = {
    NORMAL = 0,        -- 正常
    PROTECTED = 1,        -- 保护
    BURN = 2,        -- 燃烧
    PROTECTED_AND_BURN = 3,        -- 保护和燃烧
    NOVICE = 4,        -- 新手保护
    OCCUPY = 5,        -- 被占领状态
    SOVEREIGN = 6,        -- 主权状态
}

-----
-- 王城状态
M.PalaceState = {
    NOSTART = 0,        -- 未开启
    NPC = 1,        -- NPC状态
    OCCUPY = 2,        -- 占领状态
    CHOOSE = 3,        -- 推举状态
    PROTECTED = 4,        -- 保护状态
}

-----
-- 地图中的队伍类型
M.MapTroopType = {
    UNKNOWN = 0,        -- 未定义
    GATHER = 1,        -- 采集
    MONSTER = 2,        -- 打怪
    SIEGE_CITY = 3,        -- 攻击名城
    SIEGE_CASTLE = 4,        -- 攻击玩家城池
    CAMP_TEMP = 5,        -- 驻扎
    CAMP_FIXED = 6,        -- 行营
    SCOUT = 7,        -- 侦查
    CAMP_TEMP_ATTACK = 8,        -- 攻击驻扎
    CAMP_FIXED_ATTACK = 9,        -- 攻击行营
    CAMP_FIXED_OCCUPY = 10,        -- 占领行营(驻守行营)
    CAMP_FIXED_DISMANTLE = 11,        -- 拆除行营
    SIEGE_CATAPULT = 12,        -- 攻击箭塔
    WORLDBOSS = 13,        -- 攻击世界BOSS
    PATROL_CITY = 15,        -- 巡逻
    ASSIGN = 16,        -- 调兵
    TRANSPORT = 17,        -- 运输
    REINFORCEMENTS = 21,        -- 援兵
}

-----
-- 行军状态
M.MapTroopState = {
    MARCH = 1,        -- 出征
    REACH = 2,        -- 到达
    GO_HOME = 3,        -- 返回
    REMOVE = 9,        -- 移除
}

-----
-- 巡逻事件
M.PatrolEvent = {
    EMPTY = 1,        -- 空
    FIGHT = 2,        -- 战斗事件
    BUFF = 3,        -- BUFF事件
    REWARD = 4,        -- 奖励事件
}

-----
-- 资源带等级
M.ResAreaType = {
    FIRST = 1,        -- 一级资源带
    SECOND = 2,        -- 二级资源带
    THIRD = 3,        -- 三级资源带
    FOURTH = 4,        -- 四级资源带
    FIFTH = 5,        -- 五级资源带
}

-----
-- 出征错误码
M.MarchErrorNo = {
    SUCCESS = 0,        -- 成功
    OTHER = 1,        -- 其他
    SIEGE_TARGET_NOT_FOUND = 2,        -- 攻城找不到目标
    GATHER_TARGET_NOT_FOUND = 3,        -- 采集找不到目标
    SCOUT_TARGET_NOT_FOUND = 4,        -- 侦查找不到目标
    MONSTER_TARGET_NOT_FOUND = 5,        -- 打怪找不到目标
    TARGET_NOT_A_CAMP = 6,        -- 目标不是营地
    TARGET_CAN_NOT_PLACE = 7,        -- 目的地不能放置营地
    TARGET_INVALID = 10,        -- 目标无效
    CASTLE_IS_PROTECTED = 11,        -- 该城池处于保护状态之中
}

-----
-- 王国地图单元类型
M.KingMapUnitType = {
    ALLLIANCE_LEADER = 0,        -- 联盟盟主
    ALLLIANCE_MEMBER = 1,        -- 联盟成员
    ALLIANCE_NEUTRAL_CASTLE = 2,        -- 联盟萌城
    DRAGON_NEST = 3,        -- 龙穴
    MY_CASTLE = 5,        -- 自己的城堡
}

-----
-- 联盟阶级权限
M.AllianceRankPermitType = {
    TRANSFER = 1,        -- 盟主转让
    REPLACE_LEADER = 2,        -- 取代盟主
    ALLIANCE_INVITE = 3,        -- 邀请入盟
    ALLIANCE_ACCEPT_JOIN = 4,        -- 批准入盟
    KICK = 5,        -- 踢人
    RANK_UP = 6,        -- 提升阶级
    RANK_DOWN = 7,        -- 降低阶级
    CHANGE_BANNER = 8,        -- 修改联盟旗帜
    CHANGE_ANNOUNCEMENT = 9,        -- 修改联盟宣言
    BUFF = 10,        -- BUFF授予
    MAIL = 11,        -- 全体邮件
    FORCE_RECALL_TROOPS = 12,        -- 强制撤回名城驻守部队
}

-----
-- 捐献类型
M.AllianceDonateType = {
    DONATE_RESOURCE = 1,        -- 普通捐献
    DONATE_GOLD = 2,        -- 元宝捐献
    DONATE_PROP = 3,        -- 道具捐献
}

-----
-- 消息类型
M.AllianceMessageType = {
    ALLIANCE_JOIN = 1,        -- 加入联盟
    ALLIANCE_QUIT = 2,        -- 退出联盟
    ALLIANCE_KICK = 3,        -- 踢出联盟
    RANK_UP = 4,        -- 职位提升
    RANK_DOWN = 5,        -- 职位降低
    LEADER_TRANSFER = 6,        -- 盟主转让
    OPEN_ALLIANCE_BUFF = 7,        -- 开启联盟buff
    ALLIANCE_UPGRADE = 8,        -- 联盟升级
    SCIENCE_UPGRADE = 9,        -- 联盟科技升级
    NEUTRALCASTLE_OCCUPY = 10,        -- 占领中立城池
    NEUTRALCASTLE_ROBBED = 11,        -- 中立城池被抢
    NEUTRALCASTLE_BE_ATTACKED = 12,        -- 中立城池被攻击
    ALLY_BE_ATTACKED = 13,        -- 盟友被攻击
    TROOP_BE_REPATRIATED = 14,        -- 名城驻军被遣返
    HERO_HIRE_HAD_TIMES = 101,        -- 武将出租(有剩余次数)
    HERO_HIRE_NO_TIMES = 102,        -- 武将出租(没有剩余次数)
}

-----
-- 联盟BUFF类型
M.AllianceBuffType = {
    PEACE_SHIELD = 1,        -- 战争守护
    ANTI_SCOUT = 2,        -- 反侦查
    RESOURCE_CLLOECT_INCR_PER = 3,        -- 采集资源速度提高百分比
    TRAP_ATTACK_PER = 4,        -- 陷阱攻击提高百分比
    CITY_WALL_RECOVER = 5,        -- 所有联盟成员城池城墙恢复百分比
    HERO_DEFENSE_PER = 6,        -- 武将防御提高百分比
    HERO_ATTACK_PER = 7,        -- 武将攻击提高百分比
    ARMY_ATTACK_PER = 8,        -- 部队攻击提高百分比
    MARCH_SPEED_PER = 9,        -- 行军速度提高百分比
    NEUTRAL_CASTLE_WALL_RECOVER = 10,        -- 联盟中立城池城墙恢复百分比
}

-----
-- 聊天类型
M.ChatType = {
    KINGDOM = 1,        -- 国家聊天
    ALLIANCE = 2,        -- 联盟聊天
    ALL = 3,        -- 所有聊天频道(国家+联盟)
}

-----
-- 聊天子类型
M.ChatSubType = {
    NORMAL = 0,        -- 正常
    ALLIANCE_CREATE = 1,        -- 联盟创建-国家频道可见
    ALLIANCE_SHARE_MAIL = 2,        -- 联盟分享邮件-国家频道可见
    OCCUPY_FAMOUS_CITY = 3,        -- 占领名城-国家/联盟频道可见
    ALLIANCE_LEADER_TRANSFER = 4,        -- 盟主转让
}

-----
-- 排行榜子类型
M.RangeType = {
    LORD_POWER = 1,        -- 领主战斗力
    LORD_KILLS = 2,        -- 领主杀敌数量
    LORD_CASTLE = 3,        -- 领主城堡
    LORD_LEVEL = 4,        -- 领主等级
    ALLIANCE_POWER = 10,        -- 联盟战斗力
    ALLIANCE_KILLS = 11,        -- 联盟杀敌数量
    ARENA_RANK = 20,        -- 擂台排名
    BABEL_RANK = 21,        -- 千层楼排名
    BRONZE_HISTORY_RANK = 22,        -- 铜雀台历史积分排名
    BRONZE_TODAY_RANK = 23,        -- 铜雀台当天积分排名
}

-----
-- 武将阵营类型
M.heroCampType = {
    WEI_STATE = 1,        -- 魏
    SHU_STATE = 2,        -- 蜀
    WU_STATE = 3,        -- 吴
    QUN_STATE = 4,        -- 群
}

-----
-- 武将抽取类型
M.heroDrawType = {
    GENERAL = 1,        -- 良将
    STAR = 2,        -- 名将
    HOT = 3,        -- 热门
}

-----
-- 穿戴槽位类型
M.SlotType = {
    SKILL = 1,        -- 技能槽
    EQUIP = 2,        -- 装备槽
    TREASURE = 3,        -- 宝物槽
}

-----
-- 武将技能类型
M.HeroSkillType = {
    HERO_SKILL_PASSIVE = 1,        -- 被动技
    HERO_SKILL_STAR = 2,        -- 星级技
    HERO_SKILL_TALENT = 3,        -- 天赋技
    HERO_SKILL_COOPERATE = 4,        -- 合体技
    HERO_SKILL_FETTER = 5,        -- 羁绊技
}

-----
-- 章节类型
M.ChapterType = {
    NORMAL = 1,        -- 普通
    ELITE = 2,        -- 精英
    EPIC = 3,        -- 史诗
}

-----
-- 商店类型
M.StoreType = {
    STORE_GOLD = 1,        -- 元宝商店
    STORE_ARENA = 2,        -- 擂台商店
    STORE_ALLIANCE = 3,        -- 联盟商店
    STORE_BABEL = 4,        -- 千层楼商店
    STORE_BRONZE_SPARROW_TOWER = 5,        -- 铜雀台商店
    STORE_MEDAL = 6,        -- 功勋商店(千里走单骑)
}

-----
-- 商店货币类型
M.StoreCoinType = {
    GOLD = 1,        -- 元宝
    WIN_POINT = 2,        -- 胜点
    ALLIANCE_SCORE = 3,        -- 联盟积分
    SILVER = 4,        -- 银两
    BABEL_SCORE = 5,        -- 千层楼积分
    BRONZE_SPARROW_TOWER_SCORE = 6,        -- 铜雀台积分
    MEDAL = 7,        -- 功勋
}

-----
-- 擂台奖励类型
M.ArenaRewardType = {
    ARENA_RANK = 1,        -- 擂台排名奖
    ARENA_MAX_RECORD = 2,        -- 擂台最高记录奖
}

-----
-- 科目类型
M.AnswerSubjectType = {
    UNKNOWN = 0,        -- 没有选科目
    HISTORY = 1,        -- 历史
    LITERATURE = 2,        -- 文学
    GEOGRAPHY = 3,        -- 地理
    COMMON_SENSE = 4,        -- 常识
    DIET = 5,        -- 饮食
}

-----
-- 千里单骑状态
M.RidingAloneState = {
    INIT = 0,        -- 初始
    READY = 1,        -- 准备
    COMBAT = 2,        -- 战斗
    DIE = 3,        -- 死亡
}

-----
-- 系统解锁类型
M.SystemUnlockType = {
    BUILDING_LEVEL = 1,        -- 建筑等级
    HERO_LEVEL = 2,        -- 武将等级
    VIP_LEVEL = 3,        -- VIP等级
    TASK_LIMIT = 4,        -- 任务限制
    ACHIEVEMENT_LIMIT = 5,        -- 成就限制
    TECHNOLOGY_LIMIT = 6,        -- 科技限制
    USER_LEVEL = 7,        -- 城主等级
    USER_HEAD = 8,        -- 头像解锁{武将ID,武将等级，武将星级}
    HERO_STAR = 9,        -- 武将星级
    EQUIP_LEVEL = 10,        -- 装备等级
    SECTION_LIMIT = 11,        -- 关卡限制
    HERO_NUMS = 12,        -- 武将数量
    OWN_CITY = 13,        -- 拥有名城
}

-----
-- 王城争霸记录类型
M.PalaceWarRecordType = {
    BE_KING = 1,        -- (联盟简称)玩家名称 成为国王
    OCCUPY_PALACE = 2,        -- (联盟简称)玩家名称 占领王城
    ATTACK_PLAYER_FAIL = 3,        -- (联盟简称)玩家名称 击退 (联盟简称)玩家名称 守住王城
    ATTACK_NPC_FAIL = 4,        -- (联盟简称)玩家名称 被NPC打败
    KILL_PALACE_NPC = 5,        -- (联盟简称)玩家名称 击杀NPC
    OCCUPY_CATAPULT = 6,        -- (联盟简称)玩家名称 占领魔法塔
    ATTACK_CATAPULT_PLAYER_FAIL = 7,        -- (联盟简称)玩家名称 击退 (联盟简称)玩家名称 守住箭塔
    ATTACK_CATAPULT_NPC_FAIL = 8,        -- (联盟简称)玩家名称 被NPC打败
    KILL_CATAPULT_NPC = 9,        -- (联盟简称)玩家名称 击杀NPC
    CATAPULT_ATTACK_NPC = 10,        -- (联盟简称)玩家名称 累计消灭了王城 NPC 的 N 个士兵
    CATAPULT_ATTACK_PLAYER = 11,        -- (联盟简称)玩家名称 累计消灭了王城 YYY联盟 的 N 个士兵
}

-----
-- 称号类型 与模板表ID相等
M.TitleType = {
    KING = 1,        -- 国王
}

-----
-- 战报武将部队数据类型
M.HeroArmyDataType = {
    INIT = 1,        -- 开始时
    TowerTrap = 2,        -- 陷阱
    SOLOEND = 3,        -- 单挑结束
}

-----
-- 部队状态
M.MapTeamState = {
    LOCKED = 1,        -- 锁定
    IDLE = 2,        -- 闲置
    DEFENDERS = 3,        -- 守军
    MARCHING = 4,        -- 行军
    GO_HOME = 5,        -- 返回
    Camping = 6,        -- 行营中
    GARRISON = 7,        -- 驻防
    GATHER = 8,        -- 采集
}

-----
-- 运输类型
M.TransportType = {
    TRANSMIT = 0,        -- 传输
    RECEIVE = 1,        -- 接收
}

-----
-- 运输到达类型
M.TransportArriveType = {
    FAIL = 0,        -- 失败
    SUCCESS = 1,        -- 成功
    TRANSPORTING = 2,        -- 运输中
}

-----
-- 错误码类型
M.ErrorCode = {
    SUCCESS = 1,        -- 成功
    FAIL = 2,        -- 失败
    UNKNOWN = 3,        -- 未知错误
    PUBLIC_FREE_INVALID = 2421001,        -- 免费无效
    PUBLIC_PROP_NOT_ENOUGH = 2417001,        -- 道具不足
    NAME_IS_CONTAINED_BADWORDS = 2403003,        -- 输入的名字包含敏感词
    NAME_LENGTH_LIMIT = 2403004,        -- 名字长度必须在6-18个字符之间
    NAME_WAS_USED = 2403005,        -- 名字已经被使用
    NAME_IS_ILLEGAL = 2403006,        -- 名字不合法
    PUBLIC_RESOURCE_NOT_ENOUGH = 2405001,        -- {0}不足，请前往获取
    PUBLIC_ITEMS_NOT_ENOUGH = 2405003,        -- 材料不足，请前往获取
    PUBLIC_GOLD_NOT_ENOUGH = 2405005,        -- 金币不足
    PUBLIC_STAMINA_NOT_ENOUGH = 2405007,        -- 体力不足
    PUBLIC_PROP_USE_SUCCESS = 2405009,        -- 使用道具成功
    PUBLIC_SILVER_NOT_ENOUGH = 2405010,        -- 银两不足,请前往收税
    PUBLIC_VIP_OPEN = 2405016,        -- vip达到N级开启
    PUBLIC_VIP_RESET_MAX = 2405024,        -- 您的重置次数已达到上限，请提升VIP等级增加重置次数
    PUBLIC_VIP_REFRESH_MAX = 2405027,        -- 您的刷新次数已达到上限，请提升VIP等级·刷新次数
    PUBLIC_ENERGY_NOT_ENOUGH = 2405031,        -- 行动力不足
    USER_UPGRADE = 2410001,        -- 城主升级
    USER_CHANGE_HEAD_SUCCESS = 2410002,        -- 头像更换成功
    USER_RENAME_SUCCESS = 2410003,        -- 昵称修改成功
    USER_NOT_CHANGE_HEAD = 2410004,        -- 条件不足，无法更换头像
    USER_NOT_RENAME = 2410005,        -- 条件不足，无法修改名字
    HERO_LEVEL_TOP = 2413002,        -- 当前武将已达到最高等级，请提升城主等级后再来升级武将
    EQUIP_LEVEL_TOP = 2413005,        -- 当前装备等级已达上限，需要提升城主等级后继续强化
    EQUIP_SUCCINCT_NOT_ENOUGH = 2413008,        -- 洗练材料不足
    SKILL_LEVEL_TOP = 2413012,        -- 当前技能已达到最高等级，请提升城主等级后再来升级技能
    SKILL_ACTIVE_MAX = 2413013,        -- 该将领已有2个主动技能，无法再装备主动技能
    BUILDING_OPEN_FOREST_FAIL = 2419001,        -- 开启森林失败
    BUILDING_TRAP_CAPACITY_NOT_ENOUGH = 2403002,        -- 陷阱容量不足
    TRAIN_ARMY_NOT_UNLOCKED = 2415001,        -- 当前兵种未解锁
    TRAIN_QUEUES_NOT_UNLOCKED = 2415002,        -- 当前分组未解锁
    TECHNOLOGY_RESEARCH_NOT_SAN = 2409001,        -- 脑残值不足，无法研究
    TRAIN_ARMY_FINISH = 2415003,        -- 部队训练完成
    JUST_TRAIN_REDIF = 2415004,        -- 只能训练预备兵
    TRAIN_EXCEED_WOUND = 2425005,        -- 训练数超过伤兵数
    SCENARIO_NOT_ACHIEVE_CONDITION = 2420001,        -- 未达到领取条件
    SCENARIO_IS_DREW = 2420002,        -- 该奖励已领取过了
    SCENARIO_NOT_UNLOCK = 2420003,        -- 关卡未解锁
    SCENARIO_FIGHT_COUNT_NOT_ENOUGH = 2420004,        -- 挑战次数不足
    SCENARIO_NO_THREE_STAR = 2420005,        -- 未达3星不可扫荡
    SCENARIO_NOT_LEFT_RESET_COUNT = 2420006,        -- 关卡重置次数不够
    HERO_DEBRIS_NOT_ENOUGH = 2413001,        -- 武将碎片不足
    SKILLPOINT_NOT_ENOUGH = 2414002,        -- 技能点不够
    HERO_SLOT_IS_LOCK = 2414003,        -- 技能槽没解锁
    HERO_SKILL_IS_LOCK = 2414004,        -- 技能没解锁
    SKILL_IS_MAX_LEVEL = 2414005,        -- 技能已经满级
    BAG_SYNTHESIZE_FAIL = 2418003,        -- 合成失败
    BAG_FULL_MAIL = 2418004,        -- 背包已满，道具已发送至邮箱
    BAG_FULL_CLEAR = 2413006,        -- 当前背包已满，请清理背包后继续操作
    STORE_BUY_SUCCESS = 2411001,        -- 购买成功
    STORE_COIN_NOT_ENOUGH = 2411002,        --  {0}不足，无法刷新
    CASTLE_LEVEL_NOT_ENOUGH = 2405014,        -- 城池等级不够
    USER_LEVEL_NOT_ENOUGH = 2405015,        -- 玩家等级不够
    ALLIANCE_JOINED = 2404001,        -- 已加入联盟
    ALLIANCE_NOT_EXIST = 2404002,        -- 联盟不存在
    ALLIANCE_FULL = 2404003,        -- 联盟人数已满
    ALLIANCE_NOT_FOUND_USER = 2404004,        -- 玩家不存在
    ALLIANCE_NOT_MEMBER = 2404005,        -- 不是联盟成员
    ALLIANCE_NAME_REPETE = 2404006,        -- 联盟名字重复
    ALLIANCE_NAME_INVALID = 2404007,        -- 联盟名字无效
    ALLIANCE_CD_COOLING = 2404008,        -- 有联盟CD
    ALLIANCE_APPLY_REPETE = 2404009,        -- 重复申请
    ALLIANCE_WITHOUT_PERMIT = 2404010,        -- 无权限
    ALLIANCE_INVITE_REPETE = 2404011,        -- 重复邀请
    ALLIANCE_SCIENCE_LEVEL_FULL = 2404012,        -- 联盟科技满级
    ALLIANCE_SCIENCE_NOT_OPEN = 2404013,        -- 联盟科技没有开启
    ALLIANCE_CHANGE_BANER_CD_COOLING = 2404014,        -- 有联盟旗帜更换CD
    ALLIANCE_SCIENCE_DONATE_CD = 2404015,        -- 有捐献CD
    ALLIANCE_HELP_REQUEST_INVALID = 2404016,        -- 申请的帮助无效
    ALLIANCE_HELP_REQUEST_REPETE = 2404017,        -- 重复申请帮助
    ALLIANCE_HELP_NOT_FOUND = 2404018,        -- 没有该帮助
    HERO_NOT_EXIST = 2404021,        -- 武将不存在
    NOT_HIRE_OWN_HERO = 2404022,        -- 不能雇佣自己的武将
    HERO_HAD_HIRE = 2404023,        -- 武将已经出租
    HERO_EMPLOY_REPEATE = 2404025,        -- 重复雇佣
    ALLIANCE_BUFF_NOT_EXIST = 2404026,        -- 联盟BUFF不存在
    ALLIANCE_BUFF_CANNOT_OPEN = 2404027,        -- 条件不满足，不可以激活联盟BUFF
    ALLIANCE_BUFF_CANNOT_OPENMULTI = 2404028,        -- 不可以同时激活多个联盟BUFF
    ALLIANCE_BUFF_USE_MAX = 2404029,        -- 联盟BUFF激活次数已达上限
    ALLIANCE_SEND_MAIL_MAX = 2404034,        -- 盟群发邮件次数已达上限
    ALLIANCE_NOT_RANK_UP = 2404041,        -- 无法将盟友升职
    ALLIANCE_NOT_RANK_DOWN = 2404042,        -- 已达到最低阶级
    RECAL_HERO_TIME_NOT_OVER = 2404047,        -- 武将召回时间还没到
    ALLIANCE_INVITE_INVALID = 2404049,        -- 无效邀请
    ALLIANCE_LEADER_TRANSFER = 2404050,        -- 请向你的副盟主转让盟主职位
    ARENA_HERO_NOT_ENOUGH = 2416001,        -- 上阵人数不足
    ARENA_CD_COOLING = 2416002,        -- 冷却时间未到，请稍后再来挑战
    ARENA_NO_REWARD = 2416003,        -- 当前暂无奖励
    ARENA_RESET_BATTLE_COUNT = 2416005,        -- 是否立即重置挑战次数
    BABEL_NOT_MOPUP = 2423006,        -- 还未通关第{0}层，无法扫荡
    TELEPORT_ARMY_OUT = 2422002,        -- 有部队未回城，无法迁城
    TELEPORT_SUCCESS = 2422004,        -- 迁城成功
    AMI_POSITION_CANNOT_TELEPORT = 2422006,        -- 目标位置无法迁城
    TROOP_TEAM_FULL = 2412004,        -- 行军队列已满，无法出兵
    TROOP_CAMPFIXED_FULL = 2407006,        -- 当前已有{0}个行营，无法再建了
    BARRACKS_NOT_CREATE = 2407008,        -- 兵营还未开启
    TROOP_CAMPFIXED_EXIST_ARMY = 2412010,        -- 该行营已有部队
    TROOP_SIEGECITY_NO_ALLIANCE = 2404024,        -- 没有加入联盟，无法攻城
    TROOP_SIEGECITY_LIMITED_CITY = 2404048,        -- 你的联盟只能占领x个城池
    TROOP_PATROLCITY_SUCCESS = 2424003,        -- 巡逻成功
    TROOP_COUNTY_CITY_NOT_ENOUGH = 2435001,        -- 同盟需要再拥有{0}座县城才能占领郡城
    TROOP_PREFECTURE_CITY_NOT_ENOUGH = 2435002,        -- 同盟需要再拥有{0}座郡城才能占领州城
    HERO_ARMYCOUNT_CANNOT_ZERO = 2435003,        -- 武将带领的士兵数量不能为0
    HERO_NUM_CANNOT_EXCEED = 2435004,        -- 上阵武将数量不能超过{0}
    ARMY_NUM_EXCEEDING_MAX = 2435005,        -- 上阵士兵不能超过上限
    HERO_IS_MARCHING = 2435006,        -- 该武将已经出征
    CASTLE_IS_PROTECTED = 2435008,        -- 该城池处于保护状态之中
    CANNOT_SEARCH_AIM = 2435009,        -- 未找到目标
    CANNOT_ANSWER_NOW = 2435013,        -- 当前时间不可答题
    SCOUT_TROOP_FULL = 2435014,        -- 侦察队列已满
    BRONZE_SUBJECT_TYPE_NOT_MATCH = 2425002,        -- 铜雀台科目类型不匹配
    BRONZE_SUBJECT_COUNT_NOT_MATCH = 2425003,        -- 铜雀台科目题量不匹配
    ACTIVITY_NOT_EXIST = 2428001,        -- 活动不存在
    ACTIVITY_TIME_INVALID = 2428002,        -- 时间无效
    ACTIVITY_HAS_DRAWN = 2428003,        -- 已领取
    ACTIVITY_INFO_NOT_EXIST = 2428004,        -- 玩家活动不存在
    ACTIVITY_TARGET_PROGRESS_NOT_ENOUGH = 2428005,        -- 目标活动进度不够
    ACTIVITY_EXCHANGE_TIMES_FULL = 2428006,        -- 兑换次数达到上限
    ACTIVITY_RESOURCE_NOT_ENOUGH = 2428007,        -- 资源不足(兑换或者领取时所需资源不足)
    MAIL_ALLIANCE_SHARE_SUCCESS = 2427002,        -- 邮件分享成功
    PALACEWAR_IS_NOT_START = 2429001,        -- 王城战未开启
    RESOURCE_IS_NOT_ENOUGH = 2440001,        -- 资源不足
    TRANSPORT_IS_EXCEED = 2440002,        -- 运输数量超出限制
    TRANSPORT_FAIL = 2440003,        -- 运输失败
    TRANSPORT_MEMBER_ONCE_LIMIT = 2440004,        -- 同一个玩家只能运输一次
}

-----
-- 擂台数据更新类型
M.ArenaDataType = {
    HERO_LEVEL_STAR = 1,        -- 武将等级星级
    HERO_SOUL = 2,        -- 武将命魂
    HERO_SKILL = 3,        -- 武将技能等级
    USER_DATA_CHANGE = 4,        -- 玩家等级头像昵称
}





    
M.CS_TEMPLATE_CHECK = 1
M.CS_LOGIN = 5
M.CS_PING_REPLY = 9
M.CS_SET_OR_CHECK_NICKNAME = 20
M.CS_SET_LANG_TYPE = 22
M.CS_STAMINA_BUY = 24
M.CS_GET_RANDOM_NAME = 32
M.CS_SET_NAME = 34
M.CS_FILL_RESOURCE = 36
M.CS_ENERGY_BUY = 39
M.CS_BAG_USE = 51
M.CS_BAG_BUY = 53
M.CS_BAG_SELL = 55
M.CS_BAG_SYNTHESIZE = 57
M.CS_BAG_MELT = 59
M.CS_BUILDING_CREATE = 202
M.CS_BUILDING_UPGRADE = 204
M.CS_BUILDING_UPGRADE_CANCEL = 206
M.CS_BUILDING_TRAIN = 208
M.CS_BUILDING_HEAL = 222
M.CS_BUILDING_HEAL_CANCEL = 224
M.CS_BUILDING_TRAIN_CANCEL = 210
M.CS_BUILDING_COLLECT = 214
M.CS_BUILDING_COLLECT_BOOST = 216
M.CS_BUILDING_DEMOLISH = 218
M.CS_BUILDING_DEMOLISH_CANCEL = 220
M.CS_BUILDING_OPEN_BUILDER2 = 226
M.CS_BUILDING_OPEN_FOREST = 228
M.CS_BUILDING_MOVE = 230
M.CS_BUILDING_TECHNOLOGY_RESEARCH = 232
M.CS_BUILDING_TECHNOLOGY_RESEARCH_CANCEL = 234
M.CS_TAKE_TAX = 250
M.CS_CDLIST_SPEED_UP = 302
M.CS_SAVE_TEAM_INFO = 354
M.CS_ARMY_FILL = 358
M.CS_SET_CASTLE_DEFENDER = 360
M.CS_DESTROY_TRAP = 362
M.CS_SKILL_RESET = 402
M.CS_SKILL_ADD_POINT = 404
M.CS_SKILL_ACTIVE_USE = 407
M.CS_MAIL_VIEW = 502
M.CS_MAIL_DELETE = 504
M.CS_MAIL_DRAW_ATTACHMENT = 506
M.CS_MAIL_ALLIANCE_SEND = 508
M.CS_MAIL_PRIVATE_SEND = 510
M.CS_MAIL_ALLIANCE_SHARE = 512
M.CS_MAIL_ALLIANCE_SHARE_VIEW = 513
M.CS_QUEST_DRAW = 551
M.CS_STORY_QUEST_DRAW = 556
M.CS_DAILY_TASK_REWARD_DRAW = 562
M.CS_DAILY_TASK_DRAW = 564
M.CS_VIP_TAKE_BOX = 621
M.CS_ACHIEVEMENT_DRAW = 651
M.CS_SET_MEDAL = 653
M.CS_MISC_SIGN = 701
M.CS_MISC_DRAW_ONLINE = 704
M.CS_MISC_TURNPLATE_DRAW = 710
M.CS_MISC_TURNPLATE_RECORDS_FETCH = 713
M.CS_MISC_DRAW_CODE = 720
M.CS_PLAYER_SET = 724
M.CS_ACTIVITY_LOGIN_NEXT_DAY_DRAW = 753
M.CS_ACTIVITY_ACCUMULATE_LOGIN_DRAW = 757
M.CS_ACTIVITY_LOGIN_EVERYDAY_DRAW = 761
M.CS_ACTIVITY_TIME_MEAL_DRAW = 765
M.CS_ACTIVITY_TARGET_DRAW = 769
M.CS_ACTIVITY_EXCHANGE_EXCHANGE = 773
M.CS_SEVEN_TARGET_DRAW = 851
M.CS_PUSH_SETTING_SET = 901
M.CS_PUSH_DEVICE_SYNC = 902
M.CS_TOWER_FIGHT = 954
M.CS_TOWER_RESET = 958
M.CS_TOWER_RANKING_GET = 960
M.CS_TOWER_MOPUP = 962
M.CS_MAP_VIEW = 1000
M.CS_MAP_SWITCH = 1005
M.CS_MAP_JOIN = 1010
M.CS_MAP_LEAVE = 1012
M.CS_MAP_TELEPORT = 1014
M.CS_MAP_MARCH = 1016
M.CS_MAP_GO_HOME = 1018
M.CS_MAP_RECALL = 1022
M.CS_MAP_SPEED_UP = 1024
M.CS_MAP_QUERY_PLAYERINFO = 1028
M.CS_MAP_WALL_REPAIR = 1030
M.CS_MAP_WALL_OUTFIRE = 1032
M.CS_MAP_DECLARE_CANCEL = 1034
M.CS_MAP_REPATRIATE = 1036
M.CS_MAP_ALLIANCE_INVITE_LIST = 1038
M.CS_MAP_ALLIANCE_INVITE_LIST_BY_NAME = 1040
M.CS_MAP_GET_ALLIES_EXTRA_INFOS = 1042
M.CS_MAP_ALLIANCE_UNIT_TELEPORT = 1044
M.CS_MAP_NEUTRAL_CASTLE_REPATRIATE = 1046
M.CS_MAP_GET_ALLIANCE_TERRITORY_LIST = 1048
M.CS_MAP_GET_ALLIANCE_TERRITORY_INFO = 1050
M.CS_MAP_KINGMAP_UNIT_POSITION = 1056
M.CS_MAP_KINGDOM_INFO = 1058
M.CS_MAP_NEUTRAL_CASTLE_CHANGE_NAME = 1060
M.CS_MAP_GET_NEAREST_UNIT = 1062
M.CS_MAP_GET_TROOP_EXTRA_INFO = 1064
M.CS_MAP_GET_MARCH_DISTANCE = 1068
M.CS_MAP_CHECK_UNIT_CAN_PLACE = 1070
M.CS_MAP_GET_MYSTERIOUS_CITY_INFO = 1072
M.CS_MAP_GET_GOBLIN_CAMP_INFO = 1074
M.CS_MAP_RECALL_ALLIANCE_TROOP = 1078
M.CS_BOOKMARK_ADD = 1081
M.CS_BOOKMARK_EDIT = 1083
M.CS_BOOKMARK_REMOVE = 1085
M.CS_MAP_SEARCH = 1087
M.CS_ALLIANCE_CHECK_NAME = 1210
M.CS_ALLIANCE_CREATE = 1212
M.CS_ALLIANCE_SEARCH = 1214
M.CS_ALLIANCE_SEARCH_BY_ID = 1216
M.CS_ALLIANCE_APPLY = 1217
M.CS_ALLIANCE_APPLY_ACCEPT = 1219
M.CS_ALLIANCE_INVITE = 1221
M.CS_ALLIANCE_INVITE_ACCEPT = 1223
M.CS_ALLIANCE_QUIT = 1225
M.CS_ALLIANCE_KICK_MEMBER = 1227
M.CS_ALLIANCE_BANNER_CHANGE = 1229
M.CS_ALLIANCE_ANNOUNCEMENT_CHANGE = 1231
M.CS_ALLIANCE_RANK_UP = 1233
M.CS_ALLIANCE_RANK_DOWN = 1235
M.CS_ALLIANCE_LEADER_TRANSFER = 1237
M.CS_ALLIANCE_LEADER_REPLACE = 1239
M.CS_ALLIANCE_SCIENCE_DONATE = 1241
M.CS_ALLIANCE_HELP_REQUEST = 1243
M.CS_ALLIANCE_HELP_ONE = 1245
M.CS_ALLIANCE_HELP_ALL = 1246
M.CS_ALLIANCE_USER_HIRE_HERO = 1252
M.CS_ALLIANCE_USER_RECALL_HERO = 1254
M.CS_ALLIANCE_USER_EMPLOY_HERO = 1256
M.CS_ALLIANCE_COLLECT_HERO_HIRE_COST = 1258
M.CS_ALLIANCE_BUFF_OPEN = 1263
M.CS_ALLIANCE_BUFF_CLOSED = 1265
M.CS_CHAT_SEND = 1400
M.CS_CHAT_BLOCK = 1403
M.CS_CHAT_UNBLOCK = 1405
M.CS_CHAT_GM = 1410
M.CS_RANGE_FETCH = 1450
M.CS_NOVICE_SET = 1502
M.CS_GET_SERVER_INFOS = 1552
M.CS_MONTH_CARD_DRAW = 1564
M.CS_HERO_DRAW = 2003
M.CS_HERO_STAR_UPGRADE = 2009
M.CS_HERO_SKILL_UPGRADE = 2011
M.CS_HERO_DEBRIS_TURN = 2021
M.CS_HERO_CHANGE_TECHNICAL_SKILL = 2023
M.CS_HERO_QUICK_RECOVER = 2025
M.CS_HERO_SOUL_UPGRADE = 2027
M.CS_HERO_UPGRADE = 2029
M.CS_SCENARIO_FIGHT = 2104
M.CS_SCENARIO_MOPUP = 2106
M.CS_SCENARIO_STAR_DRAW = 2108
M.CS_SCENARIO_ANSWER = 2110
M.CS_SCENARIO_RESET = 2111
M.CS_STORE_BUY = 2205
M.CS_STORE_REFRESH = 2207
M.CS_ARENA_CHANGE_OPPONENT = 2309
M.CS_ARENA_FIGHT = 2311
M.CS_ARENA_SET_DEFENSE = 2313
M.CS_ARENA_DREW_REWARD = 2315
M.CS_ARENA_BATTLE_RECORD = 2319
M.CS_ARENA_BATTLE_DETAILS = 2321
M.CS_ARENA_CLEAR_BATTLE_CD = 2323
M.CS_ARENA_RESET_BATTLE_COUNT = 2325
M.CS_ARENA_REVENGE = 2327
M.CS_BABEL_FIGHT = 2410
M.CS_BABEL_MOPUP = 2412
M.CS_BABEL_RESET = 2414
M.CS_BRONZE_SPARROW_TOWER_START = 2509
M.CS_BRONZE_SPARROW_TOWER_ANSWER = 2511
M.CS_BRONZE_SPARROW_TOWER_RECIVE_REWARD = 2513
M.CS_RIDING_ALONE_FIGHT = 2602
M.CS_RIDING_ALONE_RESET = 2604
M.CS_BATTLE_START = 3000
M.CS_BATTLE_OVER = 3002
M.CS_HERO_BATTLE_START = 3004
M.CS_HERO_BATTLE_OVER = 3006
M.CS_PALACE_WAR_SUCCESSIVE_KINGS = 1600
M.CS_PALACE_WAR_RECORDS = 1602
M.CS_KINGDOM_TITLE_LIST = 1604
M.CS_KING_CHOOSED_BY_ALLIANCE_LEADER = 1606
M.CS_KING_GIVE_TITLE = 1608
M.CS_KING_CANCEL_TITLE = 1610
M.CS_KING_GET_KINGDOM_SET_INFOS = 1612
M.CS_KING_CHANGE_SERVER_NAME = 1614
M.CS_KING_CHANGE_RESOURCE_NODE_RATE_ID = 1616
M.CS_KING_GET_GIFT_GIVEN_LIST = 1618
M.CS_KING_GIVE_GIFT = 1620
M.CS_KING_GET_PLAYER_LIST_WHEN_GIVE = 1622
M.CS_KING_GET_GIFT_LEFT_COUNT = 1627
M.CS_GET_REPORT = 1629
M.CS_GET_ALLIANCE_CITIES = 1631
M.CS_TRANSPORT = 1638
M.CS_STORY_ID_UPDATE = 1640


    
M.SC_TEMPLATE_UPDATE = 2
M.SC_LOGIN_RESPONSE = 6
M.SC_LOGOUT = 7
M.SC_PING = 8
M.SC_PING_RESULT = 10
M.SC_READY = 11
M.SC_SET_OR_CHECK_NICKNAME_RESPONSE = 21
M.SC_SET_LANG_TYPE_RESPONSE = 23
M.SC_STAMINA_BUY_RESPONSE = 25
M.SC_NOTICE_MESSAGE = 26
M.SC_USER_UPGRADE_DATA = 29
M.SC_USER_UPDATE = 30
M.SC_ATTR_PLUS_UPDATE = 31
M.SC_GET_RANDOM_NAME_RESPONSE = 33
M.SC_SET_NAME_RESPONSE = 35
M.SC_FILL_RESOURCE_RESPONSE = 37
M.SC_IS_SET_NAME = 38
M.SC_ENERGY_BUY_RESPONSE = 40
M.SC_HERO_ATTR_PLUS_UPDATE = 41
M.SC_BAG_UPDATE = 50
M.SC_BAG_USE_RESPONSE = 52
M.SC_BAG_BUY_RESPONSE = 54
M.SC_BAG_SELL_RESPONSE = 56
M.SC_BAG_SYNTHESIZE_RESPONSE = 58
M.SC_BAG_MELT_RESPONSE = 60
M.SC_BUILDING_UPDATE = 201
M.SC_BUILDING_CREATE_RESPONSE = 203
M.SC_BUILDING_UPGRADE_RESPONSE = 205
M.SC_BUILDING_UPGRADE_CANCEL_RESPONSE = 207
M.SC_BUILDING_TRAIN_RESPONSE = 209
M.SC_BUILDING_HEAL_RESPONSE = 223
M.SC_BUILDING_HEAL_CANCEL_RESPONSE = 225
M.SC_BUILDING_TRAIN_CANCEL_RESPONSE = 211
M.SC_BUILDING_QUEUES_UPDATE = 212
M.SC_BUILDING_COLLECT_RESPONSE = 215
M.SC_BUILDING_COLLECT_BOOST_RESPONSE = 217
M.SC_BUILDING_DEMOLISH_RESPONSE = 219
M.SC_BUILDING_DEMOLISH_CANCEL_RESPONSE = 221
M.SC_BUILDING_OPEN_BUILDER2_RESPONSE = 227
M.SC_BUILDING_OPEN_FOREST_RESPONSE = 229
M.SC_BUILDING_MOVE_RESPONSE = 231
M.SC_BUILDING_TECHNOLOGY_RESEARCH_RESPONSE = 233
M.SC_BUILDING_TECHNOLOGY_RESEARCH_CANCEL_RESPONSE = 235
M.SC_TECHNOLOGY_TREE_UPDATE = 236
M.SC_TAKE_TAX_RESPONSE = 251
M.SC_TAX_UPDATE = 252
M.SC_CDLIST_UPDATE = 301
M.SC_CDLIST_SPEED_UP_RESPONSE = 303
M.SC_ARMY_INFOS_UPDATE = 351
M.SC_TEAM_INFOS_UPDATE = 353
M.SC_SAVE_TEAM_INFO_RESPONSE = 355
M.SC_ARMY_FILL_RESPONSE = 359
M.SC_SET_CASTLE_DEFENDER_RESPONSE = 361
M.SC_DESTROY_TRAP_RESPONSE = 363
M.SC_SKILL_UPDATE = 401
M.SC_SKILL_RESET_RESPONSE = 403
M.SC_SKILL_ADD_POINT_RESPONSE = 405
M.SC_SKILL_ACTIVE_UPDATE = 406
M.SC_SKILL_ACTIVE_USE_RESPONSE = 408
M.SC_TECHNOLOGY_INFOS_UPDATE = 451
M.SC_MAIL_UPDATE = 501
M.SC_MAIL_VIEW_RESPONSE = 503
M.SC_MAIL_DELETE_RESPONSE = 505
M.SC_MAIL_DRAW_ATTACHMENT_RESPONSE = 507
M.SC_MAIL_ALLIANCE_SEND_RESPONSE = 509
M.SC_MAIL_PRIVATE_SEND_RESPONSE = 511
M.SC_MAIL_ALLIANCE_SHARE_VIEW_REPONSE = 514
M.SC_QUEST_UPDATE = 550
M.SC_QUEST_DRAW_RESPONSE = 552
M.SC_STORY_QUEST_UPDATE = 555
M.SC_STORY_QUEST_DRAW_RESPONSE = 557
M.SC_DAILY_TASK_UPDATE = 560
M.SC_DAILY_TASK_REWARD_UPDATE = 561
M.SC_DAILY_TASK_REWARD_DRAW_RESPONSE = 563
M.SC_DAILY_TASK_DRAW_RESPONSE = 565
M.SC_BUFF_LIST_UPDATE = 601
M.SC_BUFF_SKILL_UPDATE = 602
M.SC_VIP_UPDATE = 620
M.SC_VIP_TAKE_BOX_RESPONSE = 622
M.SC_ACHIEVEMENT_UPDATE = 650
M.SC_ACHIEVEMENT_DRAW_RESPONSE = 652
M.SC_SET_MEDAL_RESPONSE = 654
M.SC_MISC_SIGN_UPDATE = 700
M.SC_MISC_SIGN_RESPONSE = 702
M.SC_MISC_ONLINE_UPDATE = 703
M.SC_MISC_DRAW_ONLINE_RESPONSE = 705
M.SC_MISC_TURNPLATE_DATA_UPDATE = 709
M.SC_MISC_TURNPLATE_DRAW_RESPONSE = 711
M.SC_MISC_TURNPLATE_REWARD_UPDATE = 712
M.SC_MISC_TURNPLATE_RECORDS_FETCH_RESPONSE = 714
M.SC_MISC_DRAW_CODE_RESPONSE = 721
M.SC_PLAYER_SET_UPDATE = 723
M.SC_PLAYER_SET_RESPONSE = 725
M.SC_ACTIVITY_RECORD_UPDATE = 750
M.SC_ACTIVITY_LOGIN_NEXT_DAY_INFO_UPDATE = 752
M.SC_ACTIVITY_LOGIN_NEXT_DAY_DRAW_RESPONSE = 754
M.SC_ACTIVITY_ACCUMULATE_LOGIN_INFO_UPDATE = 756
M.SC_ACTIVITY_ACCUMULATE_LOGIN_DRAW_RESPONSE = 758
M.SC_ACTIVITY_LOGIN_EVERYDAY_INFO_UPDATE = 760
M.SC_ACTIVITY_LOGIN_EVERYDAY_DRAW_RESPONSE = 762
M.SC_ACTIVITY_TIME_MEAL_INFO_UPDATE = 764
M.SC_ACTIVITY_TIME_MEAL_DRAW_RESPONSE = 766
M.SC_ACTIVITY_TARGET_INFO_UPDATE = 768
M.SC_ACTIVITY_TARGET_DRAW_RESPONSE = 770
M.SC_ACTIVITY_EXCHANGE_INFO_UPDATE = 772
M.SC_ACTIVITY_EXCHANGE_EXCHANGE_RESPONSE = 774
M.SC_GMS_SEND_MARQUEE = 800
M.SC_GMS_NOTICE_UPDATE = 801
M.SC_SEVEN_TARGET_UPDATE = 850
M.SC_PUSH_SETTING_UPDATE = 900
M.SC_TOWER_PUPPET_INFO_UPDATE = 951
M.SC_TOWER_INFO_UPDATE = 953
M.SC_TOWER_FIGHT_RESPONSE = 955
M.SC_TOWER_BATTLE_RESULT = 957
M.SC_TOWER_RESET_RESPONSE = 959
M.SC_TOWER_RANKING_GET_RESPONSE = 961
M.SC_TOWER_MOPUP_RESPONSE = 963
M.SC_MAP_UNIT_UPDATE = 1001
M.SC_MAP_TROOP_UPDATE = 1002
M.SC_MAP_PERSONAL_INFO_UPDATE = 1003
M.SC_MAP_TELEPORT_RESPONSE = 1015
M.SC_MAP_MARCH_RESPONSE = 1017
M.SC_MAP_UNIT_BATTLE = 1027
M.SC_MAP_QUERY_PLAYERINFO_RESPONSE = 1029
M.SC_MAP_ALLIANCE_INVITE_LIST_RESPONSE = 1039
M.SC_MAP_GET_ALLIES_EXTRA_INFOS_RESPONSE = 1043
M.SC_MAP_ALLIANCE_UNIT_TELEPORT_RESPONSE = 1045
M.SC_MAP_GET_ALLIANCE_TERRITORY_LIST_RESPONSE = 1049
M.SC_MAP_GET_ALLIANCE_TERRITORY_INFO_RESPONSE = 1051
M.SC_MAP_MONSTER_SIEGE = 1053
M.SC_MAP_MONSTER_SIEGE_INFO_UPDATE = 1055
M.SC_MAP_KINGMAP_UNIT_POSITION_RESPONSE = 1057
M.SC_MAP_KINGDOM_INFO_RESPONSE = 1059
M.SC_MAP_NEUTRAL_CASLTE_CHANGE_NAME_RESPONSE = 1061
M.SC_MAP_GET_NEAREST_UNIT_RESPONSE = 1063
M.SC_MAP_GET_TROOP_EXTRA_INFO_RESPONSE = 1065
M.SC_MAP_NEUTRAL_CASTLE_PROPERTY_UPDATE = 1067
M.SC_MAP_GET_MARCH_DISTANCE_RESPONSE = 1069
M.SC_MAP_CHECK_UNIT_CAN_PLACE_RESPONSE = 1071
M.SC_MAP_GET_MYSTERIOUS_CITY_INFO_RESPONSE = 1073
M.SC_MAP_GET_GOBLIN_CAMP_INFO_RESPONSE = 1075
M.SC_PATROL_INFO_UPDATE = 1076
M.SC_MAP_SEND_REINFORCEMENTS = 1077
M.SC_MAP_RECALL_ALLIANCE_TROOP_RESPONSE = 1079
M.SC_BOOKMARK_UPDATE = 1080
M.SC_BOOKMARK_ADD_RESPONSE = 1082
M.SC_BOOKMARK_EDIT_RESPONSE = 1084
M.SC_BOOKMARK_REMOVE_RESPONSE = 1086
M.SC_MAP_SEARCH_RESPONSE = 1088
M.SC_ALLIANCE_INFO_UPDATE = 1200
M.SC_ALLIANCE_MEMBER_INFO_UPDATE = 1201
M.SC_ALLIANCE_APPLY_INFO_UPDATE = 1202
M.SC_ALLIANCE_INVITED_INFO_UPDATE = 1203
M.SC_ALLIANCE_SCIENCE_UPDATE = 1204
M.SC_ALLIANCE_SCORE_UPDATE = 1205
M.SC_ALLIANCE_PERSONAL_UPDATE = 1206
M.SC_ALLIANCE_DONATE_ITEMS_UPDATE = 1207
M.SC_ALLIANCE_HELP_UPDATE = 1208
M.SC_ALLIANCE_REQUEST_HELP = 1209
M.SC_ALLIANCE_CHECK_NAME_RESPONSE = 1211
M.SC_ALLIANCE_CREATE_RESPONSE = 1213
M.SC_ALLIANCE_SEARCH_RESPONSE = 1215
M.SC_ALLIANCE_APPLY_RESPONSE = 1218
M.SC_ALLIANCE_APPLY_ACCEPT_RESPONSE = 1220
M.SC_ALLIANCE_INVITE_RESPONSE = 1222
M.SC_ALLIANCE_INVITE_ACCEPT_RESPONSE = 1224
M.SC_ALLIANCE_QUIT_RESPONSE = 1226
M.SC_ALLIANCE_KICK_MEMBER_RESPONSE = 1228
M.SC_ALLIANCE_BANNER_CHANGE_RESPONSE = 1230
M.SC_ALLIANCE_ANNOUNCEMENT_CHANGE_RESPONSE = 1232
M.SC_ALLIANCE_RANK_UP_RESPONSE = 1234
M.SC_ALLIANCE_RANK_DOWN_RESPONSE = 1236
M.SC_ALLIANCE_LEADER_TRANSFER_RESPONSE = 1238
M.SC_ALLIANCE_LEADER_REPLACE_RESPONSE = 1240
M.SC_ALLIANCE_SCIENCE_DONATE_RESPONSE = 1242
M.SC_ALLIANCE_HELP_REQUEST_RESPONSE = 1244
M.SC_ALLIANCE_HIRE_HERO_UPDATE = 1250
M.SC_ALLIANCE_USER_EMPLOY_HERO_UPDATE = 1251
M.SC_ALLIANCE_USER_HIRE_HERO_RESPONSE = 1253
M.SC_ALLIANCE_USER_RECALL_HERO_RESPONSE = 1255
M.SC_ALLIANCE_USER_EMPLOY_HERO_RESPONSE = 1257
M.SC_ALLIANCE_COLLECT_HERO_HIRE_COST_RESPONSE = 1259
M.SC_ALLIANCE_RECORD_UPDATE = 1260
M.SC_ALLIANCE_HERO_LEASE_RECORD_UPDATE = 1261
M.SC_ALLIANCE_BUFF_UPDATE = 1262
M.SC_ALLIANCE_BUFF_OPEN_RESPONSE = 1264
M.SC_ALLIANCE_BUFF_CLOSED_RESPONSE = 1266
M.SC_ALLIANCE_HERO_HIRE_COLLECT_SILVER_UPDATE = 1270
M.SC_ALLIANCE_HERO_HIRE_TIME_SIVLER_UPDATE = 1271
M.SC_CHAT_RECV = 1401
M.SC_CHAT_BLOCK_UPDATE = 1402
M.SC_CHAT_BLOCK_RESPONSE = 1404
M.SC_CHAT_UNBLOCK_RESPONSE = 1406
M.SC_CHAT_GM_RESPONSE = 1411
M.SC_RANGE_FETCH_RESPONSE = 1451
M.SC_NOVICE_UPDATE = 1501
M.SC_TIME_SYNC = 1550
M.SC_AGENT_INIT_COMPLETE = 1551
M.SC_GET_SERVER_INFOS_RESPONSE = 1553
M.SC_CHARGE_OPTIONS = 1560
M.SC_CHARGE_BUY_UPDATE = 1561
M.SC_CHARGE_RESPONSE = 1562
M.SC_MONTH_CARD_UPDATE = 1563
M.SC_MONTH_CARD_DRAW_RESPONSE = 1565
M.SC_HERO_UPDATE = 2000
M.SC_HERO_SKILL_UPDATE = 2001
M.SC_HERO_DRAW_DATA_UPDATE = 2002
M.SC_HERO_DRAW_RESPONSE = 2004
M.SC_HERO_STAR_UPGRADE_RESPONSE = 2010
M.SC_HERO_SKILL_UPGRADE_RESPONSE = 2012
M.SC_HERO_DEBRIS_TURN_RESPONSE = 2022
M.SC_HERO_CHANGE_TECHNICAL_SKILL_RESPONSE = 2024
M.SC_HERO_QUICK_RECOVER_RESPONSE = 2026
M.SC_HERO_SOUL_UPGRADE_RESPONSE = 2028
M.SC_HERO_UPGRADE_RESPONSE = 2030
M.SC_HERO_SOUL_UPDATE = 2031
M.SC_SCENARIO_CHAPTER_UPDATE = 2101
M.SC_SCENARIO_STAR_REWARD_UPDATE = 2102
M.SC_SCENARIO_BATTLE_RESULT = 2103
M.SC_SCENARIO_FIGHT_RESPONSE = 2105
M.SC_SCENARIO_MOPUP_RESPONSE = 2107
M.SC_SCENARIO_STAR_DRAW_RESPONSE = 2109
M.SC_SCENARIO_RESET_RESPONSE = 2112
M.SC_STORE_INFO_UPDATE = 2201
M.SC_STORE_BUY_RESPONSE = 2206
M.SC_ARENA_USER_DATA_UPDATE = 2301
M.SC_ARENA_DEFENSE_UPDATE = 2302
M.SC_ARENA_BATTLE_RESULT = 2303
M.SC_ARENA_CHANGE_OPPONENT_RESPONSE = 2310
M.SC_ARENA_FIGHT_RESPONSE = 2312
M.SC_ARENA_SET_DEFENSE_RESPONSE = 2314
M.SC_ARENA_DREW_REWARD_RESPONSE = 2316
M.SC_ARENA_BATTLE_RECORD_RESPONSE = 2320
M.SC_ARENA_BATTLE_DETAILS_RESPONSE = 2322
M.SC_ARENA_CLEAR_BATTLE_CD_RESPONSE = 2324
M.SC_ARENA_RESET_BATTLE_COUNT_RESPONSE = 2326
M.SC_ARENA_REVENGE_RESPONSE = 2328
M.SC_BABEL_DATA_UPDATE = 2401
M.SC_BABEL_BATTLE_RESULT = 2402
M.SC_BABEL_FIGHT_RESPONSE = 2411
M.SC_BABEL_MOPUP_RESPONSE = 2413
M.SC_BABEL_RESET_RESPONSE = 2415
M.SC_BRONZE_SPARROW_TOWER_UPDATE = 2501
M.SC_BRONZE_SPARROW_TOWER_START_RESPONSE = 2510
M.SC_BRONZE_SPARROW_TOWER_ANSWER_RESPONSE = 2512
M.SC_BRONZE_SPARROW_TOWER_RECIVE_REWARD_RESPONSE = 2514
M.SC_BRONZE_SPARROW_TOWER_RESET = 2515
M.SC_RIDING_ALONE_UPDATE = 2601
M.SC_RIDING_ALONE_FIGHT_RESPONSE = 2603
M.SC_RIDING_ALONE_RESET_RESPONSE = 2605
M.SC_BATTLE_START_RT = 3001
M.SC_BATTLE_OVER_RT = 3003
M.SC_HERO_BATTLE_START_RT = 3005
M.SC_HERO_BATTLE_OVER_RT = 3007
M.SC_PALACE_WAR_SUCCESSIVE_KINGS_RESPONSE = 1601
M.SC_PALACE_WAR_RECORDS_RESPONSE = 1603
M.SC_KINGDOM_TITLE_LIST_RESPONSE = 1605
M.SC_KING_CHOOSED_BY_ALLIANCE_LEADER_RESPONSE = 1607
M.SC_KING_GIVE_TITLE_RESPONSE = 1609
M.SC_KING_CANCEL_TITLE_RESPONSE = 1611
M.SC_KING_GET_KINGDOM_SET_INFOS_RESPONSE = 1613
M.SC_KING_CHANGE_SERVER_NAME_RESOPNSE = 1615
M.SC_KING_CHANGE_RESOURCE_NODE_RATE_ID_RESPONSE = 1617
M.SC_KING_GET_GIFT_GIVEN_LIST_RESPONSE = 1619
M.SC_KING_GIVE_GIFT_RESPONSE = 1621
M.SC_KING_GET_PLAYER_LIST_WHEN_GIVE_RESPONSE = 1623
M.SC_TITLE_UPDATE = 1625
M.SC_KING_GET_GIFT_LEFT_COUNT_RESPONSE = 1628
M.SC_GET_REPORT_RESPONSE = 1630
M.SC_GET_ALLIANCE_CITIES_RESPONSE = 1632
M.SC_SCENARIO_TEAM_INFOS_UPDATE = 1633
M.SC_CAN_MARCH_MONSTER_LEVEL_UPDATE = 1635
M.SC_MARKET_TRANSPORT_RECORD_UPDATE = 1637
M.SC_TRANSPORT_RESPONSE = 1639
M.SC_STORY_ID_UPDATE_RESPONSE = 1641



    
M.CS_MAP = {}
M.CS_MAP[1] = 1
M.CS_MAP[5] = 5
M.CS_MAP[9] = 9
M.CS_MAP[20] = 20
M.CS_MAP[22] = 22
M.CS_MAP[24] = 24
M.CS_MAP[32] = 32
M.CS_MAP[34] = 34
M.CS_MAP[36] = 36
M.CS_MAP[39] = 39
M.CS_MAP[51] = 51
M.CS_MAP[53] = 53
M.CS_MAP[55] = 55
M.CS_MAP[57] = 57
M.CS_MAP[59] = 59
M.CS_MAP[202] = 202
M.CS_MAP[204] = 204
M.CS_MAP[206] = 206
M.CS_MAP[208] = 208
M.CS_MAP[222] = 222
M.CS_MAP[224] = 224
M.CS_MAP[210] = 210
M.CS_MAP[214] = 214
M.CS_MAP[216] = 216
M.CS_MAP[218] = 218
M.CS_MAP[220] = 220
M.CS_MAP[226] = 226
M.CS_MAP[228] = 228
M.CS_MAP[230] = 230
M.CS_MAP[232] = 232
M.CS_MAP[234] = 234
M.CS_MAP[250] = 250
M.CS_MAP[302] = 302
M.CS_MAP[354] = 354
M.CS_MAP[358] = 358
M.CS_MAP[360] = 360
M.CS_MAP[362] = 362
M.CS_MAP[402] = 402
M.CS_MAP[404] = 404
M.CS_MAP[407] = 407
M.CS_MAP[502] = 502
M.CS_MAP[504] = 504
M.CS_MAP[506] = 506
M.CS_MAP[508] = 508
M.CS_MAP[510] = 510
M.CS_MAP[512] = 512
M.CS_MAP[513] = 513
M.CS_MAP[551] = 551
M.CS_MAP[556] = 556
M.CS_MAP[562] = 562
M.CS_MAP[564] = 564
M.CS_MAP[621] = 621
M.CS_MAP[651] = 651
M.CS_MAP[653] = 653
M.CS_MAP[701] = 701
M.CS_MAP[704] = 704
M.CS_MAP[710] = 710
M.CS_MAP[713] = 713
M.CS_MAP[720] = 720
M.CS_MAP[724] = 724
M.CS_MAP[753] = 753
M.CS_MAP[757] = 757
M.CS_MAP[761] = 761
M.CS_MAP[765] = 765
M.CS_MAP[769] = 769
M.CS_MAP[773] = 773
M.CS_MAP[851] = 851
M.CS_MAP[901] = 901
M.CS_MAP[902] = 902
M.CS_MAP[954] = 954
M.CS_MAP[958] = 958
M.CS_MAP[960] = 960
M.CS_MAP[962] = 962
M.CS_MAP[1000] = 1000
M.CS_MAP[1005] = 1005
M.CS_MAP[1010] = 1010
M.CS_MAP[1012] = 1012
M.CS_MAP[1014] = 1014
M.CS_MAP[1016] = 1016
M.CS_MAP[1018] = 1018
M.CS_MAP[1022] = 1022
M.CS_MAP[1024] = 1024
M.CS_MAP[1028] = 1028
M.CS_MAP[1030] = 1030
M.CS_MAP[1032] = 1032
M.CS_MAP[1034] = 1034
M.CS_MAP[1036] = 1036
M.CS_MAP[1038] = 1038
M.CS_MAP[1040] = 1040
M.CS_MAP[1042] = 1042
M.CS_MAP[1044] = 1044
M.CS_MAP[1046] = 1046
M.CS_MAP[1048] = 1048
M.CS_MAP[1050] = 1050
M.CS_MAP[1056] = 1056
M.CS_MAP[1058] = 1058
M.CS_MAP[1060] = 1060
M.CS_MAP[1062] = 1062
M.CS_MAP[1064] = 1064
M.CS_MAP[1068] = 1068
M.CS_MAP[1070] = 1070
M.CS_MAP[1072] = 1072
M.CS_MAP[1074] = 1074
M.CS_MAP[1078] = 1078
M.CS_MAP[1081] = 1081
M.CS_MAP[1083] = 1083
M.CS_MAP[1085] = 1085
M.CS_MAP[1087] = 1087
M.CS_MAP[1210] = 1210
M.CS_MAP[1212] = 1212
M.CS_MAP[1214] = 1214
M.CS_MAP[1216] = 1216
M.CS_MAP[1217] = 1217
M.CS_MAP[1219] = 1219
M.CS_MAP[1221] = 1221
M.CS_MAP[1223] = 1223
M.CS_MAP[1225] = 1225
M.CS_MAP[1227] = 1227
M.CS_MAP[1229] = 1229
M.CS_MAP[1231] = 1231
M.CS_MAP[1233] = 1233
M.CS_MAP[1235] = 1235
M.CS_MAP[1237] = 1237
M.CS_MAP[1239] = 1239
M.CS_MAP[1241] = 1241
M.CS_MAP[1243] = 1243
M.CS_MAP[1245] = 1245
M.CS_MAP[1246] = 1246
M.CS_MAP[1252] = 1252
M.CS_MAP[1254] = 1254
M.CS_MAP[1256] = 1256
M.CS_MAP[1258] = 1258
M.CS_MAP[1263] = 1263
M.CS_MAP[1265] = 1265
M.CS_MAP[1400] = 1400
M.CS_MAP[1403] = 1403
M.CS_MAP[1405] = 1405
M.CS_MAP[1410] = 1410
M.CS_MAP[1450] = 1450
M.CS_MAP[1502] = 1502
M.CS_MAP[1552] = 1552
M.CS_MAP[1564] = 1564
M.CS_MAP[2003] = 2003
M.CS_MAP[2009] = 2009
M.CS_MAP[2011] = 2011
M.CS_MAP[2021] = 2021
M.CS_MAP[2023] = 2023
M.CS_MAP[2025] = 2025
M.CS_MAP[2027] = 2027
M.CS_MAP[2029] = 2029
M.CS_MAP[2104] = 2104
M.CS_MAP[2106] = 2106
M.CS_MAP[2108] = 2108
M.CS_MAP[2110] = 2110
M.CS_MAP[2111] = 2111
M.CS_MAP[2205] = 2205
M.CS_MAP[2207] = 2207
M.CS_MAP[2309] = 2309
M.CS_MAP[2311] = 2311
M.CS_MAP[2313] = 2313
M.CS_MAP[2315] = 2315
M.CS_MAP[2319] = 2319
M.CS_MAP[2321] = 2321
M.CS_MAP[2323] = 2323
M.CS_MAP[2325] = 2325
M.CS_MAP[2327] = 2327
M.CS_MAP[2410] = 2410
M.CS_MAP[2412] = 2412
M.CS_MAP[2414] = 2414
M.CS_MAP[2509] = 2509
M.CS_MAP[2511] = 2511
M.CS_MAP[2513] = 2513
M.CS_MAP[2602] = 2602
M.CS_MAP[2604] = 2604
M.CS_MAP[3000] = 3000
M.CS_MAP[3002] = 3002
M.CS_MAP[3004] = 3004
M.CS_MAP[3006] = 3006
M.CS_MAP[1600] = 1600
M.CS_MAP[1602] = 1602
M.CS_MAP[1604] = 1604
M.CS_MAP[1606] = 1606
M.CS_MAP[1608] = 1608
M.CS_MAP[1610] = 1610
M.CS_MAP[1612] = 1612
M.CS_MAP[1614] = 1614
M.CS_MAP[1616] = 1616
M.CS_MAP[1618] = 1618
M.CS_MAP[1620] = 1620
M.CS_MAP[1622] = 1622
M.CS_MAP[1627] = 1627
M.CS_MAP[1629] = 1629
M.CS_MAP[1631] = 1631
M.CS_MAP[1638] = 1638
M.CS_MAP[1640] = 1640


return M

