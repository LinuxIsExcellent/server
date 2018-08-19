#include "clientimpl.h"
#include "frontserver.h"

namespace dal
{
    // 注册一个处理器
    void ClientImpl::RegisterHandler(uint16_t code, Function<void(client::PacketIn&)> handler)
    {
        handlers_.insert(std::make_pair(code, handler));
    }
    
    void ClientImpl::OnConnect(client::Connector* sender)
    {
        is_connected_ = true;
        m_parent.OnConnect();
    }
    
    void ClientImpl::OnClose(client::Connector* sender)
    {
        is_connected_ = false;
        printf("server disconnected!");
    }

    void ClientImpl::OnConnectFail(client::Connector* sender, const char* reason)
    {
        printf("connect fail:%s!", reason);
    }

    void ClientImpl::OnReceivePacket(client::Connector* sender, client::PacketIn& pktin)
    {
        handlers_map_t::iterator it = handlers_.find(pktin.code());
        if (it != handlers_.end()) {
            try {
                it->second(pktin);
            } catch (exception& ex) {
                printf("handle code=%u, catch exception: %s\n", it->first, ex.what());
            }
        } else {
            //printf("unhandled response code=%u\n", pktin.code());
        }
    }

}
