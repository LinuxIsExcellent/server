#ifndef ENGINE_METADATA_H
#define ENGINE_METADATA_H

namespace engine
{
    enum class SpellNodeType
    {
        NORMAL = 0,
        EXCAPE = 1,
        FAKE_RETREAT = 2,
    };

    enum class BuffType
    {
        PASSIVE_POWER = 1,
        PASSIVE_DEFENSE = 2,
        PASSIVE_WISDOM = 3,
        PASSIVE_SKILL = 4,
        PASSIVE_AGILE = 5,
        PASSIVE_LUCKY = 6,
        PASSIVE_LIFE = 7,                
        PASSIVE_ALL = 8,
        PASSIVE_HIT = 9,
        PASSIVE_AVOID = 10,
        PASSIVE_CRIT_HIT = 11,  
        PASSIVE_CRIT_AVOID = 12,
        PASSIVE_ATTACK_SPEED = 13,

        NIRVARNA = 14, //必杀技
        COMPLEX = 15, //合体技 

        SCARE = 16, 
        TUMBLE = 17,
        WOUND = 18,
        PARALYSIS = 19,
        WEAK = 20, 
        STUN = 21,
        RADNOM = 22,
        
        ADD_DAMAGE = 23,
        POWER_THAN_DAMAGE = 24,
        DEFENSE_THAN_DAMAGE = 25,
        SKILL_THAN_DAMAGE = 26,
        AGILE_THAN_DAMAGE = 27,
        WISDOM_THAN_DAMAGE = 28,
        LUCKY_THAN_DAMAGE = 29,
        LIFE_THAN_DAMAGE = 30,
        POWER_GAP_DAMAGE = 31,
        DEFENSE_GAP_DAMAGE = 32,
        SKILL_GAP_DAMAGE = 33,
        AGILE_GAP_DAMAGE = 34,
        WISDOM_GAP_DAMAGE = 35,
        LUCKY_GAP_DAMAGE = 36,
        LIFE_GAP_DAMAGE = 37,
        POWER_SELF_DAMAGE = 38,
        DEFENSE_SELF_DAMAGE = 39,
        SKILL_SELF_DAMAGE = 40,
        AGILE_SELF_DAMAGE = 41,
        WISDOM_SELF_DAMAGE = 42,
        LUCKY_SELF_DAMAGE = 43,
        LIFE_SELF_DAMAGE = 44,

        ADD_RAGE = 45, 
        MINUS_RAGE = 46,  
        RECOVER_WISDOM_FIGHT = 47, 
        ADD_WISDOM_PHYSICAL_DAMAGE = 48, 
        ADD_WISDOM_MAGIC_DAMAGE = 49, 
        MINUS_PHYSICAL_DAMAGE = 50, 
        MINUS_MAGIC_DAMAGE = 51, 
        CLEAN_NEGATIVE_STATE = 52,
        CLEAN_POSITIVE_STATE = 53,
        ABSORB_DAMAGE_HP = 54,
        DEDUCT_HP = 55,
    };

    enum class BuffTarget
    {
        OTHER_SINGLE = 1, 
        OTHER_DOUBLE = 2,
        OTHER_THREE = 3,
        ANY_SINGLE = 4,
        ANY_DOUBLE = 5,
        ANY_THREE = 6,
        OTHER_BACK_ONE = 7,
        OTHER_BACK_COLUME = 8,
        ME = 9,
        SELF_EXCEPT_ME = 10,
        SELF_ALL = 11,
        OTHER_ALL = 12, 
        CROSS_DOUBLE = 13, //贯通2体 
        CROSS_COLUME = 14, //贯通2体纵一列
        CROSS = 15, //贯穿十字
        CROSS_DOUBLE_COLUME = 16, //贯穿双纵一列
        ROW = 17,
        COLUME = 18,
        DOUBLE_COLUME = 19, //双纵一列
        Z_SAMPLE = 20, //折行
        HALF_MOON_SAMPLE = 21, //半月
        CROSS_SAMPLE, //十字
        X_SAMPLE, //X行
        V_SAMPLE, //锥形 V行
        W_SAMPLE, //波形
        ROW_Z_SAMPLE, //横Z
    };

    enum class BuffValueType
    {
        NONE = 0, // 无数值类型
        PERCENT = 1,            // 百分比类型
        VALUE = 2,              // 数值类型
    };

    enum class TeamType
    {
        NONE = 0,
        ATTACKER = 1,
        DEFENDER = 2,
    };

