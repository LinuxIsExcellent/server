#include "bufflist.h"
#include "buff.h"
#include "hero.h"
#include "../tpl/t_buff.h"
#include <base/logger.h>

namespace engine
{
    namespace battle
    {
        namespace buff
        {
            using namespace std;
            using namespace engine::tpl;
            //武力增减益buff
            class BuffPower : public BuffBase
            {
            public:
                BuffPower(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {}
                virtual ~BuffPower() {}

                virtual void OnStart() {

                    //m_value = m_target.hit() * buffValue() / 100;

                    m_value = buffValue() + coefficient()*level();

                    if (m_value != 0) {
                        //std::cout<< "Type: " << tBuff.type << " value: " << m_value << std::endl;
                        LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                        m_target.AddPower(m_value);
                    }
                }

                virtual void OnEnd() {
                }
            };

            // 统帅增减益buff
            class BuffDefense : public BuffBase
            {
            public:
                BuffDefense(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {}
                virtual ~BuffDefense() {}

                virtual void OnStart() {

                    //m_value = m_target.hit() * buffValue() / 100;

                    m_value = buffValue() + coefficient()*level();

                    if (m_value != 0) {
                        LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                        m_target.AddDefense(m_value);
                    }
                }

                virtual void OnEnd() {
                }
            };

            class BuffWisdom : public BuffBase
            {
            public:
                BuffWisdom(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {}
                virtual ~BuffWisdom() {}

                virtual void OnStart() {

                    //m_value = m_target.hit() * buffValue() / 100;

                    m_value = buffValue() + coefficient()*level();

                    if (m_value != 0) {
                        LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                        m_target.AddWisdom(m_value);
                    }
                }

                virtual void OnEnd() {
                }
            };

            class BuffSkill : public BuffBase
            {
            public:
                BuffSkill(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {}
                virtual ~BuffSkill() {}

                virtual void OnStart() {

                    //m_value = m_target.hit() * buffValue() / 100;
                    m_value = buffValue() + coefficient()*level();

                    if (m_value != 0) {
                        LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                        m_target.AddSkill(m_value);
                    }
                }

                virtual void OnEnd() {
                }
            };

            //攻城值
            class BuffAgile : public BuffBase
            {
            public:
                BuffAgile(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {}
                virtual ~BuffAgile() {}

                virtual void OnStart() {

                    //m_value = m_target.hit() * buffValue() / 100;

                    m_value = buffValue() + coefficient()*level();

                    if (m_value != 0) {
                        LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                        m_target.AddAgile(m_value);
                    }
                }

                virtual void OnEnd() {
                }
            };

            class BuffLucky : public BuffBase
            {
            public:
                BuffLucky(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {}
                virtual ~BuffLucky() {}

                virtual void OnStart() {

                    //m_value = m_target.hit() * buffValue() / 100;

                    m_value = buffValue() + coefficient()*level();

                    if (m_value != 0) {
                        LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                        m_target.AddLucky(m_value);
                    }
                }

                virtual void OnEnd() {
                }
            };

            //攻城值
            class BuffLife : public BuffBase
            {
            public:
                BuffLife(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {}
                virtual ~BuffLife() {}

                virtual void OnStart() {

                    //m_value = m_target.hit() * buffValue() / 100;

                    m_value = buffValue() + coefficient()*level();

                    if (m_value != 0) {
                        LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                        m_target.AddLife(m_value);
                    }
                }

                virtual void OnEnd() {
                }
            };

            class BuffAll : public BuffBase
            {
            public:
                BuffAll(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {}
                virtual ~BuffAll() {}

                virtual void OnStart() {

                    //m_value = m_target.hit() * buffValue() / 100;

                    m_value = buffValue() + coefficient()*level();

                    if (m_value != 0) {
                        LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                        m_target.AddAll(m_value);
                    }
                }

                virtual void OnEnd() {
                }
            };


            // 自身命中增减益buff
            class BuffHit : public BuffBase
            {
            public:
                BuffHit(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {}
                virtual ~BuffHit() {}

                virtual void OnStart() {

                    //m_value = m_target.hit() * buffValue() / 100;

                    m_value = m_target.hitBase() * (buffValue() + coefficient()*level()) / 100;

                    if (m_value != 0) {
                        LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                        m_target.AddHit(m_value);
                    }
                }

                virtual void OnEnd() {
                }
            };

