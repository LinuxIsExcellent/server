#include <iostream>
#include <unistd.h>
#include <libgen.h>
#include <set>
#include "configure.h"
#include "player.h"
#include "./dal/frontserver.h"

using namespace std;
using namespace dal;

int main(int argc, char** argv)
{
    srand(time(NULL));

    string path_;
    string t;
    char buf[500];
    uint32_t ret = readlink("/proc/self/exe", buf, 500);
    if (ret > 0 && ret < 500) {
        buf[ret] = '\0';
        t = dirname(buf);    //
        path_ = dirname((char*) t.c_str());
    } else {
        return 0;
    }
    system(buf);

    cout << "path = " << path_ << endl;
    Configure cfg;
    string configfile = path_ + string("/config/client_simulation.conf");
    // read config
    if (!cfg.ParseXml(configfile)) {
        printf("ERROR: Parse xml file [%s] failed.\n", configfile.c_str());
        return 0;
    }
    // 根据配置，创建相应的执行者
    if (cfg.action_list_size() <= 0 || cfg.users() <= 0) {
        // 没有可执行的任务，或没有测试的用户
        printf("ERROR: action or user does not have sufficient.\n");
        return 0;
    }
    uint32_t fw = cfg.users();
    if (fw > 1000) {
        fw = 1000;
    }
    //fw = 1000;
    Theron::Framework framework(fw);
    // Construct a receiver to receive the reply message.
    Theron::Receiver receiver;

    // 创建用户
    std::vector<Player*> players(cfg.users());
    printf("test user num %d.\n", cfg.users());
    sleep(1);

    uint32_t index = 0;
    vector<Actions> list = cfg.action_list();
    int player_num = 0;
    for (vector<Actions>::iterator it = list.begin(); it != list.end(); ++it) {
        UserActions useraction;
        Actions actions = (*it);
        for (int32_t i = actions.begin; i <= actions.end; i++) {
            useraction.path = path_;
            useraction.serverip = cfg.serverip();
            useraction.serverport = cfg.serverport();
            useraction.nameprefix = actions.nameprefix;
            useraction.namesuffix = i;
            useraction.actions = actions.actions;
            printf("\n\n#######  new Actions user %s%d #######.\n", useraction.nameprefix.c_str(), i);
            player_num ++;
            players[index++] = new Player(useraction, framework);
        }
    }

    std::cout << "player_num = " << player_num << std::endl;

    printf("\n\n  new all Actions user [%d] OK  \n", index);
    sleep(1);
    for (uint32_t j = 0; j < index; j++) {
        //printf("\n\n@@@@@@@  Start Actions user %d @@@@@@@@.\n",  j);
        framework.Send(string(""), receiver.GetAddress(), players[j]->GetAddress());
        usleep(1000 * 50);
    }

    // 等待结束
    int outstandingCount(index);
    while (outstandingCount) {
        receiver.Wait();
        outstandingCount--;
    }
    for (int32_t i = 0; i < cfg.users(); i++) {
        delete players[i];
    }
    printf(" -------  game over ------- \n");
    return 0;
}