    // 兵种类型
    enum class ArmyType
    {
        NONE = 0,
        INFANTRY = 1,        // 步兵
        RIDER = 2,        // 骑兵
        ARCHER = 3,        // 弓兵
        MACHINE = 4,        // 机械兵
    };

    enum class PassiveType
    {
        NONE = 0,
        POWER = 1,
        DEFENSE = 2, 
        WISDOM = 3,
        SKILL = 4,
        AGILE = 5,
        LUCKY = 6,
        LIFE = 7,
        ALL = 8,  //全能力
        HIT = 9,
        AVOID = 10,
        CRIT_HIT = 11,
        CRIT_AVOID = 12,
        ATTACK_SPEED = 13,
    };

    //目标状态
    enum class TargetStatus 
    {
        NONE = 0, 
        SCARE = 1, //恐慌
        TUMBLE = 2, //混乱
        WOUND = 3, //负伤
        PARALYSIS = 4, //麻痹
        WEAK = 5, //弱气
        STUN = 6, //眩晕
        ADD_PHYSICAL_DAMAGE = 7,
        ADD_MAGIC_DAMAGE = 8, 
        MINUS_PHYSICAL_DAMAGE = 9,
        MINUS_MAGIC_DAMAGE = 10,
    };

    enum class DamageType 
    {
        NONE = 0,
        PHYSICAL_HURT = 1, //物理攻击
        MAGIC_HURT = 2, //法术攻击
        MIX_HURT = 3, //混合攻击
    };

    //效果条件
    enum class EffectCondType 
    {
        NONE = 0,
        AFTER_HURT = 1,
        AFTER_RELEASE = 2,
        SELF_INFANTRY = 3,
        SELF_RIDER = 4,
        SELF_ARCHER = 5,
        SELF_MACHINE = 6,
        TARGET_INFANTRY = 7,
        TARGET_RIDER = 8,
        TARGET_ARCHER = 9,
        TARGET_MACHINE = 10,
    };

    enum class EffectRangeType 
    {
        NONE = 0,
        TARGET_SINGLE = 1,
        TARGET_ALL = 2,
        ME = 3,
        SELF_RANDOM_SINGLE = 4,
        ME_AND_RANDOM_SINGLE = 5,
        SELF_EXCEPT_ME = 6,
        SELF_ALL = 7,
        RELATE_ASSIT = 8,
    };

    enum class EffectType 
    {
        NONE = 0,
        SCARE = 1,
        TUMBLE = 2,
        WOUND = 3,
        PARALYSIS = 4,
        WEAK = 5, 
        STUN = 6,
        RADNOM = 7,
        ADD_DAMAGE = 8,
        POWER_THAN_DAMAGE = 9,
        DEFENSE_THAN_DAMAGE = 10,
        SKILL_THAN_DAMAGE = 11,
        AGILE_THAN_DAMAGE = 12,
        WISDOM_THAN_DAMAGE = 13,
        LUCKY_THAN_DAMAGE = 14,
        LIFE_THAN_DAMAGE = 15,
        POWER_GAP_DAMAGE = 16,
        DEFENSE_GAP_DAMAGE = 17,
        SKILL_GAP_DAMAGE = 18,
        AGILE_GAP_DAMAGE = 19,
        WISDOM_GAP_DAMAGE = 20,
        LUCKY_GAP_DAMAGE = 21,
        LIFE_GAP_DAMAGE = 22,
        POWER_SELF_DAMAGE = 23,
        DEFENSE_SELF_DAMAGE = 24,
        SKILL_SELF_DAMAGE = 25,
        AGILE_SELF_DAMAGE = 26,
        WISDOM_SELF_DAMAGE = 27,
        LUCKY_SELF_DAMAGE = 28,
        LIFE_SELF_DAMAGE = 29,
        ADD_RAGE = 30,
        MINUS_RAGE = 31,
        RECOVER_WISDOM_FIGHT = 32,
        ADD_WISDOM_PHYSICAL_DAMAGE = 33,
        ADD_WISDOM_MAGIC_DAMAGE = 34,
        MINUS_PHYSICAL_DAMAGE = 35,
        MINUS_MAGIC_DAMAGE = 36,
        CLEAN_NEGATIVE_STATE = 37,
        CLEAN_POSITIVE_STATE = 38,
        ABSORB_DAMAGE_HP = 39,
        DEDUCT_HP = 40,
    };

    enum class TriggerBuffState {
        NONE = 0,
        BIG_ROUND_START = 1,
        SMALL_ROUND_START = 2,
        SMALL_ROUND_END = 3, 
        BIG_ROUND_END = 4,
    };


}

#endif // ENGINE_METADATA_H