            class BuffAvoid : public BuffBase
            {
            public:
                BuffAvoid(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {}
                virtual ~BuffAvoid() {}

                virtual void OnStart() {

                    //m_value = m_target.hit() * buffValue() / 100;

                    m_value = m_target.avoidBase() * (buffValue() + coefficient() * level()) / 100;

                    if (m_value != 0) {
                        LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                        m_target.AddAvoid(m_value);
                    }
                }

                virtual void OnEnd() {
                }
            };

            class BuffCritHit : public BuffBase
            {
            public:
                BuffCritHit(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {}
                virtual ~BuffCritHit() {}

                virtual void OnStart() {

                    //m_value = m_target.hit() * buffValue() / 100;

                    m_value = m_target.critHitBase() * (buffValue() + coefficient() * level()) / 100;

                    if (m_value != 0) {
                        LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                        m_target.AddCritHit(m_value);
                    }
                }

                virtual void OnEnd() {
                }
            };

            class BuffCritAvoid : public BuffBase
            {
            public:
                BuffCritAvoid(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {}
                virtual ~BuffCritAvoid() {}

                virtual void OnStart() {

                    //m_value = m_target.hit() * buffValue() / 100;

                    m_value = m_target.critAvoidBase() * (buffValue() + coefficient() * level()) / 100;

                    if (m_value != 0) {
                        LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                        m_target.AddCritAvoid(m_value);
                    }
                }

                virtual void OnEnd() {
                }
            };

            class BuffAttackSpeed : public BuffBase
            {
            public:
                BuffAttackSpeed(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {}
                virtual ~BuffAttackSpeed() {}

                virtual void OnStart() {

                    //m_value = m_target.hit() * buffValue() / 100;

                    m_value = (buffValue() + coefficient() * level()) / 100;

                    if (m_value != 0) {
                        LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                        m_target.AddAttackSpeed(m_value);
                    }
                }

                virtual void OnEnd() {
                }
            };

            //必杀伤害
            class BuffNirvarna : public BuffBase
            {
            public:
                BuffNirvarna(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;

                    LOG_DEBUG("Check hurt : %0.3f, m_value : %0.3f", hurt,
                        m_value);
                }

                virtual ~BuffNirvarna() {}

                virtual void OnStart() {
                    LOG_DEBUG("nirvarna teamType : %d  position: %d  hurt : %d ", m_target.teamType(), m_target.Position(),
                        (int)m_value);
                    m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                    LOG_DEBUG("nirvarna2 ---- :%d ", m_target.armyHp());

                }
            };

            //合体伤害
            class BuffComplex : public BuffBase
            {
            public:
                BuffComplex(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;

                    LOG_DEBUG("Check hurt : %0.3f, m_value : %0.3f", hurt,
                        m_value);
                }

                virtual ~BuffComplex() {}

                virtual void OnStart() {
                    LOG_DEBUG("complex teamType : %d  position: %d  hurt : %d ", m_target.teamType(), m_target.Position(),
                        (int)m_value);
                    m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                    LOG_DEBUG("complex ---- :%d ", m_target.armyHp());

                }
            };

            //恐惧
            class BuffScare : public BuffBase
            {
            public:
                BuffScare(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {
                }

                virtual ~BuffScare() {}

                virtual void OnStart() {
                    m_target.SetEffectStatus(TargetStatus::SCARE);
                    m_effectValue = 10;
                }

                virtual void OnTurn() {
                    if (!m_target.IsStatus(TargetStatus::SCARE)) { return; }

                    //每回合扣除10%的血量
                    int minus_hp = m_target.armyHp() * 10 / 100;
                    LOG_DEBUG("----- SCARE ---- Status ---!!! now-hp : %d  minus: %d", m_target.armyHp(), minus_hp);
                    m_target.OnBuffDirectDamage(m_caster, minus_hp);  //可以加上状态判断
                }

                virtual void OnEnd() {
                    OnTurn();
                    m_target.ClearEffectStatus(TargetStatus::SCARE);
                }
            };

            //混乱
            class BuffTumble : public BuffBase
            {
            public:
                BuffTumble(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {
                }

                virtual ~BuffTumble() {}

                virtual void OnStart() {
                    m_target.SetEffectStatus(TargetStatus::TUMBLE);
                }

                virtual void OnTurn() {
                    if (!m_target.IsStatus(TargetStatus::TUMBLE)) { return;}
                    //混乱打自己人

                }

                virtual void OnEnd() {
                    OnTurn();
                    //Todo: 清除状态要考虑 状态重置的情况
                    m_target.ClearEffectStatus(TargetStatus::TUMBLE);
                }
            };

