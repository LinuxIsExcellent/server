#include "frontserver.h"
#include "clientimpl.h"
#include "../client/connector.h"
#include "character.h"
#include "map.h"
#include "misc.h"
#include "alliance.h"
#include "iostream"

namespace dal
{
    using namespace std;

    FrontServer::FrontServer(const string& serverip, const int32_t serverport, const std::string& path)
        :  ip_(serverip), port_(serverport), path_(path)
    {
        clientImpl_ = new ClientImpl(*this);
        
        character_ = new Character(clientImpl_, this);
        map_ = new Map(clientImpl_, this);
        misc_ = new Misc(clientImpl_, this);
        alliance_ = new Alliance(clientImpl_, this);
    }

    FrontServer::~FrontServer()
    {
        SAFE_DELETE(clientImpl_);
        SAFE_DELETE(character_);
        SAFE_DELETE(map_);
        SAFE_DELETE(misc_);
        SAFE_DELETE(alliance_);
    }
    
    void FrontServer::OnConnect()
    {
        isConnected_ = true;
    }
    
    void FrontServer::DisConnect()
    {
        if (conn_) {
            conn_->Close();
            isConnected_ = false;
        }
    }

    void FrontServer::PerformIO()
    {
        if (conn_ == nullptr) {
            conn_ = new client::Connector(clientImpl_);
            conn_->Connect(ip_.c_str(), port_);
        } else {
            conn_->PerformIO();
        }
    }
    
    
    
}
