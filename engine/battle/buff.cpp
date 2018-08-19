#include "buff.h"
#include "../tpl/t_buff.h"

namespace engine
{
    namespace battle
    {
        namespace buff
        {
            BuffBase::BuffBase(const tpl::T_Buff& tBuff, Hero& caster, Hero& target)
                : m_tBuff(tBuff), m_caster(caster), m_target(target)
            {

            }

            void BuffBase::Init(int level)
            {
                m_level = level;
                // if (m_tBuff.valueType == BuffValueType::VALUE || m_tBuff.valueType == BuffValueType::PERCENT) {
                //     // ·加成数值属性公式  总数值=基础数值*（10000+等级*参数1/10）/10000
                //     // ·加成百分比属性公式  总比例=（基础数值*参数1/10000）+（等级*参数2/100） 得出的值 是万分比
                //     int coefficient1 = 0;
                //     int coefficient2 = 0;
                //     if (m_tBuff.coefficients.size() >= 2) {
                //         coefficient1 = m_tBuff.coefficients[0];
                //         coefficient2 = m_tBuff.coefficients[1];
                //     }

                //     int factor = 1;
                //     int baseValue = m_tBuff.value;
                //     if (baseValue < 0) {
                //         factor = -1;
                //         baseValue = -baseValue;
                //     }

                //     int buffValue = 0;
                //     if (m_tBuff.valueType == BuffValueType::VALUE) {
                //         buffValue = baseValue * (10000 + level * coefficient1 / 10.0) / 10000.0;
                //     }  else if (m_tBuff.valueType == BuffValueType::PERCENT) {
                //         buffValue = (baseValue * coefficient1 / 10000.0) + (level * coefficient2 / 100.0);
                //     }

                //     m_buffValue = factor * buffValue;

                m_buffValue = m_tBuff.base_value1;
                m_coefficient = m_tBuff.level_value1;

                m_trigger_state = m_tBuff.trigger_state;

                m_lastRounds = m_tBuff.rounds;
            }

            int BuffBase::Start(int level)
            {
                OnStart();
                if (m_lastRounds > 0) { m_lastRounds--; }
                return m_effectValue;
            }

            void BuffBase::Update()
            {
                --m_lastRounds;
                if (m_lastRounds > 0) {
                    OnTurn();
                }

                if (m_lastRounds <= 0) {
                    OnEnd();
                }
            }

            void BuffBase::Clear()
            {
                m_lastRounds = 0;
            }

        }
    }
}
