#include <base/framework.h>
#include <base/logger.h>
#include <fstream>
#include "modulegateway.h"
#include "model/tpl/moduletemplateloader.h"
#include "modulebattle.h"

using namespace std;

int main(int argc, char* argv[])
{
	std::string project_path = "/home/denny/numerical_server/resource";
    if (argc > 1) {
        std::string p = argv[1];
        project_path = p + "/resource";
        std::cout << project_path << std::endl;
    }
    base::ServerRule rule("fs", 1);
    if (!framework.AutoSetup(rule, project_path)) {
        return -1;
    }

    framework.RegisterModule(fs::ModuleGateway::Create());
    framework.RegisterModule(model::tpl::ModuleTemplateLoader::Create());
    framework.RegisterModule(fs::ModuleBattle::Create());
    
    return framework.Run();
}
