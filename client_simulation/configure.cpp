#include "configure.h"
#include <unistd.h>
#include <libgen.h>
#include <base/3rd/tinyxml2.h>
#include <base/utils/file.h>

using namespace std;

Configure::Configure() : serverip_(""), serverport_(0), users_(0)
{
    action_list_.clear();
}

Configure::~Configure()
{
}

bool Configure::ParseXml(const std::string& file)
{
    try {
        // 读取全局配置信息
        tinyxml2::XMLDocument doc;
        if (!base::utils::file_is_exist(file.c_str())) {
            printf("%s not exist", file.c_str());
            return false;
        }

        tinyxml2::XMLError err = doc.LoadFile(file.c_str());
        if (err != tinyxml2::XML_NO_ERROR) {
            printf("parse common.conf fail: %s", doc.GetErrorStr2());
            return false;
        }

        tinyxml2::XMLElement* s = doc.RootElement()->FirstChildElement("global")->FirstChildElement("server");
        serverip_ = s->Attribute("ip");
        serverport_ = s->IntAttribute("port");
        printf("Configure::ParseXml load ip %s, port %d\n", serverip_.c_str(), serverport_);

        Actions actions;
        tinyxml2::XMLElement* u = doc.RootElement()->FirstChildElement("userlist");
        actions.nameprefix = u->FirstChildElement("actionlist")->Attribute("nameprefix");
        actions.begin = u->FirstChildElement("actionlist")->IntAttribute("begin");
        actions.end = u->FirstChildElement("actionlist")->IntAttribute("end");
        users_ = actions.end - actions.begin + 1;
        tinyxml2::XMLElement* a = u->FirstChildElement("actionlist")->FirstChildElement("action");
        int action_count = 0;
        while (a) {
            ActionBase actionbase;
            actionbase.actionname = a->Attribute("name");
            actionbase.loop = a->IntAttribute("loop");
            actionbase.interval = a->IntAttribute("interval");
            tinyxml2::XMLElement* d = a->FirstChildElement("do");
            printf("Configure::ParseXml action idx %d, name %s, loop %d, interval %d\n", action_count, actionbase.actionname.c_str(), actionbase.loop, actionbase.interval);
            while (d) {
                string doName = d->Attribute("name");
                string param = "";
                if (d->Attribute("param")) {
                    param = d->Attribute("param");
                }
                string text = d->GetText();
                printf("Configure::ParseXml action DO: doName %s, param %s, text %s\n", doName.c_str(), param.c_str(), text.c_str());
                actionbase.actions.push_back(ActionBase::action(doName, param));
                d = d->NextSiblingElement("do");
            }

            if (actionbase.actions.size() > 0) {
                actions.actions.push_back(actionbase);
            }
            action_count++;
            //a = u->FirstChildElement("actionlist")->NextSiblingElement("action");
            a = a->NextSiblingElement("action");
        }
        action_list_.push_back(actions);
        printf("Configure::ParseXml OVER\n");

    } catch (exception& ex) {
        printf("parse configure failed. [%s]\n", ex.what());
        return false;
    }
    return true;
}