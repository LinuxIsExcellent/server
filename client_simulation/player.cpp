#include "player.h"
#include "dal/character.h"
#include "dal/map.h"
#include "dal/misc.h"
#include "dal/alliance.h"
#include <sstream>
#include <unistd.h>
#include <libgen.h>
#include <iostream>
#include <ctime>

Player::Player(const UserActions& useractions, Theron::Framework& framework) : Theron::Actor(framework)
{
    useractions_ = useractions;
    username_ = useractions_.nameprefix + to_string(useractions_.namesuffix);
    std::cout << "useractions_.serverip: " << useractions_.serverip << " useractions_.serverport: " << useractions_.serverport << std::endl;

    RegisterHandler(this, &Player::Run);
    srand(time(NULL));
}

Player::~Player()
{
    if (fs_) {
        delete fs_;
        fs_ = nullptr;
    }
}

void Player::Run(const std::string& message, const Theron::Address from)
{
    printf("################## new Run. #####################\n");
    uint32_t timeout = 12;
    uint32_t result = 9999;
    uint32_t loops = 1;
    fs_ = new dal::FrontServer(useractions_.serverip, useractions_.serverport, useractions_.path);
    loops += std::rand() % 35;
    do {
        if (fs_ == NULL) {
            // error, return
            break;
        }
        // 建立连接
        fs_->PerformIO();
        while (timeout) {
            fs_->PerformIO();
            if (!fs_->isConnected()) {
                timeout--;
                sleep(1);
                continue;
            }
            break;
        }

        if (timeout <= 0) {
            // 连接超时，退出
            printf("Run ... user %s%d ... connect server[%s:%d] timeout.\n", useractions_.nameprefix.c_str(), useractions_.namesuffix, useractions_.serverip.c_str(), useractions_.serverport);
            break;
        }
        printf("################## Login begin. #####################\n");
        result = Login();
        fs_->PerformIO();
        printf("################## Login end. #####################\n");
        if (result != 0) {
            printf("user [%s] login fail, error=%u!\n", username_.c_str(), result);
            break;
        }
        printf("user [%s] login success!\n", username_.c_str());

        while (1) {
            // loops为0则重登
            if (loops-- == 0) {
                Stop(message, from);
                break;
            }
            fs_->PerformIO();
            for (vector<ActionBase>::iterator it = useractions_.actions.begin(); it != useractions_.actions.end(); ++it) {
                fs_->PerformIO();
                // 开始执行配置事务
                ActionBase action = (*it);
                for (int32_t i = 0; i < action.loop; i++) {
                    fs_->PerformIO();
                    for (std::vector<ActionBase::action>::iterator step = action.actions.begin(); step != action.actions.end(); step++) {
                        //printf("do class name %s step %s\n", action.actionname.c_str(), string(*step).c_str());
                        ActionBase::action& ac = *step;
                        fs_->PerformIO();
                        DoOneStep(action.actionname, ac.do_it, ac.param);
                        fs_->PerformIO();
                    }
                    if (i < action.loop) {
                        sleep(action.interval);
                    }
                }
            }
        }

    } while (0);
}

void Player::Stop(const std::string &message, const Theron::Address from)
{
    printf("############### Player::Stop ######################\n");
    if (fs_) {
        fs_->DisConnect();
        delete fs_;
        fs_ = nullptr;
    }
    uint16_t sleepTime = 10;
    sleepTime += std::rand() % 90;
    sleep(sleepTime);
    
    Run(message, from);
}

uint32_t Player::Login()
{
    // 登录
    uint32_t timeout = 10;
    uint32_t result = 9999;
    fs_->character()->Login(useractions_.namesuffix, username_);
    fs_->PerformIO();
    while (timeout > 0) {
        sleep(1);
        if ((result = fs_->character()->loginResult()) != 9999) {
            break;
        }
        fs_->PerformIO();
        timeout--;
    }
    fs_->PerformIO();
    if (timeout <= 0) {
        // 登录超时，退出
        printf("Run ... user %s%d ... login to[%s:%d] timeout.\n", useractions_.nameprefix.c_str(), useractions_.namesuffix, useractions_.serverip.c_str(), useractions_.serverport);
        return result;
    }

    return result;
}


uint32_t Player::DoOneStep(const std::string& actionname, const std::string& step, const std::string& param)
{
    uint32_t result = 0;

    printf("user [%s] do step [%s.%s].\n", username_.c_str(), actionname.c_str(), step.c_str());
    
    if (actionname == "misc") {
        if (step == "do_something") {
            fs_->misc()->DoSomething();
        }
    } else if (actionname == "map_join") {
        fs_->map()->MapView();
    } else if (actionname == "map_move") {
        fs_->map()->MapView();
    } else if (actionname == "map_march") {
        fs_->map()->MapMarch();
    } else if (actionname == "map_leave") {
        fs_->map()->MapLeave();
    } else if (actionname == "teleport") {
        fs_->map()->Teleport();
    }
    
    return result;
}

