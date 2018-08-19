#ifndef MODEL_TPL_ITEM_H
#define MODEL_TPL_ITEM_H

#include "../metadata.h"
#include <string>
#include <map>
#include <vector>

namespace model
{
    namespace tpl
    {
        struct Attribute {
            AttributeType attribute;
            AttributeAdditionType additionType;
            int value;
            Attribute(AttributeType type, AttributeAdditionType addType, int v) : attribute(type), additionType(addType), value(v) {}
        };
        
        struct EquipTpl;
        struct PropTpl;
        
        struct ItemTpl {
            ItemTpl(ItemType t) : type(t) {}
            virtual ~ItemTpl() {}
            
            int id;
            std::string name;
            ItemType type;
            int subType;
            int quality;
            int requireLordLevel;
            int requireCastleLevel;
            int price;
            
            bool IsEquip() const {
                return type == ItemType::EQUIP;
            }
            bool IsProp() const {
                return type == ItemType::PROP;
            }
            
            EquipTpl* ToEquipTpl();
            PropTpl* ToPropTpl();
            const EquipTpl* ToEquipTpl() const {
                return const_cast<ItemTpl*>(this)->ToEquipTpl();
            }
            const PropTpl* ToPropTpl() const {
                return const_cast<ItemTpl*>(this)->ToPropTpl();
            }
        };
        
        struct EquipTpl : public ItemTpl {
            EquipTpl() : ItemTpl(ItemType::EQUIP) {}
            
            ItemEquipType equipType;
            
            std::vector<Attribute> attributes;
        };
        
        struct PropTpl : public ItemTpl {
            PropTpl() : ItemTpl(ItemType::PROP) {}
            
            ItemPropType propType;
            
            
        };
        
    }
}

#endif // ITEM_H
