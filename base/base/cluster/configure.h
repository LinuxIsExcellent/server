#ifndef BASE_CLUSTER_CONFIGURE_H
#define BASE_CLUSTER_CONFIGURE_H

#include "../global.h"
#include <string>
#include <iosfwd>

namespace base
{
    namespace cluster
    {
        enum class NodeType
        {
            UNKNOWN = 100,
            NORMAL = 0,
            MASTER,
            STANDALONE
        };

        struct NodeInfo {
            uint32_t node_id = 0u;

            NodeType type = NodeType::UNKNOWN;

            std::string listen_ip;
            int32_t listen_port;

            std::string node_name;

            const char* GetTypeString() const;

            bool ReadFromConfigFile(const std::string& name, uint16_t id = 0u);
        };

        struct SetupOption {
            std::string node_name;

            std::string listen_ip;
            int listen_port = 0;


            std::string master_ip;
            int master_port = 0;

            NodeType type;

            bool ReadFromDefaultConfigFile();
        };

        std::ostream& operator << (std::ostream& out, const NodeInfo& info);
    }
}

#endif // CONFIGURE_H
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
