#include "lang.h"
#include <tinyxml/tinyxml2.h>

namespace base
{
    using namespace std;

    Lang::Lang()
    {
    }

    void Lang::Setup(const std::string& res_dir)
    {
        string file = res_dir + "/lang.xml";
        tinyxml2::XMLDocument doc;
        tinyxml2::XMLError err = doc.LoadFile(file.c_str());
        if (err != tinyxml2::XML_NO_ERROR) {
            return;
        }

        tinyxml2::XMLElement* item = doc.RootElement()->FirstChildElement("lang");
        while (item) {
            const char* key = item->Attribute("k");
            const char* val = item->GetText();
            if (key && val) {
                langs_.insert(make_pair(string(key), string(val)));
            }
            item = item->NextSiblingElement("lang");
        }
    }
}

// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
