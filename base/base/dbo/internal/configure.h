#ifndef BASE_DBO_INTERNAL_CONFIGURE_H
#define BASE_DBO_INTERNAL_CONFIGURE_H

#include "../../global.h"
#include <string>
#include <vector>

namespace base
{
    namespace dbo
    {
        namespace internal
        {
            // MySQL服务器信息
            struct ServerInfo {
                std::string dbname;             // 数据库名
                std::string username;           // 用户名
                std::string password;           // 密码
                std::string host;               // 主机地址
                int port;                       // 端口
            };

            // 连接池信息
            struct ConnectionPoolInfo {
                ConnectionPoolInfo() :
                    poolid(0), poolsize(0) {}
                int poolid;                     // 池编号
                int poolsize;                   // 连接池大小
                int max_poolsize;               // 连接池最大值
                ServerInfo serverinfo;          // 服务器信息
            };

            bool ParseDBConfigureFile(const std::string& conf_file, std::vector<ConnectionPoolInfo>& conf);
        }
    }
}

#endif // CONFIGURE_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
