#ifndef PLAYERSESSION_H
#define PLAYERSESSION_H

#include <base/gateway/usersession.h>
#include <base/utils/intrusive_list.h>
#include <base/observer.h>

namespace base
{
    namespace lua
    {
        class LuaVm;
    }
}

struct lua_State;

namespace fs
{
    class LuaMapAgent;
    
    class PlayerSession : public base::gateway::UserSession
    {
        INTRUSIVE_LIST(PlayerSession)
    public:
        PlayerSession(base::gateway::UserClient* client, int msServiceId);
        virtual ~PlayerSession();
        
        virtual const char* GetObjectName() {
            return "fs::PlayerSession";
        }

        bool Setup();
        void Exit();
        void SendLogout();

    private:
        virtual void OnUserClientReceivePacket(base::gateway::PacketIn& pktin) override;
        virtual void OnUserClientClose() override;
        virtual void OnUserSessionSend(base::gateway::PacketOut& pktout) override;
        int m_msServiceId;
        base::lua::LuaVm* m_vm;
        base::AutoObserver m_autoObserver;
        LuaMapAgent* m_luaMapAgent = nullptr;
        bool m_closed = false;

        friend class LuaPlayerSession;
        friend class LuaMapAgent;
    };
}

#endif // PLAYERSESSION_H
