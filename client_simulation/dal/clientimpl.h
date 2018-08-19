#ifndef CLIENTIMPL_H
#define CLIENTIMPL_H

#include "../client/connector.h"
#include "function.h"
#include <base/memory/memorypoolmgr.h>
#include <model/protocol.h>
#include <model/metadata.h>
#include <string>
#include <stdint.h>
#include <map>

namespace dal
{
    using namespace std;

    class FrontServer;
    
    typedef std::map<uint16_t, Function<void(client::PacketIn&)> >  handlers_map_t;

    class ClientImpl : public client::ConnectorEventHandler
    {
    public:
        ClientImpl(FrontServer& p) : m_parent(p) {}

        virtual ~ClientImpl() {
        }
        
        // 注册一个处理器
        void RegisterHandler(uint16_t code, Function<void(client::PacketIn&)> handler);
        
        virtual void OnConnect(client::Connector* sender);
        
        virtual void OnClose(client::Connector* sender);

        virtual void OnConnectFail(client::Connector* sender, const char* reason);

        virtual void OnReceivePacket(client::Connector* sender, client::PacketIn& pktin);

    private:
        FrontServer& m_parent;
        bool is_connected_ = false;
        handlers_map_t handlers_;
    };
    
#define HANDLE_MAP(code, memfun)\
impl->RegisterHandler((uint16_t)code, Bind(&memfun, this))

}



#endif // CLIENTIMPL_H