            //受伤
            class BuffWound : public BuffBase
            {
            public:
                BuffWound(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {
                }

                virtual ~BuffWound() {}

                virtual void OnStart() {
                    m_target.SetEffectStatus(TargetStatus::WOUND);
                }

                virtual void OnTurn() {
                    if (!m_target.IsStatus(TargetStatus::WOUND)) { return; }
                }

                virtual void OnEnd() {
                    OnTurn();
                    //Todo: 清除状态要考虑 状态重置的情况
                    m_target.ClearEffectStatus(TargetStatus::WOUND);
                }
            };

            //麻痹
            class BuffParalysis: public BuffBase
            {
            public:
               BuffParalysis(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {
                }

                virtual ~BuffParalysis() {}

                virtual void OnStart() {
                    m_target.SetEffectStatus(TargetStatus::PARALYSIS);
                }

                virtual void OnTurn() {
                    if (!m_target.IsStatus(TargetStatus::PARALYSIS)) { return; }
                }

                virtual void OnEnd() {
                    OnTurn();
                    //Todo: 清除状态要考虑 状态重置的情况
                    m_target.ClearEffectStatus(TargetStatus::PARALYSIS);
                }
            };

            //弱气
            class BuffWeak: public BuffBase
            {
            public:
               BuffWeak(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {
                }

                virtual ~BuffWeak() {}

                virtual void OnStart() {
                    m_target.SetEffectStatus(TargetStatus::WEAK);
                }

                virtual void OnTurn() {
                    if (!m_target.IsStatus(TargetStatus::WEAK)) { return; }
                }

                virtual void OnEnd() {
                    OnTurn();
                    //Todo: 清除状态要考虑 状态重置的情况
                    m_target.ClearEffectStatus(TargetStatus::WEAK);
                }
            };

            //眩晕
            class BuffStun: public BuffBase
            {
            public:
               BuffStun(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {
                }

                virtual ~BuffStun() {}

                virtual void OnStart() {
                    m_target.SetEffectStatus(TargetStatus::STUN);
                }

                virtual void OnTurn() {
                    if (!m_target.IsStatus(TargetStatus::STUN)) { return; }
                }

                virtual void OnEnd() {
                    OnTurn();
                    //Todo: 清除状态要考虑 状态重置的情况
                    m_target.ClearEffectStatus(TargetStatus::STUN);
                }
            };

            class BuffAddDamage: public BuffBase
            {
            public:
               BuffAddDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffAddDamage() {}

                virtual void OnStart() {
                    m_value = m_value * (buffValue() + coefficient() * level()) /100;

                    if (m_value != 0) {
                        LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                        m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                        m_effectValue = (int)(-m_value);

                    }
                }
            };

            class BuffPowerThanDamage: public BuffBase
            {
            public:
               BuffPowerThanDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffPowerThanDamage() {}

                virtual void OnStart() {
                    if (m_caster.power() > m_target.power()) {
                         m_value = m_value * (buffValue() + coefficient() * level()) /100;

                        if (m_value != 0) {
                            LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                            m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                            m_effectValue = (int)(-m_value);
                        }
                    }
                }
            };

            class BuffDefenseThanDamage: public BuffBase
            {
            public:
               BuffDefenseThanDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffDefenseThanDamage() {}

                virtual void OnStart() {
                    if (m_caster.defense() > m_target.defense()) {
                         m_value = m_value * (buffValue() + coefficient() * level()) /100;

                        if (m_value != 0) {
                            LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                            m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                            m_effectValue = (int)(-m_value);
                        }
                    }
                }
            };

            class BuffSkillThanDamage: public BuffBase
            {
            public:
               BuffSkillThanDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffSkillThanDamage() {}

                virtual void OnStart() {
                    if (m_caster.skill() > m_target.skill()) {
                         m_value = m_value * (buffValue() + coefficient() * level()) /100;

                        if (m_value != 0) {
                            LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                            m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                            m_effectValue = (int)(-m_value);
                        }
                    }
                }
            };

            class BuffAgileThanDamage: public BuffBase
            {
            public:
               BuffAgileThanDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffAgileThanDamage() {}

