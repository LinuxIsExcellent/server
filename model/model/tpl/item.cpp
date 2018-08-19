#include "item.h"

namespace model
{
    namespace tpl
    {
        EquipTpl* ItemTpl::ToEquipTpl()
        {
            return type == ItemType::EQUIP ? static_cast<EquipTpl*>(this) : nullptr;
        }

        PropTpl* ItemTpl::ToPropTpl()
        {
            return type == ItemType::PROP ? static_cast<PropTpl*>(this) : nullptr;
        }

        
    }
}
