#ifndef BASE_CONFIGUREHELPER_H
#define BASE_CONFIGUREHELPER_H

#include <tinyxml/tinyxml2.h>
#include <string>
#include <cstdint>

namespace base
{
    class ConfigureHelper
    {
    public:
        ConfigureHelper();

        bool has_error() const {
            return _has_error;
        }

        tinyxml2::XMLElement* FirstChildElementWithPath(tinyxml2::XMLElement* node, const char* path);

        tinyxml2::XMLElement* fetchConfigNodeWithServerRule();
        tinyxml2::XMLElement* fetchConfigNodeWithServerRule(const std::string& name, uint16_t id);

    private:
        bool _has_error = false;
        tinyxml2::XMLDocument _doc;
    };
}

#endif // CONFIGUREHELPER_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