                virtual void OnStart() {
                    if (m_caster.agile() > m_target.agile()) {
                         m_value = m_value * (buffValue() + coefficient() * level()) /100;

                        if (m_value != 0) {
                            LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                            m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                            m_effectValue = (int)(-m_value);
                        }
                    }
                }
            };


            class BuffWisdomThanDamage: public BuffBase
            {
            public:
               BuffWisdomThanDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffWisdomThanDamage() {}

                virtual void OnStart() {
                    if (m_caster.wisdom() > m_target.wisdom()) {
                         m_value = m_value * (buffValue() + coefficient() * level()) /100;

                        if (m_value != 0) {
                            LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                            m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                            m_effectValue = (int)(-m_value);
                        }
                    }
                }
            };


            class BuffLuckyThanDamage: public BuffBase
            {
            public:
               BuffLuckyThanDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffLuckyThanDamage() {}

                virtual void OnStart() {
                    if (m_caster.lucky() > m_target.lucky()) {
                         m_value = m_value * (buffValue() + coefficient() * level()) /100;

                        if (m_value != 0) {
                            m_effectValue = (int)(-m_value);
                            LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                            m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                        }
                    }
                }
            };


            class BuffLifeThanDamage: public BuffBase
            {
            public:
               BuffLifeThanDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffLifeThanDamage() {}

                virtual void OnStart() {
                    if (m_caster.life() > m_target.life()) {
                         m_value = m_value * (buffValue() + coefficient() * level()) /100;

                        if (m_value != 0) {
                            m_effectValue = (int)(-m_value);
                            LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                            m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                        }
                    }
                }
            };


            class BuffPowerGapDamage: public BuffBase
            {
            public:
               BuffPowerGapDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffPowerGapDamage() {}

                virtual void OnStart() {
                    float gap = m_caster.power() - m_target.power();
                    if (gap > 0) {
                         m_value = gap * (buffValue() + coefficient()*level());

                        if (m_value != 0) {
                            m_effectValue = (int)(-m_value);
                            LOG_DEBUG("Buff Type:%d  Value:%.3f  gap:%.3f ",  (int)m_tBuff.type, m_value, gap);
                            m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                        }
                    }
                }
            };

            class BuffDefenseGapDamage: public BuffBase
            {
            public:
               BuffDefenseGapDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffDefenseGapDamage() {}

                virtual void OnStart() {
                    float gap = m_caster.defense() - m_target.defense();
                    if (gap > 0) {
                         m_value = gap * (buffValue() + coefficient()*level());

                        if (m_value != 0) {
                            m_effectValue = (int)(-m_value);
                            LOG_DEBUG("Buff Type:%d  Value:%.3f  gap:%.3f ",  (int)m_tBuff.type, m_value, gap);
                            m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                        }
                    }
                }
            };

            class BuffSkillGapDamage: public BuffBase
            {
            public:
               BuffSkillGapDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffSkillGapDamage() {}

                virtual void OnStart() {
                    float gap = m_caster.skill() - m_target.skill();
                    if (gap > 0) {
                         m_value = gap * (buffValue() + coefficient()*level());

                        if (m_value != 0) {
                            m_effectValue = (int)(-m_value);
                            LOG_DEBUG("Buff Type:%d  Value:%.3f  gap:%.3f ",  (int)m_tBuff.type, m_value, gap);
                            m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                        }
                    }
                }
            };


            class BuffAgileGapDamage: public BuffBase
            {
            public:
               BuffAgileGapDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffAgileGapDamage() {}

                virtual void OnStart() {
                    float gap = m_caster.agile() - m_target.agile();
                    if (gap > 0) {
                         m_value = gap * (buffValue() + coefficient() * level());

                        if (m_value != 0) {
                            m_effectValue = (int)(-m_value);
                            LOG_DEBUG("Buff Type:%d  Value:%.3f  gap:%.3f ",  (int)m_tBuff.type, m_value, gap);
                            m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                        }
                    }
                }
            };


            class BuffWisdomGapDamage: public BuffBase
            {
            public:
               BuffWisdomGapDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffWisdomGapDamage() {}

                virtual void OnStart() {
                    float gap = m_caster.wisdom() - m_target.wisdom();
                    if (gap > 0) {
                         m_value = gap * (buffValue() + coefficient() * level());

                        if (m_value != 0) {
                            m_effectValue = (int)(-m_value);
                            LOG_DEBUG("Buff Type:%d  Value:%.3f  gap:%.3f ",  (int)m_tBuff.type, m_value, gap);
                            m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                        }
                    }
                }
            };

