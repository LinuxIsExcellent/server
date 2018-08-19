#ifndef ENGINE_BATTLE_BUFF_H
#define ENGINE_BATTLE_BUFF_H
#include <stdint.h>
#include "../metadata.h"
#include "../tpl/t_buff.h"

namespace engine
{
    namespace tpl
    {
        struct T_Buff;
    }

    namespace battle
    {
        class Hero;
        namespace buff
        {
            class BuffBase
            {
            public:
                BuffBase(const engine::tpl::T_Buff& tBuff, engine::battle::Hero& caster, engine::battle::Hero& target);
                virtual ~BuffBase() {}

                const engine::tpl::T_Buff& tBuff() const {
                    return m_tBuff;
                }

                uint16_t lastRounds() const {
                    return m_lastRounds;
                }

                float buffValue() {
                    return m_buffValue;
                }

                float coefficient() {
                    return m_coefficient;
                }

                int32_t level() {
                    return m_level;
                }

                TriggerBuffState triggerState() {
                    return m_trigger_state;
                }

                void Init(int level);
                int Start(int level);
                void Update();
                void Clear();

                engine::battle::Hero& caster() {
                    return m_caster;
                }

                engine::battle::Hero& target() {
                    return m_target;
                }

                void SetLastRounds(int rounds) {
                    m_lastRounds = 0;
                }

            private:
                virtual void OnStart() {}
                virtual void OnTurn() {}
                virtual void OnEnd() {}

            protected:
                const engine::tpl::T_Buff m_tBuff;
                engine::battle::Hero& m_caster;
                engine::battle::Hero& m_target;
                float m_value = 0;                         // 加成的值
                float m_buffValue = 0;                    // buff属性值
                int32_t m_level = 1;
                float m_coefficient = 0;
                int m_effectValue = 0; //效果值
            private:
                uint16_t m_lastRounds = 0;                  // 持续回合数

                TriggerBuffState m_trigger_state = TriggerBuffState::NONE;
            };
        }
    }
}

#endif // ENGINE_BATTLE_BUFF_H
