#include <base/framework.h>
#include <base/logger.h>
#include "map/modulemap.h"
#include "map/tpl/moduletemplateloader.h"
#include <model/tpl/moduletemplateloader.h>
#include "map/modulebattle.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    std::string project_path = "/home/denny/numerical_server/resource";
    if (argc > 1) {
        std::string p = argv[1];
        project_path = p + "/resource";
        std::cout << project_path << std::endl;
    }
    base::ServerRule rule("ms", 0);
    if (!framework.AutoSetup(rule, project_path)) {
        return -1;
    }

    // framework.RegisterModule(fs::ModuleGateway::Create());
    // framework.RegisterModule(model::tpl::ModuleTemplateLoader::Create());
    // framework.RegisterModule(fs::ModuleBattle::Create());

    // framework.RegisterModule(cs::ModuleCenterServer::Create());

    framework.RegisterModule(model::tpl::ModuleTemplateLoader::Create());
    framework.RegisterModule(ms::map::tpl::ModuleTemplateLoader::Create());
    framework.RegisterModule(ms::map::ModuleMap::Create());
    framework.RegisterModule(ms::map::ModuleBattle::Create());
    
    return framework.Run();
} 