            class BuffLuckyGapDamage: public BuffBase
            {
            public:
               BuffLuckyGapDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffLuckyGapDamage() {}

                virtual void OnStart() {
                    float gap = m_caster.lucky() - m_target.lucky();
                    if (gap > 0) {
                         m_value = gap * (buffValue() + coefficient() * level());

                        if (m_value != 0) {
                            m_effectValue = (int)(-m_value);
                            LOG_DEBUG("Buff Type:%d  Value:%.3f  gap:%.3f ",  (int)m_tBuff.type, m_value, gap);
                            m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                        }
                    }
                }
            };

            class BuffLifeGapDamage: public BuffBase
            {
            public:
               BuffLifeGapDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffLifeGapDamage() {}

                virtual void OnStart() {
                    float gap = m_caster.life() - m_target.life();
                    if (gap > 0) {
                         m_value = gap * (buffValue() + coefficient() * level());

                        if (m_value != 0) {
                            m_effectValue = (int)(-m_value);
                            LOG_DEBUG("Buff Type:%d  Value:%.3f  gap:%.3f ",  (int)m_tBuff.type, m_value, gap);
                            m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                        }
                    }
                }
            };

            class BuffPowerSelfDamage: public BuffBase
            {
            public:
               BuffPowerSelfDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffPowerSelfDamage() {}

                virtual void OnStart() {
                    float value = m_caster.power();
                    if (value > 0) {
                         m_value = value * (buffValue() + coefficient() * level());

                        if (m_value != 0) {
                            m_effectValue = (int)(-m_value);
                            LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                            m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                        }
                    }
                }
            };

            class BuffDefenseSelfDamage: public BuffBase
            {
            public:
               BuffDefenseSelfDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffDefenseSelfDamage() {}

                virtual void OnStart() {
                    float value = m_caster.defense();
                    if (value > 0) {
                         m_value = value * (buffValue() + coefficient() * level());

                        if (m_value != 0) {
                            m_effectValue = (int)(-m_value);
                            LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                            m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                        }
                    }
                }
            };


            class BuffSkillSelfDamage: public BuffBase
            {
            public:
               BuffSkillSelfDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffSkillSelfDamage() {}

                virtual void OnStart() {
                    float value = m_caster.skill();
                    if (value > 0) {
                         m_value = value * (buffValue() + coefficient() * level());

                        if (m_value != 0) {
                            m_effectValue = (int)(-m_value);
                            LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                            m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                        }
                    }
                }
            };


            class BuffAgileSelfDamage: public BuffBase
            {
            public:
               BuffAgileSelfDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffAgileSelfDamage() {}

                virtual void OnStart() {
                    float value = m_caster.agile();
                    if (value > 0) {
                         m_value = value * (buffValue() + coefficient() * level());

                        if (m_value != 0) {
                            m_effectValue = (int)(-m_value);
                            LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                            m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                        }
                    }
                }
            };

            class BuffWisdomSelfDamage: public BuffBase
            {
            public:
               BuffWisdomSelfDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffWisdomSelfDamage() {}

                virtual void OnStart() {
                    float value = m_caster.wisdom();
                    if (value > 0) {
                         m_value = value * (buffValue() + coefficient() * level());

                        if (m_value != 0) {
                            m_effectValue = (int)(-m_value);
                            LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                            m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                        }
                    }
                }
            };

            class BuffLuckySelfDamage: public BuffBase
            {
            public:
               BuffLuckySelfDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffLuckySelfDamage() {}

                virtual void OnStart() {
                    float value = m_caster.lucky();
                    if (value > 0) {
                         m_value = value * (buffValue() + coefficient() * level());

                        if (m_value != 0) {
                            m_effectValue = (int)(-m_value);
                            LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                            m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                        }
                    }
                }
            };

            class BuffLifeSelfDamage: public BuffBase
            {
            public:
               BuffLifeSelfDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffLifeSelfDamage() {}

                virtual void OnStart() {
                    float value = m_caster.life();
                    if (value > 0) {
                         m_value = value * (buffValue() + coefficient() * level());

                        if (m_value != 0) {
                            m_effectValue = (int)(-m_value);
                            LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, m_value);
                            m_target.OnBuffDirectDamage(m_caster, (int)m_value);
                        }
                    }
                }
            };

