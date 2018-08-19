#ifndef FSCLIENTSESSION_H
#define FSCLIENTSESSION_H
#include <base/gateway/usersession.h>

class lua_State;
namespace lua
{
    class Luaer;
}

namespace ts
{
    class FSClientSession : public base::gateway::UserSession
    {
    public:
        FSClientSession(base::gateway::UserClient* client);
        virtual ~FSClientSession();

    public:
        lua::Luaer* luaer() {
            return luaer_;
        }
        
        virtual void OnUserClientReceivePacket(base::gateway::PacketIn& pktin) override;
        virtual void OnUserClientClose() override;
        
        void SetMetatable(lua_State* L);
        
    private:
        lua::Luaer* luaer_;
    };
}
#endif // FSCLIENTSESSION_H
