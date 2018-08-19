#include <base/framework.h>
#include <base/logger.h>
#include "modulecenterserver.h"

using namespace std;

int main(int argc, char* argv[])
{
	std::string project_path = "/home/denny/numerical_server/resource";
    if (argc > 1) {
        std::string p = argv[1];
        project_path = p + "/resource";
        std::cout << project_path << std::endl;
    }
    base::ServerRule rule("cs", 0);
    if (!framework.AutoSetup(rule, project_path)) {
        return -1;
    }

    framework.RegisterModule(cs::ModuleCenterServer::Create());

    return framework.Run();
}