            class BuffAddRage: public BuffBase
            {
            public:
               BuffAddRage(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {
                }

                virtual ~BuffAddRage() {}

                virtual void OnStart() {
                    float value = buffValue() + coefficient() * level();
                    //LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, value);
                    m_effectValue = m_target.AddRage((int)value);
                    LOG_DEBUG("Buff Type:%d  Value:%.3f RealValue:%d ", (int)m_tBuff.type, value, m_effectValue);
                }
            };

            class BuffMinusRage: public BuffBase
            {
            public:
               BuffMinusRage(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {
                }

                virtual ~BuffMinusRage() {}

                virtual void OnStart() {
                    float value = (buffValue() + coefficient() * level()) * -1;
                    m_effectValue = m_target.AddRage((int)value);
                    LOG_DEBUG("Buff Type:%d  Value:%.3f RealValue:%d ", (int)m_tBuff.type, value, m_effectValue);
                }
            };

            class BuffRecoverWisdomFight: public BuffBase
            {
            public:
               BuffRecoverWisdomFight(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {
                }

                virtual ~BuffRecoverWisdomFight() {}

                virtual void OnStart() {
                    float value =  m_caster.wisdom() * (buffValue() + coefficient() * level());
                    m_effectValue = m_target.AddArmyHp((int)value);
                    LOG_DEBUG("Buff Type:%d  Value:%.3f RealValue:%d ", (int)m_tBuff.type, value, m_effectValue);
                }
            };

            class BuffAddWisdomPhysicalDamage: public BuffBase
            {
            public:
               BuffAddWisdomPhysicalDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffAddWisdomPhysicalDamage() {}

                virtual void OnStart() {
                    m_target.SetEffectStatus(TargetStatus::ADD_PHYSICAL_DAMAGE);
                }

                virtual void OnTurn() {
                    if (!m_target.IsStatus(TargetStatus::ADD_PHYSICAL_DAMAGE)) { return; }
                }

                virtual void OnEnd() {
                    OnTurn();
                    m_target.ClearEffectStatus(TargetStatus::ADD_PHYSICAL_DAMAGE);
                }
            };

            class BuffAddWisdomMagicDamage: public BuffBase
            {
            public:
               BuffAddWisdomMagicDamage(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffAddWisdomMagicDamage() {}

                virtual void OnStart() {
                    m_target.SetEffectStatus(TargetStatus::ADD_MAGIC_DAMAGE);
                }

                virtual void OnTurn() {
                    if (!m_target.IsStatus(TargetStatus::ADD_MAGIC_DAMAGE)) { return; }
                }

                virtual void OnEnd() {
                    OnTurn();
                    m_target.ClearEffectStatus(TargetStatus::ADD_MAGIC_DAMAGE);
                }
            };

            class BuffMinusPhysicalDamage: public BuffBase
            {
            public:
               BuffMinusPhysicalDamage(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {
                }

                virtual ~BuffMinusPhysicalDamage() {}

                virtual void OnStart() {
                    m_target.SetEffectStatus(TargetStatus::MINUS_PHYSICAL_DAMAGE);
                }

                virtual void OnTurn() {
                    if (!m_target.IsStatus(TargetStatus::MINUS_PHYSICAL_DAMAGE)) { return; }
                }

                virtual void OnEnd() {
                    OnTurn();
                    m_target.ClearEffectStatus(TargetStatus::MINUS_PHYSICAL_DAMAGE);
                }
            };

            class BuffMinusMagicDamage: public BuffBase
            {
            public:
               BuffMinusMagicDamage(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {
                }

                virtual ~BuffMinusMagicDamage() {}

                virtual void OnStart() {
                    m_target.SetEffectStatus(TargetStatus::MINUS_MAGIC_DAMAGE);
                }

                virtual void OnTurn() {
                    if (!m_target.IsStatus(TargetStatus::MINUS_MAGIC_DAMAGE)) { return; }
                }

                virtual void OnEnd() {
                    OnTurn();
                    m_target.ClearEffectStatus(TargetStatus::MINUS_MAGIC_DAMAGE);
                }
            };

            class BuffCleanNegativeStatus: public BuffBase
            {
            public:
               BuffCleanNegativeStatus(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {
                }

                virtual ~BuffCleanNegativeStatus() {}

                virtual void OnStart() {
                    m_target.CleanNegativeEffectStatus();
                }
            };

