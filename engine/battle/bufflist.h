#ifndef ENGINE_BATTLE_BUFF_BUFFLIST_H
#define ENGINE_BATTLE_BUFF_BUFFLIST_H
#include <list>
#include <vector>
#include "../metadata.h"

namespace engine
{
    namespace tpl
    {
        class T_Buff;
    }

    enum class BuffType;
    namespace battle
    {

        class Hero;
        namespace buff
        {
            class BuffBase;
            class BuffList
            {
            public:
                BuffList(engine::battle::Hero& owner)
                    : m_owner(owner) {}
                ~BuffList();

                void CreateBuff(int level,  const engine::tpl::T_Buff& tBuff, engine::battle::Hero& caster, int param1,
                    bool isPassive, int& retVal);
                // 更新buff
                void Update(TriggerBuffState state);

                void Clear();

                void ClearDebuff();
                void ClearAdbuff();

            private:
                std::list<BuffBase*> m_buffs;
                std::list<BuffBase*> m_passiveBuffs;              // 被动buff触发的buff
                engine::battle::Hero& m_owner;
            };
        }
    }
}

#endif // ENGINE_BATTLE_BUFF_BUFFLIST_H
