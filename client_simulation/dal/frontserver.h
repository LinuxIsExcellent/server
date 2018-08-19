#ifndef FRONTSERVER_H
#define FRONTSERVER_H

#include "../client/connector.h"
#include <base/memory/memorypoolmgr.h>
#include <model/protocol.h>
#include <model/metadata.h>
#include <string>
#include <stdint.h>
#include <map>

namespace dal
{
    using namespace std;
    
    class ClientImpl;
    class Character;
    class Misc;
    class Map;
    class Alliance;
    
    class FrontServer
    {
    public:
        static FrontServer& instance() {
            static FrontServer ins;
            return ins;
        }
        
        FrontServer();
        
        FrontServer(const string& serverip, const int32_t serverport, const std::string& path);
        
        ~FrontServer();
        
        client::MemoryPool& mempool() {
            return conn_->mempool();
        }
 
        bool isConnected() const {
            return isConnected_;
        }
        
        void SetServer(const std::string& ip, int32_t port) {
            ip_ = ip;
            port_ = port;
        }

        ClientImpl* clientImpl() const {
            return clientImpl_;
        }
        
        void OnConnect();
        
        void DisConnect();
        
        void PerformIO();
        
        void Send(client::PacketOut& pktout)
        {
            conn_->Send(pktout);
        }
       
    public:
        Character* character() const {
            return character_;
        }
        
        Misc* misc() const {
            return misc_;
        }
        
        Alliance* alliance() const {
            return alliance_;
        }
        
        Map* map() const {
            return map_;
        }
        
        const std::string& path() const {
            return path_;
        }
        
    private:
        ClientImpl *clientImpl_ = nullptr;
        client::Connector* conn_ = nullptr;
        bool isConnected_ = false;
        std::string ip_;
        int32_t port_;
        std::string path_;
        
    private:
        Character *character_ = nullptr;
        Misc* misc_ = nullptr;
        Alliance* alliance_ = nullptr;
        Map* map_ = nullptr;
        friend class ClientImpl;
    };

}

#endif // FRONTSERVER_H
