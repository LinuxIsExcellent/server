#include <base/framework.h>
#include "modulegateway.h"
#include <base/utils/file.h>

using namespace std;

int main(int argc, char* argv[])
{
    base::ServerRule tc;
    if (!framework.Setup("/opt/s3/server/resource", tc)) {
        return -1;
    }
    
    framework.RegisterModule(ts::ModuleGateway::Create());
    
    /*vector<string> ret = base::utils::dir_get_files("/opt/s3/server", 10);
    for (const string& file : ret) {
        cout << file << endl;
    }*/

    return framework.Run();
    
}