            class BuffCleanPositiveStatus: public BuffBase
            {
            public:
               BuffCleanPositiveStatus(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {
                }

                virtual ~BuffCleanPositiveStatus() {}

                virtual void OnStart() {
                    m_target.CleanPositiveEffectStatus();
                }
            };

            class BuffAbsorbDamageHp: public BuffBase
            {
            public:
               BuffAbsorbDamageHp(const T_Buff& tBuff, Hero& caster, Hero& target, int hurt)
                    : BuffBase(tBuff, caster, target) {
                    m_value = hurt;
                }

                virtual ~BuffAbsorbDamageHp() {}

                virtual void OnStart() {
                    float value =  m_value * (buffValue() + coefficient() * level())/100;
                    //LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, value);
                    m_target.AddArmyHp(int(value));
                    m_effectValue = m_target.AddArmyHp((int)value);
                    LOG_DEBUG("Buff Type:%d  Value:%.3f RealValue:%d ", (int)m_tBuff.type, value, m_effectValue);
                }
            };

            class BuffDeductHp: public BuffBase
            {
            public:
               BuffDeductHp(const T_Buff& tBuff, Hero& caster, Hero& target)
                    : BuffBase(tBuff, caster, target) {
                }

                virtual ~BuffDeductHp() {}

                virtual void OnStart() {
                    float value =  m_target.armyHp() * 0.2; // 20%
                    LOG_DEBUG("Buff Type:%d  Value:%.3f ", (int)m_tBuff.type, value);
                    m_effectValue = (int)(-value);
                    m_target.OnBuffDirectDamage(m_caster, (int)value); //扣自身血量
                }
            };

//-----------------------------------------------------------------------------

            BuffList::~BuffList()
            {
                Clear();
            }

