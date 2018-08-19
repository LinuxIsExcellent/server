#ifndef CONFIGURE_H
#define CONFIGURE_H
#include <string>
#include <vector>
#include <stdint.h>
#include <map>

// 一个基本动作集合
struct ActionBase {
    int32_t loop;
    int32_t interval;
    std::string actionname;
    struct action {
        action(const std::string d, std::string p) : do_it(d), param(p) {}
        std::string do_it;
        std::string param;
    };
    std::vector<action> actions;
};

// 一个用户集，包含要执行的动作
struct Actions {
    int32_t begin;
    int32_t end;
    std::string nameprefix;
    std::vector<ActionBase> actions;       
};

// 一个用户的动作集
struct UserActions {
    std::string path;
    int32_t serverport;
    uint32_t namesuffix;      // 用户名后缀　数字，方便查找前后的用户
    std::string serverip;
    std::string nameprefix;
    std::vector<ActionBase> actions;       
};

class Configure
{

public:
    Configure();
    virtual ~Configure();
    //　解析xml
    bool ParseXml(const std::string& file);

    const std::string& serverip() const {
        return serverip_;
    }
    
    int32_t serverport() const {
        return serverport_;
    }

    int32_t users() const {
        return users_;
    }
    
    const std::vector<Actions> & action_list() const {
        return action_list_;
    }
    
    int32_t action_list_size() const {
        return action_list_.size();
    }
    
private:
    std::string serverip_;
    int32_t serverport_;
    int32_t users_ = 0;  // 用户总数
    std::vector<Actions> action_list_;
    
};

#endif // CONFIGURE_H
