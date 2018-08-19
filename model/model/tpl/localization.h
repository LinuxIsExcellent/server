#ifndef MODEL_TPL_LOCALIZATION_H
#define MODEL_TPL_LOCALIZATION_H

#include "../metadata.h"
#include <string>
#include <map>

namespace model
{
    namespace tpl
    {
        struct Localization
        {
            Localization(std::string n) : name(n) {}
            
            std::string name;
            std::map<LangType, std::string> values;
        };
    }
}

#endif // LOCALIZATION_H