            void BuffList::CreateBuff(int level, const T_Buff& tBuff, Hero& caster, int param1, bool isPassive, int& retVal)
            {
                std::cout << " ### CreateBuff Buff Type: " << (int)tBuff.type << " param1 : " << param1 << std::endl;

                BuffBase* buff = nullptr;
                switch (tBuff.type) {
                    case BuffType::PASSIVE_POWER: {
                        buff = new BuffPower(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::PASSIVE_DEFENSE: {
                        buff = new BuffDefense(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::PASSIVE_WISDOM: {
                        buff = new BuffWisdom(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::PASSIVE_SKILL: {
                        buff = new BuffSkill(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::PASSIVE_AGILE: {
                        buff = new BuffAgile(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::PASSIVE_LUCKY: {
                        buff = new BuffLucky(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::PASSIVE_LIFE: {
                        buff = new BuffLife(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::PASSIVE_ALL: {
                        buff = new BuffAll(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::PASSIVE_HIT: {
                        buff = new BuffHit(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::PASSIVE_AVOID: {
                        buff = new BuffAvoid(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::PASSIVE_CRIT_HIT: {
                        buff = new BuffCritHit(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::PASSIVE_CRIT_AVOID: {
                        buff = new BuffCritAvoid(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::PASSIVE_ATTACK_SPEED: {
                        buff = new BuffAttackSpeed(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::NIRVARNA: {
                        buff = new BuffNirvarna(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::COMPLEX: {
                        buff = new BuffComplex(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::SCARE: {
                         buff = new BuffScare(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::TUMBLE: {
                        buff = new BuffTumble(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::WOUND: {
                        buff = new BuffWound(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::PARALYSIS: {
                        buff = new BuffParalysis(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::WEAK: {
                        buff = new BuffWeak(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::STUN: {
                        buff = new BuffStun(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::ADD_DAMAGE: {
                        buff = new BuffAddDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::POWER_THAN_DAMAGE: {
                        buff = new BuffPowerThanDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::DEFENSE_THAN_DAMAGE: {
                        buff = new BuffDefenseThanDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::SKILL_THAN_DAMAGE: {
                        buff = new BuffSkillThanDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::AGILE_THAN_DAMAGE: {
                        buff = new BuffAgileThanDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::WISDOM_THAN_DAMAGE: {
                        buff = new BuffWisdomThanDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::LUCKY_THAN_DAMAGE: {
                        buff = new BuffLuckyThanDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::LIFE_THAN_DAMAGE: {
                        buff = new BuffLifeThanDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::POWER_GAP_DAMAGE: {
                        buff = new BuffPowerGapDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::DEFENSE_GAP_DAMAGE: {
                        buff = new BuffDefenseGapDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::SKILL_GAP_DAMAGE: {
                        buff = new BuffSkillGapDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::AGILE_GAP_DAMAGE: {
                        buff = new BuffAgileGapDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::WISDOM_GAP_DAMAGE: {
                        buff = new BuffWisdomGapDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::LUCKY_GAP_DAMAGE: {
                        buff = new BuffLuckyGapDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::LIFE_GAP_DAMAGE: {
                        buff = new BuffLifeGapDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::POWER_SELF_DAMAGE: {
                        buff = new BuffPowerSelfDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::SKILL_SELF_DAMAGE: {
                        buff = new BuffSkillSelfDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::AGILE_SELF_DAMAGE: {
                        buff = new BuffAgileSelfDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::WISDOM_SELF_DAMAGE: {
                        buff = new BuffWisdomSelfDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::LUCKY_SELF_DAMAGE: {
                        buff = new BuffLuckySelfDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::LIFE_SELF_DAMAGE: {
                        buff = new BuffLifeSelfDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::ADD_RAGE: {
                        buff = new BuffAddRage(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::MINUS_RAGE: {
                        buff = new BuffMinusRage(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::RECOVER_WISDOM_FIGHT: {
                        buff = new BuffRecoverWisdomFight(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::ADD_WISDOM_PHYSICAL_DAMAGE: {
                        buff = new BuffAddWisdomPhysicalDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::ADD_WISDOM_MAGIC_DAMAGE: {
                        buff = new BuffAddWisdomMagicDamage(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::MINUS_PHYSICAL_DAMAGE: {
                        buff = new BuffMinusPhysicalDamage(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::MINUS_MAGIC_DAMAGE: {
                        buff = new BuffMinusMagicDamage(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::CLEAN_NEGATIVE_STATE: {
                        buff = new BuffCleanNegativeStatus(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::CLEAN_POSITIVE_STATE: {
                        buff = new BuffCleanPositiveStatus(tBuff, caster, m_owner);
                    }
                    break;
                    case BuffType::ABSORB_DAMAGE_HP: {
                        buff = new BuffAbsorbDamageHp(tBuff, caster, m_owner, param1);
                    }
                    break;
                    case BuffType::DEDUCT_HP: {
                        buff = new BuffDeductHp(tBuff, caster, m_owner);
                    }
                    break;
                    default:
                        std::cout << " !!! Default Buff Type: " << (int)tBuff.type << std::endl;
                        break;
                }

                if (buff) {
                    std::cout << " --- CastBuff --- " << (int)tBuff.type << " --- Passive: " << isPassive << std::endl;
                    buff->Init(level);

                    for (auto it = m_buffs.begin(); it != m_buffs.end(); ) {
                        auto buf = *it;
                        //重置相同的buff
                        if (buf->tBuff().type == tBuff.type) {
                            buf->Clear();
                            delete buf;
                            it = m_buffs.erase(it);
                        } else {
                            it++;
                        }
                    }

                    m_buffs.push_back(buff);

                    //Todo: 目前主动技能和被动技能还没区分
                    retVal = buff->Start(level);
                }
            }

            void BuffList::Update(TriggerBuffState state)
            {
                for (auto it = m_buffs.begin(); it != m_buffs.end();) {
                    if (state != (*it)->triggerState()) {
                        LOG_DEBUG("BuffList::Update self-state:%d  state:%d buff-type: %d", (int)((*it)->triggerState()),
                            (int)state, (int)((*it)->tBuff().type));
                        it++;
                        continue;
                    }

                    BuffBase* buff = *it;
                    LOG_DEBUG("Update ---- buff ----- %d ---- ", (int)(buff->tBuff().type));

                    if (buff->lastRounds() <= 0) {
                        delete buff;
                        it = m_buffs.erase(it);
                    } else {
                        buff->Update();
                        it++;
                    }
                }
            }

            void BuffList::Clear()
            {
                for (auto it = m_buffs.begin(); it != m_buffs.end(); ++it) {
                    delete *it;
                }
                m_buffs.clear();
            }

            void BuffList::ClearDebuff()
            {
            }

            void BuffList::ClearAdbuff()
            {


            }
        }
    }
}
